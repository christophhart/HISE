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



MidiLooper::MidiLooper(MidiPlayer* p) :
	MidiPlayerBaseType(p),
	loopLength("Loop Length"),
	clearButton("Clear"),
	addButton("Add"),
	undoButton("Undo")
{
	setColour(HiseColourScheme::ComponentBackgroundColour, Colours::black.withAlpha(0.3f));
	setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
	setColour(HiseColourScheme::ComponentFillTopColourId, Colours::white);
	setColour(HiseColourScheme::ComponentFillBottomColourId, Colours::red);

	p->getMainController()->skin(loopLength);

	addAndMakeVisible(loopLength);
	loopLength.addItem("1 Bar", 1);
	loopLength.addItem("2 Bars", 2);
	loopLength.addItem("4 Bars", 4);
	loopLength.addListener(this);
	loopLength.setSelectedId(1, dontSendNotification);

	addAndMakeVisible(clearButton);
	clearButton.addListener(this);

	addAndMakeVisible(addButton);
	addButton.addListener(this);

	addAndMakeVisible(undoButton);
	undoButton.addListener(this);

	clearButton.setLookAndFeel(&blaf);
	addButton.setLookAndFeel(&blaf);
	undoButton.setLookAndFeel(&blaf);

	startTimer(30);
}

void MidiLooper::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{
	repaint();
}

void MidiLooper::sequencesCleared()
{
	repaint();
}

int MidiLooper::getPreferredHeight() const
{
	return 32 + 60;
}

void MidiLooper::comboBoxChanged(ComboBox*)
{
	if (auto seq = getPlayer()->getCurrentSequence())
	{
		auto lengthInQuarters = (double)loopLength.getSelectedId() * 4.0;

		seq->setLengthInQuarters(lengthInQuarters);
	}
}

void MidiLooper::buttonClicked(Button* b)
{
	if (b == &clearButton)
	{
		getPlayer()->clearCurrentSequence();
	}
	else if (b == &addButton)
	{
		HiseMidiSequence::Ptr newSeq = new HiseMidiSequence();
		newSeq->setId("Loop " + String(loopUuid++));
		newSeq->createEmptyTrack();

		auto lengthInQuarters = (double)loopLength.getSelectedId() * 4.0;
		newSeq->setLengthInQuarters(lengthInQuarters);

		getPlayer()->addSequence(newSeq);
	}
	else if (b == &undoButton)
	{
		getPlayer()->enableInternalUndoManager(true);
		getPlayer()->getUndoManager()->undo();
	}
}

void MidiLooper::paint(Graphics& g)
{
	g.fillAll(findColour(HiseColourScheme::ComponentBackgroundColour));

	auto ar = getLocalBounds();
	auto top = ar.removeFromTop(32);

	if (auto seq = getPlayer()->getCurrentSequence())
	{
		auto list = seq->getRectangleList(ar.toFloat());

		auto recList = getPlayer()->getListOfCurrentlyRecordedEvents()->getRectangleList(ar.toFloat());

		RectangleList<float> recordedEvents;

		for (int i = 0; i < recList.getNumRectangles(); i++)
		{
			if (list.containsRectangle(recList.getRectangle(i)))
				recordedEvents.add(recList.getRectangle(i));
		}

		g.setColour(findColour(HiseColourScheme::ComponentFillTopColourId));

		for (auto l : list)
			g.fillRect(l);

		g.setColour(findColour(HiseColourScheme::ComponentFillBottomColourId));

		for (auto rl : recordedEvents)
			g.fillRect(rl);
	}

	auto posX = (float)getPlayer()->getPlaybackPosition() * (float)getWidth();

	g.setColour(findColour(HiseColourScheme::ComponentFillTopColourId));
	g.drawLine(posX, (float)top.getBottom(), posX, (float)getHeight(), 2.0f);
}

void MidiLooper::timerCallback()
{
	if (getPlayer()->getPlayState() == MidiPlayer::PlayState::Stop)
		return;

	if (auto seq = getPlayer()->getCurrentSequence())
	{
		auto thisPos = (int)(getPlayer()->getPlaybackPosition() * seq->getLengthInQuarters());
		lastPos = thisPos;

		repaint();
	}
}

void MidiLooper::resized()
{
	constexpr int margin = 20;

	auto ar = getLocalBounds();

	auto top = ar.removeFromTop(32);

	loopLength.setBounds(top.removeFromLeft(128));
	ar.removeFromLeft(margin);
	addButton.setBounds(top.removeFromLeft(70));
	clearButton.setBounds(top.removeFromLeft(70));
	undoButton.setBounds(top.removeFromLeft(70));
}

}
