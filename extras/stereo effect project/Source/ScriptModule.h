/*
  ==============================================================================

    ScriptModule.h
    Created: 5 Jul 2016 12:38:26pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef SCRIPTMODULE_H_INCLUDED
#define SCRIPTMODULE_H_INCLUDED


#include "JuceHeader.h"



/** This is an example class of a DSP module that can be loaded in a script processor within HISE. */
class StereoWidener : public DspBaseObject
{
public:

	enum class Parameters
	{
		Width = 0,
		PseudoStereoAmount,
		numParameters
	};

	StereoWidener() :
        width(1.0f),
        stereoAmount(0.0f)
    {}

	/** Overwrite this method and return the name of this module. 
    *
    *   This will be used to identify the module within your library so it must be unique. */
	static Identifier getName() { RETURN_STATIC_IDENTIFIER("stereo_widener"); }

	// ================================================================================================================

    void prepareToPlay(double sampleRate, int blockSize) override;

	/** Overwrite this method and do your processing on the given sample data. */
    void processBlock(float **data, int numChannels, int numSamples) override;

	// =================================================================================================================

	int getNumParameters() const override { return (int)Parameters::numParameters; }
	float getParameter(int index) const override
    {
        switch((Parameters)index)
        {
            case Parameters::Width: return width;
            case Parameters::PseudoStereoAmount: return stereoAmount;
        }
    }
    
	void setParameter(int index, float newValue) override
    {
        switch((Parameters)index)
        {
            case Parameters::Width: width = newValue; break;
            case Parameters::PseudoStereoAmount: stereoAmount = jlimit<float>(0.0f, 0.9f, newValue); break;
        }
    }

	// =================================================================================================================

	
    int getNumConstants() const { return (int)Parameters::numParameters; };

	void getIdForConstant(int index, char*name, int &size) const noexcept override
	{
		switch (index)
		{
		FILL_PARAMETER_ID(Parameters, Width, size, name);
		FILL_PARAMETER_ID(Parameters, PseudoStereoAmount, size, name);
		}
	};

	bool getConstant(int index, int& value) const noexcept override
	{ 
		if (index < getNumParameters())
		{
			value = index;
			return true;
		}

		return false;
	};
	
	bool getConstant(int index, float** data, int &size) noexcept override
	{
		return false; 
	};

private:

    
    inline float getDelayCoefficient(float delaySamples) const
    {
        return (1 - delaySamples) / (1 + delaySamples);
    };
    
    inline float getDelayedValueL(float input)
    {
        const float a = 0.99f;
        const float invA = 1.0f - a;
        
        const float thisDelay = lastDelayL * a + delayL * invA;
        lastDelayL = thisDelay;
        
        const float y = input * -thisDelay + currentValueL;
        currentValueL = y * thisDelay + input;
        
        return y;
    }
    
    inline float getDelayedValueR(float input)
    {
        const float a = 0.99f;
        const float invA = 1.0f - a;
        
        const float thisDelay = lastDelayR * a + delayR * invA;
        lastDelayR = thisDelay;
        
        const float y = input * -thisDelay + currentValueR;
        currentValueR = y * thisDelay + input;
        
        return y;
    }
    
    float width;
    float stereoAmount;
    
    float currentValueL = 0.0;
    float currentValueR = 0.0;
    float delayL = 0.0f;
    float delayR = 0.0f;
    float delaySamplesL = 0.1;
    float delaySamplesR = 1.9;
    float leftGain = 0.0;
    
    float lastWidth = 1.0f;
    float lastStereoAmount = 0.0f;
    
    float lastDelayL = 1.0f;
    float lastDelayR = 1.0f;
    
    float lastLeftGain = 1.0f;
};


#endif  // SCRIPTMODULE_H_INCLUDED
