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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{




using namespace juce;
using namespace hise;
using namespace snex;

void* SnexSource::SnexTestBaseHelper::getNodeWorkbench(NodeBase* node)
{
    return node->getScriptProcessor()->getMainController_()->getWorkbenchManager();

}

void SnexSource::recompiled(WorkbenchData::Ptr wb)
{
	getCallbackHandler().reset();
	getParameterHandler().reset();
	getComplexDataHandler().reset();

	lastResult = wb->getLastResult().compileResult;

    if(lastResult.wasOk())
    {
        if(errorLevel != ErrorLevel::Warning)
            errorLevel = ErrorLevel::OK;
    }
    else
        errorLevel = ErrorLevel::CompileFail;
    
	

	if (getParentNode()->isActive(true) && currentChannelCount != wb->getNumChannels())
	{
		lastResult = Result::fail("Channel mismatch. Expected: " + String(wb->getNumChannels()));
	}

	throwScriptnodeErrorIfCompileFail();

	if (auto objPtr = wb->getLastResult().mainClassPtr)
	{
		objPtr->initialiseObjectStorage(object);

		wb->getLastResultReference().setDataPtrForDebugging(object.getObjectPtr());

#if 0
		String s;
		int l = 0;

		objPtr->dumpTable(s, l, object.getObjectPtr(), object.getObjectPtr());

		DBG(s);
#endif

		if (lastResult.wasOk())
			lastResult = getCallbackHandler().recompiledOk(objPtr);

		if (lastResult.wasOk())
			lastResult = getParameterHandler().recompiledOk(objPtr);

		if (lastResult.wasOk())
			lastResult = getComplexDataHandler().recompiledOk(objPtr);

		lastCompiledObject = getWorkbench()->getLastJitObject();
	}

	throwScriptnodeErrorIfCompileFail();

	if (!lastResult.wasOk())
		getWorkbench()->getLastResultReference().compileResult = lastResult;
		
	for (auto l : compileListeners)
	{
		if(l != nullptr)
			l->wasCompiled(lastResult.wasOk());
	}
}

void SnexSource::throwScriptnodeErrorIfCompileFail()
{
	if (auto wb = getWorkbench())
	{
		auto& eh = parentNode->getRootNetwork()->getExceptionHandler();

		if (wb->getGlobalScope().isDebugModeEnabled())
			eh.addCustomError(parentNode, Error::NodeDebuggerEnabled, "Debug is enabled");
		else
			eh.removeError(parentNode, Error::NodeDebuggerEnabled);

		auto ok = lastResult.wasOk();

		processingEnabled = wb->getGlobalScope().isDebugModeEnabled() && ok;

		if (ok)
			parentNode->getRootNetwork()->getExceptionHandler().removeError(parentNode, Error::CompileFail);
		else
		{
			wb->getLastResult().compileResult = lastResult;
			auto e = lastResult.getErrorMessage();
			parentNode->getRootNetwork()->getExceptionHandler().addCustomError(parentNode, scriptnode::Error::CompileFail, e);
		}
	}
}

void SnexSource::logMessage(WorkbenchData::Ptr wb, int level, const String& s)
{
    if(level == 1)
    {
        errorLevel = ErrorLevel::Warning;
    }
    
	if (wb->getGlobalScope().isDebugModeEnabled() && level > 4)
	{
        
		if (auto p = dynamic_cast<Processor*>(parentNode->getScriptProcessor()))
		{
			parentNode->getScriptProcessor()->getMainController_()->writeToConsole(s.trim(), 0);
		}
	}
}

void SnexSource::debugModeChanged(bool isEnabled)
{
	throwScriptnodeErrorIfCompileFail();
}

void SnexSource::rebuildCallbacksAfterChannelChange(int numChannelsToProcess)
{
	auto wasZero = currentChannelCount == 0;
	currentChannelCount = numChannelsToProcess;

	if (wb != nullptr)
	{
		
		wb->setNumChannels(currentChannelCount);

		if (wasZero)
			wb->triggerRecompile();

#if 0
		if (auto objPtr = wb->getLastResult().mainClassPtr)
		{
			if (lastResult.wasOk())
				lastResult = getCallbackHandler().recompiledOk(objPtr);
		}
#endif
	}
}

void SnexSource::addDummyNodeCallbacks(String& s, bool addEvent, bool addReset, bool addMod, bool addPlotValue)
{
	using namespace cppgen;

	Base b(Base::OutputType::AddTabs);

	auto NV = String(getParentNode()->isPolyphonic() ? NUM_POLYPHONIC_VOICES : 1);

	String instanceType = getCurrentClassId().toString() + "<" + NV + ">";

	{
		b << String("void prepare(" + instanceType + "& instance, PrepareSpecs ps)");
		{
			StatementBlock sb(b, false);
			b << "instance.prepare(ps);";
		}

        b << String("void setExternalData(" + instanceType + "& instance, ExternalData& d, int index)");
        {
            StatementBlock sb(b, false);
            b << "instance.setExternalData(d, index);";
        }
		
		if (addReset)
		{
			b << String("void reset(" + instanceType + "& instance)");
			{
				StatementBlock sb(b, false);
				b << "instance.reset();";
			}
		}

		if (addEvent)
		{
			b << String("void handleHiseEvent(" + instanceType + "& instance, HiseEvent& e)");
			{
				StatementBlock sb(b, false);
				b << "instance.handleHiseEvent(e);";
			}
		}

		if(addMod)
		{
			b << String("bool handleModulation(" + instanceType + "& instance, double& v)");
			{
				StatementBlock sb(b, false);
				b << "return instance.handleModulation(v);";
			}
		}

		if(addPlotValue)
		{
			b << String("double getPlotValue(" + instanceType + "& instance, int m, double f)");
			{
				StatementBlock sb(b, false);
				b << "return instance.getPlotValue(m, f);";
			}
		}
	}

	auto x = b.toString();
	s << x;
}

void SnexSource::addDummyProcessFunctions(String& s, bool addFrame, const String& processDataType)
{
	using namespace cppgen;

	if (auto pn = getParentNode())
	{
		int nc = currentChannelCount;

		if (nc == 0)
			nc = 2;

		auto NV = String(getParentNode()->isPolyphonic() ? NUM_POLYPHONIC_VOICES : 1);

		String instanceType = getCurrentClassId().toString() + "<" + NV + ">";

		Base b(Base::OutputType::AddTabs);

		String def1, def2;

		String objPtr;

#if SNEX_MIR_BACKEND
		objPtr << instanceType << "& instance, ";
#else
		String instanceDef = instanceType + " instance;";
		b << instanceDef;
#endif

		if (addFrame)
		{
			def2 << "void processFrame(" << objPtr << "span<float, " << String(nc) << ">& data)"; b << def2;
			{
				StatementBlock body(b);
				b << "instance.processFrame(data);";
			}
		}

		String pType;
		if (processDataType.isNotEmpty())
			pType = processDataType;
		else
			pType << "ProcessData<" << String(nc) << ">";

		def1 << "void process(" << objPtr << pType << "& data)";
		b << def1;
		{
			StatementBlock body(b);
			b << "instance.process(data);";
		}

		s << b.toString();
	}
	else
		jassertfalse;
}

void SnexSource::ParameterHandler::updateParameters(ValueTree v, bool wasAdded)
{
	

	if (wasAdded)
	{
		auto newP = new SnexParameter(&parent, getNode(), v);
		getNode()->addParameter(newP);
	}
	else
	{
		for(auto sn_: NodeBase::ParameterIterator(*getNode()))
		{
			if (auto sn = dynamic_cast<SnexParameter*>(sn_))
			{
				if (sn->data[PropertyIds::ID].toString() == v[PropertyIds::ID].toString())
				{
					removeSnexParameter(sn);
					break;
				}
			}
		}
	}

	this->numParameters = getNode()->getNumParameters();
}

void SnexSource::ParameterHandler::updateParametersForWorkbench(bool shouldAdd)
{
	for (int i = 0; i < getNode()->getNumParameters(); i++)
	{
		if (auto sn = dynamic_cast<SnexParameter*>(getNode()->getParameterFromIndex(i)))
		{
			removeSnexParameter(sn);
			i--;
		}
	}

	if (shouldAdd)
	{
		parameterTree = getNode()->getRootNetwork()->codeManager.getParameterTree(parent.getTypeId(), parent.classId.getValue());
		parameterListener.setCallback(parameterTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(ParameterHandler::updateParameters));
	}
}

void SnexSource::ParameterHandler::removeSnexParameter(SnexParameter* p)
{
	p->data.getParent().removeChild(p->data, getNode()->getUndoManager());

	for (int i = 0; i < getNode()->getNumParameters(); i++)
	{
		if (getNode()->getParameterFromIndex(i) == p)
		{
			getNode()->removeParameter(i);
			break;
		}
	}
}

void SnexSource::ParameterHandler::addNewParameter(parameter::data p)
{
	if (auto existing = getNode()->getParameterFromName(p.info.getId()))
		return;

	auto newTree = p.createValueTree();
	parameterTree.addChild(newTree, -1, getNode()->getUndoManager());
}

void SnexSource::ParameterHandler::addParameterCode(String& code)
{
	using namespace snex::cppgen;

	cppgen::Base c(cppgen::Base::OutputType::AddTabs);

	c.addComment("Adding parameter methods", cppgen::Base::CommentType::RawWithNewLine);

	String fDef;
	fDef << "void initMainObject(" << parent.getWorkbench()->getInstanceId() << "<NUM_POLYPHONIC_VOICES>& obj)";
	c << fDef;

	{
		cppgen::StatementBlock sb(c);

		for (const auto& p : parameterTree)
		{
			String def;
			def << "obj.setParameter<" << p.getParent().indexOf(p) << ">(";
			def << Types::Helpers::getCppValueString((double)p[PropertyIds::Value]);
			def << ");";

			c << def;
			c.addComment(p[PropertyIds::ID].toString(), cppgen::Base::CommentType::AlignOnSameLine);
		}
	}

#if SNEX_MIR_BACKEND
	auto NV = String(parent.getParentNode()->isPolyphonic() ? NUM_POLYPHONIC_VOICES : 1);

	String instanceType = parent.getCurrentClassId().toString() + "<" + NV + ">";


	int pIndex = 0;

	for (const auto& p : parameterTree)
	{
        ignoreUnused(p);
        
		String def;
		def << "void setParameter" << String(pIndex) << "(" << instanceType << "& instance, double value)";
		c << def;
		StatementBlock sb(c);
		c << "instance.setParameter<" << String(pIndex) << ">(value);";

		pIndex++;
	}

#endif

	code << c.toString();
}

Result SnexSource::ComplexDataHandler::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
{
	ExternalData::forEachType([this, objectClass](ExternalData::DataType t)
		{
			auto typeTree = getDataRoot().getChild((int)t);
			auto id = ExternalData::getNumIdentifier(t);
			int numRequired = (int)objectClass->getInternalProperty(id, var(0));

			while (numRequired > typeTree.getNumChildren())
				addOrRemoveDataFromUI(t, true);

			for (int i = 0; i < getNumDataObjects(t); i++)
				getComplexBaseType(t, i)->getUpdater().addEventListener(this);
		});

	if (!hasComplexData())
		return Result::ok();

	auto r = ComplexDataHandlerLight::recompiledOk(objectClass);

	if (!r.wasOk())
		return r;

	auto debugMode = parent.getWorkbench()->getGlobalScope().isDebugModeEnabled();

	callExternalDataForAll(*this, *this, !debugMode);

	return r;
}

void SnexSource::ComplexDataHandler::initialise(NodeBase* n)
{
	dataTree = n->getValueTree().getOrCreateChildWithName(PropertyIds::ComplexData, n->getUndoManager());

	ExternalData::forEachType([this, n](ExternalData::DataType t)
	{
		auto name = ExternalData::getDataTypeName(t);

		auto typeTree = dataTree.getOrCreateChildWithName(name + "s", n->getUndoManager());

		dataListeners[(int)t].setCallback(typeTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(SnexSource::ComplexDataHandler::dataAddedOrRemoved));
	});

}

void SnexSource::ComplexDataHandler::addOrRemoveDataFromUI(ExternalData::DataType t, bool shouldAdd)
{
	auto pTree = dataListeners[(int)t].getParentTree();

	if (shouldAdd)
	{
		ValueTree newEntry(ExternalData::getDataTypeName(t));
		newEntry.setProperty(PropertyIds::Index, -1, nullptr);
		newEntry.setProperty(PropertyIds::EmbeddedData, "", nullptr);
		pTree.addChild(newEntry, -1, parent.parentNode->getUndoManager());
	}
	else
	{
		pTree.removeChild(pTree.getNumChildren() - 1, parent.parentNode->getUndoManager());
	}
}

void SnexSource::ComplexDataHandler::dataAddedOrRemoved(ValueTree v, bool wasAdded)
{
	auto typeId = v.getType().toString();

	

	ExternalData::DataType t = ExternalData::DataType::numDataTypes;

	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::Table)) t = ExternalData::DataType::Table;
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::SliderPack)) t = ExternalData::DataType::SliderPack;
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::FilterCoefficients)) t = ExternalData::DataType::FilterCoefficients;
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::AudioFile)) t = ExternalData::DataType::AudioFile;
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::DisplayBuffer)) t = ExternalData::DataType::DisplayBuffer;

	if (t != ExternalData::DataType::numDataTypes)
	{
		if (wasAdded)
			getComplexBaseType(t, v.getParent().indexOf(v));
		else
		{
			for (int i = 0; i < getNumDataObjects(t); i++)
			{
				if (dynamic_cast<data::pimpl::dynamic_base*>(getDynamicDataHolder(t, i))->getValueTree() == v)
				{
					removeDataObject(t, i);
					return;
				}
			}

			jassertfalse;
		}

	}
}

int SnexSource::ComplexDataHandler::getNumDataObjects(ExternalData::DataType t) const
{
	switch (t)
	{
	case snex::ExternalData::DataType::Table:				return tables.size();
	case snex::ExternalData::DataType::FilterCoefficients:	return filters.size();
	case snex::ExternalData::DataType::AudioFile:			return audioFiles.size();
	case snex::ExternalData::DataType::SliderPack:			return sliderPacks.size();
	case snex::ExternalData::DataType::DisplayBuffer:		return displayBuffers.size();
    default: return 0;
	}
}

hise::FilterDataObject* SnexSource::ComplexDataHandler::getFilterData(int index) 
{
	if (isPositiveAndBelow(index, filters.size()))
	{
		if (auto t = filters[index])
			return t->getFilterData(0);

		return nullptr;
	}

	auto n = new data::dynamic::filter(*this, index);
	n->initialise(getNode());
	filters.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
	{
		if (safeThis != nullptr)
		{
			for (auto l : safeThis.get()->compileListeners)
			{
				if (l != nullptr)
					l->complexDataAdded(ExternalData::DataType::FilterCoefficients, index);
			}
		}
	});

	return n->getFilterData(0);
}

hise::Table* SnexSource::ComplexDataHandler::getTable(int index)
{
	if (isPositiveAndBelow(index, tables.size()))
	{
		if (auto t = tables[index])
			return t->getTable(0);

		return nullptr;
	}

	auto n = new data::dynamic::table(*this, index);
	n->initialise(getNode());
	tables.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
	{
		if (safeThis != nullptr)
		{
			for (auto l : safeThis.get()->compileListeners)
			{
				if (l != nullptr)
					l->complexDataAdded(ExternalData::DataType::Table, index);
			}
		}
	});

	return n->getTable(0);
}

hise::SliderPackData* SnexSource::ComplexDataHandler::getSliderPack(int index)
{
	if (isPositiveAndBelow(index, sliderPacks.size()))
		return sliderPacks[index]->getSliderPack(0);

	auto n = new data::dynamic::sliderpack(*this, index);
	n->initialise(getNode());
	sliderPacks.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				for (auto l : safeThis.get()->compileListeners)
				{
					if (l != nullptr)
						l->complexDataAdded(ExternalData::DataType::SliderPack, index);
				}
			}
		});

	return n->getSliderPack(0);
}

hise::MultiChannelAudioBuffer* SnexSource::ComplexDataHandler::getAudioFile(int index)
{
	if (isPositiveAndBelow(index, audioFiles.size()))
		return audioFiles[index]->getAudioFile(0);

	auto n = new data::dynamic::audiofile(*this, index);
	n->initialise(getNode());
	audioFiles.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				for (auto l : safeThis.get()->compileListeners)
				{
					if (l != nullptr)
						l->complexDataAdded(ExternalData::DataType::AudioFile, index);
				}
			}
		});

	return n->getAudioFile(0);
}

hise::SimpleRingBuffer* SnexSource::ComplexDataHandler::getDisplayBuffer(int index)
{
	if (isPositiveAndBelow(index, displayBuffers.size()))
		return displayBuffers[index]->getDisplayBuffer(0);

	auto n = new data::dynamic::displaybuffer(*this, index);
	n->initialise(getNode());
	displayBuffers.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				for (auto l : safeThis.get()->compileListeners)
				{
					if (l != nullptr)
						l->complexDataAdded(ExternalData::DataType::DisplayBuffer, index);
				}
			}
		});

	return n->getDisplayBuffer(0);
}

bool SnexSource::ComplexDataHandler::removeDataObject(ExternalData::DataType t, int index)
{
	ExternalData empty;

	auto i = getAbsoluteIndex(t, index);

	ScopedPointer<ExternalDataHolder> pendingDelete;

	{
		SimpleReadWriteLock::ScopedWriteLock l(getComplexBaseType(t, index)->getDataLock());
		setExternalData(empty, i);

		switch (t)
		{
		case ExternalData::DataType::Table: pendingDelete = tables.removeAndReturn(index); break;
		case ExternalData::DataType::SliderPack: pendingDelete = sliderPacks.removeAndReturn(index); break;
		case ExternalData::DataType::AudioFile: pendingDelete = audioFiles.removeAndReturn(index); break;
		case ExternalData::DataType::FilterCoefficients: pendingDelete = filters.removeAndReturn(index); break;
		case ExternalData::DataType::DisplayBuffer: pendingDelete = displayBuffers.removeAndReturn(index); break;
        default: break;
		}
	}

	return true;
}



snex::ExternalDataHolder* SnexSource::ComplexDataHandler::getDynamicDataHolder(snex::ExternalData::DataType t, int index)
{
	switch (t)
	{
	case snex::ExternalData::DataType::Table: return tables[index];
	case snex::ExternalData::DataType::SliderPack: return sliderPacks[index];
	case snex::ExternalData::DataType::AudioFile: return audioFiles[index];
	case snex::ExternalData::DataType::FilterCoefficients: return filters[index];	
	case snex::ExternalData::DataType::DisplayBuffer: return displayBuffers[index];
    default: break;
	}

	return nullptr;
}

SnexSource::SnexParameter::SnexParameter(SnexSource* n, NodeBase* parent, ValueTree dataTree) :
	Parameter(parent, getTreeInNetwork(parent, dataTree)),
	pIndex(dataTree.getParent().indexOf(dataTree)),
	snexSource(n),
	treeInCodeMetadata(dataTree)
{
	// Let's be very clear about this.
	jassert(!treeInCodeMetadata.isAChildOf(parent->getRootNetwork()->getValueTree()));

	auto& pHandler = n->getParameterHandler();

	switch (pIndex)
	{
	case 0: p = parameter::inner<SnexSource::ParameterHandler, 0>(pHandler); break;
	case 1: p = parameter::inner<SnexSource::ParameterHandler, 1>(pHandler); break;
	case 2: p = parameter::inner<SnexSource::ParameterHandler, 2>(pHandler); break;
	case 3: p = parameter::inner<SnexSource::ParameterHandler, 3>(pHandler); break;
	case 4: p = parameter::inner<SnexSource::ParameterHandler, 4>(pHandler); break;
	case 5: p = parameter::inner<SnexSource::ParameterHandler, 5>(pHandler); break;
	case 6: p = parameter::inner<SnexSource::ParameterHandler, 6>(pHandler); break;
	case 7: p = parameter::inner<SnexSource::ParameterHandler, 7>(pHandler); break;
	case 8: p = parameter::inner<SnexSource::ParameterHandler, 8>(pHandler); break;
	case 9: p = parameter::inner<SnexSource::ParameterHandler, 9>(pHandler); break;
	case 10: p = parameter::inner<SnexSource::ParameterHandler, 10>(pHandler); break;
	case 11: p = parameter::inner<SnexSource::ParameterHandler, 11>(pHandler); break;
	case 12: p = parameter::inner<SnexSource::ParameterHandler, 12>(pHandler); break;
	case 13: p = parameter::inner<SnexSource::ParameterHandler, 13>(pHandler); break;
	case 14: p = parameter::inner<SnexSource::ParameterHandler, 14>(pHandler); break;
	case 15: p = parameter::inner<SnexSource::ParameterHandler, 15>(pHandler); break;
	case 16: p = parameter::inner<SnexSource::ParameterHandler, 16>(pHandler); break;
	default:
		jassertfalse;
	}

	auto ndb = new parameter::dynamic_base(p);
	
	setDynamicParameter(ndb);

	auto ids = RangeHelpers::getRangeIds();
	ids.add(PropertyIds::ID);

	syncer.setPropertiesToSync(dataTree, data, ids, parent->getUndoManager());

	parentValueUpdater.setCallback(data, { PropertyIds::Value }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(SnexSource::SnexParameter::sendValueChangeToParentListeners));
}

juce::ValueTree SnexSource::SnexParameter::getTreeInNetwork(NodeBase* parent, ValueTree dataTree)
{
	for (auto pTree : parent->getParameterTree())
	{
		if (pTree[PropertyIds::ID] == dataTree[PropertyIds::ID])
			return pTree;
	}

	auto treeInNetwork = dataTree.createCopy();
	parent->getParameterTree().addChild(treeInNetwork, -1, parent->getUndoManager());

	return treeInNetwork;
}

void SnexSource::SnexParameter::sendValueChangeToParentListeners(Identifier id, var newValue)
{
	auto d = (double)newValue;

	for (auto l : snexSource->compileListeners)
	{
		if (l != nullptr)
			l->parameterChanged(pIndex, d);
	}
}

snex::jit::FunctionData SnexSource::HandlerBase::getFunctionAsObjectCallback(const String& id, bool checkProcessFunctions)
{
	if (auto wb = parent.getWorkbench())
	{
#if SNEX_MIR_BACKEND

		auto fData = wb->getLastResult().obj[id];

		if (fData.isResolved())
		{
			addObjectPtrToFunction(fData);
			return fData;
		}

#else
		if (auto obj = wb->getLastResult().mainClassPtr)
		{
			auto numChannels = parent.getParentNode()->getCurrentChannelAmount();

			if (numChannels == 0)
				numChannels = 2;

			auto f = obj->getNodeCallback(Identifier(id), numChannels, checkProcessFunctions);

			if (f.isResolved())
			{
				addObjectPtrToFunction(f);
				return f;
			}
			else
			{
				return f;
			}
		}
#endif
	}

	return {};
}

void SnexSource::HandlerBase::addObjectPtrToFunction(FunctionData& f)
{

	jassert(f.isResolved());
	f.object = obj.getObjectPtr();

}

juce::Result SnexSource::ParameterHandlerLight::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
{
#if SNEX_MIR_BACKEND

	auto obj = parent.getWorkbench()->getLastJitObject();

	for (int i = 0; i < numParameters; i++)
	{
		String id("setParameter" + String(i));
		auto f = obj[id];

		if (f.isResolved())
		{
			addObjectPtrToFunction(f);
			pFunctions[i] = f;
			pFunctions[i].callVoid(lastValues[i]);
		}
	}

	return Result::ok();

#else
	using namespace snex::jit;

	snex::jit::FunctionClass::Ptr fc = objectClass->getFunctionClass();
	Array<FunctionData> matches;
	fc->addMatchingFunctions(matches, fc->getClassName().getChildId("setParameter"));

	SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

	for (int i = 0; i < matches.size(); i++)
	{
		auto m = matches[i];

		if (!m.templateParameters[0].constantDefined)
			matches.remove(i--);
	}

	for (int index = 0; index < matches.size(); index++)
	{
		auto objPtr = parent.wb->getLastResult().mainClassPtr;
		String s;
		
		pFunctions[index] = matches[index];

		if (pFunctions[index].templateParameters[0].constant != index)
			return Result::fail("Template index mismatch");

		addObjectPtrToFunction(pFunctions[index]);
		pFunctions[index].callVoid(lastValues[index]);
	}

	return Result::ok();
#endif
}

SnexComplexDataDisplay::SnexComplexDataDisplay(SnexSource* s) :
	source(s)
{
	setName("Complex Data Editor");
	source->addCompileListener(this);
	rebuildEditors();
}



SnexComplexDataDisplay::~SnexComplexDataDisplay()
{
	source->removeCompileListener(this);
}

void SnexComplexDataDisplay::rebuildEditors()
{
	auto updater = source->getParentNode()->getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
	
	auto& dataHandler = source->getComplexDataHandler();

	auto t = snex::ExternalData::DataType::Table;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::table_editor_without_mod(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::SliderPack;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::sliderpack_editor_without_mod(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::FilterCoefficients;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::filter_editor(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::AudioFile;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		data::pimpl::dynamic_base* d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::audiofile_editor(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	setSize(512, editors.size() * 100);
}

SnexMenuBar::SnexMenuBar(SnexSource* s) :
	popupButton("popup", this, f),
	editButton("edit", this, f),
	addButton("add", this, f),
	asmButton("asm", this, f),
	debugButton("debug", this, f),
	optimizeButton("optimize", this, f),
	cdp("popup", this, f),
	source(s)
{
    editButton.setTooltip("Edit this SNEX node in the SNEX Editor floating tile");
    snexIcon = f.createPath("snex");
	s->addCompileListener(this);

	addAndMakeVisible(classSelector);
	classSelector.setColour(ComboBox::ColourIds::textColourId, Colour(0xFFAAAAAA));
	//addAndMakeVisible(popupButton);
	addAndMakeVisible(editButton);
	//addAndMakeVisible(debugButton);
	//addAndMakeVisible(optimizeButton);
	//addAndMakeVisible(asmButton);
	classSelector.setLookAndFeel(&plaf);
	classSelector.addListener(this);

	addAndMakeVisible(addButton);
	addAndMakeVisible(cdp);

	editButton.setToggleModeWithColourChange(true);
	debugButton.setToggleModeWithColourChange(true);
	cdp.setToggleModeWithColourChange(true);

	rebuildComboBoxItems();
	refreshButtonState();

	auto wb = static_cast<snex::ui::WorkbenchManager*>(source->getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());
	wb->addListener(this);
	workbenchChanged(wb->getCurrentWorkbench());

	GlobalHiseLookAndFeel::setDefaultColours(classSelector);
}

void SnexMenuBar::wasCompiled(bool ok)
{
	iconColour = ok ? Colours::green : Colours::red;
	iconColour = iconColour.withSaturation(0.2f).withAlpha(0.8f);

	if (auto nc = findParentComponentOfClass<NodeComponent>())
		nc->repaint();

	repaint();
}

void SnexMenuBar::parameterChanged(int snexParameterId, double newValue)
{

}

SnexMenuBar::~SnexMenuBar()
{
    if(source->getParentNode() == nullptr)
        return;
    
	auto wb = static_cast<snex::ui::WorkbenchManager*>(source->getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());
	wb->removeListener(this);

	if (lastBench != nullptr)
		lastBench->removeListener(this);

	source->removeCompileListener(this);
}

void SnexMenuBar::debugModeChanged(bool isEnabled)
{
	debugMode = isEnabled;

	if (auto nc = findParentComponentOfClass<NodeComponent>())
		nc->repaint();

	repaint();
}

void SnexMenuBar::workbenchChanged(snex::ui::WorkbenchData::Ptr newWb)
{
	if (source->getWorkbench() == newWb)
	{
		if (lastBench != nullptr)
			lastBench->removeListener(this);

		lastBench = newWb.get();
		if (lastBench != nullptr)
		{
			lastBench->addListener(this);
			debugModeChanged(lastBench->getGlobalScope().isDebugModeEnabled());
		}
	}

	editButton.setToggleStateAndUpdateIcon(source->getWorkbench() == newWb && newWb != nullptr, true);
	

	repaint();
}

void SnexMenuBar::complexDataAdded(snex::ExternalData::DataType t, int index)
{
	refreshButtonState();
}

void SnexMenuBar::rebuildComboBoxItems()
{
	classSelector.clear(dontSendNotification);
	classSelector.addItemList(source->getAvailableClassIds(), 1);

	if (auto w = source->getWorkbench())
		classSelector.setText(w->getInstanceId().toString(), dontSendNotification);
}

void SnexMenuBar::refreshButtonState()
{
	bool shouldBeEnabled = source->getWorkbench() != nullptr;

	bool shouldShowPopupButton = false;

	ExternalData::forEachType([&](ExternalData::DataType t)
	{
		shouldShowPopupButton |= source->getComplexDataHandler().getNumDataObjects(t) > 0;
	});
	
	cdp.setVisible(shouldShowPopupButton);

	editButton.setEnabled(shouldBeEnabled);
	cdp.setEnabled(shouldBeEnabled);
	asmButton.setEnabled(shouldBeEnabled);
	debugButton.setEnabled(shouldBeEnabled);
	optimizeButton.setEnabled(shouldBeEnabled);
}

void SnexMenuBar::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	source->setClass(classSelector.getText());
	refreshButtonState();
}

void SnexMenuBar::buttonClicked(Button* b)
{
	if (b == &cdp)
	{
		auto ft = findParentComponentOfClass<FloatingTile>();

		if (ft->isRootPopupShown())
		{
			ft->showComponentInRootPopup(nullptr, nullptr, {});
			
			cdp.setToggleStateAndUpdateIcon(false);
		}
		else
		{
			cdp.setToggleStateAndUpdateIcon(true);
			Component* c = new SnexComplexDataDisplay(source);

			auto nc = findParentComponentOfClass<NodeComponent>();
			auto lb = nc->getLocalBounds();
			auto fBounds = ft->getLocalBounds();

			auto la = ft->getLocalArea(nc, lb);

			auto showAbove = la.getY() > fBounds.getHeight() / 2;

			ft->showComponentInRootPopup(c, nc, { lb.getCentreX(), showAbove ? 0 : lb.getBottom() });
		}
	}

	if (b == &addButton)
	{
		auto enableOptions = source->getWorkbench() != nullptr;

		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(4, "Create new file");
		m.addSeparator();
		m.addItem(1, "Add Parameter", enableOptions);

		ExternalData::forEachType([&](ExternalData::DataType t)
		{
			m.addItem(12 + (int)t, "Add " + ExternalData::getDataTypeName(t), enableOptions);
		});

		m.addSeparator();

		m.addItem(20, "Remove last Parameter", enableOptions);

		ExternalData::forEachType([&](ExternalData::DataType t)
		{
			m.addItem(24 + (int)t, "Remove " + ExternalData::getDataTypeName(t), enableOptions);
		});

		if (auto r = m.show())
		{
			if (r == 4)
			{
				auto name = PresetHandler::getCustomName(source->getId(), "Enter the name for the SNEX class file");

				if (name.isNotEmpty())
				{
					source->setClass(name);
					rebuildComboBoxItems();
					refreshButtonState();
				}

				return;
			}

			if (r == 1)
			{
				auto n = PresetHandler::getCustomName("Parameter");

				if (n.isNotEmpty())
					source->getParameterHandler().addNewParameter(parameter::data(n, { 0.0, 1.0 }));
			}
			else if (r == 20)
				source->getParameterHandler().removeLastParameter();
			else if (r > 20)
			{
				auto t = (ExternalData::DataType)(r - 24);
				source->getComplexDataHandler().addOrRemoveDataFromUI(t, false);
			}
			else
			{
				auto t = (ExternalData::DataType)(r - 12);
				source->getComplexDataHandler().addOrRemoveDataFromUI(t, true);
			}
		}
	}

	if (b == &debugButton)
	{
		source->getWorkbench()->getGlobalScope().setDebugMode(b->getToggleState());
	}

	if (b == &editButton)
	{
		auto& manager = dynamic_cast<BackendProcessor*>(findParentComponentOfClass<FloatingTile>()->getMainController())->workbenches;

		if (b->getToggleState())
			manager.setCurrentWorkbench(source->getWorkbench(), false);
		else
			manager.resetToRoot();

		GET_BACKEND_ROOT_WINDOW(b)->addEditorTabsOfType<SnexEditorPanel>();
	}
}

void SnexMenuBar::paint(Graphics& g)
{
    Colour resultColours[4] = { Colour(0xFF555555),
                                Colour(0xFFBB3434),
                                Colour(0xFFFFBA00),
                                Colour(0xFF4E8E35) };
    
    auto l = (int)source->getErrorLevel();
    
    g.setColour(resultColours[l]);
    g.fillPath(snexIcon);
}

void SnexMenuBar::resized()
{
	auto b = getLocalBounds().reduced(0, 1);
	auto h = getHeight();

	addButton.setBounds(b.removeFromLeft(h-4));

	classSelector.setBounds(b.removeFromLeft(100));
	
	b.removeFromLeft(3);

	editButton.setBounds(b.removeFromRight(h).reduced(2));
	
    f.scalePath(snexIcon, b.removeFromRight(80).reduced(3).toFloat());
    
	cdp.setBounds(b.removeFromLeft(h));
	

	b.removeFromLeft(10);
	
}

#if 0
SnexMenuBar::ComplexDataPopupButton::ComplexDataPopupButton(SnexSource* s) :
	Button("TSA"),
	source(s)
{
	t = s->getComplexDataHandler().getDataRoot();
	l.setTypesToWatch({ Identifier("Tables"), Identifier("SliderPacks"), Identifier("AudioFiles") });
	l.setCallback(t, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(ComplexDataPopupButton::update));

	setClickingTogglesState(true);

	onClick = [this]()
	{
		auto ft = findParentComponentOfClass<FloatingTile>();

		if (getToggleState())
		{
			ft->showComponentInRootPopup(nullptr, nullptr, {});
		}
		else
		{
			Component* c = new SnexComplexDataDisplay(source);

			auto nc = findParentComponentOfClass<NodeComponent>();
			auto lb = nc->getLocalBounds();
			auto fBounds = ft->getLocalBounds();

			auto la = ft->getLocalArea(nc, lb);

			auto showAbove = la.getY() > fBounds.getHeight() / 2;

			ft->showComponentInRootPopup(c, nc, { lb.getCentreX(), showAbove ? 0 : lb.getBottom() });
		}
	};
}
#endif

juce::Path SnexMenuBar::Factory::createPath(const String& url) const
{
	if (url == "snex")
	{
		snex::ui::SnexPathFactory f;
		return f.createPath(url);
	}

	Path p;

	LOAD_PATH_IF_URL("new", ColumnIcons::threeDots);
	LOAD_PATH_IF_URL("edit", ColumnIcons::openWorkspaceIcon);
	LOAD_EPATH_IF_URL("popup", HiBinaryData::ProcessorEditorHeaderIcons::popupShape);
	LOAD_EPATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_EPATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("add", ColumnIcons::threeDots);
	LOAD_EPATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	LOAD_PATH_IF_URL("asm", SnexIcons::asmIcon);
	LOAD_PATH_IF_URL("debug", SnexIcons::bugIcon);
	//LOAD_PATH_IF_URL("optimize", SnexIcons::optimizeIcon);

	return p;
}

}

