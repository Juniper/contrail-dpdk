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

#ifndef NSP_NSP_H
#define NSP_NSP_H 1

struct firmware;
struct nfp_cpp;
struct nfp_nsp;

struct nfp_nsp *nfp_nsp_open(struct nfp_cpp *cpp);
void nfp_nsp_close(struct nfp_nsp *state);
uint16_t nfp_nsp_get_abi_ver_major(struct nfp_nsp *state);
uint16_t nfp_nsp_get_abi_ver_minor(struct nfp_nsp *state);
int nfp_nsp_wait(struct nfp_nsp *state);
int nfp_nsp_device_soft_reset(struct nfp_nsp *state);
int nfp_nsp_load_fw(struct nfp_nsp *state, void *buf, unsigned int size);
int nfp_nsp_mac_reinit(struct nfp_nsp *state);

static inline int nfp_nsp_has_mac_reinit(struct nfp_nsp *state)
{
	return nfp_nsp_get_abi_ver_minor(state) > 20;
}

enum nfp_eth_interface {
	NFP_INTERFACE_NONE	= 0,
	NFP_INTERFACE_SFP	= 1,
	NFP_INTERFACE_SFPP	= 10,
	NFP_INTERFACE_SFP28	= 28,
	NFP_INTERFACE_QSFP	= 40,
	NFP_INTERFACE_CXP	= 100,
	NFP_INTERFACE_QSFP28	= 112,
};

enum nfp_eth_media {
	NFP_MEDIA_DAC_PASSIVE = 0,
	NFP_MEDIA_DAC_ACTIVE,
	NFP_MEDIA_FIBRE,
};

enum nfp_eth_aneg {
	NFP_ANEG_AUTO = 0,
	NFP_ANEG_SEARCH,
	NFP_ANEG_25G_CONSORTIUM,
	NFP_ANEG_25G_IEEE,
	NFP_ANEG_DISABLED,
};

enum nfp_eth_fec {
	NFP_FEC_AUTO_BIT = 0,
	NFP_FEC_BASER_BIT,
	NFP_FEC_REED_SOLOMON_BIT,
	NFP_FEC_DISABLED_BIT,
};

#define NFP_FEC_AUTO		BIT(NFP_FEC_AUTO_BIT)
#define NFP_FEC_BASER		BIT(NFP_FEC_BASER_BIT)
#define NFP_FEC_REED_SOLOMON	BIT(NFP_FEC_REED_SOLOMON_BIT)
#define NFP_FEC_DISABLED	BIT(NFP_FEC_DISABLED_BIT)

#define ETH_ALEN	6

/**
 * struct nfp_eth_table - ETH table information
 * @count:	number of table entries
 * @max_index:	max of @index fields of all @ports
 * @ports:	table of ports
 *
 * @eth_index:	port index according to legacy ethX numbering
 * @index:	chip-wide first channel index
 * @nbi:	NBI index
 * @base:	first channel index (within NBI)
 * @lanes:	number of channels
 * @speed:	interface speed (in Mbps)
 * @interface:	interface (module) plugged in
 * @media:	media type of the @interface
 * @fec:	forward error correction mode
 * @aneg:	auto negotiation mode
 * @mac_addr:	interface MAC address
 * @label_port:	port id
 * @label_subport:  id of interface within port (for split ports)
 * @enabled:	is enabled?
 * @tx_enabled:	is TX enabled?
 * @rx_enabled:	is RX enabled?
 * @override_changed: is media reconfig pending?
 *
 * @port_type:	one of %PORT_* defines for ethtool
 * @port_lanes:	total number of lanes on the port (sum of lanes of all subports)
 * @is_split:	is interface part of a split port
 * @fec_modes_supported:	bitmap of FEC modes supported
 */
struct nfp_eth_table {
	unsigned int count;
	unsigned int max_index;
	struct nfp_eth_table_port {
		unsigned int eth_index;
		unsigned int index;
		unsigned int nbi;
		unsigned int base;
		unsigned int lanes;
		unsigned int speed;

		unsigned int interface;
		enum nfp_eth_media media;

		enum nfp_eth_fec fec;
		enum nfp_eth_aneg aneg;

		uint8_t mac_addr[ETH_ALEN];

		uint8_t label_port;
		uint8_t label_subport;

		int enabled;
		int tx_enabled;
		int rx_enabled;

		int override_changed;

		/* Computed fields */
		uint8_t port_type;

		unsigned int port_lanes;

		int is_split;

		unsigned int fec_modes_supported;
	} ports[0];
};

struct nfp_eth_table *nfp_eth_read_ports(struct nfp_cpp *cpp);

int nfp_eth_set_mod_enable(struct nfp_cpp *cpp, unsigned int idx, int enable);
int nfp_eth_set_configured(struct nfp_cpp *cpp, unsigned int idx,
			   int configed);
int
nfp_eth_set_fec(struct nfp_cpp *cpp, unsigned int idx, enum nfp_eth_fec mode);

int nfp_nsp_read_eth_table(struct nfp_nsp *state, void *buf, unsigned int size);
int nfp_nsp_write_eth_table(struct nfp_nsp *state, const void *buf, unsigned int size);
void nfp_nsp_config_set_state(struct nfp_nsp *state, void *entries, unsigned int idx);
void nfp_nsp_config_clear_state(struct nfp_nsp *state);
void nfp_nsp_config_set_modified(struct nfp_nsp *state, int modified);
void *nfp_nsp_config_entries(struct nfp_nsp *state);
int nfp_nsp_config_modified(struct nfp_nsp *state);
unsigned int nfp_nsp_config_idx(struct nfp_nsp *state);

static inline int nfp_eth_can_support_fec(struct nfp_eth_table_port *eth_port)
{
	return !!eth_port->fec_modes_supported;
}

static inline unsigned int
nfp_eth_supported_fec_modes(struct nfp_eth_table_port *eth_port)
{
	return eth_port->fec_modes_supported;
}

struct nfp_nsp *nfp_eth_config_start(struct nfp_cpp *cpp, unsigned int idx);
int nfp_eth_config_commit_end(struct nfp_nsp *nsp);
void nfp_eth_config_cleanup_end(struct nfp_nsp *nsp);

int __nfp_eth_set_aneg(struct nfp_nsp *nsp, enum nfp_eth_aneg mode);
int __nfp_eth_set_speed(struct nfp_nsp *nsp, unsigned int speed);
int __nfp_eth_set_split(struct nfp_nsp *nsp, unsigned int lanes);

/**
 * struct nfp_nsp_identify - NSP static information
 * @version:      opaque version string
 * @flags:        version flags
 * @br_primary:   branch id of primary bootloader
 * @br_secondary: branch id of secondary bootloader
 * @br_nsp:       branch id of NSP
 * @primary:      version of primarary bootloader
 * @secondary:    version id of secondary bootloader
 * @nsp:          version id of NSP
 * @sensor_mask:  mask of present sensors available on NIC
 */
struct nfp_nsp_identify {
	char version[40];
	uint8_t flags;
	uint8_t br_primary;
	uint8_t br_secondary;
	uint8_t br_nsp;
	uint16_t primary;
	uint16_t secondary;
	uint16_t nsp;
	uint64_t sensor_mask;
};

struct nfp_nsp_identify *__nfp_nsp_identify(struct nfp_nsp *nsp);

enum nfp_nsp_sensor_id {
	NFP_SENSOR_CHIP_TEMPERATURE,
	NFP_SENSOR_ASSEMBLY_POWER,
	NFP_SENSOR_ASSEMBLY_12V_POWER,
	NFP_SENSOR_ASSEMBLY_3V3_POWER,
};

int nfp_hwmon_read_sensor(struct nfp_cpp *cpp, enum nfp_nsp_sensor_id id,
			  long *val);

#endif
