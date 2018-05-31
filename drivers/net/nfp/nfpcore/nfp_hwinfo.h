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

#include <inttypes.h>

#define HWINFO_SIZE_MIN	0x100

/* The Hardware Info Table defines the properties of the system.
 *
 * HWInfo v1 Table (fixed size)
 *
 * 0x0000: uint32_t version	        Hardware Info Table version (1.0)
 * 0x0004: uint32_t size	        Total size of the table, including
 *			        the CRC32 (IEEE 802.3)
 * 0x0008: uint32_t jumptab	        Offset of key/value table
 * 0x000c: uint32_t keys	        Total number of keys in the key/value table
 * NNNNNN:		        Key/value jump table and string data
 * (size - 4): uint32_t crc32	CRC32 (same as IEEE 802.3, POSIX csum, etc)
 *				CRC32("",0) = ~0, CRC32("a",1) = 0x48C279FE
 *
 * HWInfo v2 Table (variable size)
 *
 * 0x0000: uint32_t version	        Hardware Info Table version (2.0)
 * 0x0004: uint32_t size	        Current size of the data area, excluding CRC32
 * 0x0008: uint32_t limit	        Maximum size of the table
 * 0x000c: uint32_t reserved	        Unused, set to zero
 * NNNNNN:			Key/value data
 * (size - 4): uint32_t crc32	CRC32 (same as IEEE 802.3, POSIX csum, etc)
 *				CRC32("",0) = ~0, CRC32("a",1) = 0x48C279FE
 *
 * If the HWInfo table is in the process of being updated, the low bit
 * of version will be set.
 *
 * HWInfo v1 Key/Value Table
 * -------------------------
 *
 *  The key/value table is a set of offsets to ASCIIZ strings which have
 *  been strcmp(3) sorted (yes, please use bsearch(3) on the table).
 *
 *  All keys are guaranteed to be unique.
 *
 * N+0:	uint32_t key_1		Offset to the first key
 * N+4:	uint32_t val_1		Offset to the first value
 * N+8: uint32_t key_2		Offset to the second key
 * N+c: uint32_t val_2		Offset to the second value
 * ...
 *
 * HWInfo v2 Key/Value Table
 * -------------------------
 *
 * Packed UTF8Z strings, ie 'key1\000value1\000key2\000value2\000'
 *
 * Unsorted.
 */

#define NFP_HWINFO_VERSION_1 ('H' << 24 | 'I' << 16 | 1 << 8 | 0 << 1 | 0)
#define NFP_HWINFO_VERSION_2 ('H' << 24 | 'I' << 16 | 2 << 8 | 0 << 1 | 0)
#define NFP_HWINFO_VERSION_UPDATING	BIT(0)


struct nfp_hwinfo {
	uint8_t start[0];

	__uint32_t version;
	__uint32_t size;

	/* v2 specific fields */
	__uint32_t limit;
	__uint32_t resv;

	char data[];
};

struct nfp_hwinfo *nfp_hwinfo_read(struct nfp_cpp *cpp);

const char *nfp_hwinfo_lookup(struct nfp_hwinfo *hwinfo, const char *lookup);

