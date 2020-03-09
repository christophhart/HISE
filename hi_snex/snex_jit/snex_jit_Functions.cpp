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
	typeInfo(t_, isConst_, isRef_)
{
	debugName = toString().toStdString();
}


Symbol::Symbol()
{
	debugName = toString().toStdString();
}

Symbol::Symbol(const Array<Identifier>& ids, const TypeInfo& info):
	fullIdList(ids),
	id(ids.getLast()),
	typeInfo(info)
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

	return Symbol(parentList, typeInfo.getType(), true, false);
}

snex::jit::Symbol Symbol::getChildSymbol(const Identifier& id, const TypeInfo& t) const
{
	Array<Identifier> childList;

	childList.addArray(fullIdList);
	childList.add(id);

	return Symbol(childList, t.isValid() ? t : typeInfo);
}

snex::jit::Symbol Symbol::withParent(const Symbol& parent) const
{
	Array<Identifier> newList;

	newList.addArray(parent.fullIdList);
	newList.addArray(fullIdList);

	return Symbol(newList, typeInfo);
}

Symbol Symbol::withType(const Types::ID type) const
{
	auto s = *this;
	s.typeInfo = TypeInfo(type, isConst(), isReference());
	return s;
}

Symbol Symbol::withComplexType(ComplexType::Ptr typePtr) const
{
	auto s = *this;
	s.typeInfo = TypeInfo(typePtr, isConst());
	return s;
}

Symbol Symbol::relocate(const Symbol& newParent) const
{
	return newParent.getChildSymbol(id, typeInfo);
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

	s << typeInfo.toString();

	if (s.isNotEmpty())
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

Symbol Symbol::createIndexedSymbol(int index, Types::ID type)
{
	Identifier id(juce::String("s") + juce::String(index));

	return Symbol::createRootSymbol(id).withType(type);
}


juce::String FunctionData::getSignature(const Array<Identifier>& parameterIds) const
{
	juce::String s;

	s << returnType.toString() << " " << id << "(";

	int index = 0;

	for (auto arg : args)
	{
		
		s << arg.typeInfo.toString();

		if (arg.isReference())
			s << "&";

		auto pName = parameterIds[index].toString();

		if (pName.isEmpty())
			pName = arg.id.toString();

		if (pName.isNotEmpty())
			s << " " << pName;

		if (++index != args.size())
			s << ", ";
	}

	s << ")";

	return s;
}

bool FunctionData::matchesArgumentTypes(const Array<TypeInfo>& typeList) const
{
	if (args.size() != typeList.size())
		return false;

	for (int i = 0; i < args.size(); i++)
	{
		if (args[i].typeInfo != typeList[i])
			return false;
	}

	return true;
}

bool FunctionData::matchesArgumentTypes(TypeInfo r, const Array<TypeInfo>& argsList) const
{
	if (r != returnType)
		return false;

	return matchesArgumentTypes(argsList);
}

bool FunctionData::matchesArgumentTypes(const FunctionData& otherFunctionData, bool checkReturnType /*= true*/) const
{
	if (checkReturnType && otherFunctionData.returnType != returnType)
		return false;

	if (args.size() != otherFunctionData.args.size())
		return false;

	for (int i = 0; i < args.size(); i++)
	{
		auto thisType = args[i].typeInfo;
		auto otherType = otherFunctionData.args[i].typeInfo;

		if (thisType != otherType)
			return false;
	}

	return true;
}




bool FunctionData::matchesNativeArgumentTypes(Types::ID r, const Array<Types::ID>& nativeArgList) const
{
	Array<TypeInfo> t;

	for (auto n : nativeArgList)
		t.add(TypeInfo(n));

	return matchesArgumentTypes(TypeInfo(r), t);
}

struct InlineData
{
	InlineData(AsmCodeGenerator& gen_) :
		gen(gen_)
	{};

	AsmCodeGenerator& gen;
	AssemblyRegister::Ptr target;
	AssemblyRegister::Ptr object;
	AssemblyRegister::List args;
};

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
					// neu denken
					jassertfalse;

					if (!Types::Helpers::matchesTypeLoose(fArgs[0].typeInfo.getType(), dArgs[0].typeInfo.getType()))
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

snex::jit::FunctionClass* WrapType::getFunctionClass()
{
	auto wrapOperators = new FunctionClass(Symbol::createRootSymbol("WrapOperators"));

	auto wrapSize = size;

	auto assignOperator = new FunctionData();
	assignOperator->returnType = TypeInfo(this);
	assignOperator->addArgs("current", TypeInfo(this));
	assignOperator->addArgs("newValue", TypeInfo(Types::ID::Integer));
	assignOperator->id = wrapOperators->getSpecialSymbol(FunctionClass::AssignOverload);
	assignOperator->inliner = new Inliner(assignOperator->id, [wrapSize](InlineData* d)
	{
		auto target = d->target;
		auto& cc = d->gen.cc;

		bool wasMem = target->hasCustomMemoryLocation() && target->isMemoryLocation();;

		target->loadMemoryIntoRegister(cc);

		auto t = INT_REG_W(target);

		if (isPowerOfTwo(wrapSize))
		{
			cc.and_(t, wrapSize - 1);
		}
		else
		{
			auto d = cc.newGpd();
			auto s = cc.newInt32Const(ConstPool::kScopeLocal, wrapSize);
			cc.cdq(d, t);
			cc.idiv(d, t, s);
			cc.mov(t, d);
		}

		if (wasMem)
		{
			auto mem = target->getMemoryLocationForReference();
			cc.mov(mem, INT_REG_R(target));
			target->setCustomMemoryLocation(mem);
		}

		return Result::ok();
	});

	wrapOperators->addFunction(assignOperator);

	return wrapOperators;
}

bool ComplexType::isValidCastSource(Types::ID nativeType, ComplexType::Ptr p) const
{
	if (p == this)
		return true;

	return false;
}

bool ComplexType::isValidCastTarget(Types::ID nativeType, ComplexType::Ptr p) const
{
	if (p == this)
		return true;

	return false;
}

snex::jit::FunctionClass* VariadicTypeBase::getFunctionClass()
{
	auto fc = new FunctionClass(Symbol::createRootSymbol(type->variadicId));

	ReferenceCountedArray<FunctionClass> childFunctions;

	for (auto c : types)
	{
		childFunctions.add(c->getFunctionClass());
	}

	for (auto& f : type->functions)
	{
		uint8 offset = 0;

		FunctionData* toAdd = new FunctionData(f);

		struct FunctionWithOffset
		{
			FunctionData f;
			size_t offset = 0;
		};

		Array<FunctionWithOffset> functions;

		for (int i = 0; i < types.size(); i++)
		{
			FunctionWithOffset fo;
			fo.f = f;
			fo.offset = offset;

			if (childFunctions[i]->injectFunctionPointer(fo.f))
				functions.add(fo);

			offset += types[i]->getRequiredByteSize();
		}

		toAdd->inliner = new Inliner(f.id, [functions](InlineData* d)
		{
			auto& cc = d->gen.cc;
			auto start = cc.newGpq();

			d->args[0]->loadMemoryIntoRegister(cc);

			cc.mov(start, INT_REG_R(d->args[0]));

			for (auto f : functions)
			{
				d->gen.emitFunctionCall(d->target, f.f, nullptr, d->args);
				cc.add(PTR_REG_W(d->args[0]), (int64)f.offset);
			}

			cc.mov(PTR_REG_W(d->args[0]), start);

			return Result::ok();
		});

		fc->addFunction(toAdd);
	}

	return fc;
}

} // end namespace jit
} // end namespace snex
