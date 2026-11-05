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

	auto isMeta = id.toString().startsWith("MetaObjects::") || argId.contains("MetaObjects::");

	if (nativeType != VarTypeChecker::Undefined && !isMeta)
		return new NativeTypeDiagnostic(p, nativeType, argId);

	auto apiTree = ApiHelpers::getApiTree();

	if (isMeta)
	{
		auto mid = id.toString().fromFirstOccurrenceOf("MetaObjects::", false, false);

		auto argToUse = argId;

		if (mid.isEmpty())
		{
			mid = argId.fromFirstOccurrenceOf("MetaObjects::", false, false);

			if (mid.containsChar('.'))
			{
				mid = mid.upToFirstOccurrenceOf(".", false, false);
				argToUse = argId.fromLastOccurrenceOf(".", false, false);
			}
		}
			

		auto metaId = Identifier(mid);

		for (const auto& m : metaItems)
		{
			if (m.first == metaId)
				return m.second(p, apiTree, argToUse);
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


String LightweightDiagnostics::FunctionInfo::getSpecialReturnType(const Identifier& className, const Identifier& methodName)
{
	if (className == Identifier("Engine"))
	{
		if(methodName == Identifier("getProjectInfo"))
			return "MetaObjects::ProjectInfo";

		if (methodName == Identifier("getExtraDefinitionsInBackend"))
			return "MetaObjects::ExtraDefinitions";

		if (methodName == Identifier("getSystemStats"))
			return "MetaObjects::SystemStats";
	}

	if (methodName == Identifier("loadFromXmlFile"))
		return "DataContent";

	return {};
}

LightweightDiagnostics::FunctionInfo::FunctionInfo(const ValueTree& methodTree)
{
	jassert(methodTree.getType() == Identifier("method"));

	isStatic = methodTree.getProperty("isStatic", false);

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

	auto sn = getSpecialReturnType(className, methodName);

	if (sn.isNotEmpty())
		rt = sn;
    
	auto typed = methodTree["returnObjectType"].toString();

	for (auto c : methodTree)
	{
		auto argName = c.getType();

		if (argName == Identifier("this"))
			thisValidators = StringArray::fromTokens(c["check"].toString(), "|", "");

		for (auto& a : arguments)
		{
			if (a.id == argName)
				a.validators = StringArray::fromTokens(c["check"].toString(), "|", "");
		}
	}

	if (typed.isNotEmpty())
		rt = typed;

	if (rt.startsWith("JSON{"))
		returnType = { Identifier("JSON"), rt.fromFirstOccurrenceOf("JSON", false, false) };
	else if (rt.startsWith("Array."))
		returnType = { Identifier("Array"), rt.fromFirstOccurrenceOf(".", false, false) };
	else
		returnType = { Identifier(rt), "" };
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
#if JUCE_WINDOWS
			File jf("D:\\Development\\HISE modules\\tools\\api generator");
            jf = jf.getChildFile("enrichment/resources/constraints_normalized.json");
#else
            File jf("/Users/christophhart/HISE/tools/api generator/enrichment/resources/constraints_normalized.json");
#endif

			jassert(jf.existsAsFile());

			var newData;

			auto ok = JSON::parse(jf.loadFileAsString(), newData);

			jassert(ok.wasOk());

			if (ok.wasOk())
				data = newData;

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

					auto sev = minimumScope == ScopeSafety::Warning ? DiagnosticResult::Severity::Warning : DiagnosticResult::Severity::Error;

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

	auto typed = dynamic_cast<HiseJavascriptEngine::RootObject::ObjectTypeMap*>(sp);

	ok = DiagnosticResult::max(ok, checkCallScope(typed->getMinimumRequiredScopeSafety(), methodName));

	if (ok.shouldReport())
		return ok;

	if (hasDiagnosticCheck(methodName))
		return performDiagnostic(methodName, args);

	return DiagnosticResult::ok();
}

struct LightweightFunctionInfoToken : public TokenWithDot
{
    static ValueTree getMethodTree(const LightweightDiagnostics::FunctionInfo& fi)
    {
        auto ct = ApiHelpers::getApiTree().getChildWithName(fi.className);
        jassert(ct.isValid());
        auto mt = ct.getChildWithProperty("name", fi.methodName.toString());
        jassert(mt.isValid());
        return mt;
    }

    LightweightFunctionInfoToken(const ValueTree& mt, const String prevToken) :
        TokenWithDot(getContent(mt, prevToken), prevToken)
    {
        priority = 1000;
        c = Colour(0xFF88EECC);
		label = mt["name"].toString();
		kind = CompletionItemKind::Method;

		auto rt = mt["returnObjectType"].toString();

		if (rt.isEmpty())
			rt = mt["returnType"].toString();

		markdownDescription << "> `" << VarTypeChecker::prettify(rt) << " ";
		markdownDescription << mt.getParent().getType() << "." << mt["name"].toString();
		markdownDescription << VarTypeChecker::prettify(mt["arguments"].toString()) << "`";
		markdownDescription << "  \n" << mt["description"].toString();
    };

    Array<Range<int>> getSelectionRangeAfterInsert(const String& input) const override
    {
        auto code = getCodeToInsert(input);

        if (callbackParameterIndex != -1)
        {
            auto brIndex = code.indexOf("\t \n");

            Array<Range<int>> ranges;

            ranges.add({ brIndex + 1, brIndex + 2 });
            return ranges;
        }

        return TokenWithDot::getSelectionRangeAfterInsert(input);
    }
    
    void setFunctionBody(int parameterIndex, const String& newFunctionBody)
    {
        callbackParameterIndex = parameterIndex;
        String classId, methodName;
        
        classId = tokenContent.upToFirstOccurrenceOf(".", false, false);
        tokenContent = tokenContent.fromFirstOccurrenceOf(".", false, false);
        
        methodName = tokenContent.upToFirstOccurrenceOf("(", false, false);
        tokenContent = tokenContent.fromFirstOccurrenceOf("(", false, false);
        
        auto args = tokenContent.upToLastOccurrenceOf(")", false, false).removeCharacters("()");
        
        auto argList = StringArray::fromTokens(args, ",", "");
        
        argList.removeEmptyStrings();
        argList.set(parameterIndex, newFunctionBody);
        
        String newTokenContent;
        
        newTokenContent << classId << "." << methodName << "(";
        
        for(const auto& a: argList)
            newTokenContent << a << ", ";
        
        newTokenContent = newTokenContent.upToLastOccurrenceOf(", ", false, false);
        
        newTokenContent << ");";
        
        tokenContent = newTokenContent;
    }

    int callbackParameterIndex = -1;
    String functionBody;
};

struct LightweightConstantInfoToken : public TokenWithDot
{
    LightweightConstantInfoToken(const String& objectName, const Identifier& id) :
        TokenWithDot(objectName + "." + id.toString(), objectName)
    {
		kind = CompletionItemKind::Property;
		label = id.toString();
        priority = 100;
        c = Colour(0xFF88EECC);
    }
};



void LightweightDiagnostics::fillAutocompleteTokens(mcl::TokenCollection::List& list, const String& prevToken)
{
	if (!occurrences.empty())
		list.add(new NavigationToken(this));

    for (const auto& fi : functions)
    {
        auto mt = LightweightFunctionInfoToken::getMethodTree(fi);

        auto newToken = new LightweightFunctionInfoToken(mt, prevToken);

        auto cp = fi.getCallbackParameter();

        if (cp != -1)
        {
            // resume the work here: replace the type with a suitable parameter name, replace the trailing old parameter
            // TODO: Add a system that extrapolates / synthesises good parameter names from the docs.

            auto found = false;

            for (auto v: fi.arguments[cp].validators)
            {
                if (v.startsWith("CallbackArgCheck"))
                {
                    CallbackParseOptions cpo;

                    auto ok = cpo.parseCheckString(v.fromFirstOccurrenceOf("CallbackArgCheck.", false, false), false);

                    if (cpo.ft == CallbackParseOptions::FunctionType::Function)
                    {
                        String functionBody;
                        functionBody << "function(";

                        for (auto a : cpo.argTypes)
                        {
							jassert(a.id.isValid());
							functionBody << a.id;

							if (a.typeInfo.isRealType())
							{
								functionBody << ":";
								functionBody << VarTypeChecker::getTypeName(a.typeInfo);
							}

							functionBody << ", ";
                        }

                        functionBody = functionBody.upToLastOccurrenceOf(", ", false, false);
                        functionBody << ")\n{\n\t \n}";

                        newToken->setFunctionBody(cp, functionBody);
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                newToken->setFunctionBody(cp, "function(){\n\t \n}");
            }
        }

        list.add(newToken);
    }

    for (const auto& c : apiConstants)
    {
        if(auto m = HiseJavascriptEngine::createAutocompleteTokenFromInlineFunction(c.value))
        {
            list.add(m);
        }
        else
        {
            list.add(new LightweightConstantInfoToken(prevToken, c.name));
        }
    }
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

	StringArray candidates;

	for (const auto& f : functions)
		candidates.add(f.methodName.toString());

	return DiagnosticResult::fail(getObjectName() + ": missing function").withWrongToken(id.toString()).withFuzzySuggestion(candidates);
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

			String argId;

			auto a = args[0];

			if (a.isString() || a.isInt() || a.isBool() || a.isInt64() || a.isDouble())
				argId = a.toString();
			else if (auto obj = dynamic_cast<ObjectWithJSONConverter*>(a.getObject()))
				argId = JSON::toString(a);
			else if (auto ar = a.getArray())
				argId = JSON::toString(a);

			auto currentParentValidator = dynamic_cast<RO::ObjectTypeMap*>(currentShadowParser);
			jassert(currentParentValidator != nullptr);

			if (f.returnType.second.isNotEmpty())
				argId = f.returnType.second;

			if (auto ptr = currentParentValidator->createAnonoymousDiagnosticObject(f.returnType.first, argId))
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
		

	return DiagnosticResult::fail("missing property").withWrongToken(getObjectName() + "." + propertyId.toString()).withFuzzySuggestion(candidates);
}

DiagnosticBase::DiagnosticResult LightweightDiagnostics::checkFunction(const Identifier& methodName, int expectedNumArgs) const
{
	for (const auto& f : functions)
	{
		if (f.methodName == methodName)
		{
			if (isStaticObject != f.isStatic)
			{
				if (isStaticObject)
					return DiagnosticResult::fail("Trying to call object method on class");
				else
					return DiagnosticResult::fail("Trying to call static method on object")
					.withSuggestion(getObjectName() + "." + methodName + "()");
			}

			return DiagnosticResult::ok();
		}
	}

	StringArray candidates;

	for (auto f : functions)
		candidates.add(getObjectName() + "." + f.methodName.toString() + "()");

	return DiagnosticResult::fail("missing function").withWrongToken(getObjectName() + "." + methodName.toString() + "()").withFuzzySuggestion(candidates);
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
		auto className = VarTypeChecker::getTypeName(a.typeInfo);
		auto argId = a.customData;

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

	using SL = WeakCallbackHolder::CallableObject::StrictnessLevel;

	std::map<String, std::pair<SL, FunctionType>> strictnessMap;

	strictnessMap["Function"] =   { SL::Unsafe, FunctionType::Function };
	strictnessMap["Inline"] =     { SL::Strict, FunctionType::InlineFunction };
	strictnessMap["InlineWarn"] = { SL::Warn,   FunctionType::InlineFunction };
	strictnessMap["Realtime"] =   { SL::Strict, FunctionType::Realtime };

	if (strictnessMap.find(ftString) != strictnessMap.end())
	{
		auto v = strictnessMap.at(ftString);;
		strictness = v.first;
		ft = v.second;
	}
	else
		return Result::fail("Unknown function type " + ftString);

	auto pString = f.fromFirstOccurrenceOf("(", false, false).removeCharacters("()");
	auto parameters = StringArray::fromTokens(pString, ",", "{}");

	auto parseJSONArgs = [](const String& p)
	{
		ArgumentData rv;

		if (p.contains("MetaObjects::"))
		{
			rv.typeInfo = VarTypeChecker::WrappedType(VarTypeChecker::TypedObject::MetaObject);
			rv.id = Identifier(p.fromFirstOccurrenceOf("MetaObjects::", false, false).trim());
			rv.customData = p;
			return rv;
		}
		else if (p.contains("JSON{"))
		{
			auto argId = p.upToFirstOccurrenceOf(":JSON{", false, false);

			if (!argId.isEmpty())
				rv.id = Identifier(argId);
			else
				rv.id = {};

			rv.typeInfo = VarTypeChecker::WrappedType(VarTypeChecker::JSON);
			rv.customData = p.fromFirstOccurrenceOf("{", true, false);

			var result;
			auto ok = JSON::parse(rv.customData, result);
			jassert(ok.wasOk());
		}
		else
		{
			if (p.containsChar(':'))
			{
				rv.id = p.upToFirstOccurrenceOf(":", false, false).trim();

				auto typeName = p.fromFirstOccurrenceOf(":", false, false).trim();

				rv.optional = typeName.containsChar('?');

				typeName = typeName.removeCharacters("?");

				if (Identifier::isValidIdentifier(typeName))
					rv.typeInfo = VarTypeChecker::getTypeFromString(Identifier(typeName));
			}
			else
			{
				rv.typeInfo = VarTypeChecker::getTypeFromString(Identifier(p.trim()));
			}
		}
		
		// must be resolvable
		jassert(rv.typeInfo);

		return rv;
	};

	for (auto p : parameters)
		argTypes.push_back(parseJSONArgs(p));

	if (auto cb = dynamic_cast<WeakCallbackHolder::CallableObject*>(callback.getObject()))
	{
		if (checkParameterCount && cb->getNumArguments() != argTypes.size())
			return Result::fail("Callback signature mismatch. Expected: `f(" + pString + ")`, got: " + String(cb->getNumArguments()) + " arguments");
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

Array<Identifier> LightweightDiagnostics::Helpers::getApiClassIds()
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

    return classIds;
}

bool LightweightDiagnostics::Helpers::isApiClass(const Identifier& id)
{
    return getApiClassIds().contains(id);
}

var LightweightDiagnostics::Helpers::getTypeForLoopIterator(ProcessorWithScriptingContent* p, const var& iteratedObject)
{
	if (iteratedObject.isArray())
	{
		if (iteratedObject.size() == 0)
			return var();

		return iteratedObject[0];
	}

	if (auto bf = iteratedObject.getBuffer())
	{
		
	}

	if (auto lb = dynamic_cast<LightweightDiagnostics*>(iteratedObject.getObject()))
	{
		if (auto ar = dynamic_cast<ArrayWithKnownElementType::Diagnostics*>(lb))
			return ar->getAssignedValue(0);

		if (lb->getObjectName() == Identifier("Buffer"))
			return var(new NativeTypeDiagnostic(p, VarTypeChecker::Double, ""));
	}

	if (auto obj = iteratedObject.getDynamicObject())
	{
		const auto& prop = obj->getProperties();

		if (!prop.isEmpty())
			return var(prop.getName(0).toString());
	}

	return var();
}

VarTypeChecker::VarTypes LightweightDiagnostics::Helpers::getVarTypeFromPrimitiveType(const String& s)
{
	auto t = VarTypeChecker::getTypeFromString(s);

	if (t.isPrimitive())
		return t.asPrimitiveType();

	return VarTypeChecker::Undefined;
}

String LightweightDiagnostics::Helpers::getTypeForObjectTypeCheck(const var& value)
{
	if (auto dbo = dynamic_cast<DebugableObjectBase*>(value.getObject()))
	{
		return dbo->getObjectName().toString();
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
	auto init = [&](const Identifier& s)
	{
		initClassTree(ApiHelpers::getApiTree().getChildWithName(s));
		apiConstants.set("length", 1);
	};

	if (type == VarTypeChecker::String)
		init("String");


	else if (type == VarTypeChecker::Array)
		init("Array");


	else if (type == VarTypeChecker::Buffer)
		init("Buffer");

    else if (type == VarTypeChecker::DataContent)
    {
		ValueTree t("DataContent");

		t.setProperty("length", 1, nullptr);

		for (auto m : ApiHelpers::getApiTree().getChildWithName("Array"))
			t.addChild(m.createCopy(), -1, nullptr);

		for (auto m : ApiHelpers::getApiTree().getChildWithName("String"))
			t.addChild(m.createCopy(), -1, nullptr);

		initClassTree(t);
    }
	else
	{
		ValueTree t(VarTypeChecker::getTypeName(type));
		initClassTree(t);
	}
    
    if (argId.isNotEmpty())
    {
        if (t | VarTypeChecker::VarTypes::JSON && argId.startsWithChar('{'))
        {
			custom = true;

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

LightweightDiagnostics::FunctionInfo InbuiltFunction::createFunctionInfo(VarTypeChecker::VarTypes returnType, VarTypeChecker::VarTypes argType, CS callScope) const
{
	FunctionInfo self(getObjectName());

	self.callscope = callScope;
	self.methodName = getObjectName();

	if (self.methodName == Identifier("Rectangle"))
	{
		self.returnType = { "Rectangle", "" };

		FunctionInfo::Args a1("a1");
		a1.type = VarTypeChecker::WrappedType(VarTypeChecker::IndexOrArray);

		self.arguments.push_back(a1);

		FunctionInfo::Args a2("a2");
		a2.type = VarTypeChecker::WrappedType(VarTypeChecker::IndexOrArray);

		self.arguments.push_back(a2.asOptional(true));

		FunctionInfo::Args a3("a3");
		a3.type = VarTypeChecker::WrappedType(VarTypeChecker::IndexOrArray);

		self.arguments.push_back(a3.asOptional(true));

		FunctionInfo::Args a4("a4");
		a4.type = VarTypeChecker::WrappedType(VarTypeChecker::IndexOrArray);

		self.arguments.push_back(a4.asOptional(true));
	}
	else
	{
		self.returnType = { VarTypeChecker::getTypeName(returnType), "" };
		FunctionInfo::Args singleArg("argument");
		singleArg.type = VarTypeChecker::WrappedType(argType);

		self.arguments.push_back(singleArg);
	}

	return self;
}

ArrayWithKnownElementType::Diagnostics::Diagnostics(ProcessorWithScriptingContent* p, const ValueTree& v, const String& argId) :
	LightweightDiagnostics(p, v, argId)
{
	LightweightDiagnostics::Factory f;

	if (argId.isNotEmpty())
		returnType = var(f.createLightweightPrototype(p, Identifier(argId), "").get());
}

}
