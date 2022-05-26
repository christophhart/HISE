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

hise::MasterEffectProcessor* SlotFX::getCurrentEffect()
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

HardcodedMasterFX::HardcodedMasterFX(MainController* mc, const String& uid) :
	MasterEffectProcessor(mc, uid),
	ProcessorWithExternalData(mc),
	polyHandler(false)
{
	finaliseModChains();

#if USE_BACKEND
	auto dllManager = dynamic_cast<BackendProcessor*>(mc)->dllManager.get();
	dllManager->loadDll(false);
	factory = new scriptnode::dll::DynamicLibraryHostFactory(dllManager->projectDll);
#else
	factory = FrontendHostFactory::createStaticFactory();
#endif
}

HardcodedMasterFX::~HardcodedMasterFX()
{
	if (opaqueNode != nullptr)
	{
		factory->deinitOpaqueNode(opaqueNode);
		opaqueNode = nullptr;
	}

	factory = nullptr;
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

		getEffect()->getMainController()->skin(selector);
		
		addAndMakeVisible(selector);

		selector.setText(getEffect()->currentEffect, dontSendNotification);

		rebuildParameters();

	};

	static void update(HardcodedMasterEditor& ed, String newEffect, bool complexDataChanged, int numParameters)
	{
		ed.rebuildParameters();
		ed.selector.setText(newEffect, dontSendNotification);
	}

	void onEffectChange()
	{
		getEffect()->setEffect(selector.getText(), true);
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
				s->setup(getEffect(), i, pData.getId());
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

	HardcodedMasterFX* getEffect() { return dynamic_cast<HardcodedMasterFX*>(getProcessor()); }
	const HardcodedMasterFX* getEffect() const { return dynamic_cast<const HardcodedMasterFX*>(getProcessor()); }

	int getBodyHeight() const override 
	{ 
		if(currentParameters.isEmpty() && currentEditors.isEmpty())
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

void HardcodedMasterFX::voicesKilled()
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
		opaqueNode->reset();
}

juce::ValueTree HardcodedMasterFX::exportAsValueTree() const
{
	if (factory->getNumNodes() == 0)
	{
		jassert(treeWhenNotLoaded.isValid());
		return treeWhenNotLoaded;
	}

	ValueTree v = MasterEffectProcessor::exportAsValueTree();

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
				auto constThis = const_cast<HardcodedMasterFX*>(this);

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

void HardcodedMasterFX::restoreFromValueTree(const ValueTree& v)
{
	LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
	MasterEffectProcessor::restoreFromValueTree(v);

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
						af->setRange({min, max});
					}
				}

				index++;
			}
		});

		for (int i = 0; i < opaqueNode->numParameters; i++)
		{
			auto value = v.getProperty(opaqueNode->parameters[i].getId(), opaqueNode->parameters[i].defaultValue);
			setInternalAttribute(i, value);
		}
	}
}

hise::ProcessorEditorBody * HardcodedMasterFX::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new HardcodedMasterEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

bool HardcodedMasterFX::setEffect(const String& factoryId, bool /*unused*/)
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

		if (!factory->initOpaqueNode(newNode, idx, false))
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
		}		

		parameterNames.clear();

		for (int i = 0; i < opaqueNode->numParameters; i++)
			parameterNames.add(opaqueNode->parameters[i].getId());

		effectUpdater.sendMessage(sendNotificationAsync, currentEffect, somethingChanged, opaqueNode->numParameters);
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

bool HardcodedMasterFX::swap(HotswappableProcessor* other)
{
	if (auto otherFX = dynamic_cast<HardcodedMasterFX*>(other))
	{
		std::swap(treeWhenNotLoaded, otherFX->treeWhenNotLoaded);
		std::swap(currentEffect, otherFX->currentEffect);

		parameterNames.swapWith(otherFX->parameterNames);
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

			prepareToPlay(getSampleRate(), getLargestBlockSize());
			otherFX->prepareToPlay(otherFX->getSampleRate(), otherFX->getLargestBlockSize());
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

void HardcodedMasterFX::prepareOpaqueNode(OpaqueNode* n)
{
	if (n != nullptr && getSampleRate() > 0.0 && getLargestBlockSize() > 0)
	{
		PrepareSpecs ps;
		ps.numChannels = 2;
		ps.blockSize = getLargestBlockSize();
		ps.sampleRate = getSampleRate();
		ps.voiceIndex = &polyHandler;
		n->prepare(ps);
		n->reset();
	}
}



void HardcodedMasterFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	prepareOpaqueNode(opaqueNode.get());
}

juce::StringArray HardcodedMasterFX::getListOfAvailableNetworks() const
{
	jassert(factory != nullptr);

	StringArray sa;

	int numNodes = factory->getNumNodes();

	for (int i = 0; i < numNodes; i++)
		sa.add(factory->getId(i));

	return sa;

}

void HardcodedMasterFX::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
	{
		float* d[2] = { b.getWritePointer(0, startSample),
						b.getWritePointer(1, startSample) };

		ProcessDataDyn pd(d, numSamples, 2);

		if (eventBuffer != nullptr)
			pd.setEventBuffer(*eventBuffer);

		opaqueNode->process(pd);
	}
}

} // namespace hise
