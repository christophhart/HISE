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

constexpr int plotterDefaultSize = 44100 / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

//==============================================================================
Plotter::Plotter():
	abstractFifo(8192),
	yConverter(Table::getDefaultTextValue)
{

	setFont(GLOBAL_BOLD_FONT());

	memset(tempBuffer, 0, sizeof(float) * 8192);

	setName("Plotter");

	setColour(backgroundColour, Colours::transparentBlack);

	setColour(pathColour, Colour(0x88ffffff));
	setColour(pathColour2, Colour(0x44ffffff));

	setColour(textColour, Colours::white);

	displayBuffer.setSize(1, plotterDefaultSize);
	displayBuffer.clear();
    
	setSize(380, 200);

	startTimer(30);

}

Plotter::~Plotter()
{
	
	
};

float getAverage(const float* data, int numSamples, Plotter::Mode m)
{
	if (numSamples == 0)
		return 0.0f;

	float sum = 0.0f;

	for (int i = 0; i < numSamples; i++)
	{
		sum += data[i];
	}

	float thisValue = sum / (float)numSamples;

	if (m == Plotter::PitchMode)
	{
		thisValue = std::log2(thisValue);
		thisValue = (thisValue + 1.0f) / 2.0f;
	}
	else if (m == Plotter::PanMode)
	{
		thisValue = (thisValue + 1.0f) / 2.0f;
	}

	thisValue = jlimit<float>(0.0f, 1.0f, thisValue);
	thisValue = FloatSanitizers::sanitizeFloatNumber(thisValue);

	return thisValue;
}

void Plotter::paint (Graphics& g)
{
	Colour background = findColour(backgroundColour);

	if (!background.isTransparent())
		g.fillAll(background);

	g.setColour(findColour(textColour));

	auto topText = yConverter(1.0f);

	float bottomValue = 0.0f;

	if (currentMode == Plotter::PitchMode || currentMode == Plotter::PanMode)
	{
		bottomValue = -1.0f;
	}

	auto bottomText = yConverter(bottomValue);

	g.setFont(font);

	g.drawText(topText, getLocalBounds(), Justification::topRight);
	g.drawText(bottomText, getLocalBounds(), Justification::bottomRight);

	if (currentMode != Plotter::GainMode)
	{
		g.drawHorizontalLine(getHeight() / 2, 0.0f, (float)getWidth());
	}

	g.setGradientFill(ColourGradient(findColour(pathColour),
		0.0f, 0.0f,
		findColour(pathColour2),
		0.0f, (float)getHeight(),
		false));

	g.fillPath(drawPath);

	if (!popupPosition.isOrigin())
	{
		Font f = font;

		float yValue = (float)popupPosition.getY() / (float)getHeight();

		if (currentMode != Plotter::GainMode)
			yValue = jmap(yValue, 0.0f, 1.0f, 1.0f, -1.0f);
		else
			yValue = jmap(yValue, 0.0f, 1.0f, 1.0f, 0.0f);

		auto value = yConverter(yValue);

		auto width = f.getStringWidth(value) + 20;
		auto height = (int)f.getHeight() + 4;

		int x = jlimit<int>(0, getWidth() - width, popupPosition.getX() - width / 2);
		int y = jlimit<int>(0, getHeight() - height, popupPosition.getY() - height - 10);

		Rectangle<int> bounds = { x, y, width, height};

		g.setColour(findColour(pathColour));
		g.fillRect(bounds);
		g.setColour(findColour(textColour));
		g.drawText(value, bounds, Justification::centred);

	}
	
}

void Plotter::resized()
{
};



void Plotter::pushLockFree(const float* buffer, int startSample, int numSamples)
{
	int start1, size1, start2, size2;
	abstractFifo.prepareToWrite(numSamples, start1, size1, start2, size2);

	if (size1 > 0)
	{
		FloatVectorOperations::copy(tempBuffer + start1, buffer + startSample, size1);
	}
	
	if (size2 > 0)
	{
		FloatVectorOperations::copy(tempBuffer + start2, buffer + startSample + start1, size2);
	}

	abstractFifo.finishedWrite(size1 + size2);
}

void Plotter::rebuildPath()
{
	drawPath.clear();

	int numAvailable = abstractFifo.getNumReady();

	if (numAvailable > 0)
	{
		float* temp = (float*)alloca(sizeof(float) * numAvailable);

		popLockFree(temp, numAvailable);
		addValues(temp, 0, numAvailable);
	}

	const float samplesPerPixel = (float)displayBuffer.getNumSamples() / (float)getWidth();


	drawPath.startNewSubPath(0.0f, (float)getHeight());

	int samplePos = 0;

	for (int i = 0; i < getWidth(); i += 2)
	{
		samplePos = roundToInt(i * samplesPerPixel);
		samplePos = (position + samplePos) % displayBuffer.getNumSamples();

		int numToSearch = jmin<int>(roundToInt(samplesPerPixel * 2), displayBuffer.getNumSamples() - samplePos);

		float thisValue = getAverage(displayBuffer.getReadPointer(0, samplePos), numToSearch, currentMode);

		drawPath.lineTo((float)i, (float)getHeight() - thisValue * (float)getHeight());
	}

	drawPath.lineTo((float)getWidth(), (float)getHeight());
	drawPath.closeSubPath();

}

void Plotter::popLockFree(float* destination, int numSamples)
{
	int start1, size1, start2, size2;
	abstractFifo.prepareToRead(numSamples, start1, size1, start2, size2);

	if (size1 > 0)
		FloatVectorOperations::copy(destination, tempBuffer + start1, size1);

	if (size2 > 0)
		FloatVectorOperations::copy(destination + size1, tempBuffer + start2, size2);

	abstractFifo.finishedRead(size1 + size2);
}

void Plotter::addValues(const float* buffer, int startSample, int numSamples)
{
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	
	
	const bool wrap = position + numSamples > displayBuffer.getNumSamples();

	if (wrap)
	{
		const int numBeforeWrap = displayBuffer.getNumSamples() - position;

		if(numBeforeWrap > 0)
			FloatVectorOperations::copy(displayBuffer.getWritePointer(0, position), buffer + startSample, numBeforeWrap);

		const int numAfterWrap = numSamples - numBeforeWrap;

		position = 0;

		if(numAfterWrap > 0)
			FloatVectorOperations::copy(displayBuffer.getWritePointer(0, position), buffer + startSample + numBeforeWrap, numAfterWrap);

	}
	else
	{
		if(numSamples > 0)
			FloatVectorOperations::copy(displayBuffer.getWritePointer(0, position), buffer + startSample, numSamples);

		position += numSamples;
	}
}

void Plotter::timerCallback()
{
	rebuildPath();
	repaint();
}

void Plotter::mouseDown(const MouseEvent& m)
{
	if (m.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu menu;
		menu.setLookAndFeel(&plaf);


		menu.addItem(1024, "Freeze", true, !active);
		menu.addItem(1, "1 Second", true, displayBuffer.getNumSamples() == plotterDefaultSize);
		menu.addItem(2, "2 Seconds", true, displayBuffer.getNumSamples() == 2 * plotterDefaultSize);
		menu.addItem(4, "4 Seconds", true, displayBuffer.getNumSamples() == 4 * plotterDefaultSize);

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
			AudioSampleBuffer newDisplayBuffer(1, result * plotterDefaultSize);
			newDisplayBuffer.clear();

			{
				position = 0;
				displayBuffer.setSize(1, result * plotterDefaultSize);
				displayBuffer.clear();
			}
		}
	}
	else
	{
		stickPopup = !stickPopup;
	}

	
}

void Plotter::mouseMove(const MouseEvent& m)
{
	if (!stickPopup)
	{
		popupPosition = m.getPosition();
		repaint();
	}
	
}

void Plotter::mouseExit(const MouseEvent& /*m*/)
{
	if (!stickPopup)
	{
		popupPosition = {};
	}
	
	repaint();
}

void Plotter::setMode(Mode m)
{
	currentMode = m;
}

void Plotter::setFont(const Font& f)
{
	font = f;
}

void Plotter::setYConverter(const Table::ValueTextConverter& newYConverter)
{
	yConverter = newYConverter;
}
} // namespace hise