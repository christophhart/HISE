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


/** A read only MIDI file content display. */
class SimpleMidiViewer : public Component,
	public MidiPlayerBaseType,
	public Timer
{
public:

	ENABLE_OVERLAY_FACTORY(SimpleMidiViewer, "Midi Viewer");

	SimpleMidiViewer(MidiPlayer* player);;

	void timerCallback() override;

	void sequenceLoaded(HiseMidiSequence::Ptr) override 
	{
		rebuildRectangles();
	};

	void resized() override;

	int getPreferredHeight() const override { return 80; }

	void sequencesCleared() override
	{
		rebuildRectangles();
	};

	void paint(Graphics& g) override;

	void rebuildRectangles();

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

private:

	void updateSeekPosition(const MouseEvent& e);

	double currentSeekPosition = -1.0;
	bool resume = false;

	RectangleList<float> currentRectangles;
};


class SimpleCCViewer : public Component,
					   public MidiPlayerBaseType,
					   public PooledUIUpdater::SimpleTimer
{
public:

	SimpleCCViewer(MidiPlayer* player_);;

	ENABLE_OVERLAY_FACTORY(SimpleCCViewer, "CC Viewer");

	int getPreferredHeight() const override { return 200; }

	void sequenceLoaded(HiseMidiSequence::Ptr) override
	{
		rebuildCCValues();
	};

	void mouseDown(const MouseEvent& e) override;

	void sequencesCleared() override
	{
		rebuildCCValues();
	};


	void paint(Graphics& g) override;

	

	int getCC(TableEditor* te);

	void resized() override;

	void timerCallback() override
	{

	}

	void rebuildCCValues();

	SimpleMidiViewer noteDisplay;

	struct CCTable: public ReferenceCountedObject
	{
		using List = ReferenceCountedArray<CCTable>;
		using Ptr = ReferenceCountedObjectPtr<CCTable>;

		int ccNumber;
		SampleLookupTable ccTable;
	};

	bool isShown(CCTable::Ptr t) const;

	CCTable::Ptr getTableForCC(int ccNumber);

	CCTable::List availableTables;

	OwnedArray<TableEditor> activeEditors;
};
					  

}