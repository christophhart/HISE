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
*which must be separately licensed for cloused source applications :
*
*   http ://www.juce.com
*
* == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == == =
*/

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
#if USE_BACKEND

	currentDevice = DeviceType::Desktop;
#else

    
    
#if JUCE_IOS
    
    
    const bool isIPad = SystemStats::getDeviceDescription() == "iPad";
    
    const bool isStandalone = wrapper != AudioProcessor::WrapperType::wrapperType_AudioUnitv3;
    
    
    
    if(isIPad)
    {
        currentDevice = isStandalone ? DeviceType::iPad : DeviceType::iPadAUv3;
    }
    else
    {
        currentDevice = DeviceType::iPhone5;
    }
    
#else
    currentDevice = DeviceType::Desktop;
#endif
    
    
#endif
    
    
    int x = 5;
    
    
}

String HiseDeviceSimulator::getDeviceName(int index)
{
	DeviceType thisDevice = (index == -1) ? currentDevice : (DeviceType)index;

	switch (thisDevice)
	{
	case DeviceType::Desktop: return "Desktop";
	case DeviceType::iPad: return "iPad";
	case DeviceType::iPadRetina: return "iPadRetina";
	case DeviceType::iPadPro: return "iPadPro";
	case DeviceType::iPadAUv3: return "iPadAUv3";
	case DeviceType::iPhone5: return "iPhone5";
	case DeviceType::iPhone6: return "iPhone6";
	case DeviceType::iPodTouch6: return "iPodTouch6";
	default:
		return{};
	}
}

Rectangle<int> HiseDeviceSimulator::getDisplayResolution()
{
	switch (currentDevice)
	{
	case HiseDeviceSimulator::DeviceType::Desktop:		return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPad:			return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPadRetina:	return{ 0, 0, 1024, 768 };
	case HiseDeviceSimulator::DeviceType::iPadPro:		return{ 0, 0, 1366, 1024 };
	case HiseDeviceSimulator::DeviceType::iPadAUv3:		return{ 0, 0, 1024, 335 };
	case HiseDeviceSimulator::DeviceType::iPhone5:		return{ 0, 0, 568, 320 };
	case HiseDeviceSimulator::DeviceType::iPhone6:		return{ 0, 0, 568, 320 };
	case HiseDeviceSimulator::DeviceType::iPodTouch6:	return{ 0, 0, 568, 320 };
	case HiseDeviceSimulator::DeviceType::numDeviceTypes:
	default:
		return {};
	}
}
