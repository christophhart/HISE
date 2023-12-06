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


#pragma once

namespace hise {
using namespace juce;

class MidiPlayerEditor : public ProcessorEditorBody,
	public ComboBoxListener,
	public ButtonListener,
	public Timer,
	public MidiPlayer::SequenceListener
{
public:

	static constexpr int margin = 10;

	

	MidiPlayerEditor(ProcessorEditor* p);;
	~MidiPlayerEditor();

	void timerCallback() override;

	void updateGui() override;

	void buttonClicked(Button* b) override;

	void sequenceLoaded(HiseMidiSequence::Ptr newSequence) override;

	void sequencesCleared() override
	{
		currentSequence.clear(dontSendNotification);
		currentTrack.clear(dontSendNotification);
	}

	void comboBoxChanged(ComboBox* c) override;

	int getBodyHeight() const override
	{
		return 32 + 2 * margin + dropper.getPreferredHeight() + (currentPlayerType != nullptr ? (currentPlayerType->getPreferredHeight() + margin) : 0);
	}

	void resized() override;

	void setNewPlayerType(MidiPlayerBaseType* newType)
	{
		currentPlayerType = newType;

		if (currentPlayerType != nullptr)
			addAndMakeVisible(dynamic_cast<Component*>(currentPlayerType.get()));

		refreshBodySize();
		resized();
	}

private:

	ProcessorEditorBodyUpdater updater;

	void updateLabel()
	{
		auto currentState = dynamic_cast<MidiPlayer*>(getProcessor())->getPlayState();

		if (currentState != buttonPlayState)
		{
			buttonPlayState = currentState;

			Colour onColour = Colours::white;
			Colour offColour = Colours::white.withAlpha(0.5f);

			bool isPlay = buttonPlayState == MidiPlayer::PlayState::Play;
			bool isStop = buttonPlayState == MidiPlayer::PlayState::Stop;
			bool isRecord = buttonPlayState == MidiPlayer::PlayState::Record;

			playButton.setColours((isPlay ? onColour : offColour).withMultipliedAlpha(0.8f),
				isPlay ? onColour : offColour,
				isPlay ? onColour : offColour);

			stopButton.setColours((isStop ? onColour : offColour).withMultipliedAlpha(0.8f),
				isStop ? onColour : offColour,
				isStop ? onColour : offColour);

			recordButton.setColours((isRecord ? onColour : offColour).withMultipliedAlpha(0.8f),
				isRecord ? onColour : offColour,
				isRecord ? onColour : offColour);

			playButton.repaint();
			stopButton.repaint();
			recordButton.repaint();
		}
	}

	MidiPlayerBaseType::TransportPaths factory;

	ComboBox typeSelector;

	Slider currentPosition;

	HiseShapeButton playButton;
	HiseShapeButton stopButton;
	HiseShapeButton recordButton;

	MidiPlayer::PlayState buttonPlayState = MidiPlayer::PlayState::Stop;
	int currentTrackAmount = 0;

	ScopedPointer<MidiPlayerBaseType> currentPlayerType;
	MidiFileDragAndDropper dropper;

	HiComboBox currentSequence;
	HiComboBox currentTrack;
	ToggleButton clearButton;
	HiToggleButton loopButton;
};

}