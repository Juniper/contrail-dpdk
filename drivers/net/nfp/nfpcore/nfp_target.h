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

#ifndef NFP_BSP_NFP_TARGET_H
#define NFP_BSP_NFP_TARGET_H

#include "nfp-common/nfp_resid.h"
#include "nfp-common/nfp_cppat.h"
#include "nfp-common/nfp_platform.h"
#include "nfp_cpp.h"

#define P32 1
#define P64 2

#define PUSHPULL(_pull, _push) (((_pull) << 4) | ((_push) << 0))

#ifndef NFP_ERRNO
#include <errno.h>
#define NFP_ERRNO(x)    (errno = (x), -1)
#endif

static nfp_inline int pushpull_width(int pp)
{
	pp &= 0xf;

	if (pp == 0)
		return NFP_ERRNO(EINVAL);
	return (2 << pp);
}

#define PUSH_WIDTH(_pushpull)      pushpull_width((_pushpull) >> 0)
#define PULL_WIDTH(_pushpull)      pushpull_width((_pushpull) >> 4)

/* This structure ONLY includes items
 * that can be done with a read or write of
 * 32-bit or 64-bit words. All others are not listed.
 */

#define AT(_action, _token, _pull, _push)    \
	case NFP_CPP_ID(0, _action, _token): \
		return PUSHPULL(_pull, _push);

static nfp_inline int target_rw(uint32_t cpp_id, int pp, int start, int len)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island && (island < start || island > (start + len)))
		return NFP_ERRNO(EINVAL);

	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 0, 0, pp);
		AT(1, 0, pp, 0);
		AT(NFP_CPP_ACTION_RW, 0, pp, pp);
	default:
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp6000_nbi_dma(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 0, 0, P64)	/* ReadNbiDma */
		    AT(1, 0, P64, 0)	/* WriteNbiDma */
		    AT(NFP_CPP_ACTION_RW, 0, P64, P64)
	default :
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp6000_nbi_stats(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 0, 0, P64)	/* ReadNbiStats */
		    AT(1, 0, P64, 0)	/* WriteNbiStats */
		    AT(NFP_CPP_ACTION_RW, 0, P64, P64)
	default :
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp6000_nbi_tm(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 0, 0, P64)	/* ReadNbiTM */
		    AT(1, 0, P64, 0)	/* WriteNbiTM */
		    AT(NFP_CPP_ACTION_RW, 0, P64, P64)
	default :
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp6000_nbi_ppc(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 0, 0, P64)	/* ReadNbiPreclassifier */
		    AT(1, 0, P64, 0)	/* WriteNbiPreclassifier */
		    AT(NFP_CPP_ACTION_RW, 0, P64, P64)
	default :
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp6000_nbi(uint32_t cpp_id, uint64_t address)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);
	uint64_t rel_addr = address & 0x3fFFFF;

	if (island && (island < 8 || island > 9))
		return NFP_ERRNO(EINVAL);

	if (rel_addr < (1 << 20))
		return nfp6000_nbi_dma(cpp_id);
	if (rel_addr < (2 << 20))
		return nfp6000_nbi_stats(cpp_id);
	if (rel_addr < (3 << 20))
		return nfp6000_nbi_tm(cpp_id);
	return nfp6000_nbi_ppc(cpp_id);
}

/* This structure ONLY includes items
 * that can be done with a read or write of
 * 32-bit or 64-bit words. All others are not listed.
 */
static nfp_inline int nfp6000_mu_common(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(NFP_CPP_ACTION_RW, 0, P64, P64)	/* read_be/write_be */
		    AT(NFP_CPP_ACTION_RW, 1, P64, P64)	/* read_le/write_le */
		    AT(NFP_CPP_ACTION_RW, 2, P64, P64)	/* read_swap_be/write_swap_be */
		    AT(NFP_CPP_ACTION_RW, 3, P64, P64)	/* read_swap_le/write_swap_le */
		    AT(0, 0, 0, P64)	/* read_be */
		    AT(0, 1, 0, P64)	/* read_le */
		    AT(0, 2, 0, P64)	/* read_swap_be */
		    AT(0, 3, 0, P64)	/* read_swap_le */
		    AT(1, 0, P64, 0)	/* write_be */
		    AT(1, 1, P64, 0)	/* write_le */
		    AT(1, 2, P64, 0)	/* write_swap_be */
		    AT(1, 3, P64, 0)	/* write_swap_le */
		    AT(3, 0, 0, P32)	/* atomic_read */
		    AT(3, 2, P32, 0)	/* mask_compare_write */
		    AT(4, 0, P32, 0)	/* atomic_write */
		    AT(4, 2, 0, 0)	/* atomic_write_imm */
		    AT(4, 3, 0, P32)	/* swap_imm */
		    AT(5, 0, P32, 0)	/* set */
		    AT(5, 3, 0, P32)	/* test_set_imm */
		    AT(6, 0, P32, 0)	/* clr */
		    AT(6, 3, 0, P32)	/* test_clr_imm */
		    AT(7, 0, P32, 0)	/* add */
		    AT(7, 3, 0, P32)	/* test_add_imm */
		    AT(8, 0, P32, 0)	/* addsat */
		    AT(8, 3, 0, P32)	/* test_subsat_imm */
		    AT(9, 0, P32, 0)	/* sub */
		    AT(9, 3, 0, P32)	/* test_sub_imm */
		    AT(10, 0, P32, 0)	/* subsat */
		    AT(10, 3, 0, P32)	/* test_subsat_imm */
		    AT(13, 0, 0, P32)	/* microq128_get */
		    AT(13, 1, 0, P32)	/* microq128_pop */
		    AT(13, 2, P32, 0)	/* microq128_put */
		    AT(15, 0, P32, 0)	/* xor */
		    AT(15, 3, 0, P32)	/* test_xor_imm */
		    AT(28, 0, 0, P32)	/* read32_be */
		    AT(28, 1, 0, P32)	/* read32_le */
		    AT(28, 2, 0, P32)	/* read32_swap_be */
		    AT(28, 3, 0, P32)	/* read32_swap_le */
		    AT(31, 0, P32, 0)	/* write32_be */
		    AT(31, 1, P32, 0)	/* write32_le */
		    AT(31, 2, P32, 0)	/* write32_swap_be */
		    AT(31, 3, P32, 0)	/* write32_swap_le */
	default :
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp6000_mu_ctm(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(16, 1, 0, P32)	/* packet_read_packet_status */
	default :
		return nfp6000_mu_common(cpp_id);
	}
}

static nfp_inline int nfp6000_mu_emu(uint32_t cpp_id)
{
	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(18, 0, 0, P32)	/* read_queue */
		    AT(18, 1, 0, P32)	/* read_queue_ring */
		    AT(18, 2, P32, 0)	/* write_queue */
		    AT(18, 3, P32, 0)	/* write_queue_ring */
		    AT(20, 2, P32, 0)	/* journal */
		    AT(21, 0, 0, P32)	/* get */
		    AT(21, 1, 0, P32)	/* get_eop */
		    AT(21, 2, 0, P32)	/* get_freely */
		    AT(22, 0, 0, P32)	/* pop */
		    AT(22, 1, 0, P32)	/* pop_eop */
		    AT(22, 2, 0, P32)	/* pop_freely */
	default :
		return nfp6000_mu_common(cpp_id);
	}
}

static nfp_inline int nfp6000_mu_imu(uint32_t cpp_id)
{
	return nfp6000_mu_common(cpp_id);
}

static nfp_inline int nfp6000_mu(uint32_t cpp_id, uint64_t address)
{
	int pp;
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island == 0) {
		if (address < 0x2000000000ULL)
			pp = nfp6000_mu_ctm(cpp_id);
		else if (address < 0x8000000000ULL)
			pp = nfp6000_mu_emu(cpp_id);
		else if (address < 0x9800000000ULL)
			pp = nfp6000_mu_ctm(cpp_id);
		else if (address < 0x9C00000000ULL)
			pp = nfp6000_mu_emu(cpp_id);
		else if (address < 0xA000000000ULL)
			pp = nfp6000_mu_imu(cpp_id);
		else
			pp = nfp6000_mu_ctm(cpp_id);
	} else if (island >= 24 && island <= 27) {
		pp = nfp6000_mu_emu(cpp_id);
	} else if (island >= 28 && island <= 31) {
		pp = nfp6000_mu_imu(cpp_id);
	} else if (island == 1 ||
		   (island >= 4 && island <= 7) ||
		   (island >= 12 && island <= 13) ||
		   (island >= 32 && island <= 47) ||
		   (island >= 48 && island <= 51)) {
		pp = nfp6000_mu_ctm(cpp_id);
	} else {
		pp = NFP_ERRNO(EINVAL);
	}

	return pp;
}

static nfp_inline int nfp6000_ila(uint32_t cpp_id)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island && (island < 48 || island > 51))
		return NFP_ERRNO(EINVAL);

	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 1, 0, P32);	/* read_check_error */
		AT(2, 0, 0, P32);	/* read_int */
		AT(3, 0, P32, 0);	/* write_int */
	default:
		return target_rw(cpp_id, P32, 48, 4);
	}
}

static nfp_inline int nfp6000_pci(uint32_t cpp_id)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island && (island < 4 || island > 7))
		return NFP_ERRNO(EINVAL);

	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(2, 0, 0, P32);
		AT(3, 0, P32, 0);
	default:
		return target_rw(cpp_id, P32, 4, 4);
	}
}

static nfp_inline int nfp6000_crypto(uint32_t cpp_id)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island && (island < 12 || island > 15))
		return NFP_ERRNO(EINVAL);

	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(2, 0, P64, 0);
	default:
		return target_rw(cpp_id, P64, 12, 4);
	}
}

static nfp_inline int nfp6000_cap_xpb(uint32_t cpp_id)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island && (island < 1 || island > 63))
		return NFP_ERRNO(EINVAL);

	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 1, 0, P32);	/* RingGet */
		AT(0, 2, P32, 0);	/* Interthread Signal */
		AT(1, 1, P32, 0);	/* RingPut */
		AT(1, 2, P32, 0);	/* CTNNWr */
		AT(2, 0, 0, P32);	/* ReflectRd, signal none */
		AT(2, 1, 0, P32);	/* ReflectRd, signal self */
		AT(2, 2, 0, P32);	/* ReflectRd, signal remote */
		AT(2, 3, 0, P32);	/* ReflectRd, signal both */
		AT(3, 0, P32, 0);	/* ReflectWr, signal none */
		AT(3, 1, P32, 0);	/* ReflectWr, signal self */
		AT(3, 2, P32, 0);	/* ReflectWr, signal remote */
		AT(3, 3, P32, 0);	/* ReflectWr, signal both */
		AT(NFP_CPP_ACTION_RW, 1, P32, P32);
	default:
		return target_rw(cpp_id, P32, 1, 63);
	}
}

static nfp_inline int nfp6000_cls(uint32_t cpp_id)
{
	int island = NFP_CPP_ID_ISLAND_of(cpp_id);

	if (island && (island < 1 || island > 63))
		return NFP_ERRNO(EINVAL);

	switch (cpp_id & NFP_CPP_ID(0, ~0, ~0)) {
		AT(0, 3, P32, 0);	/* xor */
		AT(2, 0, P32, 0);	/* set */
		AT(2, 1, P32, 0);	/* clr */
		AT(4, 0, P32, 0);	/* add */
		AT(4, 1, P32, 0);	/* add64 */
		AT(6, 0, P32, 0);	/* sub */
		AT(6, 1, P32, 0);	/* sub64 */
		AT(6, 2, P32, 0);	/* subsat */
		AT(8, 2, P32, 0);	/* hash_mask */
		AT(8, 3, P32, 0);	/* hash_clear */
		AT(9, 0, 0, P32);	/* ring_get */
		AT(9, 1, 0, P32);	/* ring_pop */
		AT(9, 2, 0, P32);	/* ring_get_freely */
		AT(9, 3, 0, P32);	/* ring_pop_freely */
		AT(10, 0, P32, 0);	/* ring_put */
		AT(10, 2, P32, 0);	/* ring_journal */
		AT(14, 0, P32, 0);	/* reflect_write_sig_local */
		AT(15, 1, 0, P32);	/* reflect_read_sig_local */
		AT(17, 2, P32, 0);	/* statisic */
		AT(24, 0, 0, P32);	/* ring_read */
		AT(24, 1, P32, 0);	/* ring_write */
		AT(25, 0, 0, P32);	/* ring_workq_add_thread */
		AT(25, 1, P32, 0);	/* ring_workq_add_work */
	default:
		return target_rw(cpp_id, P32, 0, 64);
	}
}

static nfp_inline int nfp6000_target_pushpull(uint32_t cpp_id, uint64_t address)
{
	switch (NFP_CPP_ID_TARGET_of(cpp_id)) {
	case NFP6000_CPPTGT_NBI:
		return nfp6000_nbi(cpp_id, address);
	case NFP6000_CPPTGT_VQDR:
		return target_rw(cpp_id, P32, 24, 4);
	case NFP6000_CPPTGT_ILA:
		return nfp6000_ila(cpp_id);
	case NFP6000_CPPTGT_MU:
		return nfp6000_mu(cpp_id, address);
	case NFP6000_CPPTGT_PCIE:
		return nfp6000_pci(cpp_id);
	case NFP6000_CPPTGT_ARM:
		if (address < 0x10000)
			return target_rw(cpp_id, P64, 1, 1);
		else
			return target_rw(cpp_id, P32, 1, 1);
	case NFP6000_CPPTGT_CRYPTO:
		return nfp6000_crypto(cpp_id);
	case NFP6000_CPPTGT_CTXPB:
		return nfp6000_cap_xpb(cpp_id);
	case NFP6000_CPPTGT_CLS:
		return nfp6000_cls(cpp_id);
	case 0:
		return target_rw(cpp_id, P32, 4, 4);
	default:
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp_target_pushpull_width(int pp, int write_not_read)
{
	if (pp < 0)
		return pp;

	if (write_not_read)
		return PULL_WIDTH(pp);
	else
		return PUSH_WIDTH(pp);
}

static nfp_inline int nfp6000_target_action_width(uint32_t cpp_id, uint64_t address,
						  int write_not_read)
{
	int pp;

	pp = nfp6000_target_pushpull(cpp_id, address);

	return nfp_target_pushpull_width(pp, write_not_read);
}

static nfp_inline int nfp_target_action_width(uint32_t model, uint32_t cpp_id,
					      uint64_t address, int write_not_read)
{
	if (NFP_CPP_MODEL_IS_6000(model)) {
		return nfp6000_target_action_width(cpp_id, address,
						   write_not_read);
	} else {
		return NFP_ERRNO(EINVAL);
	}
}

static nfp_inline int nfp_target_cpp(uint32_t cpp_island_id, uint64_t cpp_island_address,
				     uint32_t *cpp_target_id,
				     uint64_t *cpp_target_address,
				     const uint32_t *imb_table)
{
	int err;
	int island = NFP_CPP_ID_ISLAND_of(cpp_island_id);
	int target = NFP_CPP_ID_TARGET_of(cpp_island_id);
	uint32_t imb;

	if (target < 0 || target >= 16)
		return NFP_ERRNO(EINVAL);

	if (island == 0) {
		/* Already translated */
		*cpp_target_id = cpp_island_id;
		*cpp_target_address = cpp_island_address;
		return 0;
	}

	if (!imb_table) {
		/* CPP + Island only allowed on systems with IMB tables */
		return NFP_ERRNO(EINVAL);
	}

	imb = imb_table[target];

	*cpp_target_address = cpp_island_address;
	err = _nfp6000_cppat_addr_encode(cpp_target_address, island, target,
					 ((imb >> 13) & 7),
					 ((imb >> 12) & 1),
					 ((imb >> 6) & 0x3f),
					 ((imb >> 0) & 0x3f));
	if (err == 0) {
		*cpp_target_id =
		    NFP_CPP_ID(target, NFP_CPP_ID_ACTION_of(cpp_island_id),
			       NFP_CPP_ID_TOKEN_of(cpp_island_id));
	}

	return err;
}

#endif /* NFP_BSP_NFP_TARGET_H */
/* vim: set shiftwidth=4 expandtab:  */
