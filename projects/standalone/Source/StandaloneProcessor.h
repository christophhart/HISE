/*
  ==============================================================================

    StandaloneProcessor.h
    Created: 18 Sep 2014 10:28:46am
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef STANDALONEPROCESSOR_H_INCLUDED
#define STANDALONEPROCESSOR_H_INCLUDED

#include "JuceHeader.h"

class StandaloneProcessor
{
public:

	StandaloneProcessor()
	{
		deviceManager = new AudioDeviceManager();
		callback = new AudioProcessorPlayer();

		backendProcessor = new BackendProcessor(deviceManager, callback);

		ScopedPointer<XmlElement> deviceData = backendProcessor->getSettings();

		backendProcessor->initialiseAudioDriver(deviceData);
	};

	~StandaloneProcessor()
	{
		deviceManager->removeAudioCallback(callback);
		
		callback = nullptr;
		backendProcessor = nullptr;
		deviceManager = nullptr;
	}


	BackendProcessorEditor *createEditor()
	{
		return dynamic_cast<BackendProcessorEditor*>(backendProcessor->createEditor());
	}

private:

	ScopedPointer<BackendProcessor> backendProcessor;

	ScopedPointer<AudioDeviceManager> deviceManager;

	ScopedPointer<AudioProcessorPlayer> callback;



};



#endif  // STANDALONEPROCESSOR_H_INCLUDED
