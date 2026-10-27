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




LightweightDiagnostics::Ptr LightweightDiagnostics::Factory::createInternal(ProcessorWithScriptingContent* p, const Identifier& id, const String& argId) const
{
	auto nativeType = Helpers::getVarTypeFromPrimitiveType(id.toString());

	if (nativeType != VarTypeChecker::Undefined)
		return new NativeTypeDiagnostic(p, nativeType, argId);

	auto apiTree = ApiHelpers::getApiTree();

	if (id.toString().startsWith("MetaObjects::"))
	{
		auto metaId = Identifier(id.toString().fromFirstOccurrenceOf("MetaObjects::", false, false));

		for (const auto& m : metaItems)
		{
			if (m.first == metaId)
				return m.second(p, apiTree, argId);
		}

		jassertfalse;
		return nullptr;
	}

	auto cTree = apiTree.getChildWithName(id);

	if (!cTree.isValid()) // use the full API tree and let the object resolve the actual API later
		cTree = apiTree;

	for (const auto& i : items)
	{
		if (i.first == id)
			return i.second(p, cTree, argId);
	}

	// meta items could also be constructed directly (coming from the parser)
	for (const auto& i : metaItems)
	{
		if (i.first == id)
			return i.second(p, cTree, argId);
	}

	return new LightweightDiagnostics(p, cTree, argId);
}

LightweightDiagnostics::Ptr LightweightDiagnostics::Factory::createLightweightPrototype(ProcessorWithScriptingContent* p, const Identifier& id, const String& argId) const
{
	if (auto obj = createInternal(p, id, argId))
	{
		auto cb = obj->getCacheBehaviour();

		if (cb == CacheBehaviour::NeverCache)
			return obj;

		if (cb == CacheBehaviour::ReplaceIfPolymorphic)
		{
			if (obj->getObjectName() != id)
			{
				if (auto newObj = createInternal(p, obj->getObjectName(), argId))
					return cache.getCached(newObj);
			}
		}

		return cache.getCached(obj);
	}

	jassertfalse;
	return nullptr;
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

	auto primitiveTypes = Helpers::getPrimitiveTypeList();

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
}

LightweightDiagnostics::FunctionInfo::FunctionInfo(const ValueTree& methodTree)
{
	jassert(methodTree.getType() == Identifier("method"));

	className = methodTree.getParent().getType();
	methodName = methodTree["name"].toString();

	auto args = methodTree["arguments"].toString();

	auto callScopeString = methodTree["callScope"].toString();

	if (callScopeString == "warning")
		callscope = WeakCallbackHolder::CallableObject::CallScope::Warning;
	if (callScopeString == "unsafe")
		callscope = WeakCallbackHolder::CallableObject::CallScope::Unsafe;
	if (callScopeString == "safe")
		callscope = WeakCallbackHolder::CallableObject::CallScope::Safe;
	if (callScopeString == "init")
		callscope = WeakCallbackHolder::CallableObject::CallScope::Init;

	callScopeNote = methodTree["callScopeNote"].toString();

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

	for (auto c : methodTree)
	{
		auto argName = c.getType();

		if (argName == Identifier("this"))
		{
			thisValidators = StringArray::fromTokens(c["check"].toString(), "|", "");
		}

		for (auto& a : arguments)
		{
			if (a.id == argName)
				a.validators = StringArray::fromTokens(c["check"].toString(), "|", "");
		}
	}

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

#define USE_JSON_FOR_VALIDATORS 1

#if USE_JSON_FOR_VALIDATORS

struct FileFetcher
{
	static var get()
	{
		auto now = Time::getMillisecondCounter();

		if ((data.isUndefined() || data.isVoid()) || ((now - lastReadTime) > 1000))
		{
			File jf("D:\\Development\\HISE modules\\tools\\api generator");
			jf = jf.getChildFile("enrichment/resources/constraints_normalized.json");

			jassert(jf.existsAsFile());

			auto ok = JSON::parse(jf.loadFileAsString(), data);

			jassert(ok.wasOk());

			lastReadTime = now;
		}

		return data;
	}

	static var data;
	static uint32 lastReadTime;
};

var FileFetcher::data = var();
uint32 FileFetcher::lastReadTime = 0;


#endif

LightweightDiagnostics::Validator LightweightDiagnostics::FunctionInfo::getValidators() const
{
	Validator v;

#if USE_JSON_FOR_VALIDATORS

	auto result = FileFetcher::get();

	if (auto obj = result[className][methodName].getDynamicObject())
	{
		if (auto thisList = obj->getProperty("this").getArray())
		{
			StringArray list;

			for (auto& v : *thisList)
				list.add(v.toString());

			v.validators.push_back({ -1, list });
		}

		int idx = 0;

		for (const auto& a : arguments)
		{
			for (auto& nv : obj->getProperties())
			{
				if (a.id == nv.name)
				{
					if (auto ar = nv.value.getArray())
					{
						StringArray list;

						for (auto& v : *ar)
							list.add(v.toString());

						v.validators.push_back({ idx, list });
					}

					break;
				}
			}

			idx++;
		}
	}
	
#else

	if (!thisValidators.isEmpty())
		v.validators.push_back({ -1, thisValidators });

	int idx = 0;

	for (const auto& a : arguments)
	{
		if (a.hasValidators())
			v.validators.push_back({ idx, a.validators });

		idx++;
	}
#endif

	return v;
}

LightweightDiagnostics::FunctionInfo::Args LightweightDiagnostics::FunctionInfo::Args::withType(VarTypeChecker::WrappedType t) const
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

DiagnosticBase::DiagnosticResult LightweightDiagnostics::checkCallScope(ScopeSafety minimumScope, const Identifier& methodName) const
{
	if (minimumScope > ScopeSafety::Unknown)
	{
		for (const auto& f : functions)
		{
			if (f.methodName == methodName)
			{
				if (minimumScope > f.callscope)
				{
					auto msg = f.callScopeNote;

					if (msg.isEmpty())
					{
						if (f.callscope == ScopeSafety::Init)
							msg = "Only callable in the onInit callback";
						else
							msg = "Unsafe function call in realtime context";
					}

					auto sev = f.callscope == ScopeSafety::Warning ? DiagnosticResult::Severity::Warning : DiagnosticResult::Severity::Error;

					return DiagnosticResult(getObjectName().toString() + "." + methodName.toString() + "(): " + msg)
						.withSeverity(sev)
						.withErrorCode(DiagnosticResult::ErrorCode::CallScopeMismatch);
				}

				return DiagnosticResult::ok();
			}
		}
	}

	return DiagnosticResult::unknown();
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::runTests(ShadowParserBase* sp, const Identifier& methodName, const Array<var>& args)
{
	currentShadowParser = sp;

	markMethodTouched(methodName);

	auto ok = checkFunction(methodName, args.size());

	ok = DiagnosticResult::max(ok, checkArgCount(methodName, args.size()));

	auto typed = dynamic_cast<HiseJavascriptEngine::RootObject::ObjectTypeMap*>(sp);

	ok = DiagnosticResult::max(ok, checkCallScope(typed->getMinimumRequiredScopeSafety(), methodName));

	if (ok.shouldReport())
		return ok;

	if (hasDiagnosticCheck(methodName))
		return performDiagnostic(methodName, args);

	return DiagnosticResult::ok();
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

			msg << id << "(): ";

			msg << "expects " << numExpected << " argument"
				<< (numExpected != 1 ? "s" : "")
				<< ", got " << numActual;

			if (numActual > numExpected)
				return DiagnosticResult::fail(msg).withErrorCode(DiagnosticResult::ErrorCode::ArgMismatch);


			for (int i = numActual; i < f.arguments.size(); i++)
			{
				if (!f.arguments[i].optional)
					return DiagnosticResult::fail(msg).withErrorCode(DiagnosticResult::ErrorCode::ArgMismatch);
			}

			return DiagnosticResult::ok();
		}
	}

	StringArray candidates;

	for (const auto& f : functions)
		candidates.add(f.methodName.toString());

	return DiagnosticResult::fail(getObjectName() + ": missing function")
		.withWrongToken(id.toString())
		.withFuzzySuggestion(candidates)
		.withErrorCode(DiagnosticResult::ErrorCode::MissingFunctionOrProperty);
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

var LightweightDiagnostics::getReturnType(const Identifier& functionName, const Array<var>& args) const
{
	for (const auto& f : functions)
	{
		if (f.methodName == functionName)
		{
			using RO = HiseJavascriptEngine::RootObject;

			auto argId = args[0].toString();

			auto currentParentValidator = dynamic_cast<RO::ObjectTypeMap*>(currentShadowParser);
			jassert(currentParentValidator != nullptr);

			if (auto ptr = currentParentValidator->createAnonoymousDiagnosticObject(f.returnType, argId))
				return var(ptr.get());
		}
	}

	return var();
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::checkProperty(const Identifier& propertyId) const
{
	for (const auto& f : functions)
	{
		if (f.methodName == propertyId)
			return DiagnosticResult::ok();
	}

	for (const auto& c : apiConstants)
	{
		if (c.name == propertyId)
			return DiagnosticResult::ok();
	}

	StringArray candidates;

	for(const auto& c: apiConstants)
	{
		candidates.add(getObjectName().toString() + "." + c.name.toString());
	}
		

return DiagnosticResult::fail("missing property")
	.withWrongToken(getObjectName() + "." + propertyId.toString())
	.withFuzzySuggestion(candidates)
	.withErrorCode(DiagnosticResult::ErrorCode::MissingFunctionOrProperty);
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::checkFunction(const Identifier& methodName, int expectedNumArgs) const
{
	for (const auto& f : functions)
	{
		if (f.methodName == methodName)
		{
			return DiagnosticResult::ok();
		}
	}

	StringArray candidates;

	for (auto f : functions)
		candidates.add(getObjectName() + "." + f.methodName.toString() + "()");

	return DiagnosticResult::fail("missing function")
		.withWrongToken(getObjectName() + "." + methodName.toString() + "()")
		.withFuzzySuggestion(candidates)
		.withErrorCode(DiagnosticResult::ErrorCode::MissingFunctionOrProperty);
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::shadowParseCallbackWithArgType(const CallbackParseOptions& cbo)
{
	using RO = HiseJavascriptEngine::RootObject;

	auto currentParentValidator = dynamic_cast<RO::ObjectTypeMap*>(currentShadowParser);
	jassert(currentParentValidator != nullptr);

	auto& s = currentParentValidator->getCurrentScope();
	Array<var> args;

	for (auto a : cbo.argTypes)
	{
		auto className = a.first;
		auto argId = a.second;

		if (auto ptr = currentParentValidator->createAnonoymousDiagnosticObject(className, argId))
		{
			args.add(var(ptr.get()));
		}
	}

	if (auto ifo = dynamic_cast<RO::InlineFunction::Object*>(cbo.callback.getObject()))
		return ifo->invokeShadowParse(s, var::NativeFunctionArgs(var(this), args.getRawDataPointer(), args.size()), cbo.strictness).second;

	if (auto fo = dynamic_cast<RO::FunctionObject*>(cbo.callback.getObject()))
		return fo->invokeShadowParse(s, var::NativeFunctionArgs(var(this), args.getRawDataPointer(), args.size()), cbo.strictness).second;

	return DiagnosticResult::ok();
}



Result LightweightDiagnostics::CallbackParseOptions::parseCheckString(const String& f, bool checkParameterCount)
{
	auto ftString = f.upToFirstOccurrenceOf("(", false, false);

	auto idx = StringArray({ "Function", "Realtime", "Inline" }).indexOf(ftString);

	if (idx == -1)
		return Result::fail("Unknown function type " + ftString);

	if (idx > 0)
		strictness = WeakCallbackHolder::CallableObject::StrictnessLevel::Strict;

	ft = (FunctionType)idx;

	auto pString = f.fromFirstOccurrenceOf("(", false, false).removeCharacters("()");
	auto parameters = StringArray::fromTokens(pString, ",", "{}");

	auto parseJSONArgs = [](const String& p)
		{
			std::pair<Identifier, String> rv;

			if (p.startsWith("JSON"))
			{
				rv.first = "JSON";
				rv.second = p.fromFirstOccurrenceOf("{", true, false);

				var result;
				auto ok = JSON::parse(rv.second, result);
				jassert(ok.wasOk());
			}
			else
				rv.first = p.trim();

			return rv;
		};

	for (auto p : parameters)
		argTypes.push_back(parseJSONArgs(p));

	if (auto cb = dynamic_cast<WeakCallbackHolder::CallableObject*>(callback.getObject()))
	{
		if (checkParameterCount)
		{
			auto numArgs = cb->getNumArguments();

			// allow optional parameters with the arg? syntax
			if (numArgs < argTypes.size())
			{
				auto isOptional = true;

				for (int i = numArgs; i < argTypes.size(); i++)
					isOptional &= argTypes[i].first.toString().endsWithChar('?');

				if (isOptional)
					return Result::ok();
			}

			if(numArgs != argTypes.size())
				return Result::fail("Callback signature mismatch. Expected: `f(" + pString + ")`, got: " + String(cb->getNumArguments()) + " arguments");
		}			
	}
	else
	{
		return Result::fail("not a callable object");
	}

	return Result::ok();
}


/** small helper function : returns false if an argument couldn't be resolved. Use this in your diagnostics. */
bool LightweightDiagnostics::Helpers::checkValidArgs(const Array<var>& args)
{
	for (auto& v : args)
	{
		if (v.isUndefined() || v.isVoid())
			return false;
	}

	return true;
}

bool LightweightDiagnostics::Helpers::isApiClass(const Identifier& id)
{
	static const Array<Identifier> classIds({
		"Array",
		"Colours",
		"Console",
		"Content",
		"Date",
		"Engine",
		"FileSystem",
		"Math",
		"Message",
		"ModuleIds",
		"Sampler",
		"Server",
		"Settings",
		"String",
		"Synth",
		"Threads"
		});

	return classIds.contains(id);
}

StringArray LightweightDiagnostics::Helpers::getPrimitiveTypeList()
{
	static const StringArray primitiveTypes = {
		"Integer",
		"Double",
		"undefined",
		"void",
		"var",
		"JSON",
		"ScriptObject",
		"NotUndefined",
		"bool",
		"Object",
		"Number",
		"AudioData",
		"int",
		"double",
		"float",
		"string"
	};

	return primitiveTypes;
}

VarTypeChecker::VarTypes LightweightDiagnostics::Helpers::getVarTypeFromPrimitiveType(const String& s)
{
	static const Array<VarTypeChecker::VarTypes> primitiveTypes = {
		VarTypeChecker::VarTypes::Integer,
		VarTypeChecker::VarTypes::Double,
		VarTypeChecker::VarTypes::Undefined,
		VarTypeChecker::VarTypes::Undefined,
		VarTypeChecker::VarTypes::NotUndefined,
		VarTypeChecker::VarTypes::JSON,
		VarTypeChecker::VarTypes::ScriptObject,
		VarTypeChecker::VarTypes::NotUndefined,
		VarTypeChecker::VarTypes::Integer,
		VarTypeChecker::VarTypes::Object,
		VarTypeChecker::VarTypes::Number,
		VarTypeChecker::VarTypes::AudioData,
		VarTypeChecker::VarTypes::Integer,
		VarTypeChecker::VarTypes::Double,
		VarTypeChecker::VarTypes::Double,
		VarTypeChecker::VarTypes::String
	};

	auto idx = getPrimitiveTypeList().indexOf(s);

	if (isPositiveAndBelow(idx, primitiveTypes.size()))
		return primitiveTypes[idx];

	return VarTypeChecker::VarTypes::Undefined;
}

String LightweightDiagnostics::Helpers::getTypeForObjectTypeCheck(const var& value)
{
	if (auto dbo = dynamic_cast<DebugableObjectBase*>(value.getObject()))
	{
		return dbo->getObjectName().toString();
	}

	if (auto lb = dynamic_cast<LightweightDiagnostics*>(value.getObject()))
	{
		return lb->getObjectName().toString();
	}

	if (auto dyn = value.getDynamicObject())
		return "JSON";

	if (auto ar = value.getArray())
		return "Array";

	if (value.isString())
		return "String";

	auto vt = VarTypeChecker::getType(value);

	return VarTypeChecker::getTypeName(vt).toString();
}

NativeTypeDiagnostic::NativeTypeDiagnostic(ProcessorWithScriptingContent* p, VarTypeChecker::VarTypes type, const String& argId) :
	LightweightDiagnostics(p, ValueTree(), VarTypeChecker::getTypeName(type).toString()),
	t(type)
{
	if (type == VarTypeChecker::String)
		initClassTree(ApiHelpers::getApiTree().getChildWithName("String"));

	if (type == VarTypeChecker::Array)
		initClassTree(ApiHelpers::getApiTree().getChildWithName("Array"));

	if (type == VarTypeChecker::Buffer)
		initClassTree(ApiHelpers::getApiTree().getChildWithName("Buffer"));

	if (type == VarTypeChecker::JSON && argId.isEmpty())
		initClassTree(ApiHelpers::getApiTree().getChildWithName("Object"));

	if (argId.isNotEmpty())
	{
		custom = true;

		if (t == VarTypeChecker::VarTypes::JSON)
		{
			var obj;
			auto r = JSON::parse(argId, obj);
			jassert(r.wasOk());

			ValueTree cTree(VarTypeChecker::getTypeName(type));

			if (auto d = obj.getDynamicObject())
			{
				for (const auto& nv : d->getProperties())
					cTree.setProperty(nv.name, nv.value, nullptr);
			}

			initClassTree(cTree);
		}
	}
}

}