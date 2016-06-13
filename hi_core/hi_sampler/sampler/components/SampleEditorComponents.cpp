// =================================================================================================================== SamplerSubEditor

void SamplerSubEditor::selectSounds(const Array<ModulatorSamplerSound*> &selection)
    {
        if(internalChange) return;

        internalChange = true;
        soundsSelected(selection);

        internalChange = false;
    }


// =================================================================================================================== PopupLabel

void PopupLabel::showPopup()
{
	PopupMenu p;
	ScopedPointer<PopupLookAndFeel> plaf = new PopupLookAndFeel();
	p.setLookAndFeel(plaf);

	for (int i = 0; i < options.size(); i++)
	{
		if (isTicked == 0)
		{
			p.addCustomItem(i + 1, new TooltipPopupComponent(options[i], toolTips[i], getWidth() - 4));
		}
		else
		{
			p.addItem(i + 1, options[i], true, isTicked[i]);
		}
	}

	int index = p.showAt(this);
		

	if(index != 0)
	{
		setItemIndex(index - 1);
	}
};

void PopupLabel::mouseDown(const MouseEvent &)
{
    ProcessorEditor *editor = findParentComponentOfClass<ProcessorEditor>();
    
    if(editor != nullptr)
    {
        PresetHandler::setChanged(editor->getProcessor());
    }
    
	if(isEnabled())
	{
		showPopup();
	}
}

void PopupLabel::setItemIndex(int index, NotificationType notify)
{
	index = jmax<int>(0, index);

	index = jmin<int>(index, options.size()-1);

	currentIndex = index;

	setText("", dontSendNotification);

	setText(options[index], notify);
};


// =================================================================================================================== SamplerSoundWaveform


// =================================================================================================================== SampleComponent

SampleComponent::SampleComponent(ModulatorSamplerSound *s, SamplerSoundMap *parentMap):
	sound(s),
	selected(false),
	transparency(0.3f),
	map(parentMap),
	enabled(true),
	visible(true)
{

	//setInterceptsMouseClicks(false, true);

	//addMouseListener(map, true);
    
	sound->addChangeListener(this);

	if (sound->isMissing() || sound->isPurged())
	{
		enabled = false;
	}
};

#pragma warning( push )
#pragma warning( disable: 4100 )

void SampleComponent::changeListenerCallback(SafeChangeBroadcaster *b)
{
	jassert(b == sound);
	map->updateSampleComponentWithSound(sound);

	SampleMapEditor *editor = map->findParentComponentOfClass<SampleMapEditor>();

	if (editor != nullptr)
	{
		editor->refreshRootNotes();
	}
}

#pragma warning( pop )

void SampleComponent::timerCallback()
{
	transparency *= 0.9f;
	if(transparency <= 0.3f)
	{
		stopTimer();
	}
}

void SampleComponent::drawSampleRectangle(Graphics &g, Rectangle<int> areaInt)
{
    if(sound.get() == nullptr) return;
    
    Rectangle<float> area((float)areaInt.getX(), (float)areaInt.getY(), (float)areaInt.getWidth(), (float)areaInt.getHeight());
    
    const int lowerXFade = sound->getProperty(ModulatorSamplerSound::LowerVelocityXFade);
    const int upperXFade = sound->getProperty(ModulatorSamplerSound::UpperVelocityXFade);
    
    if (lowerXFade != 0 || upperXFade != 0)
    {
        Range<int> velocityRange = sound->getVelocityRange();
        
        const float lowerCrossfadeValue = fabsf((float)lowerXFade) / (float)velocityRange.getLength();
        
        const float upperCrossfadeValue = fabsf((float)upperXFade) / (float)velocityRange.getLength();
        
        outline.clear();
        
        const float x = area.getX();
        const float y = area.getY();
        const float w = area.getWidth();
        const float h = area.getHeight();
        
        outline.startNewSubPath(x, y);
        outline.lineTo(x+w, y + upperCrossfadeValue * h);
        outline.lineTo(x+w, y + h);
        outline.lineTo(x, y + (1.0f - lowerCrossfadeValue) * h);
        
        outline.closeSubPath();
        
        g.setColour(getColourForSound(false));
        g.fillPath(outline);
        
        g.setColour(getColourForSound(true));
        
        g.drawLine(x, y, x+w, y + upperCrossfadeValue * h);
        g.drawVerticalLine(areaInt.getRight()-1, y + upperCrossfadeValue * h, y+h);
        g.drawLine(x+w, y+h, x, y+(1.0f - lowerCrossfadeValue) * h);
        
        g.drawVerticalLine(areaInt.getX(), y, y+(1.0f - lowerCrossfadeValue) * h);
        
        //		g.strokePath(outline, PathStrokeType(1.0f));
    }
    else
    {
        g.setColour(getColourForSound(false));
        g.fillRect(areaInt);
        
        g.setColour(getColourForSound(true));
        g.drawHorizontalLine(areaInt.getY(), area.getX(), area.getRight());
        g.drawHorizontalLine(areaInt.getBottom()-1, area.getX(), area.getRight());
        g.drawVerticalLine(areaInt.getX(), area.getY(), area.getBottom());
        g.drawVerticalLine(areaInt.getRight()-1, area.getY(), area.getBottom());
    }
}

bool SampleComponent::needsToBeDrawn()
{
	const bool enoughSiblings = numOverlayerSiblings > 4;

	
	return isVisible() && (isSelected() || !enoughSiblings);
}

// =================================================================================================================== SamplerSoundMap

SamplerSoundMap::SamplerSoundMap(ModulatorSampler *ownerSampler_, SamplerBody *b):
	body(b),
	ownerSampler(ownerSampler_),
	notePosition(-1),
	veloPosition(-1),
	selectedSounds(new SelectedItemSet<WeakReference<SampleComponent>>()),
	sampleLasso(new LassoComponent<WeakReference<SampleComponent>>())
{

	for (int i = 0; i < 128; i++)
	{
		pressedKeys[i] = -1;
	}

	selectedSounds->addChangeListener(this);

	addMouseListener(b, true);

	addChildComponent(sampleLasso);

	updateSoundData();

	setOpaque(true);

};

void SamplerSoundMap::changeListenerCallback(ChangeBroadcaster *b)
{
	if(b == selectedSounds)
	{
		body->getSelection().deselectAll();

		for(int i = 0; i < sampleComponents.size(); i++)
		{
			sampleComponents[i]->setSelected(false);
		}

		Array<WeakReference<SampleComponent>> selectedSampleComponents = selectedSounds->getItemArray();

		for(int i = 0; i < selectedSampleComponents.size(); i++)
		{
			if (selectedSampleComponents[i].get() != nullptr)
			{
				selectedSampleComponents[i]->setSelected(true);

				if (selectedSampleComponents[i]->getSound() != nullptr)
				{
					body->getSelection().addToSelection(selectedSampleComponents[i]->getSound());
				}
			}
		}

        refreshGraphics();
	}
	else if (dynamic_cast<ModulatorSamplerSound*>(b) != nullptr)
	{
		jassertfalse;
	}

}

void SamplerSoundMap::selectNeighbourSample(Neighbour direction)
{
	if(selectedSounds->getNumSelected() != 0)
	{
		const int lowKey = selectedSounds->getSelectedItem(0)->getSound()->getProperty(ModulatorSamplerSound::KeyLow);
		const int lowVelo = selectedSounds->getSelectedItem(0)->getSound()->getProperty(ModulatorSamplerSound::VeloLow);

		const int hiKey = selectedSounds->getSelectedItem(0)->getSound()->getProperty(ModulatorSamplerSound::KeyHigh);
		const int hiVelo = selectedSounds->getSelectedItem(0)->getSound()->getProperty(ModulatorSamplerSound::VeloHigh);

		const int group = selectedSounds->getSelectedItem(0)->getSound()->getProperty(ModulatorSamplerSound::RRGroup);

		for(int i = 0; i < sampleComponents.size(); i++)
		{
			const int thisLowKey = sampleComponents[i]->getSound()->getProperty(ModulatorSamplerSound::KeyLow);
			const int thisLowVelo = sampleComponents[i]->getSound()->getProperty(ModulatorSamplerSound::VeloLow);

			const int thisHiKey = sampleComponents[i]->getSound()->getProperty(ModulatorSamplerSound::KeyHigh);
			const int thisHiVelo = sampleComponents[i]->getSound()->getProperty(ModulatorSamplerSound::VeloHigh);

			const int thisGroup = sampleComponents[i]->getSound()->getProperty(ModulatorSamplerSound::RRGroup);

			if(thisGroup != group) continue;

			if((direction == Left || direction == Right) &&
			   thisHiVelo != hiVelo && thisLowVelo != lowVelo) continue;

			if((direction == Up || direction == Down) &&
			   thisHiKey != hiKey && thisLowKey != lowKey) continue;


			bool selectThisComponent = false;

			switch(direction)
			{
			case Left:	selectThisComponent = lowKey - thisHiKey == 1; break;
			case Right:	selectThisComponent = thisLowKey - hiKey == 1; break;
			case Up:	selectThisComponent = thisLowVelo - hiVelo == 1; break;
			case Down:	selectThisComponent = lowVelo - thisHiVelo == 1; break;
			}

			if(selectThisComponent)
			{
				body->getSelection().deselectAll();
				body->getSelection().addToSelection(sampleComponents[i]->getSound());
			}
		}
	}

    refreshGraphics();
}



void SamplerSoundMap::endSampleDragging(bool copyDraggedSounds)
{
    if(currentDragDeltaX == 0 && currentDragDeltaY == 0) return;
    
    if(copyDraggedSounds) SamplerBody::SampleEditingActions::duplicateSelectedSounds(body);

	ownerSampler->getUndoManager()->beginNewTransaction("Dragging of " + String(dragStartData.size()) + " samples");

	for(int i = 0; i <dragStartData.size(); i++)
	{
		DragData d = dragStartData[i];

        if(currentDragDeltaX < 0)
        {
            d.sound->setPropertyWithUndo(ModulatorSamplerSound::RootNote, d.root + currentDragDeltaX);
            d.sound->setPropertyWithUndo(ModulatorSamplerSound::KeyLow, d.lowKey + currentDragDeltaX);
            d.sound->setPropertyWithUndo(ModulatorSamplerSound::KeyHigh, d.hiKey + currentDragDeltaX);
        }
		else if (currentDragDeltaX > 0)
        {
            d.sound->setPropertyWithUndo(ModulatorSamplerSound::RootNote, d.root + currentDragDeltaX);
            d.sound->setPropertyWithUndo(ModulatorSamplerSound::KeyHigh, d.hiKey + currentDragDeltaX);
            d.sound->setPropertyWithUndo(ModulatorSamplerSound::KeyLow, d.lowKey + currentDragDeltaX);
        }

		if (currentDragDeltaY < 0)
		{
			
			const int lowVelo = jmax<int>(0, d.loVel + currentDragDeltaY);
			const int highVelo = jmin<int>(127, d.hiVel + currentDragDeltaY);

			d.sound->setPropertyWithUndo(ModulatorSamplerSound::VeloLow, lowVelo);
			d.sound->setPropertyWithUndo(ModulatorSamplerSound::VeloHigh, highVelo);
		}
		else if (currentDragDeltaY > 0)
		{
			const int lowVelo = jmax<int>(0, d.loVel + currentDragDeltaY);
			const int highVelo = jmin<int>(127, d.hiVel + currentDragDeltaY);

			d.sound->setPropertyWithUndo(ModulatorSamplerSound::VeloHigh, highVelo);
			d.sound->setPropertyWithUndo(ModulatorSamplerSound::VeloLow, lowVelo);
		}
			
	}

	ownerSampler->getUndoManager()->beginNewTransaction();

	sampleDraggingEnabled = false;
}

void SamplerSoundMap::modifierKeysChanged(const ModifierKeys &modifiers)
{
	if(modifiers.isAltDown() && selectedSounds->getNumSelected() != 0)
	{
		if(modifiers.isCtrlDown())
		{
			setMouseCursor(MouseCursor::CopyingCursor);

		}
		else
		{
			setMouseCursor(MouseCursor::DraggingHandCursor);
		}
	}
	else
	{
		setMouseCursor(MouseCursor::NormalCursor);
	}
}

void SamplerSoundMap::findLassoItemsInArea(Array<WeakReference<SampleComponent>> &/*itemsFound*/, const Rectangle< int > &area)
{
	currentLassoRectangle = area;
		
	refreshSelectedSoundsFromLasso();
    
    //itemsFound.addArray(lassoSelectedComponents);
}

void SamplerSoundMap::refreshSelectedSoundsFromLasso()
{
	lassoSelectedComponents.clear();

	for (int i = 0; i < sampleComponents.size(); i++)
	{
		SampleComponent *c = sampleComponents[i];

		Rectangle<int> sampleBounds = c->getBoundsInParent();

		if (c->isVisible() && currentLassoRectangle.intersectRectangle(sampleBounds))
		{
			lassoSelectedComponents.add(c);
		}
	}
}

void SamplerSoundMap::drawSoundMap(Graphics &g)
{
    g.fillAll(Colour(0xFF333333));
    
    const float noteWidth = (float)getWidth() / 128.0f;
    //const float velocityHeight = (float)getHeight() / 128.0f;
    
    if(notePosition != -1)
    {
        g.setColour(Colours::black.withAlpha(0.3f));
        g.fillRect(0, 0, 20, 20);
        
        String x = MidiMessage::getMidiNoteName(notePosition, true, true, 3);
        g.setFont(GLOBAL_MONOSPACE_FONT());
        g.setColour(Colours::white.withAlpha(0.6f));
        g.drawText(x, 0, 0, 20, 20, Justification::centredLeft, false);
    }
    
    if(!draggedFileRootNotes.isZero())
    {
        g.setColour(Colours::white);
        
        for(int i = 0; i < 128; i++)
        {
            if(draggedFileRootNotes[i])
            {
                if(semiTonesPerNote < 0)
                {
                    const float numSounds = (float)semiTonesPerNote * -1.0f;
                    
                    const float height = (float)getHeight() / numSounds;
                    float y_offset = 0.0f;
                    
                    for(int j = 0; j < numSounds; j++)
                    {
                        g.drawRect(i *noteWidth, y_offset, noteWidth, height, 1.0f);
                        y_offset += height;
                    }
                    
                    break;
                }
                
                g.drawRect(i * noteWidth, 0.0f, semiTonesPerNote * noteWidth, (float)getHeight(), 1.0f);
            }
        }
    }
    
    g.setColour(Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
    
    for(int i = 1; i < 128; i++)
    {
        g.drawVerticalLine((int)((float)i * noteWidth), 0.0f, (float)getHeight());
        //g.drawLine(i * noteWidth, 0, i * noteWidth, (float)getHeight(), 1.0f);
    }
    
    for(int i = 0; i < sampleComponents.size(); i++)
    {
        SampleComponent *c = sampleComponents[i];
		
		if (!c->isVisible()) continue;

		c->drawSampleRectangle(g, c->getBoundsInParent());
    }
}

void SamplerSoundMap::paint(Graphics &g)
{
    g.drawImageAt(currentSnapshot, 0, 0);
};

void SamplerSoundMap::paintOverChildren(Graphics &g)
{
    const float noteWidth = (float)getWidth() / 128.0f;
    const float velocityHeight = (float)getHeight() / 128.0f;
    
    for(int i = 0; i < 127; i++)
    {
        if(pressedKeys[i] != -1)
        {
            float x = (float)i*noteWidth;
            float w = noteWidth;
            float y = (float)(getHeight() - (pressedKeys[i] * velocityHeight) - 2.0f);
            float h = 4.0f;
            
            g.setColour(Colours::red);
            g.fillRect(x, y, w, h);
        }
    }

	if(sampleDraggingEnabled)
	{
        
        
		for(int i = 0; i < dragStartData.size(); i++)
		{
			DragData d = dragStartData[i];

			const int x = (int)((d.lowKey + currentDragDeltaX) * noteWidth);
			const int w = (int)((1 + d.hiKey - d.lowKey) * noteWidth);

			const int y = (int)(getHeight() - (d.hiVel + currentDragDeltaY) * velocityHeight);
			const int h = (int)((127.0f - (float)(d.loVel + currentDragDeltaY)) * velocityHeight) - y;

			//const int y = (int)(getHeight() - (float)d.sound->getProperty(ModulatorSamplerSound::VeloHigh) * velocityHeight);
			//const int h = (int)((127.0f - (float)d.sound->getProperty(ModulatorSamplerSound::VeloLow)) * velocityHeight) - y;

			g.setColour(Colours::blue.withAlpha(0.4f));

			g.drawRect(x,y,w,h, 1);

			g.setColour(Colours::blue.withAlpha(0.2f));

			g.fillRect(x,y,w,h);

		}
	}
}

void SamplerSoundMap::drawSampleComponentsForDragPosition(int numDraggedFiles, int x, int y)
{
	const float w = (float)(getWidth());
	const float h = (float)(getHeight());

	const int noteNumberForFirstNote = (int)((float)(x * 128) / (w));

	semiTonesPerNote =  (int)((4.0f*(h - (float)y)) / h);

	draggedFileRootNotes = 0;

	draggedFileRootNotes.setRange(0, 128, false);

	int noteNumber = noteNumberForFirstNote;

	for(int i = 0; i < numDraggedFiles; i++)
	{
		draggedFileRootNotes.setBit(noteNumber, true);

		noteNumber += semiTonesPerNote;
	}

	// Save the amount of notes into the semiTone variable for repaint();
	if (semiTonesPerNote == 0) semiTonesPerNote = -numDraggedFiles;

    refreshGraphics();
}


void SamplerSoundMap::updateSampleComponentWithSound(ModulatorSamplerSound *sound)
{
    const int index = (int)sound->getProperty(ModulatorSamplerSound::Property::ID);
    
    if(index < sampleComponents.size())
    {
        updateSampleComponent(index);
    }
    else
    {
        jassertfalse;
    }
};


void SamplerSoundMap::updateSampleComponent(int index)
{
	const ModulatorSamplerSound *s = sampleComponents[index]->getSound();

	if(s != nullptr)
	{
		const float noteWidth = (float)getWidth() / 128.0f;
		const int velocityHeight = getHeight() / 128;

		jassert(s != nullptr);

		const float x = (float)s->getProperty(ModulatorSamplerSound::KeyLow) * noteWidth;
		const float x_max = ((float)s->getProperty(ModulatorSamplerSound::KeyHigh) + 1.0f) * noteWidth;

		const int y = getHeight() - (int)s->getProperty(ModulatorSamplerSound::VeloHigh) * velocityHeight - velocityHeight;
		const int y_max = getHeight() - (int)s->getProperty(ModulatorSamplerSound::VeloLow) * velocityHeight;

		sampleComponents[index]->setSampleBounds((int)x, (int)y, (int)(x_max - x), (int)(y_max-y));
		
        refreshGraphics();
	}
}


void SamplerSoundMap::updateSampleComponents()
{
	for(int i = 0; i < sampleComponents.size(); i++)
	{
		updateSampleComponent(i);
	}
};


void SamplerSoundMap::updateSoundData()
{
	if(newSamplesDetected())
	{
		sampleComponents.clear();

		for(int i = 0; i < ownerSampler->getNumSounds(); i++)
		{
			SampleComponent *c = new SampleComponent(ownerSampler->getSound(i), this);

			sampleComponents.add(c);
		}
	}

	updateSampleComponents();
}

bool SamplerSoundMap::keyPressed(const KeyPress &k)
{
	if(k.getModifiers().isAltDown())
	{
		return true;
	}

	return false;
}

void SamplerSoundMap::mouseDown(const MouseEvent &e)
{
	if(e.mods.isRightButtonDown()) return;

	checkEventForSampleDragging(e.getEventRelativeTo(this));

	if(sampleDraggingEnabled)
	{
		setMouseCursor(MouseCursor::DraggingHandCursor);
	}
	else
	{
		milliSecondsSinceLastLassoCheck = Time::getMillisecondCounter();
		sampleLasso->beginLasso(e.getEventRelativeTo(this), this);
	}
    
    refreshGraphics();
}

void SamplerSoundMap::mouseUp(const MouseEvent &e)
{
	if(sampleDraggingEnabled)
	{
		endSampleDragging(e.mods.isCtrlDown());
		setMouseCursor(MouseCursor::NormalCursor);
	}

	else
	{
		if(!e.mods.isRightButtonDown() &&
			e.getPosition() == e.getMouseDownPosition() &&
			getSampleComponentAt(e.getPosition()) == nullptr)
		{
			selectedSounds->deselectAll();
		}

		sampleLasso->endLasso();


		if (!e.getOffsetFromDragStart().isOrigin())
		{
			for (int i = 0; i < lassoSelectedComponents.size(); i++)
			{
				selectedSounds->addToSelectionBasedOnModifiers(lassoSelectedComponents[i], ModifierKeys::shiftModifier);
			}
		}
        else if (!e.mods.isRightButtonDown())
        {
            SampleComponent *c = getSampleComponentAt(e.getMouseDownPosition());
            
            if(c != nullptr )
            {
                ModulatorSamplerSound *s = c->getSound();
                
				if (s != nullptr) selectedSounds->addToSelectionBasedOnModifiers(c, e.mods);
            }
        }

		milliSecondsSinceLastLassoCheck = 0;
	}

    refreshGraphics();
}

void SamplerSoundMap::mouseExit(const MouseEvent &)
{
	notePosition = -1;
	veloPosition = -1;

	draggedFileRootNotes = 0;
	sampleDraggingEnabled = false;

	setMouseCursor(MouseCursor::NormalCursor);

	repaint();
}

void SamplerSoundMap::mouseMove(const MouseEvent &e)
{
	const float noteWidth = (float)getWidth() / 128.0f;

	const float velocityHeight = (float)getHeight() / 128.0f;

	notePosition = (int)(e.getPosition().getX() / noteWidth);
	veloPosition = 127 - (int)(e.getPosition().getY() / velocityHeight);

	SampleComponent *c = getSampleComponentAt(e.getPosition());

	if(c != nullptr && c->getSound() != nullptr)
	{
		setTooltip(c->getSound()->getPropertyAsString(ModulatorSamplerSound::FileName));
	}
	else
	{
		setTooltip(MidiMessage::getMidiNoteName(notePosition, true, true, 3));
	}
};

void SamplerSoundMap::mouseDrag(const MouseEvent &e)
{
	if(sampleDraggingEnabled)
	{
		if (e.mods.isShiftDown())
		{
			currentDragLimiter = DragLimiters::VelocityOnly;
		}
		else
		{
			currentDragLimiter = DragLimiters::NoLimit;
		}


		int lowestKey = INT_MAX;
		int highestKey = 0;

		int lowestVelocity = INT_MAX;
		int highestVelocity = 0;

		for (int i = 0; i < dragStartData.size(); i++)
		{
			if (dragStartData[i].hiKey > highestKey) highestKey = dragStartData[i].hiKey;
			if (dragStartData[i].lowKey < lowestKey) lowestKey = dragStartData[i].lowKey;
			if (dragStartData[i].hiVel > highestVelocity) highestVelocity = dragStartData[i].hiVel;
			if (dragStartData[i].loVel < lowestVelocity) lowestVelocity = dragStartData[i].loVel;
		}

		int thisDragDeltaX = (int)((float)e.getDistanceFromDragStartX() / (float)getWidth() * 128.0f);
		int thisDragDeltaY = -(int)((float)e.getDistanceFromDragStartY() / (float)getHeight() * 128.0f);

		if (currentDragLimiter != VelocityOnly)
		{
			if (lowestKey + thisDragDeltaX >= 0 && highestKey + thisDragDeltaX < 128)
			{
				currentDragDeltaX = thisDragDeltaX;
			}
			else
			{
				if (thisDragDeltaX < 0)
				{
					currentDragDeltaX = -lowestKey;
				}
				else
				{
					currentDragDeltaX = 128 - highestKey;
				}
			}
		}
		
		if (currentDragLimiter != KeyOnly)
		{
			if (lowestVelocity + thisDragDeltaY >= 0 && highestVelocity + thisDragDeltaY < 128)
			{
				currentDragDeltaY = thisDragDeltaY;
			}
			else
			{
				if (thisDragDeltaY < 0)
				{
					currentDragDeltaY = -lowestVelocity;
				}
				else
				{
					currentDragDeltaY = 127 - highestVelocity;
				}
			}

		}
	}
	else
	{
		sampleLasso->dragLasso(e);
	}
    
    refreshGraphics();
}

void SamplerSoundMap::setPressedKeys(const int *pressedKeyData)
{
	for(int i = 0; i < 127; i++)
	{
		const int number = i;
		const int velocity = pressedKeyData[i];

		const bool newNote = velocity != -1 && velocity != pressedKeys[i];

		if(newNote)
		{
			for(int i = 0; i < sampleComponents.size(); i++)
			{
				if(sampleComponents[i]->isVisible() &&
					sampleComponents[i]->getSound()->appliesToMessage(1, number, velocity) &&
					sampleComponents[i]->getSound()->appliesToRRGroup(ownerSampler->getSamplerDisplayValues().currentGroup))
				{
					sampleComponents[i]->triggerNoteOnAnimation(velocity);
				}
			}
		}

		pressedKeys[i] = pressedKeyData[i];

	}

	repaint();
}
	

SampleComponent* SamplerSoundMap::getSampleComponentAt(Point<int> point)
{
	for(int i = 0; i < sampleComponents.size(); i++)
	{
		if (sampleComponents[i]->isVisible() && sampleComponents[i]->samplePathContains(point)) return sampleComponents[i];
	}

	return nullptr;
};

void SamplerSoundMap::checkEventForSampleDragging(const MouseEvent &e)
{
	sampleDraggingEnabled = e.mods.isAltDown() && e.mods.isLeftButtonDown() && selectedSounds->getNumSelected() != 0;

	if(sampleDraggingEnabled)
	{
		dragStartData.clear();

		currentDragDeltaX = 0;
		currentDragDeltaY = 0;

		for(int i = 0; i < selectedSounds->getNumSelected(); i++)
		{
			DragData d;

			d.sound = selectedSounds->getSelectedItem(i)->getSound();

			d.root = d.sound->getProperty(ModulatorSamplerSound::RootNote);
			d.lowKey = d.sound->getProperty(ModulatorSamplerSound::KeyLow);
			d.hiKey = d.sound->getProperty(ModulatorSamplerSound::KeyHigh);
			d.loVel = d.sound->getProperty(ModulatorSamplerSound::VeloLow);
			d.hiVel = d.sound->getProperty(ModulatorSamplerSound::VeloHigh);


			dragStartData.add(d);
		}
	}
}


void SamplerSoundMap::setSelectedIds(const Array<ModulatorSamplerSound*> newSelectionList)
{
	selectedSounds->deselectAll();

	for(int i = 0; i < sampleComponents.size(); i++)
	{
		if(newSelectionList.contains(sampleComponents[i]->getSound()))
		{
			selectedSounds->addToSelection(sampleComponents[i]);
		}
	}

	updateSampleComponents();
}


void SamplerSoundMap::soloGroup(int groupIndex)
{
	for(int i = 0; i < sampleComponents.size(); i++)
	{
		const bool visible = (groupIndex == - 1) || sampleComponents[i]->appliesToGroup(groupIndex);

		sampleComponents[i]->setVisible(visible);
		sampleComponents[i]->setEnabled(visible);
	}

    refreshGraphics();
}


bool SamplerSoundMap::newSamplesDetected()
{
	if(ownerSampler->getNumSounds() != sampleComponents.size()) return true;

	for(int i = 0; i < sampleComponents.size(); i++)
	{
		const ModulatorSamplerSound *s = sampleComponents[i]->getSound();

		if (s == nullptr)
			return true;
		if (s != ownerSampler->getSound(i))
			return true;
	}

	return false;
}

// =================================================================================================================== MapWithKeyboard

MapWithKeyboard::MapWithKeyboard(ModulatorSampler *ownerSampler, SamplerBody *b):
	sampler(ownerSampler),
	lastNoteNumber(-1)
{
	addAndMakeVisible(map = new SamplerSoundMap(ownerSampler, b));
};

void MapWithKeyboard::paint(Graphics &g)
{
	int x = map->getX();
	int y = map->getBottom();
	int width = map->getWidth();
	float height = 20.0f;

	float noteWidth = width / 128.0f;

	const static bool blackKeys[12] = {false, true, false, true, false, false, true, false, true, false, true, false};

	for(int i = 0; i < 128; i++)
	{
		Colour keyColour = selectedRootNotes[i] ? blackKeys[i % 12] ? Colours::darkred : Colours::red :
													blackKeys[i % 12] ? Colours::black : Colours::white;

		if(i == lastNoteNumber) keyColour = Colours::red;

		g.setColour(keyColour.withAlpha(0.4f));
		g.fillRect(x + i * noteWidth, (float)y, noteWidth, height);
		g.setColour(keyColour.withAlpha(0.05f));
		g.drawRect(x + i * noteWidth, (float)y, noteWidth, height);
	}
}

void MapWithKeyboard::resized() 
{
	const int height = getHeight() - 32;

	map->setBounds(0, 0, getWidth(), height);
    
    map->refreshGraphics();
};

void MapWithKeyboard::mouseDown(const MouseEvent &e)
{
	lastNoteNumber = (128 * e.getMouseDownPosition().getX()) / map->getWidth();

	const int velocity = ((getHeight() - e.getMouseDownY()) * 127) / 20;

	MidiMessage m = MidiMessage::noteOn(1, lastNoteNumber, (uint8)velocity);

	sampler->preMidiCallback(m);
	sampler->noteOn(1, lastNoteNumber, (float)velocity / 127.0f);

	repaint();
}

void MapWithKeyboard::mouseUp(const MouseEvent &)
{
	MidiMessage m = MidiMessage::noteOff(1, lastNoteNumber);

	sampler->preMidiCallback(m);
	sampler->noteOff(1, lastNoteNumber, 1.0f, true);

	lastNoteNumber = -1;

	repaint();

	
}

// =================================================================================================================== SamplerSoundTable

SamplerSoundTable::SamplerSoundTable(ModulatorSampler *ownerSampler_, SamplerBody *b)   :
	font (GLOBAL_FONT()),
	ownerSampler(ownerSampler_),
	body(b),
	internalSelection(false)
{

    // Create our table component and add it to this component..
    addAndMakeVisible (table);
    table.setModel (this);

    // give it a border
    table.setColour (ListBox::outlineColourId, Colours::grey);
	table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

    table.setOutlineThickness (0);


#define PROPERTY(x) ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::x), ModulatorSamplerSound::x

	table.getHeader().addColumn(PROPERTY(ID), 30, 30, 30, TableHeaderComponent::ColumnPropertyFlags::notResizable);
	table.getHeader().addColumn(PROPERTY(FileName), 320, -1, -1, TableHeaderComponent::notResizable);
	table.getHeader().addColumn(PROPERTY(RRGroup), 50, 50, 50, TableHeaderComponent::notResizable);
    table.getHeader().addColumn(PROPERTY(RootNote), 50, 50, 50, TableHeaderComponent::notResizable);
	table.getHeader().addColumn(PROPERTY(KeyHigh), 50, 50, 50, TableHeaderComponent::notResizable);
	table.getHeader().addColumn(PROPERTY(KeyLow), 50, 50, 50, TableHeaderComponent::notResizable);
	table.getHeader().addColumn(PROPERTY(VeloLow), 50, 50, 50, TableHeaderComponent::notResizable);
	table.getHeader().addColumn(PROPERTY(VeloHigh), 50, 50, 50, TableHeaderComponent::notResizable);

	table.getHeader().setLookAndFeel(&laf);

	table.setHeaderHeight(18);

	table.setMultipleSelectionEnabled (true);

	table.getHeader().setStretchToFitActive(true);
	

#undef PROPERTY
}

void SamplerSoundTable::refreshList()
{
	sortedSoundList.clear();

	for(int i = 0; i < ownerSampler->getNumSounds(); i++)
	{
		sortedSoundList.add(ownerSampler->getSound(i));
	}

    // we could now change some initial settings..
    table.getHeader().setSortColumnId (2, true); // sort forwards by the ID column

	table.updateContent();

	resized();
}

int SamplerSoundTable::getNumRows() { return ownerSampler->getNumSounds(); }

bool SamplerSoundTable::broadcasterIsSelection(ChangeBroadcaster *b) const
{
	return dynamic_cast<SelectedItemSet<ModulatorSamplerSound*>*>(b) != nullptr;
}

	
void SamplerSoundTable::paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
	if(rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));

    if (rowIsSelected)
        g.fillAll (Colour(0xaa680000));
}

void SamplerSoundTable::paintCell (Graphics& g, int rowNumber, int columnId,
                int width, int height, bool /*rowIsSelected*/) 
{

	if (rowNumber < sortedSoundList.size())
	{
		g.setColour(Colours::white.withAlpha(.8f));
		g.setFont(font);



		String text(sortedSoundList[rowNumber]->getPropertyAsString((ModulatorSamplerSound::Property)columnId));

		g.drawText(text, 2, 0, width - 4, height, Justification::centred, true);

		g.setColour(Colours::black.withAlpha(0.2f));
		g.fillRect(width - 1, 0, 1, height);
	}
	else
	{
		table.updateContent();
	}
}

void SamplerSoundTable::sortOrderChanged (int newSortColumnId, bool isForwards) 
{
    if (newSortColumnId != 0)
    {
		DemoDataSorter sorter ((ModulatorSamplerSound::Property)newSortColumnId, isForwards);

		sortedSoundList.sort(sorter);
        table.updateContent();
    }
}

void SamplerSoundTable::soundsSelected(const Array<ModulatorSamplerSound *> &selectedSounds)
{
	table.deselectAllRows();

    SparseSet<int> selection;
    
	for (int i = 0; i < sortedSoundList.size(); i++)
	{
		ModulatorSamplerSound *sound = sortedSoundList[i];

		if (selectedSounds.contains(sound))
		{
			selection.addRange(Range<int>(i, i + 1));
		}
	}
        
    table.setSelectedRows(selection);
}


void SamplerSoundTable::resized()
{
    table.setBounds(getLocalBounds());
	table.getHeader().setColumnWidth(ModulatorSamplerSound::FileName, table.getVisibleRowWidth() - 330);
}


void SamplerSoundTable::selectedRowsChanged(int /*lastRowSelected*/)
{
	if(internalSelection) return; // Quick hack to disable the endless loop

	SparseSet<int> selection = table.getSelectedRows();

	body->getSelection().deselectAll();

	for(int i = 0; i < selection.size(); i++)
	{
		body->getSelection().addToSelection(sortedSoundList[selection[i]]);
	}
};