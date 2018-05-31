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
 * nfp_rtsym.c
 * Interface for accessing run-time symbol table
 * Authors: Jakub Kicinski <jakub.kicinski@netronome.com>
 *          Jason McMullan <jason.mcmullan@netronome.com>
 *          Espen Skoglund <espen.skoglund@netronome.com>
 *          Francois H. Theron <francois.theron@netronome.com>
 */

#include "libnfp_cpp.h"
#include "nfp_cpp.h"
#include "nfp_nffw.h"
#include "nfp6000/nfp6000.h"

/* These need to match the linker */
#define SYM_TGT_LMEM		0
#define SYM_TGT_EMU_CACHE	0x17

#define cpu_to_le32
#define le32_to_cpu
#define __le32 uint32_t

#define cpu_to_le16
#define le16_to_cpu
#define __le16 uint16_t

struct nfp_rtsym_entry {
	uint8_t	type;
	uint8_t	target;
	uint8_t	island;
	uint8_t	addr_hi;
	__le32	addr_lo;
	__le16	name;
	uint8_t	menum;
	uint8_t	size_hi;
	__le32	size_lo;
};

struct nfp_rtsym_table {
	struct nfp_cpp *cpp;
	int num;
	char *strtab;
	struct nfp_rtsym symtab[];
};

static int nfp_meid(uint8_t island_id, uint8_t menum)
{
	return (island_id & 0x3F) == island_id && menum < 12 ?
		(island_id << 4) | (menum + 4) : -1;
}

static void
nfp_rtsym_sw_entry_init(struct nfp_rtsym_table *cache, uint32_t strtab_size,
			struct nfp_rtsym *sw, struct nfp_rtsym_entry *fw)
{
	sw->type = fw->type;
	sw->name = cache->strtab + le16_to_cpu(fw->name) % strtab_size;
	sw->addr = ((uint64_t)fw->addr_hi << 32) | le32_to_cpu(fw->addr_lo);
	sw->size = ((uint64_t)fw->size_hi << 32) | le32_to_cpu(fw->size_lo);

	switch (fw->target) {
	case SYM_TGT_LMEM:
		sw->target = NFP_RTSYM_TARGET_LMEM;
		break;
	case SYM_TGT_EMU_CACHE:
		sw->target = NFP_RTSYM_TARGET_EMU_CACHE;
		break;
	default:
		sw->target = fw->target;
		break;
	}

	if (fw->menum != 0xff)
		sw->domain = nfp_meid(fw->island, fw->menum);
	else if (fw->island != 0xff)
		sw->domain = fw->island;
	else
		sw->domain = -1;
}

struct nfp_rtsym_table *nfp_rtsym_table_read(struct nfp_cpp *cpp)
{
	struct nfp_rtsym_table *rtbl;
	struct nfp_mip *mip;

	mip = nfp_mip_open(cpp);
	rtbl = __nfp_rtsym_table_read(cpp, mip);
	nfp_mip_close(mip);

	return rtbl;
}

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

struct nfp_rtsym_table *
__nfp_rtsym_table_read(struct nfp_cpp *cpp, const struct nfp_mip *mip)
{
	const uint32_t dram = NFP_CPP_ID(NFP_CPP_TARGET_MU, NFP_CPP_ACTION_RW, 0) |
		NFP_ISL_EMEM0;
	uint32_t strtab_addr, symtab_addr, strtab_size, symtab_size;
	struct nfp_rtsym_entry *rtsymtab;
	struct nfp_rtsym_table *cache;
	int err, n, size;

	if (!mip)
		return NULL;

	nfp_mip_strtab(mip, &strtab_addr, &strtab_size);
	nfp_mip_symtab(mip, &symtab_addr, &symtab_size);

	if (!symtab_size || !strtab_size || symtab_size % sizeof(*rtsymtab))
		return NULL;

	/* Align to 64 bits */
	symtab_size = round_up(symtab_size, 8);
	strtab_size = round_up(strtab_size, 8);

	rtsymtab = malloc(symtab_size);
	if (!rtsymtab)
		return NULL;

	size = sizeof(*cache);
	size += symtab_size / sizeof(*rtsymtab) * sizeof(struct nfp_rtsym);
	size +=	strtab_size + 1;
	cache = malloc(size);
	if (!cache)
		goto exit_free_rtsym_raw;

	cache->cpp = cpp;
	cache->num = symtab_size / sizeof(*rtsymtab);
	cache->strtab = (void *)&cache->symtab[cache->num];

	err = nfp_cpp_read(cpp, dram, symtab_addr, rtsymtab, symtab_size);
	if (err != (int)symtab_size)
		goto exit_free_cache;

	err = nfp_cpp_read(cpp, dram, strtab_addr, cache->strtab, strtab_size);
	if (err != (int)strtab_size)
		goto exit_free_cache;
	cache->strtab[strtab_size] = '\0';

	for (n = 0; n < cache->num; n++)
		nfp_rtsym_sw_entry_init(cache, strtab_size,
					&cache->symtab[n], &rtsymtab[n]);

	free(rtsymtab);

	return cache;

exit_free_cache:
	free(cache);
exit_free_rtsym_raw:
	free(rtsymtab);
	return NULL;
}

/**
 * nfp_rtsym_count() - Get the number of RTSYM descriptors
 * @rtbl:	NFP RTsym table
 *
 * Return: Number of RTSYM descriptors
 */
int nfp_rtsym_count(struct nfp_rtsym_table *rtbl)
{
	if (!rtbl)
		return -EINVAL;
	return rtbl->num;
}

/**
 * nfp_rtsym_get() - Get the Nth RTSYM descriptor
 * @rtbl:	NFP RTsym table
 * @idx:	Index (0-based) of the RTSYM descriptor
 *
 * Return: const pointer to a struct nfp_rtsym descriptor, or NULL
 */
const struct nfp_rtsym *nfp_rtsym_get(struct nfp_rtsym_table *rtbl, int idx)
{
	if (!rtbl)
		return NULL;
	if (idx >= rtbl->num)
		return NULL;

	return &rtbl->symtab[idx];
}

/**
 * nfp_rtsym_lookup() - Return the RTSYM descriptor for a symbol name
 * @rtbl:	NFP RTsym table
 * @name:	Symbol name
 *
 * Return: const pointer to a struct nfp_rtsym descriptor, or NULL
 */
const struct nfp_rtsym *
nfp_rtsym_lookup(struct nfp_rtsym_table *rtbl, const char *name)
{
	int n;

	if (!rtbl)
		return NULL;

	for (n = 0; n < rtbl->num; n++)
		if (strcmp(name, rtbl->symtab[n].name) == 0)
			return &rtbl->symtab[n];

	return NULL;
}

/**
 * nfp_rtsym_read_le() - Read a simple unsigned scalar value from symbol
 * @rtbl:	NFP RTsym table
 * @name:	Symbol name
 * @error:	Poniter to error code (optional)
 *
 * Lookup a symbol, map, read it and return it's value. Value of the symbol
 * will be interpreted as a simple little-endian unsigned value. Symbol can
 * be 4 or 8 bytes in size.
 *
 * Return: value read, on error sets the error and returns ~0ULL.
 */
uint64_t nfp_rtsym_read_le(struct nfp_rtsym_table *rtbl, const char *name,
		      int *error)
{
	const struct nfp_rtsym *sym;
	uint32_t val32, id;
	uint64_t val;
	int err;

	//printf("nfp_rtsym_read_le for %s\n", name);
	sym = nfp_rtsym_lookup(rtbl, name);
	if (!sym) {
		printf("nfp_rtsym_read_le for %s: No entry!\n", name);
		err = -ENOENT;
		goto exit;
	}

	id = NFP_CPP_ISLAND_ID(sym->target, NFP_CPP_ACTION_RW, 0, sym->domain);

	//printf("Reading symbol %s with size %"PRIu64" at %"PRIx64"\n", name, sym->size, sym->addr);
	switch (sym->size) {
	case 4:
		err = nfp_cpp_readl(rtbl->cpp, id, sym->addr, &val32);
		val = val32;
		break;
	case 8:
		err = nfp_cpp_readq(rtbl->cpp, id, sym->addr, &val);
		break;
	default:
		printf("rtsym '%s' unsupported or non-scalar size: %"PRId64"\n",
			name, sym->size);
		err = -EINVAL;
		break;
	}

	if (err) {
		printf("nfp_rtsym_read_le for %s: Error!. (%d)\n", name, err);
		err = -EIO;
	}
exit:
	if (error)
		*error = err;

	if (err)
		return ~0ULL;
	return val;
}

uint8_t *
nfp_rtsym_map(struct nfp_rtsym_table *rtbl, const char *name,
	      unsigned int min_size, struct nfp_cpp_area **area)
{
	const struct nfp_rtsym *sym;
	uint8_t *mem;

	printf("nfp_rtsym_map for %s\n", name);
	sym = nfp_rtsym_lookup(rtbl, name);
	if (!sym) {
		printf("nfp_rtsym_lookup fails for %s\n", name);
		return NULL;
	}

	if (sym->size < min_size) {
		printf("Symbol %s too small (%"PRIu64" < %u)\n", name, sym->size,
			min_size);
		return NULL;
	}

	mem = nfp_cpp_map_area(rtbl->cpp, sym->domain, sym->target,
			       sym->addr, sym->size, area);
	if (!mem) {
		printf("Failed to map symbol %s\n", name);
		return NULL;
	}
	printf("nfp_rtsym_map: returning address %p\n", mem);

	return mem;
}
