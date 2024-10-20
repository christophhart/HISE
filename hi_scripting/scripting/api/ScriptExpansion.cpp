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

namespace hise {
using namespace juce;

struct ScriptUserPresetHandler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptUserPresetHandler, setPreCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptUserPresetHandler, setPostCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptUserPresetHandler, setPostSaveCallback);
	API_VOID_METHOD_WRAPPER_2(ScriptUserPresetHandler, setEnableUserPresetPreprocessing);
	API_VOID_METHOD_WRAPPER_1(ScriptUserPresetHandler, setCustomAutomation);
	API_VOID_METHOD_WRAPPER_3(ScriptUserPresetHandler, setUseCustomUserPresetModel);
	API_METHOD_WRAPPER_1(ScriptUserPresetHandler, isOldVersion);
    API_METHOD_WRAPPER_0(ScriptUserPresetHandler, isInternalPresetLoad);
	API_METHOD_WRAPPER_0(ScriptUserPresetHandler, isCurrentlyLoadingPreset);
	API_VOID_METHOD_WRAPPER_0(ScriptUserPresetHandler, clearAttachedCallbacks);
	API_VOID_METHOD_WRAPPER_3(ScriptUserPresetHandler, attachAutomationCallback);
	API_VOID_METHOD_WRAPPER_3(ScriptUserPresetHandler, updateAutomationValues);
	API_METHOD_WRAPPER_1(ScriptUserPresetHandler, getAutomationIndex);
	API_METHOD_WRAPPER_2(ScriptUserPresetHandler, setAutomationValue);
	API_METHOD_WRAPPER_0(ScriptUserPresetHandler, getSecondsSinceLastPresetLoad);
	API_VOID_METHOD_WRAPPER_1(ScriptUserPresetHandler, updateSaveInPresetComponents);
	API_VOID_METHOD_WRAPPER_0(ScriptUserPresetHandler, updateConnectedComponentsFromModuleState);
	API_VOID_METHOD_WRAPPER_1(ScriptUserPresetHandler, setUseUndoForPresetLoading);
	API_METHOD_WRAPPER_0(ScriptUserPresetHandler, createObjectForSaveInPresetComponents);
	API_VOID_METHOD_WRAPPER_0(ScriptUserPresetHandler, resetToDefaultUserPreset);
	API_METHOD_WRAPPER_0(ScriptUserPresetHandler, createObjectForAutomationValues);
	API_VOID_METHOD_WRAPPER_0(ScriptUserPresetHandler, runTest);
};

ScriptUserPresetHandler::ScriptUserPresetHandler(ProcessorWithScriptingContent* pwsc) :
	ConstScriptingObject(pwsc, 0),
	ControlledObject(pwsc->getMainController_()),
	preCallback(pwsc, nullptr, var(), 1),
	postCallback(pwsc, nullptr, var(), 1),
	postSaveCallback(pwsc, nullptr, var(), 1),
	customLoadCallback(pwsc, nullptr, var(), 1),
	customSaveCallback(pwsc, nullptr, var(), 1)
{
	getMainController()->getUserPresetHandler().addListener(this);

	ADD_API_METHOD_1(isOldVersion);
    ADD_API_METHOD_0(isInternalPresetLoad);
	ADD_API_METHOD_0(isCurrentlyLoadingPreset);
	ADD_API_METHOD_1(setPostCallback);
	ADD_API_METHOD_1(setPostSaveCallback);
	ADD_API_METHOD_1(setPreCallback);
	ADD_API_METHOD_2(setEnableUserPresetPreprocessing);
	ADD_API_METHOD_1(setCustomAutomation);
	ADD_API_METHOD_3(setUseCustomUserPresetModel);
	ADD_API_METHOD_3(attachAutomationCallback);
	ADD_API_METHOD_0(clearAttachedCallbacks);
	ADD_API_METHOD_1(getAutomationIndex);
	ADD_API_METHOD_2(setAutomationValue);
	ADD_API_METHOD_3(updateAutomationValues);
	ADD_API_METHOD_1(updateSaveInPresetComponents);
	ADD_API_METHOD_0(updateConnectedComponentsFromModuleState);
	ADD_API_METHOD_1(setUseUndoForPresetLoading);
	ADD_API_METHOD_0(createObjectForSaveInPresetComponents);
	ADD_API_METHOD_0(createObjectForAutomationValues);
	ADD_API_METHOD_0(getSecondsSinceLastPresetLoad);
	ADD_API_METHOD_0(resetToDefaultUserPreset);
	ADD_API_METHOD_0(runTest);
	
}

ScriptUserPresetHandler::~ScriptUserPresetHandler()
{
	clearAttachedCallbacks();
	

	getMainController()->getUserPresetHandler().removeListener(this);
}

Identifier ScriptUserPresetHandler::getObjectName() const
{ RETURN_STATIC_IDENTIFIER("UserPresetHandler"); }

int ScriptUserPresetHandler::getNumChildElements() const
{
	return 2;
}

DebugInformationBase* ScriptUserPresetHandler::getChildElement(int index)
{
	if (index == 0)
		return preCallback.createDebugObject("preCallback");
	if (index == 1)
		return postCallback.createDebugObject("postCallback");
        
	return nullptr;
}

void ScriptUserPresetHandler::presetListUpdated()
{

}

void ScriptUserPresetHandler::loadCustomUserPreset(const var& dataObject)
{
	if (customLoadCallback)
	{
		LockHelpers::SafeLock sl(getScriptProcessor()->getMainController_(), LockHelpers::Type::ScriptLock);

		var args = dataObject;
		auto ok = customLoadCallback.callSync(&args, 1, nullptr);

		if (!ok.wasOk())
			debugError(getMainController()->getMainSynthChain(), ok.getErrorMessage());
	}
}

var ScriptUserPresetHandler::saveCustomUserPreset(const String& presetName)
{
	if (customSaveCallback)
	{
		LockHelpers::SafeLock sl(getScriptProcessor()->getMainController_(), LockHelpers::Type::ScriptLock);

		var rv;
		var args = presetName;
		auto ok = customSaveCallback.callSync(&args, 1, &rv);

		if (!ok.wasOk())
			debugError(getMainController()->getMainSynthChain(), ok.getErrorMessage());

		return rv;
	}

	return {};
}

void ScriptUserPresetHandler::setUseUndoForPresetLoading(bool shouldUseUndoManager)
{
	getMainController()->getUserPresetHandler().setAllowUndoAtUserPresetLoad(shouldUseUndoManager);
}

void ScriptUserPresetHandler::setPreCallback(var presetCallback)
{
	preCallback = WeakCallbackHolder(getScriptProcessor(), this, presetCallback, 1);
	preCallback.incRefCount();
	preCallback.addAsSource(this, "preCallback");
	preCallback.setThisObject(this);
}

void ScriptUserPresetHandler::setPostCallback(var presetPostCallback)
{
	postCallback = WeakCallbackHolder(getScriptProcessor(), this, presetPostCallback, 1);
	postCallback.incRefCount();
	postCallback.addAsSource(this, "postCallback");
	postCallback.setThisObject(this);

}

void ScriptUserPresetHandler::setPostSaveCallback(var presetPostSaveCallback)
{
	postSaveCallback = WeakCallbackHolder(getScriptProcessor(), this, presetPostSaveCallback, 1);
	postSaveCallback.incRefCount();
	postSaveCallback.addAsSource(this, "postCallback");
	postSaveCallback.setThisObject(this);
}

void ScriptUserPresetHandler::setEnableUserPresetPreprocessing(bool processBeforeLoading, bool shouldUnpackComplexData)
{
	enablePreprocessing = processBeforeLoading;
	unpackComplexData = shouldUnpackComplexData;
}

bool ScriptUserPresetHandler::isInternalPresetLoad() const
{
    auto& uph = getScriptProcessor()->getMainController_()->getUserPresetHandler();
    
    return uph.isInternalPresetLoad();
}

bool ScriptUserPresetHandler::isCurrentlyLoadingPreset() const
{
	auto& uph = getScriptProcessor()->getMainController_()->getUserPresetHandler();

	return uph.isCurrentlyInsidePresetLoad();
}

bool ScriptUserPresetHandler::isOldVersion(const String& version)
{
#if USE_BACKEND
	auto thisVersion = dynamic_cast<GlobalSettingManager*>(getProcessor()->getMainController())->getSettingsObject().getSetting(HiseSettings::Project::Version);
#else
	auto thisVersion = FrontendHandler::getVersionString();
#endif

	SemanticVersionChecker svs(version, thisVersion);

	return svs.isUpdate();
}



void ScriptUserPresetHandler::setUseCustomUserPresetModel(var loadCallback, var saveCallback, bool usePersistentObject)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadCallback) && HiseJavascriptEngine::isJavascriptFunction(saveCallback))
	{
		customLoadCallback = WeakCallbackHolder(getScriptProcessor(), this, loadCallback, 1);
		customLoadCallback.incRefCount();
		customLoadCallback.addAsSource(this, "customLoadCallback");
		
		customSaveCallback = WeakCallbackHolder(getScriptProcessor(), this, saveCallback, 1);
		customSaveCallback.incRefCount();
		customSaveCallback.addAsSource(this, "customSaveCallback");

		getMainController()->getUserPresetHandler().setUseCustomDataModel(true, usePersistentObject);
	}
}



void ScriptUserPresetHandler::setCustomAutomation(var automationData)
{
	if (automationData.isArray())
	{
		using CustomData = MainController::UserPresetHandler::CustomAutomationData;

		CustomData::List newList;

		int index = 0;

		if (auto ar = automationData.getArray())
		{
			for (const auto& ad : *ar)
			{
				auto nd = new CustomData(newList, getScriptProcessor()->getMainController_(), index++, ad);

				if (!nd->r.wasOk())
					reportScriptError(nd->id + " - " + nd->r.getErrorMessage());

				newList.add(nd);
			}
		}

		if (!getMainController()->getUserPresetHandler().setCustomAutomationData(newList))
		{
			reportScriptError("you need to enable setUseCustomDataModel() before calling this method");
		}
	}
}

ScriptUserPresetHandler::AttachedCallback::AttachedCallback(ScriptUserPresetHandler* parent, MainController::UserPresetHandler::CustomAutomationData::Ptr cData_, var f, dispatch::DispatchType n_) :
  cData(cData_),
  n(n_),
  NEW_AUTOMATION_WITH_COMMA(listener(parent->getMainController()->getRootDispatcher(), *this, BIND_MEMBER_FUNCTION_2(AttachedCallback::onUpdate)))
  customAsyncUpdateCallback(parent->getScriptProcessor(), nullptr, var(), 2),
  customUpdateCallback(parent->getScriptProcessor(), nullptr, var(), 2)
{
#if USE_OLD_AUTOMATION_DISPATCH
	auto isSynchronous = n == dispatch::DispatchType::sendNotificationSync;
	if (isSynchronous)
	{
		customUpdateCallback = WeakCallbackHolder(parent->getScriptProcessor(), parent, f, 2);
		cData->syncListeners.addListener(*this, AttachedCallback::onCallbackSync, false);
	}
	else
	{
		customAsyncUpdateCallback = WeakCallbackHolder(parent->getScriptProcessor(), parent, f, 2);
		cData->asyncListeners.addListener(*this, AttachedCallback::onCallbackAsync, false);
	}
#endif
	
#if USE_NEW_AUTOMATION_DISPATCH
	if(n == dispatch::DispatchType::sendNotificationAsync)
		customAsyncUpdateCallback = WeakCallbackHolder(parent->getScriptProcessor(), parent, f, 2);
	else
		customUpdateCallback = WeakCallbackHolder(parent->getScriptProcessor(), parent, f, 2);

	cData->dispatcher.addValueListener(&listener, false, n);
#endif


}

ScriptUserPresetHandler::AttachedCallback::~AttachedCallback()
{
#if USE_OLD_AUTOMATION_DISPATCH
	if (customUpdateCallback)
		cData->syncListeners.removeListener(*this);

	if (customAsyncUpdateCallback)
		cData->asyncListeners.removeListener(*this);
#endif

	IF_NEW_AUTOMATION_DISPATCH(cData->dispatcher.removeValueListener(&listener));
	
	cData = nullptr;
}

void ScriptUserPresetHandler::AttachedCallback::onCallbackSync(AttachedCallback& c, var* args)
{
	if (c.customUpdateCallback)
		c.customUpdateCallback.callSync(args, 2, nullptr);
}

void ScriptUserPresetHandler::AttachedCallback::onCallbackAsync(AttachedCallback& c, int index, float newValue)
{
	if (c.customAsyncUpdateCallback)
	{
		var args[2];
		args[0] = index;
		args[1] = newValue;
		c.customAsyncUpdateCallback.call(args, 2);
	}
}

void ScriptUserPresetHandler::AttachedCallback::onUpdate(int index, float value)
{
	if(n == dispatch::DispatchType::sendNotificationAsync)
	{
		onCallbackAsync(*this, index, value);
	}
	else
	{
		juce::var data[2] = {juce::var(index), var(value) };
		onCallbackSync(*this, data);
	}
}

void ScriptUserPresetHandler::attachAutomationCallback(String automationId, var updateCallback, var isSynchronous)
{
	auto n = ApiHelpers::getDispatchType(isSynchronous, false);

	if (auto cData = getMainController()->getUserPresetHandler().getCustomAutomationData(Identifier(automationId)))
	{
		for (auto& c : attachedCallbacks)
		{
			if (automationId == c->cData->id)
			{
				attachedCallbacks.removeObject(c);
				debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "removing old attached callback for " + automationId);
				break;
			}
		}

		if (HiseJavascriptEngine::isJavascriptFunction(updateCallback))
		{
			attachedCallbacks.add(new AttachedCallback(this, cData, updateCallback, n));
		}

		return;
	}
	else
	{
		reportScriptError(automationId + " not found");
	}
}

void ScriptUserPresetHandler::clearAttachedCallbacks()
{
	attachedCallbacks.clear();
}

struct AutomationValueUndoAction: public UndoableAction
{
    AutomationValueUndoAction(ScriptUserPresetHandler* s, var newData_, dispatch::DispatchType n_):
      newData(newData_),
      n(n_),
      suph(s)
    {
        auto& h = suph->getMainController()->getUserPresetHandler();
        
        if(auto obj = newData.getDynamicObject())
        {
            auto od = new DynamicObject();
            
            for(auto& nv: obj->getProperties())
            {
                if(auto a = h.getCustomAutomationData(Identifier(nv.name)))
                {
                    od->setProperty(nv.name, a->lastValue);
                }
            }
            
            oldData = var(od);
        }
    }
    
    bool undo() override
    {
        if(suph != nullptr)
        {
            suph->updateAutomationValues(oldData, ApiHelpers::getDispatchTypeMagicNumber(n), false);
            return true;
        }
        
        return false;
    }
    
    bool perform() override
    {
        if(suph != nullptr)
        {
            suph->updateAutomationValues(newData, ApiHelpers::getDispatchTypeMagicNumber(n), false);
            return true;
        }
        
        return false;
    }
    
    var oldData;
    var newData;
    dispatch::DispatchType n;
    WeakReference<ScriptUserPresetHandler> suph;
};

int ScriptUserPresetHandler::getAutomationIndex(String automationID)
{
	auto& uph = getMainController()->getUserPresetHandler();

	if (uph.isUsingCustomDataModel())
	{
		for (int i = 0; i < uph.getNumCustomAutomationData(); i++)
		{
			if (uph.getCustomAutomationData(i)->id == automationID)
				return i;
		}
	}

	return -1;
}

bool ScriptUserPresetHandler::setAutomationValue(int automationIndex, float newValue)
{
	auto& uph = getMainController()->getUserPresetHandler();

	if (uph.isUsingCustomDataModel() && isPositiveAndBelow(automationIndex, uph.getNumCustomAutomationData()))
	{
		uph.getCustomAutomationData(automationIndex)->call(newValue, dispatch::DispatchType::sendNotificationSync);
		return true;
	}

	return false;
}

void ScriptUserPresetHandler::updateAutomationValues(var data, var sendMessage, bool useUndoManager)
{
	auto n = ApiHelpers::getDispatchType(sendMessage, true);

	auto& uph = getMainController()->getUserPresetHandler();

	if (data.isInt() || data.isInt64())
	{
		auto preferredProcessorIndex = (int)data;

		// just refresh the values from the current processor states
		for (int i = 0; i < uph.getNumCustomAutomationData(); i++)
		{
			uph.getCustomAutomationData(i)->updateFromConnectionValue(preferredProcessorIndex);
		}

		return;
	}

    if(!useUndoManager)
    {
		if (data.getDynamicObject() != nullptr)
		{
			reportScriptError("data must be a list of JSON objects with the structure {\"id\": \"My ID\", \"value\": 0.5}");
		}

		if (data.isArray())
		{
			struct IndexSorter
			{
				IndexSorter(MainController::UserPresetHandler& p) :
					uph(p)
				{};

				int compareElements(const var& first, const var& second) const
				{
					Identifier i1(first["id"].toString());
					Identifier i2(first["id"].toString());

                    auto firstIndex = 0;
                    auto secondIndex = 0;

                    if(auto fi = uph.getCustomAutomationData(i1))
                        firstIndex = fi->index;
                    
                    if(auto si = uph.getCustomAutomationData(i2))
                        secondIndex = si->index;
                    
					if (firstIndex < secondIndex)
						return -1;
					if (firstIndex > secondIndex)
						return 1;

					return 0;
				};

				MainController::UserPresetHandler& uph;
			};

			IndexSorter sorter(uph);

			data.getArray()->sort(sorter);

			for (auto& v : *data.getArray())
			{
				Identifier id(v["id"].toString());
				auto value = v["value"];

				if (auto cData = uph.getCustomAutomationData(id))
				{
					float fv = (float)value;
					FloatSanitizers::sanitizeFloatNumber(fv);
					cData->call(fv, n);
				}
			}
		}
    }
    else
    {
        getMainController()->getControlUndoManager()->perform(new AutomationValueUndoAction(this, data, n));
    }
}


void ScriptUserPresetHandler::resetToDefaultUserPreset()
{
	auto& uph = getMainController()->getUserPresetHandler();

	if(uph.defaultPresetManager != nullptr)
	{
		uph.defaultPresetManager->resetToDefault();
	}
	else
	{
		reportScriptError("You need to set a default user preset in order to user this method");
	}

}

double ScriptUserPresetHandler::getSecondsSinceLastPresetLoad()
{
	auto& uph = getMainController()->getUserPresetHandler();
	return uph.getSecondsSinceLastPresetLoad();
}

juce::var ScriptUserPresetHandler::createObjectForAutomationValues()
{
	auto& uph = getMainController()->getUserPresetHandler();

	Array<var> list;

	// just refresh the values from the current processor states
	for (int i = 0; i < uph.getNumCustomAutomationData(); i++)
	{
		auto ad = uph.getCustomAutomationData(i);

		DynamicObject* obj = new DynamicObject();
		obj->setProperty("id", ad->id);
		obj->setProperty("value", ad->lastValue);
		list.add(var(obj));
	}

	return var(list);
}

juce::var ScriptUserPresetHandler::createObjectForSaveInPresetComponents()
{
	auto content = getScriptProcessor()->getScriptingContent();

	auto v = content->exportAsValueTree();

	for (auto c : v)
		c.removeProperty("type", nullptr);

	return ValueTreeConverters::convertValueTreeToDynamicObject(v);
}

void ScriptUserPresetHandler::updateSaveInPresetComponents(var obj)
{
	auto content = getScriptProcessor()->getScriptingContent();

	auto v = ValueTreeConverters::convertDynamicObjectToValueTree(obj, "Content");

	for (auto c : v)
	{
		if(auto sc =  content->getComponentWithName(Identifier(c["id"])))
		{
			auto type = sc->getScriptObjectProperty("type");
			c.setProperty("type", type, nullptr);
		}
	}

	content->restoreAllControlsFromPreset(v);
}

void ScriptUserPresetHandler::updateConnectedComponentsFromModuleState()
{
	auto content = getScriptProcessor()->getScriptingContent();

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		auto sc = content->getComponent(i);

        sc->updateValueFromProcessorConnection();
	}
}



void ScriptUserPresetHandler::runTest()
{
	auto content = getScriptProcessor()->getScriptingContent();
	auto& uph = getMainController()->getUserPresetHandler();

	String report = "\n";

	auto addLine = [&](const String& s)
	{
		report << s << "\n";
	};

	auto addLineFromTokens = [&](const StringArray& s)
	{
		for (auto& t : s)
			report << t;

		report << "\n";
	};

	auto addWarning = [&](const String& s)
	{
		report << "WARNING: " << s << "\n";
	};

	auto boolString = [](bool v)
	{
		return String(v ? "true" : "false");
	};

	auto getNumElements = [&](const String& type)
	{
		if (type == "allComponents")
		{
			return String(content->getNumComponents());
		}
		if (type == "saveInPreset")
		{
			int counter = 0;

			for (int i = 0; i < content->getNumComponents(); i++)
			{
				if (content->getComponent(i)->getScriptObjectProperty("saveInPreset"))
					counter++;
			}

			return String(counter);
		}
		if (type == "automationID")
		{
			return String(uph.getNumCustomAutomationData());
		}
		if (type == "moduleStates")
		{
			return String(getScriptProcessor()->getMainController_()->getModuleStateManager().modules.size());
		}
        
        return String("unknown");
	};

				addLine("| ====================== USER PRESET TEST ================== |");
	addLineFromTokens({ "| Stats: ", "isCustomModel: ", boolString(uph.isUsingCustomDataModel()) });
	addLineFromTokens({ "|        ", "isCustomAutomation: ", boolString(uph.isUsingCustomDataModel()) });
	addLineFromTokens({ "|        ", "numSaveInPreset: ", getNumElements("saveInPreset") });
	addLineFromTokens({ "|        ", "totalComponents: ", getNumElements("allComponents")});
	addLineFromTokens({ "|        ", "automationSlots: ", getNumElements("automationID")});
	addLineFromTokens({ "|        ", "moduleStates: ", getNumElements("moduleStates") });
				addLine("| ========================================================== |");

	addLine("Testing persistency of connected components...");
	for (int i = 0; i < content->getNumComponents(); i++)
	{
		auto cp = content->getComponent(i)->getConnectedProcessor();
		auto sip = content->getComponent(i)->getScriptObjectProperty("saveInPreset");
		auto id = content->getComponent(i)->getName().toString();

		if (cp != nullptr)
		{
			if (!sip)
			{
				addWarning(id + " is connected to a processor but does not have saveInPreset enabled");
			}

			for (auto l : getScriptProcessor()->getMainController_()->getModuleStateManager().modules)
			{
				if (l->p == cp)
				{
					addWarning(id + " is connected to a processor that is restored with a module state.");
				}	
			}
		}
	}
	addLine("...OK");
	
	if (uph.isUsingCustomDataModel())
	{
		addLine("Test custom data consistency...");
		auto data1 = saveCustomUserPreset("test_save");
		loadCustomUserPreset(data1);
		auto data2 = saveCustomUserPreset("test_save");
		auto ok = JSON::toString(data1).compare(JSON::toString(data2)) == 0;

		if (!ok)
			addWarning("Data inconsistency detected");
		addLine("...OK");
	}

	auto& moduleData = getScriptProcessor()->getMainController_()->getModuleStateManager().modules;

	if (!moduleData.isEmpty())
	{
		addLine("| ============== Module State Information ================== |");
		for (auto l : moduleData)
		{
			addLineFromTokens({ "Module State for ", l->p->getId() });
			auto v = l->p->exportAsValueTree();
			l->stripValueTree(v);
			
			addLine(v.createXml()->createDocument(""));
		}
		addLine("| ========================================================== |");
	}

	debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), report);
}

var ScriptUserPresetHandler::convertToJson(const ValueTree& d)
{
	DynamicObject::Ptr p = new DynamicObject();

	{
		ValueTree dataTree;

		String version;

		if (JSONConversionHelpers::isPluginState(d))
		{
			dataTree = d.getChildWithName("InterfaceData").getChildWithName("Content");

			// Unfortunately the plugin state does not save the version number until recently...
			if (!d.hasProperty("Version"))
				version = "0.0.0";
			else
				version = d["Version"].toString();
		}
		else
		{
			dataTree = d.getChildWithName("Content");
			version = d["Version"].toString();
		}
		
		Array<var> dataArray;

		p->setProperty("version", d["Version"]);

		for (const auto& c : dataTree)
		{
			DynamicObject::Ptr cd = new DynamicObject();

			for (int i = 0; i < c.getNumProperties(); i++)
			{
				auto id = c.getPropertyName(i);
				auto value = c[id];

				if (id == Identifier("value"))
				{
					auto valueString = value.toString();

					if (unpackComplexData && valueString.startsWith("JSON"))
						value = JSON::parse(valueString.substring(4));
				}

				if (unpackComplexData && id == Identifier("data"))
					value = JSONConversionHelpers::convertBase64Data(value.toString(), c);

				cd->setProperty(id, value);
			}

			dataArray.add(var(cd.get()));
		}

		p->setProperty("Content", var(dataArray));
		
		p->setProperty("Modules", JSONConversionHelpers::valueTreeToJSON(d.getChildWithName("Modules")));
		p->setProperty("MidiAutomation", JSONConversionHelpers::valueTreeToJSON(d.getChildWithName("MidiAutomation")));
		p->setProperty("MPEData", JSONConversionHelpers::valueTreeToJSON(d.getChildWithName("MPEData")));

	}

	return var(p.get());
}



juce::ValueTree ScriptUserPresetHandler::applyJSON(const ValueTree& original, DynamicObject::Ptr obj)
{
	if (obj == nullptr)
		return original;

	auto copy = original.createCopy();

	ValueTree dataTree;

	if (JSONConversionHelpers::isPluginState(original))
	{
		dataTree = copy.getChildWithName("InterfaceData").getChildWithName("Content");
		dataTree.removeAllChildren(nullptr);
	}
	else
	{
		dataTree = copy.getChildWithName("Content");
		dataTree.removeAllChildren(nullptr);
	}

	if (auto dataArray = obj->getProperty("Content").getArray())
	{
		for (const auto& p : *dataArray)
		{
			ValueTree c("Control");

			if (auto obj = p.getDynamicObject())
			{
				for (const auto& nv : obj->getProperties())
				{
					auto vTouse = nv.value;

					if (nv.name == Identifier("value"))
					{
						if(vTouse.isArray() || vTouse.isObject())
							vTouse = "JSON" + JSON::toString(vTouse);
					}

					if (unpackComplexData && nv.name == Identifier("data"))
					{
						vTouse = JSONConversionHelpers::convertDataToBase64(vTouse, c);
					}

					c.setProperty(nv.name, vTouse, nullptr);
				}
			}
			
			dataTree.addChild(c, -1, nullptr);
		}
	}

	copy.removeChild(copy.getChildWithName("Modules"), nullptr);
	copy.removeChild(copy.getChildWithName("MidiAutomation"), nullptr);
	copy.removeChild(copy.getChildWithName("MPEData"), nullptr);

	copy.addChild(JSONConversionHelpers::jsonToValueTree(var(obj.get()), "Modules"), -1, nullptr);
	copy.addChild(JSONConversionHelpers::jsonToValueTree(var(obj.get()), "MidiAutomation"), -1, nullptr);
	copy.addChild(JSONConversionHelpers::jsonToValueTree(var(obj.get()), "MPEData"), -1, nullptr);
	return copy;
}

juce::ValueTree ScriptUserPresetHandler::prePresetLoad(const ValueTree& dataToLoad, const File& fileToLoad)
{
	currentlyLoadedFile = fileToLoad;

	if (preCallback)
	{
		var args;

		if (enablePreprocessing)
			args = convertToJson(dataToLoad);
		else
			args = var(new ScriptingObjects::ScriptFile(getScriptProcessor(), fileToLoad));

		auto r = preCallback.callSync(&args, 1, nullptr);

		if (enablePreprocessing)
			return applyJSON(dataToLoad, args.getDynamicObject());
	}

	return dataToLoad;
}

void ScriptUserPresetHandler::presetChanged(const File& newPreset)
{
	if (postCallback)
	{
		var f;

		if(newPreset.existsAsFile())
			f = var(new ScriptingObjects::ScriptFile(getScriptProcessor(), newPreset));

		postCallback.call(&f, 1);
	}
}


void ScriptUserPresetHandler::presetSaved(const File& newPreset)
{
	if (postSaveCallback)
	{
		var f;

		if (newPreset.existsAsFile())
			f = var(new ScriptingObjects::ScriptFile(getScriptProcessor(), newPreset));

		postSaveCallback.call(&f, 1);
	}
}

struct ScriptExpansionHandler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setErrorFunction);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setErrorMessage);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setCredentials);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setInstallFullDynamics);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setEncryptionKey);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, setCurrentExpansion);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, getExpansionList);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, getExpansion);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, getCurrentExpansion);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setExpansionCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setInstallCallback);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, encodeWithCredentials);
	API_METHOD_WRAPPER_0(ScriptExpansionHandler, refreshExpansions);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionHandler, setAllowedExpansionTypes);
	API_METHOD_WRAPPER_2(ScriptExpansionHandler, installExpansionFromPackage);
	API_METHOD_WRAPPER_1(ScriptExpansionHandler, getExpansionForInstallPackage);
};

ScriptExpansionHandler::ScriptExpansionHandler(JavascriptProcessor* jp_) :
	ConstScriptingObject(dynamic_cast<ProcessorWithScriptingContent*>(jp_), 3),
	ControlledObject(dynamic_cast<ControlledObject*>(jp_)->getMainController()),
	jp(jp_),
	expansionCallback(dynamic_cast<ProcessorWithScriptingContent*>(jp_), nullptr, var(), 1),
	errorFunction(dynamic_cast<ProcessorWithScriptingContent*>(jp_), nullptr, var(), 2),
	installCallback(dynamic_cast<ProcessorWithScriptingContent*>(jp_), nullptr, var(), 1)
{
	getMainController()->getExpansionHandler().addListener(this);

	ADD_API_METHOD_1(setErrorFunction);
	ADD_API_METHOD_1(setErrorMessage);
	ADD_API_METHOD_1(setCredentials);
	ADD_API_METHOD_1(setEncryptionKey);
	ADD_API_METHOD_0(getExpansionList);
	ADD_API_METHOD_1(getExpansion);
	ADD_API_METHOD_1(setExpansionCallback);
	ADD_API_METHOD_1(setCurrentExpansion);
	ADD_API_METHOD_1(setInstallFullDynamics);
	ADD_API_METHOD_1(encodeWithCredentials);
	ADD_API_METHOD_0(refreshExpansions);
	ADD_API_METHOD_2(installExpansionFromPackage);
	ADD_API_METHOD_1(setAllowedExpansionTypes);
	ADD_API_METHOD_0(getCurrentExpansion);
	ADD_API_METHOD_1(setInstallCallback);
	ADD_API_METHOD_1(getExpansionForInstallPackage);

	

	addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::FileBased), Expansion::FileBased);
	addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::Intermediate), Expansion::Intermediate);
	addConstant(Expansion::Helpers::getExpansionTypeName(Expansion::Encrypted), Expansion::Encrypted);
}

ScriptExpansionHandler::~ScriptExpansionHandler()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

void ScriptExpansionHandler::setEncryptionKey(String newKey)
{
	reportScriptError("This function is deprecated. Use the project settings to setup the project's blowfish key");
	//getMainController()->getExpansionHandler().setEncryptionKey(newKey);
}

void ScriptExpansionHandler::setCredentials(var newCredentials)
{
	if (newCredentials.getDynamicObject() == nullptr)
	{
		setErrorMessage("credentials must be an object");
		return;
	}

	getMainController()->getExpansionHandler().setCredentials(newCredentials);
}

void ScriptExpansionHandler::setInstallFullDynamics(bool shouldInstallFullDynamics)
{
	getMainController()->getExpansionHandler().setInstallFullDynamics(shouldInstallFullDynamics);
}

void ScriptExpansionHandler::setErrorFunction(var newErrorFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(newErrorFunction))
		errorFunction = WeakCallbackHolder(getScriptProcessor(), this, newErrorFunction, 1);

	errorFunction.setHighPriority();
}

void ScriptExpansionHandler::setErrorMessage(String errorMessage)
{
	logMessage(errorMessage, false);

}

var ScriptExpansionHandler::getExpansionList()
{
	auto& h = getMainController()->getExpansionHandler();

	Array<var> list;

	for (int i = 0; i < h.getNumExpansions(); i++)
	{
		list.add(new ScriptExpansionReference(dynamic_cast<ProcessorWithScriptingContent*>(jp.get()), h.getExpansion(i)));
	}

	return var(list);
}

var ScriptExpansionHandler::getExpansion(var name)
{
	if (auto e = getMainController()->getExpansionHandler().getExpansionFromName(name.toString()))
	{
		return new ScriptExpansionReference(getScriptProcessor(), e);
	}

	return var();
}

var ScriptExpansionHandler::getCurrentExpansion()
{
	if (auto e = getMainController()->getExpansionHandler().getCurrentExpansion())
		return new ScriptExpansionReference(getScriptProcessor(), e);

	return {};
}

void ScriptExpansionHandler::setExpansionCallback(var expansionLoadedCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(expansionLoadedCallback))
	{
		expansionCallback = WeakCallbackHolder(getScriptProcessor(), this, expansionLoadedCallback, 1);
		expansionCallback.incRefCount();
		expansionCallback.addAsSource(this, "onExpansionLoad");
		expansionCallback.setThisObject(this);
		
	}

	expansionCallback.setHighPriority();
}

void ScriptExpansionHandler::setInstallCallback(var installationCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(installationCallback))
	{
		installCallback = WeakCallbackHolder(getScriptProcessor(), this, installationCallback, 1);
		installCallback.incRefCount();
		installCallback.addAsSource(this, "onExpansionInstall");
		installCallback.setThisObject(this);
	}
}

var ScriptExpansionHandler::getUninitialisedExpansions()
{
	Array<var> list;

	for (auto e : getMainController()->getExpansionHandler().getListOfUnavailableExpansions())
		list.add(new ScriptExpansionReference(dynamic_cast<ProcessorWithScriptingContent*>(jp.get()), e));

	return list;
}

bool ScriptExpansionHandler::refreshExpansions()
{
	return getMainController()->getExpansionHandler().createAvailableExpansions();
}

bool ScriptExpansionHandler::setCurrentExpansion(var expansionName)
{
	if(expansionName.isString())
		return getMainController()->getExpansionHandler().setCurrentExpansion(expansionName);
	
	if (auto se = dynamic_cast<ScriptExpansionReference*>(expansionName.getObject()))
		return setCurrentExpansion(se->exp->getProperty(ExpansionIds::Name));

	

	reportScriptError("can't find expansion");
	RETURN_IF_NO_THROW(false);
}

bool ScriptExpansionHandler::encodeWithCredentials(var hxiFile)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(hxiFile.getObject()))
	{
		if (!f->f.existsAsFile())
			reportScriptError(f->toString(0) + " doesn't exist");

		return ScriptEncryptedExpansion::encryptIntermediateFile(getMainController(), f->f);
	}
	else
	{
		reportScriptError("argument is not a file");
		RETURN_IF_NO_THROW(false);
	}
}

bool ScriptExpansionHandler::installExpansionFromPackage(var packageFile, var sampleDirectory)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(packageFile.getObject()))
	{
		File targetFolder;

		if (sampleDirectory.isInt())
		{
			auto target = (ScriptingApi::FileSystem::SpecialLocations)(int)sampleDirectory;

			if (target == ScriptingApi::FileSystem::Expansions)
				targetFolder = getMainController()->getExpansionHandler().getExpansionFolder();
			if (target == ScriptingApi::FileSystem::Samples)
				targetFolder = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Samples);
		}
		else if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(sampleDirectory.getObject()))
		{
			targetFolder = sf->f;
		}

		if (!targetFolder.isDirectory())
			reportScriptError("The sample directory does not exist");
		
		if (installCallback)
		{
			currentInstaller = new InstallState(*this);
		}

		return getMainController()->getExpansionHandler().installFromResourceFile(f->f, targetFolder);
	}
	else
	{
		reportScriptError("argument is not a file");
		RETURN_IF_NO_THROW(false);
	}
}

var ScriptExpansionHandler::getExpansionForInstallPackage(var packageFile)
{
	if (auto sf = dynamic_cast<ScriptingObjects::ScriptFile*>(packageFile.getObject()))
	{
		auto& eh = getMainController()->getExpansionHandler();
		auto tf = eh.getExpansionTargetFolder(sf->f);

		if (tf == File())
			logMessage("Can't read metadata of package", false);

		if (auto e = eh.getExpansionFromRootFile(tf))
		{
			//  In order to simulate the end user process, we do not detect file based expansions
			//  with this call.
			if (e->getExpansionType() != Expansion::FileBased)
			{
				return var(new ScriptExpansionReference(getScriptProcessor(), e));
			}
		}

		return var();
	}

	reportScriptError("getExpansionForInstallPackage requires a file as parameter");
	RETURN_IF_NO_THROW(var());
}

void ScriptExpansionHandler::setAllowedExpansionTypes(var typeList)
{
	Array<Expansion::ExpansionType> l;

	if (auto ar = typeList.getArray())
	{
		for (auto& v : *ar)
			l.add((Expansion::ExpansionType)(int)v);

		getMainController()->getExpansionHandler().setAllowedExpansions(l);
	}
	else
		reportScriptError("Argument must be an array");
}

void ScriptExpansionHandler::expansionPackLoaded(Expansion* currentExpansion)
{
	expansionPackCreated(currentExpansion);
}

void ScriptExpansionHandler::expansionPackCreated(Expansion* newExpansion)
{
	if (expansionCallback)
	{
		if (newExpansion != nullptr)
		{
			var args(new ScriptExpansionReference(getScriptProcessor(), newExpansion));
			expansionCallback.call(&args, 1);
		}
		else
		{
			var args;
			expansionCallback.call(&args, 1);
		}

		
	}
}

void ScriptExpansionHandler::logMessage(const String& message, bool isCritical)
{

	if (errorFunction)
	{
		var args[2];
		args[0] = message;
		args[1] = isCritical;
		errorFunction.call(args, 2);
	}
}

hise::ProcessorWithScriptingContent* ScriptExpansionHandler::getScriptProcessor()
{
	return dynamic_cast<ProcessorWithScriptingContent*>(jp.get());
}


ScriptExpansionHandler::InstallState::InstallState(ScriptExpansionHandler& parent_) :
	parent(parent_),
	status(-1)
{
	parent.getMainController()->getExpansionHandler().addListener(this);
}

ScriptExpansionHandler::InstallState::~InstallState()
{
	parent.getMainController()->getExpansionHandler().removeListener(this);
}

void ScriptExpansionHandler::InstallState::expansionPackCreated(Expansion* newExpansion)
{
	
}

void ScriptExpansionHandler::InstallState::expansionInstallStarted(const File& targetRoot, const File& packageFile, const File& sampleDirectory)
{
	sourceFile = packageFile;
	targetFolder = targetRoot;
	sampleFolder = sampleDirectory;
	createdExpansion = nullptr;
	status = 0;
	call();
	startTimer(300);
}

void ScriptExpansionHandler::InstallState::expansionInstalled(Expansion* newExpansion)
{
	SimpleReadWriteLock::ScopedWriteLock sl(timerLock);

	stopTimer();
	status = 2;

	if (newExpansion != nullptr && newExpansion->getRootFolder() == targetFolder)
		createdExpansion = newExpansion;

	call();

	WeakReference<ScriptExpansionHandler> safeParent(&parent);

	auto f = [safeParent]()
	{
		if (safeParent != nullptr)
			safeParent.get()->currentInstaller = nullptr;
	};
}

void ScriptExpansionHandler::InstallState::call()
{
	parent.installCallback.call1(getObject());
}

void ScriptExpansionHandler::InstallState::timerCallback()
{
	if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(timerLock))
	{
		status = 1;
		call();
	}
}

var ScriptExpansionHandler::InstallState::getObject()
{
	auto newObj = new DynamicObject();
	newObj->setProperty("Status", status);
	newObj->setProperty("Progress", getProgress());
	newObj->setProperty("TotalProgress", getTotalProgress());
	newObj->setProperty("SourceFile", new ScriptingObjects::ScriptFile(parent.getScriptProcessor(), sourceFile));
	newObj->setProperty("TargetFolder", new ScriptingObjects::ScriptFile(parent.getScriptProcessor(), targetFolder));
	newObj->setProperty("SampleFolder", new ScriptingObjects::ScriptFile(parent.getScriptProcessor(), sampleFolder));
	newObj->setProperty("Expansion", (createdExpansion != nullptr) ? var(new ScriptExpansionReference(parent.getScriptProcessor(), createdExpansion)) : var());

	return var(newObj);
}

double ScriptExpansionHandler::InstallState::getProgress()
{
	return parent.getMainController()->getSampleManager().getPreloadProgress();
}

double ScriptExpansionHandler::InstallState::getTotalProgress()
{
	return parent.getMainController()->getExpansionHandler().getTotalProgress();
}

struct ScriptExpansionReference::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleMapList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getImageList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getAudioFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getMidiFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getDataFileList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getUserPresetList);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getProperties);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, loadDataFile);
	API_METHOD_WRAPPER_2(ScriptExpansionReference, writeDataFile);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getRootFolder);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getExpansionType);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, getWildcardReference);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, getSampleFolder);
	API_METHOD_WRAPPER_1(ScriptExpansionReference, setSampleFolder);
	API_METHOD_WRAPPER_0(ScriptExpansionReference, rebuildUserPresets);
	API_VOID_METHOD_WRAPPER_0(ScriptExpansionReference, unloadExpansion);
	API_VOID_METHOD_WRAPPER_1(ScriptExpansionReference, setAllowDuplicateSamples);
};

ScriptExpansionReference::ScriptExpansionReference(ProcessorWithScriptingContent* p, Expansion* e) :
	ConstScriptingObject(p, 0),
	exp(e)
{
	ADD_API_METHOD_0(getSampleMapList);
	ADD_API_METHOD_0(getImageList);
	ADD_API_METHOD_0(getAudioFileList);
	ADD_API_METHOD_0(getMidiFileList);
	ADD_API_METHOD_0(getDataFileList);
	ADD_API_METHOD_0(getUserPresetList);
	ADD_API_METHOD_0(getProperties);
	ADD_API_METHOD_1(loadDataFile);
	ADD_API_METHOD_2(writeDataFile);
	ADD_API_METHOD_0(getRootFolder);
	ADD_API_METHOD_0(getExpansionType);
	ADD_API_METHOD_1(getWildcardReference);
	ADD_API_METHOD_1(setSampleFolder);
	ADD_API_METHOD_0(getSampleFolder);
	ADD_API_METHOD_0(rebuildUserPresets);
	ADD_API_METHOD_1(setAllowDuplicateSamples);
	ADD_API_METHOD_0(unloadExpansion);
}

juce::BlowFish* ScriptExpansionReference::createBlowfish()
{
	if (auto se = dynamic_cast<ScriptEncryptedExpansion*>(exp.get()))
	{
		return se->createBlowfish();
	}

	return nullptr;
}

var ScriptExpansionReference::getRootFolder()
{
	if (objectExists())
		return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), exp->getRootFolder()));

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getSampleMapList() const
{
	if (objectExists())
	{
		auto refList = exp->pool->getSampleMapPool().getListOfAllReferences(true);

		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString().upToFirstOccurrenceOf(".xml", false, true));

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getImageList() const
{
	if (objectExists())
	{
		exp->pool->getImagePool().loadAllFilesFromProjectFolder();
		auto refList = exp->pool->getImagePool().getListOfAllReferences(true);


		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getAudioFileList() const
{
	if (objectExists())
	{
		exp->pool->getAudioSampleBufferPool().loadAllFilesFromProjectFolder();
		auto refList = exp->pool->getAudioSampleBufferPool().getListOfAllReferences(true);



		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getMidiFileList() const
{
	if (objectExists())
	{
		auto refList = exp->pool->getMidiFilePool().getListOfAllReferences(true);

		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getDataFileList() const
{
	if (objectExists())
	{
		auto refList = exp->pool->getAdditionalDataPool().getListOfAllReferences(true);

		Array<var> list;

		for (auto& ref : refList)
			list.add(ref.getReferenceString());

		return list;
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getUserPresetList() const
{
	if (objectExists())
	{
 		File userPresetRoot = exp->getSubDirectory(FileHandlerBase::UserPresets);
		
		Array<File> presets;
		userPresetRoot.findChildFiles(presets, File::findFiles, true, "*.preset");
		
		Array<var> list;

		for (auto& pr : presets)
		{
			auto name = pr.getRelativePathFrom(userPresetRoot).upToFirstOccurrenceOf(".preset", false, true);
			name = name.replaceCharacter('\\', '/');

			list.add(var(name));
		}

		return var(list);
	}

	reportScriptError("Expansion was deleted");
	RETURN_IF_NO_THROW({});
}

var ScriptExpansionReference::getSampleFolder()
{
	File sampleFolder = exp->getSubDirectory(FileHandlerBase::Samples);

	return new ScriptingObjects::ScriptFile(getScriptProcessor(), sampleFolder);
}

bool ScriptExpansionReference::setSampleFolder(var newSampleFolder)
{
	if (auto f = dynamic_cast<ScriptingObjects::ScriptFile*>(newSampleFolder.getObject()))
	{
		auto newTarget = f->f;

		if (!newTarget.isDirectory())
			reportScriptError(newTarget.getFullPathName() + " is not an existing directory");

		if (newTarget != exp->getSubDirectory(FileHandlerBase::Samples))
		{
			exp->createLinkFile(FileHandlerBase::Samples, newTarget);
			exp->checkSubDirectories();
			return true;
		}
	}

	return false;
}

var ScriptExpansionReference::loadDataFile(var relativePath)
{
	if (objectExists())
	{
		if (exp->getExpansionType() == Expansion::FileBased)
		{
			auto fileToLoad = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile(relativePath.toString());

			if(fileToLoad.existsAsFile())
				return JSON::parse(fileToLoad.loadFileAsString());
		}
		else
		{
			String rs;

			auto wc = exp->getWildcard();
			auto path = relativePath.toString();

			if (!path.contains(wc))
				rs << wc;

			rs << path;

			PoolReference ref(getScriptProcessor()->getMainController_(), rs, FileHandlerBase::AdditionalSourceCode);
			
			if (auto o = exp->pool->getAdditionalDataPool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong))
			{
				var obj;
				auto ok = JSON::parse(o->data.getFile(), obj);

				if (ok.wasOk())
					return obj;

				reportScriptError("Error at parsing JSON: " + ok.getErrorMessage());
			}
		}
	}

	return {};
}

String ScriptExpansionReference::getWildcardReference(var relativePath)
{
	if (objectExists())
		return exp->getWildcard() + relativePath.toString();

	return {};
}

bool ScriptExpansionReference::writeDataFile(var relativePath, var dataToWrite)
{
	auto content = JSON::toString(dataToWrite);

	auto targetFile = exp->getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile(relativePath.toString());

	return targetFile.replaceWithText(content);
}

var ScriptExpansionReference::getProperties() const
{
	if (objectExists())
		return exp->getPropertyObject();

	return {};
}

int ScriptExpansionReference::getExpansionType() const
{
	if (objectExists())
		return (int)exp->getExpansionType();

	return -1;
}

bool ScriptExpansionReference::rebuildUserPresets()
{
	if (auto sf = dynamic_cast<ScriptEncryptedExpansion*>(exp.get()))
	{
		ValueTree v;
		auto ok = sf->loadValueTree(v);

		if (ok.wasOk())
		{
			sf->extractUserPresetsIfEmpty(v, true);
			return true;
		}
		else
		{
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), "Error at extracting user presets: ");
			debugError(dynamic_cast<Processor*>(getScriptProcessor()), ok.getErrorMessage());
		}
	}

	return false;
}

void ScriptExpansionReference::setAllowDuplicateSamples(bool shouldAllowDuplicates)
{
	if (exp != nullptr)
		exp->pool->getSamplePool()->setAllowDuplicateSamples(shouldAllowDuplicates);
}

void ScriptExpansionReference::unloadExpansion()
{
	if (exp != nullptr)
		exp->getMainController()->getExpansionHandler().unloadExpansion(exp);
}

ScriptEncryptedExpansion::ScriptEncryptedExpansion(MainController* mc, const File& f) :
	Expansion(mc, f)
{

}


hise::Expansion::ExpansionType ScriptEncryptedExpansion::getExpansionType() const
{

	return getExpansionTypeFromFolder(getRootFolder());
}

Result ScriptEncryptedExpansion::encodeExpansion()
{
	if (getExpansionType() == Expansion::FileBased)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
		{
			return Result::fail("You have to set an encryption key using `ExpansionHandler.setEncryptionKey()` before using this method.");
		}

		String s;
		s << "Do you want to encode the expansion " << getProperty(ExpansionIds::Name) << "?  \n> The encryption key is `" << handler.getEncryptionKey() << "`.";

		if (true)//PresetHandler::showYesNoWindow("Encode expansion", s))
		{
			auto hxiFile = Expansion::Helpers::getExpansionInfoFile(getRootFolder(), Expansion::Intermediate);

			ValueTree hxiData("Expansion");

			auto metadata = data->v.createCopy();
			metadata.setProperty(ExpansionIds::Hash, handler.getEncryptionKey().hashCode64(), nullptr);

			hxiData.addChild(metadata, -1, nullptr);
			encodePoolAndUserPresets(hxiData, false);

#if HISE_USE_XML_FOR_HXI
			ScopedPointer<XmlElement> xml = hxiData.createXml();
			hxiFile.replaceWithText(xml->createDocument(""));
#else
			hxiFile.deleteFile();
			FileOutputStream fos(hxiFile);
			hxiData.writeToStream(fos);
			fos.flush();
#endif
			auto h = &getMainController()->getExpansionHandler();

			h->forceReinitialisation();
		}
	}
	else
	{
		return Result::fail("The expansion " + getProperty(ExpansionIds::Name) + " is already encoded");
	}

	return Result::ok();
}

juce::Array<hise::FileHandlerBase::SubDirectories> ScriptEncryptedExpansion::getSubDirectoryIds() const
{
	if (getExpansionType() == Expansion::FileBased)
		return Expansion::getSubDirectoryIds();
	else
	{
		if (getRootFolder().getChildFile("UserPresets").isDirectory())
			return { FileHandlerBase::UserPresets, FileHandlerBase::Samples };
		else
			return { FileHandlerBase::Samples };
	}

}


juce::Result ScriptEncryptedExpansion::loadValueTree(ValueTree& v)
{
	if(getExpansionType() == Expansion::Intermediate)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
		{
			v = ValueTree(ExpansionIds::ExpansionInfo);
			v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
			return Result::ok(); // will fail later
		}

		auto fileToLoad = Helpers::getExpansionInfoFile(getRootFolder(), Intermediate);

	#if HISE_USE_XML_FOR_HXI
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
		{
			v = ValueTree::fromXml(*xml);
			return Result::ok();
		}

		return Result::fail("Can't parse XML");
	#else
		FileInputStream fis(fileToLoad);
		v = ValueTree::readFromStream(fis);

		if (v.isValid())
			return Result::ok();
		else
			return Result::fail("Can't parse ValueTree");
	#endif
	}
	if (getExpansionType() == Expansion::Encrypted)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty() || !handler.getCredentials().isObject())
		{
			v = ValueTree(ExpansionIds::ExpansionInfo);
			v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
			return Result::ok(); // will fail later
		}

		zstd::ZDefaultCompressor comp;
		auto f = Helpers::getExpansionInfoFile(getRootFolder(), Encrypted);

		FileInputStream fis(f);

		v = ValueTree::readFromStream(fis);

		if (!v.isValid())
			return Result::fail("Can't parse expansion data file");

		return Result::ok();
	}
    
    return Result::fail("Filebased expansions not supported here");
}





Result ScriptEncryptedExpansion::initialise()
{
	auto type = getExpansionType();

	if (type == Expansion::FileBased)
		return Expansion::initialise();
	else if (type == Expansion::Intermediate)
	{
		ValueTree v;
		auto ok = loadValueTree(v);

		if (v.isValid())
			return initialiseFromValueTree(v);

		return ok;



#if 0
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
			return skipEncryptedExpansionWithoutKey();

		auto fileToLoad = Helpers::getExpansionInfoFile(getRootFolder(), type);

#if HISE_USE_XML_FOR_HXI
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
			return initialiseFromValueTree(ValueTree::fromXml(*xml));

		return Result::fail("Can't parse XML");
#else
		FileInputStream fis(fileToLoad);
		auto v = ValueTree::readFromStream(fis);

		if (v.isValid())
			return initialiseFromValueTree(v);
		else
			return Result::fail("Can't parse ValueTree");
#endif
#endif

	}
	else if (type == ExpansionType::Encrypted)
	{
		auto& handler = getMainController()->getExpansionHandler();
#if 0
		

		if (handler.getEncryptionKey().isEmpty() || !handler.getCredentials().isObject())
			return skipEncryptedExpansionWithoutKey();

		zstd::ZDefaultCompressor comp;
		auto f = Helpers::getExpansionInfoFile(getRootFolder(), type);

		FileInputStream fis(f);

		auto hxpData = ValueTree::readFromStream(fis);

		if (!hxpData.isValid())
			return Result::fail("Can't parse expansion data file");
#endif

		ValueTree hxpData;

		auto ok = loadValueTree(hxpData);

		if (hxpData.getNumChildren() == 0)
		{
			data = new Data(getRootFolder(), hxpData, getMainController());
			return Result::fail("no encryption key set for scripted encryption");
		}

		auto credTree = hxpData.getChildWithName(ExpansionIds::Credentials);
		auto base64Obj = credTree[ExpansionIds::Data].toString();

		if (ScopedPointer<BlowFish> bf = createBlowfish())
		{
			MemoryBlock mb;
			mb.fromBase64Encoding(base64Obj);
			bf->decrypt(mb);
			base64Obj = mb.toBase64Encoding();
		}

		if (base64Obj.hashCode64() != (int64)credTree[ExpansionIds::Hash])
			return Result::fail("Credential hash don't match");

		auto obj = ValueTreeConverters::convertBase64ToDynamicObject(base64Obj, true);

		if (!ExpansionHandler::Helpers::equalJSONData(obj, handler.getCredentials()))
			return Result::fail("Credentials don't match");

		return initialiseFromValueTree(hxpData);
	}

	return Result::ok();
}

juce::BlowFish* ScriptEncryptedExpansion::createBlowfish()
{
	return createBlowfish(getMainController());
}

juce::BlowFish* ScriptEncryptedExpansion::createBlowfish(MainController* mc)
{
	auto d = mc->getExpansionHandler().getEncryptionKey();

	if (d.isNotEmpty())
		return new BlowFish(d.getCharPointer().getAddress(), d.length());
	else
		return nullptr;
}

bool ScriptEncryptedExpansion::encryptIntermediateFile(MainController* mc, const File& f, File expRoot)
{
	auto& h = mc->getExpansionHandler();

	auto key = h.getEncryptionKey();

	if (key.isEmpty())
		return h.setErrorMessage("Can't encode credentials without encryption key", true);

#if HISE_USE_XML_FOR_HXI
	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml == nullptr)
		return h.setErrorMessage("Can't parse XML", true);

	auto hxiData = ValueTree::fromXml(*xml);
#else
	FileInputStream fis(f);
	auto hxiData = ValueTree::readFromStream(fis);
#endif

	if (hxiData.getType() != Identifier("Expansion"))
		return h.setErrorMessage("Invalid .hxi file", true);

	if (expRoot == File())
	{
		auto hxiName = hxiData.getChildWithName(ExpansionIds::ExpansionInfo).getProperty(ExpansionIds::Name).toString();

		if (hxiName.isEmpty())
			return h.setErrorMessage("Can't get expansion name", true);

		expRoot = h.getExpansionFolder().getChildFile(hxiName);
	}

	if (!expRoot.isDirectory())
		expRoot.createDirectory();

	auto hash = (int64)hxiData.getChildWithName(ExpansionIds::ExpansionInfo)[ExpansionIds::Hash];

	if (hash != key.hashCode64())
		return h.setErrorMessage("embedded key does not match encryption key", true);

	auto obj = h.getCredentials();

	if (!obj.isObject())
		return h.setErrorMessage("No credentials set for encryption", true);

	auto c = ValueTreeConverters::convertDynamicObjectToBase64(var(obj), "Credentials", true);
	auto credentialsHash = c.hashCode64();

	ValueTree credTree(ExpansionIds::Credentials);

	MemoryBlock mb;
	mb.fromBase64Encoding(c);

	if (ScopedPointer<BlowFish> bf = createBlowfish(mc))
		bf->encrypt(mb);
	else
		return h.setErrorMessage("Can't create blowfish key", true);

	credTree.setProperty(ExpansionIds::Hash, credentialsHash, nullptr);
	credTree.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

	hxiData.addChild(credTree, 1, nullptr);

	auto hxpFile = Expansion::Helpers::getExpansionInfoFile(expRoot, Expansion::Encrypted);

	hxpFile.deleteFile();
	hxpFile.create();

	FileOutputStream fos(hxpFile);
	hxiData.writeToStream(fos);
	fos.flush();
	h.createAvailableExpansions();
	return true;
}

void ScriptEncryptedExpansion::encodePoolAndUserPresets(ValueTree &hxiData, bool projectExport)
{
	auto& h = getMainController()->getExpansionHandler();

	h.setErrorMessage("Preparing pool export", false);

	if (!projectExport)
	{
		pool->getAdditionalDataPool().loadAllFilesFromProjectFolder();
		pool->getImagePool().loadAllFilesFromProjectFolder();
		pool->getAudioSampleBufferPool().loadAllFilesFromProjectFolder();
		pool->getMidiFilePool().loadAllFilesFromProjectFolder();
	}
	else
	{
		auto nativePool = getMainController()->getCurrentFileHandler().pool.get();

		pool->getMidiFilePool().loadAllFilesFromProjectFolder();

		auto& nip = nativePool->getImagePool();
		auto& nap = nativePool->getAudioSampleBufferPool();

		BACKEND_ONLY(ExpansionHandler::ScopedProjectExporter sps(getMainController(), true));

		auto embedImageFiles = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::EmbedImageFiles);
		
		if (embedImageFiles)
		{
			for (int i = 0; i < nip.getNumLoadedFiles(); i++)
			{
				PoolReference ref(getMainController(), nip.getReference(i).getFile().getFullPathName(), FileHandlerBase::Images);
				pool->getImagePool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
			}	
		}

		auto embedAudioFiles = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::EmbedAudioFiles);
		
		if (embedAudioFiles)
		{
			for (int i = 0; i < nap.getNumLoadedFiles(); i++)
			{
				PoolReference ref(getMainController(), nap.getReference(i).getFile().getFullPathName(), FileHandlerBase::AudioFiles);
				pool->getAudioSampleBufferPool().loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
			}	
		}
	}

	ValueTree poolData(ExpansionIds::PoolData);

	for (auto fileType : getListOfPooledSubDirectories())
	{
		h.setErrorMessage("Exporting " + FileHandlerBase::getIdentifier(fileType), false);

		if (!projectExport || fileType != FileHandlerBase::AdditionalSourceCode)
			addDataType(poolData, fileType);
	}

	auto embedUserPresets = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::EmbedUserPresets);

  if (embedUserPresets)
	{
		h.setErrorMessage("Embedding user presets", false);
		addUserPresets(hxiData);
	}

	hxiData.addChild(poolData, -1, nullptr);
}

juce::Result ScriptEncryptedExpansion::skipEncryptedExpansionWithoutKey()
{
	ValueTree v(ExpansionIds::ExpansionInfo);
	v.setProperty(ExpansionIds::Name, getRootFolder().getFileName(), nullptr);
	data = new Data(getRootFolder(), v, getMainController());
	return Result::fail("no encryption key set for scripted encryption");
}

Result ScriptEncryptedExpansion::initialiseFromValueTree(const ValueTree& hxiData)
{
	if (hxiData.getNumChildren() == 0)
	{
		data = new Data(getRootFolder(), hxiData, getMainController());
		return Result::fail("no encryption key set for scripted encryption");
	}

	data = new Data(getRootFolder(), hxiData.getChildWithName(ExpansionIds::ExpansionInfo).createCopy(), getMainController());

	extractUserPresetsIfEmpty(hxiData);

	auto hash = getProperty(ExpansionIds::Hash).getLargeIntValue();

	if (getMainController()->getExpansionHandler().getEncryptionKey().hashCode64() != hash)
		return Result::fail("Wrong hash code");

	for (auto fileType : getListOfPooledSubDirectories())
	{
		setCompressorForPool(fileType, true);
		restorePool(hxiData, fileType);
	}

	pool->getSampleMapPool().loadAllFilesFromDataProvider();
	pool->getMidiFilePool().loadAllFilesFromDataProvider();
	pool->getAdditionalDataPool().loadAllFilesFromDataProvider();
	checkSubDirectories();
	return Result::ok();
}

hise::PoolBase::DataProvider::Compressor* ScriptEncryptedExpansion::createCompressor(bool createEncrypted)
{
	if (createEncrypted)
	{
		auto key = createBlowfish();
		return new EncryptedCompressor(key);
	}
	else
	{
		return new hise::PoolBase::DataProvider::Compressor();
	}
}

void ScriptEncryptedExpansion::setCompressorForPool(SubDirectories fileType, bool createEncrypted)
{
	if (auto p = pool->getPoolBase(fileType))
	{
		p->getDataProvider()->setCompressor(createCompressor(createEncrypted));
	}
}

void ScriptEncryptedExpansion::addDataType(ValueTree& parent, SubDirectories fileType)
{
	MemoryBlock mb;

	ScopedPointer<MemoryOutputStream> mos = new MemoryOutputStream(mb, false);

	auto p = pool->getPoolBase(fileType);

	setCompressorForPool(fileType, true);

	p->getDataProvider()->writePool(mos.release());

	auto id = getIdentifier(fileType).removeCharacters("/");

	ValueTree vt(id);
	vt.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

	parent.addChild(vt, -1, nullptr);
}

void ScriptEncryptedExpansion::restorePool(ValueTree encryptedTree, SubDirectories fileType)
{
	if (auto p = pool->getPoolBase(fileType))
	{
		auto poolData = encryptedTree.getChildWithName(ExpansionIds::PoolData);

		MemoryBlock mb;

		auto childName = getIdentifier(fileType).removeCharacters("/");
		auto c = poolData.getChildWithName(childName);
		auto d = c.getProperty(ExpansionIds::Data).toString();

		mb.fromBase64Encoding(d);

		ScopedPointer<MemoryInputStream> mis = new MemoryInputStream(mb, true);

		p->getDataProvider()->restorePool(mis.release());
	}
}

void ScriptEncryptedExpansion::addUserPresets(ValueTree encryptedTree)
{
	auto userPresets = UserPresetHelpers::collectAllUserPresets(getMainController()->getMainSynthChain(), this);

	MemoryBlock mb;

	zstd::ZCompressor<hise::UserPresetDictionaryProvider> comp;
	comp.compress(userPresets, mb);
	ValueTree v("UserPresets");
	v.setProperty("Data", mb.toBase64Encoding(), nullptr);
	encryptedTree.addChild(v, -1, nullptr);
}

void ScriptEncryptedExpansion::extractUserPresetsIfEmpty(ValueTree encryptedTree, bool forceExtraction)
{
	auto presetTree = encryptedTree.getChildWithName("UserPresets");

	// the directory might not have been created yet...
	auto targetDirectory = getRootFolder().getChildFile(FileHandlerBase::getIdentifier(FileHandlerBase::UserPresets));

#if READ_ONLY_FACTORY_PRESETS
	bool createPathList = true;
#else
	bool createPathList = false;
#endif

	if (!targetDirectory.isDirectory() || forceExtraction || createPathList)
	{
		MemoryBlock mb;
		mb.fromBase64Encoding(presetTree.getProperty(ExpansionIds::Data).toString());
		ValueTree p;
		zstd::ZCompressor<hise::UserPresetDictionaryProvider> comp;
		comp.expand(mb, p);

#if READ_ONLY_FACTORY_PRESETS
		if (createPathList)
		{
			for (auto c : p)
				getMainController()->getUserPresetHandler().getFactoryPaths().addRecursive(c, getWildcard());
		}
#endif

		if (p.getNumChildren() != 0)
		{
			targetDirectory.createDirectory();
			UserPresetHelpers::extractDirectory(p, targetDirectory);
		}
	}
}

void FullInstrumentExpansion::setNewDefault(MainController* mc, ValueTree t)
{
	if (isEnabled(mc))
		mc->setDefaultPresetHandler(new DefaultHandler(mc, t));
}

bool FullInstrumentExpansion::isEnabled(const MainController* mc)
{
#if USE_BACKEND
	return dynamic_cast<const GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Project::ExpansionType) == "Full";
#else
	ignoreUnused(mc);
	return FrontendHandler::getExpansionType() == "Full";
#endif
}

Expansion* FullInstrumentExpansion::getCurrentFullExpansion(const MainController* mc)
{
	if (!isEnabled(mc))
		return nullptr;

	return mc->getExpansionHandler().getCurrentExpansion();
}

void FullInstrumentExpansion::expansionPackLoaded(Expansion* e)
{

	if (e == this)
	{
		if (fullyLoaded)
		{
			auto pr = presetToLoad.createCopy();

			getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), [pr](Processor* p)
			{
				p->getMainController()->loadPresetFromValueTree(pr);
				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		}
		else
		{
			getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), [this](Processor* p)
			{
				getMainController()->getSampleManager().setCurrentPreloadMessage("Initialising expansion");
				auto r = lazyLoad();

				if (r.wasOk())
				{
					auto pr = presetToLoad.createCopy();
					p->getMainController()->loadPresetFromValueTree(pr);
				}

				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		}


	}
}

struct PublicIconProvider : public PoolBase::DataProvider
{
	PublicIconProvider(PoolBase* pool, const String& baseString) :
		DataProvider(pool)
	{
		mb.fromBase64Encoding(baseString);
	}

	MemoryInputStream* createInputStream(const String& referenceString) override
	{
		if (referenceString.fromLastOccurrenceOf("}", false, false).toUpperCase() == "ICON.PNG")
		{
			return new MemoryInputStream(mb, false);
		}

		jassertfalse;
		return nullptr;
	}


	MemoryBlock mb;
};

juce::Result FullInstrumentExpansion::initialise()
{
	auto type = getExpansionType();

	if (type == Expansion::FileBased)
	{
		return Expansion::initialise();
	}
	if (type == Expansion::Intermediate)
	{
		auto& handler = getMainController()->getExpansionHandler();

		if (handler.getEncryptionKey().isEmpty())
			return Result::fail("The encryption key for a Full expansion must be set already");

		auto allData = getValueTreeFromFile(type);

		if (!allData.isValid())
			return Result::fail("Error parsing hxi file");

		jassert(allData.isValid() && allData.getType() == ExpansionIds::FullData);

		auto networkData = allData.getChildWithName("Networks");

		if (networkData.isValid())
		{
			MemoryBlock nmb;
			nmb.fromBase64Encoding(networkData[ExpansionIds::Data].toString());
			zstd::ZDefaultCompressor comp;
			comp.expand(nmb, networks);
		}

		data = new Data(getRootFolder(), allData.getChildWithName(ExpansionIds::ExpansionInfo).createCopy(), getMainController());

		auto iconData = allData.getChildWithName(ExpansionIds::HeaderData).getChildWithName(ExpansionIds::Icon)[ExpansionIds::Data].toString();

		if(iconData.isNotEmpty())
			pool->getImagePool().setDataProvider(new PublicIconProvider(&pool->getImagePool(), iconData));

		fullyLoaded = false;

		getMainController()->getExpansionHandler().addListener(this);

		checkSubDirectories();

		return Result::ok();
	}

	return Expansion::initialise();
}


juce::ValueTree FullInstrumentExpansion::getValueTreeFromFile(Expansion::ExpansionType type)
{
	auto hxiFile = Helpers::getExpansionInfoFile(getRootFolder(), type);

	FileInputStream fis(hxiFile);

	if (fis.readByte() == '<')
	{
		auto xml = XmlDocument::parse(hxiFile);

		if (xml == nullptr)
			return ValueTree();

		return ValueTree::fromXml(*xml);
	}
	else
	{
		fis.setPosition(0);
		return ValueTree::readFromStream(fis);
	}
}

Result FullInstrumentExpansion::lazyLoad()
{
	auto allData = getValueTreeFromFile(getExpansionType());

	if (!allData.isValid())
		return Result::fail("Can't parse ValueTree");

	auto presetData = allData.getChildWithName(ExpansionIds::Preset)[ExpansionIds::Data].toString();

	auto fontData = allData.getChildWithName(ExpansionIds::HeaderData).getChildWithName(ExpansionIds::Fonts);

	if (fontData.isValid())
	{
		zstd::ZDefaultCompressor d;
		ValueTree realFontData;
		auto fontAsBase64 = fontData[ExpansionIds::Data].toString();
		MemoryBlock mb;
		mb.fromBase64Encoding(fontAsBase64);

		d.expand(mb, realFontData);

		getMainController()->restoreCustomFontValueTree(realFontData);
	}

	ScopedPointer<BlowFish> bf = createBlowfish();

	MemoryBlock mb;
	mb.fromBase64Encoding(presetData);

	bf->decrypt(mb);

	zstd::ZCompressor<hise::PresetDictionaryProvider> comp;

	comp.expand(mb, presetToLoad);

	auto scriptTree = allData.getChildWithName(ExpansionIds::Scripts);

	if (presetToLoad.isValid())
	{
		auto bfCopy = bf.get();

		ScriptingApi::Content::Helpers::callRecursive(presetToLoad, [scriptTree, bfCopy](ValueTree& v)
		{
			if (v.hasProperty(ExpansionIds::Script))
			{
				auto hashToUse = (int)v[ExpansionIds::Script];
				auto scriptToUse = scriptTree.getChildWithProperty(ExpansionIds::Hash, hashToUse)[ExpansionIds::Data].toString();
				MemoryBlock mb;
				mb.fromBase64Encoding(scriptToUse);
				bfCopy->decrypt(mb);

				zstd::ZCompressor<hise::JavascriptDictionaryProvider> comp;
				String s;
				comp.expand(mb, s);
				v.setProperty(ExpansionIds::Script, s, nullptr);
			}

			return true;
		});
	}

	bf = nullptr;

	pool->getImagePool().setDataProvider(new PoolBase::DataProvider(&pool->getImagePool()));

	auto r = initialiseFromValueTree(allData);

	auto webResources = allData.getChildWithName("WebViewResources");

	if (webResources.isValid())
	{
		getMainController()->restoreWebResources(webResources);
	}

	if (r.wasOk())
	{
		fullyLoaded = true;
	}

	return r;
}

juce::Result ScriptEncryptedExpansion::returnFail(const String& errorMessage)
{
	auto& h = getMainController()->getExpansionHandler();
	h.setErrorMessage(errorMessage, false);
	return Result::fail(errorMessage);
}

FullInstrumentExpansion::DefaultHandler::DefaultHandler(MainController* mc, ValueTree t):
	ControlledObject(mc),
	defaultPreset(t),
	defaultIsLoaded(true)
{
	getMainController()->getExpansionHandler().addListener(this);
}

FullInstrumentExpansion::DefaultHandler::~DefaultHandler()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

void FullInstrumentExpansion::DefaultHandler::expansionPackLoaded(Expansion* e)
{
	if (e != nullptr)
	{
		defaultIsLoaded = false;
	}
	else
	{
		if (!defaultIsLoaded)
		{
			auto copy = defaultPreset.createCopy();

			getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), [copy, this](Processor* p)
			{
				defaultIsLoaded = true;
				p->getMainController()->loadPresetFromValueTree(copy);

				return SafeFunctionCall::OK;
			}, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		}
	}
}

FullInstrumentExpansion::FullInstrumentExpansion(MainController* mc, const File& f):
	ScriptEncryptedExpansion(mc, f)
{

}

FullInstrumentExpansion::~FullInstrumentExpansion()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

void FullInstrumentExpansion::setIsProjectExporter()
{
	isProjectExport = true;
}

ValueTree FullInstrumentExpansion::getEmbeddedNetwork(const String& id)
{
	return networks.getChildWithProperty("ID", id);
}


Result FullInstrumentExpansion::encodeExpansion()
{
	ValueTree allData(ExpansionIds::FullData);

	auto& h = getMainController()->getExpansionHandler();
	auto key = h.getEncryptionKey();

	auto printStats = [&h](const String& name, int number)
	{
		String m;
		m << String(number) << " " + name << ((number != 1) ? "s" : "") << " found.";
		h.setErrorMessage(m, false);
	};

	if (key.isEmpty())
	{
		return returnFail("The encryption key has not been set");
	}

	auto hxiFile = Expansion::Helpers::getExpansionInfoFile(getRootFolder(), Expansion::Intermediate);

	auto metadata = data->v.createCopy();
	metadata.setProperty(ExpansionIds::Hash, key.hashCode64(), nullptr);

	allData.addChild(metadata, -1, nullptr);

	{
		h.setErrorMessage("Encoding Fonts and Icons", false);

		ValueTree hd(ExpansionIds::HeaderData);

		{
			ValueTree fonts(ExpansionIds::Fonts);

			zstd::ZDefaultCompressor d;
			MemoryBlock fontData;
			auto fTree = getMainController()->exportCustomFontsAsValueTree();
			d.compress(fTree, fontData);
			fonts.setProperty(ExpansionIds::Data, fontData.toBase64Encoding(), nullptr);

			hd.addChild(fonts, -1, nullptr);

			printStats("font", fTree.getNumChildren());
		}

		PoolReference icon(getMainController(), String(isProjectExport ? "{PROJECT_FOLDER}" : getWildcard()) + "Icon.png", FileHandlerBase::Images);

		if (icon.getFile().existsAsFile())
		{
			MemoryBlock mb;
			icon.getFile().loadFileAsData(mb);
			ValueTree iconData(ExpansionIds::Icon);
			iconData.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

			hd.addChild(iconData, -1, nullptr);
		}

		allData.addChild(hd, -1, nullptr);
	}


	h.setErrorMessage("Collecting scripts", false);

	ScopedPointer<BlowFish> bf = createBlowfish();
	ValueTree scripts(ExpansionIds::Scripts);
	Processor::Iterator<JavascriptProcessor> iter(getMainController()->getMainSynthChain());

	while (auto jp = iter.getNextProcessor())
	{
		auto s = jp->collectScript(true);
		auto idHash = dynamic_cast<Processor*>(jp)->getId().hashCode();

		zstd::ZCompressor<hise::JavascriptDictionaryProvider> comp;
		MemoryBlock mb;
		comp.compress(s, mb);
		bf->encrypt(mb);

		ValueTree c(ExpansionIds::Script);
		c.setProperty(ExpansionIds::Hash, idHash, nullptr);
		c.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);

		scripts.addChild(c, -1, nullptr);
	}

	allData.addChild(scripts, -1, nullptr);

	printStats("script", scripts.getNumChildren());
	

#if USE_BACKEND
	{
		h.setErrorMessage("Embedding networks", false);
		networks = BackendDllManager::exportAllNetworks(getMainController(), false);
		zstd::ZDefaultCompressor d;
		MemoryBlock networkData;
		d.compress(networks, networkData);
		ValueTree b64n("Networks");
		b64n.setProperty(ExpansionIds::Data, networkData.toBase64Encoding(), nullptr);
		allData.addChild(b64n, -1, nullptr);

		printStats("network", networks.getNumChildren());
	}
#endif

	h.setErrorMessage("Embedding currently loaded project", false);

	{
		auto mTree = getMainController()->getMainSynthChain()->exportAsValueTree();

		ScriptingApi::Content::Helpers::callRecursive(mTree, [scripts](ValueTree& v)
		{
			// Replace the scripts with their hash
			if (v.hasProperty(ExpansionIds::Script))
			{
				auto hash = v["ID"].toString().hashCode();
				jassert(scripts.getChildWithProperty(ExpansionIds::Hash, hash).isValid());
				v.setProperty(ExpansionIds::Script, hash, nullptr);
			}

			return true;
		});


		zstd::ZCompressor<hise::PresetDictionaryProvider> comp;
		MemoryBlock mb;
		comp.compress(mTree, mb);
		ValueTree preset(ExpansionIds::Preset);
		bf->encrypt(mb);

		preset.setProperty(ExpansionIds::Data, mb.toBase64Encoding(), nullptr);
		allData.addChild(preset, -1, nullptr);
	}

	encodePoolAndUserPresets(allData, isProjectExport);
	
	allData.addChild(getMainController()->exportWebViewResources(), -1, nullptr);

	h.setErrorMessage("Writing file", false);

#if HISE_USE_XML_FOR_HXI
	ScopedPointer<XmlElement> xml = allData.createXml();
	hxiFile.replaceWithText(xml->createDocument(""));
#else
	hxiFile.deleteFile();
	FileOutputStream fos(hxiFile);
	allData.writeToStream(fos);
#endif

	h.setErrorMessage("Done", false);

	if(!isProjectExport)
		h.forceReinitialisation();

	return Result::ok();
}

ExpansionEncodingWindow::ExpansionEncodingWindow(MainController* mc, Expansion* eToEncode, bool isProjectExport, bool isRhapsody_) :
	DialogWindowWithBackgroundThread(isProjectExport ? "Export HISE project" : "Encode Expansion"),
	ControlledObject(mc),
	e(eToEncode),
	exportMode(isRhapsody_ ? ExportMode::Rhapsody : ExportMode::HXI),
	encodeResult(Result::ok()),
	projectExport(isProjectExport)
{
	if (isProjectExport)
	{
#if USE_BACKEND
		auto& h = GET_PROJECT_HANDLER(mc->getMainSynthChain());

		addComboBox("rhapsody", { "HXI Full Instrument Expansion", "Rhapsody Player Library", "HISE Project Archive" }, "Export Format");
		getComboBoxComponent("rhapsody")->setSelectedItemIndex((int)exportMode, dontSendNotification);

		if (mc->getExpansionHandler().getEncryptionKey().isEmpty())
		{
			auto k = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject().getSetting(HiseSettings::Project::EncryptionKey).toString();

			if (k.isNotEmpty())
				mc->getExpansionHandler().setEncryptionKey(k);
			else
				encodeResult = Result::fail("You have to specify an encryption key in order to encode the project as full expansion");
		}

		auto existingIntermediate = Expansion::Helpers::getExpansionInfoFile(h.getWorkDirectory(), Expansion::Intermediate);

		if (existingIntermediate.existsAsFile())
			existingIntermediate.deleteFile();
#endif
	}
	else
	{
		StringArray expList;
		auto l = getMainController()->getExpansionHandler().getListOfAvailableExpansions();

		for (auto v : *l.getArray())
			expList.add(v.toString());

		addComboBox("expansion", expList, "Expansion to encode");

		getComboBoxComponent("expansion")->addItem("All expansions", AllExpansionId);

		if (e != nullptr)
			getComboBoxComponent("expansion")->setText(e->getProperty(ExpansionIds::Name), dontSendNotification);
	}

	getMainController()->getExpansionHandler().addListener(this);
	addBasicComponents(true);

	showStatusMessage("Press OK to encode the expansion");
}

ExpansionEncodingWindow::~ExpansionEncodingWindow()
{
	getMainController()->getExpansionHandler().removeListener(this);
}

juce::Result ExpansionEncodingWindow::performChecks()
{
#if USE_BACKEND

	exportMode = (ExportMode)getComboBoxComponent("rhapsody")->getSelectedItemIndex();

	if (exportMode == ExportMode::HXI)
		return Result::ok();

	if (getMainController()->getExpansionHandler().getEncryptionKey() != "1234")
	{
		return Result::fail("The encryption key must be `1234` for the open export to work");
	}

	// check that there is an icon image

	if (exportMode == ExportMode::Rhapsody)
	{
		auto thumb = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Images).getChildFile("Icon.png");
		auto hasIcon = thumb.existsAsFile();

		if (!hasIcon)
			return Result::fail("The project needs a Icon.png image (with the dimensions 300x50)");

		auto dllManager = dynamic_cast<BackendProcessor*>(getMainController())->dllManager;

		auto compileNetworks = dllManager->getNetworkFiles(getMainController(), false);

		if (!compileNetworks.isEmpty())
			return Result::fail("The project must not use compiled DSP Networks");

		auto userPresetFolder = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::UserPresets);

		auto presetList = userPresetFolder.findChildFiles(File::findFiles, true, "*.preset");

		if (presetList[0].getParentDirectory().getParentDirectory().getParentDirectory() != userPresetFolder)
			return Result::fail("The project needs to have at least one user preset and must use the default three level folder hierarchy (Bank/Category/Preset)");
	}

#endif

	return Result::ok();
}

void ExpansionEncodingWindow::run()
{
#if USE_BACKEND
	if (projectExport)
	{
		if (encodeResult.failed())
			return;

		encodeResult = performChecks();

		if (encodeResult.failed())
			return;

		auto& h = GET_PROJECT_HANDLER(getMainController()->getMainSynthChain());
		auto f = Expansion::Helpers::getExpansionInfoFile(h.getWorkDirectory(), Expansion::FileBased);

		ValueTree mData(ExpansionIds::ExpansionInfo);

		auto projectName = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();

		mData.setProperty(ExpansionIds::Name, projectName, nullptr);
		mData.setProperty(ExpansionIds::Version, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Version), nullptr);
		mData.setProperty(ExpansionIds::Company, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::User::Company), nullptr);
		mData.setProperty(ExpansionIds::CompanyURL, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::User::CompanyURL), nullptr);
		mData.setProperty(ExpansionIds::Description, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::ExpansionSettings::Description), nullptr);
		mData.setProperty(ExpansionIds::Tags, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::ExpansionSettings::Tags), nullptr);
		mData.setProperty(ExpansionIds::UUID, GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::ExpansionSettings::UUID), nullptr);
		mData.setProperty(ExpansionIds::HiseVersion, PresetHandler::getVersionString(), nullptr);

		if (exportMode == ExportMode::HiseProject)
		{
			auto projectFile = h.getRootFolder().getChildFile("project_info.xml");
			auto userFile = h.getRootFolder().getChildFile("user_info.xml");

			auto projectXML = XmlDocument::parse(projectFile);
			auto userXML = XmlDocument::parse(userFile);

			if (projectXML != nullptr)
				mData.addChild(ValueTree::fromXml(*projectXML), -1, nullptr);

			if (userXML != nullptr)
				mData.addChild(ValueTree::fromXml(*userXML), -1, nullptr);
		}

        String prevContent;
        
        if(f.existsAsFile())
            prevContent = f.loadFileAsString();
        
        
		auto xml = mData.createXml();
		f.replaceWithText(xml->createDocument(""));
		ScopedPointer<FullInstrumentExpansion> e = new FullInstrumentExpansion(getMainController(), h.getWorkDirectory());
		e->initialise();
		e->setIsProjectExporter();
		encodeResult = e->encodeExpansion();

		// This file is used by the FullInstrument Expansion so
        // we have to restore it to not lose the original file data
        if(prevContent.isNotEmpty())
            f.replaceWithText(prevContent);
        else
            f.deleteFile();

		if (exportMode > ExportMode::HXI)
		{
			auto hxiFile = Expansion::Helpers::getExpansionInfoFile(h.getWorkDirectory(), Expansion::Intermediate);
			jassert(hxiFile.existsAsFile());

			if (exportMode == ExportMode::Rhapsody)
			{
				ZipFile::Builder b;

				b.addFile(hxiFile, 0);

				String zipName;
				zipName << mData[ExpansionIds::Name].toString().toLowerCase().replaceCharacter(' ', '_');
				zipName << "_data";
				zipName << "_" << mData[ExpansionIds::Version].toString().replaceCharacter('.', '_');

				auto zipFile = hxiFile.getSiblingFile(zipName + ".lwz");
				zipFile.deleteFile();
				FileOutputStream fos(zipFile);

				auto ok = b.writeToStream(fos, &getProgressCounter());

				if (ok)
					hxiFile.deleteFile();

				rhapsodyOutput = zipFile;

				jassert(ok);
			}
			else
			{
				rhapsodyOutput = hxiFile.getParentDirectory().getChildFile(projectName).withFileExtension(".hiseproject");

				auto sampleArchives = hxiFile.getParentDirectory().findChildFiles(File::findFiles, false, "*.hr1");

				rhapsodyOutput.deleteFile();

				FileOutputStream fos(rhapsodyOutput);

				FileInputStream hxiInput(hxiFile);

				fos.writeInt64(hxiInput.getTotalLength());
				fos.writeFromInputStream(hxiInput, hxiInput.getTotalLength());

				for (auto s : sampleArchives)
				{
					FileInputStream si(s);

					fos.writeInt64(si.getTotalLength());
					fos.writeFromInputStream(si, si.getTotalLength());
				}

				fos.flush();
			}
		}
	}
	else
	{
		if (getComboBoxComponent("expansion")->getSelectedId() == AllExpansionId)
		{
			auto& h = getMainController()->getExpansionHandler();

			for (int i = 0; i < h.getNumExpansions(); i++)
			{
				if (auto e = h.getExpansion(i))
				{
					//showStatusMessage("Encoding " + e->getProperty(ExpansionIds::Name));
					setProgress((double)i / (double)h.getNumExpansions());

					encodeResult = e->encodeExpansion();

					if (encodeResult.failed())
						break;
				}
			}

			return;
		}

		if (e == nullptr)
		{
			auto n = getComboBoxComponent("expansion")->getText();
			e = getMainController()->getExpansionHandler().getExpansionFromName(n);
		}

		if (e != nullptr)
			encodeResult = e->encodeExpansion();
		else
			encodeResult = Result::fail("No expansion to encode");
	}
#endif
}

void ExpansionEncodingWindow::threadFinished()
{
#if USE_BACKEND
	if (encodeResult.wasOk())
	{
		if (projectExport && rhapsodyOutput.existsAsFile())
		{
            if(!CompileExporter::isExportingFromCommandLine())
                rhapsodyOutput.revealToUser();
		}

		if(!projectExport)
			PresetHandler::showMessageWindow("Expansion encoded", "The expansion was encoded successfully");
	}
	else
		PresetHandler::showMessageWindow("Expansion encoding failed", encodeResult.getErrorMessage(), PresetHandler::IconType::Error);
#endif
}

String ScriptUnlocker::getProductID()
{
	String s;

#if USE_BACKEND
	s << GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();
	s << " ";
	s << GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Version).toString();
#else
	s << String(FrontendHandler::getProjectName());
	s << " "; 
	s << FrontendHandler::getVersionString();
#endif
	
	return s;
}

bool ScriptUnlocker::doesProductIDMatch(const String& returnedIDFromServer)
{
	if (currentObject != nullptr && currentObject->pcheck)
	{
		var args(returnedIDFromServer);
		var rv(false);

		auto s = currentObject->pcheck.callSync(&args, 1, &rv);

		if (s.wasOk())
			return rv;
	}

	// By default we don't want the product version mismatch to cause a license fail, so
	// we trim it. If you need this behaviour, define a callback that uses the branch above...
	auto realId = getProductID().upToLastOccurrenceOf(" ", false, false).trim();
	auto expectedId = returnedIDFromServer.upToLastOccurrenceOf(" ", false, false).trim();

	return realId == expectedId;
}

#if USE_BACKEND
juce::RSAKey ScriptUnlocker::getPublicKey()
{
	return juce::RSAKey(getMainController()->getSampleManager().getProjectHandler().getPublicKey());
}
#elif !USE_COPY_PROTECTION || !USE_SCRIPT_COPY_PROTECTION
juce::RSAKey ScriptUnlocker::getPublicKey()
{
    return RSAKey();
}
#endif

void ScriptUnlocker::saveState(const String&)
{
	jassertfalse;
}

String ScriptUnlocker::getState()
{
	return "";
}

String ScriptUnlocker::getWebsiteName()
{
#if USE_BACKEND
	jassertfalse;
	return "";
#else
	return FrontendHandler::getCompanyWebsiteName();
#endif
}

juce::URL ScriptUnlocker::getServerAuthenticationURL()
{
	jassertfalse;
	return {};
}

String ScriptUnlocker::readReplyFromWebserver(const String& email, const String& password)
{
	jassertfalse;
	return {};
}

void ScriptUnlocker::checkMuseHub()
{
#if USE_BACKEND
	WeakReference<ScriptUnlocker> safeRef(this);

	Timer::callAfterDelay(2000, [safeRef]()
	{
		auto ok = var(Random::getSystemRandom().nextFloat() > 0.5f);

#if JUCE_ALLOW_EXTERNAL_UNLOCK
		if(ok)
			safeRef->unlockExternal();
#endif

		if(safeRef != nullptr && safeRef->currentObject != nullptr)
			safeRef->currentObject->mcheck.call1(ok);
	});

#elif HISE_INCLUDE_MUSEHUB
	m = checkMuseHubInternal();
#endif
}

juce::var ScriptUnlocker::loadKeyFile()
{
	if (isUnlocked())
		return var(1);

	auto keyFile = getLicenseKeyFile();

	if (keyFile.existsAsFile())
	{
		String keyData = keyFile.loadFileAsString();

		StringArray keyLines = StringArray::fromLines(keyData);

		for (const auto& k : keyLines)
		{
			if (k.startsWith("Machine numbers"))
			{
				registeredMachineId = k.fromFirstOccurrenceOf(": ", false, false).trim();
				break;
			}
		}

		if (this->applyKeyFile(keyData))
		{
#if USE_FRONTEND
			dynamic_cast<FrontendProcessor*>(getMainController())->loadSamplesAfterRegistration(true);
#endif

			return var(1);
		}
	}

	return var(0);
}

juce::File ScriptUnlocker::getLicenseKeyFile()
{
#if USE_BACKEND

	auto c = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::User::Company).toString();
	auto p = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();

	return ProjectHandler::getAppDataRoot(getMainController()).getChildFile(c).getChildFile(p).getChildFile(p).withFileExtension(FrontendHandler::getLicenseKeyExtension());
#else
	return FrontendHandler::getLicenseKey();
#endif
	
}

struct BeatportManager::Wrapper
{
	API_METHOD_WRAPPER_0(BeatportManager, validate);
	API_METHOD_WRAPPER_0(BeatportManager, isBeatportAccess);
	API_VOID_METHOD_WRAPPER_1(BeatportManager, setProductId);
};

BeatportManager::BeatportManager(ProcessorWithScriptingContent* sp):
	ConstScriptingObject(sp, 0)
{
#if HISE_INCLUDE_BEATPORT
	pimpl = new Pimpl();
#endif

	ADD_API_METHOD_0(validate);
	ADD_API_METHOD_0(isBeatportAccess);
	ADD_API_METHOD_1(setProductId);
}

BeatportManager::~BeatportManager()
{
#if HISE_INCLUDE_BEATPORT
	pimpl = nullptr;
#endif
}

void BeatportManager::setProductId(const String& productId)
{
#if HISE_INCLUDE_BEATPORT
	pimpl->setProductId(productId);
#else
	debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "Product ID set to " + productId);
#endif
}

var BeatportManager::validate()
{
	auto t = Time::getMillisecondCounter();

	var obj;

#if HISE_INCLUDE_BEATPORT
	obj = pimpl->validate();
#else

	// simulate waiting...
	Thread::getCurrentThread()->wait(1500);

	auto responseFile = getBeatportProjectFolder(getScriptProcessor()->getMainController_()).getChildFile("validate_response.json");

	if(!responseFile.existsAsFile())
		reportScriptError("You need to create a validate_response.json file in the beatport folder that simulates a response");

	
	auto ok = JSON::parse(responseFile.loadFileAsString(), obj);

	if(ok.failed())
		reportScriptError("Error at loading dummy JSON: " + ok.getErrorMessage());
	
#endif

	auto now = Time::getMillisecondCounter();

	dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout((int)(now - t));

	return obj;
}

bool BeatportManager::isBeatportAccess()
{
#if HISE_INCLUDE_BEATPORT
	return pimpl->isBeatportAccess();
#else
	auto t = Time::getMillisecondCounter();

	Thread::getCurrentThread()->wait(500);
	auto responseFile = getBeatportProjectFolder(getScriptProcessor()->getMainController_()).getChildFile("validate_response.json");

	auto now = Time::getMillisecondCounter();

	dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine()->extendTimeout((int)(now - t));

	return responseFile.existsAsFile();
#endif
}


struct ScriptUnlocker::RefObject::Wrapper
{
	API_METHOD_WRAPPER_0(RefObject, isUnlocked);
	API_METHOD_WRAPPER_0(RefObject, canExpire);
	API_METHOD_WRAPPER_1(RefObject, checkExpirationData);
	API_METHOD_WRAPPER_0(RefObject, loadKeyFile);
	API_VOID_METHOD_WRAPPER_1(RefObject, setProductCheckFunction);
	API_METHOD_WRAPPER_1(RefObject, writeKeyFile);
	API_METHOD_WRAPPER_0(RefObject, getUserEmail);
	API_METHOD_WRAPPER_0(RefObject, getRegisteredMachineId);
	API_METHOD_WRAPPER_1(RefObject, isValidKeyFile);
    API_METHOD_WRAPPER_0(RefObject, keyFileExists);
	API_METHOD_WRAPPER_0(RefObject, getLicenseKeyFile);
	API_METHOD_WRAPPER_1(RefObject, contains);
	API_VOID_METHOD_WRAPPER_1(RefObject, checkMuseHub);
};

ScriptUnlocker::RefObject::RefObject(ProcessorWithScriptingContent* p) :
	ConstScriptingObject(p, 0),
#if USE_BACKEND || USE_COPY_PROTECTION
	unlocker(dynamic_cast<ScriptUnlocker*>(p->getMainController_()->getLicenseUnlocker())),
#endif
	pcheck(p, nullptr, var(), 1),
	mcheck(p, nullptr, var(), 1)
{
	if (unlocker->getLicenseKeyFile().existsAsFile())
	{
		unlocker->loadKeyFile();
	}
	
	unlocker->currentObject = this;

	ADD_API_METHOD_0(isUnlocked);
	ADD_API_METHOD_0(loadKeyFile);
	ADD_API_METHOD_1(setProductCheckFunction);
	ADD_API_METHOD_1(writeKeyFile);
	ADD_API_METHOD_0(getUserEmail);
	ADD_API_METHOD_0(getRegisteredMachineId);
	ADD_API_METHOD_1(isValidKeyFile);
	ADD_API_METHOD_0(canExpire);
	ADD_API_METHOD_1(checkExpirationData);
    ADD_API_METHOD_0(keyFileExists);
	ADD_API_METHOD_0(getLicenseKeyFile);
	ADD_API_METHOD_1(contains);
	ADD_API_METHOD_1(checkMuseHub);
}

ScriptUnlocker::RefObject::~RefObject()
{
	if (unlocker != nullptr && unlocker->currentObject == this)
		unlocker->currentObject = nullptr;
}

juce::var ScriptUnlocker::RefObject::isUnlocked() const
{
	return unlocker != nullptr ? unlocker->isUnlocked() : var(0);
}

juce::var ScriptUnlocker::RefObject::canExpire() const
{
	return unlocker != nullptr ? var(unlocker->getExpiryTime() != juce::Time(0)) : var(false);
}

juce::var ScriptUnlocker::RefObject::checkExpirationData(const String& encodedTimeString)
{
	if (unlocker != nullptr)
	{
		if (encodedTimeString.startsWith("0x"))
		{
			BigInteger bi;

			bi.parseString(encodedTimeString.substring(2), 16);
			unlocker->getPublicKey().applyToValue(bi);

			auto timeString = bi.toMemoryBlock().toString();

			auto time = Time::fromISO8601(timeString);

			auto ok = unlocker->unlockWithTime(time);

			auto delta = unlocker->getExpiryTime() - time;

			if (ok)
			{
#if USE_FRONTEND
				dynamic_cast<FrontendProcessor*>(getScriptProcessor()->getMainController_())->loadSamplesAfterRegistration(true);
#endif

				return var(roundToInt(delta.inDays()));
			}
				
			else
				return var(false);

		}

        return var("encodedTimeString data is corrupt");
	}
	else
	{
		return var("No unlocker");
	}
}

void ScriptUnlocker::RefObject::checkMuseHub(var resultCallback)
{
	if(unlocker.get() != nullptr)
	{
		mcheck = WeakCallbackHolder(getScriptProcessor(), this, resultCallback, 1);
		unlocker->checkMuseHub();
	}
}

void ScriptUnlocker::RefObject::setProductCheckFunction(var f)
{
	pcheck = WeakCallbackHolder(getScriptProcessor(), this, f, 1);
	pcheck.incRefCount();
	pcheck.setThisObject(this);
}

juce::var ScriptUnlocker::RefObject::loadKeyFile()
{
	return unlocker->loadKeyFile();
}

bool ScriptUnlocker::RefObject::keyFileExists() const
{
    return unlocker->getLicenseKeyFile().existsAsFile();
}

juce::var ScriptUnlocker::RefObject::writeKeyFile(const String& keyData)
{
	unlocker->getLicenseKeyFile().getParentDirectory().createDirectory();

	auto ok = unlocker->getLicenseKeyFile().replaceWithText(keyData);

	if (ok)
		return loadKeyFile();
	
	return {};
}

bool ScriptUnlocker::RefObject::isValidKeyFile(var possibleKeyData)
{
	if (possibleKeyData.isString())
	{
		return possibleKeyData.toString().startsWith("Keyfile for ");
	}

	return false;
}

var ScriptUnlocker::RefObject::getLicenseKeyFile()
{
	auto lf = unlocker->getLicenseKeyFile();

	return var(new ScriptingObjects::ScriptFile(getScriptProcessor(), lf));
}

String ScriptUnlocker::RefObject::getUserEmail() const
{
	return unlocker->getUserEmail();
}

String ScriptUnlocker::RefObject::getRegisteredMachineId()
{
	return unlocker->registeredMachineId;
}

bool ScriptUnlocker::RefObject::contains(String otherString)
{
	if(unlocker.get() != nullptr)
		return unlocker->contains(otherString);

	return true;
}
} // namespace hise
