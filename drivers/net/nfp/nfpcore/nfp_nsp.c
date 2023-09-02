/*
 * Copyright (c) 2018 Netronome Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * nfp_nsp.c
 * Author: Jakub Kicinski <jakub.kicinski@netronome.com>
 *         Jason McMullan <jason.mcmullan@netronome.com>
 */

#define NFP_SUBSYS "nfp_nsp"

#include <time.h>

#include "libnfp_cpp.h"
#include "nfp_cpp.h"
#include "nfp_nsp.h"
#include "nfp_nffw.h"
#include "nfp_resource.h"

#define GENMASK_ULL(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & \
	 (~0ULL >> (64 - 1 - (h))))

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define FIELD_GET(_mask, _reg)						\
	({								\
		(typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask));	\
	})

#define FIELD_FIT(_mask, _val)						\
	({								\
		!((((typeof(_mask))_val) << __bf_shf(_mask)) & ~(_mask)); \
	})

#define FIELD_PREP(_mask, _val)						\
	({								\
		((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask);	\
	})

/* Offsets relative to the CSR base */
#define NSP_STATUS		0x00
#define   NSP_STATUS_MAGIC	GENMASK_ULL(63, 48)
#define   NSP_STATUS_MAJOR	GENMASK_ULL(47, 44)
#define   NSP_STATUS_MINOR	GENMASK_ULL(43, 32)
#define   NSP_STATUS_CODE	GENMASK_ULL(31, 16)
#define   NSP_STATUS_RESULT	GENMASK_ULL(15, 8)
#define   NSP_STATUS_BUSY	BIT_ULL(0)

#define NSP_COMMAND		0x08
#define   NSP_COMMAND_OPTION	GENMASK_ULL(63, 32)
#define   NSP_COMMAND_CODE	GENMASK_ULL(31, 16)
#define   NSP_COMMAND_START	BIT_ULL(0)

/* CPP address to retrieve the data from */
#define NSP_BUFFER		0x10
#define   NSP_BUFFER_CPP	GENMASK_ULL(63, 40)
#define   NSP_BUFFER_PCIE	GENMASK_ULL(39, 38)
#define   NSP_BUFFER_ADDRESS	GENMASK_ULL(37, 0)

#define NSP_DFLT_BUFFER		0x18

#define NSP_DFLT_BUFFER_CONFIG	0x20
#define   NSP_DFLT_BUFFER_SIZE_MB	GENMASK_ULL(7, 0)

#define NSP_MAGIC		0xab10
#define NSP_MAJOR		0
#define NSP_MINOR		8

#define NSP_CODE_MAJOR		GENMASK(15, 12)
#define NSP_CODE_MINOR		GENMASK(11, 0)

enum nfp_nsp_cmd {
	SPCODE_NOOP		= 0, /* No operation */
	SPCODE_SOFT_RESET	= 1, /* Soft reset the NFP */
	SPCODE_FW_DEFAULT	= 2, /* Load default (UNDI) FW */
	SPCODE_PHY_INIT		= 3, /* Initialize the PHY */
	SPCODE_MAC_INIT		= 4, /* Initialize the MAC */
	SPCODE_PHY_RXADAPT	= 5, /* Re-run PHY RX Adaptation */
	SPCODE_FW_LOAD		= 6, /* Load fw from buffer, len in option */
	SPCODE_ETH_RESCAN	= 7, /* Rescan ETHs, write ETH_TABLE to buf */
	SPCODE_ETH_CONTROL	= 8, /* Update media config from buffer */
	SPCODE_NSP_SENSORS	= 12, /* Read NSP sensor(s) */
	SPCODE_NSP_IDENTIFY	= 13, /* Read NSP version */
};

static const struct {
	int code;
	const char *msg;
} nsp_errors[] = {
	{ 6010, "could not map to phy for port" },
	{ 6011, "not an allowed rate/lanes for port" },
	{ 6012, "not an allowed rate/lanes for port" },
	{ 6013, "high/low error, change other port first" },
	{ 6014, "config not found in flash" },
};

struct nfp_nsp {
	struct nfp_cpp *cpp;
	struct nfp_resource *res;
	struct {
		uint16_t major;
		uint16_t minor;
	} ver;

	/* Eth table config state */
	int modified;
	unsigned int idx;
	void *entries;
};

int nfp_nsp_config_modified(struct nfp_nsp *state)
{
	return state->modified;
}

void nfp_nsp_config_set_modified(struct nfp_nsp *state, int modified)
{
	state->modified = modified;
}

void *nfp_nsp_config_entries(struct nfp_nsp *state)
{
	return state->entries;
}

unsigned int nfp_nsp_config_idx(struct nfp_nsp *state)
{
	return state->idx;
}

void
nfp_nsp_config_set_state(struct nfp_nsp *state, void *entries, unsigned int idx)
{
	state->entries = entries;
	state->idx = idx;
}

void nfp_nsp_config_clear_state(struct nfp_nsp *state)
{
	state->entries = NULL;
	state->idx = 0;
}

static void nfp_nsp_print_extended_error(uint32_t ret_val)
{
	int i;

	if (!ret_val)
		return;

	for (i = 0; i < (int)ARRAY_SIZE(nsp_errors); i++)
		if (ret_val == (uint32_t)nsp_errors[i].code)
			printf("err msg: %s\n", nsp_errors[i].msg);
}

static int nfp_nsp_check(struct nfp_nsp *state)
{
	struct nfp_cpp *cpp = state->cpp;
	uint64_t nsp_status, reg;
	uint32_t nsp_cpp;
	int err;

	nsp_cpp = nfp_resource_cpp_id(state->res);
	nsp_status = nfp_resource_address(state->res) + NSP_STATUS;

	err = nfp_cpp_readq(cpp, nsp_cpp, nsp_status, &reg);
	if (err < 0)
		return err;

	if (FIELD_GET(NSP_STATUS_MAGIC, reg) != NSP_MAGIC) {
		printf("Cannot detect NFP Service Processor\n");
		return -ENODEV;
	}

	state->ver.major = FIELD_GET(NSP_STATUS_MAJOR, reg);
	state->ver.minor = FIELD_GET(NSP_STATUS_MINOR, reg);

	if (state->ver.major != NSP_MAJOR || state->ver.minor < NSP_MINOR) {
		printf("Unsupported ABI %hu.%hu\n",
			state->ver.major, state->ver.minor);
		return -EINVAL;
	}

	if (reg & NSP_STATUS_BUSY) {
		printf("Service processor busy!\n");
		return -EBUSY;
	}

	return 0;
}

/**
 * nfp_nsp_open() - Prepare for communication and lock the NSP resource.
 * @cpp:	NFP CPP Handle
 */
struct nfp_nsp *nfp_nsp_open(struct nfp_cpp *cpp)
{
	struct nfp_resource *res;
	struct nfp_nsp *state;
	int err;

	res = nfp_resource_acquire(cpp, NFP_RESOURCE_NSP);
	if (!res)
		return NULL;

	state = malloc(sizeof(*state));
	if (!state) {
		nfp_resource_release(res);
		return NULL;
	}
	memset(state, 0, sizeof(*state));
	state->cpp = cpp;
	state->res = res;

	err = nfp_nsp_check(state);
	if (err) {
		nfp_nsp_close(state);
		return NULL;
	}

	return state;
}

/**
 * nfp_nsp_close() - Clean up and unlock the NSP resource.
 * @state:	NFP SP state
 */
void nfp_nsp_close(struct nfp_nsp *state)
{
	nfp_resource_release(state->res);
	free(state);
}

uint16_t nfp_nsp_get_abi_ver_major(struct nfp_nsp *state)
{
	return state->ver.major;
}

uint16_t nfp_nsp_get_abi_ver_minor(struct nfp_nsp *state)
{
	return state->ver.minor;
}

static int
nfp_nsp_wait_reg(struct nfp_cpp *cpp, uint64_t *reg,
		 uint32_t nsp_cpp, uint64_t addr, uint64_t mask, uint64_t val)
{
	struct timespec wait; 
	int count;
	int err;

	wait.tv_sec = 0;
	wait.tv_nsec = 25000000; 
	count = 0;

	for (;;) {

		err = nfp_cpp_readq(cpp, nsp_cpp, addr, reg);
		if (err < 0)
			return err;

		if ((*reg & mask) == val)
			return 0;

		nanosleep(&wait, 0);
		if (count++ > 1000)
			return -ETIMEDOUT;
	}
}

/**
 * nfp_nsp_command() - Execute a command on the NFP Service Processor
 * @state:	NFP SP state
 * @code:	NFP SP Command Code
 * @option:	NFP SP Command Argument
 * @buff_cpp:	NFP SP Buffer CPP Address info
 * @buff_addr:	NFP SP Buffer Host address
 *
 * Return: 0 for success with no result
 *
 *	 positive value for NSP completion with a result code
 *
 *	-EAGAIN if the NSP is not yet present
 *	-ENODEV if the NSP is not a supported model
 *	-EBUSY if the NSP is stuck
 *	-EINTR if interrupted while waiting for completion
 *	-ETIMEDOUT if the NSP took longer than 30 seconds to complete
 */
static int nfp_nsp_command(struct nfp_nsp *state, uint16_t code, uint32_t option,
			   uint32_t buff_cpp, uint64_t buff_addr)
{
	uint64_t reg, ret_val, nsp_base, nsp_buffer, nsp_status, nsp_command;
	struct nfp_cpp *cpp = state->cpp;
	uint32_t nsp_cpp;
	int err;

	nsp_cpp = nfp_resource_cpp_id(state->res);
	nsp_base = nfp_resource_address(state->res);
	nsp_status = nsp_base + NSP_STATUS;
	nsp_command = nsp_base + NSP_COMMAND;
	nsp_buffer = nsp_base + NSP_BUFFER;

	err = nfp_nsp_check(state);
	if (err)
		return err;

	if (!FIELD_FIT(NSP_BUFFER_CPP, buff_cpp >> 8) ||
	    !FIELD_FIT(NSP_BUFFER_ADDRESS, buff_addr)) {
		printf("Host buffer out of reach %08x %"PRIx64"\n",
			buff_cpp, buff_addr);
		return -EINVAL;
	}

	err = nfp_cpp_writeq(cpp, nsp_cpp, nsp_buffer,
			     FIELD_PREP(NSP_BUFFER_CPP, buff_cpp >> 8) |
			     FIELD_PREP(NSP_BUFFER_ADDRESS, buff_addr));
	if (err < 0)
		return err;

	err = nfp_cpp_writeq(cpp, nsp_cpp, nsp_command,
			     FIELD_PREP(NSP_COMMAND_OPTION, option) |
			     FIELD_PREP(NSP_COMMAND_CODE, code) |
			     FIELD_PREP(NSP_COMMAND_START, 1));
	if (err < 0)
		return err;

	/* Wait for NSP_COMMAND_START to go to 0 */
	err = nfp_nsp_wait_reg(cpp, &reg,
			       nsp_cpp, nsp_command, NSP_COMMAND_START, 0);
	if (err) {
		printf("Error %d waiting for code 0x%04x to start\n",
			err, code);
		return err;
	}

	/* Wait for NSP_STATUS_BUSY to go to 0 */
	err = nfp_nsp_wait_reg(cpp, &reg,
			       nsp_cpp, nsp_status, NSP_STATUS_BUSY, 0);
	if (err) {
		printf("Error %d waiting for code 0x%04x to complete\n",
			err, code);
		return err;
	}

	err = nfp_cpp_readq(cpp, nsp_cpp, nsp_command, &ret_val);
	if (err < 0)
		return err;
	ret_val = FIELD_GET(NSP_COMMAND_OPTION, ret_val);

	err = FIELD_GET(NSP_STATUS_RESULT, reg);
	if (err) {
		printf("Result (error) code set: %d (%d) command: %d\n",
			 -err, (int)ret_val, code);
		nfp_nsp_print_extended_error(ret_val);
		return -err;
	}

	return ret_val;
}

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define SZ_1M 0x00100000

static int nfp_nsp_command_buf(struct nfp_nsp *nsp, uint16_t code, uint32_t option,
			       const void *in_buf, unsigned int in_size,
			       void *out_buf, unsigned int out_size)
{
	struct nfp_cpp *cpp = nsp->cpp;
	unsigned int max_size;
	uint64_t reg, cpp_buf;
	int ret, err;
	uint32_t cpp_id;

	if (nsp->ver.minor < 13) {
		printf("NSP: Code 0x%04x with buffer not supported (ABI %hu.%hu)\n",
			code, nsp->ver.major, nsp->ver.minor);
		return -EOPNOTSUPP;
	}

	err = nfp_cpp_readq(cpp, nfp_resource_cpp_id(nsp->res),
			    nfp_resource_address(nsp->res) +
			    NSP_DFLT_BUFFER_CONFIG,
			    &reg);
	if (err < 0)
		return err;

	max_size = max(in_size, out_size);
	if (FIELD_GET(NSP_DFLT_BUFFER_SIZE_MB, reg) * SZ_1M < max_size) {
		printf("NSP: default buffer too small for command 0x%04x (%llu < %u)\n",
			code, FIELD_GET(NSP_DFLT_BUFFER_SIZE_MB, reg) * SZ_1M,
			max_size);
		return -EINVAL;
	}

	err = nfp_cpp_readq(cpp, nfp_resource_cpp_id(nsp->res),
			    nfp_resource_address(nsp->res) +
			    NSP_DFLT_BUFFER,
			    &reg);
	if (err < 0)
		return err;

	cpp_id = FIELD_GET(NSP_BUFFER_CPP, reg) << 8;
	cpp_buf = FIELD_GET(NSP_BUFFER_ADDRESS, reg);

	if (in_buf && in_size) {
		err = nfp_cpp_write(cpp, cpp_id, cpp_buf, in_buf, in_size);
		if (err < 0)
			return err;
	}
	/* Zero out remaining part of the buffer */
	if (out_buf && out_size && out_size > in_size) {
		memset(out_buf, 0, out_size - in_size);
		err = nfp_cpp_write(cpp, cpp_id, cpp_buf + in_size,
				    out_buf, out_size - in_size);
		if (err < 0)
			return err;
	}

	ret = nfp_nsp_command(nsp, code, option, cpp_id, cpp_buf);
	if (ret < 0)
		return ret;

	if (out_buf && out_size) {
		err = nfp_cpp_read(cpp, cpp_id, cpp_buf, out_buf, out_size);
		if (err < 0)
			return err;
	}

	return ret;
}

int nfp_nsp_wait(struct nfp_nsp *state)
{
	struct timespec wait;
	int count;
	int err;

	//nfp_dbg(state->cpp, "Waiting for NSP to respond (30 sec max).\n");

	wait.tv_sec = 0;
	wait.tv_nsec = 25000000;
	count = 0;

	for (;;) {

		err = nfp_nsp_command(state, SPCODE_NOOP, 0, 0, 0);
		if (err != -EAGAIN)
			break;

		nanosleep(&wait, 0);

		if (count++ > 1000) {
			err = -ETIMEDOUT;
			break;
		}
	}
	if (err)
		printf("NSP failed to respond %d\n", err);

	return err;
}

int nfp_nsp_device_soft_reset(struct nfp_nsp *state)
{
	return nfp_nsp_command(state, SPCODE_SOFT_RESET, 0, 0, 0);
}

int nfp_nsp_mac_reinit(struct nfp_nsp *state)
{
	return nfp_nsp_command(state, SPCODE_MAC_INIT, 0, 0, 0);
}

int nfp_nsp_load_fw(struct nfp_nsp *state, void *buf, unsigned int size)
{
	return nfp_nsp_command_buf(state, SPCODE_FW_LOAD, size, buf,
				   size, NULL, 0);
}

int nfp_nsp_read_eth_table(struct nfp_nsp *state, void *buf, unsigned int size)
{
	return nfp_nsp_command_buf(state, SPCODE_ETH_RESCAN, size, NULL, 0,
				   buf, size);
}

int nfp_nsp_write_eth_table(struct nfp_nsp *state,
			    const void *buf, unsigned int size)
{
	return nfp_nsp_command_buf(state, SPCODE_ETH_CONTROL, size, buf, size,
				   NULL, 0);
}

int nfp_nsp_read_identify(struct nfp_nsp *state, void *buf, unsigned int size)
{
	return nfp_nsp_command_buf(state, SPCODE_NSP_IDENTIFY, size, NULL, 0,
				   buf, size);
}

int nfp_nsp_read_sensors(struct nfp_nsp *state, unsigned int sensor_mask,
			 void *buf, unsigned int size)
{
	return nfp_nsp_command_buf(state, SPCODE_NSP_SENSORS, sensor_mask,
				   NULL, 0, buf, size);
}