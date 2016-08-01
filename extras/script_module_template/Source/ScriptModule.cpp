#include "ScriptModule.h"

void GainExample::prepareToPlay(double sampleRate, int blockSize)
{
    ignoreUnused(sampleRate, blockSize);

	// Resizes the internal buffer. This method will be preceded by a call to getConstantValue(), if any constant points to this data.
	internalStorage.allocate(blockSize, true);
	internalStorageSize = blockSize;
}

void GainExample::processBlock(float **data, int numChannels, int numSamples)
{
    for (int channel = 0; channel < numChannels; channel++)
    {
		float* ch = data[channel];

		for (int i = 0; i < numSamples; i++)
		{
			ch[i] *= internalStorage[i];
		}
    }
}

// This registers the GainExample class to the module factory of this library.
// You can of course write multiple classes and register each one of them to a single library
// so that one library can hold a collection of different effects
void initialise()
{
	baseObjects.registerType<GainExample>();
}


bool matchPassword(const char* password)
{
	return strcmp(password, "1234") == 0;
}