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
class Plotter;
class ScriptWatchTable;
class ScriptComponentEditPanel;
class JavascriptMidiProcessor;
class Modulator;
class CustomKeyboardState;
class ModulatorSynthChain;
class FactoryType;
class JavascriptThreadPool;

class MainController;

class MPEModulator;


#if HISE_INCLUDE_RLOTTIE

class HiseRLottieManager : public RLottieManager,
	public ControlledObject
{
public:

	HiseRLottieManager(MainController* mc) :
		ControlledObject(mc),
		RLottieManager()
	{};

	File getLibraryFolder() const override
	{
#if JUCE_WINDOWS
		return File::getSpecialLocation(File::windowsSystemDirectory);
#elif JUCE_LINUX
	return File("/usr/lib/");
#else
	return File("/usr/local/lib/");
#endif
	}
};

#endif


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
	void clear(NotificationType notifyListeners);

	/** The main routine. Call this for every MidiBuffer you want to process and it handles both setting parameters as well as MIDI learning. */
	void handleParameterData(MidiBuffer &b);

	bool handleControllerMessage(const HiseEvent& e);
		
	class MPEData : public ControlledObject,
					public RestorableObject,
					public Dispatchable
	{
	public:
		MPEData(MainController* mc);;

		~MPEData();

		struct Listener
		{
		public:
			virtual ~Listener() {};

			virtual void mpeModeChanged(bool isEnabled) = 0;

			virtual void mpeModulatorAssigned(MPEModulator* m, bool wasAssigned) = 0;

			virtual void mpeDataReloaded() = 0;

			virtual void mpeModulatorAmountChanged() {};

		private:

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

        enum EventType
        {
            MPEModeChanged,
            MPEModConnectionAdded,
            MPEModConnectionRemoved,
            MPEDataReloaded,
            MPEModulatorAmountChanged,
            numEventTypes
        };
        
		void restoreFromValueTree(const ValueTree &previouslyExportedState) override;

		ValueTree exportAsValueTree() const override;

        void sendAsyncNotificationMessage(MPEModulator* mod, EventType type);
        
		void addConnection(MPEModulator* mod, NotificationType notifyListeners=sendNotification);

		void removeConnection(MPEModulator* mod, NotificationType notifyListeners=sendNotification);

		MPEModulator* getModulator(int index) const;

		MPEModulator* findMPEModulator(const String& name) const;

		StringArray getListOfUnconnectedModulators(bool prettyName) const;

		static String getPrettyName(const String& id);

		void reset();

		void clear();

		int size() const;

		void setMpeMode(bool shouldBeOn);

		bool isMpeEnabled() const { return mpeEnabled; }

		bool contains(MPEModulator* mod) const;

		void addListener(Listener* l)
		{
			listeners.addIfNotAlreadyThere(l);

			// Fire this once to setup the correct state
			l->mpeModeChanged(mpeEnabled);
		}

		void removeListener(Listener* l)
		{
			listeners.removeAllInstancesOf(l);
		}

		void sendAmountChangeMessage()
		{
			ScopedLock sl(listeners.getLock());

			for (auto l : listeners)
			{
				if (l)
					l->mpeModulatorAmountChanged();
			}
		}

	private:

		struct AsyncRestorer : private Timer
		{
		public:

			AsyncRestorer(MPEData& parent_) :
				parent(parent_)
			{};

			void restore(const ValueTree& v)
			{
				data = v;
				dirty = true;
				startTimer(50);
			}

		private:

			void timerCallback() override;

			bool dirty = false;

			ValueTree data;

			MPEData& parent;
		};

		ValueTree pendingData;

		AsyncRestorer asyncRestorer;
		

		bool mpeEnabled = false;

		struct Data;

		struct Connection;

		ScopedPointer<Data> data;

		Array<WeakReference<Listener>, CriticalSection> listeners;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPEData);
		JUCE_DECLARE_WEAK_REFERENCEABLE(MPEData);
	};
	

	struct AutomationData: public RestorableObject
	{
		AutomationData();

		~AutomationData() { clear(); }

		void clear();

		bool operator==(const AutomationData& other) const;

		void restoreFromValueTree(const ValueTree &v) override;

		ValueTree exportAsValueTree() const override;

		MainController* mc = nullptr;
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

	MPEData& getMPEData() { return mpeData; }

	const MPEData& getMPEData() const { return mpeData; }

	int getNumActiveConnections() const;
	bool setNewRangeForParameter(int index, NormalisableRange<double> range);
	bool setParameterInverted(int index, bool value);

	void setUnloadedData(const ValueTree& v)
	{
		unloadedData = v;
	};

	void loadUnloadedData()
	{
		if(unloadedData.isValid())
			restoreFromValueTree(unloadedData);

		unloadedData = {};
	}

	void setControllerPopupNumbers(BigInteger controllerNumberToShow)
	{
		controllerNumbersInPopup = controllerNumberToShow;
	}

	bool shouldAddControllerToPopup(int controllerValue) const
	{
		if (controllerNumbersInPopup.isZero())
			return true;

		return controllerNumbersInPopup[controllerValue];
	}

	bool isMappable(int controllerValue) const
	{
		if (!exclusiveMode)
			return shouldAddControllerToPopup(controllerValue);

		if (isPositiveAndBelow(controllerValue, 128))
			return automationData[controllerValue].isEmpty();

		return false;
	}

	void setExclusiveMode(bool shouldBeExclusive)
	{
		exclusiveMode = shouldBeExclusive;
	}

	void setConsumeAutomatedControllers(bool shouldConsume)
	{
		consumeEvents = shouldConsume;
	}

	void setControllerPopupNames(const StringArray& newControllerNames)
	{
		controllerNames = newControllerNames;
	}

	String getControllerName(int controllerIndex)
	{
		if (isPositiveAndBelow(controllerIndex, controllerNames.size()))
		{
			return controllerNames[controllerIndex];
		}
		else
		{
			String s;
			s << "CC#" << controllerIndex;
			return s;
		}
	}

	void setCCName(const String& newCCName) { ccName = newCCName; }

	String getCCName() const { return ccName; }

private:

	bool exclusiveMode = false;
	bool consumeEvents = true;

	StringArray controllerNames;
	String ccName;

	BigInteger controllerNumbersInPopup;

	ValueTree unloadedData;

	// ========================================================================================================

	MainController *mc;
	

	CriticalSection lock;

	

	MPEData mpeData;

	bool anyUsed;
	MidiBuffer tempBuffer;

	Array<AutomationData> automationData[128];
	AutomationData unlearnedData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiControllerAutomationHandler)

	// ========================================================================================================
};


class BufferPreviewListener
{
public:

	virtual ~BufferPreviewListener() {}

	virtual void previewStateChanged(bool isPlaying, const AudioSampleBuffer& currentBuffer) = 0;

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(BufferPreviewListener);
};


class OverlayMessageBroadcaster
{
public:

	enum State
	{
		AppDataDirectoryNotFound,
		LicenseNotFound,
		ProductNotMatching,
		UserNameNotMatching,
		EmailNotMatching,
		MachineNumbersNotMatching,
		LicenseExpired,
		LicenseInvalid,
		CriticalCustomErrorMessage,
		SamplesNotInstalled,
		SamplesNotFound,
		IllegalBufferSize,
		CustomErrorMessage,
		CustomInformation,
		numReasons
	};

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

	virtual ~OverlayMessageBroadcaster() {};

	void addOverlayListener(Listener *listener)
	{
		listeners.addIfNotAlreadyThere(listener);
	}

	void removeOverlayListener(Listener* listener)
	{
		listeners.removeAllInstancesOf(listener);
	}

	void sendOverlayMessage(int newState, const String& newCustomMessage=String());

	String getOverlayTextMessage(State s) const;

	bool isUsingDefaultOverlay() const { return useDefaultOverlay; }

	void setUseDefaultOverlay(bool shouldUseOverlay)
	{
		useDefaultOverlay = shouldUseOverlay;
	}

private:

	bool useDefaultOverlay = !HISE_DEACTIVATE_OVERLAY;

	struct InternalAsyncUpdater: public AsyncUpdater
	{
		InternalAsyncUpdater(OverlayMessageBroadcaster *parent_): parent(parent_) {}

		void handleAsyncUpdate() override
		{
			ScopedLock sl(parent->listeners.getLock());

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

	Array<WeakReference<Listener>, CriticalSection> listeners;
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

struct ScopedSoftBypassDisabler: public ControlledObject
{
	ScopedSoftBypassDisabler(MainController* mc);

	~ScopedSoftBypassDisabler();

	bool previousState;
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

	bool illegalBufferSize = false;

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

	float leftOverData[HISE_NUM_PLUGIN_CHANNELS * HISE_EVENT_RASTER];
	float* leftOverChannels[HISE_NUM_PLUGIN_CHANNELS];
	int numLeftOvers = 0;
};




} // namespace hise

#endif  // MAINCONTROLLERHELPERS_H_INCLUDED
