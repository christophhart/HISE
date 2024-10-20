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

namespace hise {
using namespace juce;


FilterBank::FilterBank(int numVoices_) :
	numVoices(numVoices_),
	mode(FilterMode::numFilterModes),
	type(FilterHelpers::FilterSubType::numFilterSubTypes)
{
	setMode(FilterMode::StateVariableLP);
	setSampleRate(44100.0);
}


FilterBank::~FilterBank()
{
	object = nullptr;
}



void FilterBank::setMode(FilterMode newMode)
{
	if (newMode == mode)
		return;

	mode = newMode;

	switch (mode)
	{
	case FilterMode::OnePoleLowPass:
		setType(FilterHelpers::FilterSubType::SimpleOnePoleSubType, SimpleOnePoleSubType::FilterType::LP);
		break;
	case FilterMode::OnePoleHighPass:
		setType(FilterHelpers::FilterSubType::SimpleOnePoleSubType, SimpleOnePoleSubType::FilterType::HP);
		break;
	case FilterMode::LowPass:
		setType(FilterHelpers::FilterSubType::StaticBiquadSubType, StaticBiquadSubType::FilterType::LowPass);
		break;
	case FilterMode::HighPass:
		setType(FilterHelpers::FilterSubType::StaticBiquadSubType, StaticBiquadSubType::FilterType::HighPass);
		break;
	case FilterMode::LowShelf:
		setType(FilterHelpers::FilterSubType::StaticBiquadSubType, StaticBiquadSubType::FilterType::LowShelf);
		calculateGainModValue = true;
		break;
	case FilterMode::HighShelf:
		setType(FilterHelpers::FilterSubType::StaticBiquadSubType, StaticBiquadSubType::FilterType::HighShelf);
		calculateGainModValue = true;
		break;
	case FilterMode::Peak:
		setType(FilterHelpers::FilterSubType::StaticBiquadSubType, StaticBiquadSubType::FilterType::Peak);
		calculateGainModValue = true;
		break;
	case FilterMode::ResoLow:
		setType(FilterHelpers::FilterSubType::StaticBiquadSubType, StaticBiquadSubType::FilterType::ResoLow);
		break;
	case FilterMode::StateVariableLP:
		setType(FilterHelpers::FilterSubType::StateVariableFilterSubType, StateVariableFilterSubType::FilterType::LP);
		break;
	case FilterMode::StateVariableHP:
		setType(FilterHelpers::FilterSubType::StateVariableFilterSubType, StateVariableFilterSubType::FilterType::HP);
		break;
	case FilterMode::StateVariableBandPass:
		setType(FilterHelpers::FilterSubType::StateVariableFilterSubType, StateVariableFilterSubType::FilterType::BP);
		break;
	case FilterMode::StateVariableNotch:
		setType(FilterHelpers::FilterSubType::StateVariableFilterSubType, StateVariableFilterSubType::FilterType::NOTCH);
		break;
	case FilterMode::RingMod:
		setType(FilterHelpers::FilterSubType::RingmodFilterSubType, 0);
		break;
	case FilterMode::Allpass:
		setType(FilterHelpers::FilterSubType::PhaseAllpassSubType, 0);
		break;
	case FilterMode::MoogLP:
		setType(FilterHelpers::FilterSubType::MoogFilterSubType, 0);
		break;
	case FilterMode::LadderFourPoleLP:
		setType(FilterHelpers::FilterSubType::LadderSubType, 0);
		break;
	default:
		break;
	}
}

#define SET_POLY_TYPE(x) case FilterHelpers::FilterSubType::x: {auto n = new InternalPolyBank<x>(numVoices); \
															   n->setType(filterSubType); \
															   newObject = n; break;}
#define SET_MONO_TYPE(x) case FilterHelpers::FilterSubType::x: { auto n = new InternalMonoBank<x>(); \
															   n->setType(filterSubType); \
															   newObject = n; break;}

void FilterBank::setType(FilterHelpers::FilterSubType newType, int filterSubType)
{
	if (type == newType && subType == filterSubType)
		return;

	ScopedPointer<InternalBankBase> newObject;

	if (isPoly())
	{
		switch (newType)
		{
			SET_POLY_TYPE(MoogFilterSubType);
			SET_POLY_TYPE(LadderSubType);
			SET_POLY_TYPE(StateVariableFilterSubType);
			SET_POLY_TYPE(StaticBiquadSubType);
			SET_POLY_TYPE(SimpleOnePoleSubType);
			SET_POLY_TYPE(PhaseAllpassSubType);
			SET_POLY_TYPE(RingmodFilterSubType);
		default:
			jassertfalse;
			break;
		}
	}
	else
	{
		switch (newType)
		{
			SET_MONO_TYPE(MoogFilterSubType);
			SET_MONO_TYPE(LadderSubType);
			SET_MONO_TYPE(StateVariableFilterSubType);
			SET_MONO_TYPE(StaticBiquadSubType);
			SET_MONO_TYPE(SimpleOnePoleSubType);
			SET_MONO_TYPE(PhaseAllpassSubType);
			SET_MONO_TYPE(RingmodFilterSubType);
		default:
			jassertfalse;
			break;
		}
	}

	newObject->setFrequency(frequency);
	newObject->setGain(gain);
	newObject->setQ(q);
	newObject->setSampleRate(sampleRate);
	

	{
		SpinLock::ScopedLockType sl(lock);
		type = newType;
		subType = filterSubType;
		object.swapWith(newObject);
	}

	newObject = nullptr;
}


#undef SET_POLY_TYPE
#undef SET_MONO_TYPE


#define RENDER_POLY(x) case FilterHelpers::FilterSubType::x: getAsPoly<x>()->render(r); break;
#define RENDER_MONO(x) case FilterHelpers::FilterSubType::x: getAsMono<x>()->render(r); break;

void FilterBank::renderPoly(FilterHelpers::RenderData& r)
{
	SpinLock::ScopedLockType sl(lock);

	switch (type)
	{
		RENDER_POLY(MoogFilterSubType);
		RENDER_POLY(LadderSubType);
		RENDER_POLY(StateVariableFilterSubType);
		RENDER_POLY(StaticBiquadSubType);
		RENDER_POLY(SimpleOnePoleSubType);
		RENDER_POLY(PhaseAllpassSubType);
		RENDER_POLY(RingmodFilterSubType);
	default:
		jassertfalse;
		break;
	}
}

void FilterBank::renderMono(FilterHelpers::RenderData& r)
{
	SpinLock::ScopedLockType sl(lock);

	switch (type)
	{
		RENDER_MONO(MoogFilterSubType);
		RENDER_MONO(LadderSubType);
		RENDER_MONO(StateVariableFilterSubType);
		RENDER_MONO(StaticBiquadSubType);
		RENDER_MONO(SimpleOnePoleSubType);
		RENDER_MONO(PhaseAllpassSubType);
		RENDER_MONO(RingmodFilterSubType);
	default:
		jassertfalse;
		break;
	}
}



#undef RENDER_POLY
#undef RENDER_MONO

#define RESET_POLY(x) case FilterHelpers::FilterSubType::x: getAsPoly<x>()->reset(voiceIndex); break;
#define RESET_MONO(x) case FilterHelpers::FilterSubType::x: getAsMono<x>()->reset(); break;

void FilterBank::reset(int voiceIndex)
{
	SpinLock::ScopedLockType sl(lock);

	switch (type)
	{
		RESET_POLY(MoogFilterSubType);
		RESET_POLY(LadderSubType);
		RESET_POLY(StateVariableFilterSubType);
		RESET_POLY(StaticBiquadSubType);
		RESET_POLY(SimpleOnePoleSubType);
		RESET_POLY(PhaseAllpassSubType);
		RESET_POLY(RingmodFilterSubType);
	default:
		break;
	}
}

void FilterBank::reset()
{
	SpinLock::ScopedLockType sl(lock);

	switch (type)
	{
		RESET_MONO(MoogFilterSubType);
		RESET_MONO(LadderSubType);
		RESET_MONO(StateVariableFilterSubType);
		RESET_MONO(StaticBiquadSubType);
		RESET_MONO(SimpleOnePoleSubType);
		RESET_MONO(PhaseAllpassSubType);
		RESET_MONO(RingmodFilterSubType);
	default:
		break;
	}
}

FilterDataObject::CoefficientData FilterBank::getCurrentCoefficients() const noexcept
{
	return FilterEffect::getDisplayCoefficients(mode, freqModValue, q, gain * gainModValue, sampleRate);
}

void FilterBank::setQ(double newQ)
{
	q = newQ;
	object->setQ(newQ);
}

void FilterBank::setGain(float newGain)
{
	gain = newGain;
	object->setGain(newGain);
}

#undef RESET_MONO
#undef RESET_POLY

FilterDataObject::CoefficientData FilterEffect::getDisplayCoefficients(FilterBank::FilterMode m, double frequency, double q, float gain, double samplerate)
{
	auto srToUse = samplerate;

	if (srToUse < 1.0)
		srToUse = 44100.0;

	frequency = jlimit<double>(20.0, srToUse / 2.0, frequency);
	gain = jlimit<float>(0.01f, 32.0f, gain);
	q = jlimit<double>(0.3, 8.0, q);

	switch (m)
	{
	case FilterBank::OnePoleLowPass:		return { IIRCoefficients::makeLowPass(srToUse, frequency), 1};
	case FilterBank::OnePoleHighPass:		return { IIRCoefficients::makeHighPass(srToUse, frequency), 1 };
	case FilterBank::LowPass:				return { IIRCoefficients::makeLowPass(srToUse, frequency), 1};
	case FilterBank::HighPass:				return { IIRCoefficients::makeHighPass(srToUse, frequency, q), 1};
	case FilterBank::LowShelf:				return { IIRCoefficients::makeLowShelf(srToUse, frequency, q, gain), 1};
	case FilterBank::HighShelf:				return { IIRCoefficients::makeHighShelf(srToUse, frequency, q, gain), 1};
	case FilterBank::Peak:					return { IIRCoefficients::makePeakFilter(srToUse, frequency, q, gain), 1};
	case FilterBank::ResoLow:				return { IIRCoefficients::makeLowPass(srToUse, frequency, q), 1};
	case FilterBank::StateVariableLP:		return { IIRCoefficients::makeLowPass(srToUse, frequency, q), 1};
	case FilterBank::StateVariableHP:		return { IIRCoefficients::makeHighPass(srToUse, frequency, q), 1};
	case FilterBank::LadderFourPoleLP:		return { IIRCoefficients::makeLowPass(srToUse, frequency, 2.0*q), 1};
	case FilterBank::LadderFourPoleHP:		return { IIRCoefficients::makeHighPass(srToUse, frequency, 2.0*q), 1};
	case FilterBank::MoogLP:				return { IIRCoefficients::makeLowPass(srToUse, frequency, q), 1};
	case FilterBank::StateVariablePeak:     return { IIRCoefficients::makePeakFilter(srToUse, frequency, q, gain), 1};
	case FilterBank::StateVariableNotch:    return { IIRCoefficients::makeNotchFilter(srToUse, frequency, q), 1};
	case FilterBank::StateVariableBandPass: return { IIRCoefficients::makeBandPass(srToUse, frequency, q), 1};
	case FilterBank::Allpass:               return { IIRCoefficients::makeAllPass(srToUse, frequency, q), 1};
	case FilterBank::RingMod:               return { IIRCoefficients::makeAllPass(srToUse, frequency, q), 1};
	default:					return { IIRCoefficients(), 1 };
	}
}



juce::String FilterEffect::getTableValueAsGain(float input)
{
	return String(jmap<float>(input, -18.0f, 18.0f), 1) + " dB";
}

void FilterEffect::setRenderQuality(int powerOfTwo)
{
	if (powerOfTwo != 0 && isPowerOfTwo(powerOfTwo))
	{
		quality = powerOfTwo;
	}
}

int FilterEffect::getSampleAmountForRenderQuality() const
{
	return quality;
}

}
