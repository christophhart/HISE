/*
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#define __STDC_WANT_LIB_EXT1__ 1

#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER) ||              \
    defined(__MINGW32__)
/* Win32, DOS, MSVC, MSVS */
#include <direct.h>

#define STRCLONE(STR) ((STR) ? _strdup(STR) : NULL)
#define HAS_DEVICE(P)                                                          \
  ((((P)[0] >= 'A' && (P)[0] <= 'Z') || ((P)[0] >= 'a' && (P)[0] <= 'z')) &&   \
   (P)[1] == ':')
#define FILESYSTEM_PREFIX_LEN(P) (HAS_DEVICE(P) ? 2 : 0)

#else

#include <unistd.h> // needed for symlink()
#define STRCLONE(STR) ((STR) ? strdup(STR) : NULL)

#endif

#ifdef __MINGW32__
#include <sys/types.h>
#include <unistd.h>
#endif

#include "miniz.h"
#include "zip.h"

#ifdef _MSC_VER
#include <io.h>

#define ftruncate(fd, sz) (-(_chsize_s((fd), (sz)) != 0))
#define fileno _fileno
#endif

#if defined(__TINYC__) && (defined(_WIN32) || defined(_WIN64))
#include <io.h>

#define ftruncate(fd, sz) (-(_chsize_s((fd), (sz)) != 0))
#define fileno _fileno
#endif

#ifndef HAS_DEVICE
#define HAS_DEVICE(P) 0
#endif

#ifndef FILESYSTEM_PREFIX_LEN
#define FILESYSTEM_PREFIX_LEN(P) 0
#endif

#ifndef ISSLASH
#define ISSLASH(C) ((C) == '/' || (C) == '\\')
#endif

#define CLEANUP(ptr)                                                           \
  do {                                                                         \
    if (ptr) {                                                                 \
      free((void *)ptr);                                                       \
      ptr = NULL;                                                              \
    }                                                                          \
  } while (0)

#define UNX_IFDIR 0040000  /* Unix directory */
#define UNX_IFREG 0100000  /* Unix regular file */
#define UNX_IFSOCK 0140000 /* Unix socket (BSD, not SysV or Amiga) */
#define UNX_IFLNK 0120000  /* Unix symbolic link (not SysV, Amiga) */
#define UNX_IFBLK 0060000  /* Unix block special       (not Amiga) */
#define UNX_IFCHR 0020000  /* Unix character special   (not Amiga) */
#define UNX_IFIFO 0010000  /* Unix fifo    (BCC, not MSC or Amiga) */

struct zip_entry_t {
  ssize_t index;
  char *name;
  mz_uint64 uncomp_size;
  mz_uint64 comp_size;
  mz_uint32 uncomp_crc32;
  mz_uint64 offset;
  mz_uint8 header[MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
  mz_uint64 header_offset;
  mz_uint16 method;
  mz_zip_writer_add_state state;
  tdefl_compressor comp;
  mz_uint32 external_attr;
  time_t m_time;
};

struct zip_t {
  mz_zip_archive archive;
  mz_uint level;
  struct zip_entry_t entry;
};

enum zip_modify_t {
  MZ_KEEP = 0,
  MZ_DELETE = 1,
  MZ_MOVE = 2,
};

struct zip_entry_mark_t {
  ssize_t file_index;
  enum zip_modify_t type;
  mz_uint64 m_local_header_ofs;
  size_t lf_length;
};



static char *zip_strrpl(const char *str, size_t n, char oldchar, char newchar) {
  char c;
  size_t i;
  char *rpl = (char *)calloc((1 + n), sizeof(char));
  char *begin = rpl;
  if (!rpl) {
    return NULL;
  }

  for (i = 0; (i < n) && (c = *str++); ++i) {
    if (c == oldchar) {
      c = newchar;
    }
    *rpl++ = c;
  }

  return begin;
}

static int zip_archive_truncate(mz_zip_archive *pzip) {
  mz_zip_internal_state *pState = pzip->m_pState;
  mz_uint64 file_size = pzip->m_archive_size;
  if ((pzip->m_pWrite == mz_zip_heap_write_func) && (pState->m_pMem)) {
    return 0;
  }
  if (pzip->m_zip_mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED) {
    if (pState->m_pFile) {
      int fd = fileno(pState->m_pFile);
      return ftruncate(fd, file_size);
    }
  }
  return 0;
}

static inline void zip_archive_finalize(mz_zip_archive *pzip) {
  mz_zip_writer_finalize_archive(pzip);
  zip_archive_truncate(pzip);
}

int zip_entry_openbyindex(struct zip_t *zip, size_t index) {
  mz_zip_archive *pZip = NULL;
  mz_zip_archive_file_stat stats;
  mz_uint namelen;
  const mz_uint8 *pHeader;
  const char *pFilename;

  if (!zip) {
    // zip_t handler is not initialized
    return ZIP_ENOINIT;
  }

  pZip = &(zip->archive);
  if (pZip->m_zip_mode != MZ_ZIP_MODE_READING) {
    // open by index requires readonly mode
    return ZIP_EINVMODE;
  }

  if (index >= (size_t)pZip->m_total_files) {
    // index out of range
    return ZIP_EINVIDX;
  }

  if (!(pHeader = &MZ_ZIP_ARRAY_ELEMENT(
            &pZip->m_pState->m_central_dir, mz_uint8,
            MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets,
                                 mz_uint32, index)))) {
    // cannot find header in central directory
    return ZIP_ENOHDR;
  }

  namelen = MZ_READ_LE16(pHeader + MZ_ZIP_CDH_FILENAME_LEN_OFS);
  pFilename = (const char *)pHeader + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;

  /*
    .ZIP File Format Specification Version: 6.3.3

    4.4.17.1 The name of the file, with optional relative path.
    The path stored MUST not contain a drive or
    device letter, or a leading slash.  All slashes
    MUST be forward slashes '/' as opposed to
    backwards slashes '\' for compatibility with Amiga
    and UNIX file systems etc.  If input came from standard
    input, there is no file name field.
  */
  if (zip->entry.name) {
    CLEANUP(zip->entry.name);
  }
#ifdef ZIP_RAW_ENTRYNAME
  zip->entry.name = STRCLONE(pFilename);
#else
  zip->entry.name = zip_strrpl(pFilename, namelen, '\\', '/');
#endif

  if (!zip->entry.name) {
    // local entry name is NULL
    return ZIP_EINVENTNAME;
  }

  if (!mz_zip_reader_file_stat(pZip, (mz_uint)index, &stats)) {
    return ZIP_ENOENT;
  }

  zip->entry.index = (ssize_t)index;
  zip->entry.comp_size = stats.m_comp_size;
  zip->entry.uncomp_size = stats.m_uncomp_size;
  zip->entry.uncomp_crc32 = stats.m_crc32;
  zip->entry.offset = stats.m_central_dir_ofs;
  zip->entry.header_offset = stats.m_local_header_ofs;
  zip->entry.method = stats.m_method;
  zip->entry.external_attr = stats.m_external_attr;
#ifndef MINIZ_NO_TIME
  zip->entry.m_time = stats.m_time;
#endif

  return 0;
}

int zip_entry_close(struct zip_t *zip) {
  mz_zip_archive *pzip = NULL;
  mz_uint level;
  tdefl_status done;
  mz_uint16 entrylen;
  mz_uint16 dos_time = 0, dos_date = 0;
  int err = 0;
  mz_uint8 *pExtra_data = NULL;
  mz_uint32 extra_size = 0;
  mz_uint8 extra_data[MZ_ZIP64_MAX_CENTRAL_EXTRA_FIELD_SIZE];
  mz_uint8 local_dir_footer[MZ_ZIP_DATA_DESCRIPTER_SIZE64];
  mz_uint32 local_dir_footer_size = MZ_ZIP_DATA_DESCRIPTER_SIZE64;

  if (!zip) {
    // zip_t handler is not initialized
    err = ZIP_ENOINIT;
    goto cleanup;
  }

  pzip = &(zip->archive);
  if (pzip->m_zip_mode == MZ_ZIP_MODE_READING) {
    goto cleanup;
  }

  level = zip->level & 0xF;
  if (level) {
    done = tdefl_compress_buffer(&(zip->entry.comp), "", 0, TDEFL_FINISH);
    if (done != TDEFL_STATUS_DONE && done != TDEFL_STATUS_OKAY) {
      // Cannot flush compressed buffer
      err = ZIP_ETDEFLBUF;
      goto cleanup;
    }
    zip->entry.comp_size = zip->entry.state.m_comp_size;
    zip->entry.offset = zip->entry.state.m_cur_archive_file_ofs;
    zip->entry.method = MZ_DEFLATED;
  }

  entrylen = (mz_uint16)strlen(zip->entry.name);
#ifndef MINIZ_NO_TIME
  mz_zip_time_t_to_dos_time(zip->entry.m_time, &dos_time, &dos_date);
#endif

  MZ_WRITE_LE32(local_dir_footer + 0, MZ_ZIP_DATA_DESCRIPTOR_ID);
  MZ_WRITE_LE32(local_dir_footer + 4, zip->entry.uncomp_crc32);
  MZ_WRITE_LE64(local_dir_footer + 8, zip->entry.comp_size);
  MZ_WRITE_LE64(local_dir_footer + 16, zip->entry.uncomp_size);

  if (pzip->m_pWrite(pzip->m_pIO_opaque, zip->entry.offset, local_dir_footer,
                     local_dir_footer_size) != local_dir_footer_size) {
    // Cannot write zip entry header
    err = ZIP_EWRTHDR;
    goto cleanup;
  }
  zip->entry.offset += local_dir_footer_size;

  pExtra_data = extra_data;
  extra_size = mz_zip_writer_create_zip64_extra_data(
      extra_data,
      (zip->entry.uncomp_size >= MZ_UINT32_MAX) ? &zip->entry.uncomp_size
                                                : NULL,
      (zip->entry.comp_size >= MZ_UINT32_MAX) ? &zip->entry.comp_size : NULL,
      (zip->entry.header_offset >= MZ_UINT32_MAX) ? &zip->entry.header_offset
                                                  : NULL);

  if ((entrylen) && (zip->entry.name[entrylen - 1] == '/') &&
      !zip->entry.uncomp_size) {
    /* Set DOS Subdirectory attribute bit. */
    zip->entry.external_attr |= MZ_ZIP_DOS_DIR_ATTRIBUTE_BITFLAG;
  }

  if (!mz_zip_writer_add_to_central_dir(
          pzip, zip->entry.name, entrylen, pExtra_data, (mz_uint16)extra_size,
          "", 0, zip->entry.uncomp_size, zip->entry.comp_size,
          zip->entry.uncomp_crc32, zip->entry.method,
          MZ_ZIP_GENERAL_PURPOSE_BIT_FLAG_UTF8 |
              MZ_ZIP_LDH_BIT_FLAG_HAS_LOCATOR,
          dos_time, dos_date, zip->entry.header_offset,
          zip->entry.external_attr, NULL, 0)) {
    // Cannot write to zip central dir
    err = ZIP_EWRTDIR;
    goto cleanup;
  }

  pzip->m_total_files++;
  pzip->m_archive_size = zip->entry.offset;

cleanup:
  if (zip) {
    zip->entry.m_time = 0;
    CLEANUP(zip->entry.name);
  }
  return err;
}

const char *zip_entry_name(struct zip_t *zip) {
  if (!zip) {
    // zip_t handler is not initialized
    return NULL;
  }

  return zip->entry.name;
}

int zip_entry_write(struct zip_t *zip, const void *buf, size_t bufsize) {
  mz_uint level;
  mz_zip_archive *pzip = NULL;
  tdefl_status status;

  if (!zip) {
    // zip_t handler is not initialized
    return ZIP_ENOINIT;
  }

  pzip = &(zip->archive);
  if (buf && bufsize > 0) {
    zip->entry.uncomp_size += bufsize;
    zip->entry.uncomp_crc32 = (mz_uint32)mz_crc32(
        zip->entry.uncomp_crc32, (const mz_uint8 *)buf, bufsize);

    level = zip->level & 0xF;
    if (!level) {
      if ((pzip->m_pWrite(pzip->m_pIO_opaque, zip->entry.offset, buf,
                          bufsize) != bufsize)) {
        // Cannot write buffer
        return ZIP_EWRTENT;
      }
      zip->entry.offset += bufsize;
      zip->entry.comp_size += bufsize;
    } else {
      status = tdefl_compress_buffer(&(zip->entry.comp), buf, bufsize,
                                     TDEFL_NO_FLUSH);
      if (status != TDEFL_STATUS_DONE && status != TDEFL_STATUS_OKAY) {
        // Cannot compress buffer
        return ZIP_ETDEFLBUF;
      }
    }
  }

  return 0;
}

ssize_t zip_entry_read(struct zip_t *zip, void **buf, size_t *bufsize) {
  mz_zip_archive *pzip = NULL;
  mz_uint idx;
  size_t size = 0;

  if (!zip) {
    // zip_t handler is not initialized
    return (ssize_t)ZIP_ENOINIT;
  }

  pzip = &(zip->archive);
  if (pzip->m_zip_mode != MZ_ZIP_MODE_READING ||
      zip->entry.index < (ssize_t)0) {
    // the entry is not found or we do not have read access
    return (ssize_t)ZIP_ENOENT;
  }

  idx = (mz_uint)zip->entry.index;
  if (mz_zip_reader_is_file_a_directory(pzip, idx)) {
    // the entry is a directory
    return (ssize_t)ZIP_EINVENTTYPE;
  }

  *buf = mz_zip_reader_extract_to_heap(pzip, idx, &size, 0);
  if (*buf && bufsize) {
    *bufsize = size;
  }
  return (ssize_t)size;
}

struct zip_t *zip_stream_open(const char *stream, size_t size, int level,
                              char mode) {
  struct zip_t *zip = (struct zip_t *)calloc((size_t)1, sizeof(struct zip_t));
  if (!zip) {
    return NULL;
  }

  if (level < 0) {
    level = MZ_DEFAULT_LEVEL;
  }
  if ((level & 0xF) > MZ_UBER_COMPRESSION) {
    // Wrong compression level
    goto cleanup;
  }
  zip->level = (mz_uint)level;

  if ((stream != NULL) && (size > 0) && (mode == 'r')) {
    if (!mz_zip_reader_init_mem(&(zip->archive), stream, size, 0)) {
      goto cleanup;
    }
  } else if ((stream == NULL) && (size == 0) && (mode == 'w')) {
    // Create a new archive.
    if (!mz_zip_writer_init_heap(&(zip->archive), 0, 1024)) {
      // Cannot initialize zip_archive writer
      goto cleanup;
    }
  } else {
    goto cleanup;
  }
  return zip;

cleanup:
  CLEANUP(zip);
  return NULL;
}

void zip_stream_close(struct zip_t *zip) {
  if (zip) {
    mz_zip_writer_end(&(zip->archive));
    mz_zip_reader_end(&(zip->archive));
    CLEANUP(zip);
  }
}