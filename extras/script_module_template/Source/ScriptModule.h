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

#define HISE_DLL 1

#include "../../../hi_core/hi_scripting/scripting/api/DspBaseModule.h"

DLL_EXPORT DspBaseObject* createDspObject(const char *name)
{
	if (DspBaseObject *b = baseObjects.createFromId(Identifier(name))) return b;

	return nullptr;
}


/** This interface class is the base class for all modules that can be loaded as plugin into the script FX processor. */
class GainExample : public DspBaseObject
{
public:

	/** Overwrite this method and return the name of this module. This will be used to identify the module within your library so it must be unique. */
	static Identifier getName() { RETURN_STATIC_IDENTIFIER("Gain") }

	// ================================================================================================================

    void prepareToPlay(double /*sampleRate*/, int /*blockSize*/) override {}

	/** Overwrite this method and do your processing on the given sample data. */
	void processBlock(float **data, int numChannels, int numSamples) override
	{
		for (int channel = 0; channel < numChannels; channel++)
		{
			for (int sample = 0; sample < numSamples; sample++)
			{
				data[channel][sample] *= gain;
			}
		}
	}

	// =================================================================================================================

	int getNumParameters() const override { return 1; }
	const Identifier &getIdForParameter(int /*index*/) const override { RETURN_STATIC_IDENTIFIER("Gain"); }
	float getParameter(int /*index*/) const override { return gain; }
	void setParameter(int /*index*/, float newValue) override { gain = newValue; }

private:

	float gain = 1.0f;
};



#endif  // SCRIPTMODULE_H_INCLUDED
