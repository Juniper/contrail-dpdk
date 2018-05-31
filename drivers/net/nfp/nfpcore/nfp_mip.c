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
 * nfp_mip.c
 * Authors: Jakub Kicinski <jakub.kicinski@netronome.com>
 *          Jason McMullan <jason.mcmullan@netronome.com>
 *          Espen Skoglund <espen.skoglund@netronome.com>
 */
#include "libnfp_cpp.h"
#include "nfp_cpp.h"
#include "nfp_nffw.h"

#define cpu_to_le32
#define le32_to_cpu
#define NFP_MIP_SIGNATURE	cpu_to_le32(0x0050494d)  /* "MIP\0" */
#define NFP_MIP_VERSION		cpu_to_le32(1)
#define NFP_MIP_MAX_OFFSET	(256 * 1024)

#define __le32 uint32_t
struct nfp_mip {
	__le32 signature;
	__le32 mip_version;
	__le32 mip_size;
	__le32 first_entry;

	__le32 version;
	__le32 buildnum;
	__le32 buildtime;
	__le32 loadtime;

	__le32 symtab_addr;
	__le32 symtab_size;
	__le32 strtab_addr;
	__le32 strtab_size;

	char name[16];
	char toolchain[32];
};

struct nfp_nffw_info *nfp_nffw_info_open(struct nfp_cpp *cpp);
int nfp_nffw_info_mip_first(struct nfp_nffw_info *state, uint32_t *cpp_id, uint64_t *off);
void nfp_nffw_info_close(struct nfp_nffw_info *state);

/* Read memory and check if it could be a valid MIP */
static int
nfp_mip_try_read(struct nfp_cpp *cpp, uint32_t cpp_id, uint64_t addr, struct nfp_mip *mip)
{
	int ret;

	ret = nfp_cpp_read(cpp, cpp_id, addr, mip, sizeof(*mip));
	if (ret != sizeof(*mip)) {
		printf("Failed to read MIP data (%d, %zu)\n",
			ret, sizeof(*mip));
		return -EIO;
	}
	if (mip->signature != NFP_MIP_SIGNATURE) {
		printf("Incorrect MIP signature (0x%08x)\n",
			 le32_to_cpu(mip->signature));
		return -EINVAL;
	}
	if (mip->mip_version != NFP_MIP_VERSION) {
		printf("Unsupported MIP version (%d)\n",
			 le32_to_cpu(mip->mip_version));
		return -EINVAL;
	}

	return 0;
}

/* Try to locate MIP using the resource table */
static int nfp_mip_read_resource(struct nfp_cpp *cpp, struct nfp_mip *mip)
{
	struct nfp_nffw_info *nffw_info;
	uint32_t cpp_id;
	uint64_t addr;
	int err;

	nffw_info = nfp_nffw_info_open(cpp);
	if (!nffw_info)
		return -ENODEV;

	err = nfp_nffw_info_mip_first(nffw_info, &cpp_id, &addr);
	if (err)
		goto exit_close_nffw;

	err = nfp_mip_try_read(cpp, cpp_id, addr, mip);
exit_close_nffw:
	nfp_nffw_info_close(nffw_info);
	return err;
}

/**
 * nfp_mip_open() - Get device MIP structure
 * @cpp:	NFP CPP Handle
 *
 * Copy MIP structure from NFP device and return it.  The returned
 * structure is handled internally by the library and should be
 * freed by calling nfp_mip_close().
 *
 * Return: pointer to mip, NULL on failure.
 */
struct nfp_mip *nfp_mip_open(struct nfp_cpp *cpp)
{
	struct nfp_mip *mip;
	int err;

	mip = malloc(sizeof(*mip));
	if (!mip)
		return NULL;

	err = nfp_mip_read_resource(cpp, mip);
	if (err) {
		free(mip);
		return NULL;
	}

	mip->name[sizeof(mip->name) - 1] = 0;

	return mip;
}

void nfp_mip_close(struct nfp_mip *mip)
{
	free(mip);
}

const char *nfp_mip_name(const struct nfp_mip *mip)
{
	return mip->name;
}

/**
 * nfp_mip_symtab() - Get the address and size of the MIP symbol table
 * @mip:	MIP handle
 * @addr:	Location for NFP DDR address of MIP symbol table
 * @size:	Location for size of MIP symbol table
 */
void nfp_mip_symtab(const struct nfp_mip *mip, uint32_t *addr, uint32_t *size)
{
	*addr = le32_to_cpu(mip->symtab_addr);
	*size = le32_to_cpu(mip->symtab_size);
}

/**
 * nfp_mip_strtab() - Get the address and size of the MIP symbol name table
 * @mip:	MIP handle
 * @addr:	Location for NFP DDR address of MIP symbol name table
 * @size:	Location for size of MIP symbol name table
 */
void nfp_mip_strtab(const struct nfp_mip *mip, uint32_t *addr, uint32_t *size)
{
	*addr = le32_to_cpu(mip->strtab_addr);
	*size = le32_to_cpu(mip->strtab_size);
}
