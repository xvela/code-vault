/*
Copyright c1997-2014 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 4.1
http://www.bombaydigital.com/
License: MIT. See LICENSE.md in the Vault top level directory.
*/

/** @file */

#include "vtypes.h"

#include "vstring.h"
#include "vexception.h"
#include "vtypes_internal.h"

#include <errno.h>

Vs64 vault::VgetMemoryUsage() {
    return 0; // FIXME - find an API to use on Mac
}

// On Mac OS X, it is usually most convenient if we use Unix line endings
// (0x0A) rather than Classic Mac OS 9 line endings (0x0D), because many
// of the Unix tools barf on Classic line endings.

static const Vu8 kUnixLineEnding = 0x0A;

const Vu8* vault::VgetNativeLineEnding(int& numBytes) {
    numBytes = 1;
    return &kUnixLineEnding;
}

// VSystemError ---------------------------------------------------------------
// Platform-specific implementation of VSystemError internal accessors.

// static
int VSystemError::_getSystemErrorCode() {
    return errno;
}

// static
int VSystemError::_getSocketErrorCode() {
    return errno;
}

// static
VString VSystemError::_getSystemErrorMessage(int errorCode) {
    return ::strerror(errorCode);
}

bool VSystemError::_isLikePosixError(int posixErrorCode) const {
    // We are POSIX. No translation necessary.
    return (posixErrorCode == mErrorCode);
}

// VPlatformAPI -----------------------------------------------------------------

// static
VString VPlatformAPI::getcwd() {
    VString result;
    result.preflight(PATH_MAX);
    char* cwdResult = vault::getcwd(result.buffer(), PATH_MAX);
    if (cwdResult == NULL) {
        throw VException(VSystemError(), "Call to getcwd failed.");
    }
    
    result.postflight((int) ::strlen(cwdResult));
    return result;
}

// static
int VPlatformAPI::open(const VString& path, int flags, mode_t mode) {
    return vault::open(path, flags, mode);
}

// static
FILE* VPlatformAPI::fopen(const VString& path, const char* mode) {
    return vault::fopen(path, mode);
}

// static
int VPlatformAPI::mkdir(const VString& path, mode_t mode) {
    return vault::mkdir(path, mode);
}

// static
int VPlatformAPI::rmdir(const VString& path) {
    return vault::rmdir(path);
}

// static
int VPlatformAPI::unlink(const VString& path) {
    return vault::unlink(path);
}

// static
int VPlatformAPI::rename(const VString& oldName, const VString& newName) {
    return vault::rename(oldName, newName);
}

// static
int VPlatformAPI::stat(const VString& path, struct stat* buf) {
    return vault::stat(path, buf);
}

