/*
  ==============================================================================

    ScriptModule.h
    Created: 5 Jul 2016 12:38:26pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef SCRIPTMODULE_H_INCLUDED
#define SCRIPTMODULE_H_INCLUDED


#include <JuceHeader.h>

#include "HiseLibraryHeader.h"

/** This is an example class of a DSP module that can be loaded in a script processor within HISE. */
class GainExample : public DspBaseObject
{
public:

	/** Overwrite this method and return the name of this module. 
    *
    *   This will be used to identify the module within your library so it must be unique. */
	static Identifier getName() { RETURN_STATIC_IDENTIFIER("Gain") }

	// ================================================================================================================

    void prepareToPlay(double sampleRate, int blockSize) override;

	/** Overwrite this method and do your processing on the given sample data. */
    void processBlock(float **data, int numChannels, int numSamples) override;

	// =================================================================================================================

	int getNumParameters() const override { return 1; }
	const Identifier &getIdForParameter(int /*index*/) const override { RETURN_STATIC_IDENTIFIER("Gain"); }
	float getParameter(int /*index*/) const override { return gain; }
	void setParameter(int /*index*/, float newValue) override { gain = newValue; }

	// =================================================================================================================

	
	int getNumConstants() const { return 4; };

	void getIdForConstant(int index, char* name, int &size) const noexcept override
	{
		switch (index)
		{
		case 0: SET_STRING(name, "someIntValue"); break;
		case 1: SET_STRING(name, "Gan"); break;
		case 2: SET_STRING(name, "someText"); break;
		case 3: SET_STRING(name, "internalStorage"); break;
		}
	};

	bool getConstant(int index, float& value) const noexcept override
	{
		return false; 
	};

	bool getConstant(int index, int& value) const noexcept override
	{ 
		if (index == 0)
		{
			value = 42;
			return true;
		}

		return false;
	};

	
	bool getConstant(int index, char* text, size_t& size) const noexcept override
	{
		if (index == 2)
		{
			SET_STRING(text, "Hello world");
			return true;
		}

		return false;
	};

	
	bool getConstant(int index, float** data, int &size) noexcept override
	{
		if (index == 1)
		{
			*data = &gain;
			size = 1;
			return true;
		}
		else if (index == 3)
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

	float gain = 1.0f;
};


#endif  // SCRIPTMODULE_H_INCLUDED
