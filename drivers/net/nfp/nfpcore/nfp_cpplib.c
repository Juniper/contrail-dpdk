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

#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include <sys/types.h>

#include "nfp_cpp.h"
#include "nfp_cpp_imp.h"

#define NFP6000_LONGNAMES 1

#include "nfp6000/nfp6000.h"
#include "nfp6000/nfp_xpb.h"

#include "libnfp_cpp.h"
#include "nfp_nffw.h"

/* NFP6000 PL */
#define NFP_PL_DEVICE_ID                        0x00000004
#define NFP_PL_DEVICE_ID_MASK                   0xff

#define NFP6000_ARM_GCSR_SOFTMODEL0             0x00400144
#define BUG_ON(x)   assert(!(x))

/*
 * Modify bits of a 32-bit value from the XPB bus
 *
 * @param cpp           NFP CPP device handle
 * @param xpb_tgt       XPB target and address
 * @param mask          mask of bits to alter
 * @param value         value to modify
 *
 * KERNEL: This operation is safe to call in interrupt or softirq context.
 *
 * @return 0 on success, or -1 on failure (and set errno accordingly).
 */
int nfp_xpb_writelm(struct nfp_cpp *cpp, uint32_t xpb_tgt, uint32_t mask, uint32_t value)
{
	int err;
	uint32_t tmp;

	err = nfp_xpb_readl(cpp, xpb_tgt, &tmp);
	if (err < 0)
		return err;

	tmp &= ~mask;
	tmp |= (mask & value);
	return nfp_xpb_writel(cpp, xpb_tgt, tmp);
}

/*
 * Modify bits of a 32-bit value from the XPB bus
 *
 * @param cpp           NFP CPP device handle
 * @param xpb_tgt       XPB target and address
 * @param mask          mask of bits to alter
 * @param value         value to monitor for
 * @param timeout_us    maximum number of us to wait (-1 for forever)
 *
 * @return >= 0 on success, or -1 on failure (and set errno accordingly).
 */
int nfp_xpb_waitlm(struct nfp_cpp *cpp, uint32_t xpb_tgt, uint32_t mask,
		   uint32_t value, int timeout_us)
{
	uint32_t tmp;
	int err;

	do {
		err = nfp_xpb_readl(cpp, xpb_tgt, &tmp);
		if (err < 0)
			goto exit;

		if ((tmp & mask) == (value & mask)) {
			if (timeout_us < 0)
				timeout_us = 0;
			break;
		}

		if (timeout_us < 0)
			continue;

		timeout_us -= 100;
		usleep(100);
	} while (timeout_us >= 0);

	if (timeout_us < 0) {
		err = NFP_ERRNO(ETIMEDOUT);
	} else {
		err = timeout_us;
	}

exit:
	return err;
}

/*
 * nfp_cpp_read - read from CPP target
 * @cpp:        CPP handle
 * @destination:    CPP id
 * @address:        offset into CPP target
 * @kernel_vaddr:   kernel buffer for result
 * @length:     number of bytes to read
 *
 */
int nfp_cpp_read(struct nfp_cpp *cpp, uint32_t destination,
		 unsigned long long address, void *kernel_vaddr, size_t length)
{
	struct nfp_cpp_area *area;
	int err;

	area = nfp_cpp_area_alloc_acquire(cpp, destination, address, length);
	if (!area) {
		printf("Area allocation/acquire failed\n");
		return -1;
	}

	err = nfp_cpp_area_read(area, 0, kernel_vaddr, length);

	nfp_cpp_area_release_free(area);
	return err;
}

/*
 * nfp_cpp_write - write to CPP target
 * @cpp:        CPP handle
 * @destination:    CPP id
 * @address:        offset into CPP target
 * @kernel_vaddr:   kernel buffer to read from
 * @length:     number of bytes to write
 *
 */
int nfp_cpp_write(struct nfp_cpp *cpp, uint32_t destination,
		  unsigned long long address,
		  const void *kernel_vaddr, size_t length)
{
	struct nfp_cpp_area *area;
	int err;

	area = nfp_cpp_area_alloc_acquire(cpp, destination, address, length);
	if (!area)
		return -1;

	err = nfp_cpp_area_write(area, 0, kernel_vaddr, length);

	nfp_cpp_area_release_free(area);
	return err;
}

/*
 * nfp_cpp_area_fill - fill a CPP area with a value
 * @area:       CPP area
 * @offset:     offset into CPP area
 * @value:      value to fill with
 * @length:     length of area to fill
 */
int nfp_cpp_area_fill(struct nfp_cpp_area *area,
		      unsigned long offset, uint32_t value, size_t length)
{
	int err;
	size_t i;
	uint64_t value64;

	value = NFP_HTOLE32(value);
	value64 = ((uint64_t)value << 32) | value;

	if ((offset + length) > area->size)
		return NFP_ERRNO(EINVAL);

	if ((area->offset + offset) & 3)
		return NFP_ERRNO(EINVAL);

	if (((area->offset + offset) & 7) == 4 && length >= 4) {
		err = nfp_cpp_area_write(area, offset, &value, sizeof(value));
		if (err < 0)
			return err;
		if (err != sizeof(value))
			return NFP_ERRNO(ENOSPC);
		offset += sizeof(value);
		length -= sizeof(value);
	}

	for (i = 0; (i + sizeof(value)) < length; i += sizeof(value64)) {
		err =
		    nfp_cpp_area_write(area, offset + i, &value64,
				       sizeof(value64));
		if (err < 0)
			return err;
		if (err != sizeof(value64))
			return NFP_ERRNO(ENOSPC);
	}

	if ((i + sizeof(value)) <= length) {
		err =
		    nfp_cpp_area_write(area, offset + i, &value, sizeof(value));
		if (err < 0)
			return err;
		if (err != sizeof(value))
			return NFP_ERRNO(ENOSPC);
		i += sizeof(value);
	}

	return (int)i;
}

static nfp_inline uint8_t __nfp_bytemask_of(int width, uint64_t addr)
{
	uint8_t byte_mask;

	if (width == 8)
		byte_mask = 0xff;
	else if (width == 4)
		byte_mask = 0x0f << (addr & 4);
	else if (width == 2)
		byte_mask = 0x03 << (addr & 6);
	else if (width == 1)
		byte_mask = 0x01 << (addr & 7);
	else
		byte_mask = 0;

	return byte_mask;
}

/* NFP6000 specific */
#define NFP6000_ARM_GCSR_SOFTMODEL0 0x00400144

/*
 * NOTE: This code should not use nfp_xpb_* functions,
 * as those are model-specific
 */
uint32_t __nfp_cpp_model_autodetect(struct nfp_cpp *cpp)
{
	uint32_t arm_id = NFP_CPP_ID(NFP_CPP_TARGET_ARM, 0, 0);
	uint32_t model = 0;

	nfp_cpp_readl(cpp, arm_id, NFP6000_ARM_GCSR_SOFTMODEL0, &model);

	if (NFP_CPP_MODEL_IS_6000(model)) {
		uint32_t tmp;

		nfp_cpp_model_set(cpp, model);

		/* The PL's PluDeviceID revision code is authoratative */
		model &= ~0xff;
		nfp_xpb_readl(cpp, NFP_XPB_DEVICE(1, 1, 16) + NFP_PL_DEVICE_ID, &tmp);
		model |= (NFP_PL_DEVICE_ID_MASK & tmp) - 0x10;
	}

	return model;
}

/*
 * nfp_cpp_map_area() - Helper function to map an area
 * @cpp:    NFP CPP handler
 * @domain: CPP domain
 * @target: CPP target
 * @addr:   CPP address
 * @size:   Size of the area
 * @area:   Area handle (output)
 *
 * Map an area of IOMEM access.  To undo the effect of this function call
 * @nfp_cpp_area_release_free(*area).
 *
 * Return: Pointer to memory mapped area or ERR_PTR
 */
uint8_t *
nfp_cpp_map_area(struct nfp_cpp *cpp, int domain, int target,
                 uint64_t addr, unsigned long size, struct nfp_cpp_area **area)
{
        uint8_t *res;
        uint32_t dest;

        dest = NFP_CPP_ISLAND_ID(target, NFP_CPP_ACTION_RW, 0, domain);

        *area = nfp_cpp_area_alloc_acquire(cpp, dest, addr, size);
        if (!*area)
                goto err_eio;

        res = nfp_cpp_area_iomem(*area);
        if (!res)
                goto err_release_free;

        return res;

err_release_free:
        nfp_cpp_area_release_free(*area);
err_eio:
        return NULL;
}

/* vim: set shiftwidth=4 expandtab: */
