/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hnode {
namespace jit {
using namespace juce;
using namespace asmjit;



juce::String FunctionData::getSignature(const Array<Identifier>& parameterIds) const
{
	String s;

	s << Types::Helpers::getCppTypeName(returnType) << " " << id << "(";

	int index = 0;

	for (auto arg : args)
	{
		s << Types::Helpers::getCppTypeName(arg);

		if (parameterIds[index].isValid())
			s << " " << parameterIds[index].toString();

		if (++index != args.size())
			s << ", ";
	}

	s << ")";

	return s;
}

bool FunctionData::matchesArgumentTypes(const Array<Types::ID>& typeList)
{
	if (args.size() != typeList.size())
		return false;

	for (int i = 0; i < args.size(); i++)
	{
		if (args[i] != typeList[i])
			return false;
	}

	return true;
}


bool FunctionData::matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType /*= true*/) const
{
	if (!Types::Helpers::matchesType(returnType, otherFunctionData.returnType))
		return false;

	if (args.size() != otherFunctionData.args.size())
		return false;

	for (int i = 0; i < args.size(); i++)
	{
		if (args[i] != otherFunctionData.args[i])
			return false;
	}

	return true;
}


bool FunctionClass::hasFunction(const Identifier& classId, const Identifier& functionId) const
{
	if (classId != className)
	{
		for (auto c : registeredClasses)
		{
			if (c->hasFunction(classId, functionId))
				return true;
		}

		return false;
	}


	for (auto f : functions)
		if (f->id == functionId)
			return true;

	return false;
}


void FunctionClass::addMatchingFunctions(Array<FunctionData>& matches, const Identifier& classId, const Identifier& functionId) const
{
	if (classId != className)
	{
		for (auto c : registeredClasses)
			c->addMatchingFunctions(matches, classId, functionId);

		return;
	}

	for (auto f : functions)
	{
		if (f->id == functionId)
			matches.add(*f);
	}
}


void FunctionClass::addFunctionClass(FunctionClass* newRegisteredClass)
{
	registeredClasses.add(newRegisteredClass);
}


void FunctionClass::addFunction(FunctionData* newData)
{
	functions.add(newData);
}


bool FunctionClass::fillJitFunctionPointer(FunctionData& dataWithoutPointer)
{
	for (auto f : functions)
	{
		if (f->id == dataWithoutPointer.id)
		{
			dataWithoutPointer.function = f->function;
			return true;
		}
	}

	return false;
}


bool FunctionClass::injectFunctionPointer(Identifier& id, void* funcPointer)
{
	for (auto f : functions)
	{
		if (f->id == id)
		{
			f->function = funcPointer;
			return true;
		}
	}

	return false;
}

} // end namespace jit
} // end namespace hnode
