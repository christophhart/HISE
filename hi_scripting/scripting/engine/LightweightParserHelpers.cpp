#include "LightweightParserHelpers.h"
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

namespace hise {
using namespace juce;





DiagnosticBase* LightweightDiagnostics::Factory::createLightweightPrototype(ProcessorWithScriptingContent* p, const Identifier& id, const String& argId) const
{
	auto cTree = ApiHelpers::getApiTree().getChildWithName(id);
	jassert(cTree.isValid());

	for (const auto& i : items)
	{
		if (i.first == id)
			return i.second(p, cTree, argId);
	}

	return new LightweightDiagnostics(p, cTree, argId);
}

Identifier LightweightDiagnostics::Factory::getReturnTypeFromApiMap(const Identifier& className, const Identifier& methodName) const
{
	for (const auto& i : apiMap->items)
	{
		if (i.className == className && i.methodName == methodName)
			return i.returnType;
	}

	return {};
}

LightweightDiagnostics::ApiMap::ApiMap()
{
	auto tree = ApiHelpers::getApiTree();

	static const StringArray primitiveTypes({
		"Integer", "Double", "undefined", "void", "var", "JSON", "ScriptObject", "NotUndefined", "bool", "Object", "Number", "AudioData", "int", "double" }
		);

	StringArray foundTypes;

	for (auto c : tree)
	{
		for (auto m : c)
		{
			auto rt = m["returnType"].toString();

			auto typed = m["returnObjectType"].toString();

			if (typed.isNotEmpty())
				rt = typed;

			if (!primitiveTypes.contains(rt))
			{
				Item ni;
				ni.className = c.getType();
				ni.methodName = m["name"].toString();
				ni.returnType = rt;

				items.push_back(ni);

			}
		}
	}

	for (const auto& c : items)
		DBG(c.toString());

}

LightweightDiagnostics::FunctionInfo::FunctionInfo(const ValueTree& methodTree)
{
	jassert(methodTree.getType() == Identifier("method"));

	methodName = methodTree["name"].toString();

	auto args = methodTree["arguments"].toString();

	args = args.substring(1, args.length() - 1);

	if (args.isNotEmpty())
	{
		auto tokens = StringArray::fromTokens(args, ",", "");

		int numRequired = (int)methodTree["numRequiredArgs"];
		int idx = 0;

		for (auto& t : tokens)
			arguments.push_back(Args::parse(t.trim()).asOptional(!(idx++ < numRequired)));
	}

	auto rt = methodTree["returnType"].toString();

	auto typed = methodTree["returnObjectType"].toString();

	if (typed.isNotEmpty())
		rt = typed;

	static const StringArray primitiveTypes({
		"Integer", "Double", "undefined", "void", "var", "JSON", "ScriptObject", "NotUndefined", "bool", "Object", "Number", "AudioData", "int", "double" }
		);

	if (!primitiveTypes.contains(rt))
	{
		returnType = Identifier(rt);
	}
}

LightweightDiagnostics::FunctionInfo LightweightDiagnostics::FunctionInfo::withNumArgs(int numArgs) const
{
	auto copy = *this;
	copy.arguments.clear();

	for (int i = 0; i < numArgs; i++)
		copy.arguments.push_back(Args("unnamed"));

	return copy;
}

LightweightDiagnostics::FunctionInfo LightweightDiagnostics::FunctionInfo::withArgs(Args a) const
{
	auto copy = *this;
	copy.arguments.push_back(a);
	return copy;
}

LightweightDiagnostics::FunctionInfo::Args LightweightDiagnostics::FunctionInfo::Args::withType(VarTypeChecker::VarTypes t) const
{
	auto copy = *this;
	copy.type = t;
	return copy;
}

LightweightDiagnostics::FunctionInfo::Args LightweightDiagnostics::FunctionInfo::Args::asOptional(bool isOptional) const
{
	auto copy = *this;
	copy.optional = isOptional;
	return copy;
}

LightweightDiagnostics::FunctionInfo::Args LightweightDiagnostics::FunctionInfo::Args::parse(const String& argWithType)
{
	auto type = argWithType.upToFirstOccurrenceOf(" ", false, false);
	auto name = argWithType.fromFirstOccurrenceOf(" ", false, false);

	return Args(name).withType(VarTypeChecker::getTypeFromString(type));
}

juce::String LightweightDiagnostics::ApiMap::Item::toString() const
{
	juce::String x;
	x << className.toString() << ".";
	x << methodName.toString() << "() -> ";
	x << returnType.toString();
	return x;
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::checkArgCount(const Identifier& id, int numActual) const
{
	for (const auto& f : functions)
	{
		if (f.methodName == id)
		{
			if (numActual == f.arguments.size())
				return DiagnosticResult::ok();

			auto numExpected = f.arguments.size();

			String msg;
			msg << "expects " << numExpected << " argument"
				<< (numExpected != 1 ? "s" : "")
				<< ", got " << numActual;

			if (numActual > numExpected)
				return DiagnosticResult::fail(msg);


			for (int i = numActual; i < f.arguments.size(); i++)
			{
				if (!f.arguments[i].optional)
					return DiagnosticResult::fail(msg);
			}

			return DiagnosticResult::ok();
		}
	}

	jassertfalse;
	return DiagnosticResult::fail("Can't find method");
}

bool LightweightDiagnostics::getIndexAndNumArgsForFunction(const Identifier& id, int& index, int& numArgs) const
{
	int idx = 0;

	for (const auto& f : functions)
	{
		if (f.methodName == id)
		{
			numArgs = f.arguments.size();
			index = idx;
			return true;
		}

		idx++;
	}

	numArgs = -1;
	index = -1;
	return false;
}

bool LightweightDiagnostics::checkProperty(const Identifier& propertyId) const
{
	for (const auto& f : functions)
	{
		if (f.methodName == propertyId)
			return true;
	}

	return apiConstants.contains(propertyId);
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::shadowParseCallbackWithArgType(const var& callback, const std::vector<Identifier>& argTypes)
{
	using RO = HiseJavascriptEngine::RootObject;

	if (auto ifo = dynamic_cast<RO::InlineFunction::Object*>(callback.getObject()))
	{
		jassert(currentParentValidator != nullptr);

		RO::ObjectTypeMap::TypeScope t(*currentParentValidator->getOrCreateObjectTypeMap());

		for (int i = 0; i < ifo->getNumArguments(); i++)
		{
			auto exp = ifo->parameterNames[i].toString();
			auto typ = argTypes[i];

			t.add(exp, typ);
		}

		currentParentValidator->process(ifo->body);
	}

	if (auto fo = dynamic_cast<RO::FunctionObject*>(callback.getObject()))
	{
		jassert(currentParentValidator != nullptr);

		RO::ObjectTypeMap::TypeScope t(*currentParentValidator->getOrCreateObjectTypeMap());

		for (int i = 0; i < fo->getNumArguments(); i++)
		{
			auto exp = fo->parameters[i].toString();
			auto typ = argTypes[i];

			t.add(exp, typ);
		}

		currentParentValidator->process(fo->body);
	}

	return DiagnosticResult::ok();
}

}