/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/



#include "PluginProcessor.h"


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{

	AudioProcessorWrapper::addAudioProcessorToList("GainProcessor", &GainProcessor::create);
	AudioProcessorWrapper::addAudioProcessorToList("Spatializer", &Spatializer::create);

	return new BackendProcessor();	
};
