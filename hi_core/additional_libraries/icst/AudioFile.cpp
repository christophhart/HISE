// AudioFile.cpp
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#include "common.h"
#include "AudioFile.h"
#include "MathDefs.h"
#include <climits>

#ifdef _WIN32	// Windows specific
	#ifndef ICSTLIB_ENABLE_MFC
		#include <windows.h>
	#endif
	#define ICSTDSP_RELINQUISH_TIME_SLICE Sleep(0)
#else			// POSIX specific
	#include <sched.h>
	#define ICSTDSP_RELINQUISH_TIME_SLICE sched_yield()
#endif

#define T_RIFF 0x46464952						// WAVE chunk tags, contain the 
#define T_WAVE 0x45564157						// ASCII code in reverse order:
#define T_FMT 0x20746d66						// T_RIFF = 'F''F''I''R', 
#define T_DATA 0x61746164						// T_FMT = ' ''t''m''f', etc.						
#define T_FACT 0x74636166						//
#define F_PCM 1									// supported WAVE data format
#define F_FLOAT 3								// tags: PCM, IEEE float,
#define F_EXT 0xfffe							// WAVE_FORMAT_EXTENSIBLE
#define T_FORM 0x4d524f46						// AIFF chunk tags, contain the
#define T_AIFF 0x46464941						// ASCII code in reverse order
#define T_AIFC 0x43464941						//
#define T_COMM 0x4d4d4f43						//
#define T_SSND 0x444e5353						//
#define F_F32 0x32336c66 						// supported AIFF data compression
#define F_F32CSND 0x32334c46					// tags: NONE, float (std + CSound)
#define F_NONE 0x454e4f4e						//

namespace icstdsp {		// begin library specific namespace

static const unsigned char WEXGUID[14] =		// WAVE_FORMAT_EXTENSIBLE GUID
{0,0,0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71};

static const float conv8 = 1.0f/128.0f;			// int-float conversion factors
static const float conv16 = 1.0f/32768.0f;
static const float conv24 = 1.0f/8388608.0f;
static const float conv32 = 1.0f/2147483648.0f;
static const float twopow30 = 1073741824.0f;

//*********************************************
//* construction, destruction, initialization
//*
AudioFile::AudioFile() 
{
	audio = NULL;						// audio data buffer
	file = NULL;						// file pointer
	safe = false;						// t: buffer is valid and contains data
	locked = false;						// t: no buffer invalidation allowed
	size = 0;							// size in frames
	rate = 0;							// sample rate in Hz
	resolution = 0;						// resolution in bit
	channels = 0;						// number of channels
	spkpos = 0;							// speaker positions
}

AudioFile::~AudioFile() 
{
	if (audio) {delete[] audio;}
}

// create new audio file
int AudioFile::Create(unsigned int nsize, unsigned int nchannels, 
		unsigned int nresolution, unsigned int nrate, unsigned int nspkpos)
{
	if (nchannels == 0) {return NOSUPPORT;}
	safe = false;
	while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
	if (audio) {delete[] audio; audio = NULL;}
	if (nsize > 0) {
		try {audio = new float[nsize*nchannels];} catch(...) {audio = NULL;}
		if (audio == NULL) {size = 0; return NOMEMDEL;}
		for (unsigned int i=0; i<(nsize*nchannels); i++) {audio[i] = 0;}
	}
	size = nsize; rate = nrate; spkpos = nspkpos;   
	resolution = (unsigned short)nresolution; channels = (unsigned short)nchannels;
	safe = true;
	return 0;
}

//******************************
//* file load operations
//*
// load file, determine format automatically
int AudioFile::Load(char *filename, unsigned int offset, unsigned int frames, 
					bool nodata)
{
	// open file
	if (filename[0] == 0) return NOFILE;
	if ((file = fopen(filename,"rb")) == NULL) return NOFILE;
	
	// check file type
	int err;
	unsigned int tag;
	fread (&tag, sizeof(int), 1, file);
	if (tag == T_RIFF) {err = LoadWave(offset, frames, nodata);}
	else if (tag == T_FORM) {err = LoadAiff(offset, frames, nodata);}
	else {err = FMTERR;} 
	    
	// close file
	fclose(file);
	return err;
}

// load audio data from open WAVE file
int AudioFile::LoadWave(unsigned int offset, unsigned int frames, bool nodata)
{
	int x;
	unsigned int i, j, k, m, data32, chunksize, bytesperword, nofsamples;
	unsigned int psize, prate, pspkpos = 0;
	unsigned short presolution, pchannels, pformat, blockalign;
	
	// check subfile type  
	fseek (file,4,SEEK_CUR);
	fread (&data32, sizeof(int), 1, file);
	if (data32!=T_WAVE) {return FMTERR;} 

	// search for format chunk skipping other chunks
	if (!GotoChunk(T_FMT, false)) {return CORRUPT;}

	// extract and check parameters from format chunk	   
	fread (&chunksize, sizeof(int), 1, file);
	fread (&pformat, sizeof(short), 1, file);
	fread (&pchannels, sizeof(short), 1, file);
	fread (&prate, sizeof(int), 1, file); 
	fseek (file,4,SEEK_CUR);
	fread (&blockalign, sizeof(short), 1, file); 
	fread (&presolution, sizeof(short), 1, file);
	if (pformat == F_EXT) {								// read subformat tag if 
		fseek (file,4,SEEK_CUR);						// WAVE_FORMAT_EXTENSIBLE
		fread (&pspkpos, sizeof(int), 1, file);
		fread (&pformat, sizeof(short), 1, file);
	}
	if ((pformat != F_PCM) && (pformat != F_FLOAT)) {return NOSUPPORT;}
	if ((presolution == 0) || (pchannels == 0)) {return CORRUPT;}
	bytesperword = 1 + (presolution-1)/8;
	if ((bytesperword*pchannels) != blockalign) {return NOSUPPORT;}

	// go back to the end of the main chunk and start searching
	// for a data chunk skipping other chunks
	fseek (file,12,SEEK_SET);
	if (!GotoChunk(T_DATA, false)) {return CORRUPT;}

	// extract size from data chunk  	      
	fread (&chunksize, sizeof(int), 1, file);
	psize = (chunksize/bytesperword)/pchannels;

	// adjust for segment reading
	if (offset < psize) {
		psize -= offset; 
		fseek (file,offset*pchannels*bytesperword,SEEK_CUR);
	}
	else {psize = 0;} 
	if (frames > 0) {psize = __min(frames,psize);}

	// fill buffer with deinterleaved data from file
	nofsamples = psize*pchannels;
	if ((nofsamples == 0) || (nodata))						// no audio data
	{
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio; audio = NULL;}
	}						
	else if ((pformat == F_FLOAT) && (bytesperword == 4))	// IEEE float 32 bit
	{
		float* buf;
		try {buf = new float[nofsamples];} catch(...) {buf = NULL;}
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(float),nofsamples,file) != nofsamples)
			{delete[] buf; return CORRUPT;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;} 
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {audio[i] = buf[k];}
		}
		delete[] buf;
	}
	else if (bytesperword == 1)								// PCM 8 bit unsigned
	{
		unsigned char* buf;
		try {buf = new unsigned char[nofsamples];} catch(...) {buf = NULL;}
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(char),nofsamples,file) != nofsamples)
			{delete[] buf; return CORRUPT;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {
				audio[i] = conv8*(static_cast<float>(buf[k]) - 128.0f);
			}
		}
		delete[] buf;
	}	
	else if (bytesperword == 2)								// PCM 16 bit
	{
		short* buf;
		try {buf = new short[nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(short),nofsamples,file) != nofsamples)
			{delete[] buf; return CORRUPT;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {
				audio[i] = conv16 * static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}
	else if (bytesperword == 3)								// PCM 24 bit
	{
		unsigned char* buf;
		try {buf = new unsigned char[3*nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(char),3*nofsamples,file) != (3*nofsamples))
			{delete[] buf; return CORRUPT;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;} 
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=3*j; m=3*pchannels;
			for (i=j*psize; i<((j+1)*psize); i++, k+=m) {
				x = static_cast<int>(buf[k]);
				x += (static_cast<int>(buf[k+1]) << 8);
				x += (static_cast<int>(static_cast<char>(buf[k+2])) << 16);
				audio[i] = conv24 * static_cast<float>(x);
			}
		}
		delete[] buf;
	}
	else if (bytesperword == 4)								// PCM 32 bit
	{
		int* buf;
		try {buf = new int[nofsamples];} catch(...) {buf = NULL;}
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(int),nofsamples,file) != nofsamples) 
			{delete[] buf; return CORRUPT;} 
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;} 
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {
				audio[i] = conv32 * static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}
	else {return NOSUPPORT;}

	// update properties and clean up
	size = psize; resolution = presolution; channels = pchannels; rate = prate;
	spkpos = pspkpos;
	if ((size > 0) && (audio)) {safe = true;}	
	return 0;
}

// load audio data from open AIFF or AIFF-C file
int AudioFile::LoadAiff(unsigned int offset, unsigned int frames, bool nodata)
{
	int x;
	unsigned int i, j, k, m, data32, chunksize, nofsamples, bytesperword;
	unsigned int psize, prate; 
	unsigned short pchannels, presolution, data16;
	bool aifc, usefloat = false;						
	
	// check subfile type
	fseek (file,4,SEEK_CUR);
	fread (&data32, sizeof(int), 1, file);
	if (data32 == T_AIFF) {aifc = false;}						// AIFF format
	else if (data32 == T_AIFC) {aifc = true;}					// AIFC format
	else {return FMTERR;}

	// search for common chunk skipping other chunks
	if (!GotoChunk(T_COMM, true)) {return CORRUPT;}

	// extract and check parameters from common chunk	   
	fread (&chunksize, sizeof(int), 1, file); rev(chunksize);
	fread (&pchannels, sizeof(short), 1, file); rev(pchannels);
	fread (&psize, sizeof(int), 1, file); rev(psize);
	fread (&presolution, sizeof(short), 1, file); rev(presolution);
	prate = static_cast<unsigned int>(0.5 + ReadExtFloat());
	if (aifc) {
		fread (&data32, sizeof(int), 1, file);
		if ((data32 == F_F32) || (data32 == F_F32CSND)) {usefloat = true;} 
		else if (data32 != F_NONE) {return NOSUPPORT;}
	}
	if ((presolution == 0) || (pchannels == 0)) {return CORRUPT;}
	bytesperword = 1 + (presolution-1)/8;
	
	// go back to the end of the form chunk and start searching for a 
	// sound data chunk skipping other chunks, jump to start of audio data,
	// AIFF files without audio data do not require a sound chunk
	if (psize > 0)	{	
		fseek (file,12,SEEK_SET);	
		if (!GotoChunk(T_SSND, true)) {return CORRUPT;}
		fseek (file,4,SEEK_CUR);
		fread (&data32, sizeof(int), 1, file); rev(data32);	// audio data offset
		fseek (file,4+data32,SEEK_CUR);
	}
	
	// adjust for segment reading
	if (offset < psize) {
		psize -= offset; 
		fseek (file,offset*pchannels*bytesperword,SEEK_CUR);
	}
	else {psize = 0;} 
	if (frames > 0) {psize = __min(frames,psize);}
	
	// fill buffer with deinterleaved data from file
	nofsamples = psize*pchannels;
	if ((nofsamples == 0) || (nodata))						// no audio data
	{
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio; audio = NULL;}
	}
	else if (usefloat)										// IEEE float 32 bit
	{
		float* buf;
		try {buf = new float[nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(float),nofsamples,file) != nofsamples)
			{delete[] buf; return CORRUPT;}
		char* p = reinterpret_cast<char*>(buf);		// big to little endian
		char ctmp1,ctmp2;							// cast to char* to preserve
		for (i=0; i<(4*nofsamples); i+=4) {			// strict aliasing compatibility
			ctmp1 = p[i];
			ctmp2 = p[i+1];
			p[i] = p[i+3];
			p[i+1] = p[i+2];
			p[i+2] = ctmp2;
			p[i+3] = ctmp1;
		}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {audio[i] = buf[k];}
		}
		delete[] buf;
	}
	else if (bytesperword == 1)								// PCM 8 bit signed
	{
		char* buf;
		try {buf = new char[nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(char),nofsamples,file) != nofsamples)
			{delete[] buf; return CORRUPT;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {
				audio[i] = conv8*static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}	
	else if (bytesperword == 2)								// PCM 16 bit
	{
		short* buf;
		try {buf = new short[nofsamples];} catch(...) {buf = NULL;}
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(short),nofsamples,file) != nofsamples)
			{delete[] buf; return CORRUPT;}
		for (i=0; i<nofsamples; i++) {						// big to little endian
			data16 = buf[i];
			buf[i] = ((data16>>8) & 0x00ff) | ((data16<<8) & 0xff00);
		}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {
				audio[i] = conv16 * static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}
	else if (bytesperword == 3)								// PCM 24 bit
	{
		unsigned char* buf;
		try {buf = new unsigned char[3*nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(char),3*nofsamples,file) != (3*nofsamples))
			{delete[] buf; return CORRUPT;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;} 
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=3*j; m=3*pchannels;
			for (i=j*psize; i<((j+1)*psize); i++, k+=m) {
				x = static_cast<int>(buf[k+2]);
				x += (static_cast<int>(buf[k+1]) << 8);
				x += (static_cast<int>(static_cast<char>(buf[k])) << 16);
				audio[i] = conv24 * static_cast<float>(x);
			}
		}
		delete[] buf;
	}
	else if (bytesperword == 4)								// PCM 32 bit
	{
		int* buf;
		try {buf = new int[nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {return NOMEMORY;}
		if (fread(buf,sizeof(int),nofsamples,file) != nofsamples) 
			{delete[] buf; return CORRUPT;} 
		for (i=0; i<nofsamples; i++) {						// big to little endian
			data32 = buf[i];
			buf[i] = ((data32>>24) & 0x000000ff) | ((data32>>8) & 0x0000ff00) | 
						((data32<<8) & 0x00ff0000) | ((data32<<24) & 0xff000000);
		}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[nofsamples];} catch (...) {audio = NULL;} 
		if (audio == NULL) {size = 0; delete[] buf; return NOMEMDEL;}
		for (j=0; j<pchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=pchannels) {
				audio[i] = conv32 * static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}
	else {return NOSUPPORT;}

	// update properties and clean up
	size = psize; resolution = presolution; channels = pchannels; rate = prate;
	spkpos = 0;
	if ((size > 0) && (audio)) {safe = true;}
	return 0;
}

// load raw file as audio
int AudioFile::LoadRaw(char *filename,	unsigned int offset, unsigned int frames,
			unsigned int nchannels, unsigned int nresolution, unsigned int nrate, 
			unsigned int nspkpos, bool format, bool bigendian)
{
	unsigned int psize, bytesperword;

	// prepare
	if ((nresolution == 0) || (nchannels == 0)) return NOSUPPORT;
	nresolution = __min(32,nresolution);
	bytesperword = 1 + (nresolution-1)/8;

	// open file
	if (filename[0] == 0) return NOFILE;
	if ((file = fopen(filename,"rb")) == NULL) return NOFILE;

	// get file size in frames
	fseek(file,0,SEEK_END);
	psize = (ftell(file)/bytesperword)/nchannels;
	
	// adjust for segment reading
	if (offset < psize) {
		psize -= offset;
		fseek(file,offset,SEEK_SET);
	}
	else {psize = 0;} 
	if (frames > 0) {psize = __min(frames,psize);}

	// fill buffer with deinterleaved data from file
	int x;
	unsigned int i,j,k,m;
	short data16;
	if (psize == 0)											// no audio data
	{
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio; audio = NULL;}
	}						
	else if (bytesperword == 1)								// PCM 8 bit
	{
		unsigned char* buf;
		try {buf = new unsigned char[psize*nchannels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		psize = fread(buf,nchannels*sizeof(char),psize,file);
		if (psize == 0)	{delete[] buf; fclose(file); return ERRREAD;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[psize*nchannels];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; fclose(file); return NOMEMDEL;}
		if (format) {										// unsigned	
			for (j=0; j<nchannels; j++) {					
				k=j;											
				for (i=j*psize; i<((j+1)*psize); i++, k+=nchannels) {
					audio[i] = conv8*(static_cast<float>(buf[k]) - 128.0f);
				}
			}
		}
		else {												// signed	
			for (j=0; j<nchannels; j++) {
				k=j;
				for (i=j*psize; i<((j+1)*psize); i++, k+=nchannels) {
					audio[i] = conv8*(static_cast<float>(static_cast<char>(buf[k])));
				}
			}
		}
		delete[] buf;
	}	
	else if (bytesperword == 2)								// PCM 16 bit
	{
		short* buf;
		try {buf = new short[psize*nchannels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		psize = fread(buf,nchannels*sizeof(short),psize,file);
		if (psize == 0)	{delete[] buf; fclose(file); return ERRREAD;}
		if (bigendian) {
			for (i=0; i<(psize*nchannels); i++) {		
				data16 = buf[i];
				buf[i] = ((data16>>8) & 0x00ff) | ((data16<<8) & 0xff00);
			}
		}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}					// relinquish thread if blocked
		if (audio) {delete[] audio;}
		try {audio = new float[psize*nchannels];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; fclose(file); return NOMEMDEL;}
		for (j=0; j<nchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=nchannels) {
				audio[i] = conv16 * static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}
	else if (bytesperword == 3)								// PCM 24 bit
	{
		unsigned char* buf;
		try {buf = new unsigned char[3*psize*nchannels];} catch(...) {buf = NULL;} 
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		psize = fread(buf,3*nchannels*sizeof(char),psize,file); 
		if (psize == 0)	{delete[] buf; fclose(file); return ERRREAD;}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[3*psize*nchannels];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; fclose(file); return NOMEMDEL;}
		if (bigendian) {
			for (j=0; j<nchannels; j++) {
				k=3*j; m=3*nchannels;
				for (i=j*psize; i<((j+1)*psize); i++, k+=m) {
					x = static_cast<int>(buf[k+2]);
					x += (static_cast<int>(buf[k+1]) << 8);
					x += (static_cast<int>(static_cast<char>(buf[k])) << 16);
					audio[i] = conv24 * static_cast<float>(x);
				}
			}
		}
		else {
			for (j=0; j<nchannels; j++) {
				k=3*j; m=3*nchannels;
				for (i=j*psize; i<((j+1)*psize); i++, k+=m) {
					x = static_cast<int>(buf[k]);
					x += (static_cast<int>(buf[k+1]) << 8);
					x += (static_cast<int>(static_cast<char>(buf[k+2])) << 16);
					audio[i] = conv24 * static_cast<float>(x);
				}
			}
		}
		delete[] buf;
	}
	else if ((bytesperword == 4) && (!format))				// PCM 32 bit
	{
		int* buf;
		try {buf = new int[psize*nchannels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		psize = fread(buf,nchannels*sizeof(int),psize,file);
		if (psize == 0)	{delete[] buf; fclose(file); return ERRREAD;}
		if (bigendian) {
			for (i=0; i<(psize*nchannels); i++) {					
				x = buf[i];
				buf[i] = ((x>>24) & 0x000000ff) | ((x>>8) & 0x0000ff00) | 
						((x<<8) & 0x00ff0000) | ((x<<24) & 0xff000000);
			}
		}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[psize*nchannels];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; fclose(file); return NOMEMDEL;}
		for (j=0; j<nchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=nchannels) {
				audio[i] = conv32 * static_cast<float>(buf[k]);
			}
		}
		delete[] buf;
	}
	else if ((bytesperword == 4) && format)					// IEEE float 32 bit
	{
		float* buf;
		try {buf = new float[psize*nchannels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		psize = fread(buf,nchannels*sizeof(float),psize,file);
		if (psize == 0)	{delete[] buf; fclose(file); return ERRREAD;}
		if (bigendian) {
			char* p = reinterpret_cast<char*>(buf);		// big to little endian
			char ctmp1,ctmp2;							// cast to char* to preserve
			for (i=0; i<(4*psize*nchannels); i+=4) {	// strict aliasing compatibility
				ctmp1 = p[i];
				ctmp2 = p[i+1];
				p[i] = p[i+3];
				p[i+1] = p[i+2];
				p[i+2] = ctmp2;
				p[i+3] = ctmp1;
			}
		}
		safe = false;
		while (locked) {ICSTDSP_RELINQUISH_TIME_SLICE;}
		if (audio) {delete[] audio;}
		try {audio = new float[psize*nchannels];} catch (...) {audio = NULL;}
		if (audio == NULL) {size = 0; delete[] buf; fclose(file); return NOMEMDEL;}
		for (j=0; j<nchannels; j++) {
			k=j;
			for (i=j*psize; i<((j+1)*psize); i++, k+=nchannels) {audio[i] = buf[k];}
		}
		delete[] buf;
	}

	// update properties and clean up
	size = psize; resolution = nresolution; channels = nchannels;
	rate = nrate; spkpos = nspkpos;
	if ((size > 0) && (audio)) {safe = true;}
	fclose(file);
	return 0;
}

//******************************
//* file save operations
//*
// save audio data to WAVE file
int AudioFile::SaveWave(char *filename)
{
	// prepare
	if ((!safe) && (size>0)) return NODATA;					// allow 0 audio frames
	if ((channels == 0) || (rate == 0)) return NOSUPPORT;
	unsigned int data32;
	unsigned short data16;
	unsigned int bytesperword = __min(4,__max(2,(resolution+7)/8));
	unsigned short blockalign = bytesperword*channels;
	unsigned int nofsamples = size*channels;
	unsigned int avgbps = rate*blockalign;
	unsigned int audiobytes = blockalign*size;
	unsigned int fill = audiobytes % 2;
	unsigned int allcsize;
	unsigned int fmtcsize; 
	bool wext = false, usefloat = false;
	if (size > ((UINT_MAX-73)/blockalign)) return UNSAVED;	// chunk size out of range 
	if ((bytesperword > 2) || (channels > 2)) {
		wext = true; allcsize = 72 + audiobytes + fill; fmtcsize = 40;}
	else {allcsize = 36 + audiobytes + fill; fmtcsize = 16;}
	if (bytesperword > 3) {usefloat = true;}

	// open file
	if (filename[0] == 0) return NOFILE;
	if ((file = fopen(filename,"wb")) == NULL) return NOFILE;

	// write main chunk header
	data32 = T_RIFF; fwrite (&data32, sizeof(int), 1, file);
	fwrite (&allcsize, sizeof(int), 1, file);
	data32 = T_WAVE; fwrite (&data32, sizeof(int), 1, file);

	// write format and optional fact chunk
	data32 = T_FMT; fwrite (&data32, sizeof(int), 1, file);
	fwrite (&fmtcsize, sizeof(int), 1, file);
	if (wext) {data16 = F_EXT;} else {data16 = F_PCM;}
	fwrite (&data16, sizeof(short), 1, file);
	fwrite (&channels, sizeof(short), 1, file);
	fwrite (&rate, sizeof(int), 1, file);
	fwrite (&avgbps, sizeof(int), 1, file);
	fwrite (&blockalign, sizeof(short), 1, file);
	data16 = 8*bytesperword; fwrite (&data16, sizeof(short), 1, file);
	if (wext)
	{
		data16 = 22; fwrite (&data16, sizeof(short), 1, file);
		data16 = 8*bytesperword; fwrite (&data16, sizeof(short), 1, file);
		fwrite (&spkpos, sizeof(int), 1, file);
		if (usefloat) {data16 = F_FLOAT;} else {data16 = F_PCM;}
		fwrite (&data16, sizeof(short), 1, file);
		fwrite (WEXGUID, sizeof(char), 14, file);
		// fact chunk
		data32 = T_FACT; fwrite (&data32, sizeof(int), 1, file);
		data32 = 4; fwrite (&data32, sizeof(int), 1, file);
		data32 = size; fwrite (&data32, sizeof(int), 1, file);
	}

	// write data chunk header
	data32 = T_DATA; fwrite (&data32, sizeof(int), 1, file);
	fwrite (&audiobytes, sizeof(int), 1, file);
	if (audiobytes == 0) {fclose(file); return 0;}				// no audio data
	
	// interleave and write audio data to file
	int x;
	unsigned int i,j,k,m;
	float conv = getmaxabs();									// normalize if range							
	if (conv > 1.0f) {conv = 1.0f/conv;} else {conv = 1.0f;}	// exceeds -1..1
	if (bytesperword == 2)										// PCM 16 bit
	{
		short* buf;
		try {buf = new short[nofsamples];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		conv *= twopow30;
		for (j=0; j<channels; j++) {
			k=j;
			for (i=j*size; i<((j+1)*size); i++, k+=channels) {
				x = (static_cast<int>(conv*audio[i] + 16384.0f)) >> 15;
				x = __min(x,32767); x = __max(x,-32768);
				buf[k] = static_cast<short>(x);
			}
		}
		if (fwrite(buf,sizeof(short),nofsamples,file) != nofsamples) {
			delete[] buf; fclose(file); remove(filename); return UNSAVED;}
		delete[] buf;
	}
	else if (bytesperword == 3)									// PCM 24 bit
	{													
		unsigned char* buf;
		try {buf = new unsigned char[3*nofsamples + 1];} catch(...) {buf = NULL;} 
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		conv *= twopow30;
		for (j=0; j<channels; j++) {
			k=3*j; m=3*channels;
			for (i=j*size; i<((j+1)*size); i++, k+=m) {
				x = (static_cast<int>(conv*audio[i] + 64.0f)) >> 7;
				x = __min(x,8388607); x = __max(x,-8388608);
				buf[k] = static_cast<unsigned char>(x & 0x000000ff);
				buf[k+1] = static_cast<unsigned char>((x >> 8) & 0x000000ff);
				buf[k+2] = static_cast<unsigned char>((x >> 16) & 0x000000ff);
			}
		}
		buf[3*nofsamples] = 0;
		if (fwrite(buf,sizeof(char),3*nofsamples+fill,file) != (3*nofsamples+fill)) {
			delete[] buf; fclose(file); remove(filename); return UNSAVED;}
		delete[] buf;
	}
	else if (bytesperword == 4)									// IEEE float 32 bit
	{													
		float* buf;
		try {buf = new float[nofsamples];} catch(...) {buf = NULL;} 
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		for (j=0; j<channels; j++) {
			k=j;
			for (i=j*size; i<((j+1)*size); i++, k+=channels) {buf[k] = conv*audio[i];}
		}
		if (fwrite(buf,sizeof(float),nofsamples,file) != nofsamples) {
			delete[] buf; fclose(file); remove(filename); return UNSAVED;}
		delete[] buf;
	}

	// clean up
	fclose(file);	
	return 0;
}

// append audio data to existing WAVE file
int AudioFile::AppendWave(char *filename)
{
	unsigned int data32, oldallcsize, oldsize, oldbytesperword, written=0;
	unsigned int bytesperword = __min(4,__max(2,(resolution+7)/8));
	unsigned short data16, oldformat;
	bool wext = false;

	// open file
	if (!safe) return NODATA;
	if ((channels == 0) || (rate == 0)) return NOSUPPORT;
	if (filename[0] == 0) return NOFILE;
	if ((file = fopen(filename,"r+b")) == NULL) return NOFILE;
	
	// check file type, get total chunk size
	fread (&data32, sizeof(int), 1, file);
	if (data32 != T_RIFF) {fclose(file); return FMTERR;} 
	fread (&oldallcsize, sizeof(int), 1, file);
	fread (&data32, sizeof(int), 1, file);
	if (data32 != T_WAVE) {fclose(file); return FMTERR;} 

	// search for format chunk skipping other chunks
	if (!GotoChunk(T_FMT, false)) {fclose(file); return CORRUPT;}

	// do the file properties match those of the audio data to append?	   
	fseek(file,4,SEEK_CUR);
	fread (&oldformat, sizeof(short), 1, file);
	fread (&data16, sizeof(short), 1, file);
	if (data16 != channels) {fclose(file); return MISMATCH;}
	fread (&data32, sizeof(int), 1, file); 
	if (data32 != rate) {fclose(file); return MISMATCH;}
	fread (&data32, sizeof(int), 1, file); 
	if (data32 != rate*bytesperword*channels) {fclose(file); return MISMATCH;}
	fread (&data16, sizeof(short), 1, file);
	if (data16 != bytesperword*channels) {fclose(file); return MISMATCH;}
	fread (&data16, sizeof(short), 1, file);
	oldbytesperword = 1 + (__max(data16,1)-1)/8;
	if (oldbytesperword != bytesperword) {fclose(file); return MISMATCH;} 
	if (oldformat == F_EXT) {							// read subformat tag if 
		fseek (file,8,SEEK_CUR);						// WAVE_FORMAT_EXTENSIBLE
		fread (&oldformat, sizeof(short), 1, file);
		wext = true;
	}
	if ((oldformat == F_PCM) && ((oldbytesperword == 2) || (oldbytesperword == 3))) {}
	else if (oldformat == F_FLOAT) {}
	else {fclose(file); return MISMATCH;}

	// search for data chunk skipping other chunks, do not support files with
	// a format chunk that follows the data chunk
	fseek(file,12,SEEK_SET);
	GotoChunk(T_FMT, false);
	fseek(file,-4,SEEK_CUR);
	if (!GotoChunk(T_DATA, false)) {fclose(file); return MISMATCH;}

	// extract number of audio frames from data chunk and jump to its end  	      
	fread (&data32, sizeof(int), 1, file);
	if ((data32 % 2) == 1) {oldallcsize--;}				// a fill byte was inserted
	oldsize = (data32/bytesperword)/channels;
	if (fseek(file,data32,SEEK_CUR) != 0) {fclose(file); return CORRUPT;}
	if (size > (((UINT_MAX-oldallcsize-1)/bytesperword)/channels)) 
		{fclose(file); return UNSAVED;}					// chunk size becomes too large

	// interleave and write audio data to file (mostly copy-paste from SaveWave)
	int x, err = 0;
	char dummy = 0;
	unsigned int i,j,k,m;
	float conv = getmaxabs();																
	if (conv > 1.0f) {conv = 1.0f/conv;} else {conv = 1.0f;}
	if (bytesperword == 2)								
	{
		short* buf;
		try {buf = new short[size*channels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		conv *= twopow30;
		for (j=0; j<channels; j++) {
			k=j;
			for (i=j*size; i<((j+1)*size); i++, k+=channels) {
				x = (static_cast<int>(conv*audio[i] + 16384.0f)) >> 15;
				x = __min(x,32767); x = __max(x,-32768);
				buf[k] = static_cast<short>(x);
			}
		}
		written = fwrite(buf,channels*sizeof(short),size,file);
		if (written != size) {err = INCOMPLETE;}
		delete[] buf;
	}
	else if (bytesperword == 3)								
	{													
		unsigned char* buf;
		try {buf = new unsigned char[3*size*channels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		conv *= twopow30;
		for (j=0; j<channels; j++) {
			k=3*j; m=3*channels;
			for (i=j*size; i<((j+1)*size); i++, k+=m) {
				x = (static_cast<int>(conv*audio[i] + 64.0f)) >> 7;
				x = __min(x,8388607); x = __max(x,-8388608);
				buf[k] = static_cast<unsigned char>(x & 0x000000ff);
				buf[k+1] = static_cast<unsigned char>((x >> 8) & 0x000000ff);
				buf[k+2] = static_cast<unsigned char>((x >> 16) & 0x000000ff);
			}
		}
		written = fwrite(buf,channels*3*sizeof(char),size,file);
		if (written != size) {err = INCOMPLETE;}
		delete[] buf;
	}
	else if (bytesperword == 4)						
	{											
		float* buf;
		try {buf = new float[size*channels];} catch(...) {buf = NULL;}
		if (buf == NULL) {fclose(file); return NOMEMORY;}
		for (j=0; j<channels; j++) {
			k=j;
			for (i=j*size; i<((j+1)*size); i++, k+=channels) {buf[k] = conv*audio[i];}
		}
		written = fwrite(buf,channels*sizeof(float),size,file);
		if (written != size) {err = INCOMPLETE;}
		delete[] buf;
	}

	// update length in all required fields
	data32 = oldallcsize + written*channels*bytesperword;
	if ((data32 % 2) == 1) {								// insert fill byte
		data32++;
		fwrite(&dummy,sizeof(char),1,file);
	}
	fseek(file,4,SEEK_SET);
	fwrite (&data32, sizeof(int), 1, file);
	fseek(file,12,SEEK_SET);								// jump to end of main chunk	
	if (wext) {												// WAVE_FORMAT_EXTENSIBLE:
		if (GotoChunk(T_FACT, false)) {						// update FACT chunk if any
			fseek(file,4,SEEK_CUR);
			data32 = oldsize + written;
			fwrite (&data32, sizeof(int), 1, file);
		}	
		fseek(file,12,SEEK_SET);							// jump to end of main chunk
	}
	GotoChunk(T_DATA, false);
	data32 = (oldsize+written)*channels*bytesperword;
	fseek(file,0,SEEK_CUR);									// dummy fseek, required if
	fwrite(&data32, sizeof(int), 1, file);					// access type (read,write)
															// changes (GotoChunk reads!)
	fclose(file);
	return err;
}
										
//******************************
//* properties
//*
// return safe pointer to audio data array
float* AudioFile::GetSafePt(unsigned int channel, bool lock) 
{
	if (channel >= channels) {locked = false; return NULL;}
	if (!(lock && locked))
	{
		locked = lock;
		if (!safe) {locked = false; return NULL;}
	}
	return audio + channel*size;
}

// return array size in frames (samples / channels)
unsigned int AudioFile::GetSize() {return size;}

// return sample rate in Hz
unsigned int AudioFile::GetRate() {return rate;}

// return resolution in bits
unsigned int AudioFile::GetResolution() {return (unsigned int)resolution;}

// return number of channels
unsigned int AudioFile::GetChannels() {return (unsigned int)channels;}

//******************************
//* auxiliary
//*
// return maximum absolute value of audio data
float AudioFile::getmaxabs()
{
	float x, max=0, min=0;
	for (unsigned int i=0; i<(size*channels); i++) {
		x = audio[i];
		max = __max(x,max);							// MSVC6 (no SSE): 4x faster than
		min = __min(x,min);							// using fabs(double) with float 
	}
	return (float)__max(max,-min);
}

// reverse byte order, convert big to little endian and vice versa
void AudioFile::rev(unsigned short &x)
{
	x = ((x>>8) & 0x00ff) | ((x<<8) & 0xff00);
}
void AudioFile::rev(unsigned int &x)
{
	x = ((x>>24) & 0x000000ff) | ((x>>8) & 0x0000ff00) | 
		((x<<8) & 0x00ff0000) | ((x<<24) & 0xff000000);
}

// read IEEE extended float value (80 bit) and convert to double
double AudioFile::ReadExtFloat()
{			
    int ex;											// exponent
	unsigned char exword[2];							
    unsigned int mh, ml;							// mantissa
	double res = HUGE_VAL;
	fread (exword, sizeof(char), 2, file);
	fread (&mh, sizeof(int), 1, file); rev(mh);
	fread (&ml, sizeof(int), 1, file); rev(ml);
	ex = ((exword[0] & 0x7f) << 8) | exword[1];
	if (ex < 0x7fff) {
		res = ldexp(static_cast<double>(mh+INT_MIN) - INT_MIN, ex-16414);
		res += ldexp(static_cast<double>(ml+INT_MIN) - INT_MIN, ex-16446);
    }
    if (exword[0] & 0x80) {return -res;} else {return res;}
}

// advance to a certain chunk
bool AudioFile::GotoChunk(unsigned int tag, bool bigendian)
{
	unsigned int data, chunksize;
	fread (&data, sizeof(int), 1, file);  
	while (data != tag)   
	{
		fread (&chunksize, sizeof(int), 1, file);
		if (bigendian) {rev(chunksize);}
		chunksize = 2*((chunksize + 1)/2);
		if (fseek (file,chunksize,SEEK_CUR) != 0) {return false;} 
		fread (&data, sizeof(int), 1, file);
	}
	return true;
}

}	// end library specific namespace

