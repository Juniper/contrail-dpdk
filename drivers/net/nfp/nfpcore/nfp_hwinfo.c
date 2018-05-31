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

/* Parse the hwinfo table that the ARM firmware builds in the ARM scratch SRAM
 * after chip reset.
 *
 * Examples of the fields:
 *   me.count = 40
 *   me.mask = 0x7f_ffff_ffff
 *
 *   me.count is the total number of MEs on the system.
 *   me.mask is the bitmask of MEs that are available for application usage.
 *
 *   (ie, in this example, ME 39 has been reserved by boardconfig.)
 */

#include <stdio.h>
#include <time.h>
#include <zlib.h>

#include "libnfp_cpp.h"
#include "nfp_cpp.h"
#include "nfp6000/nfp6000.h"
#include "nfp_resource.h"
#include "nfp_crc.h"
#include "nfp_hwinfo.h"

static int nfp_hwinfo_is_updating(struct nfp_hwinfo *hwinfo)
{
	return hwinfo->version & NFP_HWINFO_VERSION_UPDATING;
}

static int
hwinfo_db_walk(struct nfp_hwinfo *hwinfo, uint32_t size)
{
	const char *key, *val, *end = hwinfo->data + size;

	for (key = hwinfo->data; *key && key < end;
	     key = val + strlen(val) + 1) {

		val = key + strlen(key) + 1;
		if (val >= end) {
			printf("Bad HWINFO - overflowing key\n");
			return -EINVAL;
		}

		if (val + strlen(val) + 1 > end) {
			printf("Bad HWINFO - overflowing value\n");
			return -EINVAL;
		}

	}

	return 0;
}

static int
hwinfo_db_validate(struct nfp_hwinfo *db, uint32_t len)
{
	uint32_t size, new_crc, *crc;

	size = db->size;
	if (size > len) {
		printf("Unsupported hwinfo size %u > %u\n", size, len);
		return -EINVAL;
	}

	size -= sizeof(uint32_t);
	new_crc = crc32_posix((char *)db, size);
	crc = (uint32_t *)(db->start + size);
	if (new_crc != *crc) {
		printf("Corrupt hwinfo table (CRC mismatch), calculated 0x%x, expected 0x%x\n",
			new_crc, *crc);

		return -EINVAL;
	}

	return hwinfo_db_walk(db, size);
}

static struct nfp_hwinfo *
hwinfo_try_fetch(struct nfp_cpp *cpp, size_t *cpp_size)
{
	struct nfp_hwinfo *header;
	void *res;
	uint64_t cpp_addr;
	uint32_t cpp_id;
	int err;
	uint8_t *db;

	res = nfp_resource_acquire(cpp, NFP_RESOURCE_NFP_HWINFO);
	if (res) {
		cpp_id = nfp_resource_cpp_id(res);
		cpp_addr = nfp_resource_address(res);
		*cpp_size = nfp_resource_size(res);

		nfp_resource_release(res);

		if (*cpp_size < HWINFO_SIZE_MIN)
			return NULL;
#if 0
	} else if (PTR_ERR(res) == -ENOENT) {
		/* Try getting the HWInfo table from the 'classic' location */
		cpp_id = NFP_CPP_ISLAND_ID(NFP_CPP_TARGET_MU,
					   NFP_CPP_ACTION_RW, 0, 1);
		cpp_addr = 0x30000;
		*cpp_size = 0x0e000;
#endif
	} else {
		return NULL;
	}

	db = malloc(*cpp_size + 1);
	if (!db)
		return NULL;

	//printf("Hwinfo read: %08x, %"PRIx64"\n", cpp_id, cpp_addr);
	err = nfp_cpp_read(cpp, cpp_id, cpp_addr, db, *cpp_size);
	if (err != (int)*cpp_size)
		goto exit_free;

	header = (void *)db;
	printf("HWINFO header: %08x\n", *(uint32_t *)header);
	if (nfp_hwinfo_is_updating(header))
		goto exit_free;

	if (header->version != NFP_HWINFO_VERSION_2) {
		printf("Unknown HWInfo version: 0x%08x\n",
			header->version);
		goto exit_free;
	}

	/* NULL-terminate for safety */
	db[*cpp_size] = '\0';

	return (void *)db;
exit_free:
	free(db);
	return NULL;
}

static struct nfp_hwinfo *hwinfo_fetch(struct nfp_cpp *cpp, size_t *hwdb_size)
{
	struct timespec wait;
	struct nfp_hwinfo *db;
	int count;

	wait.tv_sec = 0;
	wait.tv_nsec = 10000000;
	count = 0;

	for (;;) {
		db = hwinfo_try_fetch(cpp, hwdb_size);
		if (db)
			return db;

		nanosleep(&wait, NULL);
		if (count++ > 200) {
			printf("NFP access error\n");
			return NULL;
		}
	}
}

struct nfp_hwinfo *nfp_hwinfo_read(struct nfp_cpp *cpp)
{
	struct nfp_hwinfo *db;
	size_t hwdb_size = 0;
	int err;

	db = hwinfo_fetch(cpp, &hwdb_size);
	if (!db)
		return NULL;

	err = hwinfo_db_validate(db, hwdb_size);
	if (err) {
		free(db);
		return NULL;
	}

	return db;
}

/**
 * nfp_hwinfo_lookup() - Find a value in the HWInfo table by name
 * @hwinfo:	NFP HWinfo table
 * @lookup:	HWInfo name to search for
 *
 * Return: Value of the HWInfo name, or NULL
 */
const char *nfp_hwinfo_lookup(struct nfp_hwinfo *hwinfo, const char *lookup)
{
	const char *key, *val, *end;

	if (!hwinfo || !lookup)
		return NULL;

	end = hwinfo->data + hwinfo->size - sizeof(uint32_t);

	for (key = hwinfo->data; *key && key < end;
	     key = val + strlen(val) + 1) {

		val = key + strlen(key) + 1;

		if (strcmp(key, lookup) == 0)
			return val;
	}

	return NULL;
}
