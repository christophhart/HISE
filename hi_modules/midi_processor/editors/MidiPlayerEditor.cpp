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


MidiFilePlayerEditor::MidiFilePlayerEditor(ProcessorEditor* p) :
	ProcessorEditorBody(p),
	currentSequence("Current Sequence"),
	currentTrack("Current Track"),
	playButton("Start", this, factory),
	stopButton("Stop", this, factory),
	recordButton("Record", this, factory)
{
	dynamic_cast<MidiFilePlayer*>(getProcessor())->addSequenceListener(this);

	addAndMakeVisible(typeSelector);
	p->getProcessor()->getMainController()->skin(typeSelector);


	auto availableOverlays = MidiOverlayFactory::getInstance().getIdList();
	int overlayIndex = 1;

	for (auto overlay : availableOverlays)
		typeSelector.addItem(overlay.toString(), overlayIndex++);

	typeSelector.addListener(this);
	typeSelector.setTextWhenNothingSelected("Set Player type");

	addAndMakeVisible(currentPosition);
	currentPosition.setSliderStyle(Slider::LinearBar);
	currentPosition.setTextBoxStyle(Slider::NoTextBox, false, 10, 10);
	currentPosition.setRange(0.0, 1.0f, 0.001);
	currentPosition.setColour(Slider::ColourIds::trackColourId, Colours::white.withAlpha(0.2f));
	currentPosition.setColour(Slider::ColourIds::backgroundColourId, Colours::transparentBlack);
	currentPosition.setColour(Slider::ColourIds::thumbColourId, Colours::white.withAlpha(0.2f));
	getProcessor()->getMainController()->skin(currentPosition);
	updateLabel();

	addAndMakeVisible(currentTrack);
	currentTrack.setup(getProcessor(), MidiFilePlayer::CurrentTrack, "Track");
	currentTrack.setTextWhenNoChoicesAvailable("No tracks");

	addAndMakeVisible(clearButton);
	getProcessor()->getMainController()->skin(clearButton);
	clearButton.addListener(this);
	clearButton.setButtonText("Clear all");
	clearButton.setTriggeredOnMouseDown(true);

	addAndMakeVisible(playButton);
	playButton.addListener(this);
	playButton.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(stopButton);
	stopButton.addListener(this);
	stopButton.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(recordButton);
	recordButton.addListener(this);
	recordButton.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(currentSequence);
	currentSequence.setup(getProcessor(), MidiFilePlayer::CurrentSequence, "Current Sequence");
	currentSequence.setTextWhenNoChoicesAvailable("Nothing loaded");
	currentSequence.setTextWhenNothingSelected("Nothing loaded");


	startTimer(50);
}

MidiFilePlayerEditor::~MidiFilePlayerEditor()
{
	if (auto fp = dynamic_cast<MidiFilePlayer*>(getProcessor()))
		fp->removeSequenceListener(this);
}

void MidiFilePlayerEditor::timerCallback()
{
	{
		// Update the position

		double currentPos = getProcessor()->getAttribute(MidiFilePlayer::CurrentPosition);

		if (currentPosition.getValue() != currentPos)
			currentPosition.setValue(currentPos, dontSendNotification);
	}

	auto mp = dynamic_cast<MidiFilePlayer*>(getProcessor());

	{
		// Update the track amount
		auto currentSequence = mp->getCurrentSequence();

		int newTrackAmount = 0;

		if (currentSequence != nullptr)
			newTrackAmount = currentSequence->getNumTracks();

		if (newTrackAmount != currentTrackAmount)
		{
			currentTrackAmount = newTrackAmount;
			currentTrack.clear(dontSendNotification);

			for (int i = 0; i < currentTrackAmount; i++)
				currentTrack.addItem("Track" + String(i + 1), i + 1);

			currentTrack.setSelectedId(getProcessor()->getAttribute(MidiFilePlayer::CurrentTrack), dontSendNotification);
		}
	}

	{
		int currentNumSequences = currentSequence.getNumItems();
		int actualNumSequences = mp->getNumSequences();

		if (currentNumSequences != actualNumSequences)
		{
			currentSequence.clear();

			for (int i = 0; i < actualNumSequences; i++)
			{
				currentSequence.addItem(mp->getSequenceId(i).toString(), i + 1);
			}

			currentSequence.setSelectedId(mp->getAttribute(MidiFilePlayer::CurrentSequence), dontSendNotification);
		}


	}

	int lastTrack = currentTrack.getSelectedId();
}

void MidiFilePlayerEditor::updateGui()
{
	updateLabel();
	currentSequence.updateValue(dontSendNotification);
	currentTrack.updateValue(dontSendNotification);
}

void MidiFilePlayerEditor::buttonClicked(Button* b)
{
	if (b == &clearButton)
		dynamic_cast<MidiFilePlayer*>(getProcessor())->clearSequences();
	else if (b == &playButton)
		getProcessor()->setAttribute(MidiFilePlayer::Play, 0, sendNotification);
	else if (b == &stopButton)
		getProcessor()->setAttribute(MidiFilePlayer::Stop, 0, sendNotification);
	else if (b == &recordButton)
		getProcessor()->setAttribute(MidiFilePlayer::Record, 0, sendNotification);
}

void MidiFilePlayerEditor::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{

}

void MidiFilePlayerEditor::comboBoxChanged(ComboBox* c)
{
	if (c == &typeSelector)
	{
		auto type = Identifier(c->getText());
		auto mp = dynamic_cast<MidiFilePlayer*>(getProcessor());
		setNewPlayerType(MidiOverlayFactory::getInstance().create(type, mp));
	}
}

void MidiFilePlayerEditor::resized()
{
	auto ar = getLocalBounds();

	if (currentPlayerType != nullptr)
	{
		auto typeBounds = ar.removeFromBottom(currentPlayerType->getPreferredHeight() + margin);
		dynamic_cast<Component*>(currentPlayerType.get())->setBounds(typeBounds.reduced(margin));
	}

	typeSelector.setBounds(ar.removeFromLeft(128 + 2 * margin).reduced(margin));

	ar.removeFromLeft(margin);

	currentSequence.setBounds(ar.removeFromLeft(128 + 2 * margin).reduced(margin));
	currentTrack.setBounds(ar.removeFromLeft(128 + 2 * margin).reduced(margin));

	clearButton.setBounds(ar.removeFromLeft(80 + 2 * margin).reduced(margin));

	playButton.setBounds(ar.removeFromLeft(32 + 2 * margin).reduced(margin));
	stopButton.setBounds(ar.removeFromLeft(32 + 2 * margin).reduced(margin));
	recordButton.setBounds(ar.removeFromLeft(32 + 2 * margin).reduced(margin));

	currentPosition.setBounds(ar.reduced(margin));
}

juce::Path MidiFilePlayerEditor::TransportPaths::createPath(const String& name) const
{
	if (name == "Start")
	{
		Path p;
		p.addTriangle({ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.5f });
		return p;
	}
	if (name == "Stop")
	{
		Path p;
		p.addRectangle<float>({ 0.0f, 0.0f, 1.0f, 1.0f });
		return p;
	}
	if (name == "Record")
	{
		Path p;
		p.addEllipse({ 0.0f, 0.0f, 1.0f, 1.0f });
		return p;
	}
}




}