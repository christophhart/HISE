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

namespace snex {
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
	if (checkReturnType && !Types::Helpers::matchesType(returnType, otherFunctionData.returnType))
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


bool FunctionData::matchesArgumentTypes(Types::ID r, const Array<Types::ID>& argsList)
{
	if (r != returnType)
		return false;

	return matchesArgumentTypes(argsList);
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


bool FunctionClass::hasConstant(const Identifier& classId, const Identifier& constantId) const
{
	if (classId != className)
	{
		for (auto c : registeredClasses)
		{
			if (c->hasConstant(classId, constantId))
				return true;
		}

		return false;
	}

	for (auto& c : constants)
		if (c.id == constantId)
			return true;

	return false;
}

void FunctionClass::addFunctionConstant(const Identifier& constantId, VariableStorage value)
{
	constants.add({ constantId, value });
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


juce::Array<juce::Identifier> FunctionClass::getFunctionIds() const
{
	Array<Identifier> ids;

	for (auto r : functions)
		ids.add(r->id);

	return ids;
}

bool FunctionClass::fillJitFunctionPointer(FunctionData& dataWithoutPointer)
{
	// first check strict typing
	for (auto f : functions)
	{
		if (f->id == dataWithoutPointer.id && f->matchesArgumentTypes(dataWithoutPointer, false))
		{
			dataWithoutPointer.function = f->function;

			return dataWithoutPointer.function != nullptr;

		}
	}

	for (auto f : functions)
	{
		if (f->id == dataWithoutPointer.id)
		{
			auto& fArgs = f->args;
			auto& dArgs = dataWithoutPointer.args;

			if (fArgs.size() == dArgs.size())
			{
				for (int i = 0; i < fArgs.size(); i++)
				{
					if (!Types::Helpers::matchesTypeLoose(fArgs[0], dArgs[0]))
						return false;
				}

				dataWithoutPointer.function = f->function;
				return true;
			}
		}
	}

	return false;
}


bool FunctionClass::injectFunctionPointer(FunctionData& dataToInject)
{
	for (auto f : functions)
	{
		if (f->id == dataToInject.id && f->matchesArgumentTypes(dataToInject, true))
		{
			f->function = dataToInject.function;
			return true;
		}
	}

	return false;
}

snex::VariableStorage FunctionClass::getConstantValue(const Identifier& classId, const Identifier& constantId) const
{
	if (className != classId)
	{
		for (auto r : registeredClasses)
		{
			auto v = r->getConstantValue(classId, constantId);

			if (!v.isVoid())
				return v;
		}

		return {};
	}

	for (auto& c : constants)
	{
		if (c.id == constantId)
			return c.value;
	}

	return {};
}

} // end namespace jit
} // end namespace snex
