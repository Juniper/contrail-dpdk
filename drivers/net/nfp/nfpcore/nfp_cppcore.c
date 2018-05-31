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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#include "nfp_target.h"
#include "nfp_cpp.h"
#include "nfp_cpp_imp.h"

#include "libnfp_cpp.h"

void nfp_cpp_priv_set(struct nfp_cpp *cpp, void *priv)
{
	cpp->priv = priv;
}

void *nfp_cpp_priv(struct nfp_cpp *cpp)
{
	return cpp->priv;
}

void nfp_cpp_model_set(struct nfp_cpp *cpp, uint32_t model)
{
	cpp->model = model;
}

/**
 * \brief nfp_cpp_model - Retrieve the Model ID of the NFP
 *
 * @param[in]   cpp NFP CPP handle
 * @return      NFP CPP Model ID
 */
uint32_t nfp_cpp_model(struct nfp_cpp *cpp)
{
	if (!cpp)
		return NFP_CPP_MODEL_INVALID;

	if (cpp->model == 0)
		cpp->model = __nfp_cpp_model_autodetect(cpp);

	return cpp->model;
}

void nfp_cpp_interface_set(struct nfp_cpp *cpp, uint32_t interface)
{
	cpp->interface = interface;
}

int nfp_cpp_serial(struct nfp_cpp *cpp, const uint8_t **serial)
{
	*serial = cpp->serial;
	return cpp->serial_len;
}

int nfp_cpp_serial_set(struct nfp_cpp *cpp, const uint8_t *serial,
		       size_t serial_len)
{
	if (cpp->serial_len)
		free(cpp->serial);

	cpp->serial = malloc(serial_len);
	if (!cpp->serial)
		return -1;

	memcpy(cpp->serial, serial, serial_len);
	cpp->serial_len = serial_len;

	return 0;
}

/**
 * \brief nfp_cpp_interface - Retrieve the Interface ID of the NFP
 *
 * @param[in]   cpp NFP CPP handle
 * @return      NFP CPP Interface ID
 */
uint16_t nfp_cpp_interface(struct nfp_cpp *cpp)
{
	if (!cpp)
		return NFP_CPP_INTERFACE(NFP_CPP_INTERFACE_TYPE_INVALID, 0, 0);

	return cpp->interface;
}

/**
 * nfp_cpp_area_priv - return private struct for CPP area
 * @cpp_area:   CPP area handle
 */
void *nfp_cpp_area_priv(struct nfp_cpp_area *cpp_area)
{
	return &cpp_area[1];
}

/**
 * nfp_cpp_area_cpp - return CPP handle for CPP area
 * @cpp_area:   CPP area handle
 */
struct nfp_cpp *nfp_cpp_area_cpp(struct nfp_cpp_area *cpp_area)
{
	return cpp_area->cpp;
}

/**
 * Get the name passed during allocation of the NFP CPP area handle
 *
 * @param   cpp_area    NFP CPP area handle
 * @return          Pointer to the area's name
 */
const char *nfp_cpp_area_name(struct nfp_cpp_area *cpp_area)
{
	return cpp_area->name;
}

/**
 * nfp_cpp_area_alloc - allocate a new CPP area
 * @cpp:    CPP handle
 * @dest:   CPP id
 * @address:    start address on CPP target
 * @size:   size of area in bytes
 *
 * Allocate and initialize a CPP area structure.  The area must later
 * be locked down with an 'acquire' before it can be safely accessed.
 *
 * NOTE: @address and @size must be 32-bit aligned values.
 */
struct nfp_cpp_area *nfp_cpp_area_alloc_with_name(struct nfp_cpp *cpp, uint32_t dest,
						  const char *name,
						  unsigned long long address,
						  unsigned long size)
{
	struct nfp_cpp_area *area;
	int tmp, err;
	uint64_t tmp64 = (uint64_t)address;

	BUG_ON(!cpp);

	/* CPP bus uses only a 40-bit address */
	if ((address + size) > (1ULL << 40)) {
		return NFP_ERRPTR(EFAULT);
	}

	/* Remap from cpp_island to cpp_target */
	err = nfp_target_cpp(dest, tmp64, &dest, &tmp64, cpp->imb_cat_table);
	if (err < 0)
		return NULL;
	address = (unsigned long long)tmp64;

	if (!name)
		name = "";

	area = calloc(1, sizeof(*area) + cpp->op->area_priv_size +
		      strlen(name) + 1);
	if (!area)
		return NULL;

	area->cpp = cpp;
	area->name = ((char *)area) + sizeof(*area) + cpp->op->area_priv_size;
	memcpy(area->name, name, strlen(name) + 1);

	/* Preserve errno around the call to area_init, since
	 * most implementations will blindly call nfp_target_action_width()
	 * for both read or write modes, and that will set errno to EINVAL
	 */
	tmp = errno;

	err = cpp->op->area_init(area, dest, address, size);
	if (err < 0) {
		free(area);
		return NULL;
	}

	/* Restore errno */
	errno = tmp;

	area->offset = address;
	area->size = size;

	return area;
}

struct nfp_cpp_area *nfp_cpp_area_alloc(struct nfp_cpp *cpp, uint32_t dest,
					unsigned long long address,
					unsigned long size)
{
	return nfp_cpp_area_alloc_with_name(cpp, dest, NULL, address, size);
}

/**
 * nfp_cpp_area_alloc_acquire - allocate a new CPP area and lock it down
 * @cpp:    CPP handle
 * @dest:   CPP id
 * @address:    start address on CPP target
 * @size:   size of area
 *
 * Allocate and initilizae a CPP area structure, and lock it down so
 * that it can be accessed directly.
 *
 * NOTE: @address and @size must be 32-bit aligned values.
 *
 * NOTE: The area must also be 'released' when the structure is freed.
 */
struct nfp_cpp_area *nfp_cpp_area_alloc_acquire(struct nfp_cpp *cpp,
						uint32_t destination,
						unsigned long long address,
						unsigned long size)
{
	struct nfp_cpp_area *area;

	area = nfp_cpp_area_alloc(cpp, destination, address, size);
	if (!area)
		return NULL;

	if (nfp_cpp_area_acquire(area)) {
		nfp_cpp_area_free(area);
		return NULL;
	}

	return area;
}

/**
 * nfp_cpp_area_free - free up the CPP area
 * area:    CPP area handle
 *
 * Frees up memory resources held by the CPP area.
 */
void nfp_cpp_area_free(struct nfp_cpp_area *area)
{
	if (area->cpp->op->area_cleanup)
		area->cpp->op->area_cleanup(area);
	free(area);
}

/**
 * nfp_cpp_area_release_free - release CPP area and free it
 * area:    CPP area handle
 *
 * Releases CPP area and frees up memory resources held by the it.
 */
void nfp_cpp_area_release_free(struct nfp_cpp_area *area)
{
	nfp_cpp_area_release(area);
	nfp_cpp_area_free(area);
}

/**
 * nfp_cpp_area_acquire - lock down a CPP area for access
 * @area:   CPP area handle
 *
 * Locks down the CPP area for a potential long term activity.  Area
 * must always be locked down before being accessed.
 */
int nfp_cpp_area_acquire(struct nfp_cpp_area *area)
{
	if (area->cpp->op->area_acquire) {
		int err = area->cpp->op->area_acquire(area);

		if (err < 0)
			return -1;
	}

	return 0;
}

/**
 * nfp_cpp_area_release - release a locked down CPP area
 * @area:   CPP area handle
 *
 * Releases a previously locked down CPP area.
 */
void nfp_cpp_area_release(struct nfp_cpp_area *area)
{
	if (area->cpp->op->area_release)
		area->cpp->op->area_release(area);
}

/**
 * nfp_cpp_area_iomem() - get IOMEM region for CPP area
 * @area:       CPP area handle
 *
 * Returns an iomem pointer for use with readl()/writel() style
 * operations.
 *
 * NOTE: Area must have been locked down with an 'acquire'.
 *
 * Return: pointer to the area, or NULL
 */
void *nfp_cpp_area_iomem(struct nfp_cpp_area *area)
{
        void *iomem = NULL;

        if (area->cpp->op->area_iomem)
                iomem = area->cpp->op->area_iomem(area);

        return iomem;
}

/**
 * nfp_cpp_area_read - read data from CPP area
 * @area:       CPP area handle
 * @offset:     offset into CPP area
 * @kernel_vaddr:   kernel address to put data into
 * @length:     number of bytes to read
 *
 * Read data from indicated CPP region.
 *
 * NOTE: @offset and @length must be 32-bit aligned values.
 *
 * NOTE: Area must have been locked down with an 'acquire'.
 */
int nfp_cpp_area_read(struct nfp_cpp_area *area,
		      unsigned long offset, void *kernel_vaddr, size_t length)
{
	if ((offset + length) > area->size)
		return NFP_ERRNO(EFAULT);
	return area->cpp->op->area_read(area, kernel_vaddr, offset, length);
}

/**
 * nfp_cpp_area_write - write data to CPP area
 * @area:       CPP area handle
 * @offset:     offset into CPP area
 * @kernel_vaddr:   kernel address to read data from
 * @length:     number of bytes to write
 *
 * Write data to indicated CPP region.
 *
 * NOTE: @offset and @length must be 32-bit aligned values.
 *
 * NOTE: Area must have been locked down with an 'acquire'.
 */
int nfp_cpp_area_write(struct nfp_cpp_area *area,
		       unsigned long offset, const void *kernel_vaddr,
		       size_t length)
{
	if ((offset + length) > area->size)
		return NFP_ERRNO(EFAULT);
	return area->cpp->op->area_write(area, kernel_vaddr, offset, length);
}

void *nfp_cpp_area_mapped(struct nfp_cpp_area *area)
{
	if (area->cpp->op->area_mapped)
		return area->cpp->op->area_mapped(area);
	return NULL;
}

/**
 * nfp_cpp_area_check_range - check if address range fits in CPP area
 * @area:   CPP area handle
 * @offset: offset into CPP area
 * @length: size of address range in bytes
 *
 * Check if address range fits within CPP area.  Return 0 if area fits
 * or -1 on error.
 */
int nfp_cpp_area_check_range(struct nfp_cpp_area *area,
			     unsigned long long offset, unsigned long length)
{
	if (((offset + length) > area->size))
		return NFP_ERRNO(EFAULT);

	return 0;
}

/* Return the correct CPP address, and fixup xpb_addr as needed,
 * based upon NFP model.
 */
static uint32_t nfp_xpb_to_cpp(struct nfp_cpp *cpp, uint32_t *xpb_addr)
{
	uint32_t xpb;

	if (NFP_CPP_MODEL_IS_6000(cpp->model)) {
		int island;

		xpb = NFP_CPP_ID(14, NFP_CPP_ACTION_RW, 0);
		/* Ensure that non-local XPB accesses go
		 * out through the global XPBM bus.
		 */
		island = ((*xpb_addr) >> 24) & 0x3f;
		if (island) {
			if (island == 1) {
				/* Accesses to the ARM Island overlay uses Island 0 / Global Bit */
				(*xpb_addr) &= ~0x7f000000;
				if (*xpb_addr < 0x60000)
					*xpb_addr |= (1 << 30);
				else {
					/* And only non-ARM interfaces use the island id = 1 */
					if (NFP_CPP_INTERFACE_TYPE_of
					    (nfp_cpp_interface(cpp)) !=
					    NFP_CPP_INTERFACE_TYPE_ARM)
						*xpb_addr |= (1 << 24);
				}
			} else {
				(*xpb_addr) |= (1 << 30);
			}
		}
	} else {
		return 0;
	}

	return xpb;
}

int nfp_cpp_area_readl(struct nfp_cpp_area *area,
		       unsigned long offset, uint32_t *value)
{
	int sz;
	uint32_t tmp;

	sz = nfp_cpp_area_read(area, offset, &tmp, sizeof(tmp));
	*value = NFP_LETOH32(tmp);

	return (sz == sizeof(*value)) ? 0 : -1;
}

int nfp_cpp_area_writel(struct nfp_cpp_area *area,
			unsigned long offset, uint32_t value)
{
	int sz;

	value = NFP_HTOLE32(value);
	sz = nfp_cpp_area_write(area, offset, &value, sizeof(value));
	return (sz == sizeof(value)) ? 0 : -1;
}

int nfp_cpp_area_readq(struct nfp_cpp_area *area,
		       unsigned long offset, uint64_t *value)
{
	int sz;
	uint64_t tmp;

	sz = nfp_cpp_area_read(area, offset, &tmp, sizeof(tmp));
	*value = NFP_LETOH64(tmp);

	return (sz == sizeof(*value)) ? 0 : -1;
}

int nfp_cpp_area_writeq(struct nfp_cpp_area *area,
			unsigned long offset, uint64_t value)
{
	int sz;

	value = NFP_HTOLE64(value);
	sz = nfp_cpp_area_write(area, offset, &value, sizeof(value));
	return (sz == sizeof(value)) ? 0 : -1;
}

int nfp_cpp_readl(struct nfp_cpp *cpp, uint32_t cpp_id,
		  unsigned long long address, uint32_t *value)
{
	int sz;
	uint32_t tmp;

	sz = nfp_cpp_read(cpp, cpp_id, address, &tmp, sizeof(tmp));
	*value = NFP_LETOH32(tmp);

	return (sz == sizeof(*value)) ? 0 : -1;
}

int nfp_cpp_writel(struct nfp_cpp *cpp, uint32_t cpp_id,
		   unsigned long long address, uint32_t value)
{
	int sz;

	value = NFP_HTOLE32(value);
	sz = nfp_cpp_write(cpp, cpp_id, address, &value, sizeof(value));
	return (sz == sizeof(value)) ? 0 : -1;
}

int nfp_cpp_readq(struct nfp_cpp *cpp, uint32_t cpp_id,
		  unsigned long long address, uint64_t *value)
{
	int sz;
	uint64_t tmp;

	sz = nfp_cpp_read(cpp, cpp_id, address, &tmp, sizeof(tmp));
	*value = NFP_LETOH64(tmp);

	return (sz == sizeof(*value)) ? 0 : -1;
}

int nfp_cpp_writeq(struct nfp_cpp *cpp, uint32_t cpp_id,
		   unsigned long long address, uint64_t value)
{
	int sz;

	value = NFP_HTOLE64(value);
	sz = nfp_cpp_write(cpp, cpp_id, address, &value, sizeof(value));
	return (sz == sizeof(value)) ? 0 : -1;
}

int nfp_xpb_writel(struct nfp_cpp *cpp, uint32_t xpb_addr, uint32_t value)
{
	uint32_t cpp_dest;

	cpp_dest = nfp_xpb_to_cpp(cpp, &xpb_addr);

	return nfp_cpp_writel(cpp, cpp_dest, xpb_addr, value);
}

int nfp_xpb_readl(struct nfp_cpp *cpp, uint32_t xpb_addr, uint32_t *value)
{
	uint32_t cpp_dest;

	cpp_dest = nfp_xpb_to_cpp(cpp, &xpb_addr);

	return nfp_cpp_readl(cpp, cpp_dest, xpb_addr, value);
}

static struct nfp_cpp *nfp_cpp_alloc(void *dl, int device_id)
{
	const struct nfp_cpp_operations *ops;
	struct nfp_cpp *cpp;
	int err;

	ops = nfp_cpp_transport_operations();

	if (!ops || !ops->init) {
		return NFP_ERRPTR(EINVAL);
	}

	cpp = calloc(1, sizeof(*cpp));
	if (!cpp)
		return NULL;

	cpp->op = ops;
	cpp->dl = dl;

	if (cpp->op->init) {
		err = cpp->op->init(cpp, device_id);
		if (err < 0) {
			free(cpp);
			return NULL;
		}
	}

	if (NFP_CPP_MODEL_IS_6000(nfp_cpp_model(cpp))) {
		uint32_t xpbaddr;
		size_t tgt;

		for (tgt = 0;
		     tgt <
		     (sizeof(cpp->imb_cat_table) /
		      sizeof(cpp->imb_cat_table[0])); tgt++) {
			/* Hardcoded XPB IMB Base, island 0 */
			xpbaddr = 0x000a0000 + (tgt * 4);
			err = nfp_xpb_readl(cpp, xpbaddr,
					    (uint32_t *) &cpp->imb_cat_table[tgt]);
			if (err < 0) {
				free(cpp);
				return NULL;
			}
		}
	}

	return cpp;
}

/**
 * nfp_cpp_free - free the CPP handle
 * @cpp:    CPP handle
 */
void nfp_cpp_free(struct nfp_cpp *cpp)
{
	if (cpp->op && cpp->op->free)
		cpp->op->free(cpp);
	if (cpp->serial_len)
		free(cpp->serial);
	free(cpp);
}

struct nfp_cpp *nfp_cpp_from_device_id(int id)
{
	void *dl = NULL;

	return nfp_cpp_alloc(dl, id);
}

/*
 * Local variables:
 * c-file-style: "Linux"
 * indent-tabs-mode: t
 * End:
 */
/* vim: set shiftwidth=4 expandtab: */
