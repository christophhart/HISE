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

	getState(voiceIndex)->midiChannel = unsavedChannel;
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

		for (int i = 0; i < numSamples; i++)
		{
			w[i] = s->smoother.smooth(s->targetValue);
		}

		if (polyManager.getLastStartedVoice() == voiceIndex)
		{
			setOutputValue(w[0]);
		}

		
	}
}

void MPEModulator::handleHiseEvent(const HiseEvent& m)
{
	EnvelopeModulator::handleHiseEvent(m);

	if (m.isNoteOn())
	{
		unsavedChannel = m.getChannel();
	}
	else if (m.isChannelPressure() && g == Press)
	{
		auto c = m.getChannel();

		float targetValue = (float)m.getNoteNumber() / 127.0f;

		sendTableIndexChangeMessage(false, table, targetValue);

		targetValue = table->getInterpolatedValue(targetValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

		

		for (int i = 0; i < states.size(); i++)
		{
			if (auto s = getState(i))
			{
				if (s->midiChannel == c)
				{
					s->targetValue = targetValue;
				}
			}	
		}
	}
	else if (m.isControllerOfType(74) && g == Timbre)
	{
		auto c = m.getChannel();

		float targetValue = (float)m.getChannelPressureValue() / 127.0f;

		sendTableIndexChangeMessage(false, table, targetValue);

		targetValue = table->getInterpolatedValue(targetValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

		for (int i = 0; i < states.size(); i++)
		{
			if (auto s = getState(i))
			{
				if (s->midiChannel == c)
				{
					s->targetValue = targetValue;
				}
			}
		}
	}
	else if (m.isPitchWheel() && g == Glide)
	{
		auto c = m.getChannel();

		float targetValue = ((float)m.getPitchWheelValue() - 8192.0f) / 2048.0f;
		targetValue = 0.5f * targetValue + 0.5;

		DBG(targetValue);

		sendTableIndexChangeMessage(false, table, targetValue);

		targetValue = table->getInterpolatedValue(targetValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);

		for (int i = 0; i < states.size(); i++)
		{
			if (auto s = getState(i))
			{
				if (s->midiChannel == c)
				{
					s->targetValue = targetValue;
				}
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

