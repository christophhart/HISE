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


namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;

bool FunctionClass::hasFunction(const NamespacedIdentifier& s) const
{
	if (getClassName() == s)
		return true;

	auto parent = s.getParent();

	if (parent == classSymbol || !classSymbol.isValid())
	{
		for (auto f : functions)
			if (f->id == s)
				return true;
	}

	for (auto c : registeredClasses)
	{
		if (c->hasFunction(s))
			return true;
	}

	return false;
}



bool FunctionClass::hasConstant(const NamespacedIdentifier& s) const
{
	auto parent = s.getParent();

	if (parent == classSymbol)
	{
		for (auto& c : constants)
			if (c.id == s.getIdentifier())
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

void FunctionClass::addMatchingFunctions(Array<FunctionData>& matches, const NamespacedIdentifier& symbol) const
{
	auto parent = symbol.getParent();

	if (parent == classSymbol || !classSymbol.isValid())
	{
		for (auto f : functions)
		{
			if (f->id == symbol)
				matches.add(*f);
		}

		if (classSymbol.isValid())
			return;
	}

	for (auto c : registeredClasses)
		c->addMatchingFunctions(matches, symbol);
}


void FunctionClass::addFunctionClass(FunctionClass* newRegisteredClass)
{
	registeredClasses.add(newRegisteredClass);
}


void FunctionClass::removeFunctionClass(const NamespacedIdentifier& id)
{
	for (int i = 0; i < registeredClasses.size(); i++)
	{
		auto c = registeredClasses[i];

		if (c->getClassName() == id)
		{
			registeredClasses.remove(i--);
		}
	}
}

void FunctionClass::addFunction(FunctionData* newData)
{
	if (newData->id.isExplicit())
	{
		newData->id = getClassName().getChildId(newData->id.getIdentifier());
	}

	functions.add(newData);
}


Array<NamespacedIdentifier> FunctionClass::getFunctionIds() const
{
	Array<NamespacedIdentifier> ids;

	for (auto r : functions)
		ids.add(r->id);

	return ids;
}

bool FunctionClass::fillJitFunctionPointer(FunctionData& dataWithoutPointer)
{
	// first check strict typing
	for (auto f : functions)
	{
		if (f->matchIdArgsAndTemplate(dataWithoutPointer))
		{
			dataWithoutPointer.function = f->function;
			return dataWithoutPointer.function != nullptr;
		}
	}

	for (auto f : functions)
	{
		bool idMatch = f->id == dataWithoutPointer.id;
		auto templateMatch = f->matchesTemplateArguments(dataWithoutPointer.templateParameters);

		if (idMatch && templateMatch)
		{
			auto& fArgs = f->args;
			auto& dArgs = dataWithoutPointer.args;

			if (fArgs.size() == dArgs.size())
			{


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
		if (f->matchIdArgsAndTemplate(dataToInject))
		{
			f->function = dataToInject.function;
			return true;
		}
	}

	return false;
}

snex::jit::FunctionClass* FunctionClass::getSubFunctionClass(const NamespacedIdentifier& id)
{
	for (auto f : registeredClasses)
	{
		if (f->getClassName() == id)
			return f;
	}

	return nullptr;
}

bool FunctionClass::isInlineable(const NamespacedIdentifier& id) const
{
	for (auto& f : functions)
		if (f->id == id)
			return f->inliner != nullptr;

	return false;
}

snex::jit::Inliner::Ptr FunctionClass::getInliner(const NamespacedIdentifier& id) const
{
	for (auto f : functions)
	{
		if (f->id == id)
			return f->inliner;
	}

	return nullptr;
}

void FunctionClass::addInliner(const Identifier& id, const Inliner::Func& func, Inliner::InlineType type)
{
	auto nId = getClassName().getChildId(id);

	if (isInlineable(nId))
		return;

	for (auto& f : functions)
	{
		if (f->id == nId)
		{
			if(type == Inliner::Assembly)
				f->inliner = new Inliner(nId, func, {});
			else
				f->inliner = new Inliner(nId, {}, func);
		}
			
	}
}

juce::Identifier FunctionClass::getSpecialSymbol(const NamespacedIdentifier& classId, SpecialSymbols s)
{
	switch (s)
	{
	case AssignOverload: return "operator=";
	case NativeTypeCast: return "type_cast";
	case IncOverload:    return "++operator";
	case DecOverload:    return "--operator";
	case PostIncOverload: return "operator++";
	case PostDecOverload: return "operator--";
	case Subscript:		 return "operator[]";
	case ToSimdOp:		 return "toSimd";
	case BeginIterator:  return "begin";
	case SizeFunction:	 return "size";
	case GetFrom:		 return "getFrom";
	case Destructor:     return Identifier("~" + classId.getIdentifier().toString());
	case Constructor:    return classId.getIdentifier();
	}

	return {};
}

bool FunctionClass::hasSpecialFunction(SpecialSymbols s) const
{
	auto id = getClassName().getChildId(getSpecialSymbol(getClassName(), s));
	return hasFunction(id);
}

void FunctionClass::addSpecialFunctions(SpecialSymbols s, Array<FunctionData>& possibleMatches) const
{
	auto id = getClassName().getChildId(getSpecialSymbol(getClassName(), s));
	addMatchingFunctions(possibleMatches, id);
}

FunctionData FunctionClass::getConstructor(const Array<TypeInfo>& args)
{
	for (auto f : functions)
	{
		if (f->isConstructor() && getClassName() == f->id.getParent() && f->matchesArgumentTypesWithDefault(args))
			return FunctionData(*f);
	}

	return {};
}

snex::jit::FunctionData FunctionClass::getConstructor(InitialiserList::Ptr initList)
{
	Array<TypeInfo> args;

	if (initList == nullptr)
		return getConstructor(args);

	initList->forEach([&args](InitialiserList::ChildBase* b)
		{
			if (auto e = dynamic_cast<InitialiserList::ExpressionChild*>(b))
			{
				args.add(e->expression->getTypeInfo());
			}
			else
			{
				VariableStorage v;
				if (b->getValue(v))
					args.add(TypeInfo(v.getType()));
			}

			return false;
		});


	return getConstructor(args);
}

bool FunctionClass::isConstructor(const NamespacedIdentifier& id)
{
	return id.isValid() && id.getParent().getIdentifier() == id.getIdentifier();
}

FunctionData FunctionClass::getSpecialFunction(SpecialSymbols s, TypeInfo returnType, const TypeInfo::List& argTypes) const
{
	if (hasSpecialFunction(s))
	{
		Array<FunctionData> matches;

		addSpecialFunctions(s, matches);

		if (returnType.isInvalid() && argTypes.isEmpty())
		{
			if (matches.size() == 1)
				return matches.getFirst();
		}

		for (auto& m : matches)
		{
			if (m.matchesArgumentTypes(returnType, argTypes))
				return m;
		}
	}

	return {};
}

snex::jit::FunctionData FunctionClass::getNonOverloadedFunctionRaw(NamespacedIdentifier id) const
{
	for (auto f : functions)
	{
		if (f->id == id)
			return *f;
	}

	return {};
}

snex::jit::FunctionData FunctionClass::getNonOverloadedFunction(NamespacedIdentifier id) const
{
	if (id.getParent() != getClassName())
	{
		id = getClassName().getChildId(id.getIdentifier());
	}

	Array<FunctionData> matches;

	addMatchingFunctions(matches, id);

	for (int i = 0; i < matches.size(); i++)
	{
		if (matches[i].hasTemplatedArgumentOrReturnType() ||
			matches[i].hasUnresolvedTemplateParameters())
			matches.remove(i--);
	}

	if (matches.size() == 1)
		return matches.getFirst();

	return {};
}

FunctionClass::FunctionClass(const NamespacedIdentifier& id) :
	classSymbol(id)
{

}

FunctionClass::~FunctionClass()
{
	registeredClasses.clear();
	functions.clear();
}

juce::ValueTree FunctionClass::createApiTree(FunctionClass* r)
{
	ValueTree p(r->getObjectName());

	for (auto f : r->functions)
	{
		ValueTree m("method");
		m.setProperty("name", f->id.toString(), nullptr);
		m.setProperty("description", f->getSignature(), nullptr);

		juce::String arguments = "(";

		int index = 0;

		for (auto arg : f->args)
		{
			index++;

			if (arg.typeInfo.getType() == Types::ID::Block && r->getObjectName().toString() == "Block")
				continue;

			arguments << Types::Helpers::getTypeName(arg.typeInfo.getType());

			if (arg.id.isValid())
				arguments << " " << arg.id.toString();

			if (index < (f->args.size()))
				arguments << ", ";
		}

		arguments << ")";

		m.setProperty("arguments", arguments, nullptr);
		m.setProperty("returnType", f->returnType.toString(), nullptr);
		m.setProperty("description", f->description, nullptr);
		m.setProperty("typenumber", (int)f->returnType.getType(), nullptr);

		p.addChild(m, -1, nullptr);
	}

	return p;
}

juce::ValueTree FunctionClass::getApiValueTree() const
{
	ValueTree v("Api");

	for (auto r : registeredClasses)
	{
		v.addChild(createApiTree(r), -1, nullptr);
	}

	return v;
}

snex::jit::FunctionData& FunctionClass::createSpecialFunction(SpecialSymbols s)
{
	auto f = new FunctionData();
	f->id = getClassName().getChildId(getSpecialSymbol(getClassName(), s));

	addFunction(f);
	return *f;
}

void FunctionClass::getAllFunctionNames(Array<NamespacedIdentifier>& functions) const
{
	functions.addArray(getFunctionIds());
}

void FunctionClass::setDescription(const juce::String& s, const StringArray& names)
{
	if (auto last = functions.getLast())
		last->setDescription(s, names);
}

void FunctionClass::getAllConstants(Array<Identifier>& ids) const
{
	for (auto c : constants)
		ids.add(c.id);
}

hise::DebugInformationBase* FunctionClass::createDebugInformationForChild(const Identifier& id)
{
	for (auto& c : constants)
	{
		if (c.id == id)
		{
			auto s = new SettableDebugInfo();
			s->codeToInsert << getInstanceName().toString() << "." << id.toString();
			s->dataType = "Constant";
			s->type = Types::Helpers::getTypeName(c.value.getType());
			s->typeValue = ApiHelpers::DebugObjectTypes::Constants;
			s->value = Types::Helpers::getCppValueString(c.value);
			s->name = s->codeToInsert;
			s->category = "Constant";

			return s;
		}
	}

	return nullptr;
}

snex::VariableStorage FunctionClass::getConstantValue(const NamespacedIdentifier& s) const
{
	auto parent = s.getParent();

	if (parent == classSymbol)
	{
		for (auto& c : constants)
		{
			if (c.id == s.getIdentifier())
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


const juce::var FunctionClass::getConstantValue(int index) const
{
	return var(constants[index].value.toDouble());
}

}
}