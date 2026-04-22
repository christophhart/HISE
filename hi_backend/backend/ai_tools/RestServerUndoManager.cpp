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

namespace hise {
using namespace juce;

RestServerUndoManager::ActionBase::Error::Error(const ActionBase& om)
{
	message = om.getDescription();
	ok = false;
}



RestServerUndoManager::ActionBase::Error RestServerUndoManager::ActionBase::Error::withHint(const String& hint) const
{
	auto copy = *this;
	copy.message << hint;
	return copy;
}

RestServerUndoManager::ActionBase::Error RestServerUndoManager::ActionBase::Error::withError(const String& error) const
{
	auto copy = *this;
	copy.ok = false;
	copy.message << error;
	return copy;
}

RestServerUndoManager::ActionBase::Error RestServerUndoManager::Factory::prevalidate(Domain d, const var& obj) const
{
	if (!obj.isObject())
		return ActionBase::Error().withError("must be an object");

	auto op = obj[RestApiIds::op].toString();

	if (op.isEmpty())
		return ActionBase::Error().withError("op field is required");

	StringArray supportedOps;

	if (registeredFunctions.find(d) != registeredFunctions.end())
	{
		auto& r = registeredFunctions.at(d);

		if (r.find(op) != r.end())
			return r.at(op).first(const_cast<MainController*>(getMainController()), obj);

		for (const auto& f : r)
			supportedOps.add(f.first);
	}

	return ActionBase::Error()
		.withError("unsupported op at domain " + getDomains()[(int)d] + ": " + op)
		.withHint("supported ops: " + supportedOps.joinIntoString(","));
}

RestServerUndoManager::ActionBase::Ptr RestServerUndoManager::Factory::create(Domain d, const var& obj) const
{
	if (registeredFunctions.find(d) != registeredFunctions.end())
	{
		auto& r = registeredFunctions.at(d);

		auto op = obj[RestApiIds::op].toString();

		if (r.find(op) != r.end())
		{
			auto ad = r.at(op).second(const_cast<MainController*>(getMainController()), obj);
			ad->d = d;
			return ad;
		}
	}

	return nullptr;
}

void RestServerUndoManager::Factory::registerAllFunctions()
{
	registerCreatorFunctionT<rest_undo::builder::add>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::remove>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::clone>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::set_attributes>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::set_id>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::set_bypassed>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::set_effect>(Domain::Builder);
	registerCreatorFunctionT<rest_undo::builder::set_complex_data>(Domain::Builder);

	registerCreatorFunctionT<rest_undo::ui::add>(Domain::UI);
	registerCreatorFunctionT<rest_undo::ui::remove>(Domain::UI);
	registerCreatorFunctionT<rest_undo::ui::set>(Domain::UI);
	registerCreatorFunctionT<rest_undo::ui::move>(Domain::UI);
	registerCreatorFunctionT<rest_undo::ui::rename>(Domain::UI);

	registerCreatorFunctionT<rest_undo::dsp::add>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::remove>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::move>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::connect>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::disconnect>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::set>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::bypass>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::create_parameter>(Domain::DSP);
	registerCreatorFunctionT<rest_undo::dsp::clear>(Domain::DSP);
}

hise::RestServerUndoManager::Instance* RestServerUndoManager::Instance::getOrCreate(MainController* mc, RestHelpers::ApiRoute endpoint)
{
	auto bp = dynamic_cast<BackendProcessor*>(mc);
	auto um = dynamic_cast<Instance*>(bp->getOrCreateRestServerBuildUndoManager());
	um->setCurrentEndpoint(endpoint);
	um->setValidationErrors({});
	return um;
}

void RestServerUndoManager::Instance::flushUI(Processor* p)
{
	SafeAsyncCall::callAsyncIfNotOnMessageThread<Processor>(*p, [](Processor& p)
	{
		p.sendRebuildMessage(true);
		p.getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(&p,
			MainController::ProcessorChangeHandler::EventType::RebuildModuleList,
			false);
	});
}

RestServerUndoManager::Instance::Instance(MainController* mc) :
	ControlledObject(mc),
	f(mc)
{
	clearUndoHistory();
}

hise::RestServer::Response RestServerUndoManager::Instance::getResponse(const std::vector<CallStack>& callstacks, var r)
{
	DynamicObject::Ptr result = new DynamicObject();
	result->setProperty(RestApiIds::success, callstacks.empty());

	// Flatten diff object fields onto the top-level response
	if (auto* diffObj = r.getDynamicObject())
	{
		for (const auto& prop : diffObj->getProperties())
			result->setProperty(prop.name, prop.value);
	}

	result->setProperty(RestApiIds::logs, Array<var>());
	result->setProperty(RestApiIds::errors, CallStack::toJSONList(callstacks));
	return RestServer::Response::ok(var(result.get()));
}

bool RestServerUndoManager::Instance::killVoicesAndPerform(AsyncRequest::Ptr req, ActionBase::Ptr a, bool shouldUndo /*= false*/)
{
	if (!a->needsKillVoice())
	{
		try
		{
			a->planValidation = nullptr;

			if (shouldUndo)
				a->undo();
			else
				a->perform();
		}
		catch (ActionBase::Error& e)
		{
			callStack.push_back(CallStack(e.getErrorMessage())
				.withGroup(getCurrentGroupId())
				.withPhase(CallStack::Phase::Runtime)
				.withEndpoint(currentEndpoint));
		}
		
		req->complete(getResponse(callStack, getDiffJSON(true, true)));

		if ((a->getRebuildLevel(Domain::Builder, shouldUndo) & RebuildLevel::UpdateUI) != 0)
			flushUI(getMainController()->getMainSynthChain());

		if ((a->getRebuildLevel(Domain::Undefined, shouldUndo) & RebuildLevel::ParameterSlots) != 0)
		{
			auto moduleId = a->getModuleId();

			if(auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), moduleId))
				p->updateParameterSlots(-1);
		}
			

		if ((a->getRebuildLevel(Domain::UI, shouldUndo) & RebuildLevel::Recompile) != 0)
		{
			auto uiModuleId = a->getModuleId();
			if (uiModuleId.isNotEmpty())
			{
				auto uiJp = dynamic_cast<JavascriptProcessor*>(
					ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), uiModuleId));
				if (uiJp)
				{
					auto content = dynamic_cast<ProcessorWithScriptingContent*>(uiJp)->getScriptingContent();

					// Signal the UI to tear down JUCE component wrappers before recompiling.
					// This prevents the message thread from accessing stale component state
					// while the scripting thread rebuilds the component tree.
					std::atomic<bool> uiCleared(false);

					MessageManager::callAsync([content, &uiCleared]()
					{
						content->setIsRebuilding(true);
						content->resetContentProperties();
						uiCleared.store(true);
					});

					while (!uiCleared.load())
						Thread::sleep(50);

					getMainController()->getKillStateHandler().killVoicesAndCall(
						dynamic_cast<Processor*>(uiJp),
						[](Processor* p) {
							auto jp = dynamic_cast<JavascriptProcessor*>(p);
							auto c = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();

							jp->compileScript([c](const JavascriptProcessor::SnippetResult&)
							{
								c->setIsRebuilding(false);
							});

							return SafeFunctionCall::OK;
						},
						MainController::KillStateHandler::TargetThread::ScriptingThread
					);
				}
			}
		}

		return true;
	}

	getMainController()->getKillStateHandler().killVoicesAndCall(
		getMainController()->getMainSynthChain(),
		[a, shouldUndo, req, this](Processor* p)
		{
			try
			{
				a->planValidation = nullptr;

				if (shouldUndo)
					a->undo();
				else
					a->perform();
			}
			catch (ActionBase::Error& e)
			{
				callStack.push_back(CallStack(e.getErrorMessage())
					.withGroup(getCurrentGroupId())
					.withPhase(CallStack::Phase::Runtime)
					.withEndpoint(currentEndpoint));
			}

			req->complete(getResponse(callStack, getDiffJSON(true, true)));

			if ((a->getRebuildLevel(Domain::Builder, shouldUndo) & RebuildLevel::UpdateUI) != 0)
				flushUI(p);

			if ((a->getRebuildLevel(Domain::Undefined, shouldUndo) & RebuildLevel::ParameterSlots) != 0)
			{
				auto moduleId = a->getModuleId();

				if (auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), moduleId))
					p->updateParameterSlots(-1);
			}

			if ((a->getRebuildLevel(Domain::UI, shouldUndo) & RebuildLevel::Recompile) != 0)
			{
				auto uiModuleId = a->getModuleId();
				if (uiModuleId.isNotEmpty())
				{
					auto uiJp = dynamic_cast<JavascriptProcessor*>(
						ProcessorHelpers::getFirstProcessorWithName(p->getMainController()->getMainSynthChain(), uiModuleId));
					if (uiJp)
					{
						auto content = dynamic_cast<ProcessorWithScriptingContent*>(uiJp)->getScriptingContent();
	
						dynamic_cast<JavascriptProcessor*>(uiJp)->compileScript({});
					}
				}
			}

			return SafeFunctionCall::OK;
		},
		MainController::KillStateHandler::TargetThread::SampleLoadingThread
	);

	return false;
}

void RestServerUndoManager::Instance::performAction(AsyncRequest::Ptr req, ActionBase::Ptr l)
{
	if (auto cs = getCurrentStack())
	{
		if (!cs->actions.isEmpty() && cs->currentAction != cs->actions.getLast())
			cs->actions.removeRange(cs->actions.indexOf(cs->currentAction) + 1, INT_MAX);

		cs->actions.add(l);
		cs->currentAction = cs->actions.getLast();

		if (!isTestRun(cs))
			killVoicesAndPerform(req, l);
		else
			req->complete(getResponse({}, getDiffJSON(true, true)));
	}
}

void RestServerUndoManager::Instance::performAction(AsyncRequest::Ptr req, ActionBase::List list, const PostOpCallback& postOp, const String& name /*= {}*/)
{
	ActionBase::Ptr set = new Set(getMainController(), list, postOp, name);
	performAction(req, set);
}

void RestServerUndoManager::Instance::clearUndoHistory()
{
	rootActions = new StackItem();
	rootActions->name = "root";
	planStack.clear();
}

juce::ValueTree RestServerUndoManager::Instance::getValidationPlan() const
{
	if (auto p = getCurrentStack())
	{
		if (p->planValidationState != nullptr)
		{
			return p->planValidationState->moduleTree;
		}
	}

	return {};
}

bool RestServerUndoManager::Instance::undo(AsyncRequest::Ptr req)
{
	if (auto cs = getCurrentStack())
	{
		if (cs->currentAction != nullptr)
		{
			auto actionToUndo = cs->currentAction;
			auto idx = cs->actions.indexOf(cs->currentAction) - 1;

			if (idx >= 0)
				cs->currentAction = cs->actions[idx];
			else
				cs->currentAction = nullptr;

			if (!isTestRun(cs))
				killVoicesAndPerform(req, actionToUndo, true);
			else
				req->complete(getResponse({}, getDiffJSON(true, true)));

			return true;
		}
	}

	return false;
}

bool RestServerUndoManager::Instance::redo(AsyncRequest::Ptr req)
{
	if (auto cs = getCurrentStack())
	{
		if (cs->currentAction == nullptr && !cs->actions.isEmpty())
		{
			cs->currentAction = cs->actions.getFirst();

			if (!isTestRun(cs))
				killVoicesAndPerform(req, cs->currentAction);
			else
				req->complete(getResponse(callStack, getDiffJSON(true, true)));

			return true;
		}

		if (cs->currentAction != nullptr && cs->currentAction != cs->actions.getLast())
		{
			auto idx = cs->actions.indexOf(cs->currentAction) + 1;
			cs->currentAction = cs->actions[idx];

			if (!isTestRun(cs))
				killVoicesAndPerform(req, cs->currentAction);
			else
				req->complete(getResponse(callStack, getDiffJSON(true, true)));

			return true;
		}
	}

	return false;
}

void RestServerUndoManager::Instance::pushPlan(const String& name)
{
	auto ns = new StackItem();
	ns->name = name;
	ns->planValidationState = new PlanValidationState(getMainController());

	// Eagerly create UIValidationState from "Interface" processor
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), "Interface"));

	if (jp != nullptr)
		ns->uiValidationState = new UIValidationState(getMainController());

	// DspValidationState is created lazily from the first DSP action in the
	// group (see createAction) -- unlike UIValidationState which hardcodes
	// "Interface", DSP holders can have arbitrary module IDs, and a group may
	// not always touch DSP. Lazy creation picks the moduleId from the action.

	planStack.add(ns);
}

bool RestServerUndoManager::Instance::popPlan(AsyncRequest::Ptr req)
{
	if (planStack.isEmpty())
		return false;

	auto shouldCancel = (bool)req->getRequest().getJsonBody().getProperty(RestApiIds::cancel, false);

	auto lastPlan = planStack.removeAndReturn(planStack.size() - 1);

	if (!shouldCancel)
	{
		ActionBase::List activeActions;
		for (int i = 0; i <= lastPlan->actions.indexOf(lastPlan->currentAction); i++)
			activeActions.add(lastPlan->actions[i]);
		performAction(req, activeActions, {}, lastPlan->name);
	}

	return true;
}

// ============================================================================
// UIValidationState implementation
// ============================================================================

RestServerUndoManager::UIValidationState::UIValidationState(MainController* mc) :
	ControlledObject(mc),
	moduleId("Interface")
{
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), "Interface"));

	if (jp != nullptr)
	{
		auto content = dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();
		if (content != nullptr)
			contentTree = content->getContentProperties().createCopy();
	}
}

ValueTree RestServerUndoManager::UIValidationState::findComponentRecursive(const ValueTree& tree, const String& id)
{
	if (tree.getProperty("id").toString() == id)
		return tree;

	for (int i = 0; i < tree.getNumChildren(); i++)
	{
		auto result = findComponentRecursive(tree.getChild(i), id);
		if (result.isValid())
			return result;
	}

	return {};
}

ValueTree RestServerUndoManager::UIValidationState::findComponent(const String& id) const
{
	return findComponentRecursive(contentTree, id);
}

bool RestServerUndoManager::UIValidationState::componentExists(const String& id) const
{
	return findComponent(id).isValid();
}

bool RestServerUndoManager::UIValidationState::isDescendantOf(const String& child, const String& parent) const
{
	auto parentTree = findComponent(parent);
	auto childTree = findComponent(child);

	if (!parentTree.isValid() || !childTree.isValid())
		return false;

	return parentTree.isAChildOf(childTree);
}

StringArray RestServerUndoManager::UIValidationState::getAllComponentIds() const
{
	StringArray ids;

	valuetree::Helpers::forEach(contentTree, [&](ValueTree& v)
	{
		auto id = v.getProperty("id").toString();
		if (id.isNotEmpty())
			ids.add(id);
		return false;
	});

	return ids;
}

String RestServerUndoManager::UIValidationState::getComponentType(const String& id) const
{
	auto v = findComponent(id);
	if (v.isValid())
		return v.getProperty("type").toString();
	return {};
}

void RestServerUndoManager::UIValidationState::addComponent(const String& parentId, const String& type,
                                                              const String& id, int x, int y, int w, int h)
{
	ValueTree newChild("Component");
	newChild.setProperty("type", type, nullptr);
	newChild.setProperty("id", id, nullptr);
	newChild.setProperty("x", x, nullptr);
	newChild.setProperty("y", y, nullptr);
	newChild.setProperty("width", w, nullptr);
	newChild.setProperty("height", h, nullptr);

	if (parentId.isEmpty())
	{
		contentTree.addChild(newChild, -1, nullptr);
	}
	else
	{
		auto parentTree = findComponent(parentId);
		if (parentTree.isValid())
			parentTree.addChild(newChild, -1, nullptr);
	}
}

void RestServerUndoManager::UIValidationState::removeComponent(const String& id)
{
	auto v = findComponent(id);
	if (v.isValid())
		v.getParent().removeChild(v, nullptr);
}

void RestServerUndoManager::UIValidationState::renameComponent(const String& oldId, const String& newId)
{
	auto v = findComponent(oldId);
	if (v.isValid())
		v.setProperty("id", newId, nullptr);

	// Update parentComponent references in children that referenced oldId
	valuetree::Helpers::forEach(contentTree, [&](ValueTree& child)
	{
		if (child.getProperty("parentComponent").toString() == oldId)
			child.setProperty("parentComponent", newId, nullptr);
		return false;
	});
}

void RestServerUndoManager::UIValidationState::moveComponent(const String& id, const String& newParent, int insertIndex)
{
	auto v = findComponent(id);
	if (!v.isValid())
		return;

	v.getParent().removeChild(v, nullptr);

	if (newParent.isEmpty())
	{
		contentTree.addChild(v, insertIndex, nullptr);
	}
	else
	{
		auto parentTree = findComponent(newParent);
		if (parentTree.isValid())
			parentTree.addChild(v, insertIndex, nullptr);
	}
}

void RestServerUndoManager::UIValidationState::setProperty(const String& id, const Identifier& prop, const var& value)
{
	auto v = findComponent(id);
	if (v.isValid())
		v.setProperty(prop, value, nullptr);
}

const std::map<Identifier, std::vector<Identifier>>& RestServerUndoManager::UIValidationState::getPropertyMap()
{
	static std::map<Identifier, std::vector<Identifier>> map;

	if (map.empty())
	{
		// Base properties shared by all ScriptComponent types
		std::vector<Identifier> baseProps = {
			Identifier("text"), Identifier("visible"), Identifier("enabled"),
			Identifier("x"), Identifier("y"), Identifier("width"), Identifier("height"),
			Identifier("min"), Identifier("max"), Identifier("tooltip"),
			Identifier("bgColour"), Identifier("itemColour"), Identifier("itemColour2"), Identifier("textColour"),
			Identifier("macroControl"), Identifier("saveInPreset"), Identifier("isPluginParameter"),
			Identifier("pluginParameterName"), Identifier("pluginParameterGroup"),
			Identifier("isMetaParameter"), Identifier("linkedTo"),
			Identifier("automationId"), Identifier("useUndoManager"),
			Identifier("parentComponent"), Identifier("processorId"), Identifier("parameterId"),
			Identifier("defaultValue"), Identifier("locked"), Identifier("deferControlCallback")
		};

		// For plan-mode validation, all types get at least the base properties.
		// This is a shallow check — full validation happens at runtime.
		StringArray types = { "ScriptButton", "ScriptSlider", "ScriptPanel", "ScriptComboBox",
		                      "ScriptLabel", "ScriptImage", "ScriptTable", "ScriptSliderPack",
		                      "ScriptAudioWaveform", "ScriptFloatingTile", "ScriptWebView",
		                      "ScriptedViewport" };

		for (auto& t : types)
			map[Identifier(t)] = baseProps;
	}

	return map;
}

// ============================================================================
// DspValidationState
// ============================================================================

RestServerUndoManager::DspValidationState::DspValidationState(MainController* mc, const String& moduleId_) :
	ControlledObject(mc),
	moduleId(moduleId_)
{
	auto p = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId);

	if (auto holder = dynamic_cast<scriptnode::DspNetwork::Holder*>(p))
	{
		if (auto network = holder->getActiveOrDebuggedNetwork())
			networkTree = network->getValueTree().createCopy();
	}
}

ValueTree RestServerUndoManager::DspValidationState::findNodeRecursive(const ValueTree& tree, const String& id)
{
	using namespace scriptnode;

	if (tree[PropertyIds::ID].toString() == id)
		return tree;

	auto nodes = tree.getChildWithName(PropertyIds::Nodes);

	for (int i = 0; i < nodes.getNumChildren(); i++)
	{
		auto result = findNodeRecursive(nodes.getChild(i), id);
		if (result.isValid())
			return result;
	}

	return {};
}

ValueTree RestServerUndoManager::DspValidationState::findNode(const String& nodeId) const
{
	using namespace scriptnode;

	if (!networkTree.isValid())
		return {};

	// The root node is the first child of the Network ValueTree
	auto rootNode = networkTree.getChild(0);
	return findNodeRecursive(rootNode, nodeId);
}

bool RestServerUndoManager::DspValidationState::nodeExists(const String& nodeId) const
{
	return findNode(nodeId).isValid();
}

StringArray RestServerUndoManager::DspValidationState::getAllNodeIds() const
{
	using namespace scriptnode;

	StringArray ids;

	if (!networkTree.isValid())
		return ids;

	valuetree::Helpers::forEach(networkTree, [&](ValueTree& v)
	{
		if (v.getType() == PropertyIds::Node)
		{
			auto id = v[PropertyIds::ID].toString();
			if (id.isNotEmpty())
				ids.add(id);
		}
		return false;
	});

	return ids;
}

String RestServerUndoManager::DspValidationState::getFactoryPath(const String& nodeId) const
{
	using namespace scriptnode;

	auto v = findNode(nodeId);
	if (v.isValid())
		return v[PropertyIds::FactoryPath].toString();
	return {};
}

void RestServerUndoManager::DspValidationState::addNode(const String& parentId, const String& factoryPath,
                                                         const String& nodeId, int index)
{
	using namespace scriptnode;

	// Use the factory template so the snapshot node includes the real default
	// Parameters / Properties children. Without this, subsequent ops in the
	// same batch (set, connect, etc.) can't find parameters that the runtime
	// would populate at perform time.
	NodeDatabase db;
	ValueTree newNode = db.getValueTree(factoryPath);

	if (!newNode.isValid())
		return;

	newNode.setProperty(PropertyIds::ID, nodeId, nullptr);
	newNode.setProperty(PropertyIds::Name, nodeId, nullptr);
	newNode.setProperty(PropertyIds::Bypassed, false, nullptr);

	auto parent = findNode(parentId);
	if (parent.isValid())
	{
		auto nodesChild = parent.getChildWithName(PropertyIds::Nodes);
		if (nodesChild.isValid())
			nodesChild.addChild(newNode, index, nullptr);
	}
}

void RestServerUndoManager::DspValidationState::removeNode(const String& nodeId)
{
	auto v = findNode(nodeId);
	if (v.isValid())
		v.getParent().removeChild(v, nullptr);
}

void RestServerUndoManager::DspValidationState::moveNode(const String& nodeId, const String& newParent, int index)
{
	using namespace scriptnode;

	auto v = findNode(nodeId);
	if (!v.isValid())
		return;

	v.getParent().removeChild(v, nullptr);

	auto parent = findNode(newParent);
	if (parent.isValid())
	{
		auto nodesChild = parent.getChildWithName(PropertyIds::Nodes);
		if (nodesChild.isValid())
			nodesChild.addChild(v, index, nullptr);
	}
}

void RestServerUndoManager::DspValidationState::setNodeProperty(const String& nodeId, const Identifier& prop, const var& value)
{
	auto v = findNode(nodeId);
	if (v.isValid())
		v.setProperty(prop, value, nullptr);
}

void RestServerUndoManager::DspValidationState::setParameterValue(const String& nodeId, const String& parameterId, const var& value)
{
	using namespace scriptnode;

	auto node = findNode(nodeId);
	if (!node.isValid())
		return;

	auto params = node.getChildWithName(PropertyIds::Parameters);
	for (int i = 0; i < params.getNumChildren(); i++)
	{
		auto p = params.getChild(i);
		if (p[PropertyIds::ID].toString() == parameterId)
		{
			p.setProperty(PropertyIds::Value, value, nullptr);
			return;
		}
	}
}

}
