/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          nfp_platform.h
 * @brief         Macros that aid portability.
 *
 */

#ifndef __NFP_PLATFORM_H__
#define __NFP_PLATFORM_H__

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

#ifndef BIT_ULL
#define BIT(x) (1 << (x))
#define BIT_ULL(x) (1ULL << (x))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define NFP_ERRNO(err) (errno = (err), -1)
#define NFP_ERRNO_RET(err, ret) (errno = (err), (ret))
#define NFP_NOERR(errv) (errno)
#define NFP_ERRPTR(err) (errno = (err), NULL)
#define NFP_PTRERR(errv) (errno)

/***** Host endianness *****/
#define NFP_ENDIAN_LITTLE 1234
#define NFP_ENDIAN_BIG 4321

#define NFP_SWAP16(_v_) \
	(((((uint16_t)(_v_)) & 0xFF) << 8) | ((((uint16_t)(_v_)) >> 8) & 0xFF))

#define NFP_SWAP32(_v_)                            \
	((((((uint32_t)(_v_)) & 0xFF) << 24)) |    \
	 (((((uint32_t)(_v_)) & 0xFF00) << 8)) |   \
	 (((((uint32_t)(_v_)) & 0xFF0000) >> 8)) | \
	 ((((uint32_t)(_v_)) >> 24) & 0xFF))

#define NFP_SWAP64(_v_)                            \
	(((((uint64_t)(_v_)) & 0xFF) << 56) |      \
	 ((((uint64_t)(_v_)) & 0xFF00) << 40) |    \
	 ((((uint64_t)(_v_)) & 0xFF0000) << 24) |  \
	 ((((uint64_t)(_v_)) & 0xFF000000) << 8) | \
	 ((((uint64_t)(_v_)) >> 8) & 0xFF000000) | \
	 ((((uint64_t)(_v_)) >> 24) & 0xFF0000) |  \
	 ((((uint64_t)(_v_)) >> 40) & 0xFF00) |    \
	 ((((uint64_t)(_v_)) >> 56) & 0xFF))

/***** Common macros which are not always available in standard headers *****/
#define NFP_MIN(_a_, _b_) ((_a_) < (_b_) ? (_a_) : (_b_))
#define NFP_MAX(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))

#if defined(_WIN32)
#  ifdef MAX_PATH
#    define NFP_PATH_MAX MAX_PATH
#  else
#    define NFP_PATH_MAX 260
#  endif
#elif defined(PATH_MAX)
#  define NFP_PATH_MAX PATH_MAX
#else
#  define NFP_PATH_MAX 40960	/* Arbitrary big number */
#endif

#if defined(_WIN32)
#  define NFP_HOST_ENDIAN NFP_ENDIAN_LITTLE
#elif defined(__KERNEL__)
#  ifdef __BIG_ENDIAN
#    define NFP_HOST_ENDIAN NFP_ENDIAN_BIG
#  else
#    define NFP_HOST_ENDIAN NFP_ENDIAN_LITTLE
#  endif
#elif defined(__unix__) || defined(__APPLE__) || defined(__OSX__)
#  include <sys/types.h>
#  include <sys/param.h>
#  if (BYTE_ORDER == LITTLE_ENDIAN)
#    define NFP_HOST_ENDIAN NFP_ENDIAN_LITTLE
#  elif (BYTE_ORDER == BIG_ENDIAN)
#    define NFP_HOST_ENDIAN NFP_ENDIAN_BIG
#  endif
#endif

#if (NFP_HOST_ENDIAN == NFP_ENDIAN_BIG)
#  define NFP_BETOH16(_v_) (_v_)
#  define NFP_BETOH32(_v_) (_v_)
#  define NFP_BETOH64(_v_) (_v_)
#  define NFP_LETOH16(_v_) NFP_SWAP16(_v_)
#  define NFP_LETOH32(_v_) NFP_SWAP32(_v_)
#  define NFP_LETOH64(_v_) NFP_SWAP64(_v_)
#  define NFP_HTOBE16(_v_) (_v_)
#  define NFP_HTOBE32(_v_) (_v_)
#  define NFP_HTOBE64(_v_) (_v_)
#  define NFP_HTOLE16(_v_) NFP_SWAP16(_v_)
#  define NFP_HTOLE32(_v_) NFP_SWAP32(_v_)
#  define NFP_HTOLE64(_v_) NFP_SWAP64(_v_)
#else
#  define NFP_LETOH16(_v_) (_v_)
#  define NFP_LETOH32(_v_) (_v_)
#  define NFP_LETOH64(_v_) (_v_)
#  define NFP_BETOH16(_v_) NFP_SWAP16(_v_)
#  define NFP_BETOH32(_v_) NFP_SWAP32(_v_)
#  define NFP_BETOH64(_v_) NFP_SWAP64(_v_)
#  define NFP_HTOLE16(_v_) (_v_)
#  define NFP_HTOLE32(_v_) (_v_)
#  define NFP_HTOLE64(_v_) (_v_)
#  define NFP_HTOBE16(_v_) NFP_SWAP16(_v_)
#  define NFP_HTOBE32(_v_) NFP_SWAP32(_v_)
#  define NFP_HTOBE64(_v_) NFP_SWAP64(_v_)
#endif

/***** Declarators *****/
#ifdef _MSC_VER			/* Visual C++ */
#  define nfp_inline __forceinline
#  ifndef ETIMEDOUT
#    define ETIMEDOUT 110
#  endif
#  ifndef ENOTSUP
#    define ENOTSUP 95
#  endif
#else
#  define nfp_inline inline
#endif

/***** snprintf and other text processing aliases *****/
#ifdef _MSC_VER /* Visual C++ */

#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup
#define unlink _unlink
#define __func__ __FUNCTION__
/* HAVE_* for things like wxWidgets */
#define HAVE_SNPRINTF_DECL 1

#if (_MSC_VER < 1900) /* VC++ 2015 has snprintf */
#ifdef snprintf
/* Most users just do snprintf == _snprintf, which is not exactly the same as
 * the usual snprintf. Generally the
 * differences do not affect most users, but rather be safe.
 */
#undef snprintf
#endif
#define snprintf nfp_snprintf
#endif

#include <stdio.h>
#include <stdarg.h>

static __inline int nfp_snprintf(char *str, size_t size, const char *format, ...)
{
	int ret = -1;
	va_list args;

	va_start(args, format);
	/* First try printing */
	if (size > 0)
		ret = _vsnprintf_s(str, size, _TRUNCATE, format, args);
	/* then count would-print if error */
	if (ret == -1)
		ret = _vscprintf(format, args);

	va_end(args);
	return ret;
}

static __inline int setenv(const char *name, const char *value, int overwrite)
{
	if (!overwrite && !!getenv(name))
		return 0;
	return (int)_putenv_s(name, value);
}

#define nfp_strtou32 (uint32_t)strtoul
#define nfp_strtoi32 (int32_t)strtol
#define nfp_strtou64 (uint64_t)_strtoui64
#define nfp_strtoi64 (int64_t)_strtoi64

#if (!defined(__cplusplus) || defined(__STDC_FORMAT_MACROS))
#  ifndef PRIdSZ
#    define PRIdSZ "Id"
#  endif
#  ifndef PRIiSZ
#    define PRIiSZ "Ii"
#  endif
#  ifndef PRIuSZ
#    define PRIuSZ "Iu"
#  endif
#  ifndef PRIoSZ
#    define PRIoSZ "Io"
#  endif
#  ifndef PRIxSZ
#    define PRIxSZ "Ix"
#  endif
#  ifndef PRIXSZ
#    define PRIXSZ "IX"
#  endif

#  ifndef SCNdSZ
#    define SCNdSZ "Id"
#  endif
#  ifndef SCNiSZ
#    define SCNiSZ "Ii"
#  endif
#  ifndef SCNuSZ
#    define SCNuSZ "Iu"
#  endif
#  ifndef SCNoSZ
#    define SCNoSZ "Io"
#  endif
#  ifndef SCNxSZ
#    define SCNxSZ "Ix"
#  endif
#endif

#define NFP_PATH_SEP '\\'
#define NFP_PATH_LIST_SEP ';'

typedef __int64 nfp_off64_t;
#define nfp_fseek64 _fseeki64
#define nfp_ftell64 _ftelli64

#define nfp_open_fd _open
#define nfp_close_fd _close
#define nfp_lseek_fd _lseek
#define nfp_write_fd _write
#define nfp_read_fd _read

#else /* ! Visual C++ */

#define nfp_snprintf snprintf
#define nfp_strtou32 (uint32_t)strtoul
#define nfp_strtoi32 (int32_t)strtol
#define nfp_strtou64 (uint64_t)strtoull
#define nfp_strtoi64 (int64_t)strtoll

#if (!defined(__cplusplus) || defined(__STDC_FORMAT_MACROS))
#  ifndef PRIdSZ
#    define PRIdSZ "zd"
#  endif
#  ifndef PRIiSZ
#    define PRIiSZ "zi"
#  endif
#  ifndef PRIuSZ
#    define PRIuSZ "zu"
#  endif
#  ifndef PRIoSZ
#    define PRIoSZ "zo"
#  endif
#  ifndef PRIxSZ
#    define PRIxSZ "zx"
#  endif
#  ifndef PRIXSZ
#    define PRIXSZ "zX"
#  endif
#  ifndef SCNdSZ
#    define SCNdSZ "zd"
#  endif
#  ifndef SCNiSZ
#    define SCNiSZ "zi"
#  endif
#  ifndef SCNuSZ
#    define SCNuSZ "zu"
#  endif
#  ifndef SCNoSZ
#    define SCNoSZ "zo"
#  endif
#  ifndef SCNxSZ
#    define SCNxSZ "zx"
#  endif
#endif

#define NFP_PATH_SEP '/'
#define NFP_PATH_LIST_SEP ':'

typedef off_t nfp_off64_t;
#define nfp_fseek64 fseeko
#define nfp_ftell64 ftello

#define nfp_open_fd open
#define nfp_close_fd close
#define nfp_lseek_fd lseek
#define nfp_write_fd write
#define nfp_read_fd read

#if defined(__MINGW64__) && defined(_WIN32)
static inline int setenv(const char *name, const char *value, int overwrite)
{
	int ret;
	char *s;
	size_t len;

	if (!overwrite && !!getenv(name))
		return 0;

	len = strlen(name) + 1 + strlen(value) + 1;
	s = (char *)calloc(len, 1);
	if (!s)
		return -1;
	strcat(s, name);
	strcat(s, "=");
	strcat(s, value);
	ret = _putenv(s);
	free(s);
	return ret;
}
#endif
#endif /* Visual C++ */

#ifdef _MSC_VER /* Visual C++ */
#define nfp_sleep_ms(milliseconds) Sleep(milliseconds)
#else
#define nfp_sleep_ms(milliseconds) usleep((milliseconds) * 1000)
#endif

/***** File related aliases *****/
#ifdef _MSC_VER /* Visual C++ */
static nfp_inline int nfp_mkstemp(char *tmpl)
{
	errno_t err;

	err = _mktemp_s(tmpl, strlen(tmpl) + 1);
	if (err != 0) {
		errno = err;
		return -1;
	}

	return nfp_open_fd(tmpl,
			   O_CREAT | O_TRUNC | O_RDWR | O_BINARY,
			   _S_IREAD | _S_IWRITE);
}
#else
#define nfp_mkstemp mkstemp
#endif

#ifdef _MSC_VER

#ifdef REQUIRE_GETTIMEOFDAY
/* required for timeval definition */
#include <WinSock2.h>

/*
 *    Win32 gettimeofday() replacement
 *
 * Copyright (c) 2003 SRA, Inc.
 * Copyright (c) 2003 SKC, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without a
 * written agreement is hereby granted, provided that the above
 * copyright notice and this paragraph and the following two
 * paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS
 * IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */
static int nfp_inline gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	/* FILETIME of Jan 1 1970 00:00:00. */
	static const unsigned __int64 epoch =
		((unsigned __int64)116444736000000000ULL);
	FILETIME file_time;
	SYSTEMTIME system_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;
	tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
#endif

#define usleep(waitTime)                                                   \
	do {                                                               \
		__int64 time1 = 0, time2 = 0, freq = 0;                    \
									   \
		QueryPerformanceCounter((ULARGE_INTEGER *)&time1);         \
		QueryPerformanceFrequency((ULARGE_INTEGER *)&freq);        \
									   \
		do {                                                       \
			QueryPerformanceCounter((ULARGE_INTEGER *)&time2); \
		} while ((time2 - time1) < (waitTime));                    \
	} while (0)

#endif

#endif /* __NFP_PLATFORM_H__ */
