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
class MidiControllerAutomationHandler : public UserPresetStateManager,
										public SafeChangeBroadcaster
{
public:

	MidiControllerAutomationHandler(MainController *mc_);

	void addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, int macroIndex);
	void removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NotificationType notifyListeners);

	

	bool isLearningActive() const;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	Identifier getUserPresetStateId() const override;;
	void resetUserPresetState() override;

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
					public UserPresetStateManager,
					public Dispatchable
	{
	public:
		MPEData(MainController* mc);;

		~MPEData();

		struct Listener
		{
		public:
			virtual ~Listener();;

			virtual void mpeModeChanged(bool isEnabled) = 0;

			virtual void mpeModulatorAssigned(MPEModulator* m, bool wasAssigned) = 0;

			virtual void mpeDataReloaded() = 0;

			virtual void mpeModulatorAmountChanged();;

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
        
		Identifier getUserPresetStateId() const override;

		void resetUserPresetState() override;

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

		bool isMpeEnabled() const;

		bool contains(MPEModulator* mod) const;

		void addListener(Listener* l);

		void removeListener(Listener* l);

		void sendAmountChangeMessage();

	private:

		struct AsyncRestorer : private Timer
		{
		public:

			AsyncRestorer(MPEData& parent_);;

			void restore(const ValueTree& v);

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

		~AutomationData();

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

	MPEData& getMPEData();

	const MPEData& getMPEData() const;

	int getNumActiveConnections() const;
	bool setNewRangeForParameter(int index, NormalisableRange<double> range);
	bool setParameterInverted(int index, bool value);

	void setUnloadedData(const ValueTree& v);;

	void loadUnloadedData();

	void setControllerPopupNumbers(BigInteger controllerNumberToShow);

	bool hasSelectedControllerPopupNumbers() const;

	bool shouldAddControllerToPopup(int controllerValue) const;

	bool isMappable(int controllerValue) const;

	void setExclusiveMode(bool shouldBeExclusive);

	void setConsumeAutomatedControllers(bool shouldConsume);

	void setControllerPopupNames(const StringArray& newControllerNames);

	String getControllerName(int controllerIndex);

	void setCCName(const String& newCCName);

	String getCCName() const;

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

		virtual ~Listener();

	private:

		friend class WeakReference<Listener>;

		WeakReference<Listener>::Master masterReference;
	};

	OverlayMessageBroadcaster();

	virtual ~OverlayMessageBroadcaster();;

	void addOverlayListener(Listener *listener);

	void removeOverlayListener(Listener* listener);

	void sendOverlayMessage(int newState, const String& newCustomMessage=String());

	String getOverlayTextMessage(State s) const;

	bool isUsingDefaultOverlay() const;

	void setUseDefaultOverlay(bool shouldUseOverlay);

private:

	bool useDefaultOverlay = !HISE_DEACTIVATE_OVERLAY;

	struct InternalAsyncUpdater: public AsyncUpdater
	{
		InternalAsyncUpdater(OverlayMessageBroadcaster *parent_);

		void handleAsyncUpdate() override;

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

	HiseEventBuffer shortBuffer;
	int lastBlockSizeForShortBuffer = 0;

	int fullBlockSize;

	int sampleIndexInternal = 0;
	int sampleIndexExternal = 0;

	float leftOverData[HISE_NUM_PLUGIN_CHANNELS * HISE_EVENT_RASTER];
	float* leftOverChannels[HISE_NUM_PLUGIN_CHANNELS];
	int numLeftOvers = 0;
};

class AudioRendererBase: public Thread,
						 public ControlledObject
{
public:

	AudioRendererBase(MainController* mc);;
	~AudioRendererBase() override;;

protected:

	virtual void callUpdateCallback(bool isFinished, double progress) = 0;

	/** Call this after creating the Event buffer content and it will prepare all internal buffers. */
	void initAfterFillingEventBuffer();

	Array<VariantBuffer::Ptr> channels;
	OwnedArray<HiseEventBuffer> eventBuffers;
	
    bool skipCallbacks = true;
	bool sendArtificialTransportMessages = false;

private:

	static constexpr int NumThrowAwayBuffers = 12;

	int thisNumThrowAway = 0;

	void cleanup();
	void run() override;
	bool renderAudio();
	AudioSampleBuffer getChunk(int startSample, int numSamples);

	int numSamplesToRender = 0;
	int numChannelsToRender = 0;
	int numActualSamples = 0;
	float* splitData[NUM_MAX_CHANNELS];
	int bufferSize = 0;
};


} // namespace hise

#endif  // MAINCONTROLLERHELPERS_H_INCLUDED
