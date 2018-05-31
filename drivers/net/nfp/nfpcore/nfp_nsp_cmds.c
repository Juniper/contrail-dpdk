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

#include "libnfp_cpp.h"
#include "nfp_nsp.h"
#include "nfp_nffw.h"

#define __le16	uint16_t
#define __le32	uint32_t
#define __le64	uint64_t

#define le16_to_cpu
#define le32_to_cpu
#define le64_to_cpu

struct nsp_identify {
	uint8_t version[40];
	uint8_t flags;
	uint8_t br_primary;
	uint8_t br_secondary;
	uint8_t br_nsp;
	__le16 primary;
	__le16 secondary;
	__le16 nsp;
	uint8_t reserved[6];
	__le64 sensor_mask;
};

struct nfp_nsp_identify *__nfp_nsp_identify(struct nfp_nsp *nsp)
{
	struct nfp_nsp_identify *nspi = NULL;
	struct nsp_identify *ni;
	int ret;

	if (nfp_nsp_get_abi_ver_minor(nsp) < 15)
		return NULL;

	ni = malloc(sizeof(*ni));
	if (!ni)
		return NULL;

	memset(ni, 0, sizeof(*ni));
	ret = nfp_nsp_read_identify(nsp, ni, sizeof(*ni));
	if (ret < 0) {
		printf("reading bsp version failed %d\n",
			ret);
		goto exit_free;
	}

	nspi = malloc(sizeof(*nspi));
	if (!nspi)
		goto exit_free;

	memset(nspi, 0, sizeof(*nspi));
	memcpy(nspi->version, ni->version, sizeof(nspi->version));
	nspi->version[sizeof(nspi->version) - 1] = '\0';
	nspi->flags = ni->flags;
	nspi->br_primary = ni->br_primary;
	nspi->br_secondary = ni->br_secondary;
	nspi->br_nsp = ni->br_nsp;
	nspi->primary = le16_to_cpu(ni->primary);
	nspi->secondary = le16_to_cpu(ni->secondary);
	nspi->nsp = le16_to_cpu(ni->nsp);
	nspi->sensor_mask = le64_to_cpu(ni->sensor_mask);

exit_free:
	free(ni);
	return nspi;
}

struct nfp_sensors {
	__le32 chip_temp;
	__le32 assembly_power;
	__le32 assembly_12v_power;
	__le32 assembly_3v3_power;
};

int nfp_hwmon_read_sensor(struct nfp_cpp *cpp, enum nfp_nsp_sensor_id id,
			  long *val)
{
	struct nfp_sensors s;
	struct nfp_nsp *nsp;
	int ret;

	nsp = nfp_nsp_open(cpp);
	if (!nsp)
		return -EIO;

	ret = nfp_nsp_read_sensors(nsp, BIT(id), &s, sizeof(s));
	nfp_nsp_close(nsp);

	if (ret < 0)
		return ret;

	switch (id) {
	case NFP_SENSOR_CHIP_TEMPERATURE:
		*val = le32_to_cpu(s.chip_temp);
		break;
	case NFP_SENSOR_ASSEMBLY_POWER:
		*val = le32_to_cpu(s.assembly_power);
		break;
	case NFP_SENSOR_ASSEMBLY_12V_POWER:
		*val = le32_to_cpu(s.assembly_12v_power);
		break;
	case NFP_SENSOR_ASSEMBLY_3V3_POWER:
		*val = le32_to_cpu(s.assembly_3v3_power);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
