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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef DEBUGLOGGER_H_INCLUDED
#define DEBUGLOGGER_H_INCLUDED

class MainController;
class JavascriptProcessor;

class DebugLogger : public Timer
{
public:

	enum class MessageType
	{
		Failure=0,
		Warning,
		Event,
		ParameterChange,
		numMessageTypes
	};

	enum class FailureType
	{
		Empty,
		SampleRateChange, //< A change in the sample rate
		Assertion, //< a failed assertion
		BufferSizeChange, //< a change in the buffer size
		PerformanceWarning, //< if the audio thread spends too much time in this routine it will produce this
		BurstLeft, //< an audio buffer full of junk (values above +36dB)
		BurstRight,
		ClickLeft, //< a single sample above +36dB
		ClickRight,
		AudioThreadWasLocked, //< signals a priority inversion
		Discontinuity, //< signals a discontinuity in the signal stream
		PriorityInversion, //< when the audio thread lock is locked by another thread
		SampleLoadingError,
		StreamingFailure,
		numFailureTypes
	};

	enum class Location
	{
		Empty,
		MainRenderCallback,
		SynthChainRendering,
		SynthPreVoiceRendering,
		SynthPostVoiceRenderingGainMod,
		SynthPostVoiceRendering,
		SynthRendering,
		TimerCallback,
		SynthVoiceRendering,
		MultiMicSampleRendering,
		SampleRendering,
		SampleVoiceBufferFill,
		SampleLoaderReadOperation,
		MasterEffectRendering,
		ConvolutionRendering,
		VoiceEffectRendering,
		ScriptFXRendering,
		ScriptFXRenderingPost,
		ModulatorChainVoiceRendering,
		ModulatorChainTimeVariantRendering,
		DspInstanceRendering,
		DspInstanceRenderingPost,
		NoteOnCallback,
		NoteOffCallback,
		ScriptMidiEventCallback,
		SampleStart,
		numLocations
	};

	struct PerformanceData
	{
		PerformanceData(int location_, float thisPercentage_, float averagePercentage_, Processor* p_) :
			location(location_),
			thisPercentage(thisPercentage_),
			averagePercentage(averagePercentage_),
			p(p_)
		{};

		PerformanceData() :
			location(0),
			thisPercentage(0.0f),
			averagePercentage(0.0f),
			p(nullptr)
		{};

		int location;
		float thisPercentage;
		float averagePercentage;
		float limit;
		Processor* p;
	};

	struct PriorityInversionChecker : public AudioProcessor::PreCallbackHandler
	{
	public:

		PriorityInversionChecker(AudioProcessor* p_) :
			p(p_)
		{};

		void preCallbackEvent() override;

	private:

		AudioProcessor* p;
	};

	DebugLogger(MainController* mc);

	~DebugLogger();

	struct Message;

	struct Failure;

	struct Event;

	struct PerformanceWarning;

	struct ParameterChange;

	struct StringMessage;

	struct AudioSettingChange;

	struct Listener
	{
		virtual ~Listener() { masterReference.clear(); };

		virtual void logStarted() = 0;
		virtual void logEnded() = 0;
		virtual void errorDetected() = 0;

	private:

		friend class WeakReference<Listener>;
		WeakReference<Listener>::Master masterReference;
	};

	double getCurrentTimeStamp() const;

	void addFailure(const Failure& f);

	void addPerformanceWarning(const PerformanceWarning& f);

	void addStreamingFailure(double voiceUptime);

	void logEvents(const HiseEventBuffer& masterBuffer);

	void logMessage(const String& errorMessage);

	void logPerformanceWarning(const PerformanceData& logData);

	void logParameterChange(JavascriptProcessor* p, ReferenceCountedObject* control, const var& newValue);

	void checkAudioCallbackProperties(double sampleRate, int samplesPerBlock);

	void checkSampleData(Processor* p, Location location, bool isLeftChannel, const float* data, int numSamples, const Identifier& id=Identifier());

	void checkAssertion(Processor* p, Location location, bool result, double extraData);


	void checkPriorityInversion(const CriticalSection& lockToCheck);

	void checkPriorityInversion(const SpinLock& spinLockToCheck, Location l, Processor* p, const Identifier& id);

	void timerCallback() override;

	void startLogging();
	bool isLogging() const;
	void stopLogging();

	void toggleLogging();

	bool isCurrentlyFailing() const;

	void addListener(Listener* newListener);
	void removeListener(Listener* listenerToRemove);

	String getLastErrorMessage() const;

	static void showLogFolder();

	static String getNameForLocation(Location l);
	static String getNameForFailure(FailureType f);

	static void fillBufferWithJunk(float* data, int numSamples);

	void setStackBacktrace(const String& newBackTrace) const
	{
		jassert(MessageManager::getInstance()->isThisTheMessageThread());

		messageCallbackStackBacktrace = newBackTrace;
	}

	void setPerformanceWarningLevel(int newWarningLevel);
	
	File getCurrentLogFile() const
	{
		return currentLogFile;
	}

	double getScaleFactorForWarningLevel() const
	{
		switch (warningLevel)
		{
		case 0: return 3.0;
		case 1: return 2.0;
		case 2: return 1.0;
		}

		return 1.0;
	}

	void addSorted(Array<Message*>& list, Message* m);

private:

	mutable String messageCallbackStackBacktrace;

	String actualBackTrace;

	String lastErrorMessage = "";

	int numErrorsSinceLogStart = 0;
	int callbackIndex = 0;
	int messageIndex = 0;

	void addAudioDeviceChange(FailureType changeType, double oldValue, double newValue);

	double lastSampleRate = -1.0;
	int lastSamplesPerBlock = -1;

	MainController* mc;

	Location locationForErrorInCurrentCallback = Location::Empty;

	static File getLogFile();
	static File getLogFolder();
	static String getHeader();
	String getSystemSpecs() const;

#define NUM_MESSAGE_SLOTS 256

	Array<Failure> pendingFailures;
	Array<StringMessage> pendingStringMessages;
	Array<ParameterChange> pendingParameterChanges;
	Array<PerformanceWarning> pendingPerformanceWarnings;
	Array<AudioSettingChange> pendingAudioChanges;
	Array<Event> pendingEvents;
	
	Array<WeakReference<Listener>> listeners;

	CriticalSection debugLock;
	CriticalSection messageLock;

	File currentLogFile;
	bool currentlyLogging = false;
	bool currentlyFailing = false;

	double uptime = 0.0;

	int warningLevel = 2;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugLogger)
};

// Use these macros instead of calling the functions directly:

// Checks the buffer and logs bursts and discontinuities
#define CHECK_AND_LOG_BUFFER_DATA(processor, location, data, isLeft, numSamples) processor->getMainController()->getDebugLogger().checkSampleData(processor, location, isLeft, data, numSamples);

#define CHECK_AND_LOG_BUFFER_DATA_WITH_ID(processor, id, location, data, isLeft, numSamples) processor->getMainController()->getDebugLogger().checkSampleData(processor, location, isLeft, data, numSamples, id);

#define CHECK_AND_LOG_ASSERTION(processor, location, result, extraData) processor->getMainController()->getDebugLogger().checkAssertion(processor, location, result, (double)extraData);


class DebugLoggerComponent : public Component,
							 public DebugLogger::Listener,
							 public Button::Listener,
							 public ComboBox::Listener,
							 public Timer
{
public:

	DebugLoggerComponent(DebugLogger* logger_):
		logger(logger_)
	{
		logger->addListener(this);
		addAndMakeVisible(showLogFolderButton = new TextButton("Open log folder"));
		addAndMakeVisible(closeAndShowFileButton = new TextButton("Stop & show file"));
		addAndMakeVisible(performanceLevelSelector = new ComboBox("Warning Level"));

		
		performanceLevelSelector->addItem("Low", 1);
		performanceLevelSelector->addItem("Mid", 2);
		performanceLevelSelector->addItem("High", 3);
		performanceLevelSelector->setSelectedItemIndex(2, dontSendNotification);

		performanceLevelSelector->addListener(this);
		performanceLevelSelector->setLookAndFeel(&plaf);

		showLogFolderButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
		showLogFolderButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
		showLogFolderButton->setLookAndFeel(&blaf);
		showLogFolderButton->addListener(this);

		closeAndShowFileButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
		closeAndShowFileButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
		closeAndShowFileButton->setLookAndFeel(&blaf);
		closeAndShowFileButton->addListener(this);

		startTimer(30);
	}

	~DebugLoggerComponent()
	{
		logger->removeListener(this);
	}

	void logStarted() override
	{
		setVisible(true);
	}

	void logEnded() override
	{
		setVisible(false);
	}

	void errorDetected() override
	{
		isFailing = true;
		repaint();

		startTimer(100);
	}

	void timerCallback() override
	{
		if (!logger->isCurrentlyFailing())
		{
			isFailing = false;
			stopTimer();
			repaint();
		}

	}

	void buttonClicked(Button* b) override
	{
		if (b == showLogFolderButton)
		{
			logger->showLogFolder();
		}
		else
		{
			File f = logger->getCurrentLogFile();
			logger->stopLogging();

			f.revealToUser();
		}
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		logger->setPerformanceWarningLevel(comboBoxThatHasChanged->getSelectedItemIndex());
	}

	void paint(Graphics& g) override;

	void resized() override
	{
		showLogFolderButton->setBounds(getWidth() - 120, 5, 100, 20);
		closeAndShowFileButton->setBounds(getWidth() - 120, 35, 100, 20);
		performanceLevelSelector->setBounds(getWidth() - 280, 25, 140, 30);
	}

private:

	BlackTextButtonLookAndFeel blaf;
	PopupLookAndFeel plaf;


	DebugLogger* logger;

	bool isFailing = false;

	ScopedPointer<TextButton> showLogFolderButton;
	ScopedPointer<TextButton> closeAndShowFileButton;
	ScopedPointer<ComboBox> performanceLevelSelector;
};

#endif  // DEBUGLOGGER_H_INCLUDED
