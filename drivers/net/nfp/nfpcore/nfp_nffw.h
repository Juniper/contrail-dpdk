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

#ifndef __NFP_NFFW_H__
#define __NFP_NFFW_H__

#include "nfp-common/nfp_platform.h"
#include "libnfp_cpp.h"
#include "nfp_nsp.h"

/** Init-CSR owner IDs for firmware map to firmware IDs which start at 4.
 * Lower IDs are reserved for target and loader IDs.
 */
#define NFFW_FWID_EXT   3	/* For active MEs that we didn't load. */
#define NFFW_FWID_BASE  4

#define NFFW_FWID_ALL   255

/* Implemented in nfp_rtsym.c */

#define NFP_RTSYM_TYPE_NONE             0
#define NFP_RTSYM_TYPE_OBJECT           1
#define NFP_RTSYM_TYPE_FUNCTION         2
#define NFP_RTSYM_TYPE_ABS              3

#define NFP_RTSYM_TARGET_NONE           0
#define NFP_RTSYM_TARGET_LMEM           -1
#define NFP_RTSYM_TARGET_EMU_CACHE      -7

/* Implemented in nfp_mip.c */

struct nfp_mip;

struct nfp_mip *nfp_mip_open(struct nfp_cpp *cpp);
void nfp_mip_close(struct nfp_mip *mip);

const char *nfp_mip_name(const struct nfp_mip *mip);
void nfp_mip_symtab(const struct nfp_mip *mip, uint32_t *addr, uint32_t *size);
void nfp_mip_strtab(const struct nfp_mip *mip, uint32_t *addr, uint32_t *size);

/*const *
 * Structure describing a run-time NFP symbol.
 *
 * The memory target of the symbol is generally the CPP target number
 * and can be used directly by the nfp_cpp API calls.  However, in
 * some cases (i.e., for local memory or control store) the target is
 * encoded using a negative number.
 *
 * When the target type can not be used to fully describe the location
 * of a symbol the domain field is used to further specify the
 * location (i.e., the specific ME or island number).
 *
 * For ME target resources, 'domain' is an MEID.
 * For Island target resources, 'domain' is an island ID, with the one
 * exception of "sram" symbols for backward compatibility, which are viewed
 * as global.
 *
 */
struct nfp_rtsym {
	const char *name;
	uint64_t addr;
	uint64_t size;
	int type;		/** NFP_RTSYM_TYPE_* in nfp-common/nfp_nffw_rtt.h */
	int target;		/** NFP_RTSYM_TARGET_* in nfp-common/nfp_nffw_rtt.h */
	int domain;
};

struct nfp_rtsym_table;

struct nfp_rtsym_table *nfp_rtsym_table_read(struct nfp_cpp *cpp);
struct nfp_rtsym_table *
__nfp_rtsym_table_read(struct nfp_cpp *cpp, const struct nfp_mip *mip);
int nfp_rtsym_count(struct nfp_rtsym_table *rtbl);
const struct nfp_rtsym *nfp_rtsym_get(struct nfp_rtsym_table *rtbl, int idx);
const struct nfp_rtsym *
nfp_rtsym_lookup(struct nfp_rtsym_table *rtbl, const char *name);

uint64_t nfp_rtsym_read_le(struct nfp_rtsym_table *rtbl, const char *name,
                      int *error);
uint8_t *
nfp_rtsym_map(struct nfp_rtsym_table *rtbl, const char *name,
              unsigned int min_size, struct nfp_cpp_area **area);

uint8_t * nfp_cpp_map_area(struct nfp_cpp *cpp, int domain, int target,
                 uint64_t addr, unsigned long size, struct nfp_cpp_area **area);

void *nfp_cpp_area_iomem(struct nfp_cpp_area *area);

int nfp_nsp_read_identify(struct nfp_nsp *state, void *buf, unsigned int size);
int nfp_nsp_read_sensors(struct nfp_nsp *state, unsigned int sensor_mask,
                         void *buf, unsigned int size);

struct nfp_nffw_info *nfp_nffw_info_open(struct nfp_cpp *cpp);
void nfp_nffw_info_close(struct nfp_nffw_info *state);
int nfp_nffw_info_mip_first(struct nfp_nffw_info *state, uint32_t *cpp_id, uint64_t *off);

#endif
