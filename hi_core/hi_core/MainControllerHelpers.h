/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef MAINCONTROLLERHELPERS_H_INCLUDED
#define MAINCONTROLLERHELPERS_H_INCLUDED

namespace hise { using namespace juce;

// ====================================================================================================
// Extern class definitions

class PluginParameterModulator;
class PluginParameterAudioProcessor;
class ControlledObject;
class Processor;
class Console;
class ModulatorSamplerSound;
class ModulatorSamplerSoundPool;
class AudioSampleBufferPool;
class Plotter;
class ScriptWatchTable;
class ScriptComponentEditPanel;
class JavascriptMidiProcessor;
class Modulator;
class CustomKeyboardState;
class ModulatorSynthChain;
class FactoryType;

class MainController;


/** A base class for all objects that need access to a MainController.
*	@ingroup core
*
*	If you want to have access to the main controller object, derive the class from this object and pass a pointer to the MainController
*	instance in the constructor.
*/
class ControlledObject
{
public:

	/** Creates a new ControlledObject. The MainController must be supplied. */
	ControlledObject(MainController *m);

	virtual ~ControlledObject();

	/** Provides read-only access to the main controller. */
	const MainController *getMainController() const noexcept
	{
		jassert(controller != nullptr);
		return controller;
	};

	/** Provides write access to the main controller. Use this if you want to make changes. */
	MainController *getMainController() noexcept
	{
		jassert(controller != nullptr);
		return controller;
	}

private:

	friend class WeakReference<ControlledObject>;
	WeakReference<ControlledObject>::Master masterReference;

	MainController* const controller;

	friend class MainController;
	friend class ProcessorFactory;
};

#define HI_NUM_MIDI_AUTOMATION_SLOTS 8

/** This handles the MIDI automation for the frontend plugin.
*
*	For faster performance, one CC value can only control one parameter.
*
*/
class MidiControllerAutomationHandler : public RestorableObject,
										public SafeChangeBroadcaster
{
public:

	MidiControllerAutomationHandler(MainController *mc_);

	void addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, int macroIndex);
	void removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NotificationType notifyListeners);

	bool isLearningActive() const;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	bool isLearningActive(Processor *interfaceProcessor, int attributeIndex) const;
	void deactivateMidiLearning();

	void setUnlearndedMidiControlNumber(int ccNumber, NotificationType notifyListeners);
	int getMidiControllerNumber(Processor *interfaceProcessor, int attributeIndex) const;

	void refreshAnyUsedState();
	void clear();

	/** The main routine. Call this for every MidiBuffer you want to process and it handles both setting parameters as well as MIDI learning. */
	void handleParameterData(MidiBuffer &b);

	

	struct AutomationData
	{
		AutomationData();

		void clear();

		bool operator==(const AutomationData& other) const;

		WeakReference<Processor> processor;
		int attribute;
		NormalisableRange<double> parameterRange;
		NormalisableRange<double> fullRange;
		float lastValue = -1.0f;
		int macroIndex;
		int ccNumber = -1;
		bool inverted = false;
		bool used;
	};

	/** Returns a copy of the automation data for the given index. */
	AutomationData getDataFromIndex(int index) const;

	int getNumActiveConnections() const;
	bool setNewRangeForParameter(int index, NormalisableRange<double> range);
	bool setParameterInverted(int index, bool value);
private:

	// ========================================================================================================

	

	CriticalSection lock;

	MainController *mc;
	bool anyUsed;
	MidiBuffer tempBuffer;

	Array<AutomationData> automationData[128];
	AutomationData unlearnedData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiControllerAutomationHandler)

	// ========================================================================================================
};


class OverlayMessageBroadcaster
{
public:

	class Listener
	{
	public:

		virtual void overlayMessageSent(int state, const String& message) = 0;

		virtual ~Listener()
		{
			masterReference.clear();
		}

	private:

		friend class WeakReference<Listener>;

		WeakReference<Listener>::Master masterReference;
	};

	OverlayMessageBroadcaster() :
		internalUpdater(this)
	{

	}

	void addOverlayListener(Listener *listener)
	{
		listeners.add(listener);
	}

	void removeOverlayListener(Listener* listener)
	{
		listeners.removeAllInstancesOf(listener);
	}

	void sendOverlayMessage(int newState, const String& newCustomMessage=String());

private:

	struct InternalAsyncUpdater: public AsyncUpdater
	{
		InternalAsyncUpdater(OverlayMessageBroadcaster *parent_): parent(parent_) {}

		void handleAsyncUpdate() override
		{
			for (int i = 0; i < parent->listeners.size(); i++)
			{
				if (parent->listeners[i].get() != nullptr)
				{
					parent->listeners[i]->overlayMessageSent(parent->currentState, parent->customMessage);
				}
				else
				{
					parent->listeners.remove(i--);
				}
			}
		}

		OverlayMessageBroadcaster* parent;
	};

	int currentState = 0;

	String customMessage;

	InternalAsyncUpdater internalUpdater;

	Array<WeakReference<Listener>> listeners;
};


class ConsoleLogger : public Logger
{
public:

	ConsoleLogger(Processor *p) :
		processor(p)
	{};

	void logMessage(const String &message) override;

private:

	Processor *processor;

};

class CircularAudioSampleBuffer
{
public:

	CircularAudioSampleBuffer() :
		internalBuffer(1, 0)
	{};

	CircularAudioSampleBuffer(int numChannels_, int numSamples);;

	bool writeSamples(const AudioSampleBuffer& source, int offsetInSource, int numSamples);

	bool writeMidiEvents(const MidiBuffer& source, int offsetInSource, int numSamples);


	bool readSamples(AudioSampleBuffer& destination, int offsetInDestination, int numSamples);

	bool readMidiEvents(MidiBuffer& destination, int offsetInDestination, int numSamples);

	void setReadDelta(int numSamplesBetweenReadWrite)
	{
		writeIndex = readIndex + numSamplesBetweenReadWrite;
		numAvailable += numSamplesBetweenReadWrite;
	}

	int getNumAvailableSamples() const
	{
		return numAvailable;
	};

	int getNumMidiEvents() const { return internalMidiBuffer.getNumEvents(); }

private:

	AudioSampleBuffer internalBuffer;
	MidiBuffer internalMidiBuffer;
	int size;

	int numAvailable = 0;

	int numChannels;
	int readIndex = 0;
	int writeIndex = 0;

	int midiReadIndex = 0;
	int midiWriteIndex = 0;

};



/** This introduces an artificial delay of max 256 samples and calls the internal processing loop with a fixed number of samples.
*
*	This is supposed to offer a rather ugly fallback solution for hosts who change their processing size constantly (eg. FL Studio).
*/
class DelayedRenderer
{
public:

	DelayedRenderer(MainController* mc);

	~DelayedRenderer();

	/** Checks whether this should be used. It currently is only activated on FL Studio. */
	bool shouldDelayRendering() const;

	/** Wraps the processing and delays the processing if necessary. */
	void processWrapped(AudioSampleBuffer& inputBuffer, MidiBuffer& midiBuffer);

	/** Calls prepareToPlay with either 256 samples or a smaller buffer size (if the block size is smaller). It correctly reports the latency to the host. */
	void prepareToPlayWrapped(double sampleRate, int samplesPerBlock);

private:

	class Pimpl;

	ScopedPointer<Pimpl> pimpl;

	MainController* mc;

	AudioSampleBuffer b1;
	AudioSampleBuffer b2;

	AudioSampleBuffer* readBuffer = nullptr;
	AudioSampleBuffer* writeBuffer = nullptr;

	CircularAudioSampleBuffer circularInputBuffer;
	CircularAudioSampleBuffer circularOutputBuffer;
	
	int lastBlockSize = 0;

	AudioSampleBuffer processBuffer;
	MidiBuffer delayedMidiBuffer;

	int fullBlockSize;

	int sampleIndexInternal = 0;
	int sampleIndexExternal = 0;

	
};

} // namespace hise

#endif  // MAINCONTROLLERHELPERS_H_INCLUDED
