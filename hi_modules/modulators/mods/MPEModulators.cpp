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

void MPEPanel::timerCallback()
{
	if (!pendingMessages.isEmpty())
	{
		MidiMessage m;

		while (pendingMessages.pop(m))
		{
			DBG(m.getDescription());

			if (m.isNoteOn())
			{
				pressedNotes.insert(Note::fromMidiMessage(*this, m));
			}
			else if (m.isNoteOff())
			{
				for (int i = 0; i < pressedNotes.size(); i++)
				{
					if (pressedNotes[i] == m)
						pressedNotes.removeElement(i--);
				}
			}
			else
			{
				for (auto& n : pressedNotes)
					n.updateNote(*this, m);
			}

		}

		repaint();
	}
}

void MPEPanel::handleNoteOn(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
	auto m = MidiMessage::noteOn(midiChannel, midiNoteNumber, velocity);

	pendingMessages.push(std::move(m));
}

void MPEPanel::handleNoteOff(MidiKeyboardState* /*source*/, int midiChannel, int midiNoteNumber, float velocity)
{
	auto m = MidiMessage::noteOff(midiChannel, midiNoteNumber, velocity);

	pendingMessages.push(std::move(m));
}

void MPEPanel::handleMessage(const MidiMessage& m)
{
	MidiMessage copy(m);

	pendingMessages.push(std::move(copy));
}

void MPEPanel::mouseDown(const MouseEvent& e)
{
	auto n = Note::fromMouseEvent(*this, e, nextChannelIndex);
	n.sendNoteOn(state);

	pressedNotes.insert(n);

	nextChannelIndex++;

	if (nextChannelIndex > 15)
		nextChannelIndex = 1;

	repaint();
}

void MPEPanel::mouseUp(const MouseEvent& e)
{
	for (int i = 0; i < pressedNotes.size(); i++)
	{
		if (pressedNotes[i] == e)
		{
			pressedNotes[i].sendNoteOff(state);
			pressedNotes.removeElement(i--);
			break;
		}

	}

	repaint();
}

void MPEPanel::paint(Graphics& g)
{
	Colour c1 = findColour(bgColour).withMultipliedAlpha(1.1f);
	Colour c2 = findColour(bgColour).withMultipliedAlpha(0.9f);

	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
	g.fillAll();

	static const int whiteWave[24] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0,
										0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	for (int i = 0; i < 24; i++)
	{
		auto l = getPositionForNote(lowKey + i);

		const float radius = getWidthForNote() * 0.2f;

		if (whiteWave[i] == 1)
		{
			g.setColour(findColour(waveColour));
			g.drawLine(l.getCentreX(), radius, l.getCentreX(), l.getHeight()-2.0f*radius, 4.0f);
		}
		else if (whiteWave[i] == 0)
		{
			l.reduce(4, 3);

			g.setColour(findColour(waveColour).withMultipliedAlpha(0.1f));
			g.fillRoundedRectangle(l, radius);
		}
	}

	for (const auto& n : pressedNotes)
		n.draw(*this, g);
}

juce::Rectangle<float> MPEPanel::getPositionForNote(int noteNumber) const
{
	int normalised = noteNumber - lowKey;
	float widthPerWave = getWidthForNote();

	if (normalised > 24 || normalised < 0)
		return {};

	static const int whiteWave[24] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	return { normalised * widthPerWave, 0.0f, widthPerWave, whiteWave[normalised] ? (float)getHeight()*0.5f : (float)getHeight() };
}

hise::MPEPanel::Note MPEPanel::Note::fromMouseEvent(const MPEPanel& p, const MouseEvent& e, int channelIndex)
{
	Note n;

	n.isArtificial = true;
	n.fingerIndex = e.source.getIndex();
	n.assignedMidiChannel = channelIndex;
	n.noteNumber = p.getNoteForPosition(e.getMouseDownPosition());
	n.glideValue = 64;
	n.slideValue = 8192;
	n.strokeValue = 127;
	n.liftValue = 127;
	n.pressureValue = 0;

	n.startPoint = { (int)p.getPositionForNote(n.noteNumber).getCentreX(), e.getMouseDownY() };


	n.dragPoint = n.startPoint;

	return n;
}

hise::MPEPanel::Note MPEPanel::Note::fromMidiMessage(const MPEPanel& p, const MidiMessage& m)
{
	Note n;

	n.isArtificial = false;
	n.fingerIndex = -1;
	n.assignedMidiChannel = m.getChannel();
	n.noteNumber = m.getNoteNumber();
	n.strokeValue = m.getVelocity();
	n.slideValue = 8192;
	n.liftValue = 0;
	n.glideValue = 64;
	n.pressureValue = 0;

	auto sp = p.getPositionForNote(n.noteNumber).getCentre();


	n.startPoint = { (int)sp.getX(), (int)sp.getY() };
	n.dragPoint = n.startPoint;

	return n;
}

void MPEPanel::Note::updateNote(const MPEPanel& p, const MouseEvent& e)
{
	if (*this != e)
		return;

	dragPoint = e.getPosition();

	float pitchBendValue = (float)e.getDistanceFromDragStartX() / p.getWidthForNote();
	slideValue = jlimit<int>(0, 8192*2, 8192 + (int)(pitchBendValue / 24.0f * 4096.0f));
	float normalisedGlide = -0.5f * (float)e.getDistanceFromDragStartY() / (float)p.getHeight();
	glideValue = jlimit<int>(0, 127, 64 + roundFloatToInt(normalisedGlide * 127.0f));

	p.state.injectMessage(MidiMessage::pitchWheel(assignedMidiChannel, slideValue));
	p.state.injectMessage(MidiMessage::controllerEvent(assignedMidiChannel, 74, glideValue));
}

void MPEPanel::Note::updateNote(const MPEPanel& p, const MidiMessage& m)
{
	if (*this != m)
		return;

	if (m.isPitchWheel())
	{
		slideValue = m.getPitchWheelValue();

		float normalisedSlideValue = (float)(slideValue - 8192) / 4096.0f;

		float slideOctaveValue = normalisedSlideValue * 24.0f;
		float slideDistance = slideOctaveValue * p.getWidthForNote();

		dragPoint.setX(startPoint.getX() + slideDistance);

	}
		
	else if (m.isChannelPressure())
		pressureValue = m.getChannelPressureValue();
	else if (m.isControllerOfType(74))
	{
		glideValue = m.getControllerValue();

		auto distance = (float)(glideValue-64) / 32.0f;

		dragPoint.setY(startPoint.getY() - distance * startPoint.getY());
	}
		
	else if (m.isNoteOff())
		liftValue = m.getVelocity();
}

void MPEPanel::Note::draw(const MPEPanel& p, Graphics& g) const
{
	if (!isVisible(p))
		return;

	auto area = p.getPositionForNote(noteNumber);

	g.setColour(p.findColour(keyOnColour).withAlpha((float)pressureValue / 127.0f));

	area.reduce(4.0f, 3.0f);

	auto radius = p.getWidthForNote() * 0.2f;

	g.fillRoundedRectangle(area, radius);

	g.setColour(p.findColour(dragColour));

	auto l = Line<int>(startPoint, dragPoint);

	g.drawLine((float)l.getStartX(), (float)l.getStartY(), (float)l.getEndX(), (float)l.getEndY(), 2.0f);

	Rectangle<float> r((float)dragPoint.getX(), (float)dragPoint.getY(), 0.0f, 0.0f);
	r = r.withSizeKeepingCentre(10, 10);

	g.setColour(Colours::white.withAlpha((float)strokeValue / 127.0f));

	g.drawEllipse(r, 2.0f);
}

}

