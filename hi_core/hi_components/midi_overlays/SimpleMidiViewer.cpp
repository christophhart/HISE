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



SimpleMidiViewer::SimpleMidiViewer(MidiPlayer* player) :
	MidiPlayerBaseType(player)
{
	setColour(HiseColourScheme::ComponentOutlineColourId, Colours::black.withAlpha(0.5f));
	setColour(HiseColourScheme::ComponentFillTopColourId, Colours::black.withAlpha(0.6f));
	setColour(HiseColourScheme::ComponentFillBottomColourId, Colours::white);
	setColour(HiseColourScheme::ComponentBackgroundColour, Colours::black.withAlpha(0.1f));

	startTimer(50);
}

void SimpleMidiViewer::timerCallback()
{
	repaint();
}

void SimpleMidiViewer::resized()
{
	rebuildRectangles();
}

void SimpleMidiViewer::paint(Graphics& g)
{
	g.setColour(findColour(HiseColourScheme::ComponentBackgroundColour));
	g.fillAll();

	for (auto r : currentRectangles)
	{
		g.setColour(findColour(HiseColourScheme::ComponentFillTopColourId));
		g.fillRect(r);
		g.setColour(findColour(HiseColourScheme::ComponentOutlineColourId));
		g.drawRect(r, 1.0f);
	}

	double posToUse = currentSeekPosition;

	if (posToUse == -1)
		posToUse = getPlayer()->getPlaybackPosition();

	int posX = roundToInt(posToUse * (double)getWidth());

	g.setColour(findColour(HiseColourScheme::ComponentFillBottomColourId));
	g.drawVerticalLine(posX, 0.0f, (float)getHeight());
}

void SimpleMidiViewer::rebuildRectangles()
{
	if (auto seq = getPlayer()->getCurrentSequence())
	{
		currentRectangles = seq->getRectangleList(getLocalBounds().toFloat());

	}
	else
		currentRectangles = {};

	repaint();
}

void SimpleMidiViewer::mouseDown(const MouseEvent& e)
{
	resume = getPlayer()->getPlayState() == MidiPlayer::PlayState::Play;

	getPlayer()->stop();
	updateSeekPosition(e);
}

void SimpleMidiViewer::mouseDrag(const MouseEvent& e)
{
	updateSeekPosition(e);
}

void SimpleMidiViewer::mouseUp(const MouseEvent& e)
{
	updateSeekPosition(e);
	

	if (resume)
	{
		getPlayer()->play();
		getPlayer()->setAttribute(MidiPlayer::CurrentPosition, (float)currentSeekPosition, sendNotification);
	}
	
	currentSeekPosition = -1.0;
}

void SimpleMidiViewer::updateSeekPosition(const MouseEvent& e)
{
	currentSeekPosition = (float)e.getPosition().getX() / (float)getWidth();
	repaint();
}

SimpleCCViewer::SimpleCCViewer(MidiPlayer* player_) :
	MidiPlayerBaseType(player_),
	SimpleTimer(player_->getMainController()->getGlobalUIUpdater()),
	noteDisplay(player_)
{
	addAndMakeVisible(noteDisplay);
	rebuildCCValues();
}

void SimpleCCViewer::mouseDown(const MouseEvent& e)
{
	PopupMenu m;

	int i = 1;

	m.addSectionHeader("Add MIDI CC lane");
	m.addSeparator();

	for (auto e : availableTables)
	{
		m.addItem(i++, "CC #" + String(e->ccNumber), true, isShown(e));
	}

	auto result = m.show() - 1;

	if (result != -1)
	{
		auto t = availableTables[result];

		if (isShown(t))
		{
			for (int i = 0; i < activeEditors.size(); i++)
			{
				if (activeEditors[i]->getEditedTable() == &t->ccTable)
				{
					activeEditors.remove(i);
					break;
				}
			}
		}
		else
		{
			auto newEditor = new TableEditor(getPlayer()->getMainController()->getControlUndoManager(), &t->ccTable);
			addAndMakeVisible(newEditor);
			activeEditors.add(newEditor);
		}

		resized();
	}
}

void SimpleCCViewer::paint(Graphics& g)
{
	auto b = getLocalBounds().removeFromLeft(30);

	if (activeEditors.isEmpty())
	{
		g.setColour(Colours::black.withAlpha(0.4f));

		g.fillRect(b.reduced(2));
	}

	for (auto e : activeEditors)
	{
		auto r = b.removeFromTop(e->getHeight()).reduced(1).toFloat();

		g.setColour(Colours::black.withAlpha(0.4f));
		g.drawRect(r, 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("#" + String(getCC(e)), r, Justification::centred);
	}
}

int SimpleCCViewer::getCC(TableEditor* te)
{
	auto table = te->getEditedTable();

	for (auto a : availableTables)
		if (table == &a->ccTable)
			return a->ccNumber;

	jassertfalse;
	return -1;
}

void SimpleCCViewer::resized()
{
	auto b = getLocalBounds();

	b.removeFromLeft(30);

	noteDisplay.setBounds(b);

	if (!activeEditors.isEmpty())
	{
		auto h = b.getHeight() / activeEditors.size();

		for (auto e : activeEditors)
			e->setBounds(b.removeFromTop(h));
	}

	repaint();
}

void SimpleCCViewer::rebuildCCValues()
{
	if (auto seq = getPlayer()->getCurrentSequence())
	{
		auto l = seq->getEventList(44100.0, 120, HiseMidiSequence::TimestampEditFormat::Ticks);

		for (auto t : availableTables)
		{
			t->ccTable.reset();
			t->ccTable.setTablePoint(1, 1.0, 0.0, 0.5);
		}

		for (const auto& e : l)
		{
			if (e.isController())
			{
				auto t = getTableForCC(e.getControllerNumber());
				auto tsQuarter = (double)e.getTimeStamp() / (double)HiseMidiSequence::TicksPerQuarter;
				auto lengthNormalised = tsQuarter / seq->getLengthInQuarters();

				t->ccTable.addTablePoint(lengthNormalised, e.getControllerValue() / 127.0);
			}
		}
	}
}

bool SimpleCCViewer::isShown(CCTable::Ptr t) const
{
	for (auto e : activeEditors)
		if (e->getEditedTable() == &t->ccTable)
			return true;

	return false;
}

hise::SimpleCCViewer::CCTable::Ptr SimpleCCViewer::getTableForCC(int ccNumber)
{
	for (auto t : availableTables)
		if (t->ccNumber == ccNumber)
			return t;

	CCTable::Ptr nt = new CCTable();
	nt->ccTable.setTablePoint(1, 1.0, 0.0, 0.5);
	nt->ccNumber = ccNumber;

	

	availableTables.add(nt);
	return nt;
}

}