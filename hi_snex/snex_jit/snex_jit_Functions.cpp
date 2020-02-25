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



bool Symbol::operator==(const Symbol& other) const
{
	if (other.fullIdList.size() == fullIdList.size())
	{
		for (int i = 0; i < fullIdList.size(); i++)
		{
			if (fullIdList[i] != other.fullIdList[i])
				return false;
		}

		return true;
	}

	return false;
}


snex::jit::Symbol Symbol::createRootSymbol(const Identifier& id)
{
	return Symbol({ id }, Types::ID::Dynamic, true, false);
}

Symbol::Symbol(const Array<Identifier>& list, Types::ID t_, bool isConst_, bool isRef_) :
	fullIdList(list),
	id(list.getLast()),
	type(t_),
	const_(isConst_),
	ref_(isRef_)
{
	debugName = toString().toStdString();
}


Symbol::Symbol()
{
	debugName = toString().toStdString();
}

bool Symbol::matchesIdAndType(const Symbol& other) const
{
	return other == *this;// && other.type == type;
}

Symbol Symbol::getParentSymbol() const
{
	Array<Identifier> parentList;

	for (int i = 0; i < fullIdList.size() - 1; i++)
		parentList.add(fullIdList[i]);

	if (parentList.isEmpty())
		return {};

	return Symbol(parentList, type, true, false);
}

snex::jit::Symbol Symbol::getChildSymbol(const Identifier& id, Types::ID newType, bool isConst_, bool isReference_) const
{
	Array<Identifier> childList;

	childList.addArray(fullIdList);
	childList.add(id);
	return Symbol(childList, newType != Types::ID::Dynamic ? newType : type, isConst_, isReference_);
}

snex::jit::Symbol Symbol::withParent(const Symbol& parent) const
{
	Array<Identifier> newList;

	newList.addArray(parent.fullIdList);
	newList.addArray(fullIdList);

	return Symbol(newList, type, const_, ref_);
}

Symbol Symbol::withType(const Types::ID type) const
{
	auto s = *this;
	s.type = type;
	return s;
}

Symbol Symbol::withComplexType(ComplexType::Ptr typePtr) const
{
	auto s = *this;
	s.type = Types::ID::Pointer;
	s.typePtr = typePtr;

	return s;
}

Symbol Symbol::relocate(const Symbol& newParent) const
{
	return newParent.getChildSymbol(id, type, const_, ref_);
}

bool Symbol::isParentOf(const Symbol& otherSymbol) const
{
	for (int i = 0; i < fullIdList.size(); i++)
	{
		if (otherSymbol.fullIdList[i] != fullIdList[i])
			return false;
	}

	return true;
}

juce::String Symbol::toString() const
{
	if (id.isNull())
		return "undefined";

	juce::String s;

	if (!constExprValue.isVoid())
		s << "constexpr";
	else if (const_)
		s << "const";

	if (ref_)
		s << "&";

	if(s.isNotEmpty())
		s << " ";

	for (int i = 0; i < fullIdList.size() - 1; i++)
	{
		s << fullIdList[i].toString() << ".";
	}

	s << id;

	return s;
}

Symbol::operator bool() const
{
	return id.isValid();
}



juce::String FunctionData::getSignature(const Array<Identifier>& parameterIds) const
{
	juce::String s;

	s << Types::Helpers::getCppTypeName(returnType) << " " << id << "(";

	int index = 0;

	for (auto arg : args)
	{
		s << Types::Helpers::getCppTypeName(arg.type);

		if (arg.isAlias)
			s << "&";

		auto pName = parameterIds[index].toString();

		if (pName.isEmpty())
			pName = arg.parameterName;

		if (pName.isNotEmpty())
			s << " " << pName;

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
		if (args[i].type != typeList[i])
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
		if (!(args[i] == otherFunctionData.args[i]))
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

bool FunctionClass::hasFunction(const Symbol& s) const
{
	if (getClassName() == s)
		return true;

	auto parent = s.getParentSymbol();

	if (parent == classSymbol)
	{
		for (auto f : functions)
			if (f->id == s.id)
				return true;
	}

	for (auto c : registeredClasses)
	{
		if (c->hasFunction(s))
			return true;
	}

	return false;
}


bool FunctionClass::hasConstant(const Symbol& s) const
{
	auto parent = s.getParentSymbol();

	if (parent == classSymbol)
	{
		for (auto& c : constants)
			if (c.id == s.id)
				return true;
	}

	for (auto c : registeredClasses)
	{
		if (c->hasConstant(s))
			return true;
	}

	return false;
}

void FunctionClass::addFunctionConstant(const Identifier& constantId, VariableStorage value)
{
	constants.add({ constantId, value });
}

void FunctionClass::addMatchingFunctions(Array<FunctionData>& matches, const Symbol& symbol) const
{
	auto parent = symbol.getParentSymbol();

	if (parent == classSymbol)
	{
		for (auto f : functions)
		{
			if (f->id == symbol.id)
				matches.add(*f);
		}
	}
	else
	{
		for (auto c : registeredClasses)
			c->addMatchingFunctions(matches, symbol);
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
					if (!Types::Helpers::matchesTypeLoose(fArgs[0].type, dArgs[0].type))
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

snex::VariableStorage FunctionClass::getConstantValue(const Symbol& s) const
{
	auto parent = s.getParentSymbol();

	if (parent == classSymbol)
	{
		for (auto& c : constants)
		{
			if (c.id == s.id)
				return c.value;
		}
	}

	for (auto r : registeredClasses)
	{
		auto v = r->getConstantValue(s);

		if (!v.isVoid())
			return v;
	}

	return {};
}

} // end namespace jit
} // end namespace snex
