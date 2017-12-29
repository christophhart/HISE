/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software : you can redistribute it and / or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.If not, see < http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request.Please visit the project's website to get more
*   information about commercial licencing :
*
*   http ://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*which must be separately licensed for closed source applications :
*
*   http ://www.juce.com
*
* == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == =
*/

namespace hise { using namespace juce;


#if  JUCE_MAC

struct FileLimitInitialiser
{
	FileLimitInitialiser()
	{
		rlimit lim;

		getrlimit(RLIMIT_NOFILE, &lim);
		lim.rlim_cur = lim.rlim_max = 200000;
		setrlimit(RLIMIT_NOFILE, &lim);
	}
};



static FileLimitInitialiser fileLimitInitialiser;
#endif

double ScopedGlitchDetector::locationTimeSum[30] = { .0,.0,.0,.0,.0,.0,.0,.0,.0,.0, .0,.0,.0,.0,.0,.0,.0,.0,.0,.0, .0,.0,.0,.0,.0,.0,.0,.0,.0,.0};
int ScopedGlitchDetector::locationIndex[30] = { 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, };
int ScopedGlitchDetector::lastPositiveId = 0;

ScopedGlitchDetector::ScopedGlitchDetector(Processor* const processor, int location_) :
	location(location_),
	startTime(processor->getMainController()->getDebugLogger().isLogging() ? Time::getMillisecondCounterHiRes() : 0.0),
	p(processor)
{
	if (lastPositiveId == location)
	{
		// Resets the identifier if a GlitchDetector is recreated...
		lastPositiveId = 0;
	}
}

ScopedGlitchDetector::~ScopedGlitchDetector() 
{
	if (p.get() == nullptr)
		return;

	DebugLogger& logger = p->getMainController()->getDebugLogger();

	if (logger.isLogging())
	{
		const double stopTime = Time::getMillisecondCounterHiRes();
		const double interval = (stopTime - startTime);

		const double bufferMs = 1000.0 * (double)p->getBlockSize() / p->getSampleRate();

		locationTimeSum[location] += interval;
		locationIndex[location]++;

		const double allowedPercentage = getAllowedPercentageForLocation(location) * logger.getScaleFactorForWarningLevel();
		
		double maxTime = allowedPercentage * bufferMs;
		
		if (lastPositiveId == 0 && interval > maxTime)
		{
			lastPositiveId = location;

			const double average = locationTimeSum[location] / (double)locationIndex[location];
			const double thisTime = average / bufferMs;

			DebugLogger::PerformanceData  l(location, (float)(100.0 * interval / bufferMs), (float)(100.0 * thisTime), p);

			l.limit = (float)allowedPercentage;

			logger.logPerformanceWarning(l);
		}
	}
}

double ScopedGlitchDetector::getAllowedPercentageForLocation(int locationId)
{
	DebugLogger::Location l = (DebugLogger::Location)locationId;

	// You may change these values to adapt to your system.

	switch (l)
	{
	case DebugLogger::Location::Empty: jassertfalse;				return 0.0;
	case DebugLogger::Location::MainRenderCallback:					return 0.7;
	case DebugLogger::Location::MultiMicSampleRendering:			return 0.1;
	case DebugLogger::Location::SampleRendering:					return 0.1;
	case DebugLogger::Location::ScriptFXRendering:					return 0.15;
	case DebugLogger::Location::TimerCallback:						return 0.04;
	case DebugLogger::Location::SynthRendering:						return 0.15;
	case DebugLogger::Location::SynthChainRendering:				return 0.5;
	case DebugLogger::Location::SampleStart:						return 0.02;
	case DebugLogger::Location::VoiceEffectRendering:				return 0.1;
	case DebugLogger::Location::ModulatorChainVoiceRendering:		return 0.05;
	case DebugLogger::Location::ModulatorChainTimeVariantRendering: return 0.04;
	case DebugLogger::Location::SynthVoiceRendering:				return 0.2;
	case DebugLogger::Location::NoteOnCallback:						return 0.05;
	case DebugLogger::Location::MasterEffectRendering:				return 0.3;
	case DebugLogger::Location::ScriptMidiEventCallback:			return 0.04;
	case DebugLogger::Location::ConvolutionRendering:				return 0.1;
	case DebugLogger::Location::numLocations:						return 0.0;
	default:														return 0.0;
	}
}



void AutoSaver::timerCallback()
{
#if USE_BACKEND
	Processor *mainSynthChain = mc->getMainSynthChain();

	File backupFile = getAutoSaveFile();

	ValueTree v = mainSynthChain->exportAsValueTree();

	v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
	FileOutputStream fos(backupFile);
	v.writeToStream(fos);

	debugToConsole(mainSynthChain, "Autosaving as " + backupFile.getFileName());
#endif
}

File AutoSaver::getAutoSaveFile()
{
	Processor *mainSynthChain = mc->getMainSynthChain();

	File presetDirectory = GET_PROJECT_HANDLER(mainSynthChain).getSubDirectory(ProjectHandler::SubDirectories::Presets);

	if (presetDirectory.isDirectory())
	{
		if (fileList.size() == 0)
		{
			fileList.add(presetDirectory.getChildFile("Autosave_1.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_2.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_3.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_4.hip"));
			fileList.add(presetDirectory.getChildFile("Autosave_5.hip"));
		}

		File toReturn = fileList[currentAutoSaveIndex];

		if (toReturn.existsAsFile()) toReturn.deleteFile();

		currentAutoSaveIndex = (currentAutoSaveIndex + 1) % 5;

		return toReturn;
	}
	else
	{
		return File();
	}
}

#if USE_VDSP_FFT

class VDspFFT::Pimpl
{
public:
    Pimpl(int maxOrder=MAX_VDSP_FFT_SIZE):
    maxN(maxOrder)
    {
        setup = vDSP_create_fftsetup(maxOrder, kFFTRadix2); /* supports up to 2048 (2**11) points  */
        
        const int maxLength = 1 << maxN;
        
        temp.setSize(2, maxLength);
        temp.clear();
        
        tempBuffer.realp = temp.getWritePointer(0);
        tempBuffer.imagp = temp.getWritePointer(1);
        
        temp2.setSize(2, maxLength);
        temp2.clear();
        
        tempBuffer2.realp = temp2.getWritePointer(0);
        tempBuffer2.imagp = temp2.getWritePointer(1);
        
        temp3.setSize(2, maxLength);
        temp3.clear();
        
        tempBuffer3.realp = temp3.getWritePointer(0);
        tempBuffer3.imagp = temp3.getWritePointer(1);
    }
    
    ~Pimpl()
    {
        vDSP_destroy_fftsetup(setup);
    }
    
    void complexFFTInplace(float* data, int size, bool unpack=true)
    {
        const int N = (int)log2(size);
        jassert(N <= maxN);
        const int thisLength = size;
        
        if(unpack)
        {
            vDSP_ctoz((COMPLEX *) data, 2, &tempBuffer, 1, thisLength);
        }
        else
        {
            tempBuffer.realp = data;
            tempBuffer.imagp = data + size;
        }
        
        vDSP_fft_zip(setup, &tempBuffer, 1, N, FFT_FORWARD);
        //vDSP_ztoc(&tempBuffer, 1, (COMPLEX *) data, 2, thisLength);
        
        if(unpack)
        {
            FloatVectorOperations::copy(data, tempBuffer.realp, thisLength);
            FloatVectorOperations::copy(data + thisLength, tempBuffer.imagp, thisLength);
        }
    }
    
    void complexFFTInverseInplace(float* data, int size)
    {
        const int N = (int)log2(size);
        jassert(N <= maxN);
        const int thisLength = size;
        
        FloatVectorOperations::copy(temp3.getWritePointer(0), data, size*2);
        
        COMPLEX_SPLIT s;
        s.realp = temp3.getWritePointer(0);
        s.imagp = temp3.getWritePointer(0)+size;
        
        //vDSP_ctoz((COMPLEX *) data, 2, &tempBuffer, 1, thisLength);
        vDSP_fft_zip(setup, &s, 1, N, FFT_INVERSE);
        
        
        
        
        vDSP_ztoc(&s, 1, (COMPLEX *) data, 2, thisLength);
    }
    
    
    void multiplyComplex(float* output, float* in1, int in1Offset, float* in2, int in2Offset, int numSamples, bool addToOutput)
    {
        COMPLEX_SPLIT i1;
        i1.realp = in1+in1Offset;
        i1.imagp = in1+in1Offset + numSamples;
        
        COMPLEX_SPLIT i2;
        i2.realp = in2+in2Offset;
        i2.imagp = in2+in2Offset + numSamples;
        
        COMPLEX_SPLIT o;
        o.realp = output;
        o.imagp = output + numSamples;
        
        if(addToOutput)
            vDSP_zvma(&i1, 1, &i2, 1, &o, 1, &o, 1, numSamples);
        
        else
            vDSP_zvmul(&i1, 1, &i2, 1, &o, 1, numSamples, 1);
    }
    
    
    /** Convolves the signal with the impulse response.
     *
     *   The signal is a complex float array in the form [i0, r0, i1, r1, ... ].
     *   The ir is already FFT transformed in COMPLEX_SPLIT form
     */
    void convolveComplex(float* signal, const COMPLEX_SPLIT &ir, int N)
    {
        const int thisLength = 1 << N;
        
        vDSP_ctoz((COMPLEX *) signal, 2, &tempBuffer, 1, thisLength/2);
        vDSP_fft_zrip(setup, &tempBuffer, 1, N, FFT_FORWARD);
        
        float preserveIRNyq = ir.imagp[0];
        ir.imagp[0] = 0;
        float preserveSigNyq = tempBuffer.imagp[0];
        tempBuffer.imagp[0] = 0;
        vDSP_zvmul(&tempBuffer, 1, &ir, 1, &tempBuffer, 1, N, 1);
        tempBuffer.imagp[0] = preserveIRNyq * preserveSigNyq;
        ir.imagp[0] = preserveIRNyq;
        vDSP_fft_zrip(setup, &tempBuffer, 1, N, FFT_INVERSE);
        
        vDSP_ztoc(&tempBuffer, 1, (COMPLEX *)signal, 2, N);
        
        //float scale = 1.0 / (8*N);
        
        //FloatVectorOperations::multiply(signal, scale, thisLength);
    }
    
    /** Creates a complex split structure from two float arrays. */
    static COMPLEX_SPLIT createComplexSplit(float* real, float* img)
    {
        COMPLEX_SPLIT s;
        s.imagp = img;
        s.realp = real;
        
        return s;
    }
    
    /** Creates a partial complex split structure from another one. */
    static COMPLEX_SPLIT createComplexSplit(COMPLEX_SPLIT& other, int offset)
    {
        COMPLEX_SPLIT s;
        s.imagp = other.imagp + offset;
        s.realp = other.realp + offset;
        
        return s;
    }
    
    static COMPLEX_SPLIT createComplexSplit(juce::AudioSampleBuffer &buffer)
    {
        COMPLEX_SPLIT s;
        s.realp = buffer.getWritePointer(0);
        s.imagp = buffer.getWritePointer(1);
        
        return s;
    }
    
private:
    
    FFTSetup setup;
    
    COMPLEX_SPLIT tempBuffer;
    juce::AudioSampleBuffer temp;
    
    juce::AudioSampleBuffer temp2;
    COMPLEX_SPLIT tempBuffer2;
    
    COMPLEX_SPLIT tempBuffer3;
    juce::AudioSampleBuffer temp3;
    
    int maxN;
    
};

VDspFFT::VDspFFT(int maxN)
{
    pimpl = new Pimpl(maxN);
}

VDspFFT::~VDspFFT()
{
    pimpl = nullptr;
}

void VDspFFT::complexFFTInplace(float* data, int size, bool unpack)
{
    pimpl->complexFFTInplace(data, size, unpack);
}

void VDspFFT::complexFFTInverseInplace(float* data, int size)
{
    pimpl->complexFFTInverseInplace(data, size);
}

void VDspFFT::multiplyComplex(float* output, float* in1, int in1Offset, float* in2, int in2Offset, int numSamples, bool addToOutput)
{
    pimpl->multiplyComplex(output, in1, in1Offset, in2, in2Offset, numSamples, addToOutput);
    
}

#endif

HiseDeviceSimulator::DeviceType HiseDeviceSimulator::currentDevice = HiseDeviceSimulator::DeviceType::Desktop;

void HiseDeviceSimulator::init(AudioProcessor::WrapperType wrapper)
{
#if HISE_IOS
    const bool isIPad = SystemStats::getDeviceDescription() == "iPad";
    const bool isStandalone = wrapper != AudioProcessor::WrapperType::wrapperType_AudioUnitv3;
    
    if(isIPad)
		currentDevice = isStandalone ? DeviceType::iPad : DeviceType::iPadAUv3;
    else
        currentDevice = isStandalone ? DeviceType::iPhone : DeviceType::iPhoneAUv3;
#else
	ignoreUnused(wrapper);
    currentDevice = DeviceType::Desktop;
#endif
}

String HiseDeviceSimulator::getDeviceName(int index)
{
	DeviceType thisDevice = (index == -1) ? currentDevice : (DeviceType)index;

	switch (thisDevice)
	{
	case DeviceType::Desktop: return "Desktop";
	case DeviceType::iPad: return "iPad";
	case DeviceType::iPadAUv3: return "iPadAUv3";
	case DeviceType::iPhone: return "iPhone";
	case DeviceType::iPhoneAUv3: return "iPhoneAUv3";
	default:
		return{};
	}
}

bool HiseDeviceSimulator::fileNameContainsDeviceWildcard(const File& f)
{
	String fileName = f.getFileNameWithoutExtension();

	for (int i = 0; i < (int)DeviceType::numDeviceTypes; i++)
	{
		if (fileName.contains(getDeviceName(i)))
			return true;
	}

	return false;
}

Rectangle<int> HiseDeviceSimulator::getDisplayResolution()
{
	switch (currentDevice)
	{
	case HiseDeviceSimulator::DeviceType::Desktop:		return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPad:			return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPadAUv3:		return{ 0, 0, 1024, 335 };
	case HiseDeviceSimulator::DeviceType::iPhone:		return{ 0, 0, 568, 320 };
    case HiseDeviceSimulator::DeviceType::iPhoneAUv3:	return{ 0, 0, 568, 172 };
	case HiseDeviceSimulator::DeviceType::numDeviceTypes:
	default:
		return {};
	}
}

Array<StringArray> RegexFunctions::findSubstringsThatMatchWildcard(const String &regexWildCard, const String &stringToTest)
{
	Array<StringArray> matches;
	String remainingText = stringToTest;
	StringArray m = getFirstMatch(regexWildCard, remainingText);

	while (m.size() != 0 && m[0].length() != 0)
	{
		remainingText = remainingText.fromFirstOccurrenceOf(m[0], false, false);
		matches.add(m);
		m = getFirstMatch(regexWildCard, remainingText);
	}

	return matches;
}

StringArray RegexFunctions::search(const String& wildcard, const String &stringToTest, int indexInMatch/*=0*/)
{
#if TRAVIS_CI
	return StringArray(); // Travis CI seems to have a problem with libc++...
#else
	try
	{
		StringArray searchResults;

		std::regex includeRegex(wildcard.toStdString());
		std::string xAsStd = stringToTest.toStdString();
		std::sregex_iterator it(xAsStd.begin(), xAsStd.end(), includeRegex);
		std::sregex_iterator it_end;

		while (it != it_end)
		{
			std::smatch result = *it;

			StringArray matches;
			for (auto x : result)
			{
				matches.add(String(x));
			}

			if (indexInMatch < matches.size()) searchResults.add(matches[indexInMatch]);

			++it;
		}

		return searchResults;
	}
	catch (std::regex_error e)
	{
		DBG(e.what());
		return StringArray();
	}
#endif
}

StringArray RegexFunctions::getFirstMatch(const String &wildcard, const String &stringToTest, const Processor* /*processorForErrorOutput*//*=nullptr*/)
{
#if TRAVIS_CI
	return StringArray(); // Travis CI seems to have a problem with libc++...
#else

	try
	{
		std::regex reg(wildcard.toStdString());
		std::string s(stringToTest.toStdString());
		std::smatch match;


		if (std::regex_search(s, match, reg))
		{
			StringArray sa;

			for (auto x : match)
			{
				sa.add(String(x));
			}

			return sa;
		}

		return StringArray();
	}
	catch (std::regex_error e)
	{
		jassertfalse;

		DBG(e.what());
		return StringArray();
	}
#endif
}

bool RegexFunctions::matchesWildcard(const String &wildcard, const String &stringToTest, const Processor* /*processorForErrorOutput*//*=nullptr*/)
{
#if TRAVIS_CI
	return false; // Travis CI seems to have a problem with libc++...
#else

	try
	{
		std::regex reg(wildcard.toStdString());

		return std::regex_search(stringToTest.toStdString(), reg);
	}
	catch (std::regex_error e)
	{
		DBG(e.what());

		return false;
	}
#endif
}

ScopedNoDenormals::ScopedNoDenormals()
{
#if JUCE_IOS
#else
	oldMXCSR = _mm_getcsr();
	int newMXCSR = oldMXCSR | 0x8040;
	_mm_setcsr(newMXCSR);
#endif
}

ScopedNoDenormals::~ScopedNoDenormals()
{
#if JUCE_IOS
#else
	_mm_setcsr(oldMXCSR);
#endif
}

void FloatSanitizers::sanitizeArray(float* data, int size)
{
	uint32* dataAsInt = reinterpret_cast<uint32*>(data);

	for (int i = 0; i < size; i++)
	{
		const uint32 sample = *dataAsInt;
		const uint32 exponent = sample & 0x7F800000;

		const int aNaN = exponent < 0x7F800000;
		const int aDen = exponent > 0;

		*dataAsInt++ = sample * (aNaN & aDen);

	}
}

float FloatSanitizers::sanitizeFloatNumber(float& input)
{
	uint32* valueAsInt = reinterpret_cast<uint32*>(&input);
	const uint32 exponent = *valueAsInt & 0x7F800000;

	const int aNaN = exponent < 0x7F800000;
	const int aDen = exponent > 0;

	const uint32 sanitized = *valueAsInt * (aNaN & aDen);

	return *reinterpret_cast<const float*>(&sanitized);
}

void FloatSanitizers::Test::runTest()
{
	beginTest("Testing array method");

	float d[6];

	d[0] = INFINITY;
	d[1] = FLT_MIN / 20.0f;
	d[2] = FLT_MIN / -14.0f;
	d[3] = NAN;
	d[4] = 24.0f;
	d[5] = 0.0052f;

	sanitizeArray(d, 6);

	expectEquals<float>(d[0], 0.0f, "Infinity");
	expectEquals<float>(d[1], 0.0f, "Denormal");
	expectEquals<float>(d[2], 0.0f, "Negative Denormal");
	expectEquals<float>(d[3], 0.0f, "NaN");
	expectEquals<float>(d[4], 24.0f, "Normal Number");
	expectEquals<float>(d[5], 0.0052f, "Small Number");

	beginTest("Testing single method");

	float d0 = INFINITY;
	float d1 = FLT_MIN / 20.0f;
	float d2 = FLT_MIN / -14.0f;
	float d3 = NAN;
	float d4 = 24.0f;
	float d5 = 0.0052f;

	d0 = sanitizeFloatNumber(d0);
	d1 = sanitizeFloatNumber(d1);
	d2 = sanitizeFloatNumber(d2);
	d3 = sanitizeFloatNumber(d3);
	d4 = sanitizeFloatNumber(d4);
	d5 = sanitizeFloatNumber(d5);

	expectEquals<float>(d0, 0.0f, "Single Infinity");
	expectEquals<float>(d1, 0.0f, "Single Denormal");
	expectEquals<float>(d2, 0.0f, "Single Negative Denormal");
	expectEquals<float>(d3, 0.0f, "Single NaN");
	expectEquals<float>(d4, 24.0f, "Single Normal Number");
	expectEquals<float>(d5, 0.0052f, "Single Small Number");
}

void SafeChangeBroadcaster::sendSynchronousChangeMessage()
{
	if (MessageManager::getInstance()->isThisTheMessageThread() || MessageManager::getInstance()->currentThreadHasLockedMessageManager())
	{
		ScopedLock sl(listeners.getLock());

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
			{
				listeners[i]->changeListenerCallback(this);
			}
			else
			{
				// Ooops, you called an deleted listener. 
				// Normally, it would crash now, but since this is really lame, this class only throws an assert!
				jassertfalse;

				listeners.remove(i--);
			}
		}
	}
	else
	{
		sendChangeMessage();
	}

	
}

void SafeChangeBroadcaster::addChangeListener(SafeChangeListener *listener)
{
	ScopedLock sl(listeners.getLock());

	listeners.addIfNotAlreadyThere(listener);
}

void SafeChangeBroadcaster::removeChangeListener(SafeChangeListener *listener)
{
	ScopedLock sl(listeners.getLock());

	listeners.removeAllInstancesOf(listener);
}

void SafeChangeBroadcaster::removeAllChangeListeners()
{
	dispatcher.cancelPendingUpdate();

	ScopedLock sl(listeners.getLock());

	listeners.clear();
}

void SafeChangeBroadcaster::sendChangeMessage(const String &/*identifier*/ /*= String()*/)
{
	dispatcher.triggerAsyncUpdate();
}

void SafeChangeBroadcaster::sendAllocationFreeChangeMessage()
{
	// You need to call enableAllocationFreeMessages() first...
	jassert(flagTimer.isTimerRunning());

	flagTimer.triggerUpdate();
}

void SafeChangeBroadcaster::enableAllocationFreeMessages(int timerIntervalMilliseconds)
{
	flagTimer.startTimer(timerIntervalMilliseconds);
}



float BalanceCalculator::getGainFactorForBalance(float balanceValue, bool calculateLeftChannel)
{
	if (balanceValue == 0.0f) return 1.0f;

	const float balance = jlimit(-1.0f, 1.0f, balanceValue / 100.0f);

	float panValue = (float_Pi * (balance + 1.0f)) * 0.25f;

	return 1.41421356237309504880f * (calculateLeftChannel ? cosf(panValue) : sinf(panValue));
}

void BalanceCalculator::processBuffer(AudioSampleBuffer &stereoBuffer, float *panValues, int startSample, int numSamples)
{
	FloatVectorOperations::multiply(panValues + startSample, float_Pi * 0.5f, numSamples);

	stereoBuffer.applyGain(1.4142f); // +3dB for equal power...

	float *l = stereoBuffer.getWritePointer(0, startSample);
	float *r = stereoBuffer.getWritePointer(1, startSample);

	while (--numSamples >= 0)
	{
		*l++ *= cosf(*panValues) * 1.4142f;
		*r++ *= sinf(*panValues);

		panValues++;
	}
}

String BalanceCalculator::getBalanceAsString(int balanceValue)
{
	if (balanceValue == 0) return "C";

	else return String(balanceValue) + (balanceValue > 0 ? "R" : "L");
}

SafeFunctionCall::SafeFunctionCall(Processor* p_, const ProcessorFunction& f_) :
	p(p_),
	f(f_)
{

}

SafeFunctionCall::SafeFunctionCall() :
	p(nullptr),
	f()
{

}

bool SafeFunctionCall::call()
{
	if (p.get() != nullptr)
		return f(p.get());

	return false;
}

} // namespace hise
