/*
  ==============================================================================

    pAudioFormatReader.h
    Created: 5 Jul 2014 12:53:12am
    Author:  pac

  ==============================================================================
*/

#pragma once

#include "typedefs.h"
#include "../ProtoplugDir.h"

PROTO_API pAudioFormatReader AudioFormatReader_new(const char *filename)
{ 
	File f = ProtoplugDir::Instance()->getDir().getChildFile(filename);
	if (f == File::nonexistent)
		f = File(filename);
	AudioFormatManager afm;
	afm.registerBasicFormats();
	pAudioFormatReader a = {afm.createReaderFor(f)};
	if (a.a) {
		a.sampleRate = a.a->sampleRate;
		a.bitsPerSample = a.a->bitsPerSample;
		a.lengthInSamples = a.a->lengthInSamples;
		a.numChannels = a.a->numChannels;
		a.usesFloatingPointData = a.a->usesFloatingPointData;
	}
	return a;
}

PROTO_API bool AudioFormatReader_read (pAudioFormatReader a,
		int *const *  	destSamples,
		int  	numDestChannels,
		int64  	startSampleInSource,
		int  	numSamplesToRead,
		bool  	fillLeftoverChannelsWithCopies)
{ return a.a->read(destSamples, numDestChannels, startSampleInSource, numSamplesToRead, fillLeftoverChannelsWithCopies); }

PROTO_API void AudioFormatReader_delete(pAudioFormatReader a)
{ if (a.a) delete a.a; }
