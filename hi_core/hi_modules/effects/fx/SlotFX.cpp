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

		sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Any);
		otherSlot->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Any);

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
class HardcodedMasterEditor : public ProcessorEditorBody
{
public:

	static constexpr int Margin = 10;

	HardcodedMasterEditor(ProcessorEditor* pe) :
		ProcessorEditorBody(pe)
	{
		getEffect()->effectUpdater.addListener(*this, update, true);
		getEffect()->errorBroadcaster.addListener(*this, onError);

		auto networkList = getEffect()->getModuleList();

		selector.addItem("No network", 1);
		selector.addItemList(networkList, 2);
		selector.onChange = BIND_MEMBER_FUNCTION_0(HardcodedMasterEditor::onEffectChange);

		getProcessor()->getMainController()->skin(selector);

		addAndMakeVisible(selector);

		selector.setText(getEffect()->currentEffect, dontSendNotification);
		rebuildParameters();

	};

	static void onError(HardcodedMasterEditor& editor, const String& r)
	{
		editor.prepareError = r;
		editor.repaint();
	}

	String prepareError;

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

	String getErrorMessage()
	{
		if(prepareError.isNotEmpty())
			return prepareError;

		if (getEffect()->opaqueNode != nullptr && !getEffect()->channelCountMatches)
		{
			String e; 
			e << "Channel mismatch";
			e << "Expected: " << String(getEffect()->opaqueNode->numChannels) << ", Actual: " << String(getEffect()->numChannelsToRender);
			return e;
		}
		auto bp = dynamic_cast<BackendProcessor*>(dynamic_cast<ControlledObject*>(getEffect())->getMainController());

		if (bp->dllManager->projectDll == nullptr)
			return "No DLL loaded";

		return bp->dllManager->projectDll->getInitError();
	}

	void paint(Graphics& g) override 
	{
		auto errorMessage = getErrorMessage();
		if (errorMessage.isNotEmpty())
		{
			g.setColour(Colours::white.withAlpha(0.5f));
			g.setFont(GLOBAL_BOLD_FONT());

			auto ta = body.toFloat();
			
			g.drawText("ERROR: " + errorMessage, ta, Justification::centred);
		}

	}

	void rebuildParameters()
	{
		currentEditors.clear();
		currentParameters.clear();

		if(!getErrorMessage().isEmpty())
			return;

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

			for(const auto& p: OpaqueNode::ParameterIterator(*on))
			{
				auto pData = p.info;
				auto s = new HiSlider(pData.getId());
				addAndMakeVisible(s);
				s->setup(getProcessor(), pData.index, pData.getId());
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

	Rectangle<int> body;

	void resized() override
	{
		auto b = getLocalBounds().reduced(Margin);

		b.removeFromTop(Margin);

		auto sb = b.removeFromLeft(200);

		b.removeFromLeft(Margin);

		selector.setBounds(sb.removeFromTop(28));

		body = b;

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
	factory = scriptnode::DspNetwork::createStaticFactory();
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

void HardcodedSwappableEffect::connectRuntimeTargets()
{
    if(opaqueNode != nullptr)
    {
        dynamic_cast<Processor*>(this)->getMainController()->connectToRuntimeTargets(*opaqueNode, true);
    }
}

void HardcodedSwappableEffect::disconnectRuntimeTargets()
{
    if(opaqueNode != nullptr)
    {
        dynamic_cast<Processor*>(this)->getMainController()->connectToRuntimeTargets(*opaqueNode, false);
        
        factory->deinitOpaqueNode(opaqueNode);
        opaqueNode = nullptr;
    }
}

bool HardcodedSwappableEffect::setEffect(const String& factoryId, bool /*unused*/)
{
	if (factoryId == currentEffect)
		return true;

	auto idx = getModuleList().indexOf(factoryId);

	ScopedPointer<OpaqueNode> newNode;
	listeners.clear();

	if (idx != -1)
	{
		currentEffect = factoryId;
		hash = factory->getHash(idx);
		newNode = new OpaqueNode();

		if (!factory->initOpaqueNode(newNode, idx, isPolyphonic()))
			newNode = nullptr;

        auto mc = dynamic_cast<Processor*>(this)->getMainController();
        mc->connectToRuntimeTargets(*newNode, true);
        
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

		auto ok = prepareOpaqueNode(newNode);

		errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());

		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);
            
			for (auto& p : OpaqueNode::ParameterIterator(*newNode))
			{
				auto defaultValue = p.info.defaultValue;
				p.callback.call(defaultValue);
			}

			std::swap(newNode, opaqueNode);

			numParameters = opaqueNode->numParameters;

			lastParameters.setSize(opaqueNode->numParameters * sizeof(float), true);

			for (auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
			{
				if(auto ptr = getParameterPtr(p.info.index))
					*ptr = (float)p.info.defaultValue;
			}

            channelCountMatches = checkHardcodedChannelCount();
		}		

		asProcessor().parameterNames.clear();

		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			parameterRanges.set(p.info.index, p.info.toRange());
			asProcessor().parameterNames.add(p.info.getId());

			if (auto cp = asProcessor().getChildProcessor(p.info.index))
			{
				if (auto modChain = dynamic_cast<ModulatorChain*>(cp))
				{
					auto rng = parameterRanges[p.info.index].rng;

					auto bipolar = rng.start < 0.0 && rng.end > 0.0;
					modChain->setMode(bipolar ? Modulation::PanMode : Modulation::GainMode, sendNotificationAsync);
				}
			}
		}

		asProcessor().updateParameterSlots();

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

		std::swap(previouslySavedTree, otherFX->previouslySavedTree);
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
			std::swap(lastParameters, otherFX->lastParameters);
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

juce::Result HardcodedSwappableEffect::sanityCheck()
{
	String errorMessage;

	errorMessage << dynamic_cast<Processor*>(this)->getId();
	errorMessage << ":  > ";

	if (!properlyLoaded)
	{
		errorMessage << "Can't find effect in DLL";

		return Result::fail(errorMessage);
	}
	
	if (opaqueNode != nullptr)
	{
		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			Identifier pid(p.info.getId());

			if (previouslySavedTree.isValid() && !previouslySavedTree.hasProperty(pid))
			{
				errorMessage << "Missing parameter: " << pid;
				return Result::fail(errorMessage);
			}
		}
	}

	return Result::ok();
}

void HardcodedSwappableEffect::setHardcodedAttribute(int index, float newValue)
{
	if(auto ptr = getParameterPtr(index))
		*ptr = newValue;

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode == nullptr)
		return;
	
	if (auto p = opaqueNode->getParameter(index))
		p->callback.call((double)newValue);
}

float HardcodedSwappableEffect::getHardcodedAttribute(int index) const
{
	if(auto ptr = getParameterPtr(index))
		return *ptr;

	return 0.0f;
}

juce::Path HardcodedSwappableEffect::getHardcodedSymbol() const
{
	Path p;
	p.loadPathFromData(HnodeIcons::freezeIcon, SIZE_OF_PATH(HnodeIcons::freezeIcon));
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
	previouslySavedTree = v.createCopy();

	auto effect = v.getProperty("Network", "").toString();

	if (factory->getNumNodes() == 0 && effect.isNotEmpty())
	{
		properlyLoaded = false;
		return;
	}

	

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

		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			auto value = v.getProperty(p.info.getId(), p.info.defaultValue);
			setHardcodedAttribute(p.info.index, value);
		}
	}
	else
	{
		properlyLoaded = effect.isEmpty();
	}
}

ValueTree HardcodedSwappableEffect::writeHardcodedData(ValueTree& v) const
{
	if (!properlyLoaded)
	{
		return previouslySavedTree;
	}

	v.setProperty("Network", currentEffect, nullptr);
	
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
	{
		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			auto id = p.info.getId();

			if(auto ptr = getParameterPtr(p.info.index))
				v.setProperty(id, *ptr, nullptr);
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

bool HardcodedSwappableEffect::checkHardcodedChannelCount()
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
		return opaqueNode->numChannels == numChannelsToRender;
	}
    
    return false;
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

		renderData(pd);

		

		return true;
	}


	return false;
}

void HardcodedSwappableEffect::renderData(ProcessDataDyn& data)
{
	jassert(opaqueNode != nullptr);
	opaqueNode->process(data);
}

bool HardcodedSwappableEffect::hasHardcodedTail() const
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
		return opaqueNode->hasTail();

	return false;
}

Result HardcodedSwappableEffect::prepareOpaqueNode(OpaqueNode* n)
{
	if(auto rm = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(asProcessor().getMainController()->getGlobalRoutingManager()))
		tempoSyncer.additionalEventStorage = &rm->additionalEventStorage;

	if (n != nullptr && asProcessor().getSampleRate() > 0.0 && asProcessor().getLargestBlockSize() > 0)
	{
#if USE_BACKEND
		factory->clearError();
#endif

		PrepareSpecs ps;
		ps.numChannels = numChannelsToRender;
		ps.blockSize = asProcessor().getLargestBlockSize();
		ps.sampleRate = asProcessor().getSampleRate();
		ps.voiceIndex = &polyHandler;
		n->prepare(ps);
		n->reset();

#if USE_BACKEND
		auto e = factory->getError();

		if (e.error != Error::OK)
		{
			return Result::fail(ScriptnodeExceptionHandler::getErrorMessage(e));
		}
#endif
	}

	return Result::ok();
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

juce::StringArray HardcodedSwappableEffect::getModuleList() const
{
	if (factory == nullptr)
		return {};

	jassert(factory != nullptr);

	StringArray sa;

	int numNodes = factory->getNumNodes();

	for (int i = 0; i < numNodes; i++)
		sa.add(factory->getId(i));

	return sa;
}

var HardcodedSwappableEffect::getParameterProperties() const
{
    Array<var> list;
    
    if(opaqueNode != nullptr)
    {
		SimpleReadWriteLock::ScopedReadLock sl(lock);

		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			auto key = p.info.getId();
			auto range = p.info.toRange().rng;
			auto defaultValue = p.info.defaultValue;

			Identifier id(key);

			auto prop = new DynamicObject();

			prop->setProperty("text", key);
			prop->setProperty("min", range.start);
			prop->setProperty("max", range.end);
			prop->setProperty("stepSize", range.interval);
			prop->setProperty("middlePosition", range.convertFrom0to1(0.5));
			prop->setProperty("defaultValue", defaultValue);

			list.add(var(prop));
		}
    }
    
    return var(list);
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

bool HardcodedMasterFX::isSuspendedOnSilence() const
{
	if (opaqueNode != nullptr)
		return opaqueNode->isSuspendedOnSilence();

	return true;
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

	auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

juce::Path HardcodedMasterFX::getSpecialSymbol() const
{
	return getHardcodedSymbol();
}

void HardcodedMasterFX::handleHiseEvent(const HiseEvent &m)
{
	HiseEvent copy(m);
	if (opaqueNode != nullptr)
		opaqueNode->handleHiseEvent(copy);
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

			auto p = opaqueNode->getParameter(i);

			if (modChains[i].getChain()->getMode() == Modulation::PanMode)
			{
				if (!modChains[i].getChain()->shouldBeProcessedAtAll())
					mv = 1.0f;

				if(auto ptr = getParameterPtr(i))
					p->callback.call((double)*ptr);
			}
			else
			{
				auto r = p->toRange();

				if(auto ptr = getParameterPtr(i))
				{
					auto normMaxValue = r.convertTo0to1(*ptr, false);
					normMaxValue *= mv;
					auto value = r.convertFrom0to1(normMaxValue, false);

					p->callback.call((double)value);
				}
			}
		}
	}

#endif

	auto canBeSuspended = isSuspendedOnSilence();

	if (canBeSuspended)
	{
		if (masterState.numSilentBuffers > numSilentCallbacksToWait && startSample == 0)
		{
			auto silent = ProcessDataDyn(b.getArrayOfWritePointers(), b.getNumSamples(), b.getNumChannels()).isSilent();
			
			if (silent)
			{
				getMatrix().handleDisplayValues(b, b, false);
				masterState.currentlySuspended = true;
				return;
			}
			else
			{
				masterState.numSilentBuffers = 0;
				masterState.currentlySuspended = false;
			}
		}
	}

	masterState.currentlySuspended = false;
	processHardcoded(b, eventBuffer, startSample, numSamples);

	getMatrix().handleDisplayValues(b, b, false);

	if (canBeSuspended)
	{
		if (ProcessDataDyn(b.getArrayOfWritePointers(), numSamples, b.getNumChannels()).isSilent())
			masterState.numSilentBuffers++;
		else
			masterState.numSilentBuffers = 0;
	}
}

void HardcodedMasterFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (numChannelsToRender == 2)
	{
		// Rewrite the channel indexes, the buffer already points to the
		// correct channels...
		channelIndexes[0] = 0;
		channelIndexes[1] = 1;
		MasterEffectProcessor::renderWholeBuffer(buffer);
	}
	else
	{
		applyEffect(buffer, 0, buffer.getNumSamples());
	}
}


HardcodedTimeVariantModulator::HardcodedTimeVariantModulator(hise::MainController *mc, const String &uid, Modulation::Mode m):
  HardcodedSwappableEffect(mc, false),
  Modulation(m),
  TimeVariantModulator(mc, uid, m)
{
    numChannelsToRender = 1;
}

HardcodedTimeVariantModulator::~HardcodedTimeVariantModulator()
{
    
}

void HardcodedTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
    SimpleReadWriteLock::ScopedReadLock sl(lock);

    if(opaqueNode != nullptr && channelCountMatches)
    {
		auto* modData = internalBuffer.getWritePointer(0, startSample);
        FloatVectorOperations::clear(modData, numSamples);
        
        ProcessDataDyn d(&modData, numSamples, 1);
        opaqueNode->process(d);
    }
}

void HardcodedTimeVariantModulator::handleHiseEvent(const hise::HiseEvent &m)
{
    HiseEvent copy(m);
    if (opaqueNode != nullptr)
        opaqueNode->handleHiseEvent(copy);
}

void HardcodedTimeVariantModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
    
    SimpleReadWriteLock::ScopedReadLock sl(lock);
    auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

hise::ProcessorEditorBody *HardcodedTimeVariantModulator::createEditor(hise::ProcessorEditor *parentEditor)
{
    return createHardcodedEditor(parentEditor);
}

float HardcodedTimeVariantModulator::getAttribute(int index) const
{
    return getHardcodedAttribute(index);
}

void HardcodedTimeVariantModulator::restoreFromValueTree(const juce::ValueTree &v)
{
    LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
    TimeVariantModulator::restoreFromValueTree(v);

    restoreHardcodedData(v);
}

juce::ValueTree HardcodedTimeVariantModulator::exportAsValueTree() const
{
    ValueTree v = TimeVariantModulator::exportAsValueTree();
    return writeHardcodedData(v);
}

void HardcodedTimeVariantModulator::setInternalAttribute(int index, float newValue)
{
    setHardcodedAttribute(index, newValue);
}

bool HardcodedTimeVariantModulator::checkHardcodedChannelCount()
{
    if (opaqueNode != nullptr)
    {
        return opaqueNode->numChannels == 1;
    }
    
    return false;
}

Result HardcodedTimeVariantModulator::prepareOpaqueNode(scriptnode::OpaqueNode *n)
{
    if (n != nullptr && asProcessor().getSampleRate() > 0.0 && asProcessor().getLargestBlockSize() > 0)
    {
        PrepareSpecs ps;
        ps.numChannels = 1;
        ps.blockSize = asProcessor().getLargestBlockSize() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
        ps.sampleRate = asProcessor().getSampleRate() / (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
        ps.voiceIndex = &polyHandler;
        n->prepare(ps);
        n->reset();

#if USE_BACKEND
        auto e = factory->getError();

        if (e.error != Error::OK)
        {
            return Result::fail(ScriptnodeExceptionHandler::getErrorMessage(e));
        }
#endif
    }

	return Result::ok();
}







HardcodedPolyphonicFX::HardcodedPolyphonicFX(MainController *mc, const String &uid, int numVoices):
	VoiceEffectProcessor(mc, uid, numVoices),
	HardcodedSwappableEffect(mc, true)
{
	polyHandler.setVoiceResetter(this);

#if NUM_HARDCODED_POLY_FX_MODS
	for (int i = 0; i < NUM_HARDCODED_POLY_FX_MODS; i++)
	{
		String p;
		p << "P" << String(i + 1) << " Modulation";
		modChains += { this, p };
	}

	finaliseModChains();

	for (int i = 0; i < NUM_HARDCODED_POLY_FX_MODS; i++)
		paramModulation[i] = modChains[i].getChain();
#else
	finaliseModChains();
#endif
	
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

bool HardcodedPolyphonicFX::isSuspendedOnSilence() const
{
	if (opaqueNode != nullptr)
		return opaqueNode->isSuspendedOnSilence();

	return true;
}

hise::ProcessorEditorBody * HardcodedPolyphonicFX::createEditor(ProcessorEditor *parentEditor)
{
	return createHardcodedEditor(parentEditor);
}

void HardcodedPolyphonicFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	auto samplesToUse = jmin(samplesPerBlock, HARDCODED_POLY_FX_BLOCKSIZE);

	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesToUse);
	SimpleReadWriteLock::ScopedReadLock sl(lock);
	auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

void HardcodedPolyphonicFX::startVoice(int voiceIndex, const HiseEvent& e)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	VoiceEffectProcessor::startVoice(voiceIndex, e);

	if (opaqueNode != nullptr)
	{
		voiceStack.startVoice(*opaqueNode, polyHandler, voiceIndex, e);
	}
}



void HardcodedPolyphonicFX::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	int blockSize = HARDCODED_POLY_FX_BLOCKSIZE;

	bool ok = true;

#if !NUM_HARDCODED_POLY_FX_MODS
	blockSize = numSamples;
#endif

	auto numToDo = numSamples;

	while(numToDo > 0)
	{
		auto numThisTime = jmin(numToDo, blockSize);

#if NUM_HARDCODED_POLY_FX_MODS

		float modValues[NUM_HARDCODED_POLY_FX_MODS];

		if (opaqueNode != nullptr)
		{
			int numParametersToModulate = jmin(NUM_HARDCODED_POLY_FX_MODS, opaqueNode->numParameters);

			for (int i = 0; i < numParametersToModulate; i++)
			{
				auto mv = modChains[i].getOneModulationValue(startSample);
				
				auto p = opaqueNode->getParameter(i);

				if (modChains[i].getChain()->getMode() == Modulation::PanMode)
				{
					if (!modChains[i].getChain()->shouldBeProcessedAtAll())
						mv = 1.0f;

					if(auto ptr = getParameterPtr(i))
						p->callback.call((double)*ptr);
				}
				else
				{
					auto r = p->toRange();

					if(auto ptr = getParameterPtr(i))
					{
						auto normMaxValue = r.convertTo0to1(*ptr, false);
						normMaxValue *= mv;
						auto value = r.convertFrom0to1(normMaxValue, false);

						p->callback.call((double)value);
					}
				}
			}
		}

	#endif
		
		ok &= processHardcoded(b, nullptr, startSample, numThisTime);

		startSample += numThisTime;
		numToDo -= numThisTime;
	}



	getMatrix().handleDisplayValues(b, b, false);

	isTailing = ok && voiceStack.containsVoiceIndex(voiceIndex);
}

void HardcodedPolyphonicFX::renderData(ProcessDataDyn& data)
{
	auto voiceIndex = polyHandler.getVoiceIndex();

	if (checkPreSuspension(voiceIndex, data))
		return;

	HardcodedSwappableEffect::renderData(data);

	checkPostSuspension(voiceIndex, data);
}

void HardcodedPolyphonicFX::handleHiseEvent(const HiseEvent& m)
{
#if NUM_HARDCODED_POLY_FX_MODS
	VoiceEffectProcessor::handleHiseEvent(m);
#endif

	// Already handled...
	if(m.isNoteOn())
		return;
        
	if (opaqueNode != nullptr)
		voiceStack.handleHiseEvent(*opaqueNode, polyHandler, m);
}

void HardcodedPolyphonicFX::renderVoice(int voiceIndex, AudioSampleBuffer& b, int startSample, int numSamples)
{
#if NUM_HARDCODED_POLY_FX_MODS
	preVoiceRendering(voiceIndex, startSample, numSamples);
#endif

	applyEffect(voiceIndex, b, startSample, numSamples);
}

HardcodedSwappableEffect::DataWithListener::DataWithListener(HardcodedSwappableEffect& parent, ComplexDataUIBase* p, int index_, OpaqueNode* nodeToInitialise) :
	node(nodeToInitialise),
	data(p),
	index(index_)
{
	auto mc = dynamic_cast<ControlledObject*>(&parent)->getMainController();

	if (data != nullptr)
	{

		data->getUpdater().setUpdater(mc->getGlobalUIUpdater());
		data->getUpdater().addEventListener(this);
		updateData();
	}

	if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(p))
	{
		af->setProvider(new PooledAudioFileDataProvider(mc));

		af->registerXYZProvider("SampleMap",
			[mc]() { return static_cast<MultiChannelAudioBuffer::XYZProviderBase*>(new hise::XYZSampleMapProvider(mc)); });

		af->registerXYZProvider("SFZ",
			[mc]() { return static_cast<MultiChannelAudioBuffer::XYZProviderBase*>(new hise::XYZSFZProvider(mc)); });
	}
}

} // namespace hise
