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

#pragma once

namespace hise {
using namespace juce;

struct RestServerUndoManager
{
	struct PlanValidationState: public ReferenceCountedObject,
								public ControlledObject
	{
		using Ptr = ReferenceCountedObjectPtr<PlanValidationState>;

		PlanValidationState(MainController* mc) :
		  ControlledObject(mc),
		  moduleTree(buildShallowModuleTree(mc->getMainSynthChain()))
		{
			//DBG(moduleTree.createXml()->createDocument(""));
			moduleTree.setProperty("Root", true, nullptr);
		}

		ProcessorMetadataRegistry rd;

		static ValueTree buildShallowModuleTree(Processor* root)
		{
			ValueTree v(root->getType());
			v.setProperty("ID", root->getId(), nullptr);
			
			for (int i = 0; i < root->getNumChildProcessors(); i++)
				v.addChild(buildShallowModuleTree(root->getChildProcessor(i)), -1, nullptr);

			return v;
		}

		void updateIdsToBeUnique(ValueTree& addedTree)
		{
			if (!addedTree.isValid())
				return;

			auto getBaseName = [](const String& id)
			{
				auto base = id;
				auto trailingNumbers = String(base.getTrailingIntValue());

				if (trailingNumbers.isNotEmpty())
					base = base.upToLastOccurrenceOf(trailingNumbers, false, false);

				return base;
			};

			auto isChainType = [](const Identifier& type)
			{
				return type == MidiProcessorChain::getClassType() ||
					   type == ModulatorChain::getClassType() ||
					   type == EffectProcessorChain::getClassType();
			};

			auto shouldProcessNode = [&](const ValueTree& v)
			{
				if (!v.isValid())
					return false;
				if ((bool)v["Root"])
					return false;
				if (!v.hasProperty("ID"))
					return false;
				if (v["ID"].toString().isEmpty())
					return false;
				if (isChainType(v.getType()))
					return false;

				return true;
			};

		std::map<String, int> baseNameCount;
		StringArray existingIds;

		valuetree::Helpers::forEach(moduleTree, [&](ValueTree& v)
		{
			if (!shouldProcessNode(v))
				return false;

			auto id = v["ID"].toString();
			existingIds.add(id);
			baseNameCount[getBaseName(id)]++;
			return false;
		});

		Array<ValueTree> nodesToRename;

		valuetree::Helpers::forEach(addedTree, [&](ValueTree& v)
		{
			if (shouldProcessNode(v))
				nodesToRename.add(v);

			return false;
		});

		for (auto node : nodesToRename)
		{
			if (!node.isValid() || !shouldProcessNode(node))
				continue;

			auto currentId = node["ID"].toString();

			// If the exact name doesn't collide, keep it as-is
			if (!existingIds.contains(currentId))
			{
				existingIds.add(currentId);
				baseNameCount[getBaseName(currentId)]++;
				continue;
			}

			// Collision found - strip trailing numbers and renumber
			auto baseName = getBaseName(currentId);
			auto amount = ++baseNameCount[baseName];
			auto newId = amount > 1 ? (baseName + String(amount)) : baseName;

			if (newId != currentId)
				node.setProperty("ID", newId, nullptr);

			existingIds.add(newId);
		}
		}

		bool add(const String& parent, ValueTree np, int chainIndex)
		{
			updateIdsToBeUnique(np);

			return valuetree::Helpers::forEach(moduleTree, [&](ValueTree& v)
			{
				if (v["ID"].toString() == parent)
				{
					if (chainIndex == -1)
						v.addChild(np, chainIndex, &um);
					else
					{
						if (isPositiveAndBelow(chainIndex, v.getNumChildren()))
							v.getChild(chainIndex).addChild(np, -1, &um);
						else
							return false;
					}

					return true;
				}

				return false;
			});
		}

		bool add(const String& parent, const Identifier& type, const String& child, int chainIndex)
		{
			ValueTree np(type);
			np.setProperty("ID", child, nullptr);
			
			if (auto md = rd.get(type))
			{
				for (auto& m : md->modulation)
				{
					ValueTree mod(ModulatorChain::getClassType());
					mod.setProperty("ID", m.id.toString(), nullptr);
					np.addChild(mod, -1, nullptr);
				}

				if (md->type == ProcessorMetadataIds::SoundGenerator)
				{
					np.addChild(ValueTree(MidiProcessorChain::getClassType()), 0, nullptr);
					np.addChild(ValueTree(EffectProcessorChain::getClassType()), 3, nullptr);
				}
			}

			return add(parent, np, chainIndex);
		}

		bool remove(const String& name)
		{
			return valuetree::Helpers::forEach(moduleTree, [&](ValueTree& v)
			{
				if (v["ID"].toString() == name)
				{
					v.getParent().removeChild(v, &um);
					return true;
				}

				return false;
			});
		}

		ValueTree get(const String& id) const
		{
			ValueTree rv;

			valuetree::Helpers::forEach(moduleTree, [&](ValueTree& v)
			{
				if (v["ID"].toString() == id)
				{
					rv = v;
					return true;
				}

				return false;
			});

			return rv;
		}

		bool setId(const String& oldId, const String& newId)
		{
			return valuetree::Helpers::forEach(moduleTree, [&](ValueTree& v)
			{
				if (v["ID"].toString() == oldId)
				{
					v.setProperty("ID", newId, &um);
					return true;
				}

				return false;
			});
		}

		ValueTree moduleTree;
		UndoManager um;
	};

	/** Stores a deep copy of a script processor's content ValueTree for plan-mode validation
	    of UI component operations. Created eagerly in pushPlan() using "Interface" as moduleId. */
	struct UIValidationState : public ReferenceCountedObject,
	                            public ControlledObject
	{
		using Ptr = ReferenceCountedObjectPtr<UIValidationState>;

		UIValidationState(MainController* mc);

		/** Find a component ValueTree by ID (recursive search). */
		ValueTree findComponent(const String& id) const;

		/** Check if a component with the given ID exists. */
		bool componentExists(const String& id) const;

		/** Check if child is a descendant of parent in the tree. */
		bool isDescendantOf(const String& child, const String& parent) const;

		/** Get all component IDs in the tree. */
		StringArray getAllComponentIds() const;

		/** Get the component type string for a given ID. */
		String getComponentType(const String& id) const;

		// Plan-mode mutations (modify the copied tree only)
		void addComponent(const String& parentId, const String& type, const String& id, int x, int y, int w, int h);
		void removeComponent(const String& id);
		void renameComponent(const String& oldId, const String& newId);
		void moveComponent(const String& id, const String& newParent, int insertIndex = -1);
		void setProperty(const String& id, const Identifier& prop, const var& value);

		ValueTree contentTree;   // deep copy of contentPropertyData
		String moduleId;

		/** Static map of component type -> valid property IDs for plan-mode validation. */
		static const std::map<Identifier, std::vector<Identifier>>& getPropertyMap();

	private:

		static ValueTree findComponentRecursive(const ValueTree& tree, const String& id);
	};

	struct DspValidationState : public ReferenceCountedObject,
	                            public ControlledObject
	{
		using Ptr = ReferenceCountedObjectPtr<DspValidationState>;

		DspValidationState(MainController* mc, const String& moduleId);

		/** Find a node ValueTree by ID (recursive search). */
		ValueTree findNode(const String& nodeId) const;

		/** Check if a node with the given ID exists. */
		bool nodeExists(const String& nodeId) const;

		/** Get all node IDs in the tree. */
		StringArray getAllNodeIds() const;

		/** Get the factory path for a given node ID. */
		String getFactoryPath(const String& nodeId) const;

		// Plan-mode mutations (modify the copied tree only)
		void addNode(const String& parentId, const String& factoryPath,
		             const String& nodeId, int index);
		void removeNode(const String& nodeId);
		void moveNode(const String& nodeId, const String& newParent, int index);
		void setNodeProperty(const String& nodeId, const Identifier& prop, const var& value);
		void setParameterValue(const String& nodeId, const String& parameterId, const var& value);

		ValueTree networkTree;   // deep copy of DspNetwork ValueTree
		String moduleId;

	private:

		static ValueTree findNodeRecursive(const ValueTree& tree, const String& id);
	};

	enum class Domain
	{
		Undefined  = 0x0,
		Builder    = 0x01, // modify the module tree
		UI         = 0x02, // modify the script components
		DSP        = 0x03, // modify the scriptnode graph
		numDomains
	};

	enum RebuildLevel
	{
		Nothing        = 0x0,
		UpdateUI       = 0x1,   // use this if the data model should send a UI update (eg. patch browser rebuild).
		UniqueId       = 0x2,	// use this if the data model should update the ID set
		Recompile      = 0x4,	// use this if the action should trigger a recompilation
		ParameterSlots = 0x8,   // use this if the action should trigger a parameter slot update
	};

	struct CallStack
	{
		enum class Phase
		{
			Prevalidation,
			Validation,
			Runtime,
			numPhases
		};

		static StringArray getPhaseNames()
		{
			return { "prevalidate", "validate", "runtime" };
		}

		CallStack(const String& errorMessage_) :
			errorMessage(errorMessage_)
		{}

		operator bool() const
		{
			return operationIndex != -1 &&
				phase != Phase::numPhases &&
				endpoint != RestHelpers::ApiRoute::numRoutes;
		}

		static var toJSONList(const std::vector<CallStack>& callstacks)
		{
			Array<var> list;

			for (const auto& cs : callstacks)
			{
				if (cs)
					list.add(cs.toJSON());
			}

			return var(list);
		}

		var toJSON() const
		{
			DynamicObject::Ptr e = new DynamicObject();
			e->setProperty(RestApiIds::errorMessage, errorMessage);

			Array<var> callStack;
			callStack.add("op[" + String(operationIndex) + "]: " + operationDescription);
			callStack.add("phase: " + getPhaseNames()[(int)phase]);
			callStack.add("group: "    + groupId);
			callStack.add("endpoint: " + RestHelpers::getRouteMetadata()[(int)endpoint].path);

			e->setProperty(RestApiIds::callstack, var(callStack));

			return var(e.get());
		}

		CallStack withEndpoint(RestHelpers::ApiRoute endpoint) const
		{
			auto copy = *this;
			copy.endpoint = endpoint;
			return copy;
		}

		CallStack withGroup(const String& group) const
		{
			auto copy = *this;
			copy.groupId = group;
			return copy;
		}

		CallStack withOperation(int index, const String& description) const
		{
			auto copy = *this;
			copy.operationIndex = index;
			copy.operationDescription = description;
			return copy;
		}

		CallStack withPhase(Phase p) const
		{
			auto copy = *this;
			copy.phase = p;

			// runtime doesn't have a operation index
			if(p == Phase::Runtime)
				copy.operationIndex = 0;

			return copy;
		}

		String errorMessage;
		int operationIndex = -1;
		String operationDescription;
		String groupId;
		Phase phase = Phase::numPhases;
		RestHelpers::ApiRoute endpoint = RestHelpers::ApiRoute::numRoutes;
	};

	struct Diff
	{
		enum class Type
		{
			Noop,
			Add,
			Remove,
			Modify,
			numTypes
		};

		static StringArray getTypeStrings() {
			return { "N", "+", "-", "*" };
		}

		var toJSON() const 
		{
			// this should be never JSONed...
			jassert(type != Type::Noop);

			DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty("domain", getDomains()[(int)domain]);
			obj->setProperty("action", getTypeStrings()[(int)type]);
			obj->setProperty("target", target.toString());

			return var(obj.get());
		}

		bool matches(const Diff& other) const 
		{ 
			return domain == other.domain && target == other.target; 
		}

		Diff combine(const Diff& other) const 
		{
			jassert(matches(other));

			auto type1 = type;
			auto type2 = other.type;

			auto copy = *this;

			if (type1 != type2)
			{
				if (type1 == Type::Noop)
					copy.type = type2;
				else if (type2 == Type::Noop)
					copy.type = type1;
				else if ((type1 == Type::Add && type2 == Type::Remove) ||
				         (type2 == Type::Add && type1 == Type::Remove))
					copy.type = Type::Noop;
				else if (type1 == Type::Modify)
					copy.type = type2;
				else if (type2 == Type::Modify)
					copy.type = type1;
			}
				
			return copy;
		}

		/** merges the diffs into dst. */
		static void mergeList(std::vector<Diff>& dst, const std::vector<Diff>& src)
		{
			for (const auto& s : src)
			{
				bool found = false;
				
				for (auto& d : dst)
				{
					if (d.matches(s))
					{
						d = d.combine(s);
						found = true;
						break;
					}
				}
				
				if (!found)
					dst.push_back(s);
			}
			
			dst.erase(std::remove_if(dst.begin(), dst.end(), 
				[](const Diff& d) { return d.type == Type::Noop; }), dst.end());
		}

		Domain domain;
		Identifier target;
		Type type = Type::Noop;
	};

	static StringArray getDomains() {
		return {
			"*",
			"builder",
			"ui",
			"dsp"
		};
	}

	struct ActionBase : public ControlledObject,
		public ReferenceCountedObject
	{
		struct Error
		{
			Error() = default;
			Error(const ActionBase& om);

			operator bool() const noexcept { return ok; }

			String getErrorMessage() const { return message; }
			Error withHint(const String& hint) const;
			Error withError(const String& error) const;

			String message;
			bool ok = true;
		};

		
		using List = ReferenceCountedArray<ActionBase>;
		using Ptr = ReferenceCountedObjectPtr<ActionBase>;

		ActionBase(MainController* mc) :
			ControlledObject(mc)
		{};

		/** Override this and return a short concise message that will be used in the undo history. */
		virtual String getHistoryMessage(bool undo) const = 0;

		/** Override this and return a full description of the action. */
		virtual String getDescription() const = 0;

		/** Override this and return true if this action needs to be wrapped in a killVoiceAndCall wrapper. */
		virtual bool needsKillVoice() const = 0;

		/** Override this and return a non-zero flag if this action invalidates the data model and needs some king of rebuild. 
		* 
		*	Use a bitwise or and the RebuildLevel enum (eg RebuildLevel::ModuleTree | RebuildLevel::UniqueId).
		* 
		*	This can vary between the undo/redo task (eg. performing "add" task should check unique IDs, undoing doesn't).
		* 
		*	The Domain parameter should be used to determine whether this affects the given domain at all
		*   (eg. builder.add should return 0 whenever the Domain UI is requested.
		*/
		virtual int getRebuildLevel(Domain d, bool undo) const = 0;

		/** Override this method and add to the diff list. */
		virtual void addToDiffList(std::vector<Diff>& diffList, bool undo) = 0;

		virtual String getActionId() const = 0;

		virtual ~ActionBase() {};

		virtual Error validate() = 0;
		virtual void perform() = 0;
		virtual void undo() = 0;

		/** Override this to return the moduleId for UI actions (used for recompile targeting). */
		virtual String getModuleId() const { return {}; }

		// filled in by the factory...
		Domain d = Domain::Undefined;

		PlanValidationState::Ptr planValidation;
		UIValidationState::Ptr uiValidation;
		DspValidationState::Ptr dspValidation;
	};

	struct Factory: public ControlledObject
	{
		using CreatorFunction = std::function<ActionBase::Ptr(MainController* mc, const var&)>;
		using ValidatorFunction = std::function<ActionBase::Error(MainController* mc, const var& obj)>;

		Factory(MainController* mc):
		  ControlledObject(mc)
		{ 
			registerAllFunctions(); 
		}

		ActionBase::Error prevalidate(Domain d, const var& obj) const;

		ActionBase::Ptr create(Domain d, const var& obj) const;

		void registerAllFunctions();

		void registerCreatorFunction(Domain d, const String& op, const ValidatorFunction& vf, const CreatorFunction& cf)
		{
			registeredFunctions[d][op] = { vf, cf };
		}

		template <typename T> void registerCreatorFunctionT(Domain d)
		{
			registerCreatorFunction(d, T::getActionIdStatic(), T::prevalidate, T::create);
		}

	private:

		std::map<Domain, std::map<String, std::pair<ValidatorFunction, CreatorFunction>>> registeredFunctions;
	};

	struct Instance : public ControlledObject
	{
		static Instance* getOrCreate(MainController* mc, RestHelpers::ApiRoute endpoint);
		static void flushUI(Processor* p);

		using PostOpCallback = std::function<void(ActionBase::List, bool)>;
		using AsyncRequest = RestServer::AsyncRequest;

		struct StackItem : public ReferenceCountedObject
		{
			using List = ReferenceCountedArray<StackItem>;
			using Ptr = ReferenceCountedObjectPtr<StackItem>;

			void toDiffList(std::vector<Diff>& diffList, bool merge)
			{
				auto idx = actions.indexOf(currentAction);

				for (int i = 0; i <= idx; i++)
				{
					if (merge)
					{
						std::vector<Diff> temp;
						actions[i]->addToDiffList(temp, false);
						Diff::mergeList(diffList, temp);
					}
					else
					{
						actions[i]->addToDiffList(diffList, false);
					}
				}
			}

			String name;
			PlanValidationState::Ptr planValidationState;
			UIValidationState::Ptr uiValidationState;
			DspValidationState::Ptr dspValidationState;
			ActionBase::Ptr currentAction;
			ActionBase::List actions;
		};

		Instance(MainController* mc);;

		static RestServer::Response getResponse(const std::vector<CallStack>& callstack, var result=var());

		bool killVoicesAndPerform(AsyncRequest::Ptr req, ActionBase::Ptr a, bool shouldUndo = false);

		void performAction(AsyncRequest::Ptr req, ActionBase::List list, const PostOpCallback& postOp, const String& name = {});
		void performAction(AsyncRequest::Ptr req, ActionBase::Ptr l);
		void clearUndoHistory();

		ValueTree getValidationPlan() const;

		bool undo(AsyncRequest::Ptr req);
		bool redo(AsyncRequest::Ptr req);

		void pushPlan(const String& name);
		bool popPlan(AsyncRequest::Ptr req);;

		ActionBase::Ptr createAction(Domain d, const var& obj) const
		{
			auto ad = f.create(d, obj);
			ad->planValidation = getCurrentValidationState();
			ad->uiValidation = getCurrentUIValidationState();

			// Lazy DspValidationState creation: inside a plan group, create/replace
			// the snapshot based on the DSP action's moduleId. Different from UI,
			// which hardcodes "Interface" at pushPlan time -- DSP module IDs vary.
			if (d == Domain::DSP)
			{
				auto cs = getCurrentStack();
				if (cs != nullptr && cs != rootActions)
				{
					auto actionModuleId = obj[RestApiIds::moduleId].toString();
					if (actionModuleId.isNotEmpty()
						&& (cs->dspValidationState == nullptr
							|| cs->dspValidationState->moduleId != actionModuleId))
					{
						cs->dspValidationState = new DspValidationState(
							const_cast<MainController*>(getMainController()),
							actionModuleId);
					}
				}
			}

			ad->dspValidation = getCurrentDspValidationState();
			return ad;
		}

		ActionBase::Error prevalidate(Domain d, const var& obj) const { return f.prevalidate(d, obj); }

		PlanValidationState::Ptr getCurrentValidationState() const
		{
			return getCurrentStack()->planValidationState;
		}

		UIValidationState::Ptr getCurrentUIValidationState() const
		{
			return getCurrentStack()->uiValidationState;
		}

		DspValidationState::Ptr getCurrentDspValidationState() const
		{
			return getCurrentStack()->dspValidationState;
		}

		String getCurrentGroupId() const { return getCurrentStack()->name; }

		var getHistory(bool flatten, bool currentGroup, Domain domainFilter = Domain::Undefined) const
		{
			auto r = currentGroup ? getCurrentStack() : rootActions;
			bool expand = !currentGroup && !flatten;
			
			DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty(RestApiIds::scope, currentGroup ? "group" : "root");
			obj->setProperty(RestApiIds::groupName, r->name);
			obj->setProperty(RestApiIds::cursor, r->actions.indexOf(r->currentAction));
			
			Array<var> history;
			int idx = 0;
			
			for (auto a : r->actions)
				actionToHistoryJSON(history, a, expand, flatten, domainFilter, idx);
			
			obj->setProperty(RestApiIds::history, var(history));
			return var(obj.get());
		}
		
		void setValidationErrors(const std::vector<CallStack>& preRuntimeErrors)
		{
			callStack = preRuntimeErrors;
		}

		void setCurrentEndpoint(RestHelpers::ApiRoute endpoint)
		{
			currentEndpoint = endpoint;
		}

	private:
		
		RestHelpers::ApiRoute currentEndpoint;

		std::vector<CallStack> callStack;

		void actionToHistoryJSON(Array<var>& history, ActionBase* a,
		                          bool expand, bool flatten, 
		                          Domain domainFilter, int& idx) const
		{
			if (auto s = dynamic_cast<Set*>(a))
			{
				if (flatten)
				{
					// Dissolve: recurse into sub-actions at the current level
					for (auto sub : s->subActions)
						actionToHistoryJSON(history, sub, expand, flatten, domainFilter, idx);
				}
				else if (expand)
				{
					// Expanded group with children array
					DynamicObject::Ptr h = new DynamicObject();
					h->setProperty(RestApiIds::index, idx++);
					h->setProperty(RestApiIds::type, "group");
					h->setProperty(RestApiIds::name, s->name);
					
					Array<var> children;
					int childIdx = 0;
					for (auto sub : s->subActions)
						actionToHistoryJSON(children, sub, expand, flatten, domainFilter, childIdx);
					
					// Only add if it has children after domain filtering
					if (children.size() > 0)
					{
						h->setProperty("children", var(children));
						history.add(var(h.get()));
					}
				}
				else
				{
					// Collapsed group: name and count
					DynamicObject::Ptr h = new DynamicObject();
					h->setProperty(RestApiIds::index, idx++);
					h->setProperty(RestApiIds::type, "group");
					h->setProperty(RestApiIds::name, s->name);
					h->setProperty(RestApiIds::count, s->subActions.size());
					history.add(var(h.get()));
				}
			}
			else
			{
				// Regular action - apply domain filter
				if (domainFilter != Domain::Undefined && a->d != domainFilter)
					return;
				
				DynamicObject::Ptr h = new DynamicObject();
				h->setProperty(RestApiIds::index, idx++);
				h->setProperty(RestApiIds::type, "action");
				h->setProperty(RestApiIds::domain, getDomains()[(int)a->d]);
				h->setProperty(RestApiIds::action, a->getActionId());
				h->setProperty(RestApiIds::description, a->getHistoryMessage(false));
				history.add(var(h.get()));
			}
		}
		
	public:

		var getDiffJSON(bool flatten, bool currentGroup, Domain domainFilter=Domain::Undefined) const
		{
			std::vector<Diff> diffs;

			auto r = currentGroup ? getCurrentStack() : rootActions;
			r->toDiffList(diffs, flatten);

			if (domainFilter != Domain::Undefined)
			{
				diffs.erase(std::remove_if(diffs.begin(), diffs.end(),
					[domainFilter](const Diff& d) { return d.domain != domainFilter; }), diffs.end());
			}

			Array<var> diffList;

			for (const auto& d : diffs)
				diffList.add(d.toJSON());

			DynamicObject::Ptr obj = new DynamicObject();

			obj->setProperty(RestApiIds::scope, currentGroup ? "group" : "root");
			obj->setProperty(RestApiIds::groupName, r->name);
			obj->setProperty(RestApiIds::diff, var(diffList));

			return var(obj.get());
		}

	private:

		Factory f;

		bool isTestRun(StackItem::Ptr s) const
		{
			return s != rootActions;
		}

		struct Set : public ActionBase
		{
			Set(MainController* mc, ActionBase::List l, const PostOpCallback& postOp, const String& name_) :
				ActionBase(mc),
				subActions(l),
				postOpCallback(postOp),
				name(name_)
			{}

			String getDescription() const override { return name; }

			String getHistoryMessage(bool undo) const override
			{
				return (undo ? "Undo " : "Perform ") + name;
			}

			void addToDiffList(std::vector<Diff>& diffList, bool undo) override
			{
				for (auto a : subActions)
					a->addToDiffList(diffList, undo);
			}

			int getRebuildLevel(Domain d, bool undo) const override
			{
				int level = 0;

				for (auto a : subActions)
					level |= a->getRebuildLevel(d, undo);

				return level;
			}

			void perform() override
			{
				int successfulCount = 0;
				try
				{
					for (auto a : subActions)
					{
						a->planValidation = planValidation;
						a->uiValidation = uiValidation;
						a->dspValidation = dspValidation;
						a->perform();
						successfulCount++;
					}
				}
				catch (...)
				{
					// Transactional rollback: undo already-performed sub-actions in reverse.
					// Best-effort -- a rethrown undo error during rollback would leave us
					// with even more partial state, so we swallow them and let the original
					// exception describe the failure.
					for (int i = successfulCount - 1; i >= 0; i--)
					{
						try { subActions[i]->undo(); }
						catch (...) { /* best-effort */ }
					}
					throw;
				}

				if (!subActions.isEmpty() && postOpCallback)
					postOpCallback(subActions, false);
			}

			bool needsKillVoice() const override
			{
				for (auto s : subActions)
				{
					if (s->needsKillVoice())
						return true;
				}

				return false;
			}

			String getActionId() const override
			{
				return "group";
			}

			String getModuleId() const override
			{
				for (auto a : subActions)
				{
					auto id = a->getModuleId();
					if (id.isNotEmpty())
						return id;
				}
				return {};
			}

			Error validate() override
			{
				for (auto a : subActions)
				{
					a->planValidation = planValidation;
					a->uiValidation = uiValidation;
					auto e = a->validate();

					if (!e)
						return e;
				}

				return {};
			}

			void undo() override
			{
				int undoneCount = 0;
				try
				{
					for (int i = subActions.size() - 1; i >= 0; i--)
					{
						subActions[i]->undo();
						undoneCount++;
					}
				}
				catch (...)
				{
					// Symmetric rollback: re-perform already-undone sub-actions
					// in their original forward order so we end up back at the
					// post-perform state instead of a half-undone mix.
					// Best-effort -- if re-perform also throws, we're degraded
					// but the original exception describes the primary failure.
					for (int i = subActions.size() - undoneCount; i < subActions.size(); i++)
					{
						try { subActions[i]->perform(); }
						catch (...) { /* best-effort */ }
					}
					throw;
				}

				if (!subActions.isEmpty() && postOpCallback)
					postOpCallback(subActions, true);
			}

			String name;
			PostOpCallback postOpCallback;
			List subActions;
		};

		StackItem::Ptr getCurrentStack() const
		{
			if (!planStack.isEmpty())
				return planStack.getLast();

			return rootActions;
		}

		StackItem::Ptr rootActions;
		StackItem::List planStack;
	};

};

}
