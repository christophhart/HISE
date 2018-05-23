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


MPEModulator::MPEModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m):
	EnvelopeModulator(mc, id, voiceAmount, m),
	Modulation(m),
	table(new SampleLookupTable())
{
	for (int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));
}

MPEModulator::~MPEModulator()
{

}

void MPEModulator::setInternalAttribute(int parameter_index, float newValue)
{
	if (parameter_index == SpecialParameters::GestureCC)
		g = (Gesture)(int)newValue;
	if (parameter_index == SpecialParameters::SmoothingTime)
		updateSmoothingTime(newValue);
}

float MPEModulator::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex == SpecialParameters::GestureCC)
	{
		return (float)(int)Gesture::Press;
	}
	if (parameterIndex == SpecialParameters::SmoothingTime)
		return 200.0f;

}

float MPEModulator::getAttribute(int parameter_index) const
{
	if (parameter_index == SpecialParameters::GestureCC)
	{
		return (float)(int)g;
	}
	if (parameter_index == SpecialParameters::SmoothingTime)
		return smoothingTime;
}

void MPEModulator::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttribute(GestureCC, "GestureCC");
	loadAttribute(SmoothingTime, "SmoothingTime");
	loadTable(table, "Table");
}

juce::ValueTree MPEModulator::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(GestureCC, "GestureCC");
	saveAttribute(SmoothingTime, "SmoothingTime");
	saveTable(table, "Table");

	return v;
}

void MPEModulator::startVoice(int voiceIndex)
{
	EnvelopeModulator::startVoice(voiceIndex);

	if (auto s = getState(voiceIndex))
	{
		s->midiChannel = unsavedChannel;

		switch (g)
		{
		case Press:		s->targetValue = 0.0f; break;
		case Slide:		s->targetValue = 0.5f; break;
		case Glide:		s->targetValue = 0.5f; break;
		case Stroke:	s->targetValue = 0.0f; break;
		case Lift:		s->targetValue = getMode() == Modulation::GainMode ? 0.0f : 0.5f; break;
		default:
			break;
		}

		s->targetValue = table->getInterpolatedValue(s->targetValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

		s->smoother.resetToValue(s->targetValue);

		if (g == Stroke)
		{
			s->targetValue = unsavedStrokeValue;
		}

		activeStates.insert(s);
	}
}

void MPEModulator::stopVoice(int voiceIndex)
{
	EnvelopeModulator::stopVoice(voiceIndex);
}

void MPEModulator::reset(int voiceIndex)
{
	EnvelopeModulator::reset(voiceIndex);

	if (auto s = getState(voiceIndex))
	{
		activeStates.remove(s);

		s->smoother.setDefaultValue(0.0f);
		s->midiChannel = -1;
	}
}

bool MPEModulator::isPlaying(int /*voiceIndex*/) const
{
	return true;
}

void MPEModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	for (auto s_ : states)
	{
		auto s = static_cast<MPEState*>(s_);
		
		s->smoother.prepareToPlay(sampleRate);
		s->smoother.setSmoothingTime(smoothingTime);
	}
}

void MPEModulator::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();

	if (auto s = getState(voiceIndex))
	{
		auto w = internalBuffer.getWritePointer(0, startSample);

		s->smoother.fillBufferWithSmoothedValue(s->targetValue, w, numSamples);

		if (polyManager.getLastStartedVoice() == voiceIndex)
		{
			setOutputValue(w[0]);
		}
	}
}

void MPEModulator::handleHiseEvent(const HiseEvent& m)
{
	EnvelopeModulator::handleHiseEvent(m);

	float inputValue;

	if (m.isNoteOn())
	{
		unsavedChannel = m.getChannel();

		if (g == Stroke)
		{
			inputValue = (float)m.getVelocity() / 127.0f;

			inputValue = jlimit(0.0f, 1.0f, inputValue);

			const float targetValue = table->getInterpolatedValue(inputValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

			sendTableIndexChangeMessage(false, table, inputValue);

			unsavedStrokeValue = targetValue;
		}
		
		return;
	}
	else if (g == Press && m.isChannelPressure())
	{
		inputValue = (float)m.getNoteNumber() / 127.0f;
	}
	else if (g == Slide && m.isControllerOfType(74))
	{
		inputValue = (float)m.getControllerValue() / 127.0f;
	}
	else if (g == Glide && m.isPitchWheel())
	{
		inputValue = ((float)m.getPitchWheelValue() - 8192.0f) / 2048.0f;
		inputValue = 0.5f * inputValue + 0.5;
	}
	else if (g == Lift && m.isNoteOff())
	{
		inputValue = (float)m.getVelocity() / 127.0f;
	}
	else
	{
		return;
	}

	auto c = m.getChannel();

	inputValue = jlimit(0.0f, 1.0f, inputValue);

	const float targetValue = table->getInterpolatedValue(inputValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

	bool found = false;

	for (auto s : activeStates)
	{
		if (s->midiChannel == c)
		{
			s->targetValue = inputValue;

			if (!found && s->index == polyManager.getLastStartedVoice())
			{
				sendTableIndexChangeMessage(false, table, inputValue);
				found = true;
			}
		}
			
	}

}

hise::ProcessorEditorBody * MPEModulator::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new MPEModulatorEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);

	jassertfalse;
	return nullptr;

#endif
}

hise::MPEModulator::MPEState * MPEModulator::getState(int voiceIndex)
{
	if (auto s = states[voiceIndex])
		return static_cast<MPEState*>(s);

	return nullptr;
}

const hise::MPEModulator::MPEState * MPEModulator::getState(int voiceIndex) const
{
	if (auto s = states[voiceIndex])
		return static_cast<MPEState*>(s);

	return nullptr;
}

}

