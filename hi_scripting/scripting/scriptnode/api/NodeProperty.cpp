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

namespace scriptnode {
using namespace juce;
using namespace hise;


ScriptFunctionManager::~ScriptFunctionManager()
{
	if (mc != nullptr)
		mc->removeScriptListener(this);
}

bool NodeProperty::initialise(NodeBase* n)
{
	jassert(n != nullptr);

	valueTreePropertyid = baseId;

	um = n->getUndoManager();

	auto propTree = n->getPropertyTree();

	d = propTree.getChildWithProperty(PropertyIds::ID, getValueTreePropertyId().toString());

	if (!d.isValid())
	{
		d = ValueTree(PropertyIds::Property);
		d.setProperty(PropertyIds::ID, getValueTreePropertyId().toString(), nullptr);
		d.setProperty(PropertyIds::Value, defaultValue, nullptr);
		d.setProperty(PropertyIds::Public, isPublic, nullptr);
		propTree.addChild(d, -1, n->getUndoManager());
	}

	postInit(n);

	return true;
}

juce::Identifier NodeProperty::getValueTreePropertyId() const
{
	return valueTreePropertyid;
}

void ScriptFunctionManager::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (jp == processor)
	{
		updateFunction(PropertyIds::Callback, functionName);
	}
}

void ScriptFunctionManager::updateFunction(Identifier, var newValue)
{
	if (jp != nullptr)
	{
		auto functionId = Identifier(newValue.toString());

		functionName = newValue;

		function = jp->getScriptEngine()->getInlineFunction(functionId);
		ok = function.getObject() != nullptr;
	}
	else
		ok = false;
}

double ScriptFunctionManager::callWithDouble(double inputValue)
{
	if (ok)
	{
		input[0] = inputValue;
		return (double)jp->getScriptEngine()->executeInlineFunction(function, input, &lastResult, 1);
	}

	return inputValue;
}

void ScriptFunctionManager::postInit(NodeBase* n)
{
	mc = n->getScriptProcessor()->getMainController_();
	mc->addScriptListener(this);

	jp = dynamic_cast<JavascriptProcessor*>(n->getScriptProcessor());

	functionListener.setCallback(getPropertyTree(), { PropertyIds::Value },
		valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(ScriptFunctionManager::updateFunction));
}

void SnexSource::recompiled(WorkbenchData::Ptr wb)
{
	ok = false;

	getParameterHandler().reset();
	getComplexDataHandler().reset();

	if (wb->getLastResult().compiledOk())
	{
		instanceObject = wb->getLastJitObject();

		

		// We need to duplicate the instance when it has no state
		if (!instanceObject.isStateless())
		{
			auto c = wb->getCompileHandler()->createCompiler();

			auto code = wb->getCode();
			preprocess(code);

			instanceObject = c->compileJitObject(code);

			auto tree = getComplexDataHandler().getDataRoot();

			ExternalData::forEachType([&](ExternalData::DataType t)
			{
				auto typeTree = tree.getChild((int)t);
				
				auto id = NamespacedIdentifier(ExternalData::getNumIdentifier(t));
				int numRequired = c->getNamespaceHandler().getConstantValue(id).toInt();

				while (numRequired > typeTree.getNumChildren())
				{
					getComplexDataHandler().addOrRemoveDataFromUI(t, true);
				}
			});
		}
		
		ok = setupCallbacks(instanceObject);

		if (ok)
		{
			getParameterHandler().recompiledOk(instanceObject);
			getComplexDataHandler().recompiledOk(instanceObject);
		}
	}

	for (auto l : compileListeners)
	{
		l->wasCompiled(ok);
	}
}

void SnexSource::ParameterHandler::recompiledOk(snex::JitObject& obj)
{
	for (auto p : parameterTree)
	{
		auto index = p.getParent().indexOf(p);
		String fId = "set";
		fId << p[PropertyIds::ID].toString();
		pFunctions[index] = obj[fId];

		if (auto np = getNode()->getParameter(p[PropertyIds::ID].toString()))
			pFunctions[index].callVoid(np->getValue());
	}
}

void SnexSource::ComplexDataHandler::recompiledOk(JitObject& obj)
{
	if (externalFunction = obj["setExternalData"])
	{
		

		if (hasComplexData())
		{
			ExternalData::forEachType([this](ExternalData::DataType t)
			{
				for (int i = 0; i < getNumDataObjects(t); i++)
				{
					auto absoluteIndex = getAbsoluteIndex(t, i);

					ExternalData ed(getComplexBaseType(t, i), absoluteIndex);

					SimpleReadWriteLock::ScopedWriteLock l(ed.obj->getDataLock());
					setExternalData(ed, absoluteIndex);
				}
			});
		}
	}
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
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::AudioFile)) t = ExternalData::DataType::AudioFile;

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
	case snex::ExternalData::DataType::Table: return tables.size();
	case snex::ExternalData::DataType::AudioFile: return audioFiles.size();
	case snex::ExternalData::DataType::SliderPack: return sliderPacks.size();
	}
}

hise::Table* SnexSource::ComplexDataHandler::getTable(int index)
{
	if (isPositiveAndBelow(index, tables.size()))
	{
		return tables[index]->getTable(0);
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
	if(isPositiveAndBelow(index, sliderPacks.size()))
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
	}
}

SnexSource::SnexParameter::SnexParameter(SnexSource* n, NodeBase* parent, ValueTree dataTree) :
	Parameter(parent, dataTree),
	pIndex(dataTree.getParent().indexOf(dataTree))
{
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
	}

	auto ndb = new parameter::dynamic_base(p);
	ndb->dataTree = dataTree;
	setCallbackNew(ndb);

	for (auto pTree : parent->getParameterTree())
	{
		if (pTree[PropertyIds::ID] == dataTree[PropertyIds::ID])
		{
			treeInNetwork = pTree;
			break;
		}
	}

	if (!treeInNetwork.isValid())
	{
		treeInNetwork = dataTree.createCopy();
		parent->getParameterTree().addChild(treeInNetwork, -1, parent->getUndoManager());
	}

	setTreeWithValue(treeInNetwork);

	auto ids = RangeHelpers::getRangeIds();
	ids.add(PropertyIds::ID);

	syncer.setPropertiesToSync(dataTree, treeInNetwork, ids, parent->getUndoManager());
}

}