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

#if LOG_REFERENCE_CHECKS
#define LOG_REFERENCE_CHECK(x) DBG(x)
#else
#define LOG_REFERENCE_CHECK(x)
#endif


HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::Reference(const var& parent_, const var& child_, Identifier parentId_, Identifier childId_) :
	parent(parent_),
	child(child_),
	parentId(parentId_),
	childId(childId_),
	description(toString())
{

}

HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::Reference(const Reference& other) :
	parent(other.parent),
	child(other.child),
	parentId(other.parentId),
	childId(other.childId),
	description(other.description)
{

}

String HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::toString() const
{
	if (isEmpty())
		return String();

	String ref;

	ref << "Reference: " << parentId << " -> " << childId;

	return ref;
}

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::isEmpty() const
{
	return parent == var() && child == var() && parentId == Identifier() && childId == Identifier();
}


bool HiseJavascriptEngine::CyclicReferenceCheckBase::updateList(ThreadData& data, const var& varToCheck, const Identifier& parentId)
{
	data.overflowProtection++;

	data.numChecked++;

	LOG_REFERENCE_CHECK("Checking all references from " + parentId.toString());
	LOG_REFERENCE_CHECK("==================================================================");

	if (data.overflowProtection > 200)
	{
		LOG_REFERENCE_CHECK("updateList overflow");
		data.overflowHit = true;
		return false;
	}

	if (auto cc = dynamic_cast<CyclicReferenceCheckBase*>(varToCheck.getObject()))
	{
		if (!cc->updateCyclicReferenceList(data, parentId))
			return false;
	}
	if (auto obj = varToCheck.getDynamicObject())
	{
		auto set = obj->getProperties();

		for (int i = 0; i < set.size(); i++)
		{
			auto v = set.getValueAt(i);
			const String name = set.getName(i).toString();

			if (!Reference::ListHelpers::varHasReferences(v))
			{
				LOG_REFERENCE_CHECK("  Skipping variable " + name + ": no references");
				continue;
			}

			Identifier pIdToUse = Reference::ListHelpers::getIdWithParent(parentId, name, true);

			data.coallescateOverflowProtection = 0;

			if (!Reference::ListHelpers::addAllReferencesWithTarget(varToCheck, parentId, v, pIdToUse, data))
				return false;

			if (!updateList(data, v, pIdToUse))
				return false;

		}
	}
	else if (auto ar = varToCheck.getArray())
	{
		for (int i = 0; i < ar->size(); i++)
		{
			auto v = ar->getUnchecked(i);
			const String name = String(i);

			if (!Reference::ListHelpers::varHasReferences(v))
			{
				continue;
			}

			Identifier pIdToUse = Reference::ListHelpers::getIdWithParent(parentId, name, false);

			data.coallescateOverflowProtection = 0;

			if (!Reference::ListHelpers::addAllReferencesWithTarget(varToCheck, parentId, v, pIdToUse, data))
				return false;

			if (!updateList(data, v, pIdToUse))
				return false;
		}

	}
	

	data.overflowProtection--;

	return true;
}


bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::addAllReferencesWithTarget(const var& sourceVar, const Identifier& sourceId, const var& targetVar, const Identifier& targetId, ThreadData& data)
{
	LOG_REFERENCE_CHECK("  Add all references with target " + targetId.toString() + " (source is " + sourceId.toString() + ")");

	data.coallescateOverflowProtection++;

	for (auto ref : data.referenceList)
	{
		LOG_REFERENCE_CHECK("    Checking " + ref.toString());

		if (ListHelpers::checkEqualitySafe(ref.child, sourceVar))
		{
			Reference r(ref.parent, targetVar, ref.parentId, targetId);

			if (r.isCyclicReference())
			{
				LOG_REFERENCE_CHECK("      Cyclic " + r.toString() + " found.");
				data.referenceList.clearQuick();
				
				data.cyclicReferenceString = r.description;
				return false;
			}

			if (!checkIfExist(data.referenceList, r))
			{
				LOG_REFERENCE_CHECK("      Coallescating reference for target " + targetId.toString() + " with source " + ref.parentId.toString());
				data.referenceList.add(r);
			}
		}

	}

	Reference r(sourceVar, targetVar, sourceId, targetId);

	if (!checkIfExist(data.referenceList, r))
	{
		LOG_REFERENCE_CHECK("    Adding " + r.toString());
		data.referenceList.add(r);
	}

	data.coallescateOverflowProtection--;

	return true;
}

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::checkIfExist(const List& references, const Reference& referenceToCheck)
{
	for (auto existingRef : references)
	{
		if (referenceToCheck.equals(existingRef))
			return true;
	}


	return false;
}

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::checkEqualitySafe(const var& a, const var& b)
{
	if (a.isArray() && b.isArray())
	{
		return a.getArray()->getRawDataPointer() == b.getArray()->getRawDataPointer();
	}
	else
	{
		return a == b;
	}
}


bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::isVarWithReferences(const var &v)
{
	return (v.isObject() || v.isArray());
}

Identifier HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::getIdWithParent(const Identifier &parentId, const String& name, bool isObject)
{
	if (isObject)
	{
		return Identifier(parentId.toString() + "." + name);
	}
	else
	{
		return Identifier(parentId.toString() + "[" + name + "]");
	}
}

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::varHasReferences(const var& v)
{
	if (dynamic_cast<ScriptingApi::Content::ScriptPanel*>(v.getObject()) != nullptr)
		return true;

	if (!isVarWithReferences(v))
		return false;

	if (dynamic_cast<HiseJavascriptEngine::CyclicReferenceCheckBase*>(v.getObject()) != nullptr)
	{
		return true;
	}
	else if (auto obj = v.getDynamicObject())
	{
		auto set = obj->getProperties();

		for (int i = 0; i < set.size(); i++)
		{
			if (isVarWithReferences(set.getValueAt(i)))
				return true;
		}
	}
	else if (auto ar = v.getArray())
	{
		for (auto v_ : *ar)
		{
			if (isVarWithReferences(v_))
				return true;
		}

	}

	return false;
}


bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::equals(const Reference& other) const
{
	return ListHelpers::checkEqualitySafe(child, other.child) && ListHelpers::checkEqualitySafe(parent, other.parent);
}

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::isCyclicReference() const
{
	if (isEmpty())
		return false;

	bool isCycle = ListHelpers::checkEqualitySafe(child, parent);

	LOG_REFERENCE_CHECK("     checking for cycle with " + parentId.toString() + " to " + childId.toString() + ": " + (isCycle ? "true" : "false"));

	return isCycle;
}

bool HiseJavascriptEngine::checkCyclicReferences(CyclicReferenceCheckBase::ThreadData& data, const Identifier& id)
{
	
	return root->updateCyclicReferenceList(data, id);
}


bool HiseJavascriptEngine::RootObject::updateCyclicReferenceList(ThreadData& data, const Identifier& /*id*/)
{
	data.thread->showStatusMessage("Checking root variables");

	auto set = getProperties();

	for (int i = 0; i < set.size(); i++)
	{
		if (!updateList(data, set.getValueAt(i), set.getName(i)))
			return false;

		if (data.thread->threadShouldExit())
			return false;
	}

	if (!hiseSpecialData.updateCyclicReferenceList(data, Identifier("rootNamespace")))
		return false;

	return true;
}

bool HiseJavascriptEngine::RootObject::JavascriptNamespace::updateCyclicReferenceList(ThreadData& data, const Identifier& parentId)
{
	auto nsString = id.toString() + ".";

	data.thread->showStatusMessage("Checking namespace " + parentId.toString());

	for (int i = 0; i < constObjects.size(); i++)
	{
		auto n_ = constObjects.getName(i);

		if (!updateList(data, constObjects.getValueAt(i), Identifier(nsString + n_)))
			return false;

		if (data.thread->threadShouldExit())
			return false;
		
	}
	
	for (int i = 0; i < varRegister.getNumUsedRegisters(); i++)
	{
		auto n_ = varRegister.getRegisterId(i).toString();

		if (!updateList(data, varRegister.getFromRegister(i), Identifier(nsString + n_)))
			return false;

		if (data.thread->threadShouldExit())
			return false;
	}

	for (int i = 0; i < inlineFunctions.size(); i++)
	{
		auto f = dynamic_cast<InlineFunction::Object*>(inlineFunctions[i].get());

		if (!f->updateCyclicReferenceList(data, f->name))
			return false;
		
		if (data.thread->threadShouldExit())
			return false;
	}

	return true;
}


bool HiseJavascriptEngine::RootObject::HiseSpecialData::updateCyclicReferenceList(ThreadData& data, const Identifier &parentId)
{
	data.thread->showStatusMessage("Checking root namespace");

	if (!JavascriptNamespace::updateCyclicReferenceList(data, parentId))
	{
		return false;
	}

	if (data.thread->threadShouldExit())
		return false;

	for (int i = 0; i < namespaces.size(); i++)
	{
		*data.progress = (double)i / (double)namespaces.size();

		if (!namespaces[i]->updateCyclicReferenceList(data, namespaces[i]->id))
			return false;

		if (data.thread->threadShouldExit())
			return false;
	}

	return true;
}


bool HiseJavascriptEngine::RootObject::FunctionObject::updateCyclicReferenceList(ThreadData& data, const Identifier& id)
{
	if (auto obj = lastScopeForCycleCheck.getDynamicObject())
	{
		if (!Reference::ListHelpers::addAllReferencesWithTarget(this, id, lastScopeForCycleCheck, Identifier("scope"), data))
			return false;

		obj->removeProperty(Identifier("this"));

		if (!updateList(data, lastScopeForCycleCheck, id))
			return false;

		if (data.thread->threadShouldExit())
			return false;
	}

	return true;
}

bool ScriptingApi::Content::ScriptPanel::updateCyclicReferenceList(ThreadData& data, const Identifier& id)
{
	var thisAsVar(this);

	auto pString = id.toString() + ".";

	auto pDataId = Identifier(pString + "data");
	auto pJsonId = Identifier(pString + "popupData");

	auto pData = getConstantValue(0);
	auto pJson = jsonPopupData;

	if (!Reference::ListHelpers::addAllReferencesWithTarget(thisAsVar, getName(), pData, pDataId, data))
		return false;

	if (!updateList(data, pData, pDataId))
		return false;

	if (data.thread->threadShouldExit())
		return false;

	if (!Reference::ListHelpers::addAllReferencesWithTarget(thisAsVar, getName(), pJson, pJsonId, data))
		return false;

	if (!updateList(data, pJson, pJsonId))
		return false;

	if (data.thread->threadShouldExit())
		return false;

	return true;
}

bool HiseJavascriptEngine::RootObject::InlineFunction::Object::updateCyclicReferenceList(ThreadData& data, const Identifier &id)
{
	for (int i = 0; i < localProperties->size(); i++)
	{
		auto lId = Identifier(id.toString() + "." + localProperties->getName(i).toString());

		if (!updateList(data, localProperties->getValueAt(i), lId))
		{
			enableCycleCheck = false;
			cleanLocalProperties();
			return false;
		}
			
		if (data.thread->threadShouldExit())
		{
			enableCycleCheck = false;
			cleanLocalProperties();
			return false;
		}	
	}

	enableCycleCheck = false;
	cleanLocalProperties();
	return true;
}


void HiseJavascriptEngine::RootObject::prepareCycleReferenceCheck()
{
#if ENABLE_SCRIPTING_SAFE_CHECKS
	auto set = getProperties();

	for (int i = 0; i < set.size(); i++)
	{
		if (auto c = dynamic_cast<CyclicReferenceCheckBase*>(set.getValueAt(i).getObject()))
			c->prepareCycleReferenceCheck();
	}

	hiseSpecialData.prepareCycleReferenceCheck();
#endif
}

HiseJavascriptEngine::RootObject::ScopedLocalThisObject::ScopedLocalThisObject(RootObject& r_, const var& newObject):
	r(r_)
{
	if (!newObject.isUndefined())
	{
		prevObject = r.localThreadThisObject.get();
		r.localThreadThisObject = newObject;
	}
}

HiseJavascriptEngine::RootObject::ScopedLocalThisObject::~ScopedLocalThisObject()
{
	if (!prevObject.isUndefined())
	{
		r.localThreadThisObject = prevObject;
	}
}

HiseJavascriptEngine::RootObject::LocalScopeCreator::ScopedSetter::ScopedSetter(
	ReferenceCountedObjectPtr<RootObject> r_, LocalScopeCreator::Ptr p):
	r(r_.get())
{
#if ENABLE_SCRIPTING_BREAKPOINTS
	auto isMessageThread = MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread();

	if (!isMessageThread)
	{
		auto& cp = r->currentLocalScopeCreator.get();
		prevValue = p;
		std::swap(prevValue, cp);
		ok = true;
	}
#endif
}

HiseJavascriptEngine::RootObject::LocalScopeCreator::ScopedSetter::~ScopedSetter()
{
#if ENABLE_SCRIPTING_BREAKPOINTS
	if (ok)
	{
		auto& cp = r->currentLocalScopeCreator.get();
		std::swap(cp, prevValue);
	}
#endif
}

void HiseJavascriptEngine::RootObject::HiseSpecialData::prepareCycleReferenceCheck()
{
	JavascriptNamespace::prepareCycleReferenceCheck();

	for (int i = 0; i < namespaces.size(); i++)
	{
		namespaces[i]->prepareCycleReferenceCheck();
	}
}

void HiseJavascriptEngine::RootObject::JavascriptNamespace::prepareCycleReferenceCheck()
{
	for (int i = 0; i < varRegister.getNumUsedRegisters(); i++)
	{
		if (auto c = dynamic_cast<CyclicReferenceCheckBase*>(varRegister.getFromRegister(i).getObject()))
			c->prepareCycleReferenceCheck();
	}

	for (int i = 0; i < constObjects.size(); i++)
	{
		if (auto c = dynamic_cast<CyclicReferenceCheckBase*>(constObjects.getValueAt(i).getObject()))
			c->prepareCycleReferenceCheck();
	}

	for (auto i: inlineFunctions)
	{
		auto f = dynamic_cast<CyclicReferenceCheckBase*>(i);
		f->prepareCycleReferenceCheck();
	}
}

void HiseJavascriptEngine::RootObject::FunctionObject::prepareCycleReferenceCheck()
{
	enableCycleCheck = true;
}

void HiseJavascriptEngine::RootObject::InlineFunction::Object::prepareCycleReferenceCheck()
{
	enableCycleCheck = true;
}

void ScriptingApi::Content::ScriptPanel::prepareCycleReferenceCheck()
{

}

#undef LOG_REFERENCE_CHECK

} // namespace hise