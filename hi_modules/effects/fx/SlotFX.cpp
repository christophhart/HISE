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

	reset();
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

			reset();
			return true;
		}
	}
	else
	{
		jassertfalse;
		reset();
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
		auto networkList = getEffect()->getListOfAvailableNetworks();

		selector.addItem("No network", 1);
		selector.addItemList(networkList, 2);
		selector.onChange = BIND_MEMBER_FUNCTION_0(HardcodedMasterEditor::onEffectChange); 

		getEffect()->getMainController()->skin(selector);
		
		addAndMakeVisible(selector);

		selector.setText(getEffect()->currentEffect, dontSendNotification);

		rebuildParameters();

	};

	void onEffectChange()
	{
		getEffect()->setEffect(selector.getText());
		
		rebuildParameters();
	}

	void rebuildParameters()
	{
		currentParameters.clear();

		if (auto on = getEffect()->opaqueNode.get())
		{
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
		if(currentParameters.isEmpty())
			return 32 * 2 * Margin; 
		else
		{
			int numRows = currentParameters.size() / 4 + 1;
			return numRows * (48 + 4 * Margin);
		}
	}

	void updateGui() override
	{
		for (auto p : currentParameters)
			p->updateValue();
	}

	OwnedArray<MacroControlledObject> currentParameters;

	ComboBox selector;
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
	}

	return v;
}

void HardcodedMasterFX::restoreFromValueTree(const ValueTree& v)
{
	LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
	MasterEffectProcessor::restoreFromValueTree(v);

	auto effect = v.getProperty("Network", "No Effect").toString();

	setEffect(effect);

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
	{

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

void HardcodedMasterFX::setEffect(const String& factoryId)
{
	if (factoryId == currentEffect)
		return;

	auto idx = getListOfAvailableNetworks().indexOf(factoryId);

	ScopedPointer<OpaqueNode> newNode;

	if (idx != -1)
	{
		currentEffect = factoryId;
		newNode = new OpaqueNode();

		if (!factory->initOpaqueNode(newNode, idx, false))
			newNode = nullptr;

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
	}
	else
	{
		currentEffect = {};

		{
			SimpleReadWriteLock::ScopedWriteLock sl(lock);
			std::swap(newNode, opaqueNode);
		}
	}

	if (newNode != nullptr)
	{
		// We need to deinitialise it with the DLL to prevent heap corruption
		factory->deinitOpaqueNode(newNode);

		newNode = nullptr;
	}
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
