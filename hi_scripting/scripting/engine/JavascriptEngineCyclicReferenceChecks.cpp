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



bool isVarWithReferences(const var &v)
{
	return (v.isObject() || v.isArray());
}


Identifier getIdWithParent(const Identifier &parentId, const String& name, bool isObject)
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

bool varHasReferences(const var& v)
{
	if (auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(v.getObject()))
		return true;

	if (!isVarWithReferences(v))
		return false;

	if (auto obj = v.getDynamicObject())
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



bool HiseJavascriptEngine::CyclicReferenceCheckBase::updateList(Reference::List& references, const var& varToCheck, const Identifier& parentId)
{
	overflowProtection++;

	DBG("Checking all references from " + parentId.toString());
	DBG("==================================================================");

	if (overflowProtection > 200)
	{
		DBG("updateList overflow");
		return false;
	}

	if (auto obj = varToCheck.getDynamicObject())
	{
		auto set = obj->getProperties();

		for (int i = 0; i < set.size(); i++)
		{
			auto v = set.getValueAt(i);
			const String name = set.getName(i).toString();

			if (!varHasReferences(v))
			{
				DBG("  Skipping variable " + name + ": no references");
				continue;
			}


			Identifier pIdToUse = getIdWithParent(parentId, name, true);

			if (!Reference::ListHelpers::addAllReferencesWithTarget(varToCheck, parentId, v, pIdToUse, references))
				return false;

			Reference::ListHelpers::overFlowProtection = 0;

			if (!updateList(references, v, pIdToUse))
				return false;

		}
	}
	else if (auto ar = varToCheck.getArray())
	{
		for (int i = 0; i < ar->size(); i++)
		{
			auto v = ar->getUnchecked(i);
			const String name = String(i);

			if (!varHasReferences(v))
			{
				continue;
			}

			Identifier pIdToUse = getIdWithParent(parentId, name, false);

			if (!Reference::ListHelpers::addAllReferencesWithTarget(varToCheck, parentId, v, pIdToUse, references))
				return false;

			Reference::ListHelpers::overFlowProtection = 0;

			if (!updateList(references, v, pIdToUse))
				return false;
		}

	}
	else if (auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(varToCheck.getObject()))
	{
		auto pString = parentId.toString() + ".";

		auto pData = Identifier(pString + "data");
		auto pJSON = Identifier(pString + "popupData");

		if (!Reference::ListHelpers::addAllReferencesWithTarget(varToCheck, parentId, panel->getDataObject(), pData, references))
			return false;
		
		if (!updateList(references, panel->getDataObject(), Identifier(pString + "data")))
			return false;

		if (!Reference::ListHelpers::addAllReferencesWithTarget(varToCheck, parentId, panel->getDataObject(), pData, references))
			return false;

		if (!updateList(references, panel->getJSONPopupData(), Identifier(pString + "popupData")))
			return false;
	}

	overflowProtection--;

	return true;
}



int HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::overFlowProtection = 0;

int HiseJavascriptEngine::CyclicReferenceCheckBase::overflowProtection = 0;

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::ListHelpers::addAllReferencesWithTarget(const var& sourceVar, const Identifier& sourceId, const var& targetVar, const Identifier& targetId, List& references)
{
	DBG("  Add all references with target " + targetId.toString() + " (source is " + sourceId.toString() + ")");

	overFlowProtection++;

	for (auto ref : references)
	{
		DBG("    Checking " + ref.toString());

		if (ListHelpers::checkEqualitySafe(ref.child, sourceVar))
		{
			Reference r(ref.parent, targetVar, ref.parentId, targetId);

			if (r.isCyclicReference())
			{
				DBG("      Cyclic " + r.toString() + " found.");
				references.add(r);
				return false;
			}

			if (!checkIfExist(references, r))
			{
				DBG("      Coallescating reference for target " + targetId.toString() + " with source " + ref.parentId.toString());
				references.add(r);
			}
		}

	}

	Reference r(sourceVar, targetVar, sourceId, targetId);

	if (!checkIfExist(references, r))
	{
		DBG("    Adding " + r.toString());
		references.add(r);
	}

	overFlowProtection--;

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

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::equals(const Reference& other) const
{
	return ListHelpers::checkEqualitySafe(child, other.child) && ListHelpers::checkEqualitySafe(parent, other.parent);
}

bool HiseJavascriptEngine::CyclicReferenceCheckBase::Reference::isCyclicReference() const
{
	bool isCycle = ListHelpers::checkEqualitySafe(child, parent);

	DBG("     checking for cycle with " + parentId.toString() + " to " + childId.toString() + ": " + (isCycle ? "true" : "false"));

	return isCycle;
}

bool HiseJavascriptEngine::checkCyclicReferences(CyclicReferenceCheckBase::Reference::List& references)
{
	return root->updateCyclicReferenceList(references);
}


bool HiseJavascriptEngine::RootObject::updateCyclicReferenceList(Reference::List& references)
{
	overflowProtection = 0;

	if (!updateList(references, var(this), Identifier("root")))
		return false;

	if (!hiseSpecialData.updateCyclicReferenceList(references))
		return false;

	return true;
}

bool HiseJavascriptEngine::RootObject::JavascriptNamespace::updateCyclicReferenceList(Reference::List& references)
{
	auto nsString = id.toString() + ".";

	for (int i = 0; i < constObjects.size(); i++)
	{
		auto n_ = constObjects.getName(i);

		if (!updateList(references, constObjects.getValueAt(i), Identifier(nsString + n_)))
			return false;
	}
	
	for (int i = 0; i < varRegister.getNumUsedRegisters(); i++)
	{
		auto n_ = varRegister.getRegisterId(i).toString();

		if (!updateList(references, varRegister.getFromRegister(i), Identifier(nsString + n_)))
			return false;
	}

	return true;
}


bool HiseJavascriptEngine::RootObject::HiseSpecialData::updateCyclicReferenceList(CyclicReferenceCheckBase::Reference::List& references)
{
	if (!JavascriptNamespace::updateCyclicReferenceList(references))
	{
		return false;
	}

	for (int i = 0; i < namespaces.size(); i++)
	{
		if (!namespaces[i]->updateCyclicReferenceList(references))
			return false;
	}

	return true;
}

