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
namespace rest_undo {



using namespace juce;
using namespace scriptnode;

using ActionBase       = RestServerUndoManager::ActionBase;
using Error            = RestServerUndoManager::ActionBase::Error;
using RebuildLevel     = RestServerUndoManager::RebuildLevel;
using Domain           = RestServerUndoManager::Domain;
using Diff             = RestServerUndoManager::Diff;

#define BUILDER_ID(x) static String getActionIdStatic() { return #x; } \
String getActionId() const override { return getActionIdStatic(); } \
static ActionBase::Ptr create(MainController* mc, const var& obj) { return new x(mc, obj);}

namespace builder {

	struct ProcessorReference
	{
		ProcessorReference(const String& id_) :
			id(id_)
		{}

		Processor* get() const { return processor.get(); }

		String getId() const
		{
			if (planData.isValid())
				return planData["ID"].toString();

			if (processor != nullptr)
				return processor->getId();

			return id;
		}
		Identifier getType() const
		{
			if (planData.isValid())
				return planData.getType();

			if (processor != nullptr)
				return processor->getType();

			return type;
		}

		bool initAtValidation(RestServerUndoManager::PlanValidationState::Ptr data, MainController* mc)
		{
			if (data != nullptr)
				return initPlanValidation(data);

			return initAtPerform(mc);
		}

		bool initPlanValidation(RestServerUndoManager::PlanValidationState::Ptr data)
		{
			if (data != nullptr)
			{
				processor = nullptr;
				planData = data->get(getId());
			}
			else
				planData = ValueTree();

			return existsInPlanValidation();
		}

		bool initAtPerform(MainController* mc)
		{
			planData = ValueTree();
			processor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), id);
			return existsInRuntime();
		}

		bool existsInRuntime() const { return processor != nullptr; }

		bool existsInPlanValidation() const
		{
			if (planData.isValid())
				return (bool)valuetree::Helpers::getRoot(planData)["Root"];

			return false;
		}

		bool isInPlanValidationMode() const
		{
			return planData.isValid();
		}

		bool isDeleted() const
		{
			if (isInPlanValidationMode())
				return !existsInPlanValidation();

			return !existsInRuntime();
		}

		void setProcessor(Processor* p)
		{
			processor = p;
            
            if(p != nullptr)
                type = p->getType();
        }

        void setType(const Identifier& t)
        {
            type = t;
        }
        
		void setId(String newId)
		{
			id = newId;

			if (processor != nullptr)
				processor->setId(newId, sendNotificationAsync);
		}

		String id;
        Identifier type;
		WeakReference<Processor> processor;
		ValueTree planData;
	};

struct Helpers
{
	static String getChainId(int index)
	{
		using namespace raw::IDs;

		if (index == Chains::Direct)
			return "direct";
		if (index == Chains::Midi)
			return "midi";
		if (index == Chains::Gain)
			return "gain";
		if (index == Chains::Pitch)
			return "pitch";
		if (index == Chains::FX)
			return "fx";

		return String(index);
	}

	static void throwProcessorDeleted(const String& moduleId)
	{
		throw Error().withError(moduleId + " was deleted");
	}

	static void throwRuntimeError(const String& errorMessage)
	{
		throw Error().withError(errorMessage);
	}

	static String getHintForUnknownString(const String& errorString, const StringArray& availableOptions)
	{
		auto correction = FuzzySearcher::suggestCorrection(errorString, availableOptions, 0.6);

		String hint;

		if (correction.isNotEmpty())
			hint << ". Did you mean " << correction;
		else if (availableOptions < 6)
			hint << ". Available parameters: " << availableOptions.joinIntoString(",");

		return hint;
	}

	static bool isChain(const Identifier& id)
	{
		return id == MidiProcessorChain::getClassType() ||
			   id == ModulatorChain::getClassType() ||
			   id == EffectProcessorChain::getClassType();
	}

	static std::pair<ProcessorReference, int> getParentSynthAndChainIndex(const ProcessorReference& p)
	{
		if (p.existsInRuntime())
		{
			auto parentChain = p.get()->getParentProcessor(false);
			auto realParent = parentChain;

			if (isChain(parentChain->getType()))
				realParent = parentChain->getParentProcessor(false);

			ProcessorReference parent("");

			if (realParent == nullptr)
				return { parent, -2};

			parent.setId(realParent->getId());
			parent.setProcessor(realParent);

			if (parentChain == realParent)
				return { parent, -1 };

			for (int i = 0; i < realParent->getNumInternalChains(); i++)
			{
				if (realParent->getChildProcessor(i) == parentChain)
					return { parent, i };
			}

			return { parent, -2 };
		}
		else
		{
			auto pData = p.planData;

			auto parentChain = pData.getParent();
			auto realParent = parentChain;

			if (isChain(parentChain.getType()))
				realParent = parentChain.getParent();

			ProcessorReference parent("");

			if (!realParent.isValid())
				return { parent, -2 };

			parent.setId(realParent["ID"]);
			parent.planData = realParent;

			if (parentChain == realParent)
				return { parent, -1 };

			auto chainIndex = realParent.indexOf(parentChain);

			if(chainIndex != -1)
				return { parent, chainIndex };

			return { parent, -2 };
		}
	}

	static void deleteProcessorAsync(ProcessorReference& pToDelete)
	{
        auto pm = pToDelete.get();
        
        if(pm == nullptr)
            return;
        
        pToDelete.setType(pm->getType());
        
		auto f = [](Processor* p)
		{
			auto mc = p->getMainController();

			if (auto c = dynamic_cast<Chain*>(p->getParentProcessor(false)))
			{
				c->getHandler()->remove(p, false);
				jassert(!p->isOnAir());
			}

			mc->getProcessorChangeHandler().sendProcessorChangeMessage(mc->getMainSynthChain(), 
				MainController::ProcessorChangeHandler::EventType::RebuildModuleList, 
				false);

			return SafeFunctionCall::OK;
		};

		pm->getMainController()->getGlobalAsyncModuleHandler().removeAsync(pm, f);
	}
};



struct add : public ActionBase
{
	BUILDER_ID(add);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::type].toString().isEmpty())
			return Error().withError("add requires type");

		if (op[RestApiIds::parent].toString().isEmpty())
			return Error().withError("add requires a parent");

		auto chainVal = op[RestApiIds::chain];

		if (chainVal.isVoid() || chainVal.isUndefined())
			return Error().withError("add requires 'chain' (int: -1 = direct, 0 = midi, 1 = gain, 2 = pitch, 3 = fx)");

		return {};
	}

	add(MainController* mc, const var& obj) :
		ActionBase(mc),
		typeId(obj["type"].toString()),
		parentProcessor(obj["parent"].toString()),
		createdProcessor(obj["name"].toString()),
		chainIndex((int)obj["chain"])
	{}

	Identifier typeId;
	int chainIndex;

	ProcessorReference parentProcessor;
	ProcessorReference createdProcessor;

	int getRebuildLevel(Domain d, bool undo) const override
	{
		if (d != Domain::Builder)
			return 0;

		if (!undo)
		{
			// adding a module needs to set the unique ID afterwards
			return RebuildLevel::UpdateUI | RebuildLevel::UniqueId;
		}
		else
		{
			return RebuildLevel::UpdateUI;
		}
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = createdProcessor.getId();
		d.domain = Domain::Builder;
		d.type = undo ? Diff::Type::Remove : Diff::Type::Add;

		diffList.push_back(d);
	}

	void perform() override
	{
		// should be caught before
		jassert(validate());

		if (!parentProcessor.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(parentProcessor.getId());

		raw::Builder b(getMainController());
		createdProcessor.setProcessor(b.create(parentProcessor.get(), typeId, chainIndex));

        if(createdProcessor.id.isNotEmpty())
            createdProcessor.setId(createdProcessor.id);
	}

	void undo() override
	{
		if (!createdProcessor.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(createdProcessor.getId());

		auto mc = getMainController();
		Helpers::deleteProcessorAsync(createdProcessor);
	}

	bool needsKillVoice() const override { return true; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Remove " : "Add ") + createdProcessor.getId();
	}

	String getDescription() const override
	{
		String s;
		s << "add " << typeId << " to " << parentProcessor.getId() << "." << Helpers::getChainId(chainIndex) << ": ";
		return s;
	}

	Error expectChainIndex(const Identifier& typeId, int expectedIndex) const
	{
		if (chainIndex != expectedIndex)
		{
			String msg;
			msg << "Use chain index " << String(expectedIndex) << "(" << Helpers::getChainId(expectedIndex) << ")";
			return Error(*this).withHint(msg);
		}

		return {};
	}

	Error expectWildcardMatch(const ProcessorMetadata& md, const String& wildcard) const
	{
		ProcessorMetadata::ConstrainerParser cp(wildcard);
		auto r = cp.check(md);

		if (r.failed())
		{
			return Error(*this)
				.withError(r.getErrorMessage() + ": " + md.id.toString())
				.withHint("Constrainer: " + wildcard);
		}

		return {};
	}

	Error validate() override
	{
		using namespace raw::IDs;

		if (!parentProcessor.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find parent with ID " + parentProcessor.getId());

		auto parentId = parentProcessor.getType();

		ProcessorMetadataRegistry rd;
		const ProcessorMetadata* pm = rd.get(parentId);
		const ProcessorMetadata* md = rd.get(typeId);

		if (md == nullptr)
		{
			StringArray sa;

			for (auto& id : rd.getRegisteredTypes())
				sa.add(id.toString());

			auto e = Error().withError("unknown module type " + typeId.toString());

			auto hint = FuzzySearcher::suggestCorrection(typeId.toString(), sa);

			if (hint.isNotEmpty())
				e = e.withHint(". Did you mean: " + hint);

			return e;
		}

		Error e;

		// check type match
		if (md->type == ProcessorMetadataIds::MidiProcessor)
		{
			e = expectChainIndex(md->type, Chains::Midi);
			if (!e)
				return e;
		}
		else if (md->type == ProcessorMetadataIds::Effect)
		{
			e = expectChainIndex(md->type, Chains::FX);
			if (!e)
				return e;

			return expectWildcardMatch(*md, pm->fxConstrainerWildcard);
		}
		else if (md->type == ProcessorMetadataIds::SoundGenerator)
		{
			e = expectChainIndex(md->type, Chains::Direct);
			if (!e)
				return e;

			e = expectWildcardMatch(*md, pm->constrainerWildcard);

			if (!e)
				return e;
		}
		else
		{
			jassert(md->type == ProcessorMetadataIds::Modulator);

			if (pm->type == ProcessorMetadataIds::SoundGenerator)
			{
				if(chainIndex == Chains::Direct || chainIndex == Chains::Midi || chainIndex == Chains::FX)
				{
					e = expectChainIndex(md->type, Chains::Gain);

					if (!e)
						return e;
				}
			}
				
			String hint;

			bool found = false;

			for (const auto& mod : pm->modulation)
			{
				if (mod.chainIndex == chainIndex)
				{
					if (mod.disabled)
						return Error(*this).withError(mod.id.toString() + " is disabled");
					
					e = expectWildcardMatch(*md, mod.constrainerWildcard);

					if (!e)
						return e;

					found = true;
					break;
				}

				if (!mod.disabled)
					hint << String(mod.chainIndex) << "(" << mod.id.toString() << "), ";
			}

			if (!found)
			{
				return Error(*this).withError("Illegal modulation chain index " + String(chainIndex))
					.withHint("Available modulation chains: " + hint);
			}
		}

		if (planValidation != nullptr)
		{
			if (!planValidation->add(parentProcessor.getId(), typeId, createdProcessor.getId(), chainIndex))
				return Error().withError("Plan model update failed");
		}

		return {};
	}
};

struct remove : public ActionBase
{
	BUILDER_ID(remove);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("remove requires 'target'");
		return {};
	}

	remove(MainController* mc, const var& obj) :
		ActionBase(mc),
		targetProcessor(obj["target"].toString()),
		parentProcessor({ ProcessorReference(""), -2 })
	{}

	ProcessorReference targetProcessor;
	std::pair<ProcessorReference, int> parentProcessor;

	ValueTree savedState;

	int getRebuildLevel(Domain d, bool undo) const override
	{
		if (d != Domain::Builder) return 0;
		return RebuildLevel::UpdateUI;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = targetProcessor.getId();
		d.domain = Domain::Builder;
		d.type = undo ? Diff::Type::Add : Diff::Type::Remove;
		diffList.push_back(d);
	}

	void perform() override 
	{ 
		if (!targetProcessor.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(targetProcessor.getId());

		savedState = targetProcessor.get()->exportAsValueTree();

		Helpers::deleteProcessorAsync(targetProcessor);
	}

	void undo() override 
	{ 
		if(!parentProcessor.first.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(parentProcessor.first.getId());

		raw::Builder b(getMainController());

		targetProcessor.setProcessor(b.create(parentProcessor.first.get(), targetProcessor.getType(), parentProcessor.second));
		targetProcessor.setId(savedState["ID"].toString());
		targetProcessor.get()->restoreFromValueTree(savedState);
	}

	bool needsKillVoice() const override { return true; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Add " : "Remove ") + targetProcessor.getId();
	}

	String getDescription() const override
	{
		return "remove " + targetProcessor.getId();
	}

	Error validate() override
	{
		if (!targetProcessor.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find module with ID " + targetProcessor.getId());

		parentProcessor = Helpers::getParentSynthAndChainIndex(targetProcessor);

		if (planValidation != nullptr)
		{
			if(!planValidation->remove(targetProcessor.getId()))
				return Error().withError("Plan model update failed");
		}

		return {};
	}
};

struct clone : public ActionBase
{
	BUILDER_ID(clone);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::source].toString().isEmpty())
			return Error().withError("clone requires 'source'");
		int c = (int)op.getProperty(RestApiIds::count, 0);
		if (c < 1)
			return Error().withError("clone requires count >= 1");
		if (c > 100)
			return Error().withError("clone count must be <= 100");
		return {};
	}

	clone(MainController* mc, const var& obj) :
		ActionBase(mc),
		source(obj["source"].toString()),
		insertPosition({ ProcessorReference(""), -2 }),
		cloneCount((int)obj["count"])
	{}

	ProcessorReference source;
	std::pair<ProcessorReference, int> insertPosition;
	Array<ProcessorReference> createdClones;

	const int cloneCount;

	int getRebuildLevel(Domain d, bool undo) const override
	{
		if (d != Domain::Builder) 
			return 0;
		return RebuildLevel::UpdateUI | (undo ? 0 : RebuildLevel::UniqueId);
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		for (const auto& name : createdClones)
		{
			Diff d;
			d.target = name.getId();
			d.domain = Domain::Builder;
			d.type = undo ? Diff::Type::Remove : Diff::Type::Add;
			diffList.push_back(d);
		}
	}

	void perform() override 
	{ 
		if (!source.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(source.getId());

		if(!insertPosition.first.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(insertPosition.first.getId());

		auto copy = source.get()->exportAsValueTree();

		raw::Builder b(getMainController());

		auto t = source.getType();

		for (int i = 0; i < cloneCount; i++)
		{
			auto cloned = b.create(insertPosition.first.get(), t, insertPosition.second);

			cloned->setId(source.getId(), dontSendNotification);
			auto thisId = FactoryType::getUniqueName(cloned, source.getId());
			cloned->setId(thisId, dontSendNotification);
			copy.setProperty("ID", thisId, nullptr);
			cloned->restoreFromValueTree(copy);

			ProcessorReference nd(thisId);
			nd.setProcessor(cloned);
			createdClones.add(nd);
		}
	}

	void undo() override 
	{ 
		if(!insertPosition.first.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(insertPosition.first.getId());

		for (auto b : createdClones)
		{
			if (b.initAtPerform(getMainController()))
				Helpers::deleteProcessorAsync(b);
			else
				Helpers::throwProcessorDeleted(b.getId());
		}
	}

	bool needsKillVoice() const override { return true; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Remove " : "Clone ") + String(cloneCount) + "x " + source.getId();
	}

	String getDescription() const override
	{
		String msg;
		msg << "clone " << source.getId() << " -> ";

		for (auto cn : createdClones)
			msg << cn.getId() << " ";

		return msg;
	}

	Error validate() override
	{
		if (!source.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find module with ID " + source.getId());

		insertPosition = Helpers::getParentSynthAndChainIndex(source);

		if(!insertPosition.first.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find module with ID " + insertPosition.first.getId());

		if (planValidation != nullptr)
		{
			for (int i = 0; i < cloneCount; i++)
			{
				auto clone = source.planData.createCopy();
				
				if (!planValidation->add(insertPosition.first.getId(), clone, insertPosition.second))
					return Error().withError("Plan model update failed");
			}
		}

		return {};
	}
};

struct set_attributes : public ActionBase
{
	BUILDER_ID(set_attributes);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("set_attributes requires 'target'");
		if (!op[RestApiIds::attributes].isObject())
			return Error().withError("set_attributes requires 'attributes' object");
		auto modeStr = op.getProperty(RestApiIds::mode, "value").toString();
		if (modeStr != "value" && modeStr != "normalized" && modeStr != "raw")
			return Error().withError("set_attributes mode must be 'value', 'normalized', or 'raw'");
		return {};
	}

	set_attributes(MainController* mc, const var& obj) :
		ActionBase(mc),
		target(obj["target"].toString()),
		attributes(obj["attributes"]),
		mode(obj.getProperty("mode", "value").toString())
	{}

	var attributes;
	String mode;

	ProcessorReference target;

	mutable std::map<Identifier, float> oldValues;
	mutable std::map<Identifier, float> newValues;

	var previousAttributes; // saved on perform for undo

	int getRebuildLevel(Domain d, bool undo) const override
	{
		// no rebuild necessary - the attributes send their update message
		return 0;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = target.getId();
		d.domain = Domain::Builder;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	void perform() override 
	{ 
		if (!target.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(target.getId());

		static const Identifier intensity("Intensity");

		auto targetProcessor = target.get();

		for (const auto& p : newValues)
		{
			if (p.first == intensity)
			{
				if (auto mod = dynamic_cast<Modulation*>(targetProcessor))
				{
					oldValues[p.first] = mod->getIntensity();
					mod->setIntensity(p.second);
					continue;
				}
			}

			auto idx = targetProcessor->getParameterIndexForIdentifier(p.first);

			if (idx == -1)
				Helpers::throwRuntimeError("Illegal parameter " + p.first);

			oldValues[p.first] = targetProcessor->getAttribute(idx);
			targetProcessor->setAttribute(idx, p.second, sendNotificationAsync);
		}
	}

	void undo() override 
	{ 
		if (!target.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(target.getId());

		auto targetProcessor = target.get();

		static const Identifier intensity("Intensity");

		for (const auto& p : oldValues)
		{
			if (p.first == intensity)
			{
				if (auto mod = dynamic_cast<Modulation*>(targetProcessor))
				{
					mod->setIntensity(p.second);
					continue;
				}
			}

			auto idx = targetProcessor->getParameterIndexForIdentifier(p.first);

			if (idx == -1)
				Helpers::throwRuntimeError("Illegal parameter " + p.first);

			targetProcessor->setAttribute(idx, p.second, sendNotificationAsync);
		}
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : "Set ") + String("attributes on ") + target.getId();
	}

	String getDescription() const override
	{
		return "set_attributes on " + target.getId();
	}

	Error validate() override
	{
		if (!target.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find module with ID " + target.getId());

		ProcessorMetadata md;

		bool skipDynamicParameters = false;

		// validate attribute names and ranges using ProcessorMetadata
		if (auto tp = target.get())
			md = tp->getMetadata();
		else
		{
			ProcessorMetadataRegistry rd;

			if (auto pmd = rd.get(target.getType()))
				md = *pmd;
			else
				return Error().withError("illegal type" + target.getType().toString());

			skipDynamicParameters = md.dataType == ProcessorMetadata::DataType::Dynamic;
		}

		for (auto& jsonParameter : attributes.getDynamicObject()->getProperties())
		{
			bool found = false;

			for (const ProcessorMetadata::ParameterMetadata& p : md.parameters)
			{
				if (jsonParameter.name.toString().toLowerCase() == "intensity")
				{
					if (md.type == ProcessorMetadataIds::Modulator)
					{
						oldValues["Intensity"] = 0.0f; // tbd
						newValues["Intensity"] = (float)jsonParameter.value;
						found = true;
						break;
					}
				}

				if (jsonParameter.name == p.id)
				{
					double value;

					if (!p.vtc.itemList.isEmpty())
					{
						auto v = jsonParameter.value.toString().toLowerCase();

						int idx = 1;

						bool valueFound = false;

						for (auto& it : p.vtc.itemList)
						{
							if (it.toLowerCase() == v)
							{
								value = (double)idx + 1;
								valueFound = true;
								break;
							}
	
							idx++;
						}

						if (!valueFound)
							return Error().withError(p.id.toString() + ": illegal value. ")
							.withHint(Helpers::getHintForUnknownString(v, p.vtc.itemList));
					}
					else
						value = (double)jsonParameter.value;

					if (p.type == ProcessorMetadata::ParameterMetadata::Type::List)
						value += 1.0;

					if (p.range.getRange().contains(value - 0.001))
					{
						newValues[p.id] = (float)value;
						oldValues[p.id] = 0.0f; // tbd
					}
					else
					{
						String e;
						e << "parameter " << p.id.toString() << ": " << String(value) << " is outside range [";
						e << String(p.range.rng.start) << " - " << String(p.range.rng.end) << "]";

						return Error().withError(e);
					}

					found = true;
					break;
				}
			}

			if (!found)
			{
				// In validation mode, dynamic parameters might not be visible to the dynamic metadata
				// so we need to let it slip through.
				if (skipDynamicParameters)
				{
					debugToConsole(getMainController()->getMainSynthChain(),
						"Skipping dynamic parameter validation for " + jsonParameter.name);

					continue;
				}

				StringArray parameterNames;
				for (auto& p : md.parameters)
					parameterNames.add(p.id.toString());

				auto errorName = jsonParameter.name.toString();
				auto hint = Helpers::getHintForUnknownString(errorName, parameterNames);

				return Error().withError("Unknown parameter " + errorName)
					.withHint(hint);
			}
		}

		return {};
	}
};

struct set_id : public ActionBase
{
	BUILDER_ID(set_id);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("set_id requires 'target'");
		if (op[RestApiIds::name].toString().isEmpty())
			return Error().withError("set_id requires 'name'");
		return {};
	}

	set_id(MainController* mc, const var& obj) :
		ActionBase(mc),
		previousName(obj["target"].toString()),
		newName(obj["name"].toString()),
		targetProcessor(previousName)
	{}

	String newName;
	String previousName;
	ProcessorReference targetProcessor;

	int getRebuildLevel(Domain d, bool undo) const override
	{
		return 0;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = undo ? newName : previousName;
		d.domain = Domain::Builder;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	void perform() override 
	{ 
		if (!targetProcessor.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(targetProcessor.getId());

		previousName = targetProcessor.getId();
		targetProcessor.setId(newName);
	}

	void undo() override 
	{ 
		if (!targetProcessor.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(targetProcessor.getId());

		targetProcessor.setId(previousName);
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return undo ? "Rename " + newName + " back to " + previousName
		            : "Rename " + previousName + " to " + newName;
	}

	String getDescription() const override
	{
		return "set_id " + previousName + " -> " + newName;
	}

	Error validate() override
	{
		if (!targetProcessor.initAtValidation(planValidation, getMainController()) || targetProcessor.isDeleted())
			return Error().withError("Can't find module with ID " + targetProcessor.getId());
		
		if (previousName == newName)
			return {};

		if (planValidation != nullptr)
		{
			if(planValidation->get(newName).isValid())
				return Error().withError("A module with the name " + newName + " already exists");

			if (!planValidation->setId(previousName, newName))
				return Error().withError("Plan model update failed");
		}
		else
		{
			if (auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), newName))
				return Error().withError("A module with the name " + newName + " already exists");
		}

		return {};
	}
};

struct set_bypassed : public ActionBase
{
	BUILDER_ID(set_bypassed);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("set_bypassed requires 'target'");
		auto bypassVal = op[RestApiIds::bypassed];
		if (bypassVal.isVoid() || bypassVal.isUndefined())
			return Error().withError("set_bypassed requires 'bypassed'");
		return {};
	}

	set_bypassed(MainController* mc, const var& obj) :
		ActionBase(mc),
		target(obj["target"].toString()),
		bypassed((bool)obj["bypassed"])
	{}

	ProcessorReference target;

	bool bypassed;
	bool previousBypassed = false;

	int getRebuildLevel(Domain d, bool undo) const override
	{
		return 0;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = target.getId();
		d.domain = Domain::Builder;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	void perform() override 
	{ 
		if (!target.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(target.getId());
		
		auto targetProcessor = target.get();
		previousBypassed = targetProcessor->isBypassed();
		targetProcessor->setBypassed(bypassed, sendNotificationAsync);
	}

	void undo() override 
	{ 
		if (!target.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(target.getId());

		auto targetProcessor = target.get();

		targetProcessor->setBypassed(previousBypassed, sendNotificationAsync);
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : (bypassed ? "Bypass " : "Unbypass ")) + target.getId();
	}

	String getDescription() const override
	{
		return String(bypassed ? "bypass " : "unbypass ") + target.getId();
	}

	Error validate() override
	{
		if(!target.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find module with ID " + target.getId());

		return {};
	}
};

struct set_effect : public ActionBase
{
	BUILDER_ID(set_effect);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("set_effect requires 'target'");
		if (op[RestApiIds::effect].toString().isEmpty())
			return Error().withError("set_effect requires 'effect'");
		return {};
	}

	set_effect(MainController* mc, const var& obj) :
		ActionBase(mc),
		target(obj["target"].toString()),
		effectName(obj["effect"].toString())
	{}

	ProcessorReference target;

	String effectName;
	String previousEffectName;

	int getRebuildLevel(Domain d, bool undo) const override
	{
		return 0;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = target.getId();
		d.domain = Domain::Builder;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	Error checkRuntimeEffectSlot()
	{
		auto slotFX = dynamic_cast<HotswappableProcessor*>(target.get());

		if (slotFX == nullptr)
			return Error().withError("not a hotswappable processor");

		auto list = slotFX->getModuleList();

		if (!list.contains(effectName))
			return Error().withError("unknown effect")
			.withHint(Helpers::getHintForUnknownString(effectName, list));

		return {};
	}

	void perform() override 
	{ 
		if (!target.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(target.getId());

		auto ok = checkRuntimeEffectSlot();

		if (!ok)
			throw ok;

		auto slotFX = dynamic_cast<HotswappableProcessor*>(target.get());

		previousEffectName = slotFX->getCurrentEffectId();
		LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);
		slotFX->setEffect(effectName, false);
	}

	void undo() override 
	{ 
		if (!target.initAtPerform(getMainController()))
			Helpers::throwProcessorDeleted(target.getId());

		auto ok = checkRuntimeEffectSlot();

		if (!ok)
			throw ok;

		auto slotFX = dynamic_cast<HotswappableProcessor*>(target.get());

		LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);
		slotFX->setEffect(previousEffectName, false);
	}

	bool needsKillVoice() const override { return true; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore effect on " : "Set effect '" + effectName + "' on ") + target.getId();
	}

	String getDescription() const override
	{
		return "set_effect " + effectName + " on " + target.getId();
	}

	Error validate() override
	{
		if(!target.initAtValidation(planValidation, getMainController()))
			return Error().withError("Can't find module with ID " + target.getId());

		if (target.existsInRuntime())
			return checkRuntimeEffectSlot();

		return {};
	}
};

struct set_complex_data : public ActionBase
{
	BUILDER_ID(set_complex_data);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("set_complex_data requires 'target'");
		return {};
	}

	set_complex_data(MainController* mc, const var& obj) :
		ActionBase(mc),
		target(obj["target"].toString()),
		targetProcessor(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), target))
	{}

	String target;
	ExternalData::DataType dt;

	var data;
	var previousData;
	WeakReference<Processor> targetProcessor;

    String getModuleId() const override { return target; }
    
	int getRebuildLevel(Domain d, bool undo) const override
	{
		if (d != Domain::Builder) return 0;
		return RebuildLevel::UpdateUI;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = target;
		d.domain = Domain::Builder;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	void perform() override { /* TODO */ }
	void undo() override { /* TODO */ }

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : "Set ") + String("complex data on ") + target;
	}

	String getDescription() const override
	{
		return "set_complex_data on " + target;
	}

	Error validate() override
	{
		return Error().withError("not implemented yet");

		if (targetProcessor == nullptr)
			return Error().withError("Can't find module with ID " + target);
		return {};
	}
};

} // builder

namespace ui {

// Valid component types for the add operation
static const StringArray& getValidComponentTypes()
{
	static StringArray types = {
		"ScriptButton", "ScriptSlider", "ScriptPanel", "ScriptComboBox",
		"ScriptLabel", "ScriptImage", "ScriptTable", "ScriptSliderPack",
		"ScriptAudioWaveform", "ScriptFloatingTile", "ScriptWebView", "ScriptedViewport"
	};
	return types;
}

static ScriptingApi::Content* getContent(MainController* mc, const String& moduleId)
{
	auto jp = dynamic_cast<JavascriptProcessor*>(
		ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId));
	if (jp == nullptr)
		return nullptr;
	return dynamic_cast<ProcessorWithScriptingContent*>(jp)->getScriptingContent();
}

static String autoGenerateId(const String& componentType, ScriptingApi::Content* content,
                              RestServerUndoManager::UIValidationState::Ptr uiState)
{
	StringArray existingIds;

	if (uiState != nullptr)
		existingIds = uiState->getAllComponentIds();
	else if (content != nullptr)
	{
		for (int i = 0; i < content->getNumComponents(); i++)
			existingIds.add(content->getComponent(i)->getName().toString());
	}

	for (int suffix = 1; ; suffix++)
	{
		auto candidate = componentType + String(suffix);
		if (!existingIds.contains(candidate))
			return candidate;
	}
}

// ============================================================================
// ui::add
// ============================================================================

struct add : public ActionBase
{
	BUILDER_ID(add);

	static Error prevalidate(MainController*, const var& op)
	{
		auto compType = op[RestApiIds::componentType].toString();

		if (compType.isEmpty())
			return Error().withError("add requires 'componentType'");

		if (!getValidComponentTypes().contains(compType))
		{
			auto hint = FuzzySearcher::suggestCorrection(compType, getValidComponentTypes());
			auto e = Error().withError("Unknown component type: " + compType);
			if (hint.isNotEmpty())
				e = e.withHint(". Did you mean: " + hint);
			return e;
		}

		return {};
	}

	add(MainController* mc, const var& obj) :
		ActionBase(mc),
		componentType(obj[RestApiIds::componentType].toString()),
		componentId(obj[RestApiIds::id].toString()),
		parentId(obj[RestApiIds::parentId].toString()),
		moduleId(obj[RestApiIds::moduleId].toString()),
		x((int)obj.getProperty(RestApiIds::x, 0)),
		y((int)obj.getProperty(RestApiIds::y, 0)),
		w((int)obj.getProperty(RestApiIds::width, 128)),
		h((int)obj.getProperty(RestApiIds::height, 48))
	{
		if (moduleId.isEmpty()) moduleId = "Interface";
	}

	String componentType, componentId, parentId, moduleId;
	int x, y, w, h;

	String getModuleId() const override { return moduleId; }

	int getRebuildLevel(Domain d, bool undo) const override
	{
		if (undo && d == Domain::UI)
			return (int)RebuildLevel::Recompile;
		return 0;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = Identifier(componentId);
		d.domain = Domain::UI;
		d.type = undo ? Diff::Type::Remove : Diff::Type::Add;
		diffList.push_back(d);
	}

	Error validate() override
	{
		// Auto-generate ID if not provided
		if (componentId.isEmpty())
		{
			auto content = getContent(getMainController(), moduleId);
			componentId = autoGenerateId(componentType, content, uiValidation);
		}

		// Check uniqueness
		if (uiValidation != nullptr)
		{
			if (uiValidation->componentExists(componentId))
				return Error().withError("A component with ID '" + componentId + "' already exists");

			if (parentId.isNotEmpty() && !uiValidation->componentExists(parentId))
				return Error().withError("Parent component '" + parentId + "' does not exist");

			uiValidation->addComponent(parentId, componentType, componentId, x, y, w, h);
		}
		else
		{
			auto content = getContent(getMainController(), moduleId);
			if (content == nullptr)
				return Error().withError("Can't find script processor: " + moduleId);

			if (content->getComponentWithName(Identifier(componentId)) != nullptr)
				return Error().withError("A component with ID '" + componentId + "' already exists");

			// Defer parentId existence check to perform() to support batched ops
			// where a preceding add creates the parent.
		}

		return {};
	}

	void perform() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr)
			throw Error().withError("Can't find script processor: " + moduleId);

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		ValueTree newChild("Component");
		newChild.setProperty("type", componentType, nullptr);
		newChild.setProperty("id", componentId, nullptr);
		newChild.setProperty("x", x, nullptr);
		newChild.setProperty("y", y, nullptr);
		newChild.setProperty("width", w, nullptr);
		newChild.setProperty("height", h, nullptr);

		if (parentId.isNotEmpty())
		{
			auto parentTree = content->getValueTreeForComponent(Identifier(parentId));
			if (!parentTree.isValid())
				throw Error().withError("Parent component '" + parentId + "' does not exist");
			parentTree.addChild(newChild, -1, nullptr);
		}
		else
		{
			content->getContentProperties().addChild(newChild, -1, nullptr);
		}

		// Create the C++ ScriptComponent and add to components array
		content->addComponentsFromValueTree(newChild);
		content->componentAdded();
	}

	void undo() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr) return;

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		auto v = content->getValueTreeForComponent(Identifier(componentId));
		if (v.isValid())
			v.getParent().removeChild(v, nullptr);
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Remove " : "Add ") + componentType + " '" + componentId + "'";
	}

	String getDescription() const override
	{
		return "add " + componentType + " '" + componentId + "'";
	}
};

// ============================================================================
// ui::remove
// ============================================================================

struct remove : public ActionBase
{
	BUILDER_ID(remove);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("remove requires 'target'");
		return {};
	}

	remove(MainController* mc, const var& obj) :
		ActionBase(mc),
		targetId(obj[RestApiIds::target].toString()),
		moduleId(obj[RestApiIds::moduleId].toString())
	{
		if (moduleId.isEmpty()) moduleId = "Interface";
	}

	String targetId, moduleId;
	ValueTree savedState;
	int savedIndex = -1;
	String savedParentId;

	String getModuleId() const override { return moduleId; }

	int getRebuildLevel(Domain d, bool) const override
	{
		if (d == Domain::UI)
			return (int)RebuildLevel::Recompile;
		return 0;
	}

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = Identifier(targetId);
		d.domain = Domain::UI;
		d.type = undo ? Diff::Type::Add : Diff::Type::Remove;
		diffList.push_back(d);
	}

	Error validate() override
	{
		if (uiValidation != nullptr)
		{
			if (!uiValidation->componentExists(targetId))
				return Error().withError("Component '" + targetId + "' does not exist");

			uiValidation->removeComponent(targetId);
		}
		// In runtime mode, defer existence check to perform() to support
		// batched ops where a preceding add creates the target.
		return {};
	}

	void perform() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr)
			throw Error().withError("Can't find script processor: " + moduleId);

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		auto v = content->getValueTreeForComponent(Identifier(targetId));
		if (!v.isValid())
			throw Error().withError("Component '" + targetId + "' does not exist");

		savedState = v.createCopy();
		savedParentId = v.getParent().getProperty("id").toString();
		savedIndex = v.getParent().indexOf(v);
		v.getParent().removeChild(v, nullptr);
	}

	void undo() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr) return;

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		if (savedParentId.isNotEmpty())
		{
			auto parentTree = content->getValueTreeForComponent(Identifier(savedParentId));
			if (parentTree.isValid())
				parentTree.addChild(savedState, savedIndex, nullptr);
		}
		else
		{
			content->getContentProperties().addChild(savedState, savedIndex, nullptr);
		}
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : "Remove ") + targetId;
	}

	String getDescription() const override
	{
		return "remove '" + targetId + "'";
	}
};

// ============================================================================
// ui::set
// ============================================================================

struct set : public ActionBase
{
	BUILDER_ID(set);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("set requires 'target'");
		if (!op[RestApiIds::properties].isObject())
			return Error().withError("set requires 'properties' object");

		// Reject parentComponent changes - use the 'move' operation instead
		if (auto props = op[RestApiIds::properties].getDynamicObject())
		{
			static const Identifier pcId("parentComponent");
			if (props->hasProperty(pcId))
				return Error().withError("Cannot change 'parentComponent' via set - use the 'move' operation instead");
		}

		return {};
	}

	set(MainController* mc, const var& obj) :
		ActionBase(mc),
		targetId(obj[RestApiIds::target].toString()),
		moduleId(obj[RestApiIds::moduleId].toString()),
		newProperties(obj[RestApiIds::properties]),
		forceOverride((bool)obj.getProperty(RestApiIds::force, false))
	{
		if (moduleId.isEmpty()) moduleId = "Interface";
	}

	String targetId, moduleId;
	var newProperties;
	bool forceOverride;
	NamedValueSet oldValues;

	String getModuleId() const override { return moduleId; }

	int getRebuildLevel(Domain, bool) const override { return 0; }

	void addToDiffList(std::vector<Diff>& diffList, bool /*undo*/) override
	{
		Diff d;
		d.target = Identifier(targetId);
		d.domain = Domain::UI;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	Error validate() override
	{
		auto props = newProperties.getDynamicObject();
		if (props == nullptr)
			return Error().withError("properties must be an object");

		if (uiValidation != nullptr)
		{
			if (!uiValidation->componentExists(targetId))
				return Error().withError("Component '" + targetId + "' does not exist");

			// Plan-mode: validate property names against static type map
			auto compType = uiValidation->getComponentType(targetId);
			auto& propertyMap = RestServerUndoManager::UIValidationState::getPropertyMap();
			auto it = propertyMap.find(Identifier(compType));

			if (it != propertyMap.end())
			{
				for (int i = 0; i < props->getProperties().size(); i++)
				{
					auto propName = props->getProperties().getName(i);
					bool found = false;
					for (auto& validProp : it->second)
					{
						if (validProp == propName)
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						StringArray validNames;
						for (auto& vp : it->second)
							validNames.add(vp.toString());

						auto hint = FuzzySearcher::suggestCorrection(propName.toString(), validNames);
						auto e = Error().withError("Unknown property '" + propName.toString() + "' on " + compType);
						if (hint.isNotEmpty())
							e = e.withHint(". Did you mean: " + hint);
						return e;
					}
				}
			}

			// Apply to validation tree
			for (int i = 0; i < props->getProperties().size(); i++)
			{
				auto propName = props->getProperties().getName(i);
				auto propValue = props->getProperties().getValueAt(i);
				uiValidation->setProperty(targetId, propName, propValue);
			}
		}
		else
		{
			auto content = getContent(getMainController(), moduleId);
			if (content == nullptr)
				return Error().withError("Can't find script processor: " + moduleId);

			auto sc = content->getComponentWithName(Identifier(targetId));

			// In runtime mode, the component may not exist yet if a preceding
			// add op in the same batch creates it. Defer existence check to perform().
			if (sc == nullptr)
				return {};

			// Runtime: full validation with live component
			for (int i = 0; i < props->getProperties().size(); i++)
			{
				auto propName = props->getProperties().getName(i);

				if (!sc->hasProperty(propName))
				{
					StringArray validNames;
					for (int j = 0; j < sc->getNumIds(); j++)
						validNames.add(sc->getIdFor(j).toString());

					auto hint = FuzzySearcher::suggestCorrection(propName.toString(), validNames);
					auto e = Error().withError("Unknown property '" + propName.toString() + "' on '" + targetId + "'");
					if (hint.isNotEmpty())
						e = e.withHint(". Did you mean: " + hint);
					return e;
				}

				// Check options for list-type properties
				auto opts = sc->getOptionsFor(propName);
				if (!opts.isEmpty())
				{
					auto val = props->getProperties().getValueAt(i).toString();
					if (!opts.contains(val))
					{
						return Error().withError("Invalid value '" + val + "' for property '" + propName.toString() + "'")
							.withHint("Valid options: " + opts.joinIntoString(", "));
					}
				}

				if (!forceOverride && sc->isPropertyOverwrittenByScript(propName))
					return Error().withError("Property '" + propName.toString() + "' on '" + targetId + "' is locked by script (use force=true to override)");

				// Save old value for undo
				oldValues.set(propName, sc->getScriptObjectProperty(propName));
			}
		}

		return {};
	}

	void perform() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr)
			throw Error().withError("Can't find script processor: " + moduleId);

		ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher());

		auto sc = content->getComponentWithName(Identifier(targetId));
		if (sc == nullptr)
			throw Error().withError("Component '" + targetId + "' does not exist");

		auto props = newProperties.getDynamicObject();
		if (props == nullptr) return;

		// Save old values if not already done (deferred from validation)
		if (oldValues.isEmpty())
		{
			for (int i = 0; i < props->getProperties().size(); i++)
			{
				auto propName = props->getProperties().getName(i);
				oldValues.set(propName, sc->getScriptObjectProperty(propName));
			}
		}

		for (int i = 0; i < props->getProperties().size(); i++)
		{
			auto propName = props->getProperties().getName(i);
			auto propValue = props->getProperties().getValueAt(i);

			sc->setScriptObjectPropertyWithChangeMessage(propName, propValue, sendNotification);
		}
	}

	void undo() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr) return;

		auto sc = content->getComponentWithName(Identifier(targetId));
		if (sc == nullptr) return;

        ValueTreeUpdateWatcher::ScopedDelayer sd(content->getUpdateWatcher());
        
		for (int i = 0; i < oldValues.size(); i++)
		{
			auto propName = oldValues.getName(i);
			auto propValue = oldValues.getValueAt(i);
			sc->setScriptObjectPropertyWithChangeMessage(propName, propValue, sendNotification);
		}
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Undo set on " : "Set properties on ") + targetId;
	}

	String getDescription() const override
	{
		return "set properties on '" + targetId + "'";
	}
};

// ============================================================================
// ui::move
// ============================================================================

struct move : public ActionBase
{
	BUILDER_ID(move);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("move requires 'target'");

		// parent can be empty string (= move to root), but field must exist
		if (!op.hasProperty(RestApiIds::parent))
			return Error().withError("move requires 'parent' (empty string for root)");

		return {};
	}

	move(MainController* mc, const var& obj) :
		ActionBase(mc),
		targetId(obj[RestApiIds::target].toString()),
		newParentId(obj[RestApiIds::parent].toString()),
		moduleId(obj[RestApiIds::moduleId].toString()),
		insertIndex((int)obj.getProperty(RestApiIds::index, -1)),
		keepPosition((bool)obj.getProperty(RestApiIds::keepPosition, false))
	{
		if (moduleId.isEmpty()) moduleId = "Interface";
	}

	String targetId, newParentId, moduleId;
	int insertIndex;
	bool keepPosition;
	String oldParentId;
	int oldX = 0, oldY = 0, oldIndex = -1;

	String getModuleId() const override { return moduleId; }

    int getRebuildLevel(Domain d, bool) const override
    {
        if(d == Domain::UI)
            return (int)RebuildLevel::Recompile;
        return 0;
    }

	void addToDiffList(std::vector<Diff>& diffList, bool /*undo*/) override
	{
		Diff d;
		d.target = Identifier(targetId);
		d.domain = Domain::UI;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	Error validate() override
	{
		if (uiValidation != nullptr)
		{
			if (!uiValidation->componentExists(targetId))
				return Error().withError("Component '" + targetId + "' does not exist");

			if (newParentId.isNotEmpty() && !uiValidation->componentExists(newParentId))
				return Error().withError("Parent component '" + newParentId + "' does not exist");

			if (newParentId.isNotEmpty() && uiValidation->isDescendantOf(newParentId, targetId))
				return Error().withError("Cannot move '" + targetId + "' to its own descendant '" + newParentId + "'");

			uiValidation->moveComponent(targetId, newParentId, insertIndex);
		}
		// In runtime mode, defer existence checks to perform() to support
		// batched ops where preceding ops create the targets.

		return {};
	}

	void perform() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr)
			throw Error().withError("Can't find script processor: " + moduleId);

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		auto v = content->getValueTreeForComponent(Identifier(targetId));
		if (!v.isValid())
			throw Error().withError("Component '" + targetId + "' does not exist");

		static const Identifier pc("parentComponent");
		static const Identifier xId("x");
		static const Identifier yId("y");

		// Save old state for undo
		oldParentId = v.getParent().getProperty("id").toString();
		oldX = (int)v.getProperty(xId);
		oldY = (int)v.getProperty(yId);
		oldIndex = v.getParent().indexOf(v);

		// Resolve new parent tree
		ValueTree newParentTree;
		if (newParentId.isNotEmpty())
			newParentTree = content->getValueTreeForComponent(Identifier(newParentId));
		else
			newParentTree = content->getContentProperties();

		if (!newParentTree.isValid()) return;

		// Compute position adjustment if keepPosition is enabled
		Point<int> newPos(oldX, oldY);

		if (keepPosition)
		{
			auto cPos = ContentValueTreeHelpers::getLocalPosition(v);
			ContentValueTreeHelpers::getAbsolutePosition(v, cPos);

			auto nPos = ContentValueTreeHelpers::getLocalPosition(newParentTree);
			ContentValueTreeHelpers::getAbsolutePosition(newParentTree, nPos);

			newPos = cPos - nPos;
		}

		// Adjust insertIndex if moving within same parent (moveItems pattern)
		int idx = insertIndex;
		if (v.getParent() == newParentTree && idx >= 0 && v.getParent().indexOf(v) < idx)
			--idx;

		// Reparent
		v.getParent().removeChild(v, nullptr);
		v.setProperty(pc, newParentId, nullptr);

		if (keepPosition)
		{
			v.setProperty(xId, newPos.getX(), nullptr);
			v.setProperty(yId, newPos.getY(), nullptr);
		}

		newParentTree.addChild(v, idx, nullptr);
	}

	void undo() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr) return;

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		auto v = content->getValueTreeForComponent(Identifier(targetId));
		if (!v.isValid()) return;

		static const Identifier pc("parentComponent");

		v.getParent().removeChild(v, nullptr);
		v.setProperty(pc, oldParentId, nullptr);
		v.setProperty("x", oldX, nullptr);
		v.setProperty("y", oldY, nullptr);

		if (oldParentId.isNotEmpty())
		{
			auto parentTree = content->getValueTreeForComponent(Identifier(oldParentId));
			if (parentTree.isValid())
				parentTree.addChild(v, oldIndex, nullptr);
		}
		else
		{
			content->getContentProperties().addChild(v, oldIndex, nullptr);
		}
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		auto target = undo ? oldParentId : newParentId;
		return "Move '" + targetId + "' to " + (target.isEmpty() ? "root" : "'" + target + "'");
	}

	String getDescription() const override
	{
		return "move '" + targetId + "' to " + (newParentId.isEmpty() ? "root" : "'" + newParentId + "'");
	}
};

// ============================================================================
// ui::rename
// ============================================================================

struct rename : public ActionBase
{
	BUILDER_ID(rename);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("rename requires 'target'");
		if (op[RestApiIds::newId].toString().isEmpty())
			return Error().withError("rename requires 'newId'");

		auto newName = op[RestApiIds::newId].toString();

		// Validate identifier characters
		if (!Identifier::isValidIdentifier(newName))
			return Error().withError("'" + newName + "' is not a valid identifier (alphanumeric + underscore, must start with letter)");

		return {};
	}

	rename(MainController* mc, const var& obj) :
		ActionBase(mc),
		targetId(obj[RestApiIds::target].toString()),
		newName(obj[RestApiIds::newId].toString()),
		moduleId(obj[RestApiIds::moduleId].toString())
	{
		if (moduleId.isEmpty()) moduleId = "Interface";
	}

	String targetId, newName, moduleId;

	String getModuleId() const override { return moduleId; }

    int getRebuildLevel(Domain d, bool) const override
    {
        if(d == Domain::UI)
            return (int)RebuildLevel::Recompile;
        return 0;
    }

	void addToDiffList(std::vector<Diff>& diffList, bool undo) override
	{
		Diff d;
		d.target = Identifier(undo ? targetId : newName);
		d.domain = Domain::UI;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	Error validate() override
	{
		if (targetId == newName)
			return {};

		if (uiValidation != nullptr)
		{
			if (!uiValidation->componentExists(targetId))
				return Error().withError("Component '" + targetId + "' does not exist");

			if (uiValidation->componentExists(newName))
				return Error().withError("A component with ID '" + newName + "' already exists");

			uiValidation->renameComponent(targetId, newName);
		}
		// In runtime mode, defer existence checks to perform() to support
		// batched ops where preceding ops create the targets.

		return {};
	}

	void perform() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr)
			throw Error().withError("Can't find script processor: " + moduleId);

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		auto v = content->getValueTreeForComponent(Identifier(targetId));
		if (!v.isValid())
			throw Error().withError("Component '" + targetId + "' does not exist");

		auto v2 = content->getValueTreeForComponent(Identifier(newName));

		if (v2.isValid())
			throw Error().withError("A component with ID'" + newName + "' already exists");

		v.setProperty("id", newName, nullptr);

		// Update parentComponent references in any children that referenced the old name
		valuetree::Helpers::forEach(content->getContentProperties(), [&](ValueTree& child)
		{
			if (child.getProperty("parentComponent").toString() == targetId)
				child.setProperty("parentComponent", newName, nullptr);
			return false;
		});
	}

	void undo() override
	{
		auto content = getContent(getMainController(), moduleId);
		if (content == nullptr) return;

		ValueTreeUpdateWatcher::ScopedSuspender ss(content->getUpdateWatcher());

		auto v = content->getValueTreeForComponent(Identifier(newName));
		if (v.isValid())
			v.setProperty("id", targetId, nullptr);

		// Reverse parentComponent references
		valuetree::Helpers::forEach(content->getContentProperties(), [&](ValueTree& child)
		{
			if (child.getProperty("parentComponent").toString() == newName)
				child.setProperty("parentComponent", targetId, nullptr);
			return false;
		});
	}

	bool needsKillVoice() const override { return false; }

	String getHistoryMessage(bool undo) const override
	{
		return undo ? ("Rename '" + newName + "' back to '" + targetId + "'")
		            : ("Rename '" + targetId + "' to '" + newName + "'");
	}

	String getDescription() const override
	{
		return "rename '" + targetId + "' to '" + newName + "'";
	}
};

} // ui

namespace dsp {

using namespace scriptnode;

struct Helpers
{
	// Helper to resolve the active DspNetwork from a module ID
	static DspNetwork* getNetworkFromModule(MainController* mc, const String& moduleId)
	{
		auto p = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), moduleId);

		if (auto holder = dynamic_cast<DspNetwork::Holder*>(p))
			return holder->getActiveOrDebuggedNetwork();

		return nullptr;
	}

	using Error = RestServerUndoManager::ActionBase::Error;

	

	static ValueTree findValueTree(const ValueTree& r, const std::function<bool(const ValueTree&)>& f)
	{
		ValueTree result;

		valuetree::Helpers::forEach(r, [&](const ValueTree& c)
		{
			if (f(c))
			{
				result = c;
				return true;
			}

			return false;
		});

		return result;
	}

	static ValueTree findNode(const ValueTree& r, const String& nodeId)
	{
		return findValueTree(r, [&](const ValueTree& c)
		{
			return c.getType() == PropertyIds::Node && c[PropertyIds::ID].toString() == nodeId;
		});
	}

	static Error getErrorForModule404(MainController* mc, const String& moduleId)
	{
		auto l = ProcessorHelpers::getListOfAllProcessors<DspNetwork::Holder>(mc->getMainSynthChain());

		StringArray moduleList;

		for (auto h : l)
			moduleList.add(dynamic_cast<Processor*>(h.get())->getId());

		return Error().withError(moduleId + " not found. ").withHint(builder::Helpers::getHintForUnknownString(moduleId, moduleList));
	}

	static Error getErrorForNode404(const ValueTree& v, const String& nodeId)
	{
		StringArray nodeList;

		valuetree::Helpers::forEach(v, [&](const ValueTree& c)
		{
			if (c.getType() == PropertyIds::Node)
			{
				nodeList.add(c[PropertyIds::ID].toString());
			}

			return false;
		});

		auto hint = builder::Helpers::getHintForUnknownString(nodeId, nodeList);
		return Error().withError(nodeId + " not found. ").withHint(hint);
	}

	static Error getErrorForFactory404(const String& factoryPath)
	{
		scriptnode::NodeDatabase db;

		StringArray list = db.getNodeIds(false);
		list.addArray(db.getNodeIds(true));

		auto hint = builder::Helpers::getHintForUnknownString(factoryPath, list);
		return Error().withError(factoryPath + " not a valid node type. ").withHint(hint);
	}

	static Error getErrorForParameter404(const ValueTree& n, const String& parameterId)
	{
		StringArray ids;

		for (auto p : n.getChildWithName(PropertyIds::Parameters))
			ids.add(p[PropertyIds::ID].toString());

		for (auto p : n.getChildWithName(PropertyIds::Properties))
			ids.add(p[PropertyIds::ID].toString());

		auto hint = builder::Helpers::getHintForUnknownString(parameterId, ids);
		return Error().withError("Unknown parameter or property " + parameterId + ". ").withHint(hint);
	}

	static ValueTree getRootTree(const RestServerUndoManager::ActionBase* a, const String& moduleId)
	{
		if (a->dspValidation != nullptr)
		{
			jassert(moduleId == a->dspValidation->moduleId);
			return a->dspValidation->networkTree;
		}

		auto mc = const_cast<RestServerUndoManager::ActionBase*>(a)->getMainController();

		if (auto an = getNetworkFromModule(mc, moduleId))
			return an->getValueTree();

		return {};
	}

	static String makeUniqueId(const ValueTree& v, String id)
	{
		int trailingIndex = id.getTrailingIntValue();
		auto idWithoutNumber = trailingIndex == 0 ? id : id.upToLastOccurrenceOf(String(trailingIndex), false, false);

		trailingIndex++;

		id = idWithoutNumber + String(trailingIndex);

		auto findNode = [&](const ValueTree& c)
		{
			return c.getType() == PropertyIds::Node && c[PropertyIds::ID].toString() == id;
		};

		auto existingNode = findValueTree(v, findNode);

		while (existingNode.isValid())
		{
			trailingIndex++;
			id = idWithoutNumber + String(trailingIndex);
			existingNode = findValueTree(v, findNode);
		}

		return id;
	}

	static void deleteNode(ActionBase* a, const String& moduleId, const ValueTree& nodeToDelete)
	{
		auto idToDelete = nodeToDelete[PropertyIds::ID].toString();

		nodeToDelete.getParent().removeChild(nodeToDelete, nullptr);

		if (a->dspValidation == nullptr)
		{
			if (auto an = getNetworkFromModule(a->getMainController(), moduleId))
				an->deleteIfUnused(idToDelete);
		}
	}

	static Array<Identifier> getInlineNodeProperties(bool includeContainer)
	{
		Array<Identifier> ids ={
			PropertyIds::Bypassed,
			PropertyIds::NodeColour,
			PropertyIds::Folded,
			PropertyIds::Name,
			PropertyIds::Comment
		};

		if (includeContainer)
		{
			ids.add(PropertyIds::ShowParameters);
		}
		    
		return ids;
	}

	static ValueTree findParameterOrProperty(const ValueTree& n, const String& id, bool searchProperties)
	{
		jassert(n.getType() == PropertyIds::Node);

		if (id.isEmpty())
		{
			auto fp = n[PropertyIds::FactoryPath].toString();

			// special path for allowing "connect SEND to RECEIVE"
			if (fp == "routing.receive")
				return n;
		}

		auto nodeIds = getInlineNodeProperties(true);

		if (nodeIds.contains(Identifier(id)))
			return n;

		for (auto p : n.getChildWithName(PropertyIds::Parameters))
		{
			if (p[PropertyIds::ID].toString() == id)
				return p;
		}

		if (searchProperties)
		{
			for (auto p : n.getChildWithName(PropertyIds::Properties))
			{
				if (p[PropertyIds::ID].toString() == id)
					return p;
			}

			if (n.getParent().getType() == PropertyIds::Network)
			{
				if (n.getParent().hasProperty(id))
					return n;
			}
		}

		return {};
	}

	static bool addConnection(ValueTree conTree, const String& nodeId, const String& parameterId)
	{
		if (parameterId.isEmpty())
		{
			jassert(conTree.getType() == PropertyIds::Property);

			auto connections = conTree[PropertyIds::Value].toString();

			if (connections.isEmpty())
			{
				conTree.setProperty(PropertyIds::Value, nodeId, nullptr);
				return true;
			}
			
			if (connections.contains(nodeId))
				return false;

			connections << ";" << nodeId;

			conTree.setProperty(PropertyIds::Value, connections, nullptr);
			return true;
		}

		jassert(conTree.getType() == PropertyIds::Connections ||
			conTree.getType() == PropertyIds::ModulationTargets);

		for (const auto& c : conTree)
		{
			if (c[PropertyIds::NodeId].toString() == nodeId &&
				c[PropertyIds::ParameterId].toString() == parameterId)
				return false;
		}

		ValueTree cTree(PropertyIds::Connection);
		cTree.setProperty(PropertyIds::NodeId, nodeId, nullptr);
		cTree.setProperty(PropertyIds::ParameterId, parameterId, nullptr);

		conTree.addChild(cTree, -1, nullptr);
		return true;
	}

	static bool removeConnection(ValueTree conTree, const String& nodeId, const String& parameterId)
	{
		if (parameterId.isEmpty())
		{
			jassert(conTree.getType() == PropertyIds::Property);

			auto connections = conTree[PropertyIds::Value].toString();

			if (!connections.contains(nodeId))
				return false;

			connections.replace(nodeId, "");

			if (connections == ";")
				connections = {};

			conTree.setProperty(PropertyIds::Value, connections, nullptr);
			return true;
		}

		jassert(conTree.getType() == PropertyIds::Connections ||
			conTree.getType() == PropertyIds::ModulationTargets);

		for (auto c : conTree)
		{
			if (c[PropertyIds::NodeId].toString() == nodeId &&
				c[PropertyIds::ParameterId].toString() == parameterId)
			{
				c.getParent().removeChild(c, nullptr);
				return true;
			}
		}

		return false;
	}

	static String getSourceOutput(const ValueTree& conTree)
	{
		jassert(conTree.getType() == PropertyIds::Connection);

		auto pTree = valuetree::Helpers::findParentWithType(conTree, PropertyIds::Parameter);
		auto mTree = valuetree::Helpers::findParentWithType(conTree, PropertyIds::ModulationTargets);
		auto sTree = valuetree::Helpers::findParentWithType(conTree, PropertyIds::SwitchTarget);

		if (pTree.isValid())
			return pTree[PropertyIds::ID].toString();
		if (mTree.isValid())
			return "0";
		if (sTree.isValid())
			return String(sTree.getParent().indexOf(sTree));
		
		jassertfalse;
		return {};
	}

	static ValueTree getConnectionParent(const ValueTree& sn, const String& sourceOutput)
	{
		auto fp = sn[PropertyIds::FactoryPath].toString();

		if (fp == "routing.send")
		{
			for (auto p : sn.getChildWithName(PropertyIds::Properties))
			{
				if (p[PropertyIds::ID].toString() == PropertyIds::Connection.toString())
					return p;
			}

			jassertfalse;
			return {};
		}

		bool isParameterConnection = String(sourceOutput.getIntValue()) != sourceOutput;

		if (isParameterConnection)
		{
			auto sp = Helpers::findParameterOrProperty(sn, sourceOutput, false);

			if (!sp.isValid())
				throw Helpers::getErrorForParameter404(sn, sourceOutput);

			return sp.getOrCreateChildWithName(PropertyIds::Connections, nullptr);
		}
		else
		{
			auto switchTree = sn.getChildWithName(PropertyIds::SwitchTargets);

			auto idx = sourceOutput.getIntValue();

			if (switchTree.isValid())
			{
				auto conTree = switchTree.getChild(idx);

				if (!conTree.isValid())
					throw Error().withError("invalid modulation output index for node " + sn[PropertyIds::ID].toString())
					.withHint("index: " + String(idx) + ", maxIndex: " + String(switchTree.getNumChildren()));

				return conTree.getChildWithName(PropertyIds::Connections);
			}
			else
			{
				if (idx != 0)
					throw Error().withError("invalid modulation output index for node " + sn[PropertyIds::ID].toString())
					.withHint("index: " + String(idx) + ", maxIndex: 0");

				return sn.getChildWithName(PropertyIds::ModulationTargets);
			}
		}
	}
};



struct add : public ActionBase
{
	BUILDER_ID(add);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::factoryPath].toString().isEmpty())
			return Error().withError("add requires 'factoryPath'");
		if (op[RestApiIds::parent].toString().isEmpty())
			return Error().withError("add requires 'parent'");
		return {};
	}

	add(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		factoryPath(obj[RestApiIds::factoryPath].toString()),
		parentId(obj[RestApiIds::parent].toString()),
		nodeId(obj[RestApiIds::nodeId].toString()),
		insertIndex(obj.getProperty(RestApiIds::index, -1))
	{}

	String moduleId;
	String factoryPath;
	String parentId;
	String nodeId;
	int insertIndex;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = nodeId.isNotEmpty() ? nodeId : factoryPath;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Add;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Remove " : "Add ") + factoryPath + " as " + nodeId;
	}

	String getDescription() const override
	{
		return "add " + factoryPath + " to " + parentId;
	}

	scriptnode::NodeDatabase db;

	Error validate() override
	{
		auto rn = Helpers::getRootTree(this, moduleId);

		// check module & network exists
		if (!rn.isValid())
			return Helpers::getErrorForModule404(getMainController(), moduleId);

		// check factory path is valid
		if (!db.getValueTree(factoryPath).isValid())
			return Helpers::getErrorForFactory404(factoryPath);

		if (nodeId.isNotEmpty())
		{
			// check node doesn't exist
			auto en = Helpers::findValueTree(rn, [&](const ValueTree& c)
			{
				return c.getType() == PropertyIds::Node && c[PropertyIds::ID].toString() == nodeId;
			});

			if (en.isValid())
				return Error().withError("node with ID " + nodeId + " already exists. Tip: omit nodeId to auto-generate a unique ID");
		}

		// omit the parent ID check at validation, might be created in a op before that
		if (dspValidation != nullptr)
		{
			auto pe = validateParent(rn);
			if (!pe)
				return pe;

			// Mirror the perform on the plan snapshot so subsequent ops in the
			// same group (and mid-group GET /api/dsp/tree?group=current reads)
			// see the accumulated state. Auto-ID generation is deferred to
			// perform() at commit time; we only mirror nodes with explicit IDs.
			if (nodeId.isNotEmpty())
				dspValidation->addNode(parentId, factoryPath, nodeId, insertIndex);
		}

		return {};
	}

	Error validateParent(const ValueTree& rn)
	{
		// check parent node exists
		auto pn = Helpers::findValueTree(rn, [&](const ValueTree& c)
		{
			return c.getType() == PropertyIds::Node && c[PropertyIds::ID].toString() == parentId;
		});

		auto childList = pn.getChildWithName(PropertyIds::Nodes);

		if (!childList.isValid())
			return Error().withError(parentId + " is not a container node");

		if (!pn.isValid())
			return Helpers::getErrorForNode404(rn, parentId);

		return {};
	}

	void perform() override
	{
		auto rn = Helpers::getRootTree(this, moduleId);

		if(!rn.isValid())
			throw Helpers::getErrorForModule404(getMainController(), moduleId);

		auto pe = validateParent(rn);

		if (!pe)
			throw pe;

		ValueTree nodeToAdd = db.getValueTree(factoryPath);

		if(!nodeToAdd.isValid())
			throw Helpers::getErrorForFactory404(factoryPath);

		if (nodeId.isEmpty())
		{
			nodeId = factoryPath.fromFirstOccurrenceOf(".", false, false);
			nodeToAdd.setProperty(PropertyIds::Name, nodeId, nullptr);
			nodeId = Helpers::makeUniqueId(rn, nodeId);
		}
		else
			nodeToAdd.setProperty(PropertyIds::Name, nodeId, nullptr);
		
		nodeToAdd.setProperty(PropertyIds::ID, nodeId, nullptr);
		

		auto pn = Helpers::findValueTree(rn, [&](const ValueTree& c)
		{
			return c.getType() == PropertyIds::Node && c[PropertyIds::ID].toString() == parentId;
		});

		auto childList = pn.getChildWithName(PropertyIds::Nodes);
		childList.addChild(nodeToAdd, insertIndex, nullptr);
	}

	void undo() override
	{
		auto rn = Helpers::getRootTree(this, moduleId);

		if (!rn.isValid())
			throw Helpers::getErrorForModule404(getMainController(), moduleId);

		auto nodeToDelete = Helpers::findValueTree(rn, [&](const ValueTree& c)
		{
			return c.getType() == PropertyIds::Node && c[PropertyIds::ID].toString() == nodeId;
		});

		if (!nodeToDelete.isValid())
			throw Helpers::getErrorForNode404(rn, nodeId);

		Helpers::deleteNode(this, moduleId, nodeToDelete);
	}
};

struct remove : public ActionBase
{
	BUILDER_ID(remove);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::nodeId].toString().isEmpty())
			return Error().withError("remove requires 'nodeId'");
		return {};
	}

	remove(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		nodeId(obj[RestApiIds::nodeId].toString())
	{}

	String moduleId;
	String nodeId;

	ValueTree removedTree;
	String parentId;
	int oldIndex;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = nodeId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Remove;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : "Remove ") + nodeId;
	}

	String getDescription() const override
	{
		return "remove " + nodeId;
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (dspValidation != nullptr)
		{
			if (!Helpers::findNode(rv, nodeId).isValid())
				return Helpers::getErrorForNode404(rv, nodeId);

			// Mirror the removal onto the plan snapshot.
			dspValidation->removeNode(nodeId);
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		removedTree = Helpers::findNode(rv, nodeId);

		if (removedTree.isValid())
		{
			oldIndex = removedTree.getParent().indexOf(removedTree);
			parentId = removedTree.getParent().getParent()[PropertyIds::ID].toString();
			Helpers::deleteNode(this, moduleId, removedTree);
		}
		else
			throw Helpers::getErrorForNode404(Helpers::getRootTree(this, moduleId), nodeId);
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (removedTree.isValid())
		{
			auto parentNode = Helpers::findNode(rv, parentId);

			parentNode.getChildWithName(PropertyIds::Nodes).addChild(removedTree.createCopy(), oldIndex, nullptr);
		}
	}
};

struct move : public ActionBase
{
	BUILDER_ID(move);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::nodeId].toString().isEmpty())
			return Error().withError("move requires 'nodeId'");
		if (op[RestApiIds::parent].toString().isEmpty())
			return Error().withError("move requires 'parent'");
		return {};
	}

	move(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		nodeId(obj[RestApiIds::nodeId].toString()),
		newParentId(obj[RestApiIds::parent].toString()),
		insertIndex(obj.getProperty(RestApiIds::index, -1))
	{}

	String moduleId;
	String nodeId;
	String newParentId;
	int insertIndex;

	// Stored for undo
	String oldParentId;
	int oldIndex = -1;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = nodeId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : "Move ") + nodeId + " to " + (undo ? oldParentId : newParentId);
	}

	String getDescription() const override
	{
		return "move " + nodeId + " to " + newParentId;
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			return Helpers::getErrorForModule404(getMainController(), moduleId);

		if (dspValidation != nullptr)
		{
			auto n = Helpers::findNode(rv, nodeId);

			if (!n.isValid())
				return Helpers::getErrorForNode404(rv, nodeId);

			auto np = Helpers::findNode(rv, newParentId);

			if (!np.isValid())
				return Helpers::getErrorForNode404(rv, newParentId);

			if (!np.getChildWithName(PropertyIds::Nodes).isValid())
				return Error().withError(newParentId + " is not a container");

			// Mirror the move onto the plan snapshot.
			dspValidation->moveNode(nodeId, newParentId, insertIndex);
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto n = Helpers::findNode(rv, nodeId);

		if (!n.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		auto np = Helpers::findNode(rv, newParentId);

		if (!np.isValid())
			throw Helpers::getErrorForNode404(rv, newParentId);

		if (!np.getChildWithName(PropertyIds::Nodes).isValid())
			throw Error().withError(newParentId + " is not a container");

		oldIndex = n.getParent().indexOf(n);
		oldParentId = n.getParent().getParent()[PropertyIds::ID].toString();

		n.getParent().removeChild(n, nullptr);
		np.getChildWithName(PropertyIds::Nodes).addChild(n, insertIndex, nullptr);

		
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto n = Helpers::findNode(rv, nodeId);

		if (!n.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		auto op = Helpers::findNode(rv, oldParentId);

		if (!op.isValid())
			throw Helpers::getErrorForNode404(rv, oldParentId);

		if (!op.getChildWithName(PropertyIds::Nodes).isValid())
			throw Error().withError(newParentId + " is not a container");

		n.getParent().removeChild(n, nullptr);
		op.getChildWithName(PropertyIds::Nodes).addChild(n, oldIndex, nullptr);
	}
};

struct connect : public ActionBase
{
	BUILDER_ID(connect);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::source].toString().isEmpty())
			return Error().withError("connect requires 'source'");
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("connect requires 'target'");

		// send / receive can be used without parameter
		//if (op[RestApiIds::parameter].toString().isEmpty())
		//	return Error().withError("connect requires 'parameter'");
		return {};
	}

	connect(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		sourceId(obj[RestApiIds::source].toString()),
		targetId(obj[RestApiIds::target].toString()),
		parameterName(obj[RestApiIds::parameter].toString()),
		sourceOutput(obj.getProperty(RestApiIds::sourceOutput, 0).toString()),
		matchRange((bool)obj.getProperty(RestApiIds::matchRange, false))
	{}

	String moduleId;
	String sourceId;
	String targetId;
	String parameterName;
	String sourceOutput;
	bool matchRange;

	// Captured previous source range for undo when matchRange is true
	bool capturedSourceRange = false;
	scriptnode::InvertableParameterRange oldTargetRange;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = targetId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);

		if (matchRange)
		{
			Diff s;
			s.target = sourceId;
			s.domain = Domain::DSP;
			s.type = Diff::Type::Modify;
			diffList.push_back(s);
		}
	}

	String getHistoryMessage(bool undo) const override
	{
		auto base = (undo ? "Disconnect " : "Connect ") + sourceId + "." + sourceOutput + " -> " + targetId + "." + parameterName;
		return matchRange ? (base + " (match range)") : base;
	}

	String getDescription() const override
	{
		auto base = "connect " + sourceId + "." + sourceOutput + " -> " + targetId + "." + parameterName;
		return matchRange ? (base + " (match range)") : base;
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			return Helpers::getErrorForModule404(getMainController(), moduleId);

		if (dspValidation != nullptr)
		{
			auto sn = Helpers::findNode(rv, sourceId);

			if (!sn.isValid())
				throw Helpers::getErrorForNode404(rv, sourceId);

			auto tn = Helpers::findNode(rv, targetId);

			if (!tn.isValid())
				throw Helpers::getErrorForNode404(rv, targetId);

			auto pn = Helpers::findParameterOrProperty(tn, parameterName, false);

			if (!pn.isValid())
				throw Helpers::getErrorForParameter404(tn, parameterName);

			try
			{
				auto conTree = Helpers::getConnectionParent(sn, sourceOutput);

				if (!conTree.isValid())
					return Error().withError("illegal connection source node");

				// Mirror the connection onto the plan snapshot. conTree is already
				// inside the snapshot via getRootTree, so addConnection mutates it.
				if (!Helpers::addConnection(conTree, targetId, parameterName))
					return Error().withError("Connection already exists");

				if (matchRange)
				{
					// TODO: mirror range copy (target.pn -> source parameter) onto the plan snapshot.
					// Reject if source is not a parameter-bearing node with a settable range.
				}

				return {};
			}
			catch (Error& e)
			{
				return e;
			}
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, sourceId);

		if (!sn.isValid())
			throw Helpers::getErrorForNode404(rv, sourceId);

		auto tn = Helpers::findNode(rv, targetId);

		if (!tn.isValid())
			throw Helpers::getErrorForNode404(rv, targetId);

		auto pn = Helpers::findParameterOrProperty(tn, parameterName, false);

		if (!pn.isValid())
			throw Helpers::getErrorForParameter404(tn, parameterName);

		auto conTree = Helpers::getConnectionParent(sn, sourceOutput);

		if(!Helpers::addConnection(conTree, targetId, parameterName))
			throw Error().withError("Connection already exists");

		if (matchRange)
		{
			auto sourceParameter = valuetree::Helpers::findParentWithType(conTree, PropertyIds::Parameter);

			if (!sourceParameter.isValid())
				throw Error().withError("matchRange requires a parameter source");

			if(pn.getType() != PropertyIds::Parameter)
				throw Error().withError("matchRange requires a parameter target");

			oldTargetRange = RangeHelpers::getDoubleRange(pn);
			
			RangeHelpers::storeDoubleRange(sourceParameter, oldTargetRange, nullptr, scriptnode::RangeHelpers::IdSet::scriptnode);
			capturedSourceRange = true;
		}
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, sourceId);

		if (!sn.isValid())
			throw Helpers::getErrorForNode404(rv, sourceId);

		auto tn = Helpers::findNode(rv, targetId);

		if (!tn.isValid())
			throw Helpers::getErrorForNode404(rv, targetId);

		auto pn = Helpers::findParameterOrProperty(tn, parameterName, false);

		if (!pn.isValid())
			throw Helpers::getErrorForParameter404(tn, parameterName);

		

		auto conTree = Helpers::getConnectionParent(sn, sourceOutput);

		if (!Helpers::removeConnection(conTree, targetId, parameterName))
			throw Error().withError("Connection doesn't exist");

		if (matchRange && capturedSourceRange)
		{
			auto sourceParameter = valuetree::Helpers::findParentWithType(conTree, PropertyIds::Parameter);
			RangeHelpers::storeDoubleRange(sourceParameter, oldTargetRange, nullptr);
		}
	}
};

struct disconnect : public ActionBase
{
	BUILDER_ID(disconnect);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::source].toString().isEmpty())
			return Error().withError("disconnect requires 'source'");
		if (op[RestApiIds::target].toString().isEmpty())
			return Error().withError("disconnect requires 'target'");
		if (op[RestApiIds::parameter].toString().isEmpty())
			return Error().withError("disconnect requires 'parameter'");
		return {};
	}

	disconnect(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		sourceId(obj[RestApiIds::source].toString()),
		targetId(obj[RestApiIds::target].toString()),
		parameterName(obj[RestApiIds::parameter].toString())
	{}

	String moduleId;
	String sourceId;
	String targetId;
	String parameterName;
	String oldSourceOutput;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = targetId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Reconnect " : "Disconnect ") + sourceId + " -> " + targetId + "." + parameterName;
	}

	String getDescription() const override
	{
		return "disconnect " + sourceId + " -> " + targetId + "." + parameterName;
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			return Helpers::getErrorForModule404(getMainController(), moduleId);

		if (dspValidation != nullptr)
		{
			auto sn = Helpers::findNode(rv, sourceId);

			if (!sn.isValid())
				return Helpers::getErrorForNode404(rv, sourceId);

			auto con = Helpers::findValueTree(sn, [&](const ValueTree& c)
			{
				return c.getType() == PropertyIds::Connection &&
					c[PropertyIds::NodeId] == targetId &&
					c[PropertyIds::ParameterId] == parameterName;
			});

			if (!con.isValid())
				return Error().withError("Connection not found");

			// Mirror the disconnect onto the plan snapshot. con is already inside
			// the snapshot via getRootTree; removing it mutates the snapshot.
			con.getParent().removeChild(con, nullptr);
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, sourceId);

		if (!sn.isValid())
			throw Helpers::getErrorForNode404(rv, sourceId);

		auto con = Helpers::findValueTree(sn, [&](const ValueTree& c)
		{
			return c.getType() == PropertyIds::Connection &&
				c[PropertyIds::NodeId] == targetId &&
				c[PropertyIds::ParameterId] == parameterName;
		});

		if (!con.isValid())
			throw Error().withError("Connection not found");

		oldSourceOutput = Helpers::getSourceOutput(con);

		con.getParent().removeChild(con, nullptr);
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, sourceId);

		if (!sn.isValid())
			throw Helpers::getErrorForNode404(rv, sourceId);

		auto tn = Helpers::findNode(rv, targetId);

		if (!tn.isValid())
			throw Helpers::getErrorForNode404(rv, targetId);

		auto pn = Helpers::findParameterOrProperty(tn, parameterName, false);

		if (!pn.isValid())
			throw Helpers::getErrorForParameter404(tn, parameterName);

		auto conTree = Helpers::getConnectionParent(sn, oldSourceOutput);

		if (!Helpers::addConnection(conTree, targetId, parameterName))
			throw Error().withError("Connection already exists");
	}
};

struct set : public ActionBase
{
	BUILDER_ID(set);

	static bool isRangeWrite(const var& op)
	{
		return op.hasProperty(RestApiIds::min) || 
			   op.hasProperty(RestApiIds::max) || 
			   op.hasProperty(RestApiIds::middlePosition) ||
			   op.hasProperty(RestApiIds::stepSize) ||
			   op.hasProperty(RestApiIds::skewFactor) ||
			   op.hasProperty(RestApiIds::defaultValue);
	}

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::nodeId].toString().isEmpty())
			return Error().withError("set requires 'nodeId'");
		if (op[RestApiIds::parameterId].toString().isEmpty())
			return Error().withError("set requires 'parameterId'");

		const bool rangeWrite = isRangeWrite(op);
		auto v = op[RestApiIds::value];
		const bool hasValue = !(v.isVoid() || v.isUndefined());

		if (rangeWrite)
		{
			if (hasValue)
				return Error().withError("set op cannot combine 'value' with range fields (min/max) - use two separate ops");

			if (op.hasProperty(RestApiIds::skewFactor) && op.hasProperty(RestApiIds::middlePosition))
				return Error().withError("set range-write: 'skewFactor' and 'middlePosition' are mutually exclusive");

			if (op.hasProperty(RestApiIds::min) && op.hasProperty(RestApiIds::max))
			{
				const double mn = (double)op[RestApiIds::min];
				const double mx = (double)op[RestApiIds::max];
				if (!(mn < mx))
					return Error().withError("set range-write: 'min' must be less than 'max'");
			}

			return {};
		}

		if (!hasValue)
			return Error().withError("set requires 'value'");
		return {};
	}

	set(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		nodeId(obj[RestApiIds::nodeId].toString()),
		parameterId(obj[RestApiIds::parameterId].toString()),
		newValue(obj[RestApiIds::value]),
		rangeWrite(isRangeWrite(obj))
	{
		if (rangeWrite)
		{
			hasMin = obj.hasProperty(RestApiIds::min);
			if(hasMin)
				newMin = (double)obj[RestApiIds::min];

			hasMax = obj.hasProperty(RestApiIds::max);
			if(hasMax)
				newMax = (double)obj[RestApiIds::max];

			hasSkewFactor = obj.hasProperty(RestApiIds::skewFactor);
			if (hasSkewFactor)
				newSkewFactor = (double)obj[RestApiIds::skewFactor];

			hasMiddlePosition = obj.hasProperty(RestApiIds::middlePosition);
			if (hasMiddlePosition)
				newMiddlePosition = (double)obj[RestApiIds::middlePosition];

			hasStepSize = obj.hasProperty(RestApiIds::stepSize);
			if (hasStepSize)
				newStepSize = (double)obj[RestApiIds::stepSize];
		}
	}

	String moduleId;
	String nodeId;
	String parameterId;
	var newValue;
	var oldValue;

	// Range-write state
	bool rangeWrite = false;
	double newMin = 0.0;
	bool hasMin = false;
	double newMax = 1.0;
	bool hasMax = false;
	
	double newSkewFactor = 1.0;
	bool hasSkewFactor = false;
	
	double newMiddlePosition = 0.5;
	bool hasMiddlePosition = false;
	
	double newStepSize = 0.0;
	bool hasStepSize = false;

	// Captured previous range for undo
	double oldMin = 0.0;
	double oldMax = 1.0;
	double oldSkewFactor = 1.0;
	double oldStepSize = 0.0;
	double oldMiddlePosition = 0.5;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = nodeId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		if (rangeWrite)
			return (undo ? "Restore range of " : "Set range of ") + nodeId + "." + parameterId;

		return (undo ? "Restore " : "Set ") + nodeId + "." + parameterId +
			(undo ? "" : (" to " + newValue.toString()));
	}

	String getDescription() const override
	{
		if (rangeWrite)
		{
			String msg;
			msg << "set range " + nodeId + "." + parameterId < " to [";

			if (hasMin)
				msg << "min:" << String(newMin) << " ";

			if (hasMax)
				msg << "min:" << String(newMax) << " ";

			if (hasSkewFactor)
				msg << "min:" << String(newSkewFactor) << " ";

			if (hasMiddlePosition)
				msg << "min:" << String(newMiddlePosition) << " ";

			if (hasStepSize)
				msg << "step:" << String(newStepSize) << " ";

			msg << "]";
			return msg;
		}

		return "set " + nodeId + "." + parameterId + " to " + newValue.toString();
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			return Helpers::getErrorForModule404(getMainController(), moduleId);

		if (dspValidation != nullptr)
		{
			auto n = Helpers::findNode(rv, nodeId);

			if (!n.isValid())
				return Helpers::getErrorForNode404(rv, nodeId);

			auto p = Helpers::findParameterOrProperty(n, parameterId, true);

			if (!p.isValid())
				return Helpers::getErrorForParameter404(n, parameterId);

			if (rangeWrite)
			{
				return writeParameterRange(p, false);
				// TODO: mirror range write onto plan snapshot (p inside rv).
				// Reject if p is not a parameter value tree (e.g. discrete/enum property).
				return {};
			}

			// Mirror perform() onto the plan snapshot. rv here is the snapshot
			// tree, so p is already inside it. The branch matches perform()'s
			// (pre-existing) behavior for network-property vs regular paths.
			if (p.getType() == PropertyIds::Network || p.getType() == PropertyIds::Node)
				p.setProperty(parameterId, newValue, nullptr);
			else
				p.setProperty(PropertyIds::Value, newValue, nullptr);
		}

		return {};
	}

	Error writeParameterRange(ValueTree& v, bool oldValues)
	{
		if (v.getType() != PropertyIds::Parameter)
			return Error().withError("range only settable on parameters");

		auto r = RangeHelpers::getDoubleRange(v, scriptnode::RangeHelpers::IdSet::scriptnode);

		if (!oldValues)
		{
			oldMin = r.rng.start;
			oldMax = r.rng.start;
			oldStepSize = r.rng.start;
			oldSkewFactor = r.rng.start;
			oldMiddlePosition = r.convertFrom0to1(0.5, false);
		}

		if (hasMin)
			r.rng.start = oldValues ? oldMin : newMin;

		if (hasMax)
			r.rng.end = oldValues ? oldMax : newMax;

		if (hasStepSize)
			r.rng.interval = oldValues ? oldStepSize : newStepSize;

		if (hasSkewFactor)
			r.rng.skew = oldValues ? oldSkewFactor : newSkewFactor;

		if (hasMiddlePosition)
			r.rng.setSkewForCentre(oldValues ? oldMiddlePosition : newMiddlePosition);

		RangeHelpers::storeDoubleRange(v, r, nullptr);
		
		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			throw Helpers::getErrorForModule404(getMainController(), moduleId);

		auto n = Helpers::findNode(rv, nodeId);

		if (!n.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		auto p = Helpers::findParameterOrProperty(n, parameterId, true);

		if (!p.isValid())
			throw Helpers::getErrorForParameter404(n, parameterId);

		if (rangeWrite)
		{
			auto ok = writeParameterRange(p, false);

			if (!ok)
				throw ok;

			return;
		}

		if (p.getType() == PropertyIds::Network || p.getType() == PropertyIds::Node)
		{
			oldValue = p[parameterId];
			p.setProperty(parameterId, newValue, nullptr);

			if (parameterId == PropertyIds::Comment.toString())
				rv.setProperty(PropertyIds::ShowComments, true, nullptr);
		}
		else
		{
			oldValue = p[PropertyIds::Value];
			p.setProperty(PropertyIds::Value, newValue, nullptr);
		}
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			throw Helpers::getErrorForModule404(getMainController(), moduleId);

		auto n = Helpers::findNode(rv, nodeId);

		if (!n.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		auto p = Helpers::findParameterOrProperty(n, parameterId, true);

		if (!p.isValid())
			throw Helpers::getErrorForParameter404(n, parameterId);

		if (rangeWrite)
		{
			auto ok = writeParameterRange(p, true);

			if (!ok)
				throw ok;

			
			return;
		}

		if (p.getType() == PropertyIds::Network)
			p.setProperty(parameterId, oldValue, nullptr);
		else
			p.setProperty(PropertyIds::Value, oldValue, nullptr);

	}
};

struct bypass : public ActionBase
{
	BUILDER_ID(bypass);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::nodeId].toString().isEmpty())
			return Error().withError("bypass requires 'nodeId'");
		auto bypassVal = op[RestApiIds::bypassed];
		if (bypassVal.isVoid() || bypassVal.isUndefined())
			return Error().withError("bypass requires 'bypassed'");
		return {};
	}

	bypass(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		nodeId(obj[RestApiIds::nodeId].toString()),
		bypassed((bool)obj[RestApiIds::bypassed])
	{}

	String moduleId;
	String nodeId;
	bool bypassed;
	bool previousBypassed = false;

	int getRebuildLevel(Domain, bool) const override { return 0; }
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = nodeId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Restore " : (bypassed ? "Bypass " : "Unbypass ")) + nodeId;
	}

	String getDescription() const override
	{
		return String(bypassed ? "bypass " : "unbypass ") + nodeId;
	}

	Error validate() override
	{
		if (dspValidation != nullptr)
		{
			auto rv = Helpers::getRootTree(this, moduleId);

			if (!Helpers::findNode(rv, nodeId).isValid())
				return Helpers::getErrorForNode404(rv, nodeId);

			// Mirror onto the plan snapshot so mid-group reads and later ops
			// in the same group observe the accumulated state.
			dspValidation->setNodeProperty(nodeId, PropertyIds::Bypassed, bypassed);
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto n = Helpers::findNode(rv, nodeId);

		if (!n.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		previousBypassed = n[PropertyIds::Bypassed];
		n.setProperty(PropertyIds::Bypassed, bypassed, nullptr);
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		auto n = Helpers::findNode(rv, nodeId);

		if (!n.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		n.setProperty(PropertyIds::Bypassed, previousBypassed, nullptr);
	}
};

struct create_parameter : public ActionBase
{
	BUILDER_ID(create_parameter);

	static Error prevalidate(MainController*, const var& op)
	{
		if (op[RestApiIds::nodeId].toString().isEmpty())
			return Error().withError("create_parameter requires 'nodeId'");
		if (op[RestApiIds::parameterId].toString().isEmpty())
			return Error().withError("create_parameter requires 'parameterId'");
		return {};
	}

	create_parameter(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString()),
		nodeId(obj[RestApiIds::nodeId].toString()),
		parameterId(obj[RestApiIds::parameterId].toString()),
		defaultValue(obj.getProperty(RestApiIds::defaultValue, 0.0))
	{
		auto minValue = (obj.getProperty(RestApiIds::min, 0.0));
		auto maxValue = (obj.getProperty(RestApiIds::max, 1.0));
		auto stepSize = (obj.getProperty(RestApiIds::stepSize, 0.0));

		range = { minValue, maxValue, stepSize };

		if (obj.hasProperty(RestApiIds::skewFactor))
			range.rng.skew = (double)obj[RestApiIds::skewFactor];

		if(obj.hasProperty(RestApiIds::middlePosition))
			range.rng.setSkewForCentre((double)obj[RestApiIds::middlePosition]);
		
		range.inv = (obj.getProperty(RestApiIds::inverted, false));
	}

	scriptnode::InvertableParameterRange range;

	String moduleId;
	String nodeId;
	String parameterId;
	double defaultValue;

	int getRebuildLevel(Domain d, bool undo) const override 
	{ 
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, nodeId);

		if (sn.getParent() == rv)
			return RestServerUndoManager::RebuildLevel::ParameterSlots;
		
		return 0; 
	}
	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = nodeId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Modify;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return (undo ? "Remove parameter " : "Create parameter ") + parameterId + " on " + nodeId;
	}

	String getDescription() const override
	{
		return "create parameter " + parameterId + " on " + nodeId;
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, nodeId);

		if (dspValidation != nullptr)
		{
			if (!sn.isValid())
				return Helpers::getErrorForNode404(rv, nodeId);

			if (!sn[PropertyIds::FactoryPath].toString().startsWith("container"))
				return Error().withError("Can't add parameters to non-container nodes");

			auto pn = Helpers::findParameterOrProperty(sn, parameterId, false);

			if (pn.isValid())
				return Error().withError("Parameter " + nodeId + "." + parameterId + " already exists");

			// Mirror the parameter creation onto the plan snapshot. sn is already
			// inside the snapshot tree (via getRootTree), so we build and insert
			// the Parameter subtree directly.
			auto pTree = sn.getChildWithName(PropertyIds::Parameters);

			ValueTree np(PropertyIds::Parameter);
			np.setProperty(PropertyIds::ID, parameterId, nullptr);
			scriptnode::RangeHelpers::storeDoubleRange(np, range, nullptr);
			np.setProperty(PropertyIds::DefaultValue, defaultValue, nullptr);
			np.setProperty(PropertyIds::Value, defaultValue, nullptr);

			pTree.addChild(np, -1, nullptr);
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, nodeId);

		if (!sn.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		if (!sn[PropertyIds::FactoryPath].toString().startsWith("container"))
			throw Error().withError("Can't add parameters to non-container nodes");

		

		auto pn = Helpers::findParameterOrProperty(sn, parameterId, false);

		if (pn.isValid())
			throw Error().withError("Parameter " + nodeId + "." + parameterId + " already exists");

		sn.setProperty(PropertyIds::ShowParameters, true, nullptr);

		auto pTree = sn.getChildWithName(PropertyIds::Parameters);

		ValueTree np(PropertyIds::Parameter);

		np.setProperty(PropertyIds::ID, parameterId, nullptr);
		scriptnode::RangeHelpers::storeDoubleRange(np, range, nullptr);
		np.setProperty(PropertyIds::DefaultValue, defaultValue, nullptr);
		np.setProperty(PropertyIds::Value, defaultValue, nullptr);

		pTree.addChild(np, -1, nullptr);
	}

	void undo() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);
		auto sn = Helpers::findNode(rv, nodeId);

		if (!sn.isValid())
			throw Helpers::getErrorForNode404(rv, nodeId);

		auto pn = Helpers::findParameterOrProperty(sn, parameterId, false);

		if (!pn.isValid())
			throw Helpers::getErrorForParameter404(sn, parameterId);

		pn.getParent().removeChild(pn, nullptr);
	}
};

struct clear : public ActionBase
{
	BUILDER_ID(clear);

	static Error prevalidate(MainController*, const var&)
	{
		return {};
	}

	clear(MainController* mc, const var& obj) :
		ActionBase(mc),
		moduleId(obj[RestApiIds::moduleId].toString())
	{}

	String moduleId;
	ValueTree savedState; // For undo: snapshot before clearing

	int getRebuildLevel(Domain, bool) const override 
	{ 
		return RebuildLevel::ParameterSlots; 
	}

	bool needsKillVoice() const override { return false; }

	void addToDiffList(std::vector<Diff>& diffList, bool) override
	{
		Diff d;
		d.target = moduleId;
		d.domain = Domain::DSP;
		d.type = Diff::Type::Remove;
		diffList.push_back(d);
	}

	String getHistoryMessage(bool undo) const override
	{
		return undo ? ("Restore network " + moduleId) : ("Clear network " + moduleId);
	}

	String getDescription() const override
	{
		return "clear network " + moduleId;
	}

	Error validate() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			return Helpers::getErrorForModule404(getMainController(), moduleId);

		if (dspValidation != nullptr)
		{
			// Mirror the clear onto the plan snapshot. The snapshot root Node is
			// the first child of the Network tree; its Nodes child holds all
			// children. Removing them matches what a fresh network looks like for
			// downstream ops (and for mid-group GET ?group=current reads).
			auto rootNode = rv.getChild(0);
			auto nodesChild = rootNode.getChildWithName(PropertyIds::Nodes);
			if (nodesChild.isValid())
				nodesChild.removeAllChildren(nullptr);
		}

		return {};
	}

	void perform() override
	{
		auto rv = Helpers::getRootTree(this, moduleId);

		if (!rv.isValid())
			throw Helpers::getErrorForModule404(getMainController(), moduleId);

		auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), moduleId);

		if (auto h = dynamic_cast<DspNetwork::Holder*>(p))
		{
			savedState = Helpers::getRootTree(this, moduleId);
			h->clearAllNetworks();
		}
	}

	void undo() override
	{
		auto p = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), moduleId);

		if (auto h = dynamic_cast<DspNetwork::Holder*>(p))
		{
			h->getOrCreate(savedState);
			p->prepareToPlay(p->getSampleRate(), p->getLargestBlockSize());
		}
	}
};

} // dsp
} // rest_undo
} // hise
