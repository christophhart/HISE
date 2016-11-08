#include "ScriptModule.h"

void StereoWidener::prepareToPlay(double sampleRate, int blockSize)
{
    ignoreUnused(sampleRate, blockSize);
}


void StereoWidener::processBlock(float **data, int numChannels, int numSamples)
{
    if(numChannels == 2)
    {
        delayL = getDelayCoefficient(1.0 + 0.9 * stereoAmount);
        delayR = getDelayCoefficient(1.0 - 0.9 * stereoAmount);
        
        const float a = 0.99f;
        const float invA = 1.0f - a;
        
        float* l = data[0];
        float* r = data[1];
        
        const float leftGainBuffer = (1.0f - stereoAmount) * 0.5f + 0.5f;
        
        for(int i = 0; i < numSamples; i++)
        {
            const float thisL = getDelayedValueL(l[i] + currentValueL * 0.5f);
            const float thisR = getDelayedValueR(r[i] + currentValueR * 0.5f);
            
            const float thisM = thisL + thisR;
            const float thisS = thisL - thisR;
            
            const float thisWidth = a * lastWidth + invA * width;
            lastWidth = thisWidth;
            
            l[i] = 0.25f * (thisM + thisWidth * thisS);
            r[i] = 0.25f * (thisM - thisWidth * thisS);
            
            const float thisLeftGain = a * lastLeftGain + invA * leftGainBuffer;
            lastLeftGain = thisLeftGain;
            
            r[i] *= thisLeftGain;
        }
    }
    
}

LoadingErrorCode initialise(const char* name)
{
	HelperFunctions::registerDspModule<StereoWidener>();

	return LoadingErrorCode::LoadingSuccessful;
}
