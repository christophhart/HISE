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



SimpleMidiViewer::SimpleMidiViewer(MidiFilePlayer* player) :
	MidiFilePlayerBaseType(player)
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

	int posX = roundDoubleToInt(getPlayer()->getPlaybackPosition() * (double)getWidth());

	g.setColour(findColour(HiseColourScheme::ComponentFillBottomColourId));
	g.drawVerticalLine(posX, 0.0f, (float)getHeight());
}

void SimpleMidiViewer::rebuildRectangles()
{
	if (auto seq = getPlayer()->getCurrentSequence())
	{
		currentRectangles = seq->getRectangleList(getBounds().toFloat());
		repaint();
	}
}

void SimpleMidiViewer::mouseDown(const MouseEvent& e)
{
	auto position = (float)e.getPosition().getX() / (float)getWidth();

	getPlayer()->setAttribute(MidiFilePlayer::CurrentPosition, position, sendNotification);
}

}