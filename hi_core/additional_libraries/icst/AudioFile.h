// AudioFile.h
// Encapsulates an audio data buffer of float -1..1 and provides file IO.
// Allows safe buffer access from a second thread via GetSafePt(lock=true).
// If a file with supported write format is loaded and the audio buffer remains
// unchanged, save/append will create an exact copy of the original audio data.
// If the buffer value range exceeds 1, a normalized file will be written.
// Supported read formats: 
//		WAVE (PCM8/16/24/32, FLOAT32, both standard and WAVE_FORMAT_EXTENSIBLE)
//		AIFF (PCM8/16/24/32) 
//		AIFFC (PCM8/16/24/32, FLOAT32)
//		RAW (PCM8s+u/16/24/32, FLOAT32, big and little endian)
// Supported write formats:
//		WAVE (standard: PCM16, WAVE_FORMAT_EXTENSIBLE: PCM16/24, FLOAT32)
// Supported append formats:
//		WAVE (PCM16/24, FLOAT32, both standard and WAVE_FORMAT_EXTENSIBLE)
// *** begin notes ***
//		LoadRaw has not been thoroughly tested yet
// *** end notes ***  
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#ifndef _ICST_DSPLIB_AUDIOFILE_INCLUDED
#define _ICST_DSPLIB_AUDIOFILE_INCLUDED

namespace icstdsp {		// begin library specific namespace

//*** specific error codes ***
const int NOFILE	= -1;				// file not found
const int CORRUPT	= -2;				// file corrupt
const int ERRREAD	= -3;				// read file error
const int UNSAVED	= -4;				// could not write to file at all
const int INCOMPLETE = -5;				// could write only a part to file
const int FMTERR	= -6;				// no WAVE/AIFF format
const int NOSUPPORT	= -7;				// unsupported data format
const int MISMATCH	= -8;				// properties do not match
const int NODATA	= -9;				// audio buffer empty
const int NOMEMORY	= -10;				// out of audio data memory
const int NOMEMDEL	= -11;				// NOMEMORY, audio buffer deleted  

//*** for WAVE_FORMAT_EXTENSIBLE ***
// combine any of these flags to assign channels to speakers,
// use 0 for undefined speaker arrangements
const unsigned int SPK_FRONT_LEFT = 0x1;	
const unsigned int SPK_FRONT_RIGHT = 0x2;	
const unsigned int SPK_FRONT_CENTER = 0x4; 
const unsigned int SPK_LOW_FREQUENCY = 0x8; 
const unsigned int SPK_BACK_LEFT = 0x10;
const unsigned int SPK_BACK_RIGHT = 0x20;
const unsigned int SPK_FRONT_LEFT_OF_CENTER = 0x40;
const unsigned int SPK_FRONT_RIGHT_OF_CENTER = 0x80;
const unsigned int SPK_BACK_CENTER = 0x100;
const unsigned int SPK_SIDE_LEFT = 0x200;
const unsigned int SPK_SIDE_RIGHT = 0x400;
const unsigned int SPK_TOP_CENTER = 0x800;
const unsigned int SPK_TOP_FRONT_LEFT = 0x1000;
const unsigned int SPK_TOP_FRONT_CENTER = 0x2000;
const unsigned int SPK_TOP_FRONT_RIGHT = 0x4000;
const unsigned int SPK_TOP_BACK_LEFT = 0x8000;
const unsigned int SPK_TOP_BACK_CENTER = 0x10000;
const unsigned int SPK_TOP_BACK_RIGHT = 0x20000;

class AudioFile
{
public:
	AudioFile();
	~AudioFile();
	int Create(							// create new audio file
		unsigned int nsize=0,			// size in frames (samples/channels)
		unsigned int nchannels=1,		// number of channels
		unsigned int nresolution=16,	// resolution in bit
		unsigned int nrate=44100,		// sample rate in Hz
		unsigned int nspkpos=0 );		// speaker positions (see above)
	int Load(	char *filename,			// load file, determine format automatically
				unsigned int offset=0,	// frame offset for reading audio data
				unsigned int frames=0,	// number of frames to read (0: all)
				bool nodata=false );	// t: read properties only (an existing
										// audio buffer will be deleted)
	int LoadRaw(						// load raw file as audio
		char *filename,					// see Create and Load for parameters
		unsigned int offset=0,			//
		unsigned int frames=0,			//
		unsigned int nchannels=1,		// nresolution:
		unsigned int nresolution=16,	// 1-8:PCM8 signed or unsigned(format=true)	
		unsigned int nrate=44100,		// 9-16:PCM16, 17-24:PCM24, >24:PCM32 or
		unsigned int nspkpos=0,			// float(format=true)
		bool format=false,				// 
		bool bigendian=false );			// t: data interpreted as big endian
	int SaveWave(char *filename);		// save as WAVE file (16 bit audio is saved 
										// as extensible if channels > 2)
	int AppendWave(char *filename);		// append to existing WAVE file
	unsigned int GetSize();				// return size in frames
	unsigned int GetRate();				// return sample rate in Hz
	unsigned int GetChannels();			// return number of channels
	unsigned int GetResolution();		// return resolution in bits
	unsigned int GetSpkPos();			// return speaker positions
	float* GetSafePt(					// return pointer to audio data or NULL
		unsigned int channel=0,			// 0..channels-1
		bool lock=false	);				// true and pointer not NULL: pointer and
										// properties remain unchanged until next
										// call with false, locks Create/Load(Raw)
										// in a wait state during this time!							
private:								//*** see AudioFile.cpp for details	***	
	FILE* file;									
	float* audio;						
	volatile bool safe;							
	volatile bool locked;						
	unsigned int size;  				
	unsigned int rate;  				
	unsigned int spkpos;  				
	unsigned short resolution;  		
	unsigned short channels;  											
	void rev(unsigned int &x);			
	void rev(unsigned short &x);
	float getmaxabs();
	int LoadWave(unsigned int offset, unsigned int frames, bool nodata);		
	int LoadAiff(unsigned int offset, unsigned int frames, bool nodata);				
	double ReadExtFloat();
	bool GotoChunk(unsigned int tag, bool bigendian);
	AudioFile& operator = (const AudioFile& src);
	AudioFile(const AudioFile& src);
};

}	// end library specific namespace

#endif

