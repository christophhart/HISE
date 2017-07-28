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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

//==============================================================================
Plotter::Plotter() :
freeModePlotterQueue(nullptr),
freeMode(false)
{
	setName("Modulator Data Plotter: Idle");

	setColour(backgroundColour, Colours::transparentBlack);

	setColour(pathColour, Colour(0x88ffffff));
	setColour(pathColour2, Colour(0x11ffffff));

	resetPlotter();

    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

	addAndMakeVisible (speedSlider = new Slider ("new slider"));
    speedSlider->setRange (10, 1024, 0);
    speedSlider->setSliderStyle (Slider::LinearBar);
	speedSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    speedSlider->addListener (this);
	speedSlider->setVisible(false);

	setSize(380, 200);

}

Plotter::~Plotter()
{
	speedSlider = nullptr;
	
};

void Plotter::addPlottedModulator(Modulator *m)
{
	modQueue.add(new PlotterQueue(m));

	m->setPlotter(this);

	if(getParentComponent() != nullptr) getParentComponent()->repaint();
}

void Plotter::removePlottedModulator(Modulator *m)
{
	m->setPlotter(nullptr);
	m->sendChangeMessage();

	for(int i = 0; i < modQueue.size(); i++)
	{
		if(modQueue[i]->attachedMod == m) 
		{
			modQueue.remove(i);
			break;
		}
	}

	

	if(modQueue.size() == 0)
	{
		setName("Modulator Data Plotter: Idle");
		
	}
	else
	{
		setName("Modulator Data Plotter: " + modQueue.getLast()->attachedMod->getId());
	}
}

void Plotter::handleAsyncUpdate()
{
	for (int i = 0; i < currentQueuePosition; i++)
	{
		currentRingBufferPosition = (currentRingBufferPosition + 1) % 1024;

		float value = 0.0f;

		if (freeMode)
		{
			value = freeModePlotterQueue.data[i];
		}
		else
		{
			for (int j = 0; j < modQueue.size(); j++)
			{
				value += modQueue[j]->data[i];
			}
		}

		internalBuffer[currentRingBufferPosition] = value;

	}

	currentQueuePosition = 0;

	if (freeMode)
	{
		freeModePlotterQueue.pos = 0;
	}
	else
	{
		for (int i = 0; i < modQueue.size(); i++)
		{
			modQueue.getUnchecked(i)->pos = 0;
		}
	}
	
	repaint();
};

void Plotter::addValue(const Modulator *m, float addedValue)
{

	for(int i = 0; i < modQueue.size(); i++)
	{
		if(modQueue[i]->attachedMod == m)
		{
			modQueue[i]->addValue(addedValue);

			if(i == modQueue.size() -1) 
			{
				currentQueuePosition++;
				triggerAsyncUpdate();
			}

		}
		
	}	
};

void Plotter::addValue(float addedValue)
{
	freeModePlotterQueue.addValue(addedValue);
	freeModePlotterQueue.addValue(addedValue);
	currentQueuePosition++;
	currentQueuePosition++;
	freeModePlotterQueue.addValue(addedValue);
	freeModePlotterQueue.addValue(addedValue);
	currentQueuePosition++;
	currentQueuePosition++;
	triggerAsyncUpdate();
}

void Plotter::paint (Graphics& g)
{

	if (freeMode)
	{
		g.fillAll(Colour(0xFF161616));
	}
	else
	{
		Colour background = findColour(backgroundColour);

		if (!background.isTransparent()) g.fillAll((modQueue.size() == 0 || freeMode) ? background.withAlpha(0.7f) : background);
	}

	Path drawPath;

	const float factor = 1024.0f / (float)getWidth();

	drawPath.startNewSubPath(0.0f, (float)getHeight());
	for(int i = 0; i< 1024; i++)
	{
		int pos = (i + currentRingBufferPosition) % 1024;

		const float thisValue = jlimit<float>(0.0f, 1.0f, internalBuffer[pos]);

		drawPath.lineTo(i/factor, getHeight() - thisValue * getHeight());
	}
    
	drawPath.lineTo((float)getWidth(), (float)getHeight());

	if (freeMode)
	{
		g.setColour(Colours::red.withBrightness(0.6f));
			
		g.fillPath(drawPath);
	}
	else
	{
		//KnobLookAndFeel::fillPathHiStyle(g, drawPath, getWidth(), getHeight(), false);

		g.setGradientFill(ColourGradient(findColour(pathColour),
			0.0f, 0.0f,
			findColour(pathColour2),
			0.0f, (float)getHeight(),
			false));

		g.fillPath(drawPath);

		//DropShadow d(Colours::white.withAlpha(drawBorders ? 0.2f : 0.1f), 5, Point<int>());

		//d.drawForPath(g, p);
	}

	

	
	
}

void Plotter::resized()
{
	

	//if(getWidth() != ) setSize(512, getHeight());
	// It must be 512 pixels wide to make it work correctly...
	//jassert(getWidth() == 512);
	
	speedSlider->setBounds(0, getHeight() - 16, getWidth(), 16);
    
}

void Plotter::resetPlotter()
{
	//cancelPendingUpdate();
	currentQueuePosition = 0;
	currentRingBufferPosition = 0;
		
	smoothedValue = 0.0f;
	smoothIndex = 0;
	smoothLimit = 2;

	for(int i = 0; i<1024; i++) internalBuffer[i] = 0.0f;
	
	
	for(int j = 0; j < modQueue.size(); j++)
	{
		modQueue[j]->reset();
	}

	repaint();
};

void Plotter::mouseDown(const MouseEvent &e)
{
	if (e.mods.isRightButtonDown() && dynamic_cast<ScriptContentComponent*>(getParentComponent()) == nullptr)
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		enum 
		{
			RemovePlotter = 1
		};

		m.addItem(RemovePlotter, "Remove Modulator From Plotter", modQueue.size() != 0);

		const int result = m.show();

		if (result == RemovePlotter)
		{
			
			while (modQueue.size() != 0)
			{
				removePlottedModulator(modQueue[0]->attachedMod.get());
			}
		}
	}
}

void Plotter::setFreeMode(bool shouldUseFreeMode)
{
	freeMode = shouldUseFreeMode;

	setOpaque(shouldUseFreeMode);

	freeModePlotterQueue = PlotterQueue(nullptr);
}
