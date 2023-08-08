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



class MidiLooper : public Component,
				   public MidiPlayerBaseType,
				   public Timer,
				   public ComboBox::Listener,
				   public Button::Listener
{
public:

	ENABLE_OVERLAY_FACTORY(MidiLooper, "Looper");

	MidiLooper(MidiPlayer* p);;

	void sequenceLoaded(HiseMidiSequence::Ptr newSequence) override;

	void sequencesCleared() override;

	int getPreferredHeight() const override;

	void comboBoxChanged(ComboBox* ) override;
	void buttonClicked(Button* b) override;
	void paint(Graphics& g) override;
	void timerCallback() override;
	void resized() override;

private:

	BlackTextButtonLookAndFeel blaf;

	int loopUuid = 1;
	ComboBox loopLength;
	TextButton clearButton;
	TextButton addButton;
	TextButton undoButton;

	int lastPos;
};

}