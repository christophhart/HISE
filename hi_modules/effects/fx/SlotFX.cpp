/*
  ==============================================================================

    SlotFX.cpp
    Created: 28 Jun 2017 2:51:50pm
    Author:  Christoph

  ==============================================================================
*/

namespace hise { using namespace juce;

SlotFX::SlotFX(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid)
{
	finaliseModChains();

	createList();

	clearEffect();
}

ProcessorEditorBody * SlotFX::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new SlotFXEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif

	
}

void SlotFX::handleHiseEvent(const HiseEvent &m)
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->handleHiseEvent(m);
		}
	}
}

void SlotFX::startMonophonicVoice()
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->startMonophonicVoice();
		}
	}
}

void SlotFX::stopMonophonicVoice()
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->stopMonophonicVoice();
		}
	}
}

void SlotFX::resetMonophonicVoice()
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			w->resetMonophonicVoice();
		}
	}
}

void SlotFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (isClear)
		return;

	if (auto w = wrappedEffect.get())
	{
		if (!w->isSoftBypassed())
		{
			wrappedEffect->renderAllChains(0, buffer.getNumSamples());

			if (buffer.getNumChannels() > 2)
			{
				auto l = getLeftSourceChannel();
				auto r = getRightSourceChannel();

				if (l + r != 1)
				{
					float* ptr[2] = { buffer.getWritePointer(l), buffer.getWritePointer(r) };
					AudioSampleBuffer mBuffer(ptr, 2, buffer.getNumSamples());
					wrappedEffect->renderWholeBuffer(mBuffer);
					return;
				}
			}

			wrappedEffect->renderWholeBuffer(buffer);
		}
	}
}

bool SlotFX::swap(HotswappableProcessor* otherSwap)
{
	if (auto otherSlot = dynamic_cast<SlotFX*>(otherSwap))
	{
		auto te = wrappedEffect.release();
		auto oe = otherSlot->wrappedEffect.release();

		int tempIndex = currentIndex;

		currentIndex = otherSlot->currentIndex;
		otherSlot->currentIndex = tempIndex;

		{
			ScopedLock sl(getMainController()->getLock());

			bool tempClear = isClear;
			isClear = otherSlot->isClear;
			otherSlot->isClear = tempClear;

			wrappedEffect = oe;
			otherSlot->wrappedEffect = te;
		}

		wrappedEffect.get()->sendRebuildMessage(true);
		otherSlot->wrappedEffect.get()->sendRebuildMessage(true);

		sendChangeMessage();
		otherSlot->sendChangeMessage();

		return true;
	}

	return false;
}

hise::Processor* SlotFX::getCurrentEffect()
{
	if (wrappedEffect != nullptr)
		return wrappedEffect.get();


	jassertfalse;
	return nullptr;
}

bool SlotFX::setEffect(const String& typeName, bool /*synchronously*/)
{
	LockHelpers::freeToGo(getMainController());

	int index = effectList.indexOf(typeName);

	if (currentIndex == index)
		return true;

	if (index != -1)
	{
		ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);

		f->setConstrainer(new Constrainer());

		currentIndex = index;

		if (auto p = f->createProcessor(f->getProcessorTypeIndex(typeName), typeName))
		{
			if (getSampleRate() > 0)
				p->prepareToPlay(getSampleRate(), getLargestBlockSize());

			p->setParentProcessor(this);
			auto newId = getId() + "_" + p->getId();
			p->setId(newId);
			ScopedPointer<MasterEffectProcessor> pendingDeleteProcessor;

			if (wrappedEffect != nullptr)
			{
				LOCK_PROCESSING_CHAIN(this);
				wrappedEffect->setIsOnAir(false);
				wrappedEffect.swapWith(pendingDeleteProcessor);
				
			}

			if (pendingDeleteProcessor != nullptr)
				getMainController()->getGlobalAsyncModuleHandler().removeAsync(pendingDeleteProcessor.release(), ProcessorFunction());

			{
				LOCK_PROCESSING_CHAIN(this);

				wrappedEffect = dynamic_cast<MasterEffectProcessor*>(p);
				wrappedEffect->setIsOnAir(isOnAir());
				wrappedEffect->setKillBuffer(*(this->killBuffer));
				isClear = wrappedEffect == nullptr || dynamic_cast<EmptyFX*>(wrappedEffect.get()) != nullptr;;
			}

			if (auto sp = dynamic_cast<JavascriptProcessor*>(wrappedEffect.get()))
			{
				hasScriptFX = true;
				sp->compileScript();
			}

			return true;
		}
		else
		{
			// Should have been catched before...
			jassertfalse;

			clearEffect();
			return true;
		}
	}
	else
	{
		jassertfalse;
		clearEffect();
		return false;
	}
}

void SlotFX::createList()
{
	ScopedPointer<FactoryType> f = new EffectProcessorChainFactoryType(128, this);
	f->setConstrainer(new Constrainer());
	auto l = f->getAllowedTypes();

	for (int i = 0; i < l.size(); i++)
		effectList.add(l[i].type.toString());

	f = nullptr;
}

#if USE_BACKEND
struct HardcodedMasterEditor : public ProcessorEditorBody
{
	static constexpr int Margin = 10;

	HardcodedMasterEditor(ProcessorEditor* pe) :
		ProcessorEditorBody(pe)
	{
		getEffect()->effectUpdater.addListener(*this, update, true);

		auto networkList = getEffect()->getListOfAvailableNetworks();

		selector.addItem("No network", 1);
		selector.addItemList(networkList, 2);
		selector.onChange = BIND_MEMBER_FUNCTION_0(HardcodedMasterEditor::onEffectChange);

		getProcessor()->getMainController()->skin(selector);

		addAndMakeVisible(selector);

		selector.setText(getEffect()->currentEffect, dontSendNotification);

		rebuildParameters();

	};

	static void update(HardcodedMasterEditor& ed, String newEffect, bool complexDataChanged, int numParameters)
	{
		ed.rebuildParameters();
		ed.selector.setText(newEffect, dontSendNotification);
		ed.repaint();
	}

	void onEffectChange()
	{
		getEffect()->setEffect(selector.getText(), true);
		repaint();
	}

	void paint(Graphics& g) override 
	{
		if (getEffect()->opaqueNode != nullptr && !getEffect()->channelCountMatches)
		{
			g.setColour(Colours::white.withAlpha(0.5f));
			g.setFont(GLOBAL_BOLD_FONT());

			auto ta = selector.getBounds().translated(0, 40).toFloat();

			g.drawText("Channel mismatch!", ta, Justification::centredTop);

			String e;
			
			e << "Expected: " << String(getEffect()->opaqueNode->numChannels) << ", Actual: " << String(getEffect()->numChannelsToRender);

			g.drawText("Channel mismatch!", ta, Justification::centredTop);
			g.drawText(e, ta, Justification::centredBottom);
		}

	}

	void rebuildParameters()
	{
		currentEditors.clear();
		currentParameters.clear();

		if (auto on = getEffect()->opaqueNode.get())
		{
			ExternalData::forEachType([&](ExternalData::DataType dt)
				{
					int numObjects = on->numDataObjects[(int)dt];

					for (int i = 0; i < numObjects; i++)
					{
						auto f = ExternalData::createEditor(getEffect()->getComplexBaseType(dt, i));

						auto c = dynamic_cast<Component*>(f);

						currentEditors.add(f);
						addAndMakeVisible(c);
					}
				});

			for (int i = 0; i < on->numParameters; i++)
			{
				auto pData = on->parameters[i];
				auto s = new HiSlider(pData.getId());
				addAndMakeVisible(s);
				s->setup(getProcessor(), i, pData.getId());
				auto nr = pData.toRange().rng;

				s->setRange(pData.min, pData.max, jmax<double>(0.001, pData.interval));
				s->setSkewFactor(pData.skew);
				s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
				s->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
				s->setColour(Slider::thumbColourId, Colour(0x80666666));
				s->setColour(Slider::textBoxTextColourId, Colours::white);

				currentParameters.add(s);
			}
		}

		refreshBodySize();
		resized();
		updateGui();
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(Margin);

		b.removeFromTop(Margin);

		auto sb = b.removeFromLeft(200);

		b.removeFromLeft(Margin);

		selector.setBounds(sb.removeFromTop(28));

		Rectangle<int> currentRow;

		for (auto e : currentEditors)
		{
			dynamic_cast<Component*>(e)->setBounds(b.removeFromTop(120));
			b.removeFromTop(Margin);
		}

		for (int i = 0; i < currentParameters.size(); i++)
		{
			if ((i % 4) == 0)
			{
				currentRow = b.removeFromTop(48);
				b.removeFromTop(Margin);
			}

			dynamic_cast<Component*>(currentParameters[i])->setBounds(currentRow.removeFromLeft(128));
			currentRow.removeFromLeft(Margin);
		}
	}

	HardcodedSwappableEffect* getEffect() { return dynamic_cast<HardcodedSwappableEffect*>(getProcessor()); }
	const HardcodedSwappableEffect* getEffect() const { return dynamic_cast<const HardcodedSwappableEffect*>(getProcessor()); }

	int getBodyHeight() const override
	{
		if (currentParameters.isEmpty() && currentEditors.isEmpty())
			return 32 + 2 * Margin;
		else
		{
			int numRows = currentParameters.size() / 4 + 1;

			int numEditorRows = currentEditors.size();

			return numRows * (48 + 4 * Margin) + numEditorRows * (120 + Margin);
		}
	}

	void updateGui() override
	{
		for (auto p : currentParameters)
			p->updateValue();
	}

	OwnedArray<ComplexDataUIBase::EditorBase> currentEditors;

	OwnedArray<MacroControlledObject> currentParameters;

	ComboBox selector;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HardcodedMasterEditor);
};
#endif

HardcodedSwappableEffect::HardcodedSwappableEffect(MainController* mc, bool isPolyphonic) :
	HotswappableProcessor(),
	ProcessorWithExternalData(mc),
	polyHandler(isPolyphonic),
	mc_(mc)
{
	tempoSyncer.publicModValue = &modValue;
	polyHandler.setTempoSyncer(&tempoSyncer);
	mc->addTempoListener(&tempoSyncer);

#if USE_BACKEND
	auto dllManager = dynamic_cast<BackendProcessor*>(mc)->dllManager.get();
	dllManager->loadDll(false);
	factory = new scriptnode::dll::DynamicLibraryHostFactory(dllManager->projectDll);
#else
	factory = FrontendHostFactory::createStaticFactory();
#endif
}

HardcodedSwappableEffect::~HardcodedSwappableEffect()
{
	mc_->removeTempoListener(&tempoSyncer);

	if (opaqueNode != nullptr)
	{
		factory->deinitOpaqueNode(opaqueNode);
		opaqueNode = nullptr;
	}

	factory = nullptr;
}

bool HardcodedSwappableEffect::setEffect(const String& factoryId, bool /*unused*/)
{
	if (factoryId == currentEffect)
		return true;

	auto idx = getListOfAvailableNetworks().indexOf(factoryId);

	ScopedPointer<OpaqueNode> newNode;
	listeners.clear();

	if (idx != -1)
	{
		currentEffect = factoryId;
		newNode = new OpaqueNode();

		if (!factory->initOpaqueNode(newNode, idx, isPolyphonic()))
			newNode = nullptr;

		bool somethingChanged = false;

		// Create all complex data types we need...
		ExternalData::forEachType([&](ExternalData::DataType dt)
		{
			auto numObjects = newNode->numDataObjects[(int)dt];
			somethingChanged |= (getNumDataObjects(dt) != numObjects);

			for (int i = 0; i < numObjects; i++)
			{
				auto ed = getComplexBaseType(dt, i);
				listeners.add(new DataWithListener(*this, ed, i, newNode.get()));
			}
		});

		prepareOpaqueNode(newNode);

		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);
			std::swap(newNode, opaqueNode);

			for (int i = 0; i < opaqueNode->numParameters; i++)
				lastParameters[i] = opaqueNode->parameters[i].defaultValue;

			checkHardcodedChannelCount();
		}		

		asProcessor().parameterNames.clear();

		for (int i = 0; i < opaqueNode->numParameters; i++)
        {
            parameterRanges.set(i, opaqueNode->parameters[i].toRange());
			asProcessor().parameterNames.add(opaqueNode->parameters[i].getId());
            
            if(auto cp = asProcessor().getChildProcessor(i))
            {
                if(auto modChain = dynamic_cast<ModulatorChain*>(cp))
                {
                    auto rng = parameterRanges[i].rng;
                    
                    auto bipolar = rng.start < 0.0 && rng.end > 0.0;
                    modChain->setMode(bipolar ? Modulation::PanMode : Modulation::GainMode, sendNotificationAsync);
                }
            }
        }

		effectUpdater.sendMessage(sendNotificationAsync, currentEffect, somethingChanged, opaqueNode->numParameters);

		

		tempoSyncer.tempoChanged(mc_->getBpm());
	}
	else
	{
		currentEffect = {};

		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);
			std::swap(newNode, opaqueNode);
		}

		effectUpdater.sendMessage(sendNotificationAsync, currentEffect, true, 0);
	}

	if (newNode != nullptr)
	{
		// We need to deinitialise it with the DLL to prevent heap corruption
		factory->deinitOpaqueNode(newNode);
		newNode = nullptr;
	}

	return opaqueNode != nullptr;
}

bool HardcodedSwappableEffect::swap(HotswappableProcessor* other)
{
	if (auto otherFX = dynamic_cast<HardcodedSwappableEffect*>(other))
	{
		if (otherFX->isPolyphonic() != isPolyphonic())
			return false;

		std::swap(treeWhenNotLoaded, otherFX->treeWhenNotLoaded);
		std::swap(currentEffect, otherFX->currentEffect);

		auto& ap = asProcessor();
		auto& op = otherFX->asProcessor();


		ap.parameterNames.swapWith(op.parameterNames);
		tables.swapWith(otherFX->tables);
		sliderPacks.swapWith(otherFX->sliderPacks);
		audioFiles.swapWith(otherFX->audioFiles);
		filterData.swapWith(otherFX->filterData);
		displayBuffers.swapWith(otherFX->displayBuffers);
		listeners.swapWith(otherFX->listeners);

		for (int i = 0; i < OpaqueNode::NumMaxParameters; i++)
		{
			std::swap(lastParameters[i], otherFX->lastParameters[i]);
		}

		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);
			SimpleReadWriteLock::ScopedWriteLock sl2(otherFX->lock);

			std::swap(opaqueNode, otherFX->opaqueNode);
		}
		
		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);
			SimpleReadWriteLock::ScopedWriteLock sl2(otherFX->lock);

			ap.prepareToPlay(ap.getSampleRate(), ap.getLargestBlockSize());
			op.prepareToPlay(op.getSampleRate(), op.getLargestBlockSize());
		}
		
		effectUpdater.sendMessage(sendNotificationAsync, 
								  currentEffect, 
								  true, 
								  opaqueNode != nullptr ? opaqueNode->numParameters : 0);

		otherFX->effectUpdater.sendMessage(sendNotificationAsync, 
										   otherFX->currentEffect, 
										   true, 
										   otherFX->opaqueNode != nullptr ? otherFX->opaqueNode->numParameters : 0);

		return true;
	}

	return false;
}

void HardcodedSwappableEffect::setHardcodedAttribute(int index, float newValue)
{
	lastParameters[index] = newValue;

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr && isPositiveAndBelow(index, opaqueNode->numParameters))
		opaqueNode->parameterFunctions[index](opaqueNode->parameterObjects[index], (double)newValue);
}

float HardcodedSwappableEffect::getHardcodedAttribute(int index) const
{
	if (isPositiveAndBelow(index, OpaqueNode::NumMaxParameters))
		return lastParameters[index];

	return 0.0f;
}

juce::Path HardcodedSwappableEffect::getHardcodedSymbol() const
{
	Path p;
	p.loadPathFromData(HnodeIcons::freezeIcon, sizeof(HnodeIcons::freezeIcon));
	return p;
}

hise::ProcessorEditorBody* HardcodedSwappableEffect::createHardcodedEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND
	return new HardcodedMasterEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;
#endif
}

void HardcodedSwappableEffect::restoreHardcodedData(const ValueTree& v)
{
	if (factory->getNumNodes() == 0)
	{
		treeWhenNotLoaded = v.createCopy();
		return;
	}

	auto effect = v.getProperty("Network", "No Effect").toString();

	setEffect(effect, false);

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
	{
		ExternalData::forEachType([&](ExternalData::DataType dt)
		{
			if (dt == ExternalData::DataType::DisplayBuffer ||
				dt == ExternalData::DataType::FilterCoefficients)
				return;

			auto parentId = Identifier(ExternalData::getDataTypeName(dt, true));

			auto dataTree = v.getChildWithName(parentId);

			jassert(dataTree.getNumChildren() == opaqueNode->numDataObjects[(int)dt]);

			int index = 0;
			for (auto d : dataTree)
			{
				if (auto cd = getComplexBaseType(dt, index))
				{
					auto b64 = d[PropertyIds::EmbeddedData].toString();
					cd->fromBase64String(b64);

					if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(cd))
					{
						auto min = (int)d[PropertyIds::MinValue];
						auto max = (int)d[PropertyIds::MaxValue];
						af->setRange({ min, max });
					}
				}

				index++;
			}
		});

		for (int i = 0; i < opaqueNode->numParameters; i++)
		{
			auto value = v.getProperty(opaqueNode->parameters[i].getId(), opaqueNode->parameters[i].defaultValue);
			setHardcodedAttribute(i, value);
		}
	}
}

ValueTree HardcodedSwappableEffect::writeHardcodedData(ValueTree& v) const
{
	if (factory->getNumNodes() == 0)
	{
		jassert(treeWhenNotLoaded.isValid());
		return treeWhenNotLoaded;
	}

	v.setProperty("Network", currentEffect, nullptr);

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
	{
		for (int i = 0; i < opaqueNode->numParameters; i++)
		{
			auto id = opaqueNode->parameters[i].getId();
			auto value = lastParameters[i];
			v.setProperty(id, value, nullptr);
		}

		ExternalData::forEachType([&](ExternalData::DataType dt)
		{
			if (dt == ExternalData::DataType::DisplayBuffer ||
				dt == ExternalData::DataType::FilterCoefficients)
				return;

			int numObjects = opaqueNode->numDataObjects[(int)dt];

			ValueTree dataTree(ExternalData::getDataTypeName(dt, true));

			for (int i = 0; i < numObjects; i++)
			{
				ValueTree d(ExternalData::getDataTypeName(dt, false));

				jassert(isPositiveAndBelow(i, getNumDataObjects(dt)));
				auto constThis = const_cast<HardcodedSwappableEffect*>(this);

				d.setProperty(PropertyIds::EmbeddedData, constThis->getComplexBaseType(dt, i)->toBase64String(), nullptr);

				if (dt == ExternalData::DataType::AudioFile)
				{
					auto range = audioFiles[i]->getCurrentRange();
					d.setProperty(PropertyIds::MinValue, range.getStart(), nullptr);
					d.setProperty(PropertyIds::MaxValue, range.getEnd(), nullptr);
				}

				dataTree.addChild(d, -1, nullptr);
			}

			if (dataTree.getNumChildren() > 0)
				v.addChild(dataTree, -1, nullptr);
		});
	}

	return v;
}

void HardcodedSwappableEffect::checkHardcodedChannelCount()
{
	numChannelsToRender = 0;

	memset(channelIndexes, 0, sizeof(int)*NUM_MAX_CHANNELS);

	auto asrp = dynamic_cast<RoutableProcessor*>(this);

	jassert(asrp != nullptr);

	for (int i = 0; i < asrp->getMatrix().getNumSourceChannels(); i++)
	{
		auto c = asrp->getMatrix().getConnectionForSourceChannel(i);

		if (c != -1)
			channelIndexes[numChannelsToRender++] = c;
	}

	if (opaqueNode != nullptr)
	{
		channelCountMatches = opaqueNode->numChannels == numChannelsToRender;
	}
}

bool HardcodedSwappableEffect::processHardcoded(AudioSampleBuffer& b, HiseEventBuffer* e, int startSample, int numSamples)
{
	if (opaqueNode != nullptr)
	{
		if (!channelCountMatches)
			return false;

		auto d = (float**)alloca(sizeof(float*) * numChannelsToRender);

		for (int i = 0; i < numChannelsToRender; i++)
		{
			d[i] = b.getWritePointer(channelIndexes[i], startSample);
		}

		ProcessDataDyn pd(d, numSamples, numChannelsToRender);

		if (e != nullptr)
			pd.setEventBuffer(*e);

		opaqueNode->process(pd);

		return true;
	}


	return false;
}

bool HardcodedSwappableEffect::hasHardcodedTail() const
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
		return opaqueNode->hasTail();

	return false;
}

void HardcodedSwappableEffect::prepareOpaqueNode(OpaqueNode* n)
{
	if (n != nullptr && asProcessor().getSampleRate() > 0.0 && asProcessor().getLargestBlockSize() > 0)
	{
		PrepareSpecs ps;
		ps.numChannels = numChannelsToRender;
		ps.blockSize = asProcessor().getLargestBlockSize();
		ps.sampleRate = asProcessor().getSampleRate();
		ps.voiceIndex = &polyHandler;
		n->prepare(ps);
		n->reset();

		auto e = factory->getError();

		if (e.error != Error::OK)
		{
			jassertfalse;
		}
	}
}







int HardcodedSwappableEffect::getNumDataObjects(ExternalData::DataType t) const
{
	switch (t)
	{
	case ExternalData::DataType::Table: return tables.size();
	case ExternalData::DataType::SliderPack: return sliderPacks.size();
	case ExternalData::DataType::AudioFile: return audioFiles.size();
	case ExternalData::DataType::DisplayBuffer: return displayBuffers.size();
	case ExternalData::DataType::FilterCoefficients: return 0;
	default: jassertfalse; return 0;
	}
}

juce::StringArray HardcodedSwappableEffect::getListOfAvailableNetworks() const
{
	jassert(factory != nullptr);

	StringArray sa;

	int numNodes = factory->getNumNodes();

	for (int i = 0; i < numNodes; i++)
		sa.add(factory->getId(i));

	return sa;

}





HardcodedMasterFX::HardcodedMasterFX(MainController* mc, const String& uid) :
	MasterEffectProcessor(mc, uid),
	HardcodedSwappableEffect(mc, false)
{
#if NUM_HARDCODED_FX_MODS
	for (int i = 0; i < NUM_HARDCODED_FX_MODS; i++)
	{
		String p;
		p << "P" << String(i + 1) << " Modulation";
		modChains += { this, p };
	}

	finaliseModChains();

	for (int i = 0; i < NUM_HARDCODED_FX_MODS; i++)
		paramModulation[i] = modChains[i].getChain();
#else
	finaliseModChains();
#endif

	getMatrix().setNumAllowedConnections(NUM_MAX_CHANNELS);
	connectionChanged();
}

HardcodedMasterFX::~HardcodedMasterFX()
{
	modChains.clear();
}



bool HardcodedMasterFX::hasTail() const
{
	return hasHardcodedTail();
}

void HardcodedMasterFX::voicesKilled()
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
		opaqueNode->reset();
}

void HardcodedMasterFX::setInternalAttribute(int index, float newValue)
{
	setHardcodedAttribute(index, newValue);
}

juce::ValueTree HardcodedMasterFX::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	return writeHardcodedData(v);
}

void HardcodedMasterFX::restoreFromValueTree(const ValueTree& v)
{
	LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
	MasterEffectProcessor::restoreFromValueTree(v);

	restoreHardcodedData(v);

	
}

float HardcodedMasterFX::getAttribute(int index) const
{

	return getHardcodedAttribute(index);
}

hise::ProcessorEditorBody* HardcodedMasterFX::createEditor(ProcessorEditor *parentEditor)
{
	return createHardcodedEditor(parentEditor);
}

void HardcodedMasterFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	prepareOpaqueNode(opaqueNode.get());
}

juce::Path HardcodedMasterFX::getSpecialSymbol() const
{
	return getHardcodedSymbol();
}

void HardcodedMasterFX::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

#if NUM_HARDCODED_FX_MODS
	float modValues[NUM_HARDCODED_FX_MODS];

	if (opaqueNode != nullptr)
	{
		int numParametersToModulate = jmin(NUM_HARDCODED_FX_MODS, opaqueNode->numParameters);

		for (int i = 0; i < numParametersToModulate; i++)
		{
			auto mv = modChains[i].getOneModulationValue(startSample);

			auto value = lastParameters[i] * mv;
			opaqueNode->parameterFunctions[i](opaqueNode->parameterObjects[i], (double)value);
		}
	}

#endif

	processHardcoded(b, eventBuffer, startSample, numSamples);
}

void HardcodedMasterFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (numChannelsToRender == 2)
	{
		MasterEffectProcessor::renderWholeBuffer(buffer);
	}
	else
	{
		applyEffect(buffer, 0, buffer.getNumSamples());
	}
}

HardcodedPolyphonicFX::HardcodedPolyphonicFX(MainController *mc, const String &uid, int numVoices):
	VoiceEffectProcessor(mc, uid, numVoices),
	HardcodedSwappableEffect(mc, true)
{
	polyHandler.setVoiceResetter(this);
	finaliseModChains();

	getMatrix().setNumAllowedConnections(NUM_MAX_CHANNELS);
	getMatrix().init();
	getMatrix().setOnlyEnablingAllowed(true);
	connectionChanged();
}



float HardcodedPolyphonicFX::getAttribute(int parameterIndex) const
{
	return getHardcodedAttribute(parameterIndex);
}

void HardcodedPolyphonicFX::setInternalAttribute(int parameterIndex, float newValue)
{
	setHardcodedAttribute(parameterIndex, newValue);
}

void HardcodedPolyphonicFX::restoreFromValueTree(const ValueTree &v)
{
	VoiceEffectProcessor::restoreFromValueTree(v);

	DBG(v.createXml()->createDocument(""));

	ValueTree r = v.getChildWithName("RoutingMatrix");

	if (r.isValid())
	{
		getMatrix().restoreFromValueTree(r);
	}

	restoreHardcodedData(v);
}

juce::ValueTree HardcodedPolyphonicFX::exportAsValueTree() const
{
	auto v = VoiceEffectProcessor::exportAsValueTree();

	v.addChild(getMatrix().exportAsValueTree(), -1, nullptr);

	return writeHardcodedData(v);
}

bool HardcodedPolyphonicFX::hasTail() const
{
	return hasHardcodedTail();
}

hise::ProcessorEditorBody * HardcodedPolyphonicFX::createEditor(ProcessorEditor *parentEditor)
{
	return createHardcodedEditor(parentEditor);
}

void HardcodedPolyphonicFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	SimpleReadWriteLock::ScopedReadLock sl(lock);
	prepareOpaqueNode(opaqueNode.get());
}

void HardcodedPolyphonicFX::startVoice(int voiceIndex, const HiseEvent& e)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
	{
		voiceStack.startVoice(*opaqueNode, polyHandler, voiceIndex, e);
	}
}



void HardcodedPolyphonicFX::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	auto ok = processHardcoded(b, nullptr, startSample, numSamples);

	isTailing = ok && voiceStack.containsVoiceIndex(voiceIndex);
}

} // namespace hise
