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
Plotter::Plotter(PooledUIUpdater* updater):
	yConverter(Table::getDefaultTextValue)
{
	setSpecialLookAndFeel(new GlobalHiseLookAndFeel(), true);

	rb = new SimpleRingBuffer();

	rb->getUpdater().addEventListener(rb.get());
	rb->getUpdater().addEventListener(this);

	rb->getUpdater().setUpdater(updater);

	{
		SimpleReadWriteLock::ScopedWriteLock sl(rb->getDataLock());
		rb->setRingBufferSize(1, plotterDefaultSize, false);
	}
	
	setFont(GLOBAL_BOLD_FONT());

	setName("Plotter");
	setColour(backgroundColour, Colours::transparentBlack);
	setColour(pathColour, Colour(0x88ffffff));
	setColour(pathColour2, Colour(0x44ffffff));
	setColour(textColour, Colours::white);

	setSize(380, 200);
}

Plotter::~Plotter()
{
	if(cleanupFunction)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(rb->getDataLock());
		cleanupFunction(this);
	}
	
	SimpleReadWriteLock::ScopedWriteLock sl(deleteLock);
	rb = nullptr;
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
	auto slaf = dynamic_cast<RingBufferComponentBase::LookAndFeelMethods*>(&getLookAndFeel());

	if(slaf != nullptr)
	{
		slaf->drawOscilloscopeBackground(g, *this, getLocalBounds().toFloat());
	}
	else
	{
		Colour background = findColour(backgroundColour);

		if (!background.isTransparent())
			g.fillAll(background);
	}

	auto tc = findColour(textColour);

	if(!tc.isTransparent())
	{
		g.setColour(tc);

		auto topText = yConverter(1.0f);

		float bottomValue = 0.0f;

		if (currentMode == Plotter::PitchMode || currentMode == Plotter::PanMode)
			bottomValue = -1.0f;

		auto bottomText = yConverter(bottomValue);

		g.setFont(font);

		g.drawText(topText, getLocalBounds(), Justification::topRight);
		g.drawText(bottomText, getLocalBounds(), Justification::bottomRight);

		if (currentMode != Plotter::GainMode)
		{
			g.drawHorizontalLine(getHeight() / 2, 0.0f, (float)getWidth());
		}
	}

	if(slaf != nullptr)
	{
		slaf->drawOscilloscopePath(g, *this, drawPath);
	}
	else
	{
		g.setGradientFill(ColourGradient(findColour(pathColour),
		0.0f, 0.0f,
		findColour(pathColour2),
		0.0f, (float)getHeight(),
		false));

		g.fillPath(drawPath);
	}

	if (!popupPosition.isOrigin() && !tc.isTransparent())
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
	SimpleReadWriteLock::ScopedReadLock sl(deleteLock);

	const float* d[1] = { buffer + startSample };
	rb->write(d, 1, numSamples);
}

void Plotter::rebuildPath()
{
	if(!active)
		return;

	drawPath = rb->getPropertyObject()->createPath({0, rb->getReadBuffer().getNumSamples()}, {0.0, 1.0}, getLocalBounds().toFloat(), 0.0f);

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
			active = !active;
		}
		else if (result > 0)
		{
			rb->setRingBufferSize(1, result * plotterDefaultSize, true);
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