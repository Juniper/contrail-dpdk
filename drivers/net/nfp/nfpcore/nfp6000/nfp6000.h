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

#ifndef NFP6000_NFP6000_H
#define NFP6000_NFP6000_H

/* CPP Target IDs */
#define NFP_CPP_TARGET_INVALID          0
#define NFP_CPP_TARGET_NBI              1
#define NFP_CPP_TARGET_QDR              2
#define NFP_CPP_TARGET_ILA              6
#define NFP_CPP_TARGET_MU               7
#define NFP_CPP_TARGET_PCIE             9
#define NFP_CPP_TARGET_ARM              10
#define NFP_CPP_TARGET_CRYPTO           12
#define NFP_CPP_TARGET_ISLAND_XPB       14	/* Shared with CAP */
#define NFP_CPP_TARGET_ISLAND_CAP       14	/* Shared with XPB */
#define NFP_CPP_TARGET_CT_XPB           14
#define NFP_CPP_TARGET_LOCAL_SCRATCH    15
#define NFP_CPP_TARGET_CLS              NFP_CPP_TARGET_LOCAL_SCRATCH

#define NFP_ISL_EMEM0                   24

#define NFP_MU_ADDR_ACCESS_TYPE_MASK    3ULL
#define NFP_MU_ADDR_ACCESS_TYPE_DIRECT  2ULL

static inline int nfp_cppat_mu_locality_lsb(int mode, int addr40)
{
        switch (mode) {
        case 0 ... 3:
                return addr40 ? 38 : 30;
        default:
                return -EINVAL;
        }
}

#endif /* NFP6000_NFP6000_H */
