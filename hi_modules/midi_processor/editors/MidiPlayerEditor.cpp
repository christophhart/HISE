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


MidiPlayerEditor::MidiPlayerEditor(ProcessorEditor* p) :
	ProcessorEditorBody(p),
	currentSequence("Current Sequence"),
	currentTrack("Current Track"),
	playButton("Start", this, factory),
	stopButton("Stop", this, factory),
	recordButton("Record", this, factory),
	dropper(dynamic_cast<MidiPlayer*>(getProcessor())),
	loopButton("Loop Enabled")
{
	dynamic_cast<MidiPlayer*>(getProcessor())->addSequenceListener(this);

	addAndMakeVisible(typeSelector);
	p->getProcessor()->getMainController()->skin(typeSelector);

	auto availableOverlays = MidiOverlayFactory::getInstance().getIdList();
	int overlayIndex = 1;

	for (auto overlay : availableOverlays)
		typeSelector.addItem(overlay.toString(), overlayIndex++);

	typeSelector.addListener(this);
	typeSelector.setTextWhenNothingSelected("Set Player type");

	

	addAndMakeVisible(dropper);

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
	currentTrack.setup(getProcessor(), MidiPlayer::CurrentTrack, "Track");
	currentTrack.setTextWhenNoChoicesAvailable("No tracks");
	currentTrack.setTextWhenNothingSelected("No tracks");

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
	currentSequence.setup(getProcessor(), MidiPlayer::CurrentSequence, "Current Sequence");
	currentSequence.setTextWhenNoChoicesAvailable("Nothing loaded");
	currentSequence.setTextWhenNothingSelected("Nothing loaded");

	addAndMakeVisible(loopButton);
	loopButton.setup(getProcessor(), MidiPlayer::LoopEnabled, "Loop Enabled");

	startTimer(50);

	typeSelector.setSelectedItemIndex(1, sendNotificationAsync);
}

MidiPlayerEditor::~MidiPlayerEditor()
{
	if (auto fp = dynamic_cast<MidiPlayer*>(getProcessor()))
		fp->removeSequenceListener(this);
}

void MidiPlayerEditor::timerCallback()
{
	{
		// Update the position

		double currentPos = getProcessor()->getAttribute(MidiPlayer::CurrentPosition);

		if (currentPosition.getValue() != currentPos)
			currentPosition.setValue(currentPos, dontSendNotification);
	}

	auto mp = dynamic_cast<MidiPlayer*>(getProcessor());

	{
		// Update the track amount
		auto thisSequence = mp->getCurrentSequence();

		int newTrackAmount = 0;

		if (thisSequence != nullptr)
			newTrackAmount = thisSequence->getNumTracks();

		if (newTrackAmount != currentTrackAmount)
		{
			currentTrackAmount = newTrackAmount;
			currentTrack.clear(dontSendNotification);

			for (int i = 0; i < currentTrackAmount; i++)
				currentTrack.addItem("Track" + String(i + 1), i + 1);

			currentTrack.setSelectedId((int)getProcessor()->getAttribute(MidiPlayer::CurrentTrack), dontSendNotification);
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
				auto name = mp->getSequenceId(i).toString();
				if (name.isEmpty())
					name = "Sequence " + String(i + 1);

				currentSequence.addItem(name, i + 1);
			}

			currentSequence.setSelectedId((int)mp->getAttribute(MidiPlayer::CurrentSequence), dontSendNotification);
		}
	}
}

void MidiPlayerEditor::updateGui()
{
	updateLabel();
	currentSequence.updateValue(dontSendNotification);
	currentTrack.updateValue(dontSendNotification);
	loopButton.updateValue(dontSendNotification);
}

void MidiPlayerEditor::buttonClicked(Button* b)
{
	auto pl = dynamic_cast<MidiPlayer*>(getProcessor());

	if (b == &clearButton)
		pl->clearSequences();
	else if (b == &playButton)
		pl->play(0);
	else if (b == &stopButton)
		pl->stop(0);
	else if (b == &recordButton)
		pl->record(0);
}

void MidiPlayerEditor::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{

}

void MidiPlayerEditor::comboBoxChanged(ComboBox* c)
{
	if (c == &typeSelector)
	{
		auto type = Identifier(c->getText());
		auto mp = dynamic_cast<MidiPlayer*>(getProcessor());
		setNewPlayerType(MidiOverlayFactory::getInstance().create(type, mp));
	}
}

void MidiPlayerEditor::resized()
{
	auto ar = getLocalBounds();



	if (currentPlayerType != nullptr)
	{
		auto typeBounds = ar.removeFromBottom(currentPlayerType->getPreferredHeight() + margin);
		dynamic_cast<Component*>(currentPlayerType.get())->setBounds(typeBounds.reduced(margin));
	}

	dropper.setBounds(ar.removeFromBottom(dropper.getPreferredHeight()).reduced(margin));

	typeSelector.setBounds(ar.removeFromLeft(128 + 2 * margin).reduced(margin));

	ar.removeFromLeft(margin);

	currentSequence.setBounds(ar.removeFromLeft(128 + 2 * margin).reduced(margin));
	currentTrack.setBounds(ar.removeFromLeft(128 + 2 * margin).reduced(margin));

	clearButton.setBounds(ar.removeFromLeft(80 + 2 * margin).reduced(margin));
	loopButton.setBounds(ar.removeFromLeft(80 + 2 * margin).reduced(margin));


	playButton.setBounds(ar.removeFromLeft(32 + 2 * margin).reduced(margin));
	stopButton.setBounds(ar.removeFromLeft(32 + 2 * margin).reduced(margin));
	recordButton.setBounds(ar.removeFromLeft(32 + 2 * margin).reduced(margin));

	currentPosition.setBounds(ar.reduced(margin));
}






}