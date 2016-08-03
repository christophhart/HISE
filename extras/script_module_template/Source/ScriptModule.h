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
class GainExample : public DspBaseObject
{
public:

	enum class Parameters
	{
		Gain = 0,
		Volume,
		Frequency,
		numParameters
	};

	GainExample() : gain(1.0f) {}

	/** Overwrite this method and return the name of this module. 
    *
    *   This will be used to identify the module within your library so it must be unique. */
	static Identifier getName() { RETURN_STATIC_IDENTIFIER("Gain"); }

	// ================================================================================================================

    void prepareToPlay(double sampleRate, int blockSize) override;

	/** Overwrite this method and do your processing on the given sample data. */
    void processBlock(float **data, int numChannels, int numSamples) override;

	// =================================================================================================================

	int getNumParameters() const override { return (int)Parameters::numParameters; }
	float getParameter(int /*index*/) const override { return gain.get(); }
	void setParameter(int /*index*/, float newValue) override { gain.set(newValue); }

	void setStringParameter(int index, const char* textToCopy, size_t lengthToCopy)
	{

		fileName = HelperFunctions::createStringFromChar(textToCopy, lengthToCopy);
	}

	const char* getStringParameter(int index, size_t& length) override 
	{
		length = fileName.length();
		return fileName.getCharPointer(); 


	}


	// =================================================================================================================

	
	int getNumConstants() const { return 4; };

	void getIdForConstant(int index, char*name, int &size) const noexcept override
	{
		switch (index)
		{
		FILL_PARAMETER_ID(Parameters, Gain, size, name);
		FILL_PARAMETER_ID(Parameters, Volume, size, name);
		FILL_PARAMETER_ID(Parameters, Frequency, size, name);
		case 3: size = HelperFunctions::writeString(name, "internalStorage"); break;
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
		if (index == 3)
		{
			*data = internalStorage.getData();
			size = internalStorageSize;
			return true;
		}
		return false; 
	};

private:

	// This will be exposed to the scriptengine as buffer object via a constant.
	HeapBlock<float> internalStorage;
	int internalStorageSize = 0;

	String fileName;

	Atomic<float> gain;
};


#endif  // SCRIPTMODULE_H_INCLUDED
