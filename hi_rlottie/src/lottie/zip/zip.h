/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#ifndef ZIP_H
#define ZIP_H

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#if !defined(_POSIX_C_SOURCE) && defined(_MSC_VER)
// 64-bit Windows is the only mainstream platform
// where sizeof(long) != sizeof(void*)
#ifdef _WIN64
typedef long long ssize_t; /* byte count or error */
#else
typedef long ssize_t; /* byte count or error */
#endif
#endif

/**
 * Default zip compression level.
 */
#define ZIP_DEFAULT_COMPRESSION_LEVEL 6

/**
 * Error codes
 */
#define ZIP_ENOINIT -1      // not initialized
#define ZIP_EINVENTNAME -2  // invalid entry name
#define ZIP_ENOENT -3       // entry not found
#define ZIP_EINVMODE -4     // invalid zip mode
#define ZIP_EINVLVL -5      // invalid compression level
#define ZIP_ENOSUP64 -6     // no zip 64 support
#define ZIP_EMEMSET -7      // memset error
#define ZIP_EWRTENT -8      // cannot write data to entry
#define ZIP_ETDEFLINIT -9   // cannot initialize tdefl compressor
#define ZIP_EINVIDX -10     // invalid index
#define ZIP_ENOHDR -11      // header not found
#define ZIP_ETDEFLBUF -12   // cannot flush tdefl buffer
#define ZIP_ECRTHDR -13     // cannot create entry header
#define ZIP_EWRTHDR -14     // cannot write entry header
#define ZIP_EWRTDIR -15     // cannot write to central dir
#define ZIP_EOPNFILE -16    // cannot open file
#define ZIP_EINVENTTYPE -17 // invalid entry type
#define ZIP_EMEMNOALLOC -18 // extracting data using no memory allocation
#define ZIP_ENOFILE -19     // file not found
#define ZIP_ENOPERM -20     // no permission
#define ZIP_EOOMEM -21      // out of memory
#define ZIP_EINVZIPNAME -22 // invalid zip archive name
#define ZIP_EMKDIR -23      // make dir error
#define ZIP_ESYMLINK -24    // symlink error
#define ZIP_ECLSZIP -25     // close archive error
#define ZIP_ECAPSIZE -26    // capacity size too small
#define ZIP_EFSEEK -27      // fseek error
#define ZIP_EFREAD -28      // fread error
#define ZIP_EFWRITE -29     // fwrite error

/**
 * @struct zip_t
 *
 * This data structure is used throughout the library to represent zip archive -
 * forward declaration.
 */
struct zip_t;

/**
 * Closes the zip archive, releases resources - always finalize.
 *
 * @param zip zip archive handler.
 */
void zip_close(struct zip_t *zip);

/**
 * Opens a new entry by index in the zip archive.
 *
 * This function is only valid if zip archive was opened in 'r' (readonly) mode.
 *
 * @param zip zip archive handler.
 * @param index index in local dictionary.
 *
 * @return the return code - 0 on success, negative number (< 0) on error.
 */
int zip_entry_openbyindex(struct zip_t *zip, size_t index);

/**
 * Closes a zip entry, flushes buffer and releases resources.
 *
 * @param zip zip archive handler.
 *
 * @return the return code - 0 on success, negative number (< 0) on error.
 */
int zip_entry_close(struct zip_t *zip);

/**
 * Returns a local name of the current zip entry.
 *
 * The main difference between user's entry name and local entry name
 * is optional relative path.
 * Following .ZIP File Format Specification - the path stored MUST not contain
 * a drive or device letter, or a leading slash.
 * All slashes MUST be forward slashes '/' as opposed to backwards slashes '\'
 * for compatibility with Amiga and UNIX file systems etc.
 *
 * @param zip: zip archive handler.
 *
 * @return the pointer to the current zip entry name, or NULL on error.
 */
const char *zip_entry_name(struct zip_t *zip);

/**
 * Extracts the current zip entry into output buffer.
 *
 * The function allocates sufficient memory for a output buffer.
 *
 * @param zip zip archive handler.
 * @param buf output buffer.
 * @param bufsize output buffer size (in bytes).
 *
 * @note remember to release memory allocated for a output buffer.
 *       for large entries, please take a look at zip_entry_extract function.
 *
 * @return the return code - the number of bytes actually read on success.
 *         Otherwise a negative number (< 0) on error.
 */
ssize_t zip_entry_read(struct zip_t *zip, void **buf,
                                         size_t *bufsize);
/**
 * Opens zip archive stream into memory.
 *
 * @param stream zip archive stream.
 * @param size stream size.
 *
 * @return the zip archive handler or NULL on error
 */
struct zip_t *zip_stream_open(const char *stream, size_t size,
                                                int level, char mode);

/**
 * Close zip archive releases resources.
 *
 * @param zip zip archive handler.
 *
 * @return
 */
void zip_stream_close(struct zip_t *zip);

#endif
