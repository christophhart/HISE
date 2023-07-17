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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

namespace ScriptingObjects
{

#define DECLARE_ID(x) static const Identifier x(#x);

namespace MatrixIds
{
	DECLARE_ID(Chain);
	DECLARE_ID(Component);
	DECLARE_ID(ID);
	DECLARE_ID(Intensity);
	DECLARE_ID(Inverted);
	DECLARE_ID(Mode);
	DECLARE_ID(Parameter);
	DECLARE_ID(Source);
	DECLARE_ID(Target);
    DECLARE_ID(AddConstUIMod);
	DECLARE_ID(items);
}

struct ValueModeHelpers
{
	static String getModeName(ScriptModulationMatrix::ValueMode m)
	{
		static const StringArray modulationModeList = { "Default", "Scale", "Unipolar", "Bipolar", "Undefined" };

		return modulationModeList[(int)m];
	}

	static ScriptModulationMatrix::ValueMode getModeToUse(ScriptModulationMatrix::ValueMode defaultMode, ScriptModulationMatrix::ValueMode connectionMode)
	{
		if (connectionMode != ScriptModulationMatrix::ValueMode::Default && connectionMode != ScriptModulationMatrix::ValueMode::Undefined)
			return connectionMode;

		return defaultMode;
	}

	static ScriptModulationMatrix::ValueMode getMode(const String& m)
	{
		static const StringArray modulationModeList = { "Default", "Scale", "Unipolar", "Bipolar"};

		if (!modulationModeList.contains(m))
			return ScriptModulationMatrix::ValueMode::Undefined;

		return (ScriptModulationMatrix::ValueMode)modulationModeList.indexOf(m);
	}

	static float getIntensityValue(const var& obj)
	{
		var intObject = obj[MatrixIds::Intensity];

		float s;

		if (intObject.isObject())
		{
			s = (float)intObject.getProperty(scriptnode::PropertyIds::Value, 0.0f);
		}
		else
			s = (float)intObject;

		FloatSanitizers::sanitizeFloatNumber(s);
		return s;
	}

	
};

using RoutingManager = scriptnode::routing::GlobalRoutingManager;

struct ScriptingObjects::ScriptModulationMatrix::SourceData : public ControlledObject
{
	

	SourceData(ScriptModulationMatrix* parent_, Modulator* mod_) :
		ControlledObject(parent_->getMainController()),
		parent(parent_),
		mod(mod_)
	{
		rm = RoutingManager::Helpers::getOrCreate(getMainController());
		cable = rm->getSlotBase(mod->getId(), RoutingManager::SlotBase::SlotType::Cable);

		parent->container->connectToGlobalCable(mod, var(cable.get()), true);
	}

	RoutingManager::Ptr rm;
	RoutingManager::SlotBase::Ptr cable;
	WeakReference<ScriptModulationMatrix> parent;
	WeakReference<Modulator> mod;
};

struct ScriptingObjects::ScriptModulationMatrix::ParameterTargetCable : public RoutingManager::CableTargetBase,
	public ReferenceCountedObject
{
	ParameterTargetCable(ParameterTargetData* p, const String& sourceId_) :
		parent(p),
		sourceId(sourceId_)
	{};

	virtual void selectCallback(Component*) {};

	virtual String getTargetId() const { return "Modulation Matrix Parameter Target"; }

	void sendValue(double v) override
	{
		if (v != value)
		{
			value = v;
			parent->updateValue();
		}
	}

	Path getTargetIcon() const override
	{
		return {};
	}

	String sourceId;
	double value = 1.0;
	double intensity = 1.0;
	bool inverted = false;

	ScriptModulationMatrix::ValueMode customMode = ScriptModulationMatrix::ValueMode::Default;

	WeakReference<ParameterTargetData> parent;
};

struct ScriptingObjects::ScriptModulationMatrix::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, addModulatorTarget);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, addParameterTarget);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setNumModulationSlots);
	API_METHOD_WRAPPER_3(ScriptModulationMatrix, connect);
	API_METHOD_WRAPPER_1(ScriptModulationMatrix, getModValue);
	API_METHOD_WRAPPER_1(ScriptModulationMatrix, getTargetId);
	API_METHOD_WRAPPER_1(ScriptModulationMatrix, getComponentId);
	API_METHOD_WRAPPER_1(ScriptModulationMatrix, getConnectionData);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, toBase64);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, fromBase64);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setConnectionCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, updateConnectionData);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setEditCallback);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, getSourceList);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, getTargetList);
	API_METHOD_WRAPPER_2(ScriptModulationMatrix, getIntensitySliderData);
	API_METHOD_WRAPPER_2(ScriptModulationMatrix, getValueModeData);
	API_METHOD_WRAPPER_3(ScriptModulationMatrix, updateIntensity);
	API_METHOD_WRAPPER_3(ScriptModulationMatrix, updateValueMode);
	API_METHOD_WRAPPER_2(ScriptModulationMatrix, canConnect);
	API_VOID_METHOD_WRAPPER_0(ScriptModulationMatrix, clearAllConnections);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setUseUndoManager);
};

ScriptModulationMatrix::ScriptModulationMatrix(ProcessorWithScriptingContent* p, const String& cid) :
	ConstScriptingObject(p, 10),
	ControlledObject(p->getMainController_()),
	connectionCallback(p, nullptr, var(), 3),
	editCallback(p, nullptr, var(), 1)
{
	container = dynamic_cast<GlobalModulatorContainer*>(ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), cid));

	if (container == nullptr)
		reportScriptError(cid + " is not a global modulation container");

	addConstant("Gain", raw::IDs::Chains::Gain);
	addConstant("Pitch", raw::IDs::Chains::Pitch);
	addConstant("Frequency", raw::IDs::Chains::FilterFrequency);
	addConstant("Scale", "Scale");
	addConstant("Intensity", "Intensity");
	addConstant("Bipolar", "Bipolar");
	addConstant("Unipolar", "Unipolar");
	addConstant("Add", "Add");
	addConstant("Delete", "Delete");
	addConstant("Update", "Update");
	addConstant("Rebuild", "Rebuild");

	ADD_API_METHOD_1(addModulatorTarget);
	ADD_API_METHOD_1(addParameterTarget);
	ADD_API_METHOD_1(setNumModulationSlots);
	ADD_API_METHOD_3(connect);
	ADD_API_METHOD_1(getModValue);
	ADD_API_METHOD_1(getConnectionData);
	ADD_API_METHOD_1(fromBase64);
	ADD_API_METHOD_0(toBase64);
	ADD_API_METHOD_1(setConnectionCallback);
	ADD_API_METHOD_1(updateConnectionData);
	ADD_API_METHOD_1(setEditCallback);
	ADD_API_METHOD_0(getSourceList);
	ADD_API_METHOD_0(getTargetList);
	ADD_API_METHOD_2(getIntensitySliderData);
	ADD_API_METHOD_3(updateIntensity);
	ADD_API_METHOD_2(getValueModeData);
	ADD_API_METHOD_3(updateValueMode);
	ADD_API_METHOD_1(getTargetId);
	ADD_API_METHOD_1(getComponentId);
	ADD_API_METHOD_2(canConnect);
	ADD_API_METHOD_0(clearAllConnections);
	ADD_API_METHOD_1(setUseUndoManager);

	auto h = dynamic_cast<ModulatorChain*>(container->getChildProcessor(1));

	for (int i = 0; i < h->getNumChildProcessors(); i++)
		sourceData.add(new SourceData(this, dynamic_cast<Modulator*>(h->getChildProcessor(i))));

	broadcaster.addListener(*this, onUpdateMessage, false);

	getScriptProcessor()->getMainController_()->getUserPresetHandler().addStateManager(this);
}



ScriptModulationMatrix::~ScriptModulationMatrix()
{
	getScriptProcessor()->getMainController_()->getUserPresetHandler().removeStateManager(this);
}

juce::ValueTree ScriptModulationMatrix::exportAsValueTree() const
{
	Array<var> l;

	for (auto t : targetData)
	{
		l.addArray(*t->getConnectionData().getArray());
	}

	return ValueTreeConverters::convertVarArrayToFlatValueTree(var(l), getUserPresetStateId(), "Connection");
}

void ScriptModulationMatrix::restoreFromValueTree(const ValueTree &previouslyExportedState)
{
	auto obj = ValueTreeConverters::convertFlatValueTreeToVarArray(previouslyExportedState);

	ScopedRefreshDeferrer srd(*this);

	clearConnectionsInternal();

	if (obj.isArray())
	{
		for (auto& c : *obj.getArray())
		{
			for (auto& t : targetData)
			{
				if (c["Target"].toString() == t->modId)
				{
					t->connect(c["Source"].toString(), true);
					t->updateConnectionData(c);
				}
			}
		}
	}
}

void ScriptModulationMatrix::setNumModulationSlots(var numSlotArray)
{
	if (!getScriptProcessor()->objectsCanBeCreated())
		reportScriptError("You must declare all modulation targets at onInit");

	if (numSlotArray.isArray() && numSlotArray.size() == 3)
	{
		numSlots[0] = (int)numSlotArray[0];
		numSlots[1] = (int)numSlotArray[1];
		numSlots[2] = (int)numSlotArray[2];
	}
	else
	{
		reportScriptError("You must pass in an array with three numbers into setNumModulationSlots");
	}
}

void ScriptModulationMatrix::addModulatorTarget(var newTargetData)
{
	if (!getScriptProcessor()->objectsCanBeCreated())
		reportScriptError("You must declare all modulation targets at onInit");

	targetData.add(new ModulatorTargetData(this, newTargetData));
	refreshBypassStates();
}

void ScriptModulationMatrix::addParameterTarget(var newTargetData)
{
	if (!getScriptProcessor()->objectsCanBeCreated())
		reportScriptError("You must declare all modulation targets at onInit");

	targetData.add(new ParameterTargetData(this, newTargetData));
	refreshBypassStates();
}

bool ScriptModulationMatrix::connect(String sourceId, String targetId, bool addConnection)
{
	if (um == nullptr)
	{
		return connectInternal(sourceId, targetId, addConnection);
	}
	else
	{
		auto at = addConnection ? MatrixUndoAction::ActionType::Add : MatrixUndoAction::ActionType::Remove;

		return um->perform(new MatrixUndoAction(this, at, {}, {}, sourceId, targetId));
	}
}

float ScriptModulationMatrix::getModValue(var component)
{
	String componentId;

	if (component.isString())
		componentId = component.toString();
	else if (auto sc = dynamic_cast<ScriptComponent*>(component.getObject()))
		componentId = sc->getId();

	for (auto t : targetData)
	{
		if (t->componentId == componentId)
		{
			return t->getModValue();
		}
	}

	return 1.0f;
}

String ScriptModulationMatrix::getTargetId(String componentId)
{
	for (auto t : targetData)
	{
		if (t->componentId == componentId)
			return t->modId;
	}

	return {};
}

String ScriptModulationMatrix::getComponentId(String targetId)
{
	for (auto t : targetData)
	{
		if (t->modId == targetId)
			return t->componentId;
	}

	return {};
}

bool ScriptModulationMatrix::canConnect(String source, String target)
{
	for (auto t : targetData)
	{
		if (t->modId == target)
			return t->canConnect(source);
	}

	return false;
}

juce::var ScriptModulationMatrix::getConnectionData(String componentId)
{
	Array<var> data;

	for (auto t : targetData)
	{
		if (t->componentId == componentId || componentId.isEmpty())
			data.addArray(*t->getConnectionData().getArray());
	}

	return var(data);
}

void ScriptModulationMatrix::updateConnectionData(var connectionList)
{
	if (connectionList.getDynamicObject() != nullptr)
	{
		Array<var> x;
		x.add(connectionList);
		updateConnectionData(x);
		return;
	}

	if (um == nullptr)
	{
		updateConnectionDataInternal(connectionList);
	}
	else
	{
		var oldValue = toBase64();

		um->perform(new MatrixUndoAction(this, MatrixUndoAction::ActionType::Update, oldValue, connectionList));
	}
}

bool ScriptModulationMatrix::updateIntensity(String source, String target, float intensityValue)
{
	if (um == nullptr)
	{
		return updateIntensityInternal(source, target, intensityValue);
	}
	else
	{
		auto oldValue = getIntensitySliderData(source, target)[scriptnode::PropertyIds::Value];
		var newValue(intensityValue);

		return um->perform(new MatrixUndoAction(this, MatrixUndoAction::ActionType::Intensity, oldValue, newValue, source, target));;
	}	
}

bool ScriptModulationMatrix::updateValueMode(String source, String target, String valueMode)
{
	if(um == nullptr)
		return updateValueModeInternal(source, target, valueMode);
	else
	{
		auto oldValue = getValueModeData(source, target)[scriptnode::PropertyIds::Value];
		var newValue(valueMode);

		return um->perform(new MatrixUndoAction(this, MatrixUndoAction::ActionType::ValueMode, oldValue, newValue, source, target));
	}
}


String ScriptModulationMatrix::toBase64()
{
	zstd::ZDefaultCompressor comp;
	
	auto v = exportAsValueTree();
	MemoryBlock mb;

	comp.compress(v, mb);

	return mb.toBase64Encoding();
}

void ScriptModulationMatrix::fromBase64(String b64)
{
	zstd::ZDefaultCompressor comp;

	MemoryBlock mb;
	mb.fromBase64Encoding(b64);

	ValueTree v;
	comp.expand(mb, v);

	restoreFromValueTree(v);
}

void ScriptModulationMatrix::clearAllConnections()
{
	if(um == nullptr)
		clearConnectionsInternal();
	else
	{
		um->perform(new MatrixUndoAction(this, MatrixUndoAction::ActionType::Clear, toBase64(), var()));
	}
}

void ScriptModulationMatrix::setConnectionCallback(var updateFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(updateFunction))
	{
		connectionCallback = WeakCallbackHolder(getScriptProcessor(), this, updateFunction, 3);
		connectionCallback.incRefCount();
		connectionCallback.setHighPriority();
	}
}

void ScriptModulationMatrix::setEditCallback(var editFunction)
{
	if (!targetData.isEmpty())
		reportScriptError("You must call this function before adding modulation targets");

	if (HiseJavascriptEngine::isJavascriptFunction(editFunction))
	{
		editCallback = WeakCallbackHolder(getScriptProcessor(), this, editFunction, 1);
		editCallback.incRefCount();
		editCallback.setThisObject(this);
	}
}

juce::var ScriptModulationMatrix::getSourceList() const
{
	Array<var> list;

	for (auto s : sourceData)
		list.add(s->mod->getId());

	return var(list);
}

juce::var ScriptModulationMatrix::getTargetList() const
{
	Array<var> list;

	for (auto t : targetData)
		list.add(t->modId);

	return var(list);
}

var ScriptModulationMatrix::getIntensitySliderData(String sourceId, String targetId)
{
	for (auto t : targetData)
	{
		if (t->modId == targetId)
			return t->getIntensitySliderData(sourceId);
	}

	return {};
}

juce::var ScriptModulationMatrix::getValueModeData(String sourceId, String targetId)
{
	for (auto t : targetData)
	{
		if (t->modId == targetId)
			return t->getValueModeData(sourceId);
	}

	return {};
}

void ScriptModulationMatrix::setUseUndoManager(bool shouldUseUndoManager)
{
	if (shouldUseUndoManager)
		um = getScriptProcessor()->getMainController_()->getControlUndoManager();
	else
		um = nullptr;
}

void ScriptModulationMatrix::sendUpdateMessage(String source, String target, ConnectionEvent eventType)
{
	if (!connectionCallback)
		return;

	broadcaster.sendMessage(deferRefresh ? dontSendNotification : sendNotificationAsync, source, target, eventType);
}

void ScriptModulationMatrix::onUpdateMessage(ScriptModulationMatrix& m, const String& source, const String& target, ConnectionEvent eventType)
{
	if (m.connectionCallback)
	{
		static const StringArray eventNames = {
			"Add",
			"Delete",
			"Update",
			"Intensity",
			"ValueMode",
			"Rebuild"
		};

		m.connectionCallback.call({ var(source), var(target), var(eventNames[(int)eventType]) });
	}
}



void ScriptModulationMatrix::refreshBypassStates()
{
	if (deferRefresh)
		return;

	Array<TargetDataBase*> activeTargets;

	for (auto s : sourceData)
	{
		bool found = false;

		for (auto& t : targetData)
		{
			bool tfound = t->checkActiveConnections(s->mod->getId());

			if (tfound)
				activeTargets.add(t);

			found |= tfound;
		}

		s->mod->setBypassed(!found, sendNotificationAsync);

		s->cable->cleanup();
	}

	for (auto t : targetData)
	{
		if (activeTargets.contains(t))
			t->start();
		else
			t->stop();
	}


}

void ScriptModulationMatrix::clearConnectionsInternal()
{
	for (auto t : targetData)
	{
		t->clear();
	}

	refreshBypassStates();
	sendUpdateMessage("", "", ConnectionEvent::Rebuild);
}

void ScriptModulationMatrix::updateConnectionDataInternal(var connectionList)
{
	if (connectionList.isArray())
	{
		for (const auto& s : *connectionList.getArray())
		{
			auto targetId = s["Target"].toString();

			if (targetId.isEmpty())
				reportScriptError("missing target ID");

			for (auto t : targetData)
			{
				if (t->modId == targetId)
				{
					t->updateConnectionData(s);
					sendUpdateMessage(s["Source"].toString(), targetId, ConnectionEvent::Update);
				}
			}
		}
	}

}

bool ScriptModulationMatrix::connectInternal(const String& sourceId, const String& targetId, bool addConnection)
{
	for (auto s : sourceData)
	{
		if (s->mod->getId() == sourceId)
		{
			for (auto t : targetData)
			{
				if (t->modId == targetId)
				{
					if (t->connect(sourceId, addConnection))
					{
						dynamic_cast<ScriptComponent*>(t->sc.getObject())->sendRepaintMessage();
						refreshBypassStates();
						return true;
					}
				}
			}
		}
	}

	refreshBypassStates();
	return false;
}

bool ScriptModulationMatrix::updateIntensityInternal(String source, String target, float intensityValue)
{
	for (auto t : targetData)
	{
		if (t->modId == target)
		{
			if (t->updateIntensity(source, intensityValue))
			{
				sendUpdateMessage(source, target, ConnectionEvent::Intensity);
				return true;
			}

		}
	}

	return false;
}

bool ScriptModulationMatrix::updateValueModeInternal(String source, String target, String valueMode)
{
	auto mode = ValueModeHelpers::getMode(valueMode);

	if (mode == ValueMode::Undefined)
		reportScriptError("invalid value mode " + valueMode);

	for (auto t : targetData)
	{
		if (t->modId == target)
		{
			if (t->updateValueMode(source, mode))
			{
				sendUpdateMessage(source, target, ConnectionEvent::ValueMode);
				return true;
			}
		}
	}

	return false;
}

int ScriptModulationMatrix::getSourceIndex(const String& id) const
{
	int idx = 0;

	for (auto s : sourceData)
	{
		if (s->mod->getId() == id)
			return idx;

		idx++;
	}

	return -1;
}

int ScriptModulationMatrix::getTargetIndex(const String& id) const
{
	int idx = 0;

	for (auto t : targetData)
	{
		if (t->modId == id)
			return idx;

		idx++;
	}

	return -1;
}

hise::Modulator* ScriptModulationMatrix::getSourceMod(const String& id) const
{
	auto index = getSourceIndex(id);

	if (isPositiveAndBelow(index, sourceData.size()))
		return sourceData[index]->mod.get();

	return nullptr;
}

juce::ReferenceCountedObject* ScriptModulationMatrix::getSourceCable(const String& id) const
{
	auto index = getSourceIndex(id);

	if (isPositiveAndBelow(index, sourceData.size()))
		return sourceData[index]->cable.get();

	return nullptr;
}

ScriptModulationMatrix::TargetDataBase::TargetDataBase(ScriptModulationMatrix* p, const var& json, bool isMod_) :
	SimpleTimer(p->getMainController()->getGlobalUIUpdater()),
	parent(p),
	isMod(isMod_)
{

}





void ScriptModulationMatrix::TargetDataBase::timerCallback()
{
	auto thisValue = getModValue();

	if (thisValue != lastValue)
	{
		lastValue = thisValue;
		dynamic_cast<ScriptComponent*>(sc.getObject())->sendRepaintMessage();
	}
}

void ScriptModulationMatrix::TargetDataBase::init(const var& json)
{
	auto p = parent.get();

	verifyProperty(json, MatrixIds::ID);

	modId = json[MatrixIds::ID].toString();

	verifyProperty(json, MatrixIds::Target);

	auto targetId = json[MatrixIds::Target].toString();

	processor = ProcessorHelpers::getFirstProcessorWithName(p->getMainController()->getMainSynthChain(), targetId);

	verifyExists(processor.get(), MatrixIds::Target);

	verifyProperty(json, MatrixIds::Component);

	componentId = json[MatrixIds::Component].toString();

	sc = var(p->getScriptProcessor()->getScriptingContent()->getComponentWithName(componentId));

	verifyExists(sc.getObject(), MatrixIds::Component);

	if (auto comp = dynamic_cast<ScriptComponent*>(sc.getObject()))
	{
		r.rng.start = comp->getScriptObjectProperty(ScriptComponent::Properties::min);
		r.rng.end = comp->getScriptObjectProperty(ScriptComponent::Properties::max);

		if (comp->hasProperty("middlePosition"))
		{
			auto mp = comp->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::middlePosition);

			if (r.rng.getRange().contains(mp))
				r.rng.setSkewForCentre(mp);
		}

		if (comp->hasProperty("stepSize"))
			r.rng.interval = comp->getScriptObjectProperty("stepSize");
		if (dynamic_cast<ScriptingApi::Content::ScriptComboBox*>(comp))
			r.rng.interval = 1.0;
	}

	
}

hise::MacroControlledObject::ModulationPopupData::Ptr ScriptModulationMatrix::TargetDataBase::getModulationData()
{
	MacroControlledObject::ModulationPopupData::Ptr newData = new MacroControlledObject::ModulationPopupData();

	newData->modulationId = modId;

	for (auto s : parent->sourceData)
		newData->sources.add(s->mod->getId());

	WeakReference<ScriptModulationMatrix> safeParent(parent.get());
	WeakReference<TargetDataBase> safeThis(this);

	if (parent->editCallback)
	{
		newData->editCallback = [safeParent](String name)
		{
			if (safeParent != nullptr)
				safeParent->editCallback.call1(name);
		};
	}

	newData->queryFunction = [safeThis](int index, bool checkTicked)
	{
		if (safeThis != nullptr)
			return safeThis->queryFunction(index, checkTicked);

		return false;
	};

	newData->toggleFunction = [safeThis](int index, bool value)
	{
		if (safeThis != nullptr)
		{
			auto source = safeThis->parent->sourceData[index]->mod->getId();
			
			safeThis->parent->connect(source, safeThis->modId, value);
		}
	};

	return newData;
}

void ScriptModulationMatrix::TargetDataBase::verifyProperty(const var& json, const Identifier& id)
{
	if (!json.hasProperty(id))
		parent->reportScriptError("JSON must have property " + id.toString().quoted());
}

void ScriptModulationMatrix::TargetDataBase::verifyExists(void* obj, const Identifier& id)
{
	if (obj == nullptr)
		parent->reportScriptError(id + " does not exist");
}

void ScriptModulationMatrix::ModulatorTargetData::updateConnectionData(const var& obj)
{
	auto sm = parent->getSourceMod(obj[MatrixIds::Source]);

	forEach(sm, [&obj](Modulator* sm, ModulatorTargetData& data, GlobalModulator* gm)
	{
		if (gm->isConnected() && gm->getOriginalModulator() == sm)
		{
			auto intensity = ValueModeHelpers::getIntensityValue(obj);
			auto newMode = ValueModeHelpers::getMode(obj[MatrixIds::Mode].toString());

			data.updateValueMode(sm->getId(), newMode);
			data.updateIntensity(sm->getId(), intensity);
			
			dynamic_cast<Modulator*>(gm)->setAttribute(GlobalModulator::Inverted, (int)obj.getProperty(MatrixIds::Inverted, 0), sendNotification);
			return true;
		}

		return false;
	});
}

juce::var ScriptModulationMatrix::ModulatorTargetData::getConnectionData() const
{
	Array<var> data;

	forEach(nullptr, [&data](Modulator* sm, ModulatorTargetData& td, GlobalModulator* gm)
	{
		if (gm->isConnected())
		{
			DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty(MatrixIds::Source, gm->getOriginalModulator()->getId());
			obj->setProperty(MatrixIds::Target, td.modId);

			auto mode = td.getValueModeValue(gm);
			obj->setProperty(MatrixIds::Mode, mode);

			auto intensity = td.getIntensityValue(gm);
			obj->setProperty(MatrixIds::Intensity, intensity);
			obj->setProperty(MatrixIds::Inverted, dynamic_cast<Modulator*>(gm)->getAttribute(GlobalModulator::Inverted));

			data.add(var(obj.get()));
		}

		return false;
	});

	return data;
}

hise::MacroControlledObject::ModulationPopupData::Ptr ScriptModulationMatrix::ModulatorTargetData::getModulationData()
{
	auto newData = TargetDataBase::getModulationData();

	if (componentMod != nullptr || targetMode == TargetMode::FrequencyMode)
	{
		WeakReference<ModulatorTargetData> safeThis(this);

		newData->valueCallback = [safeThis](double v)
		{
			if (safeThis != nullptr)
			{
				safeThis->componentValue = v;
				safeThis->updateValue();
			}
		};
	}

	return newData;
}

int ScriptModulationMatrix::ModulatorTargetData::getTypeIndex(GlobalModulator* gm) const
{
	auto asMod = dynamic_cast<Modulator*>(gm);

	for (int i = 0; i < 3; i++)
	{
		if (modulators[i].contains(asMod))
			return i;

		if (targetMode == TargetMode::FrequencyMode && freqAddMods[i].contains(asMod))
			return i;
	}

	jassertfalse;
	return -1;
}

void ScriptModulationMatrix::ModulatorTargetData::updateValue()
{
    if(componentMod != nullptr)
    {
        auto mv = componentValue;
        
        auto mod = dynamic_cast<Modulation*>(componentMod.get());
        
        if(mod->getMode() == Modulation::Mode::GainMode)
            mv = 1.0f - mv;
        
        mod->setIntensityFromSlider(mv);
    }
	else if(targetMode == TargetMode::FrequencyMode)
    {
		processor->setAttribute(PolyFilterEffect::Parameters::Frequency, componentValue, sendNotification);
	}
}

bool ScriptModulationMatrix::ModulatorTargetData::isBipolarFreqMod(GlobalModulator* gm) const
{
	if (targetMode != TargetMode::FrequencyMode)
		return false;

	auto asMod = dynamic_cast<Modulator*>(gm);

	return freqAddMods[0].contains(asMod) ||
		   freqAddMods[1].contains(asMod) ||
		   freqAddMods[2].contains(asMod);
}

String ScriptModulationMatrix::ModulatorTargetData::getValueModeValue(GlobalModulator* gm) const
{
	if (dynamic_cast<Modulation*>(gm)->getMode() == Modulation::Mode::GainMode)
	{
		// This will only happen in frequency mode...
		return ValueModeHelpers::getModeName(ValueMode::Scale);
	}

	auto isBipolar = dynamic_cast<Modulation*>(gm)->isBipolar();

	return ValueModeHelpers::getModeName(isBipolar ? ValueMode::Bipolar : ValueMode::Unipolar);
}

double ScriptModulationMatrix::ModulatorTargetData::getIntensityValue(GlobalModulator* gm) const
{
	auto intensity = dynamic_cast<Modulation*>(gm)->getDisplayIntensity();

	if (isBipolarFreqMod(gm))
		intensity *= 0.01f;

	return intensity;
}

bool ScriptModulationMatrix::ModulatorTargetData::canConnect(const String& sourceId) const
{
	auto sm = parent->getSourceMod(sourceId);

	bool alreadyConnected = forEach(sm, [](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		return gm->getOriginalModulator() == sm;
	});

	if (alreadyConnected)
		return false;

	return forEach(sm, [](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		if (gm->getOriginalModulator() == nullptr)
			return true;
		
		return false;
	});
}

bool ScriptModulationMatrix::ModulatorTargetData::connect(const String& sourceId, bool addConnection)
{
	auto sm = parent->getSourceMod(sourceId);

	return forEach(sm, [sourceId, addConnection](Modulator* sm, TargetDataBase& d, GlobalModulator* gm)
	{
		if (gm->getOriginalModulator() == sm)
		{
			if (!addConnection)
			{
				gm->disconnect();
				dynamic_cast<ScriptComponent*>(d.sc.getObject())->sendRepaintMessage();
				d.parent->sendUpdateMessage(sourceId, d.modId, ConnectionEvent::Delete);
			}

			return true;
		}

		if (!gm->isConnected() && addConnection)
		{
			auto item = d.parent->container->getId() + ":" + sourceId;
			gm->connectToGlobalModulator(item);
			dynamic_cast<ScriptComponent*>(d.sc.getObject())->sendRepaintMessage();
			d.parent->sendUpdateMessage(sourceId, d.modId, ConnectionEvent::Add);
			return true;
		}

		return false;
	});
}

void ScriptModulationMatrix::ModulatorTargetData::clear()
{
	forEach(nullptr, [](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		gm->disconnect();
		auto mod = dynamic_cast<Modulation*>(gm);
		mod->setIntensity(mod->getInitialValue());

		return false;
	});

	dynamic_cast<ScriptComponent*>(sc.getObject())->sendRepaintMessage();
}

juce::var ScriptModulationMatrix::ModulatorTargetData::getIntensitySliderData(String sourceId) const
{
	auto sm = parent->getSourceMod(sourceId);

	var obj;

	scriptnode::InvertableParameterRange rng;

	switch (targetMode)
	{
	case TargetMode::GainMode: 
		rng = { 0.0, 1.0 }; break;
	case TargetMode::PitchMode: 
		rng = { -12.0, 12.0 }; 
		rng.rng.interval = dynamic_cast<ScriptComponent*>(sc.getObject())->getScriptObjectProperty(Identifier("stepSize"));
		break;
	case TargetMode::FrequencyMode:
		rng = { 0.0, 1.0 }; break;
    case TargetMode::GlobalMode:
        rng = { 0.0, 1.0 }; break;
	case TargetMode::PanMode: 
		rng = { -100.0, 100.0 }; 
		rng.rng.interval = 1.0;
		break;
    default:
        jassertfalse;
        break;
	}

	scriptnode::RangeHelpers::storeDoubleRange(obj, rng);

	obj.getDynamicObject()->setProperty("defaultValue", 0.0);

	forEach(sm, [&](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		if (gm->getOriginalModulator() == sm)
		{
			if(isBipolarFreqMod(gm))
				obj.getDynamicObject()->setProperty(scriptnode::PropertyIds::MinValue, -1.0);

			obj.getDynamicObject()->setProperty(scriptnode::PropertyIds::Value, d.getIntensityValue(gm));
			return true;
		}

		return false;
	});

	if (!obj.hasProperty(scriptnode::PropertyIds::Value))
	{
		obj.getDynamicObject()->setProperty(scriptnode::PropertyIds::Value, 0.0);
	}

	return obj;
}

juce::var ScriptModulationMatrix::ModulatorTargetData::getValueModeData(const String& sourceId) const
{
	Array<var> modes;

	auto modChain = dynamic_cast<Modulation*>(processor->getChildProcessor(subIndex));
	auto mode = modChain->getMode();

	switch (targetMode)
	{
	case TargetMode::GainMode: 
		modes.add(ValueModeHelpers::getModeName(ValueMode::Scale));
		break;
	case TargetMode::FrequencyMode:
		modes.add(ValueModeHelpers::getModeName(ValueMode::Scale));
		modes.add(ValueModeHelpers::getModeName(ValueMode::Unipolar));
		modes.add(ValueModeHelpers::getModeName(ValueMode::Bipolar));
		break;
	default:
		modes.add(ValueModeHelpers::getModeName(ValueMode::Unipolar));
		modes.add(ValueModeHelpers::getModeName(ValueMode::Bipolar));
		break;
	}

	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(MatrixIds::items, var(modes));

	if (targetMode != TargetMode::FrequencyMode && mode == Modulation::Mode::GainMode)
		obj->setProperty(scriptnode::PropertyIds::Value, ValueModeHelpers::getModeName(ValueMode::Scale));
	else
	{
		forEach(parent->getSourceMod(sourceId), [&](Modulator* sm, ModulatorTargetData& m, GlobalModulator* gm)
		{
			if (gm->getOriginalModulator() == sm)
			{
				obj->setProperty(scriptnode::PropertyIds::Value, m.getValueModeValue(gm));
				return true;
			}

			return false;
		});
	}

	return var(obj.get());
}

bool ScriptModulationMatrix::ModulatorTargetData::updateValueMode(const String& sourceId, ValueMode newMode)
{
	return forEach(parent->getSourceMod(sourceId), [newMode](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		if (d.targetMode == TargetMode::FrequencyMode)
		{
			if (gm->getOriginalModulator() == sm)
			{
				auto asMod = dynamic_cast<Modulator*>(gm);

				auto typeIndex = d.getTypeIndex(gm);

				for (int i = 0; i < d.freqValueModes[typeIndex].size(); i++)
				{
					if (d.modulators[typeIndex][i] == asMod || d.freqAddMods[typeIndex][i] == asMod)
					{
						auto isScaled = d.freqValueModes[typeIndex][i] == ValueMode::Scale;
						auto shouldBeScaled = newMode == ValueMode::Scale;

						if (isScaled != shouldBeScaled)
						{
							auto other = dynamic_cast<GlobalModulator*>(shouldBeScaled ?
								d.modulators[typeIndex][i].get() :
								d.freqAddMods[typeIndex][i].get());

							jassert(other != gm);

							gm->disconnect();
							dynamic_cast<Modulator*>(gm)->setBypassed(true, sendNotificationAsync);
							gm = other;

							gm->connectToGlobalModulator(d.parent->container->getId() + ":" + sm->getId());
							dynamic_cast<Modulator*>(gm)->setBypassed(false, sendNotificationAsync);
						}

						d.freqValueModes[typeIndex].set(i, newMode);

						if (!shouldBeScaled)
							dynamic_cast<Modulation*>(gm)->setIsBipolar(newMode == ValueMode::Bipolar);
					}
				}

				return true;
			}
		}
		else
		{
			if (gm->getOriginalModulator() == sm)
			{
				auto asMod = dynamic_cast<Modulation*>(gm);

				if (asMod->getMode() == Modulation::Mode::GainMode)
					return false;

				if (asMod->isBipolar() == (newMode == ValueMode::Bipolar))
					return false;

				asMod->setIsBipolar(newMode == ValueMode::Bipolar);
				return true;
			}
		}
		
		return false;
	});
}

bool ScriptModulationMatrix::ModulatorTargetData::updateIntensity(const String& sourceId, float newValue)
{
	return forEach(parent->getSourceMod(sourceId), [newValue](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		if (gm->getOriginalModulator() == sm)
		{
			auto vToUse = newValue;

			if (d.isBipolarFreqMod(gm)) // uses PAN range (wtf lol)
				vToUse *= 100.0;

			dynamic_cast<Modulation*>(gm)->setIntensityFromSlider(vToUse);
			return true;
		}
		
		return false;
	});
}

bool ScriptModulationMatrix::ModulatorTargetData::checkActiveConnections(const String& sourceId)
{
	auto sm = parent->getSourceMod(sourceId);

	bool tfound = false;

	if (targetMode == TargetMode::FrequencyMode)
	{
		auto f = [&](int typeIndex)
		{
			int idx = 0;

			for (auto& m : freqValueModes[typeIndex])
			{
				auto list = m == ValueMode::Scale ? &freqAddMods : &modulators;
				(*list)[typeIndex][idx++]->setBypassed(true, sendNotificationAsync);
			}
		};
		
		f(0);
		f(1);
		f(2);
	}

	forEach(sm, [&tfound](Modulator* sm, ModulatorTargetData& d, GlobalModulator* gm)
	{
		auto asMod = dynamic_cast<Modulator*>(gm);

		auto synth = asMod->getParentProcessor(true);
		auto gainChain = synth->getChildProcessor(ModulatorSynth::GainModulation);
		auto isGainMod = asMod->getParentProcessor(false) == gainChain;
		auto isFirstEnvelope = d.modulators[2][0].get() == asMod;

		// let the first gain envelope active to let the voice play
		auto leaveAlwaysEnabled = (isGainMod && isFirstEnvelope);

		if (!leaveAlwaysEnabled)
		{
			asMod->setBypassed(!gm->isConnected(), sendNotificationAsync);
		}

		tfound |= gm->getOriginalModulator() == sm;

		return false;
	});

	return tfound;
}



template <typename ModulatorType> Modulator* createOrGet(Processor* p, int subIndex, const String& id)
{
	if (auto existingMod = ProcessorHelpers::getFirstProcessorWithName(p, id))
		return dynamic_cast<Modulator*>(existingMod);

	raw::Builder b(p->getMainController());
	auto newMod = b.create<ModulatorType>(p, subIndex);
	newMod->setId(id);
	return newMod;
}

void ScriptModulationMatrix::ModulatorTargetData::init(const var& json)
{
	TargetDataBase::init(json);

	auto p = parent.get();

	verifyProperty(json, MatrixIds::Chain);

	subIndex = (int)json.getProperty(MatrixIds::Chain, -1);

	auto modChain = dynamic_cast<ModulatorChain*>(processor->getChildProcessor(subIndex));

	verifyExists(modChain, MatrixIds::Chain);

	targetMode = (TargetMode)(int)modChain->getMode();

	if (dynamic_cast<FilterEffect*>(processor.get()) != nullptr && subIndex == PolyFilterEffect::FrequencyChain)
		targetMode = TargetMode::FrequencyMode;

	switch (targetMode)
	{
	case TargetMode::GainMode:
	case TargetMode::FrequencyMode:
	case TargetMode::GlobalMode: defaultValue = 1.0f; break;
	case TargetMode::PitchMode:
	case TargetMode::PanMode: defaultValue = 0.0f; break;
	default:
		jassertfalse;
	}


	int thisSlots[3] = { p->numSlots[0], p->numSlots[1], p->numSlots[2] };


	auto tsa = json.getProperty("Slots", 0);

	if (tsa.isArray())
	{
		thisSlots[0] = (int)tsa[0];
		thisSlots[1] = (int)tsa[1];
		thisSlots[2] = (int)tsa[2];
	}

	if (json.getProperty(MatrixIds::AddConstUIMod, false))
	{
		componentMod = createOrGet<ConstantModulator>(processor, subIndex, modId + " UIMod");
	}
	
	for (int i = 0; i < thisSlots[0]; i++)
		modulators[0].add(createOrGet<GlobalVoiceStartModulator>(processor, subIndex, modId + " VS" + String(i)));

	for (int i = 0; i < thisSlots[1]; i++)
		modulators[1].add(createOrGet<GlobalTimeVariantModulator>(processor, subIndex, modId + " TV" + String(i)));

	for (int i = 0; i < thisSlots[2]; i++)
		modulators[2].add(createOrGet<GlobalEnvelopeModulator>(processor, subIndex, modId + " EN" + String(i)));

	if (targetMode == TargetMode::FrequencyMode)
	{
		auto defaultMode = ValueModeHelpers::getMode(json[MatrixIds::Mode]);

		// make sure the bipolar mod is cranked up fully.
		processor->setAttribute(PolyFilterEffect::Parameters::BipolarIntensity, 1.0, sendNotification);

		if (defaultMode == ValueMode::Undefined)
			defaultMode = ValueMode::Scale;

		for (int i = 0; i < thisSlots[0]; i++)
		{
			freqValueModes[0].add(defaultMode);
			freqAddMods[0].add(createOrGet<GlobalVoiceStartModulator>(processor, PolyFilterEffect::BipolarFrequencyChain, modId + " VSAdd" + String(i)));
		}
			

		for (int i = 0; i < thisSlots[1]; i++)
		{
			freqValueModes[1].add(defaultMode);
			freqAddMods[1].add(createOrGet<GlobalTimeVariantModulator>(processor, PolyFilterEffect::BipolarFrequencyChain, modId + " TVAdd" + String(i)));
		}
			
		for (int i = 0; i < thisSlots[2]; i++)
		{
			freqValueModes[2].add(defaultMode);
			freqAddMods[2].add(createOrGet<GlobalEnvelopeModulator>(processor, PolyFilterEffect::BipolarFrequencyChain, modId + " ENAdd" + String(i)));
		}
	}

	dynamic_cast<ScriptComponent*>(sc.getObject())->setModulationData(getModulationData());

	auto synthChain = p->getMainController()->getMainSynthChain();

	synthChain->sendRebuildMessage(true);
}

bool ScriptModulationMatrix::ModulatorTargetData::queryFunction(int index, bool checkTicked) const
{
	if (checkTicked)
	{
		auto sm = parent->sourceData[index]->mod.get();

		return forEach(sm, [index](Modulator* sm, ModulatorTargetData& data, GlobalModulator* gm)
		{
			return gm->getOriginalModulator() == sm;
		});
	}
	else
	{
		auto sm = parent->sourceData[index]->mod.get();

		return forEach(sm, [](Modulator*, ModulatorTargetData& d, GlobalModulator* gm)
		{
			return !gm->isConnected();
		});
	}
}

bool ScriptModulationMatrix::ModulatorTargetData::forEach(Modulator* sourceMod, const IteratorFunction& f) const
{
	auto& thisRef = *const_cast<ModulatorTargetData*>(this);

 	std::function<bool(int)> f2 = [&](int typeIndex)
	{
		for (auto m : modulators[typeIndex])
		{
			if (f(sourceMod, thisRef, dynamic_cast<GlobalModulator*>(m.get())))
				return true;
		}

		return false;
	};;

	if (targetMode == TargetMode::FrequencyMode)
	{
		f2 = [&](int typeIndex)
		{
			int numToCheck = freqValueModes[typeIndex].size();

			for (int i = 0; i < numToCheck; i++)
			{
				auto isScale = freqValueModes[typeIndex][i] == ValueMode::Scale;
				auto m = isScale ? modulators[typeIndex][i] : freqAddMods[typeIndex][i];

				if (f(sourceMod, thisRef, dynamic_cast<GlobalModulator*>(m.get())))
					return true;
			}

			return false;
		};
	}

	if (sourceMod == nullptr || dynamic_cast<VoiceStartModulator*>(sourceMod) != nullptr)
	{
		if (f2(0))
			return true;
	}
	if (sourceMod == nullptr || dynamic_cast<TimeVariantModulator*>(sourceMod) != nullptr)
	{
		if (f2(1))
			return true;
	}
	if (sourceMod == nullptr || dynamic_cast<EnvelopeModulator*>(sourceMod) != nullptr)
	{
		if (f2(2))
			return true;
	}

	return false;
}

float ScriptModulationMatrix::ModulatorTargetData::getModValue() const
{
	auto dv = processor->getChildProcessor(subIndex)->getDisplayValues().outL;

	switch (targetMode)
	{
	case TargetMode::PitchMode:
	{
		if (dv == 0.0)
			return r.convertTo0to1(componentValue, false);

		auto value = scriptnode::conversion_logic::pitch2st().getValue(dv);

		auto copy = r;
		copy.rng.interval = 0.0;

		auto mValue = copy.convertTo0to1(value, false);

		return (float)jlimit(0.0, 1.0, mValue);
	}
	case TargetMode::PanMode:
	{
		auto somethingActive = dynamic_cast<ModulatorSynth*>(processor->getParentProcessor(true))->getNumActiveVoices() != 0;
		
		if (somethingActive)
			return jlimit(0.0f, 1.0f, (float)dv);
		else
			return r.convertTo0to1(componentValue, false);
	}
	case TargetMode::FrequencyMode:
	{
		auto c = r.convertTo0to1(componentValue, false);

		auto gainMod = dv;

		auto bimod = processor->getChildProcessor(PolyFilterEffect::BipolarFrequencyChain);

		auto bipolarMod = bimod->getDisplayValues().outL;

		if (!dynamic_cast<ModulatorChain*>(bimod)->shouldBeProcessedAtAll())
			bipolarMod = 0.0;

		return jlimit(0.0, 1.0, (c + bipolarMod) * gainMod);
	}
	default:
	{
		auto normValue = (float)dynamic_cast<ScriptComponent*>(sc.getObject())->getValueNormalized();
		dynamic_cast<Modulation*>(processor->getChildProcessor(subIndex))->applyModulationValue(dv, normValue);
		return normValue;
	}
	}
}

void ScriptModulationMatrix::ParameterTargetData::updateConnectionData(const var& obj)
{
	auto cable = parent->getSourceCable(obj["Source"]);

	forEach(cable, [&obj](ReferenceCountedObject* cable, ParameterTargetData& d, ParameterTargetCable* target)
	{
		if (static_cast<RoutingManager::Cable*>(cable)->containsTarget(target))
		{
			target->intensity = ValueModeHelpers::getIntensityValue(obj);
			target->inverted = obj.getProperty(MatrixIds::Inverted, false);
			target->customMode = ValueModeHelpers::getMode(obj.getProperty(MatrixIds::Mode, "Default"));

			d.updateValue();
			return true;
		}

		return false;
	});
}

juce::var ScriptModulationMatrix::ParameterTargetData::getConnectionData() const
{
	Array<var> data;

	forEach(nullptr, [&data](ReferenceCountedObject*, ParameterTargetData& d, ParameterTargetCable* target)
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty(MatrixIds::Source, target->sourceId);
		obj->setProperty(MatrixIds::Target, d.modId);
		obj->setProperty(MatrixIds::Intensity, target->intensity);
		obj->setProperty(MatrixIds::Inverted, target->inverted);
		obj->setProperty(MatrixIds::Mode, ValueModeHelpers::getModeName(ValueModeHelpers::getModeToUse(d.valueMode, target->customMode)));

		data.add(var(obj.get()));

		return false;
	});

	return var(data);
}

void ScriptModulationMatrix::ParameterTargetData::updateValue()
{
	auto sv = (double)componentValue;

	for (auto& s : parameterTargets)
	{
		auto pt = static_cast<ParameterTargetCable*>(s.getObject());
		auto v = (1.0 - pt->value) * (double)pt->inverted + pt->value * (1.0 - (double)pt->inverted);

		auto intensity = pt->intensity;

		auto modeToUse = ValueModeHelpers::getModeToUse(valueMode, pt->customMode);

		switch (modeToUse)
		{
		case ValueMode::Scale:
			sv *= (1.0 - intensity + intensity * v);
			break;
		case ValueMode::Unipolar:
			sv = jlimit(0.0, 1.0, sv + intensity * v);
			break;
		case ValueMode::Bipolar:
			sv = jlimit(0.0, 1.0, sv + 2.0 * intensity * (v - 0.5));
			break;
		default:
			jassertfalse;
			break;
		}
	}

	auto valueToSend = r.convertFrom0to1(sv, true);

	if (valueToSend != lastParameterValue)
	{
		lastParameterValue = valueToSend;
		processor->setAttribute(subIndex, valueToSend, sendNotification);
		dynamic_cast<ScriptComponent*>(sc.getObject())->sendRepaintMessage();
	}
}

void ScriptModulationMatrix::ParameterTargetData::clear()
{
	parameterTargets.clear();
	dynamic_cast<ScriptComponent*>(sc.getObject())->sendRepaintMessage();
}



bool ScriptModulationMatrix::ParameterTargetData::connect(const String& sourceId, bool addConnection)
{
	auto cable = parent->getSourceCable(sourceId);

	if (addConnection)
	{
		auto ok = forEach(cable, [sourceId](ReferenceCountedObject*, TargetDataBase& d, ParameterTargetCable* target)
		{
			return target->sourceId == sourceId;
		});

		if (ok)
			return true;

		auto parameterTarget = var(new ParameterTargetCable(dynamic_cast<ParameterTargetData*>(this), sourceId));
		static_cast<RoutingManager::Cable*>(cable)->addTarget(dynamic_cast<RoutingManager::CableTargetBase*>(parameterTarget.getObject()));

		parameterTargets.add(parameterTarget);
		parent->sendUpdateMessage(sourceId, modId, ConnectionEvent::Add);
	}
	else
	{
		forEach(cable, [sourceId](ReferenceCountedObject* cable, ParameterTargetData& d, ParameterTargetCable* c)
		{
			auto typed = static_cast<RoutingManager::Cable*>(cable);

			if (typed->containsTarget(c))
			{
				typed->removeTarget(c);
				d.parameterTargets.remove(var(c));
				dynamic_cast<ScriptComponent*>(d.sc.getObject())->sendRepaintMessage();
				d.parent->sendUpdateMessage(sourceId, d.modId, ConnectionEvent::Delete);
				return true;
			}

			return false;
		});
	}

	updateValue();
	return true;
}

bool ScriptModulationMatrix::ParameterTargetData::checkActiveConnections(const String& sourceId)
{
	auto cable = parent->getSourceCable(sourceId);

	return forEach(cable, [](ReferenceCountedObject* cable, TargetDataBase& d, ParameterTargetCable* target)
	{
		return static_cast<RoutingManager::Cable*>(cable)->containsTarget(target);
	});
}

bool ScriptModulationMatrix::ParameterTargetData::updateIntensity(const String& sourceId, float newValue)
{
	return forEach(parent->getSourceCable(sourceId), [newValue](ReferenceCountedObject* cable, ParameterTargetData& d, ParameterTargetCable* target)
	{
		if (static_cast<RoutingManager::Cable*>(cable)->containsTarget(target))
		{
			target->intensity = newValue;
			d.updateValue();
			return true;
		}

		return false;
	});
}


bool ScriptModulationMatrix::ParameterTargetData::updateValueMode(const String& sourceId, ValueMode newMode)
{
	return forEach(parent->getSourceCable(sourceId), [newMode](ReferenceCountedObject* cable, ParameterTargetData& d, ParameterTargetCable* target)
	{
		if (static_cast<RoutingManager::Cable*>(cable)->containsTarget(target))
		{
			if (target->customMode != newMode)
			{
				target->customMode = newMode;
				d.updateValue();
				return true;
			}
			
			return false;
		}

		return false;
	});
}

bool ScriptModulationMatrix::ParameterTargetData::canConnect(const String& sourceId) const
{
	auto sourceCable = parent->getSourceCable(sourceId);

	return !forEach(sourceCable, [](ReferenceCountedObject* cable, ParameterTargetData& data, ParameterTargetCable* pc)
	{
		return static_cast<RoutingManager::Cable*>(cable)->containsTarget(pc);
	});
}

juce::var ScriptModulationMatrix::ParameterTargetData::getIntensitySliderData(String sourceId) const
{
	auto sourceCable = parent->getSourceCable(sourceId);

	var obj;
	
	forEach(sourceCable, [&obj](ReferenceCountedObject* cable, ParameterTargetData& data, ParameterTargetCable* pc)
	{
		if (static_cast<RoutingManager::Cable*>(cable)->containsTarget(pc))
		{
			scriptnode::InvertableParameterRange rng(0.0, 1.0);

			auto m = ValueModeHelpers::getModeToUse(data.valueMode, pc->customMode);

			if (m == ValueMode::Bipolar || m == ValueMode::Unipolar)
				rng.rng.start = -1.0;

			scriptnode::RangeHelpers::storeDoubleRange(obj, rng);

			obj.getDynamicObject()->setProperty(scriptnode::PropertyIds::Value, pc->intensity);
			return true;
		}

		return false;
	});

	if (!obj.isObject())
	{
		scriptnode::InvertableParameterRange rng(0.0, 1.0);

		if (valueMode == ValueMode::Bipolar)
			rng.rng.start = -1.0;

		scriptnode::RangeHelpers::storeDoubleRange(obj, rng);

		obj.getDynamicObject()->setProperty(scriptnode::PropertyIds::Value, 0.0);
	}

	obj.getDynamicObject()->setProperty("defaultValue", 0.0);

	return obj;
}

juce::var ScriptModulationMatrix::ParameterTargetData::getValueModeData(const String& sourceId) const
{
	Array<var> modes;
	modes.add(ValueModeHelpers::getModeName(ValueMode::Default));
	modes.add(ValueModeHelpers::getModeName(ValueMode::Scale));
	modes.add(ValueModeHelpers::getModeName(ValueMode::Unipolar));
	modes.add(ValueModeHelpers::getModeName(ValueMode::Bipolar));
	
	DynamicObject::Ptr obj = new DynamicObject();

	obj->setProperty(MatrixIds::items, var(modes));

	forEach(parent->getSourceCable(sourceId), [&](ReferenceCountedObject* cable, ParameterTargetData& d, ParameterTargetCable* target)
	{
		if (static_cast<RoutingManager::Cable*>(cable)->containsTarget(target))
		{
			obj->setProperty(scriptnode::PropertyIds::Value, ValueModeHelpers::getModeName(target->customMode));
			return true;
		}

		return false;
	});

	return var(obj.get());
}

void ScriptModulationMatrix::ParameterTargetData::init(const var& json)
{
	TargetDataBase::init(json);

	verifyProperty(json, MatrixIds::Parameter);

	auto param = json["Parameter"];

	if (param.isString())
		subIndex = processor->getParameterIndexForIdentifier(param.toString());
	else
		subIndex = (int)subIndex;

	if (subIndex == -1)
		verifyExists(nullptr, MatrixIds::Parameter);

	verifyProperty(json, MatrixIds::Mode);

	valueMode = ValueModeHelpers::getMode(json[MatrixIds::Mode]);

	if (valueMode == ValueMode::Undefined)
		verifyExists(nullptr, MatrixIds::Mode);

	dynamic_cast<ScriptComponent*>(sc.getObject())->setModulationData(getModulationData());
}

hise::MacroControlledObject::ModulationPopupData::Ptr ScriptModulationMatrix::ParameterTargetData::getModulationData()
{
	auto newData = TargetDataBase::getModulationData();

	WeakReference<ParameterTargetData> safeThis(this);

	newData->valueCallback = [safeThis](double v)
	{
		if (safeThis != nullptr)
		{
			safeThis->componentValue = safeThis->r.convertTo0to1(v, true);
			safeThis->updateValue();
		}
	};

	return newData;
}

bool ScriptModulationMatrix::ParameterTargetData::queryFunction(int index, bool checkTicked) const
{
	if (checkTicked)
	{
		auto cable = parent->sourceData[index]->cable.get();

		return forEach(cable, [](ReferenceCountedObject* cable, ParameterTargetData& d, ParameterTargetCable* target)
		{
			return static_cast<RoutingManager::Cable*>(cable)->containsTarget(target);
		});
	}
	else
	{
		return true;
	}
}

bool ScriptModulationMatrix::ParameterTargetData::forEach(ReferenceCountedObject* cable, const IteratorFunction& f) const
{
	jassert(!isMod);

	auto& thisRef = *const_cast<ParameterTargetData*>(this);

	for (auto& pt : parameterTargets)
	{
		auto target = static_cast<ParameterTargetCable*>(pt.getObject());

		if (f(cable, thisRef, target))
			return true;
	}

	return false;
}

float ScriptModulationMatrix::ParameterTargetData::getModValue() const
{
	auto realValue = processor->getAttribute(subIndex);
	return (float)r.convertTo0to1((double)realValue, true);
}


bool ScriptModulationMatrix::MatrixUndoAction::perform()
{
	if (matrix == nullptr)
		return false;

	switch (type)
	{
	case ActionType::Clear:
		matrix->clearConnectionsInternal();
		break;
	case ActionType::Add:
		return matrix->connectInternal(source, target, true);
	case ActionType::Remove:
		return matrix->connectInternal(source, target, false);
	case ActionType::Intensity:
		return matrix->updateIntensityInternal(source, target, newValue);
	case ActionType::ValueMode:
		return matrix->updateValueModeInternal(source, target, newValue);
	case ActionType::Update:
		matrix->updateConnectionDataInternal(newValue);
		return true;
	default:
		break;
	}

	return true;
}

bool ScriptModulationMatrix::MatrixUndoAction::undo()
{
	if (matrix == nullptr)
		return false;

	switch (type)
	{
	case ActionType::Clear:
	case ActionType::Update:
		matrix->fromBase64(oldValue.toString());
		break;
	case ActionType::Add:
		return matrix->connectInternal(source, target, false);
	case ActionType::Remove:
		return matrix->connectInternal(source, target, true);
	case ActionType::Intensity:
		return matrix->updateIntensityInternal(source, target, oldValue);
	case ActionType::ValueMode:
		return matrix->updateValueModeInternal(source, target, oldValue);
	default:
		break;
	}

	return true;
}

}

} 
