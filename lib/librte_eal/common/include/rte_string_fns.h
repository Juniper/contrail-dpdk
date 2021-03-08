/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *
 * String-related functions as replacement for libc equivalents
 */

#ifndef _RTE_STRING_FNS_H_
#define _RTE_STRING_FNS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include<stdio.h>
#include <string.h>
#include <rte_common.h>

/**
 * Takes string "string" parameter and splits it at character "delim"
 * up to maxtokens-1 times - to give "maxtokens" resulting tokens. Like
 * strtok or strsep functions, this modifies its input string, by replacing
 * instances of "delim" with '\\0'. All resultant tokens are returned in the
 * "tokens" array which must have enough entries to hold "maxtokens".
 *
 * @param string
 *   The input string to be split into tokens
 *
 * @param stringlen
 *   The max length of the input buffer
 *
 * @param tokens
 *   The array to hold the pointers to the tokens in the string
 *
 * @param maxtokens
 *   The number of elements in the tokens array. At most, maxtokens-1 splits
 *   of the string will be done.
 *
 * @param delim
 *   The character on which the split of the data will be done
 *
 * @return
 *   The number of tokens in the tokens array.
 */
int
rte_strsplit(char *string, int stringlen,
             char **tokens, int maxtokens, char delim);

static inline size_t
rte_strlcpy(char *dst, const char *src, size_t size)
{
    return (size_t)snprintf(dst, size, "%s", src);
}
static inline size_t
rte_strlcat(char *dst, const char *src, size_t size)
{
    size_t l = strnlen(dst, size);
    if (l < size)
        return l + rte_strlcpy(&dst[l], src, size - l);
    return l + strlen(src);
}
/* pull in a strlcpy function */
#ifdef RTE_EXEC_ENV_FREEBSD
#ifndef __BSD_VISIBLE /* non-standard functions are hidden */
#define strlcpy(dst, src, size) rte_strlcpy(dst, src, size)
#define strlcat(dst, src, size) rte_strlcat(dst, src, size)
#endif

#else /* non-BSD platforms */
#ifdef RTE_USE_LIBBSD
#include <bsd/string.h>

#else /* no BSD header files, create own */
#define strlcpy(dst, src, size) rte_strlcpy(dst, src, size)
#define strlcat(dst, src, size) rte_strlcat(dst, src, size)

#endif /* RTE_USE_LIBBSD */
#endif /* FREEBSD */


#ifdef __cplusplus
}
#endif

#endif /* RTE_STRING_FNS_H */
