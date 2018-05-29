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

namespace hise { using namespace juce;

//==============================================================================
Plotter::Plotter()
{
	setName("Plotter");

	setColour(backgroundColour, Colours::transparentBlack);

	setColour(pathColour, Colour(0x88ffffff));
	setColour(pathColour2, Colour(0x44ffffff));


	displayBuffer.setSize(1, 44100);
	
    
	setSize(380, 200);

	startTimer(30);

}

Plotter::~Plotter()
{
	
	
};

void Plotter::paint (Graphics& g)
{
	Colour background = findColour(backgroundColour);

	if (!background.isTransparent())
		g.fillAll(background);

	Path drawPath;

	const float samplesPerPixel = (float)displayBuffer.getNumSamples() / (float)getWidth();

	drawPath.startNewSubPath(0.0f, (float)getHeight());

	int samplePos = 0;

	for(int i = 0; i< getWidth(); i+=2)
	{
		samplePos = roundDoubleToInt(i * samplesPerPixel);
		samplePos = (position + samplePos) % displayBuffer.getNumSamples();

		int numToSearch = jmin<int>(roundFloatToInt(samplesPerPixel*2), displayBuffer.getNumSamples() - samplePos);
		float thisValue = FloatVectorOperations::findMaximum(displayBuffer.getReadPointer(0, samplePos), numToSearch);

		thisValue = jlimit<float>(0.0f, 1.0f, thisValue);
		drawPath.lineTo((float)i, (float)getHeight() - thisValue * (float)getHeight());
	}
    
	drawPath.lineTo((float)getWidth(), (float)getHeight());
	drawPath.closeSubPath();

	g.setGradientFill(ColourGradient(findColour(pathColour),
			0.0f, 0.0f,
			findColour(pathColour2),
			0.0f, (float)getHeight(),
			false));

	g.fillPath(drawPath);
}

void Plotter::resized()
{
};



void Plotter::addValues(const AudioSampleBuffer& b, int startSample, int numSamples)
{
	SpinLock::ScopedLockType sl(swapLock);

	const bool wrap = position + numSamples > displayBuffer.getNumSamples();

	if (wrap)
	{
		const int numBeforeWrap = displayBuffer.getNumSamples() - position;

		FloatVectorOperations::copy(displayBuffer.getWritePointer(0, position), b.getReadPointer(0, startSample), numBeforeWrap);

		const int numAfterWrap = numSamples - numBeforeWrap;

		position = 0;

		FloatVectorOperations::copy(displayBuffer.getWritePointer(0, position), b.getReadPointer(0, startSample + numBeforeWrap), numAfterWrap);

	}
	else
	{
		FloatVectorOperations::copy(displayBuffer.getWritePointer(0, position), b.getReadPointer(0, startSample), numSamples);

		position += numSamples;
	}
}

void Plotter::timerCallback()
{
	repaint();
}

void Plotter::mouseDown(const MouseEvent& /*m*/)
{
	PopupLookAndFeel plaf;
	PopupMenu menu;
	menu.setLookAndFeel(&plaf);


	menu.addItem(1024, "Freeze", true, !active);
	menu.addItem(1, "1 Second", true, displayBuffer.getNumSamples() == 44100);
	menu.addItem(2, "2 Seconds", true, displayBuffer.getNumSamples() == 2*44100);
	menu.addItem(4, "4 Seconds", true, displayBuffer.getNumSamples() == 4*44100);

	int result = menu.show();

	if (result == 1024)
	{
		if (active)
		{
			active = false;
			stopTimer();
		}
		else
		{
			active = true;
			startTimer(30);
		}
	}
	else if (result > 0)
	{
		AudioSampleBuffer newDisplayBuffer(1, result * 44100);
		newDisplayBuffer.clear();

		{
			SpinLock::ScopedLockType sl(swapLock);
			position = 0;
			displayBuffer.setSize(1, result * 44100);
			displayBuffer.clear();
		}
	}
}

} // namespace hise