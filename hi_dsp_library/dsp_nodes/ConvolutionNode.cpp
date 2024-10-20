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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace filters
{


hise::MultiChannelAudioBuffer& convolution::getImpulseBufferBase()
{
	auto b = dynamic_cast<MultiChannelAudioBuffer*>(externalData.obj);
	jassert(b != nullptr);
	return *b;
}

void convolution::setExternalData(const snex::ExternalData& d, int index)
{
	base::setExternalData(d, index);

	if(!d.isEmpty())
	{
		getImpulseBufferBase().setDisabledXYZProviders({ Identifier("SampleMap"), Identifier("SFZ") });
		setImpulse(sendNotificationSync);
	}
}

void convolution::prepare(PrepareSpecs specs)
{
	if (specs.blockSize == 1)
		Error::throwError(Error::IllegalFrameCall);

	prepareBase(specs.sampleRate, specs.blockSize);
}

void convolution::reset()
{
	resetBase();
}

void convolution::setMultithread(double shouldBeMultithreaded)
{
	useBackgroundThread = shouldBeMultithreaded > 0.5;

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
        
        auto tToUse = useBackgroundThread && !nonRealtime ? &backgroundThread : nullptr;
        
		convolverL->setUseBackgroundThread(tToUse);
		convolverR->setUseBackgroundThread(tToUse);
	}
}

void convolution::setDamping(double targetSustainDb)
{
	if (damping != targetSustainDb)
	{
		damping = Decibels::decibelsToGain(targetSustainDb);
		setImpulse(sendNotificationAsync);
	}
}

void convolution::setHiCut(double targetFreq)
{
	if (cutoffFrequency != targetFreq)
	{
		cutoffFrequency = (double)targetFreq;
		calcCutoff();
	}
}

void convolution::setPredelay(double newDelay)
{
	if (newDelay != predelayMs)
	{
		predelayMs = newDelay;
		calcPredelay();
	}
}

void convolution::setGate(double shouldBeEnabled)
{
	processingEnabled = shouldBeEnabled >= 0.5;
	enableProcessing(processingEnabled);
}

void convolution::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(convolution, Gate);
		p.setParameterValueNames({ "Off", "On" });
		p.setDefaultValue(1.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(convolution, Predelay);
		p.setRange({ 0.0, 1000.0, 1.0 });
		p.setDefaultValue(0.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(convolution, Damping);
		p.setRange({ -100.0, 0.0, 0.1 });
		p.setDefaultValue(0.0);
		p.setSkewForCentre(-12.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(convolution, HiCut);
		p.setRange({ 20.0, 20000.0, 1.0 });
		p.setDefaultValue(20000.0);
		p.setSkewForCentre(1000.0);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(convolution, Multithread);
		p.setParameterValueNames({ "Off", "On" });
		data.add(std::move(p));
	}
}

const hise::MultiChannelAudioBuffer& filters::convolution::getImpulseBufferBase() const
{
	auto b = dynamic_cast<const MultiChannelAudioBuffer*>(externalData.obj);
	jassert(b != nullptr);
	return *b;
}

}



}
