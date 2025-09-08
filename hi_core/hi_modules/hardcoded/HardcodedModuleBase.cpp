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



namespace hise
{
using namespace juce;


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

			auto ta = selector.getBoundsInParent().translated(0, 20);

			if(currentParameters.isEmpty())
			{
				ta = getLocalBounds();
				ta.removeFromLeft(selector.getRight() + 10);
			}

			g.drawMultiLineText("ERROR: " + errorMessage, ta.getX(), ta.getY() + 15, ta.getWidth());
		}

	}

	void rebuildParameters()
	{
		currentEditors.clear();
		currentParameters.clear();

#if 0
		if(!getErrorMessage().isEmpty())
			return;
#endif

		std::function<int(ExternalData::DataType)> numObjectsFunction;

		if (auto on = getEffect()->opaqueNode.get())
		{
			numObjectsFunction = [on](ExternalData::DataType dt) { return on->numDataObjects[(int)dt]; };

		}
		else if (getEffect()->hasLoadedButUncompiledEffect())
		{
			numObjectsFunction = [this](ExternalData::DataType dt){ return getEffect()->getNumDataObjects(dt); };
		}

		if(numObjectsFunction)
		{
			ExternalData::forEachType([&](ExternalData::DataType dt)
			{
				int numObjects = numObjectsFunction(dt);

				for (int i = 0; i < numObjects; i++)
				{
					auto f = ExternalData::createEditor(getEffect()->getComplexBaseType(dt, i));

					auto c = dynamic_cast<Component*>(f);

					currentEditors.add(f);
					addAndMakeVisible(c);
				}
			});
		}
		
		if (auto on = getEffect()->opaqueNode.get())
		{
			for(const auto& p: OpaqueNode::ParameterIterator(*on))
			{
				auto pData = p.info;
				auto s = new HiSlider(pData.getId());
				addAndMakeVisible(s);
				s->setup(getProcessor(), pData.index + getEffect()->getParameterOffset(), pData.getId());
				auto nr = pData.toRange().rng;

				auto vtc = p.getValueToTextConverter();

				if(vtc.active)
				{
					s->valueFromTextFunction = vtc;
					s->textFromValueFunction = vtc;
				}

				s->setRange(pData.min, pData.max, jmax<double>(0.001, pData.interval));
				s->setSkewFactor(pData.skew);
				s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
				s->setTextBoxStyle(Slider::TextBoxRight, true, 80, 20);
				s->setColour(Slider::thumbColourId, Colour(0x80666666));
				s->setColour(Slider::textBoxTextColourId, Colours::white);

				currentParameters.add(s);
			}
		}
		else if (getEffect()->hasLoadedButUncompiledEffect())
		{
			auto& p = getEffect()->asProcessor();
			for(int i = getEffect()->getParameterOffset(); i < p.getNumAttributes(); i++)
			{
				auto id = p.getIdentifierForParameterIndex(i);
				auto s = new HiSlider(id.toString());
				addAndMakeVisible(s);
				s->setup(getProcessor(), i, id.toString());

				auto v = (double)p.getAttribute(i);
				
				s->setRange(jmin(0.0, v), jmax(1.0, v), 0.01);
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
	tempoSyncer.ppqFunction = [mc](int ts){ return mc->getMasterClock().getPPQPos(ts); };
	tempoSyncer.publicModValue = &modValue;
	polyHandler.setTempoSyncer(&tempoSyncer);
	mc->addTempoListener(&tempoSyncer);

#if USE_BACKEND
	auto dllManager = dynamic_cast<BackendProcessor*>(mc)->dllManager.get();
	dllManager->loadDll(false);
	dllManager->reloadBroadcaster.addListener(*this, onDllReload, false);
	factory = new scriptnode::dll::DynamicLibraryHostFactory(dllManager->projectDll);
#else
	factory = scriptnode::DspNetwork::createStaticFactory();
#endif
}

ModulatorChain::ModChainWithBuffer* HardcodedSwappableEffect::getModulationChainForParameter(int parameterIndex) const
{
	auto modIndex = modProperties.getModulationChainIndex(parameterIndex);

	if(modProperties.isUsed(modIndex))
	{
		if(auto mc = getModulationChains())
		{
			modIndex = getExtraModulationIndex(modIndex);

			if(isPositiveAndBelow(modIndex, mc->size()))
				return mc->begin() + modIndex;
		}
	}

	return nullptr;
}

Identifier HardcodedSwappableEffect::getSanitizedParameterId(const String& id)
{
	auto sanitized = id.removeCharacters(" \t\n/-+.,");
	return Identifier(sanitized);
}

HardcodedSwappableEffect::~HardcodedSwappableEffect()
{
	jassert(shutdownCalled);
	

	// Call disconnectRuntimeTargets in the derived destructor!
	jassert(disconnected);

	

	factory = nullptr;
}

void HardcodedSwappableEffect::connectRuntimeTargets(Processor* p)
{
	if(p->getMainController()->isBeingDeleted())
		return;

	jassert(p != nullptr);

    if(opaqueNode != nullptr)
    {
        p->connectToRuntimeTargets(*opaqueNode, true);
    }
}

#if USE_BACKEND
void HardcodedSwappableEffect::preallocateUnloadedParameters(Array<Identifier> unloadedParameters_)
{
	if(hasLoadedButUncompiledEffect())
	{
		unloadedParameters = unloadedParameters_;
		numParameters = unloadedParameters.size();
		lastParameters.setSize(numParameters, false);

		if(getParameterOffset() == 0)
			asProcessor().parameterNames.clear();
		else
			asProcessor().parameterNames.removeRange(getParameterOffset(), asProcessor().parameterNames.size() - getParameterOffset());

		for(auto p: unloadedParameters)
			asProcessor().parameterNames.add(p);

		asProcessor().updateParameterSlots();
	}
}
#endif

bool HardcodedSwappableEffect::hasLoadedButUncompiledEffect() const
{
#if USE_BACKEND
	return currentEffect.isNotEmpty() && (factory == nullptr || factory->getNumNodes() == 0);
#else
		return false;
#endif
}

ModulatorChain::ExtraModulatorRuntimeTargetSource::ParameterInitData HardcodedSwappableEffect::
getParameterInitData(int pIndex)
{
	ModulatorChain::ExtraModulatorRuntimeTargetSource::ParameterInitData pData;

	jassert(opaqueNode != nullptr);

	if(auto p = opaqueNode->getParameter(pIndex))
	{
		pData.ir = p->toRange();
		pData.vtc = p->getValueToTextConverter();
		pData.initValue = *getParameterPtr(pIndex);
	}

	return pData;
}

void HardcodedSwappableEffect::disconnectRuntimeTargets(Processor* p)
{
	jassert(p != nullptr);

    if(opaqueNode != nullptr)
    {
        p->connectToRuntimeTargets(*opaqueNode, false);
    }

	disconnected = true;
}

bool HardcodedSwappableEffect::setEffect(const String& factoryId, bool /*unused*/)
{
	if (factoryId == currentEffect)
		return true;

	if(opaqueNode != nullptr)
	{
		asProcessor().connectToRuntimeTargets(*opaqueNode, false);
	}

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

        asProcessor().connectToRuntimeTargets(*newNode, true);

		ExternalDataHolder::garbageCollectFilterCoefficients();

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

			modProperties = opaqueNode->getModulationProperties();

			for (auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
			{
				if(auto ptr = getParameterPtr(p.info.index))
					*ptr = (float)p.info.defaultValue;
			}

            channelCountMatches = checkHardcodedChannelCount();
		}		

        Array<Identifier> offsetParameters;
        
        for(int i = 0; i < getParameterOffset(); i++)
        {
            offsetParameters.add(asProcessor().parameterNames[i]);
        }
        
        asProcessor().parameterNames.clear();
        
        if(!offsetParameters.isEmpty())
            asProcessor().parameterNames.addArray(offsetParameters);

		auto illegalIds = getIllegalParameterIds();

		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			parameterRanges.set(p.info.index, p.info.toRange());

			auto nid = p.info.getId();

			if(illegalIds.contains(nid))
			{
				String em;
				em << "Reserved parameter ID in network: " << nid;
				errorBroadcaster.sendMessage(sendNotificationAsync, em);
				channelCountMatches = false;
			}

			asProcessor().parameterNames.add(nid);
		}

		asProcessor().updateParameterSlots();
		
		effectUpdater.sendMessage(sendNotificationAsync, currentEffect, somethingChanged, opaqueNode->numParameters);

		tempoSyncer.tempoChanged(mc_->getBpm());
	}
	else
	{
		if(factory == nullptr || factory->getNumNodes() == 0)
		{
			// just set the effect so it will be exported properly
			currentEffect = factoryId;
		}
		else
		{
			currentEffect = {};
			effectUpdater.sendMessage(sendNotificationAsync, currentEffect, true, 0);
		}

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

	return opaqueNode != nullptr;
}

bool HardcodedSwappableEffect::swap(HotswappableProcessor* other)
{
	if (auto otherFX = dynamic_cast<HardcodedSwappableEffect*>(other))
	{
		if (otherFX->isPolyphonic() != isPolyphonic())
			return false;

#if USE_BACKEND
		std::swap(previouslySavedTree, otherFX->previouslySavedTree);
#endif
		std::swap(currentEffect, otherFX->currentEffect);

		auto& ap = asProcessor();
		auto& op = otherFX->asProcessor();

		unloadedParameters.swapWith(otherFX->unloadedParameters);
		std::swap(numParameters, otherFX->numParameters);

		ap.parameterNames.swapWith(op.parameterNames);
		tables.swapWith(otherFX->tables);
		sliderPacks.swapWith(otherFX->sliderPacks);
		audioFiles.swapWith(otherFX->audioFiles);
		filterData.swapWith(otherFX->filterData);
		displayBuffers.swapWith(otherFX->displayBuffers);
		listeners.swapWith(otherFX->listeners);

		std::swap(lastParameters, otherFX->lastParameters);

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

StringArray HardcodedSwappableEffect::getIllegalParameterIds() const
{
	return { "Type", "Bypassed", "ID", "Network" };
}

juce::Result HardcodedSwappableEffect::sanityCheck()
{
#if USE_BACKEND
	String errorMessage;

	errorMessage << dynamic_cast<Processor*>(this)->getId();
	errorMessage << ":  > ";

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
#endif
	
	return Result::ok();
}

void HardcodedSwappableEffect::setHardcodedAttribute(int parameterIndex, float newValue)
{
	if(auto ptr = getParameterPtr(parameterIndex))
		*ptr = newValue;

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode == nullptr)
		return;

	if(auto p = opaqueNode->getParameter(parameterIndex))
	{
		if(auto mc = getModulationChainForParameter(parameterIndex))
		{
			auto normValue = p->toRange().convertTo0to1(newValue, false);
			mc->getChain()->setInitialValue(normValue);
		}
		else
		{
			p->callback.call((double)newValue);
		}
	}

	handleFilterStatisticUpdate();
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
	auto effect = v.getProperty("Network", "").toString();

	setEffect(effect, false);

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	std::function<int(ExternalData::DataType)> numDataObjectFunction;

	// First check which complex data objects to restore
	if(hasLoadedButUncompiledEffect())
	{
		numDataObjectFunction = [v](ExternalData::DataType dt)
		{
			auto parentId = Identifier(ExternalData::getDataTypeName(dt, true));
			return v.getChildWithName(parentId).getNumChildren();
		};
	}
	else if(opaqueNode != nullptr)
	{
		auto on = opaqueNode.get();
		numDataObjectFunction = [on](ExternalData::DataType dt) { return on->numDataObjects[(int)dt]; };
	}

	if(numDataObjectFunction)
	{
		ExternalData::forEachType([&](ExternalData::DataType dt)
		{
			if (dt == ExternalData::DataType::DisplayBuffer ||
				dt == ExternalData::DataType::FilterCoefficients)
				return;

			auto parentId = Identifier(ExternalData::getDataTypeName(dt, true));
			auto dataTree = v.getChildWithName(parentId);

			jassert(dataTree.getNumChildren() == numDataObjectFunction(dt));

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
	}


	if(hasLoadedButUncompiledEffect())
	{
#if USE_BACKEND
		previouslySavedTree = v.createCopy();

		unloadedParameters.clear();
		DBG(v.createXml()->createDocument(""));

		int propertyOffset = -1;

		for(int i = 0; i < v.getNumProperties(); i++)
		{
			if(v.getPropertyName(i) == scriptnode::PropertyIds::Network)
			{
				propertyOffset = i + 1;
				break;
			}
		}

		jassert(propertyOffset == 4);

		numParameters = jmax(0, v.getNumProperties() - propertyOffset);

		if(numParameters > 0)
		{
			lastParameters.setSize(numParameters);

			Array<Identifier> up;

			for(int i = 0; i < numParameters; i++)
			{
				up.add(v.getPropertyName(i + propertyOffset));
			}

			preallocateUnloadedParameters(up);

			for(int i = 0; i < numParameters; i++)
			{
				if(auto ptr = getParameterPtr(i))
				{
					auto value = (float)v[up.getLast()];
					*ptr = value;
				}
			}
		}
#endif
	}
	else if (opaqueNode != nullptr)
	{
		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			auto id = getSanitizedParameterId(p.info.getId());
			auto value = v.getProperty(id, p.info.defaultValue);
			setHardcodedAttribute(p.info.index, value);
		}
	}
}

ValueTree HardcodedSwappableEffect::writeHardcodedData(ValueTree& v) const
{
	v.setProperty("Network", currentEffect, nullptr);
	
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	std::function<int(ExternalData::DataType)> numObjectsFunction;

	if (opaqueNode != nullptr)
	{
		for (const auto& p : OpaqueNode::ParameterIterator(*opaqueNode))
		{
			auto id = getSanitizedParameterId(p.info.getId());

			if(auto ptr = getParameterPtr(p.info.index))
				v.setProperty(id, *ptr, nullptr);
		}

		auto on = opaqueNode.get();
		numObjectsFunction = [on](ExternalData::DataType dt) { return on->numDataObjects[(int)dt]; };
	}
	else if (hasLoadedButUncompiledEffect())
	{
		for(int i = 0; i < unloadedParameters.size(); i++)
		{
			if(auto ptr = getParameterPtr(i))
				v.setProperty(unloadedParameters[i], *ptr, nullptr);
		}

		numObjectsFunction = [this](ExternalData::DataType dt) { return getNumDataObjects(dt); };
	}

	if(numObjectsFunction)
	{
		ExternalData::forEachType([&](ExternalData::DataType dt)
		{
			if (dt == ExternalData::DataType::DisplayBuffer ||
				dt == ExternalData::DataType::FilterCoefficients)
				return;

			int numObjects = numObjectsFunction(dt);

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
		setupChannelData(d, b, startSample);

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

float* HardcodedSwappableEffect::getParameterPtr(int index) const
{
	if(isPositiveAndBelow(index, numParameters))
		return static_cast<float*>(lastParameters.getObjectPtr()) + index;
	return
		nullptr;
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
		ps.blockSize = modProperties.getBlockSize(asProcessor().getLargestBlockSize());
		ps.sampleRate = asProcessor().getSampleRate();
		ps.voiceIndex = &polyHandler;
		n->prepare(ps);
		n->reset();

#if USE_BACKEND
		auto e = factory->getError();

		if (e.error != Error::OK)
			return Result::fail(ScriptnodeExceptionHandler::getErrorMessage(e));
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
	case ExternalData::DataType::FilterCoefficients: return filterData.size();
	default: jassertfalse; return 0;
	}
}

#if USE_BACKEND
void HardcodedSwappableEffect::onDllReload(HardcodedSwappableEffect& fx, const std::pair<scriptnode::dll::ProjectDll*,
	scriptnode::dll::ProjectDll*>& update)
{
	auto thisId = fx.currentEffect;

	auto prevNumParameters = fx.numParameters;

	HeapBlock<float> parametersToRetain;

	parametersToRetain.allocate(prevNumParameters, false);
	memcpy(parametersToRetain.get(), fx.lastParameters.getObjectPtr(), sizeof(float) * prevNumParameters);

	fx.setEffect("", true);
	fx.factory = new scriptnode::dll::DynamicLibraryHostFactory(update.second);
	fx.setEffect(thisId, true);

	debugToConsole(dynamic_cast<Processor*>(&fx), "Reloading FX " + thisId);

	if(fx.numParameters == prevNumParameters)
	{
		for(int i = 0; i < prevNumParameters; i++)
			fx.setHardcodedAttribute(i, parametersToRetain[i]);
	}
	else
	{
		debugToConsole(dynamic_cast<Processor*>(&fx), "Parameter amount changed, ignoring previous values");
	}
}
#endif

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

HardcodedSwappableEffect::DataWithListener::~DataWithListener()
{
	if (data != nullptr)
		data->getUpdater().removeEventListener(this);
}

void HardcodedSwappableEffect::DataWithListener::updateData()
{
	if (node != nullptr)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(data->getDataLock());

		ExternalData ed(data.get(), index);

		SimpleRingBuffer::ScopedPropertyCreator sps(data.get());

		node->setExternalData(ed, index);
	}

}

void HardcodedSwappableEffect::DataWithListener::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var newValue)
{
	if (t == ComplexDataUIUpdaterBase::EventType::ContentRedirected ||
		t == ComplexDataUIUpdaterBase::EventType::ContentChange)
	{
		updateData();
	}
}
}
