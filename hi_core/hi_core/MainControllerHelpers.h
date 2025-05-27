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

struct ValueToTextConverter
{
	struct ConverterFunctions
	{
		static String Frequency(double input)
		{
			if (input < 30.0f)
				return String(input, 1) + " Hz";
			else if (input < 1000.0f)
				return String(roundToInt(input)) + " Hz";
			else
				return String(input / 1000.0, 1) + " kHz";
		}

		static String Time(double v)
		{
			if(v > 1000.0)
				return String(v * 0.001, 1) + "s";
			else
				return String(roundToInt(v)) + "ms";
		}

		static String TempoSync(double v)
		{
			return TempoSyncer::getTempoName(roundToInt(v));
		}

		static String Decibel(double v)
		{
			return Decibels::toString(v, std::abs(v < 18 ? 1 : 0), -120.0);
		}

		static String Pan(double v)
		{		
			if (v == 0)
				return "C";
			
			String result = String(roundToInt(std::abs(v)));
						
			if (v > 0)
				result += "R";
			else if (v < 0)
				result += "L";

			return result;
		}

		static String NormalizedPercentage(double v)
		{
			return String(roundToInt(v * 100.0)) + "%";
		}
	};

	struct InverterFunctions
	{
		static double Frequency(const String& input)
		{
			if(input.containsChar('k'))
			{
				return input.getDoubleValue() * 1000.0;
			}
			else
			{
				return input.getDoubleValue();
			}
		}

		static double Time(const String& input)
		{
			if(input.containsChar('s') && !input.containsChar('m'))
				return input.getDoubleValue() * 1000.0;
			else
				return input.getDoubleValue();
		}

		static double Decibel(const String& v)
		{
			if(v == "-INF")
				return -100.0;

			return v.getDoubleValue();
		}

		static double TempoSync(const String& input)
		{
			return (double)TempoSyncer::getTempoIndex(input);
		}

		static double Pan(const String& input)
		{
			return input.getDoubleValue() * 0.01;
		}

		static double NormalizedPercentage(const String& input)
		{
			return input.getDoubleValue() * 0.01;
		}
	};

	String getTextForValue(double v) const
	{
		if(!active)
			return String(v, 0);

		if(!itemList.isEmpty())
		{
			auto idx = jlimit<int>(0, itemList.size(), roundToInt(v));
			return itemList[idx];
		}

		if(valueToTextFunction)
			return valueToTextFunction(v);

		auto numDecimalPlaces = jlimit(0, 4, roundToInt(log10(stepSize) * -1.0));
		String valueString(v, numDecimalPlaces);
		valueString << suffix;
		return valueString;
	}

	String operator()(double v) const
	{
		return getTextForValue(v);
		
	}

	double getValueForText(const String& v) const
	{
		if(!active)
			return v.getDoubleValue();

		if(!itemList.isEmpty())
			return (double)itemList.indexOf(v);

		if(textToValueFunction)
			return textToValueFunction(v);

		return v.getDoubleValue();
	}

	double operator()(const String& v) const
	{
		return getValueForText(v);
	}

	static ValueToTextConverter createForOptions(const StringArray& options)
	{
		ValueToTextConverter vtc;
		vtc.active = true;
		vtc.itemList = options;
		return vtc;
	}

	static ValueToTextConverter createForMode(const String& modeString)
	{
		ValueToTextConverter vtc;
		
#define FUNC(x) if(modeString == #x) { vtc.active = true; vtc.valueToTextFunction = ConverterFunctions::x; vtc.textToValueFunction = InverterFunctions::x; }

		FUNC(Frequency);
		FUNC(Time);
		FUNC(TempoSync);
		FUNC(Pan);
		FUNC(NormalizedPercentage);
		FUNC(Decibel);

#undef FUNC

		return vtc;
	}

	static ValueToTextConverter fromString(const String& converterString)
	{
		ValueToTextConverter vtc;

		if(converterString.isNotEmpty())
		{
			zstd::ZDefaultCompressor comp;

			MemoryBlock mb;
			mb.fromBase64Encoding(converterString);
			ValueTree v;

			comp.expand(mb, v);

			vtc.active = (bool)v["active"];

			vtc.itemList = StringArray::fromLines(v["items"].toString().trim());
			vtc.itemList.removeEmptyStrings();

#define FUNC(x) if(v.getProperty("function", "").toString() == #x) { vtc.valueToTextFunction = ConverterFunctions::x; vtc.textToValueFunction = InverterFunctions::x; }
			FUNC(Frequency);
			FUNC(Time);
			FUNC(TempoSync);
			FUNC(Pan);
			FUNC(NormalizedPercentage);
#undef FUNC
		}
		
		return vtc;
	}

	String toString() const
	{
		ValueTree v("ValueConverter");

		if(!itemList.isEmpty())
			v.setProperty("items", itemList.joinIntoString("\n"), nullptr);

		v.setProperty("active", active, nullptr);

		if(suffix.isNotEmpty())
			v.setProperty("suffix", suffix, nullptr);
		
#define FUNC(x) if(valueToTextFunction == ConverterFunctions::x) v.setProperty("function", #x, nullptr);

		FUNC(Frequency);
		FUNC(Time);
		FUNC(TempoSync);
		FUNC(Pan);
		FUNC(NormalizedPercentage);

#undef FUNC

		MemoryBlock mb;
		zstd::ZDefaultCompressor comp;
		comp.compress(v, mb);
		return mb.toBase64Encoding();
	};

	typedef String(*CF)(double);
	typedef double(*ICF)(const String&);

	bool active = false;
	CF valueToTextFunction = nullptr;
	ICF textToValueFunction = nullptr;
	StringArray itemList;
	double stepSize = 0.01;
	String suffix;
};

class RuntimeTargetHolder
{
public:

	virtual ~RuntimeTargetHolder() {};
	virtual void connectRuntimeTargets(MainController* mc) = 0;
	virtual void disconnectRuntimeTargets(MainController* mc) = 0;
};

/** This handles the MIDI automation for the frontend plugin.
*
*	For faster performance, one CC value can only control one parameter.
*
*/
class MidiControllerAutomationHandler : public UserPresetStateManager,
										public SafeChangeBroadcaster
{
public:

	// Used to identify the automation data
	struct Key
	{
		static constexpr int16 MaxCCNumber = 128;
		static constexpr int8 MaxChannel = 16;
		
		Key(): channel(-2), ccNumber(0) {};

		Key(int channel_, int ccNumber_):
		  channel((int8)channel_),
		  ccNumber((int16)ccNumber_)
		{
			jassert(channel == -1 || channel < MaxChannel);
			jassert(isPositiveAndBelow(ccNumber, (int)MaxCCNumber));
		};

		Key(const MidiMessage& m, bool useOmni):
		  channel(useOmni ? -1 : m.getChannel()-1),
		  ccNumber(m.getControllerNumber())
		{
			
		}

		Key asOmni() const
		{
			jassert(!isOmni());
			Key k(*this);
			k.channel = -1;
			return k;
		}

		Key(const HiseEvent& e, bool useOmni):
		  channel(useOmni ? -1 : e.getChannel()-1),
		  ccNumber(e.getControllerNumber())
		{}

		bool isValid() const { return isOmni() || (isPositiveAndBelow(channel, MaxChannel) && isPositiveAndBelow(ccNumber, MaxCCNumber)); }
		bool isOmni() const { return channel == -1; }
		bool isInvalid() const { return channel == -2; }

		int getMidiChannelBasedOne() const
		{
			if(isOmni()) return -1;

			return channel + 1;
		}

		bool matchesOtherKey(const Key& other) const
		{
			auto channelMatches = other.channel == channel || other.isOmni() || isOmni();
			auto ccMatches = other.ccNumber == ccNumber;
			return channelMatches && ccMatches;
		}

		int8 channel = -2;
		int16 ccNumber = 0;

		
	};
    
    template <typename T> struct Container
    {
        struct Iterator
        {
            Iterator(const Container& c_, bool justOmni_):
              c(c_),
              justOmni(justOmni_),
              k(-1, 0)
            {}

            bool next(T* t=nullptr)
            {
                auto& l = c[k];

                if(isPositiveAndBelow(indexInVector, l.size()))
                {
                    if(t != nullptr)
                        *t = l[indexInVector];

                    indexInVector++;
                    return true;
                }

                indexInVector = 0;

                if(!bumpKey())
                    return false;

                while(c[k].empty())
                {
                    if(!bumpKey())
                        return false;
                }

                jassert(!c[k].empty());

                if(t != nullptr)
                    *t = c[k][indexInVector];

                indexInVector++;

                return true;
            }
                    

            Key getCurrentKey() const { return k; }
            int getPositionInVector() const { return indexInVector-1; }

            void eraseCurrentElement()
            {
                jassert(indexInVector != 0);

                auto& l = const_cast<Container*>(&c)->operator[](k);
                indexInVector--;
                l.erase(l.begin() + indexInVector);
            }

            int getNumItems() const
            {
                Iterator copy(*this);
                copy.reset();

                int numActive = 0;

                while(copy.next())
                    numActive++;

                return numActive;
            }

            int getIndexForKey(const Key& k) const
            {
                Iterator copy(*this);
                copy.reset();

                int currentIndex = 0;

                while(copy.next())
                {
                    if(copy.getCurrentKey().matchesOtherKey(k))
                        return currentIndex;

                    currentIndex++;
                }

                return -1;
            }

            bool perform(int indexInFlatList, const std::function<bool(T&)>& f) const
            {
                auto copy = Iterator(*this);
                copy.reset();

                int currentIndex = 0;

                while(copy.next())
                {
                    if(currentIndex++ == indexInFlatList)
                    {
                        auto k = copy.getCurrentKey();
                        auto pos = copy.getPositionInVector();
                        auto& l = const_cast<Container*>(&c)->operator[](k);
                        return f(l[pos]);
                    }
                }

                return false;
            }

            T getDataFromIndex(int index) const
            {
                auto copy = Iterator(*this);
                copy.reset();

                int currentIndex = 0;

                while(copy.next())
                {
                    if (index == currentIndex++)
                    {
                        return c[copy.getCurrentKey()][copy.getPositionInVector()];
                    }
                }

                return {};
            }

            int removeMatches(const Key& key) const
            {
                int numRemoved = 0;
                auto copy = Iterator(*this);
                copy.reset();

                while(copy.next())
                {
                    if(copy.getCurrentKey().matchesOtherKey(key))
                    {
                        copy.eraseCurrentElement();
                        numRemoved++;
                    }
                }

                return numRemoved;
            }

        private:

            void reset()
            {
                k = { -1, 0 };
                indexInVector = 0;
            }

            bool bumpKey()
            {
                if (justOmni)
                {
                    return ++k.ccNumber < Key::MaxCCNumber;
                }
                else
                {
                    if(++k.ccNumber == Key::MaxCCNumber)
                    {
                        k.ccNumber = 0;
                        k.channel++;
                        return k.channel < (Key::MaxChannel);
                    }

                    return true;
                }
            }

            const bool justOmni;
            const Container& c;
            Key k;
            int indexInVector = 0;
        };

        

        std::vector<T>& operator[](const Key& k)
        {
            if(!k.isValid())
            {
                jassertfalse;
                static std::vector<T> empty;
                return empty;
            }

            if(k.isOmni())
                return data[Key::MaxChannel][k.ccNumber];

            return data[k.channel][k.ccNumber];
        }

        Iterator createIterator(bool justOmni) const { return Iterator(*this, justOmni); }

        void clear()
        {
            for(auto& ch: data)
            {
                for(auto& cc: ch)
                    cc.clear();
            }
        }

        const std::vector<T>& operator[](const Key& k) const
        {
            return const_cast<Container*>(this)->operator[](k);
        }

        const std::array<std::vector<T>, Key::MaxCCNumber>& getChannelData(int8 channelIndex) const
        {
            if(isPositiveAndBelow(channelIndex, Key::MaxChannel))
                return data[channelIndex];
            else
                return data[Key::MaxChannel];
        }

    private:

        std::array<std::array<std::vector<T>, Key::MaxCCNumber>, Key::MaxChannel + 1> data;
    };

	MidiControllerAutomationHandler(MainController *mc_);

	void addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, const ValueToTextConverter& converter, int macroIndex);
	void removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NotificationType notifyListeners);

	bool isLearningActive() const;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	Identifier getUserPresetStateId() const override;
	
	void resetUserPresetState() override;

	bool isLearningActive(Processor *interfaceProcessor, int attributeIndex) const;
	void deactivateMidiLearning();

	void setUnlearndedMidiControlNumber(Key k, NotificationType notifyListeners);
	Key getMidiControllerNumber(Processor *interfaceProcessor, int attributeIndex) const;

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
		Key k;
		bool inverted = false;
		bool used;
		ValueToTextConverter textConverter;
	};

	/** Returns a copy of the automation data for the given index. */
	AutomationData getDataFromIndex(int index) const;

	MPEData& getMPEData();

	const MPEData& getMPEData() const;

	Container<AutomationData>::Iterator createIterator() const;

	int getIndexForKey(Key key) const;

	int getNumActiveConnections() const;
	bool setNewRangeForParameter(int index, NormalisableRange<double> range);
	bool setParameterInverted(int index, bool value);

	void setUnloadedData(const ValueTree& v);;

	void loadUnloadedData();

	void setControllerPopupNumbers(BigInteger controllerNumberToShow);

	bool hasSelectedControllerPopupNumbers() const;

	bool shouldAddControllerToPopup(int controllerNumber) const;

	bool isMappable(int controllerNumber) const;

	void setExclusiveMode(bool shouldBeExclusive);

	void setConsumeAutomatedControllers(bool shouldConsume);

	void setControllerPopupNames(const StringArray& newControllerNames);

	String getControllerName(int controllerIndex);

	void setCCName(const String& newCCName);

	String getCCName() const;

private:

	bool filterChannels = false;

	bool exclusiveMode = false;
	bool consumeEvents = true;

	StringArray controllerNames;
	String ccName;

	BigInteger controllerNumbersInPopup;

	mutable SimpleReadWriteLock unloadLock;

	ValueTree unloadedData;

	// ========================================================================================================

	MainController *mc;

	CriticalSection lock;

	MPEData mpeData;

	bool anyUsed;
	MidiBuffer tempBuffer;

	Container<AutomationData> automationData;

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

	static constexpr int NumThrowAwayBuffers = 128;

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
