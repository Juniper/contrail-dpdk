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

#ifndef NFP_CPP_IMP_H
#define NFP_CPP_IMP_H

#include "nfp_cpp.h"

/*
 * NFP CPP operations structure
 */
struct nfp_cpp_operations {
	/* Size of priv area in struct nfp_cpp_area */
	size_t area_priv_size;

	/* Instance an NFP CPP */
	int (*init)(struct nfp_cpp *cpp, int id);

	/*
	 * Free the bus.
	 * Called only once, during nfp_cpp_unregister()
	 */
	void (*free)(struct nfp_cpp *cpp);

	/*
	 * Initialize a new NFP CPP area
	 * NOTE: This is _not_ serialized
	 */
	int (*area_init)(struct nfp_cpp_area *area,
			 uint32_t dest,
			 unsigned long long address,
			 unsigned long size);
	/*
	 * Clean up a NFP CPP area before it is freed
	 * NOTE: This is _not_ serialized
	 */
	void (*area_cleanup)(struct nfp_cpp_area *area);

	/*
	 * Acquire resources for a NFP CPP area
	 * Serialized
	 */
	int (*area_acquire)(struct nfp_cpp_area *area);
	/*
	 * Release resources for a NFP CPP area
	 * Serialized
	 */
	void (*area_release)(struct nfp_cpp_area *area);
	/*
	 * Return a void IO pointer to a NFP CPP area
	 * NOTE: This is _not_ serialized
	 */

	void *(*area_iomem)(struct nfp_cpp_area *area);

	void *(*area_mapped)(struct nfp_cpp_area *area);
	/*
	 * Perform a read from a NFP CPP area
	 * Serialized
	 */
	int (*area_read)(struct nfp_cpp_area *area,
			 void *kernel_vaddr,
			 unsigned long offset,
			 unsigned int length);
	/*
	 * Perform a write to a NFP CPP area
	 * Serialized
	 */
	int (*area_write)(struct nfp_cpp_area *area,
			  const void *kernel_vaddr,
			  unsigned long offset,
			  unsigned int length);
};

/*
 * This should be the only external function the transport
 * module supplies
 */
const struct nfp_cpp_operations *nfp_cpp_transport_operations(void);

/*
 * Set the model id
 *
 * @param   cpp     NFP CPP operations structure
 * @param   model   Model ID
 */
void nfp_cpp_model_set(struct nfp_cpp *cpp, uint32_t model);

/*
 * Set the private instance owned data of a nfp_cpp struct
 *
 * @param   cpp     NFP CPP operations structure
 * @param   interface Interface ID
 */
void nfp_cpp_interface_set(struct nfp_cpp *cpp, uint32_t interface);

/*
 * Set the private instance owned data of a nfp_cpp struct
 *
 * @param   cpp     NFP CPP operations structure
 * @param   serial  NFP serial byte array
 * @param   len     Length of the serial byte array
 */
int nfp_cpp_serial_set(struct nfp_cpp *cpp, const uint8_t *serial, size_t serial_len);

/*
 * Set the private data of the nfp_cpp instance
 *
 * @param   cpp NFP CPP operations structure
 * @return      Opaque device pointer
 */
void nfp_cpp_priv_set(struct nfp_cpp *cpp, void *priv);

/*
 * Return the private data of the nfp_cpp instance
 *
 * @param   cpp NFP CPP operations structure
 * @return      Opaque device pointer
 */
void *nfp_cpp_priv(struct nfp_cpp *cpp);

/*
 * Get the privately allocated portion of a NFP CPP area handle
 *
 * @param   cpp_area    NFP CPP area handle
 * @return          Pointer to the private area, or NULL on failure
 */
void *nfp_cpp_area_priv(struct nfp_cpp_area *cpp_area);

#endif /* NFP_CPP_IMP_H */
