namespace hise { using namespace juce;

SamplerSubEditor::SamplerSubEditor(SampleEditHandler* handler_) :
	internalChange(false),
	handler(handler_)
{
	handler->allSelectionBroadcaster.addListener(*this, [](SamplerSubEditor& s, int num) { s.soundsSelected(num); }, false);
	handler->selectionBroadcaster.addListener(*this, [](SamplerSubEditor& s, ModulatorSamplerSound::Ptr sound, int micIndex)
	{
			s.soundsSelected(1);
	}, false);
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



SamplerTools::Mode SampleComponent::getModeForSample() const
{
    auto nothing = SamplerTools::Mode::Nothing;
    
    switch(toolMode)
    {
        case SamplerTools::Mode::LoopArea:
            return sound->getSampleProperty(SampleIds::LoopEnabled) ? toolMode : nothing;
        case SamplerTools::Mode::SampleStartArea:
            return sound->getSampleProperty(SampleIds::SampleStartMod) ? toolMode: nothing;
        case SamplerTools::Mode::GainEnvelope:
            return sound->getEnvelope(Modulation::Mode::GainMode) != nullptr ? toolMode : nothing;
        case SamplerTools::Mode::PitchEnvelope:
            return sound->getEnvelope(Modulation::Mode::PitchMode) != nullptr ? toolMode : nothing;
        case SamplerTools::Mode::FilterEnvelope:
            return sound->getEnvelope(Modulation::Mode::PanMode) != nullptr ? toolMode : nothing;
        default:
            return nothing;
    }
}

juce::Colour SampleComponent::getColourForSound(bool wantsOutlineColour) const
{
	if (sound.get() == nullptr) return Colours::transparentBlack;

    auto toolToUse = getModeForSample();
    
    auto base = JUCE_LIVE_CONSTANT_OFF(0.2f);
    auto alpha = JUCE_LIVE_CONSTANT_OFF(0.45f);
    auto delta = JUCE_LIVE_CONSTANT_OFF(0.7f);

    auto b = jlimit(0.0f, 1.0f, transparency + base + (isMainSelection ? delta : 0.0f));
    
    if((int)toolToUse > 0)
    {
        if(!wantsOutlineColour)
        {
            
            return SamplerTools::getToolColour(toolToUse).withAlpha(b);
        }
    }
    
	if (selected || dragSelection)
	{
		if (wantsOutlineColour)
		{
			return Colour(SIGNAL_COLOUR);
		}
		else
		{
            
            
			

			auto w = Colours::white.withAlpha(transparency);
			auto c = Colour(SIGNAL_COLOUR).withBrightness(b).withAlpha(alpha);

            
            
			if (dragSelection)
				return w.interpolatedWith(c, 0.4f);
			else
			{
				if (map->getSampler()->getSampleEditHandler()->applyToMainSelection && !isMainSelection)
					return w.interpolatedWith(c, 0.25f);
				else
					return c;
			}
		}
	}

	

	if (sound->isMissing())
	{
		if (sound->isPurged()) return Colours::violet.withAlpha(0.3f);
		else return Colours::violet.withAlpha(0.3f);
	}
	else
	{
		auto w = sound->hasUnpurgedButUnloadedSounds() ? Colours::grey : Colours::white;

		w = w.withAlpha(transparency);

		if (sound->isPurged()) return Colours::brown.withAlpha(0.3f);
		else return wantsOutlineColour ? w.withAlpha(0.7f) : w;
	}
}

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
		auto strokeColour = getColourForSound(true);
		auto fillColour = getColourForSound(false);

        g.setColour(fillColour);

        g.fillRect(areaInt);
        
        g.setColour(strokeColour);
        g.drawHorizontalLine(areaInt.getY(), area.getX(), area.getRight());
        g.drawHorizontalLine(areaInt.getBottom()-1, area.getX(), area.getRight());
        g.drawVerticalLine(areaInt.getX(), area.getY(), area.getBottom());
        g.drawVerticalLine(areaInt.getRight()-1, area.getY(), area.getBottom());

		if (played)
		{
			g.setColour(fillColour.withMultipliedAlpha(0.4f));
			g.fillRect(area.reduced(3));
		}
    }
}

bool SampleComponent::needsToBeDrawn()
{
	const bool enoughSiblings = numOverlayerSiblings > 4;

	
	return isVisible() && (isSelected() || !enoughSiblings);
}

bool SampleComponent::appliesToGroup(const Array<int>& groupIndexesToCompare) const
{
	if (sound.get() != nullptr)
	{
		for (auto rr : groupIndexesToCompare)
			if (sound->appliesToRRGroup(rr))
				return true;
	}
	
	return false;
}

// =================================================================================================================== SamplerSoundMap

SamplerSoundMap::SamplerSoundMap(ModulatorSampler *ownerSampler_):
	PreloadListener(ownerSampler_->getMainController()->getSampleManager()),
	SimpleTimer(ownerSampler_->getMainController()->getGlobalUIUpdater()),
	ownerSampler(ownerSampler_),
	handler(ownerSampler->getSampleEditHandler()),
	notePosition(-1),
	veloPosition(-1),
	sampleLasso(new LassoComponent<ModulatorSamplerSound::Ptr>())
{
    sampleLasso->setColour(LassoComponent<SampleComponent>::ColourIds::lassoFillColourId, Colours::white.withAlpha(0.1f));
    sampleLasso->setColour(LassoComponent<SampleComponent>::ColourIds::lassoOutlineColourId, Colour(SIGNAL_COLOUR));
    
	ownerSampler->getSampleEditHandler()->noteBroadcaster.addListener(*this, keyChanged);

	ownerSampler->getSampleMap()->addListener(this);

	handler->allSelectionBroadcaster.addListener(*this, selectionChanged);

	memset(pressedKeys, 255, 128);

	addChildComponent(sampleLasso);

	updateSoundData();

	//setOpaque(true);
};

SamplerSoundMap::~SamplerSoundMap()
{
	if (ownerSampler != nullptr)
	{
		ownerSampler->getSampleMap()->removeListener(this);
	}
		
	sampleComponents.clear();
}

void SamplerSoundMap::keyChanged(SamplerSoundMap& map, int noteNumber, int velocity)
{
	map.pressedKeys[noteNumber] = velocity;

	auto currentGroup = map.ownerSampler->getSamplerDisplayValues().currentGroup;

	for (auto s: map.sampleComponents)
	{
		if (!s->isVisible() || s->getSound() == nullptr)
			continue;

		if (currentGroup != -1 && !s->getSound()->appliesToRRGroup(currentGroup))
			continue;

		

		auto thisVelocity = velocity;
		if (thisVelocity == 0)
			thisVelocity = s->getSound()->getSampleProperty(SampleIds::LoVel);

		if (s->getSound()->appliesToMessage(1, noteNumber, thisVelocity))
			s->setSampleIsPlayed(velocity > 0);
	}

	map.repaint();
}

void SamplerSoundMap::setDisplayedSound(SamplerSoundMap& map, ModulatorSamplerSound::Ptr sound, int)
{
	for (SampleComponent* s : map.sampleComponents)
	{
		s->checkSelected(sound);
	}

	map.repaint();
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
		SampleSelection oldSelection;

		for (int i = 0; i < dragStartData.size(); i++)
		{
			DragData d = dragStartData[i];

			oldSelection.add(d.sound);

			if (currentDragDeltaX < 0)
			{
				d.sound->setSampleProperty(SampleIds::Root, d.data.rootNote + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::LoKey, d.data.lowKey + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::HiKey, d.data.highKey + currentDragDeltaX);
			}
			else if (currentDragDeltaX > 0)
			{
				d.sound->setSampleProperty(SampleIds::Root, d.data.rootNote + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::HiKey, d.data.highKey + currentDragDeltaX);
				d.sound->setSampleProperty(SampleIds::LoKey, d.data.lowKey + currentDragDeltaX);
			}

			if (currentDragDeltaY < 0)
			{

				const int lowVelo = jmax<int>(0, d.data.lowVelocity + currentDragDeltaY);
				const int highVelo = jmin<int>(127, d.data.highVelocity + currentDragDeltaY);

				d.sound->setSampleProperty(SampleIds::LoVel, lowVelo);
				d.sound->setSampleProperty(SampleIds::HiVel, highVelo);
			}
			else if (currentDragDeltaY > 0)
			{
				const int lowVelo = jmax<int>(0, d.data.lowVelocity + currentDragDeltaY);
				const int highVelo = jmin<int>(127, d.data.highVelocity + currentDragDeltaY);

				d.sound->setSampleProperty(SampleIds::HiVel, highVelo);
				d.sound->setSampleProperty(SampleIds::LoVel, lowVelo);
			}
		}

		dragStartData.clear();

		auto f2 = [this, oldSelection]()
		{
			handler->getSelectionReference().deselectAll();

			for (auto s : oldSelection)
				handler->getSelectionReference().addToSelection(s);

			handler->setMainSelectionToLast();
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

void SamplerSoundMap::sampleAmountChanged()
{
	auto old = currentSoloGroup;
	currentSoloGroup.clear();

	RepaintSkipper skipper(*this);

	soloGroup(old);
	updateSampleComponents();
}

void SamplerSoundMap::selectionChanged(SamplerSoundMap& map, int numSelected)
{
	BigInteger bi;
	
	for (auto sound : *map.handler)
	{
		bi.setBit((int)sound->getSampleProperty(SampleIds::ID), true);
	}
		
	for (auto c : map.sampleComponents)
	{
		c->setSelected(false, true);
		auto id = (int)c->getSound()->getSampleProperty(PropertyIds::ID);
		c->setSelected(bi[id]);
	}
	
	map.repaint();
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

	if (isPreloading)
		start();
	else
		stop();

	repaintIfIdle();
}

void SamplerSoundMap::modifierKeysChanged(const ModifierKeys &modifiers)
{
	if(modifiers.isAltDown() && handler->getNumSelected() != 0)
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

void SamplerSoundMap::findLassoItemsInArea(Array<ModulatorSamplerSound::Ptr> &itemsFound, const Rectangle< int > &area)
{
	for (auto s : sampleComponents)
	{
		if (!s->isVisible())
			continue;

		s->setSelected(false, true);

		if (itemsFound.contains(s->getSound()))
			continue;

		auto sb = s->getBoundsInParent();
		
		if (area.expanded(1).intersects(sb))
		{
			itemsFound.add(s->getSound());
			s->setSelected(true, true);
		}
	}

	repaintIfIdle();
}

juce::SelectedItemSet<hise::ModulatorSamplerSound::Ptr> & SamplerSoundMap::getLassoSelection()
{
	return dragSet;
}

void SamplerSoundMap::drawSoundMap(Graphics &g)
{
    g.fillAll(Colour(0xff1d1d1d));
    
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
		g.setColour(Colours::white.withAlpha(0.8f));

		auto s = getSampler()->getMainController()->getSampleManager().getPreloadMessage();

		if (s.isEmpty())
			s = "Preloading";

		g.drawText(s, getLocalBounds().toFloat(), Justification::centred);

		auto b = getLocalBounds().toFloat().withSizeKeepingCentre(200.0f, 15.0f).translated(0.0f, 30.0f);

		g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 2.0f);
		b = b.reduced(3.0f);

		auto w = (float)getSampler()->getMainController()->getSampleManager().getPreloadProgressConst() * b.getWidth();

		b.setWidth(jmax(w, b.getHeight()));

		g.fillRoundedRectangle(b, b.getHeight() / 2.0f);
	}
	else
	{
		const float noteWidth = (float)getWidth() / 128.0f;
		const float velocityHeight = (float)getHeight() / 128.0f;

		auto v = getSampler()->getMidiInputLockValue(SampleIds::LoVel);

		if (v != -1)
		{
			float y = (float)(getHeight() - (v * velocityHeight));
			
			g.setColour(Colour(SIGNAL_COLOUR));
			g.drawHorizontalLine(y, 0.0f, (float)getWidth());

			Path p;
			p.loadPathFromData(ColumnIcons::lockIcon, sizeof(ColumnIcons::lockIcon));
			Rectangle<float> d(0.0f, y, 32, 32);
			if (d.getBottom() > getHeight())
				d = d.translated(0, -32);

			PathFactory::scalePath(p, d.reduced(4.0f));
			g.fillPath(p);
		}

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

		if (!dragStartData.isEmpty())
		{
			for (const auto& d: dragStartData)
			{
				const int x = (int)((d.data.lowKey + currentDragDeltaX) * noteWidth);
				const int w = (int)((1 + d.data.highKey - d.data.lowKey) * noteWidth);

				const int y = (int)(getHeight() - (d.data.highVelocity + currentDragDeltaY) * velocityHeight);
				const int h = (int)((127.0f - (float)(d.data.lowVelocity + currentDragDeltaY)) * velocityHeight) - y;

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
		
		repaintIfIdle();
	}

}


void SamplerSoundMap::updateSampleComponents()
{
	RepaintSkipper skipper(*this);

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

		sampleAmountChanged();
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

	hasDraggedSamples = shouldDragSamples(e);

	if (hasDraggedSamples)
	{
		createDragData(e);
	}
	else
	{
		if(!e.mods.isShiftDown() && !e.mods.isCommandDown())
			dragSet.deselectAll();

		setMouseCursor(MouseCursor::NormalCursor);
		sampleLasso->beginLasso(e.getEventRelativeTo(this), this);
	}

    refreshGraphics();
}



void SamplerSoundMap::mouseUp(const MouseEvent &e)
{
	if (e.mods.isRightButtonDown())
		return;

	refreshGraphics();

	if(hasDraggedSamples)
		endSampleDragging(e.mods.isAltDown());
	else
	{
		struct SampleSorter
		{
			static int compareElements(ModulatorSamplerSound::Ptr s1, ModulatorSamplerSound::Ptr s2)
			{
				auto v1 = (int)s1->getSampleProperty(SampleIds::HiVel);
				auto v2 = (int)s2->getSampleProperty(SampleIds::HiVel);

				if (v1 < v2)
					return 1;
				if (v1 > v2)
					return -1;

				auto n1 = (int)s1->getSampleProperty(SampleIds::HiKey);
				auto n2 = (int)s2->getSampleProperty(SampleIds::HiKey);

				if (n1 > n2)
					return 1;
				if (n1 < n2)
					return -1;

				return 0;
			}
		};

		SampleSelection newSelection = dragSet.getItemArray();

		SampleSorter sorter;
		newSelection.sort(sorter);

		sampleLasso->endLasso();

		if (!e.mods.isShiftDown())
			handler->getSelectionReference().deselectAll();

		for(auto s: newSelection)
			handler->getSelectionReference().addToSelection(s);

		handler->setMainSelectionToLast();

		if (auto s = getSampleComponentAt(e.getPosition()))
		{
			handler->selectionBroadcaster.sendMessage(sendNotificationAsync, s->getSound(), 0);
		}
	}
	
	setMouseCursor(shouldDragSamples(e) ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);

    refreshGraphics();
}

void SamplerSoundMap::mouseExit(const MouseEvent &)
{
	notePosition = -1;
	veloPosition = -1;

	draggedFileRootNotes = 0;

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
		setTooltip(c->getSound()->getPropertyAsString(SampleIds::FileName));
	else
		setTooltip(MidiMessage::getMidiNoteName(notePosition, true, true, 3));

	auto shouldDrag = shouldDragSamples(e);

	setMouseCursor(shouldDrag ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
};

void SamplerSoundMap::mouseDrag(const MouseEvent &e)
{
	if(hasDraggedSamples)
	{
		int lowestKey = INT_MAX;
		int highestKey = 0;

		int lowestVelocity = INT_MAX;
		int highestVelocity = 0;

		for (int i = 0; i < dragStartData.size(); i++)
		{
			if (dragStartData[i].data.highKey > highestKey) highestKey = dragStartData[i].data.highKey;
			if (dragStartData[i].data.lowKey < lowestKey) lowestKey = dragStartData[i].data.lowKey;
			if (dragStartData[i].data.highVelocity > highestVelocity) highestVelocity = dragStartData[i].data.highVelocity;
			if (dragStartData[i].data.lowVelocity < lowestVelocity) lowestVelocity = dragStartData[i].data.lowVelocity;
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
	}
	else
	{
		sampleLasso->dragLasso(e);
	}
    
    refreshGraphics();
}
	

SampleComponent* SamplerSoundMap::getSampleComponentAt(Point<int> point) const
{
	for (auto s : sampleComponents)
	{
		if (s->isVisible() && s->samplePathContains(point))
			return s;
	}
	
	return nullptr;
};


void SamplerSoundMap::createDragData(const MouseEvent& e)
{
	dragStartData.clear();
	dragStartData.ensureStorageAllocated(handler->getNumSelected());

	currentDragDeltaX = 0;
	currentDragDeltaY = 0;

	for (auto sound : *handler)
	{
		DragData d;
		d.sound = sound;
		d.data = StreamingHelpers::getBasicMappingDataFromSample(sound->getData());
		dragStartData.add(d);
	}
}

void SamplerSoundMap::soloGroup(BigInteger groupIndex)
{
	if (groupIndex != currentSoloGroup)
	{
		currentSoloGroup = groupIndex;

		Array<int> indexes;

		auto limit = groupIndex.getHighestBit() + 1;

		for (int i = 0; i < limit; i++)
		{
			if (groupIndex[i])
				indexes.add(i + 1);
		}

		for (int i = 0; i < sampleComponents.size(); i++)
		{
			const bool visible = groupIndex.isZero() || sampleComponents[i]->appliesToGroup(indexes);

			sampleComponents[i]->setVisible(visible);
			sampleComponents[i]->setEnabled(visible);
		}

		refreshGraphics();
	}
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

bool SamplerSoundMap::shouldDragSamples(const MouseEvent& e) const
{
	if (handler->getNumSelected() == 0)
		return false;

	auto selection = handler->getSelectionReference().getItemArray();

	auto ok = false;

	for (auto s : sampleComponents)
	{
		if (s->isVisible() && s->isSelected())
			ok |= s->getBoundsInParent().contains(e.getPosition());
	}

	return ok;
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

		table.getHeader().addColumn(c.toString(), columnIds.indexOf(c)+1, i1, i2, i3, TableHeaderComponent::ColumnPropertyFlags::defaultFlags);
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
		sortId = 1;
		forward = true;
	}

	isDefaultOrder = sortId == 1 && forward == true;

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
	auto newDefaultOrder = newSortColumnId == 1 && isForwards;

	bool needsSorting = !newDefaultOrder || (newDefaultOrder != isDefaultOrder);

	isDefaultOrder = newDefaultOrder;

    if (newSortColumnId != 0 && needsSorting)
    {
		DemoDataSorter sorter (columnIds[newSortColumnId-1], isForwards);

		sortedSoundList.sort(sorter);
        table.updateContent();
    }
}

void SamplerSoundTable::soundsSelected(int numSelected)
{
	ScopedValueSetter<bool> svs(internalSelection, true);

	table.deselectAllRows();

    SparseSet<int> selection;
    
	for (int i = 0; i < sortedSoundList.size(); i++)
	{
		ModulatorSamplerSound *sound = sortedSoundList[i].get();

		if (handler->getSelectionReference().isSelected(sound))
			selection.addRange(Range<int>(i, i + 1));
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

	handler->getSelectionReference().deselectAll();

	for(int i = 0; i < selection.size(); i++)
	{
		handler->getSelectionReference().addToSelection(sortedSoundList[selection[i]]);
	}

	handler->setMainSelectionToLast();
};

} // namespace hise
