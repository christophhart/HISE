

DebugLogger::DebugLogger(MainController* mc_):
	mc(mc_)
{

}

DebugLogger::~DebugLogger()
{

}

double DebugLogger::getCurrentTimeStamp() const
{
	return 0.001 * (Time::getMillisecondCounterHiRes() - uptime);
}

void DebugLogger::addFailure(const DebugLogger::Failure& f)
{
	ScopedLock sl(debugLock);
	pendingFailures.add(f);
}

void DebugLogger::logMessage(const String& errorMessage)
{
	if (isLogging())
	{
		ScopedLock sl(messageLock);

		messages.add(errorMessage);
		callbackIndexesForMessage.add(callbackIndex);
	}
}

struct DebugLogger::Failure
{
	Failure() :
		location(Location::Empty),
		type(FailureType::Empty),
		timestamp(0.0)
	{};

	Failure(int callbackIndex_, Location loc_, FailureType t, Processor* faultyModule, double ts, double extraValue_, const Identifier& id_=Identifier()) :
		location(loc_),
		type(t),
		timestamp(ts),
		p(faultyModule),
		extraValue(extraValue_),
		callbackIndex(callbackIndex_),
		id(id_)
	{};

	Failure(int callbackIndex_, FailureType f, double old, double newvalue, double ts) :
		location(Location::Empty),
		type(f),
		oldValue(old),
		newValue(newvalue),
		timestamp(ts),
		callbackIndex(callbackIndex_)
		{};

	Failure(const PerformanceData& d, double timestamp_, int callbackIndex_, int voiceAmount) :
		location((Location)d.location),
		type(FailureType::PerformanceWarning),
		oldValue(d.averagePercentage),
		newValue(d.thisPercentage),
		extraValue(voiceAmount),
		timestamp(timestamp_),
		limit(d.limit),
		p(d.p),
		callbackIndex(callbackIndex_),
		id(Identifier())
	{};

		

	String getErrorString(int errorIndex=-1)
	{
		static const String ok("All OK");

		if (type == FailureType::Empty)
			return ok;

		NewLine nl;
		String errorMessage;

		if (errorIndex == -1)
		{
			errorMessage << "### " << DebugLogger::getNameForFailure(type) << nl;
		}
		else
		{
			errorMessage << "### #" << String(errorIndex) << ": " << DebugLogger::getNameForFailure(type) << nl;
		}

		

		errorMessage << "- Time: **" << String(timestamp, 2) << "**  " << " / ";
		errorMessage << "CallbackIndex: **" << String(callbackIndex) << "**  " << nl;

		if (type == FailureType::SampleRateChange || type == FailureType::BufferSizeChange)
		{
			errorMessage << "- Old: **" << String(oldValue, 0) << "**  " << nl;
			errorMessage << "- New: **" << String(newValue, 0) << "**  " << nl << nl;
			return errorMessage;
		}

		errorMessage << "- Location: `";

		if (p.get() != nullptr)
		{
			errorMessage << p.get()->getId() << "::";
		}

		if (!id.isNull())
		{
			errorMessage << id.toString() << "::";
		}

		errorMessage << DebugLogger::getNameForLocation(location) << "`  " << nl;

		if (type == FailureType::PerformanceWarning)
		{
			errorMessage << "- Voice Amount: **" << String(extraValue, 0) << "**  " << nl;
			errorMessage << "- Limit: `" << String(100.0 * limit , 1) << "%` Avg: `" << String(oldValue, 2) << "%`, Peak: `" << String(newValue, 1) << "%`  " << nl;
		}
		else if (extraValue != 0.0)
		{
			errorMessage << "- AdditionalInfo: **" << String(extraValue, 3) << "**  " << nl;
		}

		
		errorMessage << nl;
		return errorMessage;
	}

	Location location;
	FailureType type;
	double timestamp = 0.0;

	double oldValue = 0.0;
	double newValue = 0.0;
	double limit = 0.0;

	WeakReference<Processor> p;

	double extraValue = 0.0;
	int callbackIndex;

	Identifier id;
};

void DebugLogger::logPerformanceWarning(const PerformanceData& logData)
{
	if (!isLogging())
		return;

	const int voiceAmount = logData.p->getMainController()->getNumActiveVoices();

	Failure f(logData, getCurrentTimeStamp(), callbackIndex, voiceAmount);
	addFailure(f);
}

void DebugLogger::checkAudioCallbackProperties(double sampleRate, int samplesPerBlock)
{
	if (!isLogging()) return;

	callbackIndex++;

	locationForErrorInCurrentCallback = Location::Empty;

	if (sampleRate != lastSampleRate)
	{
		addAudioDeviceChange(FailureType::SampleRateChange, lastSampleRate, sampleRate);
		lastSampleRate = sampleRate;
	}

	if (samplesPerBlock != lastSamplesPerBlock)
	{
		addAudioDeviceChange(FailureType::BufferSizeChange, lastSamplesPerBlock, samplesPerBlock);
		lastSamplesPerBlock = samplesPerBlock;
	}
}

void DebugLogger::checkSampleData(Processor* p, Location location, bool isLeft, const float* data, int numSamples, const Identifier& id)
{
	if (!isLogging()) return;

	if (location <= locationForErrorInCurrentCallback) // poor man's stack trace
		return;

	auto range = FloatVectorOperations::findMinAndMax(data, numSamples);

	const float maxValue = 32.0f;

	double errorValue = 0.0;
	bool isError = false;
	int numFaultySamples = 0;

	FailureType failureType = FailureType::Empty;

	if (range.getEnd() > maxValue)
	{
		isError = true;

		errorValue = range.getEnd();

		for (int i = 0; i < numSamples; i++)
		{
			if (data[i] > maxValue)
				numFaultySamples++;
		}

	}

	if (range.getStart() < -maxValue)
	{
		isError = true;

		errorValue = range.getStart();

		for (int i = 0; i < numSamples; i++)
		{
			if (data[i] < -maxValue)
				numFaultySamples++;
		}
	}

	if (isError)
	{
		locationForErrorInCurrentCallback = location;

		if (numFaultySamples == 1)
		{
			failureType = isLeft ? FailureType::ClickLeft : FailureType::ClickRight;
		}
		else
		{
			failureType = isLeft ? FailureType::BurstLeft : FailureType::BurstRight;
		}

		Failure f(callbackIndex, location, failureType, p, getCurrentTimeStamp(), errorValue, id);
		addFailure(f);
	}
}

void DebugLogger::checkAssertion(Processor* p, Location location, bool result, double extraData)
{
	if (!isLogging()) return;

	if (!result)
	{
		Failure f(callbackIndex, location, FailureType::Assertion, p, getCurrentTimeStamp(), extraData);
		addFailure(f);
	}
}


void DebugLogger::checkPriorityInversion(const CriticalSection& lockToCheck)
{
	if (!isLogging())
		return;

	ScopedTryLock sl(lockToCheck);

	if (!sl.isLocked())
	{
		Failure f(callbackIndex - 1, Location::MainRenderCallback, FailureType::PriorityInversion, nullptr, getCurrentTimeStamp(), 0.0);

		actualBackTrace = messageCallbackStackBacktrace;

		addFailure(f);
	}
}

void DebugLogger::checkPriorityInversion(const SpinLock& spinLockToCheck, Location l, Processor* p, const Identifier& id)
{
	if (!isLogging())
		return;

	bool success = spinLockToCheck.tryEnter();

	if (success)
		spinLockToCheck.exit();
	else
	{
		Failure f(callbackIndex, l, FailureType::PriorityInversion, p, getCurrentTimeStamp(), 0.0, id);
		addFailure(f);
	}
}

void DebugLogger::addAudioDeviceChange(FailureType changeType, double oldValue, double newValue)
{
	if (isLogging())
	{
		Failure f(callbackIndex, changeType, oldValue, newValue, getCurrentTimeStamp());

		{
			ScopedLock sl(debugLock);
			pendingMessages.add(f);
		}
	}
}

void DebugLogger::startLogging()
{
	currentLogFile = getLogFile();
	currentlyLogging = true;
	currentLogFile.create();
	numErrorsSinceLogStart = 0;

	lastSampleRate = -1.0;
	lastSamplesPerBlock = -1;

	callbackIndex = 0;

	uptime = Time::getMillisecondCounterHiRes();

	FileOutputStream fos(currentLogFile);

	fos << getHeader();
	fos << getSystemSpecs();
	

	pendingFailures.ensureStorageAllocated(200);

	startTimer(200);

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->logStarted();
		}
	}
}

void DebugLogger::timerCallback()
{
	if (messages.size() != 0)
	{
		FileOutputStream fos(currentLogFile, 512);
		NewLine nl;

		ScopedLock sl(messageLock);

		for (auto m : messages)
		{
			fos << "#### " << m << "  " << nl;
			fos << "- Callback Index: **" << String(callbackIndex) << "**  " << nl << nl;
		}

		messages.clear();
		callbackIndexesForMessage.clear();
	}


	Array<Failure> a;

	if ( !pendingFailures.isEmpty())
	{
		FileOutputStream fos(currentLogFile, 512);

		a.ensureStorageAllocated(pendingFailures.size());

		{
			ScopedLock sl(debugLock);
			a.addArray(pendingFailures);
			pendingFailures.clearQuick();
		}

		

		for (int i = 0; i < a.size(); i++)
		{
			fos << a[i].getErrorString(numErrorsSinceLogStart);

			if (a[i].type == FailureType::PriorityInversion && actualBackTrace.isNotEmpty())
			{
				NewLine nl;

				fos << "#### Stack Backtrace" << nl << nl;
				fos << "```" << nl;

				fos << actualBackTrace;
				fos << "```" << nl << nl;
				actualBackTrace = String();
			}

			numErrorsSinceLogStart++;
		}
		
		lastErrorMessage = getNameForFailure(a.getLast().type);

		currentlyFailing = true;

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
				listeners[i]->errorDetected();
		}

		return;
	}
	else
	{
		currentlyFailing = false;
	}

	if (!pendingMessages.isEmpty())
	{
		FileOutputStream fos(currentLogFile, 512);

		a.clearQuick();

		a.ensureStorageAllocated(pendingMessages.size());

		{
			ScopedLock sl(debugLock);
			a.addArray(pendingMessages);
			pendingMessages.clearQuick();
		}

		for (int i = 0; i < a.size(); i++)
			fos << a[i].getErrorString();
	}
}

bool DebugLogger::isLogging() const
{
	return currentlyLogging && numErrorsSinceLogStart < 200;
}

void DebugLogger::stopLogging()
{
	currentlyLogging = false;
	stopTimer();

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->logEnded();
		}
	}
}

void DebugLogger::toggleLogging()
{
	if (isLogging()) stopLogging();
	else			 startLogging();
}

bool DebugLogger::isCurrentlyFailing() const
{
	return currentlyFailing;
}

void DebugLogger::addListener(Listener* newListener)
{
	listeners.addIfNotAlreadyThere(newListener);
}

void DebugLogger::removeListener(Listener* listenerToRemove)
{
	listeners.removeAllInstancesOf(listenerToRemove);
}

String DebugLogger::getLastErrorMessage() const
{
	String rs;

	rs << "# Errors: " << String(numErrorsSinceLogStart) << ", Last Error Type: " << lastErrorMessage;

	return rs;
}

void DebugLogger::showLogFolder()
{
	getLogFolder().revealToUser();
}

#define RETURN_CASE_STRING_LOCATION(x) case DebugLogger::Location::x: return #x

String DebugLogger::getNameForLocation(Location l)
{
	switch (l)
	{
		RETURN_CASE_STRING_LOCATION(Empty);
		RETURN_CASE_STRING_LOCATION(MainRenderCallback);
		RETURN_CASE_STRING_LOCATION(MultiMicSampleRendering);
		RETURN_CASE_STRING_LOCATION(SampleRendering);
		RETURN_CASE_STRING_LOCATION(ScriptFXRendering);
		RETURN_CASE_STRING_LOCATION(ScriptFXRenderingPost);
		RETURN_CASE_STRING_LOCATION(DspInstanceRendering);
		RETURN_CASE_STRING_LOCATION(DspInstanceRenderingPost);
		RETURN_CASE_STRING_LOCATION(TimerCallback);
		RETURN_CASE_STRING_LOCATION(SynthRendering);
		RETURN_CASE_STRING_LOCATION(SynthChainRendering);
		RETURN_CASE_STRING_LOCATION(SampleStart);
		RETURN_CASE_STRING_LOCATION(VoiceEffectRendering);
		RETURN_CASE_STRING_LOCATION(ModulatorChainVoiceRendering);
		RETURN_CASE_STRING_LOCATION(ModulatorChainTimeVariantRendering);
		RETURN_CASE_STRING_LOCATION(SynthVoiceRendering);
		RETURN_CASE_STRING_LOCATION(NoteOnCallback);
		RETURN_CASE_STRING_LOCATION(NoteOffCallback);
		RETURN_CASE_STRING_LOCATION(MasterEffectRendering);
		RETURN_CASE_STRING_LOCATION(ScriptMidiEventCallback);
		RETURN_CASE_STRING_LOCATION(ConvolutionRendering);
	}

	return "Unknown Location";

}
	
#undef RETURN_CASE_STRING_LOCATION

#define RETURN_CASE_STRING_FAILURE(x) case DebugLogger::FailureType::x: return #x

String DebugLogger::getNameForFailure(FailureType f)
{
	switch (f)
	{
		RETURN_CASE_STRING_FAILURE(Empty);
		RETURN_CASE_STRING_FAILURE(SampleRateChange);
		RETURN_CASE_STRING_FAILURE(Assertion);
		RETURN_CASE_STRING_FAILURE(BufferSizeChange);
		RETURN_CASE_STRING_FAILURE(PerformanceWarning);
		RETURN_CASE_STRING_FAILURE(BurstLeft);
		RETURN_CASE_STRING_FAILURE(BurstRight);
		RETURN_CASE_STRING_FAILURE(ClickLeft);
		RETURN_CASE_STRING_FAILURE(ClickRight);
		RETURN_CASE_STRING_FAILURE(AudioThreadWasLocked);
		RETURN_CASE_STRING_FAILURE(Discontinuity);
		RETURN_CASE_STRING_FAILURE(PriorityInversion);
		RETURN_CASE_STRING_FAILURE(SampleLoadingError);
	}

	return "Unknown failure";
}

void DebugLogger::fillBufferWithJunk(float* data, int numSamples)
{
	Random r;

	bool shouldFillWithBurst = r.nextFloat() > 0.992f;
	bool shouldFillWithClick = r.nextFloat() > 0.992f;
	bool shouldAddInf = r.nextFloat() > 0.992f;
	bool shouldAddNaN = r.nextFloat() > 0.992f;

	if (shouldFillWithBurst)
	{
		for (int i = 0; i < numSamples; i++)
		{
			data[i] = (2.0f * r.nextFloat() - 1.0f) * FLT_MAX;
		}

		return;
	}

	if (shouldFillWithClick)
	{
		data[0] = (2.0f * r.nextFloat() - 1.0f) * FLT_MAX;

		return;
	}

	if (shouldAddInf)
	{
		data[0] = INFINITY;
		return;
	}

	if (shouldAddNaN)
	{
		data[0] = NAN;
		return;
	}
}

void DebugLogger::setPerformanceWarningLevel(int newWarningLevel)
{
	logMessage("New Warning level selected: " + String(newWarningLevel));

	warningLevel = newWarningLevel;
}

#undef RETURN_CASE_STRING_FAILURE

File DebugLogger::getLogFile()
{
	File f = getLogFolder().getChildFile("Debuglog.txt");

	return f.getNonexistentSibling();
}

File DebugLogger::getLogFolder()
{
#if USE_BACKEND

	File f = File(PresetHandler::getDataFolder()).getChildFile("Logs/");

#else

	File f = ProjectHandler::Frontend::getAppDataDirectory().getChildFile("Logs/");

#endif

	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

String DebugLogger::getHeader()
{
	String header;
	NewLine nl;

	header << "# Debug Log file\n\n" << nl;
#if USE_BACKEND
	header << "Product: **HISE**  " << nl;
	header << "Version: **" << ProjectInfo::versionString << "**  " << nl;
#else

	header << "Product: **" << ProjectHandler::Frontend::getCompanyName() << " - " << ProjectHandler::Frontend::getProjectName() << "**  " << nl;
	header << "Version: **" << ProjectHandler::Frontend::getVersionString() << "**  " << nl;

#endif

	header << "Time created: **" << Time::getCurrentTime().formatted("%d.%B %Y - %H:%M:%S") << "**  " << nl << nl;

	return header;
}


void DebugLoggerComponent::paint(Graphics& g)
{
	g.fillAll(isFailing ? Colours::red.withAlpha(0.8f) : Colours::black.withAlpha(0.8f));

	g.setColour(Colours::white.withAlpha(0.4f));

	g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 2.0f);

	Rectangle<int> r = getLocalBounds();

	r.reduce(20, 20);
	r.setWidth(getWidth() - 200);

	g.setColour(Colours::white.withAlpha(0.1f));

	g.setFont(GLOBAL_BOLD_FONT().withHeight(40.0f));

	g.drawText("DEBUG LOG ENABLED", r, Justification::centred);

	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText(logger->getLastErrorMessage(), r, Justification::centredLeft);

	g.drawText("Warning Level:", performanceLevelSelector->getX(), 5, 140, 20, Justification::centred);
}


String DebugLogger::getSystemSpecs() const
{
	NewLine nl;

	String stats = "## System specification\n\n";

	stats << "Device: **" << SystemStats::getDeviceDescription() << "**  " << nl;
	stats << "User name: **" << SystemStats::getFullUserName() << "**  " << nl;
	stats << "CPU vendor: **" << SystemStats::getCpuVendor() << "**  " << nl;
	stats << "CPU cores: **" << SystemStats::getNumCpus() << "**  " << nl;
	stats << "CPU speed: **" << SystemStats::getCpuSpeedInMegaherz() << "**  " << nl;
	stats << "Memory size: **" << SystemStats::getMemorySizeInMegabytes() << "**  " << nl;
	stats << "Page size: **" << SystemStats::getPageSize() << "**  " << nl;
	stats << "OS: **" << SystemStats::getOperatingSystemName() << (SystemStats::isOperatingSystem64Bit() ? " 64bit" : " 32bit") << "**  " << nl;

#if !(IS_STANDALONE_APP || IS_STANDALONE_FRONTEND)

	juce::PluginHostType hostType;

	auto pt = hostType.getPluginLoadedAs();

	if (pt == AudioProcessor::wrapperType_AudioUnit)
	{
		stats << "Plugin Format: **AU**  " << nl;
	}
	else if (pt == AudioProcessor::wrapperType_AAX)
	{
		stats << "Plugin Format: **AAX**  " << nl;
	}
	else if (pt == AudioProcessor::wrapperType_VST)
	{
		stats << "Plugin Format: **VST**  " << nl;
	}

	hostType.getHostDescription();

	stats << "Host: **" << hostType.getHostDescription() << "**  " << nl;
	stats << "Host Path: **" << hostType.getHostPath() << "**  " << nl;
	
#else
	stats << "Host: **Standalone**  " << nl;
#endif

#if JUCE_64BIT
	stats << "Process bit architecture: **64 bit**  " << nl;
#else
	stats << "Process bit architecture: **32 bit**  " << nl;
#endif
	stats << "Sandboxed: **" << (SystemStats::isRunningInAppExtensionSandbox() ? " Yes" : " No") << "**  " << nl;
	
	stats << nl;

	return stats;
}

void DebugLogger::PriorityInversionChecker::preCallbackEvent()
{
	dynamic_cast<MainController*>(p)->getDebugLogger().checkPriorityInversion(p->getCallbackLock());
}
