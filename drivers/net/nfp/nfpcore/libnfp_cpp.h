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

#ifndef __LIBNFP_CPP_H__
#define __LIBNFP_CPP_H__

#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "nfp-common/nfp_platform.h"

#define BUG_ON(x)               assert(!(x))

struct nfp_cpp_mutex;

struct nfp_cpp {
	uint32_t model;
	uint32_t interface;
	uint8_t *serial;
	int serial_len;
	void *priv;
	/* Mutex cache */
	struct nfp_cpp_mutex *mutex_cache;
	const struct nfp_cpp_operations *op;

	/* NFP-6xxx originating island IMB CPP Address Translation.
	 * CPP Target ID is index into array.
	 * Values are obtained at runtime from local island XPB CSRs. */
	uint32_t imb_cat_table[16];
	void *dl;		/* dlopen() handle */
};

struct nfp_cpp_area {
	struct nfp_cpp *cpp;
	char *name;
	unsigned long long offset;
	unsigned long size;
	/* Here follows the 'priv' part of nfp_cpp_area. */
};

uint32_t __nfp_cpp_model_autodetect(struct nfp_cpp *cpp);

int __nfp_cpp_explicit_read(struct nfp_cpp *cpp, uint32_t cpp_id, uint64_t addr,
			    void *buff, size_t len, int width_read);
int __nfp_cpp_explicit_write(struct nfp_cpp *cpp, uint32_t cpp_id, uint64_t addr,
			     const void *buff, size_t len, int width_write);

#ifdef _WIN32
typedef HANDLE fd_t;
#define FD_INVALID      INVALID_HANDLE_VALUE

static nfp_inline fd_t fd_open(const char *path, int flags, ...)
{
	int mode, share, create;

	switch (flags & O_ACCMODE) {
	case O_RDONLY:
		mode = GENERIC_READ;
		share = FILE_SHARE_READ;
		break;
	case O_RDWR:
		mode = GENERIC_READ | GENERIC_WRITE;
		share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	case O_WRONLY:
		mode = GENERIC_WRITE;
		share = FILE_SHARE_WRITE;
		break;
	default:
		errno = EINVAL;
		return FD_INVALID;
	}

	if (flags & O_CREAT)
		create = (flags & O_TRUNC) ? CREATE_ALWAYS : CREATE_NEW;
	else
		create = (flags & O_TRUNC) ? TRUNCATE_EXISTING : OPEN_EXISTING;

	return CreateFile(path, mode, share, NULL, create, 0, NULL);
}

static nfp_inline void fd_close(HANDLE fd)
{
	CloseHandle(fd);
}

#ifndef _MSC_VER
#define fd_ioctl(fd, cmd, argp) ({ \
	BOOL tf; \
	DWORD len = 0; \
	tf = DeviceIoControl(fd, cmd, argp, sizeof(*(argp)), \
	    argp, sizeof(*(argp)),  \
	    &len, NULL); \
	tf ? (int)len : -1; \
    })
#else
/* added static inline version of fd_ioctl function for Windows */
static nfp_inline int fd_ioctl_win32(fd_t fd, void *cmd, void *argp,
				     size_t argpSize)
{
	BOOL tf;
	DWORD len = 0;

	tf = DeviceIoControl(fd, (DWORD)cmd, (LPVOID) argp, argpSize,
			     argp, argpSize, &len, NULL);
	return tf ? (int)len : -1;
}
#endif

static nfp_inline ssize_t fd_pread(HANDLE fd, void *buf, size_t count,
				   nfp_off64_t offset)
{
#ifndef _MSC_VER
	OVERLAPPED location = { };
#else
	OVERLAPPED location;
#endif
	DWORD len = 0;
	BOOL tf;

#ifdef _MSC_VER
	memset(&location, 0, sizeof(location));
#endif
	location.Offset = (DWORD)offset;
	location.OffsetHigh = (DWORD) (offset >> 32);

	tf = ReadFile(fd, buf, count, &len, &location);
	if (!tf)
		return (ssize_t) -1;

	return (ssize_t)len;
}

static nfp_inline ssize_t fd_pwrite(HANDLE fd, const void *buf, size_t count,
				    nfp_off64_t offset)
{
#ifndef _MSC_VER
	OVERLAPPED location = { };
#else
	OVERLAPPED location;
#endif
	DWORD len = 0;
	BOOL tf;

#ifdef _MSC_VER
	memset(&location, 0, sizeof(location));
#endif
	location.Offset = (DWORD)offset;
	location.OffsetHigh = (DWORD) (offset >> 32);

	tf = WriteFile(fd, buf, count, &len, &location);
	if (!tf)
		return (ssize_t) -1;

	return (ssize_t)len;
}

#define O_NONBLOCK      _O_BINARY

static nfp_inline int getpagesize(void)
{
	SYSTEM_INFO sysinfo;

	GetSystemInfo(&sysinfo);
	return sysinfo.dwPageSize;
}
#else
typedef int fd_t;
#define FD_INVALID      (-1)

#define fd_open         open
#define fd_close        close
#define fd_pread        pread
#define fd_pwrite       pwrite
#define fd_ioctl        ioctl
#endif

#define NFP_DEBUG_ENV(envname) \
    static int _debug = -1; \
    static const char *_debug_env = "NFP_CPP_" #envname "_DEBUG";

#define Dn(n, x) do { \
    if (_debug < 0) { \
	const char *_cp = getenv(_debug_env); \
	_debug = 0; \
	if (_cp) \
	    _debug = strtoul(_cp, NULL, 0); \
    } \
    if (_debug & (1 << n)) { \
	x; \
    } \
} while (0)

#ifndef _MSC_VER
#define D0(x) Dn(0, x)
#define D1(x) Dn(1, x)
#define D(x)  D0(x)
#else
/* REVIEW DINANG - TEMPTEMP - need to implement the debug filters */
#define D(x)   x
#define D1(x)   x
#define D2(x)   x
#endif

#ifndef _MSC_VER
#define bug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
/* change variadic macro syntax for Windows */
#define bug(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#endif

#endif /* !__LIBNFP_CPP_H__ */
/* vim: set shiftwidth=4 expandtab: */
