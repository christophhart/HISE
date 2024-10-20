/*
 * This is the Loris C++ Class Library, implementing analysis, 
 * manipulation, and synthesis of digitized sounds using the Reassigned 
 * Bandwidth-Enhanced Additive Sound Model.
 *
 * Loris is Copyright (c) 1999-2010 by Kelly Fitz and Lippold Haken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * SdifFile.C
 *
 * Implementation of class SdifFile, which reads and writes SDIF files.
 *
 * Lippold Haken, 4 July 2000, using CNMAT SDIF library
 * Lippold Haken, 20 October 2000, using IRCAM SDIF library (tutorial by Diemo Schwarz)
 * Lippold Haken, 22 December 2000, using 1LBL frames
 * Lippold Haken, 27 March 2001, write only 7-column 1TRC, combine reading and writing classes
 * Lippold Haken, 31 Jan 2002, write either 4-column 1TRC or 6-column RBEP
 * Lippold Haken, 20 Apr 2004, back to using CNMAT SDIF library
 * Lippold Haken, 06 Oct 2004, write 64-bit float files, read both 32-bit and 64-bit float
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

/*

Portions of this code are from the CNMAT SDIF library.

Copyright (c) 1996. 1997, 1998, 1999.  The Regents of the University of California
(Regents). All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation, without fee and without a signed licensing agreement, is hereby
granted, provided that the above copyright notice, this paragraph and the
following two paragraphs appear in all copies, modifications, and
distributions.  Contact The Office of Technology Licensing, UC Berkeley, 2150
Shattuck Avenue, Suite 510, Berkeley, CA 94720-1620, (510) 643-7201, for
commercial licensing opportunities.

Written by Matt Wright, Amar Chaudhary, and Sami Khoury, The Center for New
Music and Audio Technologies, University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
     PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
     DOCUMENTATION, EVEN IF REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF
     SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

SDIF spec: http://www.cnmat.berkeley.edu/SDIF/

*/

#if HAVE_CONFIG_H
	#include "config.h"
#endif

#include "SdifFile.h"
#include "LorisExceptions.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialPtrs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <list>
#include <string>
#include <vector>

#if HAVE_M_PI
	const double Pi = M_PI;
#else
	const double Pi = 3.14159265358979324;
#endif

using namespace std;

//	begin namespace
namespace Loris {

// -- CNMAT SDIF definitions --
// ---------------------------------------------------------------------------
//	CNMAT SDIF types
// ---------------------------------------------------------------------------

//	try to use the information gathered by configure -- if not using 
//  config.h, then pick some (hopefully-) reasonable values for
//	these things and hope for the best...
#if ! defined( SIZEOF_SHORT )
#define SIZEOF_SHORT 2
#endif

#if ! defined( SIZEOF_INT )
#define SIZEOF_INT 4
#endif

#if ! defined( SIZEOF_LONG )
#define SIZEOF_LONG 4	
#endif

#if defined(SIZEOF_SHORT) && (SIZEOF_SHORT == 2)
typedef unsigned short  sdif_unicode;
typedef short           sdif_int16;
#elif defined(SIZEOF_INT) && (SIZEOF_INT == 2)
typedef unsigned int    sdif_unicode;
typedef int             sdif_int16;
#else
#error "SdifFile.C: cannot identify a two-byte integer type, define SIZEOF_SHORT or SIZEOF_INT"
#endif

#if defined(SIZEOF_INT) && (SIZEOF_INT == 4)
typedef int             sdif_int32;
typedef unsigned int    sdif_uint32;
#elif defined(SIZEOF_LONG) && (SIZEOF_LONG == 4)
typedef long            sdif_int32;
typedef unsigned long   sdif_uint32;
#else
#error "SdifFile.C: cannot identify a four-byte integer type, define SIZEOF_INT or SIZEOF_LONG"
#endif

// It is probably an unnecessary nuisance to check these, since there's
// no alternative type to try. Requiring builders to define these 
// symbols doesn't really serve any purpose. C++ doesn't provide a 
// standard way to find a data type having a particular size 
//  (as stdint.h does for interger types in C99).
/*
#if SIZEOF_FLOAT == 4
typedef float           sdif_float32;
#else
#error "SdifFile.C: cannot identify a four-byte floating-point type"
#endif

#if SIZEOF_DOUBLE == 8
typedef double          sdif_float64;
#else
#error "SdifFile.C: cannot identify a eight-byte floating-point type"
#endif
*/
typedef float           sdif_float32;
typedef double          sdif_float64;



// ---------------------------------------------------------------------------
//	SDIF_GlobalHeader
// ---------------------------------------------------------------------------
typedef struct {
    char  SDIF[4];          /* must be 'S', 'D', 'I', 'F' */
    sdif_int32 size;        /* size of header frame, not including SDIF or size. */
    sdif_int32 SDIFversion;
    sdif_int32 SDIFStandardTypesVersion;
} SDIF_GlobalHeader;

// ---------------------------------------------------------------------------
//	SDIF_FrameHeader
// ---------------------------------------------------------------------------
typedef struct {
    char         frameType[4];        /* should be a registered frame type */
    sdif_int32   size;                /* # bytes in this frame, not including
                                         frameType or size */
    sdif_float64 time;                /* time corresponding to frame */
    sdif_int32   streamID;            /* frames that go together have the same ID */
    sdif_int32   matrixCount;         /* number of matrices in frame */
} SDIF_FrameHeader;


// ---------------------------------------------------------------------------
//	SDIF_MatrixHeader
// ---------------------------------------------------------------------------
typedef struct {
    char matrixType[4];
    sdif_int32 matrixDataType;
    sdif_int32 rowCount;
    sdif_int32 columnCount;
} SDIF_MatrixHeader;


/* Version numbers for SDIF_GlobalHeader associated with this library */
#define SDIF_SPEC_VERSION 3
#define SDIF_LIBRARY_VERSION 1

// ---------------------------------------------------------------------------
//	Enumerations for type definitions in matrices.
// ---------------------------------------------------------------------------
typedef enum {
    SDIF_FLOAT32 = 0x0004,
    SDIF_FLOAT64 = 0x0008,
    SDIF_INT16 = 0x0102,
    SDIF_INT32 = 0x0104,
    SDIF_INT64 = 0x0108,
    SDIF_UINT32 = 0x0204,
    SDIF_UTF8 = 0x0301,
    SDIF_BYTE = 0x0401,
    SDIF_NO_TYPE = -1
} SDIF_MatrixDataType;

typedef enum {
    SDIF_FLOAT = 0,
    SDIF_INT = 1,
    SDIF_UINT = 2,
    SDIF_TEXT = 3,
    SDIF_ARBITRARY = 4
} SDIF_MatrixDataTypeHighOrder;

/* SDIF_GetMatrixDataTypeSize --
   Find the size in bytes of the data type indicated by "d" */
#define SDIF_GetMatrixDataTypeSize(d) ((d) & 0xff)

// -- CNMAT SDIF errors --
// ---------------------------------------------------------------------------
//	CNMAT SDIF error handling machinery.
// ---------------------------------------------------------------------------
typedef enum {
    ESDIF_SUCCESS=0,
    ESDIF_SEE_ERRNO=1,
    ESDIF_BAD_SDIF_HEADER=2,
    ESDIF_BAD_FRAME_HEADER=3,
    ESDIF_SKIP_FAILED=4,
    ESDIF_BAD_MATRIX_DATA_TYPE=5,
    ESDIF_BAD_SIZEOF=6,
    ESDIF_END_OF_DATA=7,  /* Not necessarily an error */
    ESDIF_BAD_MATRIX_HEADER=8,
    ESDIF_OBSOLETE_FILE_VERSION=9,
    ESDIF_OBSOLETE_TYPES_VERSION=10,
    ESDIF_WRITE_FAILED=11,
    ESDIF_READ_FAILED=12,
    ESDIF_OUT_OF_MEMORY=13,  /* Used only by sdif-mem.c */
    ESDIF_DUPLICATE_MATRIX_TYPE_IN_FRAME=14
} SDIFresult;
static const char *error_string_array[] = {
    "Everything's cool",
    "This program should display strerror(errno) instead of this string", 
    "Bad SDIF header",
    "Frame header's size is too low for time tag and stream ID",
    "fseek() failed while skipping over data",
    "Unknown matrix data type encountered in SDIF_WriteFrame().",
    (char *) NULL,   /* this will be set by SizeofSanityCheck() */
    "End of data",
    "Bad SDIF matrix header",
    "Obsolete SDIF file from an old version of SDIF",
    "Obsolete version of the standard SDIF frame and matrix types",
    "I/O error: couldn't write",
    "I/O error: couldn't read",
    "Out of memory",
    "Frame has two matrices with the same MatrixType"
};

// -- CNMAT SDIF endian --
// ---------------------------------------------------------------------------
//	CNMAT SDIF little endian machinery.
// ---------------------------------------------------------------------------

//	WORDS_BIGENDIAN is defined (or not) in config.h, determined 
//	at configure-time, changed from test of LITTLE_ENDIAN
//	which might be erroneously defined in some standard header.

//  If we didn't run configure, try to make a good guess.
#if !(HAVE_CONFIG_H) && !defined(WORDS_BIGENDIAN)
    #if (defined(__ppc__) || defined(__ppc64__))
    #define WORDS_BIGENDIAN 1
    #else
    #undef WORDS_BIGENDIAN
    #endif
#endif

#if !defined(WORDS_BIGENDIAN)
#define BUFSIZE 4096
static	char	p[BUFSIZE];
#endif


static SDIFresult SDIF_Write1(const void *block, size_t n, FILE *f) {
    return (fwrite (block,1,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
}


static SDIFresult SDIF_Write2(const void *block, size_t n, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    const char *q = (const char *)block;
    int	i, m = 2*n;

    if ((n << 1) > BUFSIZE) {
	/* Too big for buffer */
	int num = BUFSIZE >> 1;
	if (r = SDIF_Write2(block, num, f)) return r;
	return SDIF_Write2(((char *) block) + (num<<1), n-num, f);
    }

    for (i = 0; i < m; i += 2) {
	p[i] = q[i+1];
	p[i+1] = q[i];
    }

    return (fwrite(p,2,n,f)==n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;

#else
    return (fwrite (block,2,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#endif
}



static SDIFresult SDIF_Write4(const void *block, size_t n, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
   const char *q = (const char *)block;
    int i, m = 4*n;

    if ((n << 2) > BUFSIZE) 
    {
		int num = BUFSIZE >> 2;
		if (r = SDIF_Write4(block, num, f)) return r;
		return SDIF_Write4(((char *) block) + (num<<2), n-num, f);
    }

    for (i = 0; i < m; i += 4) 
    {
		p[i] = q[i+3];
		p[i+3] = q[i];
		p[i+1] = q[i+2];
		p[i+2] = q[i+1];
    }

    return (fwrite(p,4,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#else
    return (fwrite(block,4,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#endif
}



static SDIFresult SDIF_Write8(const void *block, size_t n, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
   const char *q = (const char *)block;
    int i, m = 8*n;

    if ((n << 3) > BUFSIZE) {
	int num = BUFSIZE >> 3;
	if (r = SDIF_Write8(block, num, f)) return r;
	return SDIF_Write8(((char *) block) + (num<<3), n-num, f);
    }

    for (i = 0; i < m; i += 8) {
	p[i] = q[i+7];
	p[i+7] = q[i];
	p[i+1] = q[i+6];
	p[i+6] = q[i+1];
	p[i+2] = q[i+5];
	p[i+5] = q[i+2];
	p[i+3] = q[i+4];
	p[i+4] = q[i+3];
    }

    return (fwrite(p,8,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#else
    return (fwrite(block,8,n,f) == n) ? ESDIF_SUCCESS : ESDIF_WRITE_FAILED;
#endif
}


static SDIFresult SDIF_Read1(void *block, size_t n, FILE *f) {
    return (fread (block,1,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
}


static SDIFresult SDIF_Read2(void *block, size_t n, FILE *f) {

#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    char *q = (char *)block;
    int i, m = 2*n;

    if ((n << 1) > BUFSIZE) {
	int num = BUFSIZE >> 1;
	if (r = SDIF_Read2(block, num, f)) return r;
	return SDIF_Read2(((char *) block) + (num<<1), n-num, f);
    }

    if (fread(p,2,n,f) != n) return ESDIF_READ_FAILED;

    for (i = 0; i < m; i += 2) {
	q[i] = p[i+1];
	q[i+1] = p[i];
    }

    return ESDIF_SUCCESS;
#else
    return (fread(block,2,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
#endif

}


static SDIFresult SDIF_Read4(void *block, size_t n, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    char *q = (char *)block;
    int i, m = 4*n;

    if ((n << 2) > BUFSIZE) {
	int num = BUFSIZE >> 2;
	if (r = SDIF_Read4(block, num, f)) return r;
	return SDIF_Read4(((char *) block) + (num<<2), n-num, f);
    }

    if (fread(p,4,n,f) != n) return ESDIF_READ_FAILED;

    for (i = 0; i < m; i += 4) {
	q[i] = p[i+3];
	q[i+3] = p[i];
	q[i+1] = p[i+2];
	q[i+2] = p[i+1];
    }

    return ESDIF_SUCCESS;

#else
    return (fread(block,4,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
#endif

}


static SDIFresult SDIF_Read8(void *block, size_t n, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    char *q = (char *)block;
    int i, m = 8*n;

    if ((n << 3) > BUFSIZE) {
	int num = BUFSIZE >> 3;
	if (r = SDIF_Read8(block, num, f)) return r;
	return SDIF_Read8(((char *) block) + (num<<3), n-num, f);
    }

    if (fread(p,8,n,f) != n) return ESDIF_READ_FAILED;

    for (i = 0; i < m; i += 8) {
	q[i] = p[i+7];
	q[i+7] = p[i];
	q[i+1] = p[i+6];
	q[i+6] = p[i+1];
	q[i+2] = p[i+5];
	q[i+5] = p[i+2];
	q[i+3] = p[i+4];
	q[i+4] = p[i+3];
    }

    return ESDIF_SUCCESS;

#else
    return (fread(block,8,n,f) == n) ? ESDIF_SUCCESS : ESDIF_READ_FAILED;
#endif
}

// -- CNMAT SDIF intialization --
// ---------------------------------------------------------------------------
//	CNMAT SDIF initialization.
// ---------------------------------------------------------------------------
static int SizeofSanityCheck(void) {
    int OK = 1;
    static char errorMessage[sizeof("sizeof(sdif_float64) is 999!!!")];

    if (sizeof(sdif_int16) != 2) {
    	sprintf(errorMessage, "sizeof(sdif_int16) is %d!", (int)sizeof(sdif_int16));
		OK = 0;
    }

    if (sizeof(sdif_int32) != 4) {
    	sprintf(errorMessage, "sizeof(sdif_int32) is %d!", (int)sizeof(sdif_int32));
		OK = 0;
    }

    if (sizeof(sdif_float32) != 4) {
		sprintf(errorMessage, "sizeof(sdif_float32) is %d!", (int)sizeof(sdif_float32));
		OK = 0;
    }

    if (sizeof(sdif_float64) != 8) {
		sprintf(errorMessage, "sizeof(sdif_float64) is %d!", (int)sizeof(sdif_float64));
		OK = 0;
    }

    if (!OK) {
        error_string_array[ESDIF_BAD_SIZEOF] = errorMessage;
    }
    return OK;
}



static SDIFresult SDIF_Init(void) {
	if (!SizeofSanityCheck()) {
		return ESDIF_BAD_SIZEOF;
	}
	return ESDIF_SUCCESS;
}

// -- CNMAT SDIF frame header --
// ---------------------------------------------------------------------------
//	CNMAT SDIF frame headers.
// ---------------------------------------------------------------------------
static void SDIF_Copy4Bytes(char *target, const char *string) {
    target[0] = string[0];
    target[1] = string[1];
    target[2] = string[2];
    target[3] = string[3];
}

static int SDIF_Char4Eq(const char *ths, const char *that) {
    return ths[0] == that[0] && ths[1] == that[1] &&
	ths[2] == that[2] && ths[3] == that[3];
}

static void SDIF_FillGlobalHeader(SDIF_GlobalHeader *h) {
    SDIF_Copy4Bytes(h->SDIF, "SDIF");
    h->size = 8;
    h->SDIFversion = SDIF_SPEC_VERSION;
    h->SDIFStandardTypesVersion = SDIF_LIBRARY_VERSION;
}

static SDIFresult SDIF_WriteGlobalHeader(const SDIF_GlobalHeader *h, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    if (r = SDIF_Write1(&(h->SDIF), 4, f)) return r;
    if (r = SDIF_Write4(&(h->size), 1, f)) return r;
    if (r = SDIF_Write4(&(h->SDIFversion), 1, f)) return r;
    if (r = SDIF_Write4(&(h->SDIFStandardTypesVersion), 1, f)) return r;
    return ESDIF_SUCCESS;
#else

    return (fwrite(h, sizeof(*h), 1, f) == 1) ?ESDIF_SUCCESS:ESDIF_WRITE_FAILED;

#endif
}

static SDIFresult SDIF_ReadFrameHeader(SDIF_FrameHeader *fh, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;

    if (SDIF_Read1(&(fh->frameType),4,f)) {
    	if (feof(f)) {
	    return ESDIF_END_OF_DATA;
	}
	return ESDIF_READ_FAILED;
    }
    if (r = SDIF_Read4(&(fh->size),1,f)) return r;
    if (r = SDIF_Read8(&(fh->time),1,f)) return r;
    if (r = SDIF_Read4(&(fh->streamID),1,f)) return r;
    if (r = SDIF_Read4(&(fh->matrixCount),1,f)) return r;
    return ESDIF_SUCCESS;
#else
    size_t amount_read;

    amount_read = fread(fh, sizeof(*fh), 1, f);
    if (amount_read == 1) return ESDIF_SUCCESS;
    if (amount_read == 0) {
	/* Now that fread failed, maybe we're at EOF. */
	if (feof(f)) {
	    return ESDIF_END_OF_DATA;
	}
    }
    return ESDIF_READ_FAILED;
#endif /* ! WORDS_BIGENDIAN */
}


static SDIFresult SDIF_WriteFrameHeader(const SDIF_FrameHeader *fh, FILE *f) {

#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;

    if (r = SDIF_Write1(&(fh->frameType),4,f)) return r;
    if (r = SDIF_Write4(&(fh->size),1,f)) return r;
    if (r = SDIF_Write8(&(fh->time),1,f)) return r;
    if (r = SDIF_Write4(&(fh->streamID),1,f)) return r;
    if (r = SDIF_Write4(&(fh->matrixCount),1,f)) return r;
#ifdef __WIN32__
    fflush(f);
#endif
    return ESDIF_SUCCESS;
#else

    return (fwrite(fh, sizeof(*fh), 1, f) == 1)?ESDIF_SUCCESS:ESDIF_WRITE_FAILED;

#endif
}

static SDIFresult SkipBytes(FILE *f, int bytesToSkip) {
#ifdef STREAMING
    /* Can't fseek in a stream, so waste some time needlessly copying
       some bytes in memory */
    {
#define BLOCK_SIZE 1024
		char buf[BLOCK_SIZE];
		while (bytesToSkip > BLOCK_SIZE) 
		{
		    if (fread (buf, BLOCK_SIZE, 1, f) != 1) 
		    {
				return ESDIF_READ_FAILED;
		    }
		    bytesToSkip -= BLOCK_SIZE;
		}

		if (fread (buf, bytesToSkip, 1, f) != 1) 
		{
		    return ESDIF_READ_FAILED;
		}
    }
#else
    /* More efficient implementation */
    if (fseek(f, bytesToSkip, SEEK_CUR) != 0) 
    {
		return ESDIF_SKIP_FAILED;
    }
#endif
   return ESDIF_SUCCESS;
}

static SDIFresult SDIF_SkipFrame(const SDIF_FrameHeader *head, FILE *f) {
    /* The header's size count includes the 8-byte time tag, 4-byte
       stream ID and 4-byte matrix count that we already read. */
    int bytesToSkip = head->size - 16;

    if (bytesToSkip < 0) {
	return ESDIF_BAD_FRAME_HEADER;
    }

    return SkipBytes(f, bytesToSkip);
}

// -- CNMAT SDIF matrix header --
// ---------------------------------------------------------------------------
//	CNMAT SDIF matrix headers.
// ---------------------------------------------------------------------------
static SDIFresult SDIF_ReadMatrixHeader(SDIF_MatrixHeader *m, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    if (r = SDIF_Read1(&(m->matrixType),4,f)) return r;
    if (r = SDIF_Read4(&(m->matrixDataType),1,f)) return r;
    if (r = SDIF_Read4(&(m->rowCount),1,f)) return r;
    if (r = SDIF_Read4(&(m->columnCount),1,f)) return r;
    return ESDIF_SUCCESS;
#else
    if (fread(m, sizeof(*m), 1, f) == 1) {
	return ESDIF_SUCCESS;
    } else {
	return ESDIF_READ_FAILED;
    }
#endif

}

static SDIFresult SDIF_WriteMatrixHeader(const SDIF_MatrixHeader *m, FILE *f) {
#if !defined(WORDS_BIGENDIAN)
    SDIFresult r;
    if (r = SDIF_Write1(&(m->matrixType),4,f)) return r;
    if (r = SDIF_Write4(&(m->matrixDataType),1,f)) return r;
    if (r = SDIF_Write4(&(m->rowCount),1,f)) return r;
    if (r = SDIF_Write4(&(m->columnCount),1,f)) return r;
    return ESDIF_SUCCESS;
#else
    return (fwrite(m, sizeof(*m), 1, f) == 1) ? ESDIF_SUCCESS:ESDIF_READ_FAILED;
#endif
}


static int SDIF_GetMatrixDataSize(const SDIF_MatrixHeader *m) {
    int size;
    size = SDIF_GetMatrixDataTypeSize(m->matrixDataType) *
	    m->rowCount * m->columnCount;

    if ((size % 8) != 0) {
        size += (8 - (size % 8));
    }

    return size;
}

static int SDIF_PaddingRequired(const SDIF_MatrixHeader *m) {
    int size;
    size = SDIF_GetMatrixDataTypeSize(m->matrixDataType) *
            m->rowCount * m->columnCount;

    if ((size % 8) != 0) {
	return (8 - (size % 8));
    } else {
	return 0;
    }
}

// -- CNMAT SDIF matrix data --
// ---------------------------------------------------------------------------
//	CNMAT SDIF matrix data.
// ---------------------------------------------------------------------------
static SDIFresult SDIF_SkipMatrix(const SDIF_MatrixHeader *head, FILE *f) {
    int size = SDIF_GetMatrixDataSize(head);
    
    if (size < 0) {
        return ESDIF_BAD_MATRIX_HEADER;
    }
    
    return SkipBytes(f, size);
}


static SDIFresult SDIF_WriteMatrixPadding(FILE *f, const SDIF_MatrixHeader *head) {
    int paddingBytes;
    sdif_int32 paddingBuffer[2] = {0,0};
    SDIFresult r;

    paddingBytes = SDIF_PaddingRequired(head);
    if ((r = SDIF_Write1(paddingBuffer, paddingBytes, f))) return r;

    return ESDIF_SUCCESS;
}


static SDIFresult SDIF_WriteMatrixData(FILE *f, const SDIF_MatrixHeader *head, void *data) {
    size_t datumSize = (size_t) SDIF_GetMatrixDataTypeSize(head->matrixDataType);
    size_t numItems = (size_t) (head->rowCount * head->columnCount);

#if !defined(WORDS_BIGENDIAN)
	SDIFresult r;
    switch (datumSize) {
        case 1:
            if (r = SDIF_Write1(data, numItems, f)) return r;
            break;
        case 2:
            if (r = SDIF_Write2(data, numItems, f)) return r;
            break;
        case 4:
            if (r = SDIF_Write4(data, numItems, f)) return r;
            break;
        case 8:
            if (r = SDIF_Write8(data, numItems, f)) return r;
            break;
        default:
            return ESDIF_BAD_MATRIX_DATA_TYPE;
    }
#else
    if (fwrite(data, datumSize, numItems, f) != numItems) {
        return ESDIF_READ_FAILED;
    }
#endif

    /* Handle padding */
    return SDIF_WriteMatrixPadding(f, head);
}

// -- CNMAT SDIF open and close --
// ---------------------------------------------------------------------------
//	CNMAT SDIF file open and close.
// ---------------------------------------------------------------------------

static SDIFresult SDIF_BeginWrite(FILE *output) {
    SDIF_GlobalHeader h;

    SDIF_FillGlobalHeader(&h);
    return SDIF_WriteGlobalHeader(&h, output);
}

static SDIFresult SDIF_OpenWrite(const char *filename, FILE **resultp) {
    FILE *result;
    SDIFresult r;

    if ((result = fopen(filename, "wb")) == NULL) 
    {
		return ESDIF_SEE_ERRNO;
    }
    if ((r = SDIF_BeginWrite(result))) 
    {
		fclose(result);
		return r;
    }
    *resultp = result;
    return ESDIF_SUCCESS;
}

static SDIFresult SDIF_CloseWrite(FILE *f) {
    fflush(f);
    if (fclose(f) == 0) {
	return ESDIF_SUCCESS;
    } else {
	return ESDIF_SEE_ERRNO;
    }
}

static SDIFresult SDIF_BeginRead(FILE *input) {
    SDIF_GlobalHeader sgh;
    SDIFresult r;

    /* make sure the header is OK. */
    if ((r = SDIF_Read1(sgh.SDIF, 4, input))) return r;
    if (!SDIF_Char4Eq(sgh.SDIF, "SDIF")) return ESDIF_BAD_SDIF_HEADER;
    if ((r = SDIF_Read4(&sgh.size, 1, input))) return r;
    if (sgh.size % 8 != 0) return ESDIF_BAD_SDIF_HEADER;
    if (sgh.size < 8) return ESDIF_BAD_SDIF_HEADER;
    if ((r = SDIF_Read4(&sgh.SDIFversion, 1, input))) return r;
    if ((r = SDIF_Read4(&sgh.SDIFStandardTypesVersion, 1, input))) return r;

    if (sgh.SDIFversion < 3) {
	return ESDIF_OBSOLETE_FILE_VERSION;
    }

    if (sgh.SDIFStandardTypesVersion < 1) {
	return ESDIF_OBSOLETE_TYPES_VERSION;
    }

    /* skip size-8 bytes.  (We already read the first two version numbers,
       but maybe there's more data in the header frame.) */

    if (sgh.size == 8) {
	return ESDIF_SUCCESS;
    }

    if (SkipBytes(input, sgh.size-8)) {
	return ESDIF_BAD_SDIF_HEADER;
    }

    return ESDIF_SUCCESS;
}

static SDIFresult SDIF_OpenRead(const char *filename, FILE **resultp) {
    FILE *result = NULL;
    SDIFresult r;

    if ((result = fopen(filename, "rb")) == NULL) {
        return ESDIF_SEE_ERRNO;
    }

    if ((r = SDIF_BeginRead(result))) {
        fclose(result);
        return r;
    }

    *resultp = result;
    return ESDIF_SUCCESS;
}

static SDIFresult SDIF_CloseRead(FILE *f) {
    if (fclose(f) == 0) {
	return ESDIF_SUCCESS;
    } else {
	return ESDIF_SEE_ERRNO;
    }
}

// -- construction --
						 
// ---------------------------------------------------------------------------
//	SdifFile construction helpers
// ---------------------------------------------------------------------------

// import_sdif reads SDIF data from the specified file path and
// stores data in its PartialList and MarkerContainer arguments.
static void import_sdif( const std::string &, SdifFile::partials_type &, 
						 SdifFile::markers_type & );

// export_sdif writes the data in its  PartialList and MarkerContainer 
// arguments to a specified SDIF file path. Writes bandwidth-enhanced
// Partials if enhanced is true, otherwise writes sinusoidal partials.
static void export_sdif( const std::string &, const SdifFile::partials_type &, 
						 const SdifFile::markers_type &, bool enhanced );
						 
// ---------------------------------------------------------------------------
//	SdifFile constructor from filename
// ---------------------------------------------------------------------------
//	Initialize an instance of SdifFile by importing Partial data from 
//	from the file having the specified filename or path.
//
SdifFile::SdifFile( const std::string & filename )
{
	import_sdif( filename, partials_, markers_ );
}

// ---------------------------------------------------------------------------
//	SdifFile constructor, empty
// ---------------------------------------------------------------------------
//	Initialize an empty instance of SdifFile having no Partials. 
//
SdifFile::SdifFile( void )
{
}

// -- access --
// ---------------------------------------------------------------------------
//	markers
// ---------------------------------------------------------------------------
//	Return a reference to the MarkerContainer (see Marker.h) for this SdifFile. 
SdifFile::markers_type & SdifFile::markers( void )
{
	return markers_;
}

const SdifFile::markers_type & SdifFile::markers( void ) const  
{
	return markers_;
}

// ---------------------------------------------------------------------------
//	partials
// ---------------------------------------------------------------------------
//	Return a reference (or const reference) to the bandwidth-enhanced
//	Partials represented by the envelope parameter streams in this SdifFile.
SdifFile::partials_type & SdifFile::partials( void ) 
{ 
	return partials_; 
}

const SdifFile::partials_type & SdifFile::partials( void ) const 
{ 
	return partials_; 
}

// -- mutation --
// ---------------------------------------------------------------------------
//	addPartial
// ---------------------------------------------------------------------------
//	Add a copy of the specified Partial to this SdifFile.
//
//	This member exists only for consistency with other File I/O
//	classes in Loris. The same operation can be achieved by directly
//	accessing the PartialList.
//
void SdifFile::addPartial( const Loris::Partial & p )
{
	partials_.push_back( p );
}

// ---------------------------------------------------------------------------
//	write (to path)
// ---------------------------------------------------------------------------
//	Export the envelope Partials represented by this SdifFile to
//	the file having the specified filename or path.
//
void SdifFile::write( const std::string & path )
{
	export_sdif( path, partials_, markers_, true );
}

// ---------------------------------------------------------------------------
//	write (to path)
// ---------------------------------------------------------------------------
//	Export the envelope Partials represented by this SdifFile to
//	the file having the specified filename or path in the 1TRC
//	format, resampled, and without phase or bandwidth information.
//
void SdifFile::write1TRC( const std::string & path )
{
	export_sdif( path, partials_, markers_, false );
}


// -- Loris SDIF definitions --
// ---------------------------------------------------------------------------
//	Loris SDIF types
// ---------------------------------------------------------------------------
//	Row of matrix data in SDIF RBEP, 1TRC, or RBEL format.
//
//  The RBEP matrices are for reassigned bandwidth enhanced partials (in 6 columns).
//	The 1TRC matrices are for sine-only partials (in 4 columns).
//  The first four columns of an RBEP matrix correspond to the 4 columns in 1TRC.
//	In the past, Loris exported a 7-column 1TRC; this is no longer exported, but can be imported.
//
//	The RBEL format always has two columns, index and partial label.
//	The RBEL matrix is optional; it has partial label information (in 2 columns).
int lorisRowMaxElements = 7;
int lorisRowEnhancedElements = 6;
int lorisRowSineOnlyElements = 4;

typedef struct {
    sdif_float64 index, freqOrLabel, amp, phase, noise, timeOffset, resampledFlag;
} RowOfLorisData64;

typedef struct {
    sdif_float32 index, freqOrLabel, amp, phase, noise, timeOffset, resampledFlag;
} RowOfLorisData32;


//  SDIF signatures used by Loris.
typedef char sdif_signature[4];
static sdif_signature lorisEnhancedSignature = { 'R','B','E','P' };
static sdif_signature lorisLabelsSignature = { 'R','B','E','L' };
static sdif_signature lorisSineOnlySignature = { '1','T','R','C' };
static sdif_signature lorisMarkersSignature = { 'R','B','E','M' };


//	Exception class for handling errors in SDIF library:
class SdifLibraryError : public FileIOException
{
public:
	SdifLibraryError( const std::string & str, const std::string & where = "" ) : 
		FileIOException( std::string("SDIF library error -- ").append( str ), where ) {}
};	//	end of class SdifLibraryError

//	macro to check for SDIF library errors and throw exceptions when
//	they occur, which we really ought to do after every SDIF library
//	call:
#define ThrowIfSdifError( errNum, report )										\
	if (errNum)																	\
	{																			\
		const char* errPtr = error_string_array[errNum];								\
		if (errPtr)																\
		{																		\
	        debugger << "SDIF error " << errPtr << endl;						\
			std::string s(report);												\
			s.append(", SDIF error message: ");									\
			s.append(errPtr);													\
			Throw( SdifLibraryError, s );										\
		}																		\
	}	

// -- SDIF reading helpers --
// ---------------------------------------------------------------------------
//	processRow64
// ---------------------------------------------------------------------------
//	Add to existing Loris partials, or create new Loris partials for this data.
//
static void
processRow64( const sdif_signature msig, const RowOfLorisData64 & rowData, const double frameTime, 
				  std::vector< Partial > & partialsVector )
{	

//
// Skip this if the data point is not from the original data (7-column 1TRC format).
//
	if (rowData.resampledFlag)
		return;
	
//
// Make sure we have enough partials for this partial's index.
//
	if (partialsVector.size() <= rowData.index)
	{
		partialsVector.resize( long(rowData.index) + 500 );
	}

//
// Create a new breakpoint and insert it.
//	
	if (SDIF_Char4Eq(msig, lorisEnhancedSignature) || SDIF_Char4Eq(msig, lorisSineOnlySignature)) 
	{
		Breakpoint newbp( rowData.freqOrLabel, rowData.amp, rowData.noise, rowData.phase );
		partialsVector[long(rowData.index)].insert( frameTime + rowData.timeOffset, newbp );
	}
//
// Set partial label.
//
	else if (SDIF_Char4Eq(msig, lorisLabelsSignature)) 
	{
		partialsVector[long(rowData.index)].setLabel( (int) rowData.freqOrLabel );
	}
		
}

// ---------------------------------------------------------------------------
//	processRow32
// ---------------------------------------------------------------------------
//	Add to existing Loris partials, or create new Loris partials for this data.
//  This is for reading 32-bit float files.
//
static void
processRow32( const sdif_signature msig, const RowOfLorisData32 & rowData, const double frameTime, 
				  std::vector< Partial > & partialsVector )
{	

//
// Skip this if the data point is not from the original data (7-column 1TRC format).
//
	if (rowData.resampledFlag)
		return;
	
//
// Make sure we have enough partials for this partial's index.
//
	if (partialsVector.size() <= rowData.index)
	{
		partialsVector.resize( long(rowData.index) + 500 );
	}

//
// Create a new breakpoint and insert it.
//	
	if (SDIF_Char4Eq(msig, lorisEnhancedSignature) || SDIF_Char4Eq(msig, lorisSineOnlySignature)) 
	{
		Breakpoint newbp( rowData.freqOrLabel, rowData.amp, rowData.noise, rowData.phase );
		partialsVector[long(rowData.index)].insert( frameTime + rowData.timeOffset, newbp );
	}
//
// Set partial label.
//
	else if (SDIF_Char4Eq(msig, lorisLabelsSignature)) 
	{
		partialsVector[long(rowData.index)].setLabel( (int) rowData.freqOrLabel );
	}
		
}

// ---------------------------------------------------------------------------
//	readMarkers
// ---------------------------------------------------------------------------
//
static void
readMarkers( FILE * file, SDIF_FrameHeader fh, SdifFile::markers_type & markersVector )
{
//
// Read Loris markers from SDIF file in a RBEM frame.
// This precedes the envelope data in the file.
// Let exceptions propagate.
//
	SDIFresult ret;
	int cols = 1;
//
// The frame must contain exactly two matrices.
//
	if (fh.matrixCount != 2)
	{
		Throw( FileIOException, "Markers frame has bad format." );
	}

//							
// Read the numeric (marker times) matrix.
//
	{
		SDIF_MatrixHeader mh;
	    ret = SDIF_ReadMatrixHeader(&mh,file);
		ThrowIfSdifError( ret, "Error reading SDIF file" );
		
		// Error if matrix has unexpected data type.
		if ((mh.matrixDataType != SDIF_FLOAT32 && mh.matrixDataType != SDIF_FLOAT64) || mh.columnCount != cols) 
		{
			Throw( FileIOException, "Markers frame has bad format." );
		}
		
		// Read each row of matrix data.
		for (int row = 0; row < mh.rowCount; row++)
		{
			if (mh.matrixDataType == SDIF_FLOAT64)
			{
				sdif_float64 markerTime64;
				SDIF_Read8(&markerTime64,1,file);
				markersVector.push_back(Marker(markerTime64, ""));
			}
			else
			{
				sdif_float32 markerTime32;
				SDIF_Read4(&markerTime32,1,file);
				markersVector.push_back(Marker(markerTime32, ""));
			}
		}
		
		// Skip over padding, if any.
		if ((mh.matrixDataType == SDIF_FLOAT32) && ((mh.rowCount * mh.columnCount) & 0x1)) 
		{
		    sdif_float32 pad;
		    SDIF_Read4(&pad,1,file);
		}
	}
	
//							
// Read the string (marker names) matrix.
//
	{
		SDIF_MatrixHeader mh;
	    ret = SDIF_ReadMatrixHeader(&mh,file);
		ThrowIfSdifError( ret, "Error reading SDIF file" );
		
		// Error if matrix has unexpected data type.
		if (mh.matrixDataType != SDIF_UTF8 || mh.columnCount != cols) 
		{
			Throw( FileIOException, "Markers frame has bad format." );
		}
		
		// Read strings.
		std::string markerName;
		int markerNumber = 0;
		for (int row = 0; row < mh.rowCount; row++)
		{
			char ch;
			SDIF_Read1(&ch,1,file);
			
			// If we have reached the end of a name, assign it to a marker.
			if (ch == '\0')
			{
				// Save the name of the marker.
				markersVector[markerNumber].setName(markerName);
				
				// Prepare to get name of next marker.
				markerNumber++;
				if (markerNumber > markersVector.size()) 
				{
					Throw( FileIOException, "Markers frame has bad format." );
				}
				markerName.erase();
			}
			else
			{
				markerName += ch;
			}
		}
			
		// There should be one marker name for each marker time.
		if (markerNumber != markersVector.size()) 
		{
			Throw( FileIOException, "Markers frame has bad format." );
		}
		
		// Skip padding.
		ret = SkipBytes(file, SDIF_PaddingRequired(&mh));
	}
}

// ---------------------------------------------------------------------------
//	readLorisMatrices
// ---------------------------------------------------------------------------
// Let exceptions propagate.
//
static void
readLorisMatrices( FILE *file, std::vector< Partial > & partialsVector, SdifFile::markers_type & markersVector )
{
	SDIFresult ret;

//
// Read all frames matching the file selection.
//
	SDIF_FrameHeader fh;
	while (!(ret = SDIF_ReadFrameHeader(&fh, file)))
	{	

		// Check for Loris Markers frame.
		if (SDIF_Char4Eq(fh.frameType, lorisMarkersSignature))
		{
			readMarkers( file, fh, markersVector );
			continue;		
		}
			
		// Skip frames until we find one we are interested in.
		if (!SDIF_Char4Eq(fh.frameType, lorisEnhancedSignature) 
					&& !SDIF_Char4Eq(fh.frameType, lorisSineOnlySignature) 
					&& !SDIF_Char4Eq(fh.frameType, lorisLabelsSignature))
		{
			ret = SDIF_SkipFrame(&fh, file);	
			ThrowIfSdifError( ret, "Error reading SDIF file" );
			continue;		
		}
		

		// Read all matrices in this frame.
		for (int m = 0; m < fh.matrixCount; m++)
		{
			SDIF_MatrixHeader mh;
		    ret = SDIF_ReadMatrixHeader(&mh,file);
			ThrowIfSdifError( ret, "Error reading SDIF file" );
			
			// Skip matrix if it has unexpected data type.
			if ((mh.matrixDataType != SDIF_FLOAT32 && mh.matrixDataType != SDIF_FLOAT64) 
							|| mh.columnCount > lorisRowMaxElements) 
			{
				ret = SDIF_SkipMatrix(&mh, file);	
				ThrowIfSdifError( ret, "Error reading SDIF file" );
				continue;		
			}
			
			// Read each row of matrix data.
			for (int row = 0; row < mh.rowCount; row++)
			{
			
				if (mh.matrixDataType == SDIF_FLOAT64)
				{
					// Fill a rowData structure with one row from the matrix.
					RowOfLorisData64 rowData64 = { 0.0 };
					sdif_float64 *rowDataPtr = &rowData64.index;
					for (int col = 1; col <= mh.columnCount; col++)
					{
						SDIF_Read8(rowDataPtr++,1,file);
					}
					
					// Add rowData as a new breakpoint in a partial, or,
					// if its a RBEL matrix, read label mapping.
					processRow64(mh.matrixType, rowData64, fh.time, partialsVector);
				}
				else
				{
					// Fill a rowData structure with one row from the matrix.
					RowOfLorisData32 rowData32 = { 0.0 };
					sdif_float32 *rowDataPtr = &rowData32.index;
					for (int col = 1; col <= mh.columnCount; col++)
					{
						SDIF_Read4(rowDataPtr++,1,file);
					}
					
					// Add rowData as a new breakpoint in a partial, or,
					// if its a RBEL matrix, read label mapping.
					processRow32(mh.matrixType, rowData32, fh.time, partialsVector);
				}
			}
			
			// Skip over padding, if any.
			if ((mh.matrixDataType == SDIF_FLOAT32) && ((mh.rowCount * mh.columnCount) & 0x1)) 
			{
			    sdif_float32 pad;
			    SDIF_Read4(&pad,1,file);
			}
		}
	} 
	
	// At this point, ret should be ESDIF_END_OF_DATA.
	if (ret != ESDIF_END_OF_DATA)
		ThrowIfSdifError( ret, "Error reading SDIF file" );
}

// ---------------------------------------------------------------------------
//	read
// ---------------------------------------------------------------------------
// Let exceptions propagate.
//
static void import_sdif( const std::string &infilename, 
						 SdifFile::partials_type & partials, 
						 SdifFile::markers_type & markers)
{

//
// Initialize CNMSAT SDIF routines.
//
	SDIFresult ret = SDIF_Init();
	if (ret)
	{
		Throw( FileIOException, "Could not initialize SDIF routines." );
	}

//
// Open SDIF file for reading.
// Note: Currently we do not specify any selection criterion in this call.
//
	FILE *file;
	ret = SDIF_OpenRead(infilename.c_str(), &file);
	if (ret)
	{
		Throw( FileIOException, "Could not open SDIF file for reading." );
	}

//
// Read SDIF data.
//	
	try 
	{
	
		// Build up partialsVector.
		std::vector< Partial > partialsVector;
		SdifFile::markers_type markersVector;
		readLorisMatrices( file, partialsVector, markersVector );
		
		// Copy partialsVector to partials list.
		for (int i = 0; i < partialsVector.size(); ++i)
		{
			if (partialsVector[i].numBreakpoints() > 0)
			{
				partials.push_back( partialsVector[i] );
			}
		}
		
		// Copy markersVector to markers list.
		for (int i = 0; i < markersVector.size(); ++i)
		{
			markers.push_back( markersVector[i] );
		}
	}
	catch ( Exception & ex ) 
	{
		partials.clear();
		markers.clear();
		ex.append(" Failed to read SDIF file.");
		SDIF_CloseRead(file);
		throw;
	}

//
// Close SDIF input file.
//
	SDIF_CloseRead(file);
	
//
// Complain if no Partials were imported:
//
	if ( partials.size() == 0 )
	{
		notifier << "No Partials were imported from " << infilename 
				 << ", no (non-empty) SDIF frames found." << endl;
	}
	
}

// -- SDIF writing helpers --
// ---------------------------------------------------------------------------
//	makeSortedBreakpointTimes
// ---------------------------------------------------------------------------
//	Collect the times of all breakpoints in the analysis, and sort by time.
//  Sorted breakpoints are used in finding frame start times in SDIF writing.
//
struct BreakpointTime
{
	long index;			// index identifying which partial has the breakpoint
	double time;        // time of the breakpoint
};

struct earlier_time
{
	bool operator()( const BreakpointTime & lhs, const BreakpointTime & rhs ) const
		{ return lhs.time < rhs.time; }
};

static void
makeSortedBreakpointTimes( const ConstPartialPtrs & partialsVector, 
						   std::list< BreakpointTime > & allBreakpoints ) 
{

// Make list of all breakpoint times from all partials.
	for (int i = 0; i < partialsVector.size(); i++) 
	{
		for ( Partial::const_iterator it = partialsVector[i]->begin(); 
			  it != partialsVector[i]->end();
			  ++it ) 
		{
			BreakpointTime bpt;
			bpt.index = i;
			bpt.time = it.time();
			allBreakpoints.push_back( bpt );
		}
	}

// Sort list of all breakpoint times.
	allBreakpoints.sort( earlier_time() );
}

// ---------------------------------------------------------------------------
//	getNextFrameTime
// ---------------------------------------------------------------------------
//	Get time of next frame.
//  This helps make SDIF files with exact timing (7-column 1TRC format).
//  This uses the previously sorted allBreakpoints list.
//
//	All Breakpoints should be const, but for some reason, gcc (on SGI at 
//	least) makes trouble converting and comparing iterators and const_iterators.
//
static double getNextFrameTime( const double frameTime,
								std::list< BreakpointTime > & allBreakpoints,
								std::list< BreakpointTime >::iterator & bpTimeIter)
{
//
// Build up vector of partials that have a breakpoint in this frame, update the vector
// as we increase the frame duration.  Return when a partial gets a second breakpoint.
//
// This vector is only used locally. We search this vector of indices to determine
// whether or not a Partial has already contributed a Breakpoint to the current 
// frame.
//
	double nextFrameTime = frameTime;
	std::vector< long > partialsWithBreakpointsInFrame;
	
	// const std::list< BreakpointTime >::iterator & first = bpTimeIter;
	
	//	invariant:
	//	Breakpoints in allBreakpoints before the position
	//	of bpTimeIter have be added to a SDIF frame, either
	//	the current one or an earlier one. If it is not
	//	equal to bpTimeIter, then all Breakpoints between
	// 	those two positions have the same time.
	std::list< BreakpointTime >::iterator it = bpTimeIter;
	while ( it != allBreakpoints.end() && 
			( std::find( partialsWithBreakpointsInFrame.begin(),
		                 partialsWithBreakpointsInFrame.end(),
		                 it->index ) == 
              partialsWithBreakpointsInFrame.end() ) )
	{		
		// Add breakpoint to list of potential breakpoints for frame, 
		// then iterate to soonest breakpoint on any partial.  The final decision
		// to add this breakpoint to the frame is made below, if bpTimeIter is 
		// updated.
		partialsWithBreakpointsInFrame.push_back( it->index );
		
		
		//  If the new breakpoint is at a new time, it could potentially be the
		//	first breakpoint in the next frame. If there are several breakpoints at
		//	the exact same time (could happen if these envelopes came from a spc
		//	file or from resampled envelopes), always start the frame at the first
		//  of these.  Set bpTimeIter if this is a good start of a new frame.
		//
		//	Don't want to increment bpTimeIter until we are certain that all 
		//	coincident Breakpoints can be added to the current frame (that is,
		//	that none of them are from Partials that already have a Breakpoint
		//	in this frame).
        //
        //  epsilon controls how close together in time two breakpoints can be.        
        //  Keep this large enough that double-precision floating point math
        //  can find a time between two breakpoints close in time. One nanosecond
        //  ought to be plenty close.
		++it;
        const double epsilon = 1e-9;
		if ( ( it == allBreakpoints.end() ) || ( (it->time - bpTimeIter->time) > epsilon ) )
		{
			bpTimeIter = it;
		}
	}

	if ( bpTimeIter == allBreakpoints.end() )
	{
		//	We are at the end of the sound; no "next frame" there,
		//	set the next frame time to something later than the last
		//	Breakpoint and the current frame time (the current frame
		//	might be empty, so have to check both).
		nextFrameTime = std::max( (double)allBreakpoints.back().time, frameTime ) + 1;
	}
	else
	{
		Assert( bpTimeIter != allBreakpoints.begin() );
		
		//	Compute the next frame time:
		//	If possible, round it to the nearest millisecond before
		//	the first Breakpoint in the next frame, otherwise just
		//	pick a time between the last Breakpoint in the current
		//	frame and the first Breakpoint in the next.
		std::list< BreakpointTime >::iterator prev = bpTimeIter;
		--prev;

		//	prev and bpTimeIter cannot have the same time, because
		//	if there are several Breakpoints at the same time, bpTimeIter
		//	will be the first of them in the list:
		Assert( bpTimeIter->time > prev->time );
		
		//	This seems to be sensitive to floating point error,
		//	probably because times are stored in 32 bit floats.
		//	We need the error to round toward the later time.
        //
        //  Note: times are no longer stored in 32 bit floats, 
        //  why is this still so flakey?
		nextFrameTime = bpTimeIter->time - ( 0.5 * ( bpTimeIter->time - prev->time ) );
        /*
        notifier << "next frame time " << nextFrameTime << endl;
        notifier << " bp time " << bpTimeIter->time << endl;
        notifier << " prev time " << prev->time << endl;
        notifier << " diff = " << bpTimeIter->time - prev->time << endl;
        */
		Assert( bpTimeIter->time >= nextFrameTime );
		Assert( nextFrameTime > prev->time );
		
		//	Try to make frame times whole milliseconds.
		//	MUST use 32-bit floats for time, or else floating
		//	point rounding errors cause us to drop breakpoints!
		double nextFramePrevRnd = 0.001 * std::floor( 1000. * nextFrameTime );
		if ( ( nextFramePrevRnd < nextFrameTime ) && ( nextFramePrevRnd > prev->time ) )
		{
			nextFrameTime = nextFramePrevRnd;
		}
		else
		{
			//	Try tenth-milliseconds, otherwise give up.
			nextFramePrevRnd = 0.0001 * std::floor( 10000. * nextFrameTime );
			if ( ( nextFramePrevRnd < nextFrameTime ) && ( nextFramePrevRnd > prev->time ) )
			{
				nextFrameTime = nextFramePrevRnd;
			}
		}			
	}
    
    // notifier << "   returning next frame time " << nextFrameTime << endl;

#if Debug_Loris		
	if ( ! ( nextFrameTime > frameTime ) )
	{
		if ( bpTimeIter != allBreakpoints.end() )
		{
			std::cout << bpTimeIter->time << std::endl;
		}
		else
		{
			std::cout << "end" << std::endl;
		}
		// std::cout << first->time << ", index " << first->index << std::endl;
		std::cout << nextFrameTime << std::endl;
		std::cout << frameTime << std::endl;
		std::cout << partialsWithBreakpointsInFrame.size() << std::endl;
	}	
	Assert( nextFrameTime > frameTime );
#endif
	return nextFrameTime;
}		


// ---------------------------------------------------------------------------
//	indexPartials
// ---------------------------------------------------------------------------
//	Make a vector of partial pointers. 
//  The vector index will be the sdif 1TRC index for the partial. 
//
static void
indexPartials( const PartialList & partials, ConstPartialPtrs & partialsVector )
{
	for ( PartialList::const_iterator it = partials.begin(); it != partials.end(); ++it )
	{
		if ( it->size() != 0 )
		{
			partialsVector.push_back( (Partial *)&(*it) );	//@@@ Kluge Here  (Partial *)
	//		partialsVector.push_back( &(*it) );	
		}
	}
}


// ---------------------------------------------------------------------------
//	collectActiveIndices
// ---------------------------------------------------------------------------
//	Collect all partials active in a particular frame. 
//
//  Return true if frameTime is beyond end of all the partials.
//	Don't need to return this, can just check frame time against
//	the time of the last BreakpointTime in the allBreakpoints vector.
//
static void
collectActiveIndices( const ConstPartialPtrs & partialsVector, 
                      const bool enhanced,
                      const double frameTime, 
                      const double nextFrameTime,
                      std::vector< int > & activeIndices )
{
#if 1 //Debug_Loris		
	if ( ! ( nextFrameTime > frameTime ) ) 
	{
		std::cout << nextFrameTime << " <= " << frameTime << std:: endl;
		//std::cout << "amp 128 : " << partialsVector[128]->amplitudeAt( frameTime ) << std::endl;
		
	} 
#endif
	Assert( nextFrameTime > frameTime );
		
	for ( int i = 0; i < partialsVector.size(); i++ ) 
	{
		Assert( partialsVector[ i ] != 0 );
		
		const Partial & mightBeActive = *( partialsVector[ i ] );
		
		// Is there a breakpoint within the frame?
		// Skip the partial if there is no breakpoint and either:
		//		(1) we are writing enhanced format, 
		// 	 or (2) the partial has zero amplitude.
		//
		//	Include this Partial if:
		//	(1) it has a Breakpoint in the frame, or 
		//	(2A) we are not writing enhanced data, and
		//	(2B) the Partial has non-zero amplitude at the time of
		//		 this frame.
		//
		Partial::const_iterator it = mightBeActive.findAfter( frameTime );
		if ( it != mightBeActive.end() )
		{
#if Debug_Loris
			//	DEBUGGING
			//	if this one is in this frame, then 
			//	the one before it had better be in the previous frame!
			if ( ( it != mightBeActive.begin() ) && ( it.time() < nextFrameTime ) )
			{
				Partial::const_iterator prev = it;
				--prev;
				Assert( prev.time() < frameTime );
			}
			Assert( it != mightBeActive.end() );
#endif			
			
			//	mightBeActive is active in this frame if the Breakpoint
			//	at it is earlier than the next frame time.
			//
			//	1TRC (non-enhanced) contains data for every non-silent
			//	active Partial at the time of the frame.
			if ( ( it.time() < nextFrameTime ) ||
				 ( !enhanced && mightBeActive.amplitudeAt( frameTime ) != 0.0 ) ) 
			{
				activeIndices.push_back( i );	
			}			
		}

	}
}

// ---------------------------------------------------------------------------
//	writeEnvelopeLabels
// ---------------------------------------------------------------------------
//
static void
writeEnvelopeLabels( FILE * out, const ConstPartialPtrs & partialsVector )
{
//
// Write Loris labels to SDIF file in a RBEL matrix.
// This precedes the 1TRC data in the file.
// Let exceptions propagate.
//

	int streamID = 2; 				// stream id different from envelope's stream id
	double frameTime = 0.0;

//
// Allocate RBEL matrix data.
//
	int cols = 2;
	sdif_float64 *data = new sdif_float64[ partialsVector.size() * cols ];

//
// For each partial index, specify the partial label.
//
	sdif_float64 *dp = data;
	int anyLabel = false;
	for (int i = 0; i < partialsVector.size(); i++) 
	{
		int labl = partialsVector[i]->label();
		anyLabel |= (labl != 0);
		*dp++ = i;					// column 1: index
		*dp++ = labl;				// column 2: label
	}	

//							
// Write out matrix data, if there were any labels.
//
	if (anyLabel)
	{
		// Write the frame header.
		SDIF_FrameHeader fh;
		SDIF_Copy4Bytes(fh.frameType, lorisLabelsSignature);
		fh.size = 
				// size of remaining frame header
				  sizeof(sdif_float64) + 2 * sizeof(sdif_int32) 		
				// size of matrix header
				+ sizeof(SDIF_MatrixHeader) 							
				// size of matrix data plus any padding
				+ 8 * ((partialsVector.size() * cols * sizeof(sdif_float64) + 7) / 8);	
		fh.time = frameTime;
		fh.streamID = streamID;
		fh.matrixCount = 1;
		SDIFresult ret = SDIF_WriteFrameHeader(&fh, out);
		
		// Write the matrix header.
		SDIF_MatrixHeader mh;
		SDIF_Copy4Bytes(mh.matrixType, lorisLabelsSignature);
		mh.matrixDataType = SDIF_FLOAT64;
		mh.rowCount = partialsVector.size();
		mh.columnCount = cols;
		ret = SDIF_WriteMatrixHeader(&mh, out);
		
		// Write the matrix data, and any necessary padding.
		ret = SDIF_WriteMatrixData(out, &mh, data);
	}

//	
// Free RBEL matrix space.
//
	delete [] data;
}

// ---------------------------------------------------------------------------
//	writeMarkers
// ---------------------------------------------------------------------------
//
static void
writeMarkers( FILE * out, const SdifFile::markers_type &markers )
{
//
// Write Loris markers to SDIF file in a RBEM frame.
// This precedes the envelope data in the file.
// Let exceptions propagate.
//

//
// Exit if there are no markers.
//
	if ( markers.empty() )
	{
		return;
	}

	int streamID = 2; 				// stream id different from envelope's stream id
	double frameTime = 0.0;

//
// We will need two matrices: one numeric (marker times) matrix data and character (marker names) matrix.
//
	std::vector<sdif_float64> markerTimes;
	std::string markerNames;

//
// Get matrix data from each marker.
//
	for (int marker = 0; marker < markers.size(); marker++)
	{
		markerTimes.push_back( markers[marker].time() );
		markerNames += markers[marker].name() + '\0';
	}

//							
// Write out frame with two marker matrices.
//
    // Write the frame header.
    int cols = 1;
    {
            SDIF_FrameHeader fh;
            SDIF_Copy4Bytes( fh.frameType, lorisMarkersSignature );
            fh.size =
                            // size of remaining frame header
                              sizeof( sdif_float64 ) + 2 * sizeof( sdif_int32 )
                            // size of matrix headers
                            + 2 * sizeof( SDIF_MatrixHeader )
                            // size of numeric (time) matrix data
                            + markerTimes.size() * sizeof( sdif_float64 )
                            // size of marker names data plus padding
                            + 8 * ( ( markerNames.size() + 7 ) / 8 );
            fh.time = frameTime;
            fh.streamID = streamID;
            fh.matrixCount = 2;
            // SDIFresult ret = return value never checked!
				SDIF_WriteFrameHeader(&fh, out);
    }
	
	// Write the numeric (marker times) matrix.
	{
		// Write the numeric (time) matrix header.
		SDIF_MatrixHeader mh;
		SDIF_Copy4Bytes( mh.matrixType, lorisMarkersSignature );
		mh.matrixDataType = SDIF_FLOAT64;
		mh.rowCount = markerTimes.size();
		mh.columnCount = cols;
		SDIFresult ret = SDIF_WriteMatrixHeader( &mh, out );
		
		// Write the numeric (time) matrix data, and any necessary padding.
		ret = SDIF_WriteMatrixData( out, &mh, &markerTimes[0] );
	}
	
	// Write the string (marker names) matrix.
	{
		// Write the string (names) matrix header.
		SDIF_MatrixHeader mh;
		SDIF_Copy4Bytes( mh.matrixType, lorisMarkersSignature );
		mh.matrixDataType = SDIF_UTF8;
		mh.rowCount = markerNames.size();
		mh.columnCount = cols;
		SDIFresult ret = SDIF_WriteMatrixHeader( &mh, out );
		
		// Write the string (names) matrix data, and any necessary padding.
		ret = SDIF_WriteMatrixData( out, &mh, &markerNames[0] );
	}
}


// ---------------------------------------------------------------------------
//	assembleMatrixData
// ---------------------------------------------------------------------------
//	The activeIndices vector contains indices for partials that have data at this time.
//	Assemble SDIF matrix data for these partials.
//
static void
assembleMatrixData( sdif_float64 *data, const bool enhanced,
					const ConstPartialPtrs & partialsVector, 
					const std::vector< int > & activeIndices, 
					const double frameTime )
{	
	// The array matrix data is row-major order at "data".
	sdif_float64 *rowDataPtr = data;
	
	for ( int i = 0; i < activeIndices.size(); i++ ) 
	{
		
		int index = activeIndices[ i ];
		const Partial * par = partialsVector[ index ];
		
		// For enhanced format we use exact timing; the activeIndices only includes
		// partials that have breakpoints in this frame.
		// For sine-only format we resample at frame times, for enhanced, use
		// the Breakpoints themselves.
		Assert( par->endTime() >= frameTime );
		double tim = frameTime;		
		Breakpoint params;
		if ( enhanced )
		{
		    Partial::const_iterator pos = par->findAfter( frameTime );
		    tim = pos.time();
		    params = pos.breakpoint();
		}
		else
		{
		    params = par->parametersAt( frameTime );
		}
				
		// Must have phase between 0 and 2*Pi.
		double phas = params.phase(); 
		if (phas < 0)
		{
			phas += 2. * Pi; 
		}
		
		// Fill in values for this row of matrix data.
		*rowDataPtr++ = index;							// first row of matrix   (standard)
		*rowDataPtr++ = params.frequency(); 		    // second row of matrix  (standard)
		*rowDataPtr++ = params.amplitude();		        // third row of matrix   (standard)
		*rowDataPtr++ = phas;							// fourth row of matrix  (standard)
		if (enhanced)
		{
			*rowDataPtr++ = params.bandwidth();	        // fifth row of matrix   (loris)
			*rowDataPtr++ = tim - frameTime;			// sixth row of matrix   (loris)
		}
	}
}


// ---------------------------------------------------------------------------
//	writeEnvelopeData
// ---------------------------------------------------------------------------
//
static void
writeEnvelopeData( FILE * out,
				   const ConstPartialPtrs & partialsVector,
				   const bool enhanced )
{
//
// Export SDIF file from Loris data.
// Let exceptions propagate.
//

	int streamID = 1; 						// one stream id for all SDIF frames

//
// Make a sorted list of all breakpoints in all partials, and initialize the list iterater.
// This stuff does nothing if we are writing 5-column 1TRC format.
//
	std::list< BreakpointTime > allBreakpoints;
	makeSortedBreakpointTimes( partialsVector, allBreakpoints );
	std::list< BreakpointTime >::iterator bpTimeIter = allBreakpoints.begin();
	
#if Debug_Loris	
	const std::list< BreakpointTime >::size_type DEBUG_allBreakpointsSize = allBreakpoints.size();
	std::list< BreakpointTime >::size_type DEBUG_cumNumTracks = 0;
#endif

//
// Output Loris envelope data in SDIF frame format.
// First frame starts at millisecond of first breakpoint.
//
	double nextFrameTime = allBreakpoints.front().time;
	if ( 1000. * nextFrameTime - int( 1000. * nextFrameTime ) != 0. )
	{
		// HEY! Looks like this could give negative frame times, 
		// is that allowed?
		nextFrameTime = std::floor( 1000. * nextFrameTime - .001 ) / 1000.0;
	}
	
	do 
	{

//
// Go to next frame.
//
		double frameTime = nextFrameTime;
		nextFrameTime = getNextFrameTime( frameTime, allBreakpoints, bpTimeIter );

//
// Make a vector of partial indices that includes all partials active at this time.
//
		std::vector< int > activeIndices;
		collectActiveIndices( partialsVector, enhanced, frameTime, 
							  nextFrameTime, activeIndices );

//
// Write frame header, matrix header, and matrix data.
// We always have one matrix per frame.
// The matrix size depends on the number of partials active at this time.
//
		int numTracks = activeIndices.size();
		
#if Debug_Loris	
		DEBUG_cumNumTracks += numTracks;
#endif
		
		if ( numTracks > 0 ) 	//	could activeIndices ever be empty?
		{
		
			// Allocate matrix data.
			int cols = ( enhanced ? lorisRowEnhancedElements : lorisRowSineOnlyElements );

			//	I think this will go a lot faster if we aren't doing so much
			//	dynamic memory allocation for each frame. if I use a vector, 
			//	then we only need to allocate more memory when a frame has more
			//	columns than any previous frame. Construct the vector once,
			//	resize it for each frame, and clear it when done (doesn't 
			//	deallocate memory).
			// sdif_float64 *data = new sdif_float64[numTracks * cols];
			static std::vector< sdif_float64 > dataVector;
			dataVector.resize( numTracks * cols );

			// Fill in matrix data.
			sdif_float64 *data = &dataVector[ 0 ];
			assembleMatrixData( data, enhanced, partialsVector, activeIndices, frameTime );
					
			// Write the frame header.
			SDIF_FrameHeader fh;
			SDIF_Copy4Bytes( fh.frameType, enhanced ? lorisEnhancedSignature : lorisSineOnlySignature );
			fh.size = 		
					// size of remaining frame header
					  sizeof(sdif_float64) + 2 * sizeof(sdif_int32) 
					// size of matrix header
					+ sizeof(SDIF_MatrixHeader) 							
					// size of matrix data plus any padding
					+ 8*((numTracks * cols * sizeof(sdif_int32) + 7)/8);	
			fh.streamID = streamID;
			fh.time = frameTime;
			fh.matrixCount = 1;
			SDIFresult ret = SDIF_WriteFrameHeader(&fh, out);
			
			// Write the matrix header.
			SDIF_MatrixHeader mh;
			SDIF_Copy4Bytes( mh.matrixType, enhanced ? lorisEnhancedSignature : lorisSineOnlySignature );
			mh.matrixDataType = SDIF_FLOAT64;
			mh.rowCount = numTracks;
			mh.columnCount = cols;
			ret = SDIF_WriteMatrixHeader( &mh, out );
			
			// Write the matrix data, and any necessary padding.
			ret = SDIF_WriteMatrixData( out, &mh, data );
			
			// Free matrix space.
			// delete [] data;
			//	Instead of deallocating, just clear the static 
			//	vector, setting its logical size to zero.
			dataVector.clear();
		}
	}
	while ( nextFrameTime < allBreakpoints.back().time );
	
#if Debug_Loris	
	std::cout << "SDIF export found " << DEBUG_allBreakpointsSize << " Breakpoints" << std::endl;
	std::cout << "and exported " << DEBUG_cumNumTracks << std::endl;
#endif
}

// ---------------------------------------------------------------------------
//	Export
// ---------------------------------------------------------------------------
// Export SDIF file.
//
static void export_sdif( const std::string & filename, 
						 const SdifFile::partials_type & partials, 
						 const SdifFile::markers_type &markers, const bool enhanced )
{
//
// Initialize CNMAT SDIF routines.
//
	SDIFresult ret = SDIF_Init();
	if (ret)
	{
		Throw( FileIOException, "Could not initialize SDIF routines." );
	}
//
// Open SDIF file for writing.
//
	FILE *out;
	ret = SDIF_OpenWrite(filename.c_str(), &out);
	if (ret)
	{
		Throw( FileIOException, "Could not open SDIF file for writing: " + filename );
	}
	
	// We are no longer defining frame types.

	//		// Define RBEP matrix and frame type for enhanced partials.
	//		if (enhanced)
	//		{
	//			SdifMatrixTypeT *parsMatrixType = SdifCreateMatrixType(lorisEnhancedSignature,NULL);
	//			SdifMatrixTypeInsertTailColumnDef(parsMatrixType,"Index");  
	//			SdifMatrixTypeInsertTailColumnDef(parsMatrixType,"Frequency");  
	//			SdifMatrixTypeInsertTailColumnDef(parsMatrixType,"Amplitude");  
	//			SdifMatrixTypeInsertTailColumnDef(parsMatrixType,"Phase");  
	//			SdifMatrixTypeInsertTailColumnDef(parsMatrixType,"Noise");  
	//			SdifMatrixTypeInsertTailColumnDef(parsMatrixType,"TimeOffset");  
	//			SdifPutMatrixType(out->MatrixTypesTable, parsMatrixType);//
	//
	//			SdifFrameTypeT *parsFrameType = SdifCreateFrameType(lorisEnhancedSignature,NULL);
	//			SdifFrameTypePutComponent(parsFrameType, lorisEnhancedSignature, "RABWE_Partials");
	//			SdifPutFrameType(out->FrameTypesTable, parsFrameType);
	//		}
	//
	//		// Define RBEL matrix and frame type for labels.
	//		SdifMatrixTypeT *labelsMatrixType = SdifCreateMatrixType(lorisLabelsSignature,NULL);
	//		SdifMatrixTypeInsertTailColumnDef(labelsMatrixType,"Index");  
	//		SdifMatrixTypeInsertTailColumnDef(labelsMatrixType,"Label");  
	//		SdifPutMatrixType(out->MatrixTypesTable, labelsMatrixType);
	//
	//		SdifFrameTypeT *labelsFrameType = SdifCreateFrameType(lorisLabelsSignature,NULL);
	//		SdifFrameTypePutComponent(labelsFrameType, lorisLabelsSignature, "RABWE_Labels");
	//		SdifPutFrameType(out->FrameTypesTable, labelsFrameType);
	//
	//		// Write file header information 
	//		SdifFWriteGeneralHeader( out );    
	//		
	//		// Write ASCII header information 
	//		SdifFWriteAllASCIIChunks( out );    
	
//
// Write SDIF data.
//	
	try 
	{
		// Make vector of pointers to partials.
		ConstPartialPtrs partialsVector;
		indexPartials( partials, partialsVector );

		// Write labels.
		writeEnvelopeLabels( out, partialsVector );
		
		// Write markers.
		writeMarkers(out, markers);
		
		// Write partials to SDIF file.
		writeEnvelopeData( out, partialsVector, enhanced );
	}
	catch ( Exception & ex ) 
	{
		ex.append( " Failed to write SDIF file." );
		SDIF_CloseWrite( out );
		throw;
	}

//
// Close SDIF input file.
//
	SDIF_CloseWrite( out );
}

// ---------------------------------------------------------------------------
//	Export
// ---------------------------------------------------------------------------
// Legacy static export member. 
//
void
SdifFile::Export( const std::string & filename, const PartialList & partials, 
				  const bool enhanced )
{
	SdifFile fout( partials.begin(), partials.end() );
	if ( enhanced )
		fout.write( filename );
	else
		fout.write1TRC( filename );
}


}	//	end of namespace Loris


