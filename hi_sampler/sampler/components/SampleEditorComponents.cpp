namespace hise { using namespace juce;


// =================================================================================================================== SamplerSubEditor

void SamplerSubEditor::selectSounds(const SampleSelection &selection)
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

    plaf->setColour (PopupMenu::ColourIds::highlightedBackgroundColourId , Colour(SIGNAL_COLOUR));
    
	for (int i = 0; i < options.size(); i++)
	{
		if (isTicked == 0)
		{
			p.addCustomItem(i + 1, std::unique_ptr<TooltipPopupComponent>(new TooltipPopupComponent(options[i], toolTips[i], getWidth() - 4)));
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
#if USE_BACKEND
    ProcessorEditor *pEditor = findParentComponentOfClass<ProcessorEditor>();
    
    if(pEditor != nullptr)
    {
        PresetHandler::setChanged(pEditor->getProcessor());
    }
#endif
    
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


String PopupLabel::getOptionDescription() const
{
	String desc;

	NewLine nl;

	desc << "Use one of these options:" << nl << nl;

	for (int i = 0; i < options.size(); i++)
	{
		desc << options[i] << ": " << toolTips[i] << nl;
	}

	return desc;
}

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
	if (sound->isMissing() || sound->isPurged())
		enabled = false;
};

#pragma warning( push )
#pragma warning( disable: 4100 )

#pragma warning( pop )


bool SampleComponent::samplePathContains(Point<int> localPoint) const
{
	if (outline.isEmpty())
		return bounds.contains(localPoint);
	else
		return outline.contains(localPoint.toFloat(), 0.0f);
}

void SampleComponent::drawSampleRectangle(Graphics &g, Rectangle<int> areaInt)
{
    if(sound.get() == nullptr) return;
    
	ScopedLock sl(map->getSampler()->getExportLock());

    Rectangle<float> area((float)areaInt.getX(), (float)areaInt.getY(), (float)areaInt.getWidth(), (float)areaInt.getHeight());
    
    const int lowerXFade = sound->getSampleProperty(SampleIds::LowerVelocityXFade);
    const int upperXFade = sound->getSampleProperty(SampleIds::UpperVelocityXFade);
    
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

SamplerSoundMap::SamplerSoundMap(ModulatorSampler *ownerSampler_):
	PreloadListener(ownerSampler_->getMainController()->getSampleManager()),
	ownerSampler(ownerSampler_),
	handler(ownerSampler->getSampleEditHandler()),
	notePosition(-1),
	veloPosition(-1),
	selectedSounds(new SelectedItemSet<WeakReference<SampleComponent>>()),
	sampleLasso(new LassoComponent<WeakReference<SampleComponent>>())
{
    sampleLasso->setColour(LassoComponent<SampleComponent>::ColourIds::lassoFillColourId, Colours::white.withAlpha(0.1f));
    sampleLasso->setColour(LassoComponent<SampleComponent>::ColourIds::lassoOutlineColourId, Colour(SIGNAL_COLOUR));
    
    
	ownerSampler->getSampleMap()->addListener(this);

	for (uint8 i = 0; i < 128; i++)
	{
		pressedKeys[i] = 255;
	}

	selectedSounds->addChangeListener(this);

	addChildComponent(sampleLasso);

	updateSoundData();

	

	setOpaque(true);
};

SamplerSoundMap::~SamplerSoundMap()
{
	if (ownerSampler != nullptr)
	{
		ownerSampler->getSampleMap()->removeListener(this);
	}
		
	sampleComponents.clear();
}

void SamplerSoundMap::changeListenerCallback(ChangeBroadcaster *b)
{
	if(b == selectedSounds)
	{
		handler->getSelection().deselectAll();

		for(int i = 0; i < sampleComponents.size(); i++)
		{
			sampleComponents[i]->setSelected(false);
		}

		auto& selectedSampleComponents = selectedSounds->getItemArray();

		for(int i = 0; i < selectedSampleComponents.size(); i++)
		{
			if (selectedSampleComponents[i].get() != nullptr)
			{
				selectedSampleComponents[i]->setSelected(true);

				if (selectedSampleComponents[i]->getSound() != nullptr)
				{
					handler->getSelection().addToSelection(selectedSampleComponents[i]->getSound());
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
		auto sound = selectedSounds->getSelectedItem(0).get();

		if (sound == nullptr)
			return;

		const int lowKey = selectedSounds->getSelectedItem(0)->getSound()->getSampleProperty(SampleIds::LoKey);
		const int lowVelo = selectedSounds->getSelectedItem(0)->getSound()->getSampleProperty(SampleIds::LoVel);

		const int hiKey = selectedSounds->getSelectedItem(0)->getSound()->getSampleProperty(SampleIds::HiKey);
		const int hiVelo = selectedSounds->getSelectedItem(0)->getSound()->getSampleProperty(SampleIds::HiVel);

		const int group = selectedSounds->getSelectedItem(0)->getSound()->getSampleProperty(SampleIds::RRGroup);

		for(int i = 0; i < sampleComponents.size(); i++)
		{
			const int thisLowKey = sampleComponents[i]->getSound()->getSampleProperty(SampleIds::LoKey);
			const int thisLowVelo = sampleComponents[i]->getSound()->getSampleProperty(SampleIds::LoVel);

			const int thisHiKey = sampleComponents[i]->getSound()->getSampleProperty(SampleIds::HiKey);
			const int thisHiVelo = sampleComponents[i]->getSound()->getSampleProperty(SampleIds::HiVel);

			const int thisGroup = sampleComponents[i]->getSound()->getSampleProperty(SampleIds::RRGroup);

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
				handler->getSelection().deselectAll();
				handler->getSelection().addToSelection(sampleComponents[i]->getSound());
			}
		}
	}

    refreshGraphics();
}



void SamplerSoundMap::endSampleDragging(bool copyDraggedSounds)
{
	if (currentDragDeltaX == 0 && currentDragDeltaY == 0)
	{
		dragStartData.clear();
		return;
	}
    

    if(copyDraggedSounds) 
		SampleEditHandler::SampleEditingActions::duplicateSelectedSounds(handler);

	auto f = [this, copyDraggedSounds](Processor* )
	{
		for (int i = 0; i < dragStartData.size(); i++)
		{
			DragData d = dragStartData[i];

			if (currentDragDeltaX < 0)
			{
				d.sound->setSampleProperty(SampleIds::Root, d.root + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::LoKey, d.lowKey + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::HiKey, d.hiKey + currentDragDeltaX);
			}
			else if (currentDragDeltaX > 0)
			{
				d.sound->setSampleProperty(SampleIds::Root, d.root + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::HiKey, d.hiKey + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::LoKey, d.lowKey + currentDragDeltaX);
			}

			if (currentDragDeltaY < 0)
			{

				const int lowVelo = jmax<int>(0, d.loVel + currentDragDeltaY);
				const int highVelo = jmin<int>(127, d.hiVel + currentDragDeltaY);

				d.sound->setSampleProperty(SampleIds::LoVel, lowVelo);
				d.sound->setSampleProperty(SampleIds::HiVel, highVelo);
			}
			else if (currentDragDeltaY > 0)
			{
				const int lowVelo = jmax<int>(0, d.loVel + currentDragDeltaY);
				const int highVelo = jmin<int>(127, d.hiVel + currentDragDeltaY);

				d.sound->setSampleProperty(SampleIds::HiVel, highVelo);
				d.sound->setSampleProperty(SampleIds::LoVel, lowVelo);
			}

		}

		sampleDraggingEnabled = false;
		
		auto f2 = [this]()
		{
			this->refreshGraphics();
		};

		MessageManager::callAsync(f2);
		

		return SafeFunctionCall::OK;
	};

	// If we copy this we need to make sure that the order stays the same...
	if (!copyDraggedSounds)
		f(ownerSampler);
	else
		ownerSampler->killAllVoicesAndCall(f);
	
}

void SamplerSoundMap::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& /*newValue*/)
{
	ignoreUnused(s, id);

#if 0
	auto index = s->getId();

	if (SampleIds::Helpers::isMapProperty(id) && index < sampleComponents.size())
	{
		updateSampleComponent(index);
	}
#endif

	
}

void SamplerSoundMap::preloadStateChanged(bool isPreloading_)
{
	isPreloading = isPreloading_;

	if (!isPreloading)
		updateSoundData();

	repaint();
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
    
    if(ownerSampler->getSampleMap()->isMonolith())
    {
		String mt = "Monolith";

		Font f = GLOBAL_BOLD_FONT();

		int width = f.getStringWidth(mt) + 20;

        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRect(0, 0, width, 20);
		
        g.setFont(f);
        g.setColour(Colours::white.withAlpha(0.5f));
        g.drawText(mt, 0, 0, width, 20, Justification::centred, false);
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
	//g.drawImageAt(currentSnapshot, 0, 0);
	drawSoundMap(g);
};

void SamplerSoundMap::paintOverChildren(Graphics &g)
{
	
	

	if (isPreloading)
	{
		g.fillAll(Colour(0xAA222222));
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);
		g.drawText("Preloading", getLocalBounds().toFloat(), Justification::centred);
	}
	else
	{
		const float noteWidth = (float)getWidth() / 128.0f;
		const float velocityHeight = (float)getHeight() / 128.0f;

		for (int i = 0; i < 127; i++)
		{
			if (pressedKeys[i] != 0)
			{
				float x = (float)i*noteWidth;
				float w = noteWidth;
				float y = (float)(getHeight() - (pressedKeys[i] * velocityHeight) - 2.0f);
				float h = 4.0f;

				g.setColour(Colour(SIGNAL_COLOUR));
				g.fillRect(x, y, w, h);
			}
		}

		if (sampleDraggingEnabled)
		{


			for (int i = 0; i < dragStartData.size(); i++)
			{
				DragData d = dragStartData[i];

				const int x = (int)((d.lowKey + currentDragDeltaX) * noteWidth);
				const int w = (int)((1 + d.hiKey - d.lowKey) * noteWidth);

				const int y = (int)(getHeight() - (d.hiVel + currentDragDeltaY) * velocityHeight);
				const int h = (int)((127.0f - (float)(d.loVel + currentDragDeltaY)) * velocityHeight) - y;

				//const int y = (int)(getHeight() - (float)d.sound->getSampleProperty(SampleIds::HiVel) * velocityHeight);
				//const int h = (int)((127.0f - (float)d.sound->getSampleProperty(SampleIds::LoVel)) * velocityHeight) - y;

				g.setColour(Colours::blue.withAlpha(0.4f));

				g.drawRect(x, y, w, h, 1);

				g.setColour(Colours::blue.withAlpha(0.2f));

				g.fillRect(x, y, w, h);

			}
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


void SamplerSoundMap::drawSampleMapForDragPosition()
{
	semiTonesPerNote = 128;
	draggedFileRootNotes = 0;
	draggedFileRootNotes.setBit(0, true);

	refreshGraphics();
}

void SamplerSoundMap::updateSampleComponentWithSound(ModulatorSamplerSound *sound)
{
    const int index = (int)sound->getSampleProperty(SampleIds::ID);
    
    if(index < sampleComponents.size())
    {
        updateSampleComponent(index, dontSendNotification);
    }
    else
    {
        jassertfalse;
    }
};


void SamplerSoundMap::updateSampleComponent(int index, NotificationType )
{
	ModulatorSamplerSound* s = sampleComponents[index]->getSound();

	
	if(s != nullptr)
	{
		const float noteWidth = (float)getWidth() / 128.0f;
		const int velocityHeight = getHeight() / 128;

		jassert(s != nullptr);

		const float x = (float)s->getSampleProperty(SampleIds::LoKey) * noteWidth;
		const float x_max = ((float)s->getSampleProperty(SampleIds::HiKey) + 1.0f) * noteWidth;

		const int y = getHeight() - (int)s->getSampleProperty(SampleIds::HiVel) * velocityHeight - velocityHeight;
		const int y_max = getHeight() - (int)s->getSampleProperty(SampleIds::LoVel) * velocityHeight;

		sampleComponents[index]->setSampleBounds((int)x, (int)y, (int)(x_max - x), (int)(y_max-y));
		
		repaint();
	}
}


void SamplerSoundMap::updateSampleComponents()
{
	for(int i = 0; i < sampleComponents.size(); i++)
	{
		updateSampleComponent(i, dontSendNotification);
	}
};


void SamplerSoundMap::updateSoundData()
{
	if (isPreloading)
		return;

	if(newSamplesDetected())
	{
		sampleComponents.clear();

		ModulatorSampler::SoundIterator sIter(ownerSampler, false);

		while (auto sound = sIter.getNextSound())
		{
			sampleComponents.add(new SampleComponent(sound, this));
		}
	}
	else
	{
		refreshGraphics();
	}

	sampleMapWasChanged(ownerSampler->getSampleMap()->getReference());
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
	refreshGraphics();

	if(sampleDraggingEnabled)
	{
		endSampleDragging(e.mods.isAltDown());
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
			auto modifiers = e.mods;

			// Deselect the previous ones...
			if (!modifiers.isShiftDown())
				selectedSounds->deselectAll();

			// We need to add the shift modifier to that it selects all samples in the list
			if (!modifiers.isShiftDown() && !modifiers.isCommandDown())
				modifiers = modifiers.withFlags(ModifierKeys::shiftModifier);

			for (int i = 0; i < lassoSelectedComponents.size(); i++)
			{
				selectedSounds->addToSelectionBasedOnModifiers(lassoSelectedComponents[i], modifiers);
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

	setMouseCursor(isDragOperation(e) ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);

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

bool SamplerSoundMap::isDragOperation(const MouseEvent& e)
{
	bool selectModifiersActive = e.mods.isShiftDown() || e.mods.isCommandDown();

	bool hoverOverSelection = false;

	if (auto hoveredComponent = getSampleComponentAt(e.getPosition()))
	{
		if (auto s = hoveredComponent->getSound())
		{
			for (auto& s_ : *selectedSounds)
			{
				if (s_.get() == nullptr)
					continue;

				if (s == s_.get()->getSound())
				{
					hoverOverSelection = true;
					break;
				}
			}
		}
	}

	bool dragOperation = hoverOverSelection && selectedSounds->getNumSelected() != 0 && !selectModifiersActive;

	return dragOperation;
}



void SamplerSoundMap::mouseMove(const MouseEvent &e)
{
	const float noteWidth = (float)getWidth() / 128.0f;

	const float velocityHeight = (float)getHeight() / 128.0f;

	notePosition = (int)(e.getPosition().getX() / noteWidth);
	veloPosition = 127 - (int)(e.getPosition().getY() / velocityHeight);

	SampleComponent *c = getSampleComponentAt(e.getPosition());

	if(c != nullptr && c->getSound() != nullptr)
		setTooltip(c->getSound()->getPropertyAsString(SampleIds::FileName));
	else
		setTooltip(MidiMessage::getMidiNoteName(notePosition, true, true, 3));

	setMouseCursor(isDragOperation(e) ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
};

void SamplerSoundMap::mouseDrag(const MouseEvent &e)
{
	if(sampleDraggingEnabled)
	{
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

		if (e.mods.isShiftDown())
		{
			if (std::abs(e.getDistanceFromDragStartX()) > std::abs(e.getDistanceFromDragStartY()))
				thisDragDeltaY = 0;
			else
				thisDragDeltaX = 0;
		}
		
		if (e.mods.isCommandDown())
		{
			thisDragDeltaY -= thisDragDeltaY % 10;
			thisDragDeltaX -= thisDragDeltaX % 12;
		}

		if (lowestKey + thisDragDeltaX >= 0 && highestKey + thisDragDeltaX < 128)
			currentDragDeltaX = thisDragDeltaX;
		else
		{
			if (thisDragDeltaX < 0)
				currentDragDeltaX = -lowestKey;
			else
				currentDragDeltaX = 127 - highestKey;
		}

		if (lowestVelocity + thisDragDeltaY >= 0 && highestVelocity + thisDragDeltaY < 128)
			currentDragDeltaY = thisDragDeltaY;
		else
		{
			if (thisDragDeltaY < 0)
				currentDragDeltaY = -lowestVelocity;
			else
				currentDragDeltaY = 127 - highestVelocity;
		}
		
		if (e.mods.isAltDown())
			setMouseCursor(MouseCursor::CopyingCursor);
		else
			setMouseCursor(MouseCursor::DraggingHandCursor);

#if 0
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
#endif



	}
	else
	{
		sampleLasso->dragLasso(e);
	}
    
    refreshGraphics();
}

void SamplerSoundMap::setPressedKeys(const uint8 *pressedKeyData)
{
	for(int i = 0; i < 127; i++)
	{
		const int number = i;
		const int velocity = pressedKeyData[i];

		const bool change = velocity != pressedKeys[i];

		if(change)
		{
			for (int j = 0; j < sampleComponents.size(); j++)
			{
				if (sampleComponents[j]->isVisible() && sampleComponents[j]->getSound() != nullptr &&
					sampleComponents[j]->getSound()->appliesToMessage(1, number, velocity) &&
					sampleComponents[j]->getSound()->appliesToRRGroup(ownerSampler->getSamplerDisplayValues().currentGroup))
				{
					sampleComponents[j]->setSampleIsPlayed(velocity > 0);
				}
			}
		}

		pressedKeys[i] = (uint8)velocity;
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
	sampleDraggingEnabled = isDragOperation(e);

	if(sampleDraggingEnabled)
	{
		dragStartData.clear();

		currentDragDeltaX = 0;
		currentDragDeltaY = 0;

		for(int i = 0; i < selectedSounds->getNumSelected(); i++)
		{
			if (auto sc = selectedSounds->getSelectedItem(i))
			{
				DragData d;

				d.sound = sc->getSound();

				d.root = d.sound->getSampleProperty(SampleIds::Root);
				d.lowKey = d.sound->getSampleProperty(SampleIds::LoKey);
				d.hiKey = d.sound->getSampleProperty(SampleIds::HiKey);
				d.loVel = d.sound->getSampleProperty(SampleIds::LoVel);
				d.hiVel = d.sound->getSampleProperty(SampleIds::HiVel);

				dragStartData.add(d);
			}
		}
	}
}


void SamplerSoundMap::setSelectedIds(const SampleSelection& newSelectionList)
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
	handler->setDisplayOnlyRRGroup(groupIndex);

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

	ModulatorSampler::SoundIterator sIter(ownerSampler, false);

	if (sIter.size() != sampleComponents.size())
		return true;

	int i = 0;

	while (auto sound = sIter.getNextSound())
	{
		if (auto sc = sampleComponents[i])
		{
			const ModulatorSamplerSound *s = sc->getSound();

			i++;

			if (s == nullptr)
				return true;
			if (s != sound)
				return true;
		}
		else
			break;


		
	}

	return false;
}

// =================================================================================================================== MapWithKeyboard

MapWithKeyboard::MapWithKeyboard(ModulatorSampler *ownerSampler):
	sampler(ownerSampler),
	lastNoteNumber(-1)
{
    setBufferedToImage(true);
    
	addAndMakeVisible(map = new SamplerSoundMap(ownerSampler));
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
		Colour keyColour = selectedRootNotes[i] ? blackKeys[i % 12] ? Colour(SIGNAL_COLOUR).withMultipliedBrightness(0.5f) :
                                                                      Colour(SIGNAL_COLOUR) :
													blackKeys[i % 12] ? Colours::black : Colours::white;

		if(i == lastNoteNumber) keyColour = Colour(SIGNAL_COLOUR);

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
	Rectangle<int> keyboardArea(0, map->getBottom(), getWidth(), 20);

	if (!keyboardArea.contains(e.getMouseDownPosition()))
	{
		return;
	}

	lastNoteNumber = (128 * e.getMouseDownPosition().getX()) / map->getWidth();

	const int velocity = (int)(127.0f * ((float)(e.getMouseDownY() - keyboardArea.getY()) / 20.0f));

	HiseEvent m(HiseEvent::Type::NoteOn, (uint8)lastNoteNumber, (uint8)velocity, 1);
	m.setArtificial();

	sampler->getMainController()->getEventHandler().pushArtificialNoteOn(m);

    ScopedLock sl(sampler->getMainController()->getLock());
	sampler->preHiseEventCallback(m);
	sampler->noteOn(m);

	repaint();
}

void MapWithKeyboard::mouseUp(const MouseEvent &e)
{
	Rectangle<int> keyboardArea(0, map->getBottom(), getWidth(), 20);

	if (!keyboardArea.contains(e.getMouseDownPosition()))
	{
		return;
	}

	HiseEvent m(HiseEvent::Type::NoteOff, (uint8)lastNoteNumber, 127, 1);
	m.setArtificial();

	m.setEventId(sampler->getMainController()->getEventHandler().getEventIdForNoteOff(m));

	sampler->preHiseEventCallback(m);
    
    ScopedLock sl(sampler->getMainController()->getLock());
	sampler->noteOff(m);

	lastNoteNumber = -1;

	repaint();

	
}

// =================================================================================================================== SamplerSoundTable

SamplerSoundTable::SamplerSoundTable(ModulatorSampler *ownerSampler_, SampleEditHandler* handler)   :
	SamplerSubEditor(handler),
	PreloadListener(ownerSampler_->getMainController()->getSampleManager()),
	font (GLOBAL_FONT()),
	ownerSampler(ownerSampler_),
	internalSelection(false)
{
    // Create our table component and add it to this component..
    addAndMakeVisible (table);
    table.setModel (this);

    // give it a border
    table.setColour (ListBox::outlineColourId, Colours::grey);
	table.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

    table.setOutlineThickness (0);


	columnIds.add(SampleIds::ID);
	columnIds.add(SampleIds::FileName);
	columnIds.add(SampleIds::RRGroup);
	columnIds.add(SampleIds::Root);
	columnIds.add(SampleIds::LoKey);
	columnIds.add(SampleIds::HiKey);
	columnIds.add(SampleIds::LoVel);
	columnIds.add(SampleIds::HiVel);

	for (auto c : columnIds)
	{
		int i1, i2, i3 = 50;

		if (c == SampleIds::FileName)
		{
			i1 = 320;
			i2 = 220;
			i3 = 1200;
		}
		else
		{
			i1 = 40;
			i2 = 30;
			i3 = 80;
		}

		table.getHeader().addColumn(c.toString(), columnIds.indexOf(c)+1, i1, i2, i3, TableHeaderComponent::ColumnPropertyFlags::sortable | TableHeaderComponent::resizable | TableHeaderComponent::visible);
	}

	table.getHeader().setLookAndFeel(&laf);

	table.setHeaderHeight(18);

	table.setMultipleSelectionEnabled (true);

	table.getHeader().setStretchToFitActive(true);
	
	refreshList();
}

SamplerSoundTable::~SamplerSoundTable()
{
	table.getHeader().setLookAndFeel(nullptr);
}

void SamplerSoundTable::refreshList()
{
	if (isPreloading)
		return;

	sortedSoundList.clear();

	auto sortId = table.getHeader().getSortColumnId();

	bool forward = table.getHeader().isSortedForwards();

	if (sortId == 0)
	{
		// sort forwards by the ID column
		sortId = 2;
		forward = true;
	}

	ModulatorSampler::SoundIterator sIter(ownerSampler, false);

	while (auto sound = sIter.getNextSound())
		sortedSoundList.add(sound.get());

	// we could now change some initial settings..
	table.getHeader().setSortColumnId(sortId, forward);
	sortOrderChanged(sortId, forward);

	table.updateContent();

	resized();
}

int SamplerSoundTable::getNumRows() { return ownerSampler->getNumSounds(); }

bool SamplerSoundTable::broadcasterIsSelection(ChangeBroadcaster *b) const
{
	return dynamic_cast<SelectedItemSet<ModulatorSamplerSound*>*>(b) != nullptr;
}

	
void SamplerSoundTable::preloadStateChanged(bool isPreloading_)
{
	isPreloading = isPreloading_;

	if (!isPreloading)
		refreshList();
}

void SamplerSoundTable::paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
	if(rowNumber % 2) g.fillAll(Colours::white.withAlpha(0.05f));

    if (rowIsSelected)
    {
        g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.6f));
        g.fillAll();
        g.setColour(Colours::black.withAlpha(0.1f));
        g.drawHorizontalLine(height-1, 0.0, (float)width);
        
    }
    
}

void SamplerSoundTable::paintCell (Graphics& g, int rowNumber, int columnId,
                int width, int height, bool rowIsSelected)
{

	if (rowNumber < sortedSoundList.size())
	{
        if(rowIsSelected)
        {
            g.setFont(GLOBAL_BOLD_FONT());
            g.setColour(Colours::black.withAlpha(.8f));
        }
        else
        {
            g.setFont(font);
            g.setColour(Colours::white.withAlpha(.8f));
        }
		
		
		if (sortedSoundList[rowNumber].get() == nullptr)
			return;


		auto id = columnIds[columnId-1];

		String text(sortedSoundList[rowNumber]->getPropertyAsString(id));

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
		DemoDataSorter sorter (columnIds[newSortColumnId-1], isForwards);

		sortedSoundList.sort(sorter);
        table.updateContent();
    }
}

void SamplerSoundTable::soundsSelected(const SampleSelection &selectedSounds)
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


void SamplerSoundTable::refreshPropertyForRow(int index, const Identifier& id)
{
	if (columnIds.contains(id))
		table.repaintRow(index);
}

void SamplerSoundTable::selectedRowsChanged(int /*lastRowSelected*/)
{
	if(internalSelection) return; // Quick hack to disable the endless loop

	SparseSet<int> selection = table.getSelectedRows();

	handler->getSelection().deselectAll();

	for(int i = 0; i < selection.size(); i++)
	{
		handler->getSelection().addToSelection(sortedSoundList[selection[i]]);
	}
};

} // namespace hise
