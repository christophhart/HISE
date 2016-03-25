/*
  ==============================================================================

    Plotter.cpp
    Created: 23 Jan 2014 4:58:09pm
    Author:  Christoph

  ==============================================================================
*/

//==============================================================================
Plotter::Plotter(BaseDebugArea *area) :
AutoPopupDebugComponent(area),
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
	/*if(mod.get() != nullptr)
	{
		mod->setPlotter(nullptr);
		mod->sendChangeMessage();

	}*/

	modQueue.add(new PlotterQueue(m));

#if USE_BACKEND
	if(m != nullptr)
	{

		setName("Modulator Data Plotter: " + m->getId());

		showComponentInDebugArea(true);

		m->setPlotter(this);
	}
	else
	{
		setName("Modulator Data Plotter: Idle");

		
	}
#endif
	

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
		drawPath.lineTo(i/factor, getHeight() - internalBuffer[pos] * getHeight());
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
