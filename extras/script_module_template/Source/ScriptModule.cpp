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


		const float thisGain = gain.get();

		if (numSamples <= internalStorageSize)
		{
			for (int i = 0; i < numSamples; i++)
			{
				ch[i] *= internalStorage[i];
			}
		}
		else
		{
			for (int i = 0; i < numSamples; i++)
				ch[i] *= 1.0f * thisGain;
		}
    }
}

LoadingErrorCode initialise(const char* name)
{
	if (strcmp(name, "1234") != 0)
	{
		return LoadingErrorCode::KeyInvalid;
	}

	HelperFunctions::registerDspModule<GainExample>();

	return LoadingErrorCode::LoadingSuccessful;
}
