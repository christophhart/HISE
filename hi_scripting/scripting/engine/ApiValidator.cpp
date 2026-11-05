#include "ApiValidator.h"
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

struct HiseJavascriptEngine::RootObject::ObjectTypeMap::ScopeStack
{
	ScopeStack(ObjectTypeMap& parent_) :
		parent(parent_)
	{}

	struct ParserScope
	{
		ParserScope(ScopeStack& p, DynamicObject* obj, Strictness scopeStrictness, ScopeSafety scopeSafety) :
			s((p.stack.isEmpty() ? nullptr : &p.stack.getLast()->s), &p.parent.root, obj, &p.parent),
			strictness(scopeStrictness),
			safety(scopeSafety)
		{}

		const Strictness strictness;
		const ScopeSafety safety;

		Scope s;
	};

	using DR = DiagnosticBase::DiagnosticResult;

	ScopeSafety getMinimumRequiredScopeSafety() const
	{
	
		auto onInitAllowed = true;
		
		bool nonRealtimeAllowed = true;
		bool warnAtNonRealtime = false;

		for (auto s : stack)
		{
			if (s->strictness > Strictness::AllowInit)
				onInitAllowed = false;

			if (s->strictness > Strictness::Unsafe)
			{
				nonRealtimeAllowed = false;
				warnAtNonRealtime = s->strictness == Strictness::Warn;
			}
				
		}

		if (!nonRealtimeAllowed)
			return warnAtNonRealtime ? ScopeSafety::Warning : ScopeSafety::Safe;

		if (!onInitAllowed)
			return ScopeSafety::Unsafe;
	}

	DR push(DynamicObject::Ptr newScope, Strictness strictness = Strictness::Unset, ScopeSafety safety = ScopeSafety::Unknown)
	{
		stack.add(new ParserScope(*this, newScope.get(), strictness, safety));

		if (needsRealtimeSafety() && safety != ScopeSafety::Safe)
		{
			return DR("pushing a non realtime safe scope")
				.withSeverity(safety == ScopeSafety::Unsafe ? DR::Severity::Error : DR::Severity::Warning)
				.withClassification(DR::Classification::CallScope)
				.withSuggestion("inline function");
		}

		return DR::ok();
	}

	Scope& getCurrentScope() { return stack.getLast()->s; }

	DynamicObject::Ptr getCurrentScopeObject() const { return stack.getLast()->s.scope; }

	DynamicObject::Ptr pop()
	{
		if (auto last = stack.getLast())
		{
			auto obj = last->s.scope;

			stack.removeLast();

			return obj;
		}

		return nullptr;
	}

private:

	bool needsRealtimeSafety() const
	{
		auto needs = false;

		for (auto s : stack)
			needs |= s->strictness == Strictness::Strict;

		return needs;
	};

	ObjectTypeMap& parent;
	OwnedArray<ParserScope> stack;
};

HiseJavascriptEngine::RootObject::ObjectTypeMap::TypeScope::TypeScope(ObjectTypeMap& otm, Range<int> codeLocation_, Strictness strictness, ScopeSafety safety) :
	parent(otm),
	codeLocation(codeLocation_)
{
	callScopeReport = parent.scopeStack->push(new DynamicObject(), strictness, safety);
}

HiseJavascriptEngine::RootObject::ObjectTypeMap::TypeScope::~TypeScope()
{
	auto scopeToBeDeleted = parent.scopeStack->pop();

	parent.scopeHistory.push_back({ codeLocation, scopeToBeDeleted });
	parent.currentThisObject = prevThisObject;
}

HiseJavascriptEngine::RootObject::Scope* HiseJavascriptEngine::RootObject::ObjectTypeMap::TypeScope::getCurrentScope() const
{
	return &parent.scopeStack->getCurrentScope();
}

DynamicObject* HiseJavascriptEngine::RootObject::ObjectTypeMap::TypeScope::getCurrentScopeObject() const
{
	return parent.scopeStack->getCurrentScopeObject().get();
}

HiseJavascriptEngine::RootObject::ObjectTypeMap::ScopedCallStack::ScopedCallStack(ShadowParserBase* parser, const Statement* s, const String& customStackName) :
	parent(*dynamic_cast<ObjectTypeMap*>(parser))
{
	UriLocation ci;

	s->location.fillLocation(ci);

	auto expression = customStackName;

	if (expression.isEmpty())
		expression = s->getProfileName();

	parent.currentCallstack.add({ expression, ci });
}

HiseJavascriptEngine::RootObject::ObjectTypeMap::ScopedCallStack::~ScopedCallStack()
{
	parent.currentCallstack.removeLast();
}



HiseJavascriptEngine::RootObject::ObjectTypeMap::ObjectTypeMap(RootObject& root_, Array<UriError>* target) :
	root(root_),
	pwsc(dynamic_cast<ProcessorWithScriptingContent*>(root_.hiseSpecialData.processor)),
	scopeStack(new ScopeStack(*this)),
	diagnostics(target)
{

	DynamicObject::Ptr rootScope = new DynamicObject();

	using CS = WeakCallbackHolder::CallableObject::CallScope;

#define ADD_INBUILT_FUNCTION(id, returnType, argType, callScope) rootScope->setProperty(id, new InbuiltFunction(pwsc, id, returnType, argType, callScope));

	ADD_INBUILT_FUNCTION("exec", VarTypeChecker::NotUndefined, VarTypeChecker::String, CS::Unsafe);
	ADD_INBUILT_FUNCTION("eval", VarTypeChecker::NotUndefined, VarTypeChecker::String, CS::Unsafe);
	ADD_INBUILT_FUNCTION("trace", VarTypeChecker::String, VarTypeChecker::NotUndefined, CS::Warning);
	ADD_INBUILT_FUNCTION("charToInt", VarTypeChecker::Integer, VarTypeChecker::String, CS::Safe);
	ADD_INBUILT_FUNCTION("parseInt", VarTypeChecker::Integer, VarTypeChecker::String, CS::Safe);
	ADD_INBUILT_FUNCTION("parseFloat", VarTypeChecker::Double, VarTypeChecker::String, CS::Safe);
	ADD_INBUILT_FUNCTION("isNan", VarTypeChecker::Integer, VarTypeChecker::Number, CS::Safe);
	ADD_INBUILT_FUNCTION("isFinite", VarTypeChecker::Integer, VarTypeChecker::Number, CS::Safe);
	ADD_INBUILT_FUNCTION("typeof", VarTypeChecker::String, VarTypeChecker::NotUndefined, CS::Unsafe);
	ADD_INBUILT_FUNCTION("Rectangle", VarTypeChecker::ScriptObject, VarTypeChecker::NotUndefined, CS::Unsafe);
	
#undef ADD_INBUILT_FUNCTION
	
	rootScope->setProperty("AsyncNotification", ApiHelpers::AsyncMagicNumber);
	rootScope->setProperty("AsyncHiPriorityNotification", ApiHelpers::AsyncHiPriorityMagicNumber);
	rootScope->setProperty("SyncNotification", ApiHelpers::SyncMagicNumber);

	auto bf = getApiClassFromMap("Buffer");
	bf->setIsStaticObject(true);
	
	rootScope->setProperty("Buffer", bf.get());
	rootScope->setProperty("Content", getApiClassFromMap("Content").get());

	scopeStack->push(rootScope, Strictness::AllowInit, ScopeSafety::Init);
}

HiseJavascriptEngine::RootObject::ObjectTypeMap::~ObjectTypeMap()
{
	delete scopeStack;
	scopeStack = nullptr;
}

HiseJavascriptEngine::RootObject::ObjectTypeMap::ScopeSafety HiseJavascriptEngine::RootObject::ObjectTypeMap::getMinimumRequiredScopeSafety() const
{
	return scopeStack->getMinimumRequiredScopeSafety();
}

HiseJavascriptEngine::RootObject::ObjectTypeMap::Strictness HiseJavascriptEngine::RootObject::ObjectTypeMap::getStrictnessForCallback(const Identifier& callbackId) const
{
	std::map<Identifier, Strictness> defaultStrictness;

	auto midiCallbackStrictness = Strictness::Strict;

	if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(pwsc))
	{
		if (jmp->isDeferred())
			midiCallbackStrictness = Strictness::Unsafe;
	}

	defaultStrictness["onInit"] = Strictness::AllowInit;
	defaultStrictness["onNoteOn"] = midiCallbackStrictness;
	defaultStrictness["onNoteOff"] = midiCallbackStrictness;
	defaultStrictness["onController"] = midiCallbackStrictness;
	defaultStrictness["onTimer"] = midiCallbackStrictness;
	defaultStrictness["onControl"] = Strictness::Unsafe;
	defaultStrictness["prepareToPlay"] = Strictness::Unsafe;
	defaultStrictness["process"] = Strictness::Strict;

	if (defaultStrictness.find(callbackId) != defaultStrictness.end())
		return defaultStrictness.at(callbackId);

	return Strictness::Unset;
}



LightweightDiagnostics::Ptr HiseJavascriptEngine::RootObject::ObjectTypeMap::getLightweighDiagnosticsFromVar(const var& obj) const
{
	if (auto lb = dynamic_cast<LightweightDiagnostics*>(obj.getObject()))
		return lb;

	auto type = VarTypeChecker::getType(obj);

	if (type.asPrimitiveType() == VarTypeChecker::VarTypes::Array)
		return lightweightDiagnostics.createLightweightPrototype(pwsc, "Array", "");

    if (type.asPrimitiveType() == VarTypeChecker::VarTypes::String)
        return lightweightDiagnostics.createLightweightPrototype(pwsc, "String", obj.toString());

    
	if (auto js = obj.getDynamicObject())
		return lightweightDiagnostics.createLightweightPrototype(pwsc, "JSON", JSON::toString(obj));

	return lightweightDiagnostics.createLightweightPrototype(pwsc, VarTypeChecker::getTypeName(type), "");
}

HiseJavascriptEngine::RootObject::Scope& HiseJavascriptEngine::RootObject::ObjectTypeMap::getCurrentScope()
{
	return scopeStack->getCurrentScope();
}

void HiseJavascriptEngine::RootObject::ObjectTypeMap::shadowParse(Statement* s)
{
	var rv;
	auto resultCode = s->perform(getCurrentScope(), &rv);
}

void HiseJavascriptEngine::RootObject::ObjectTypeMap::addDiagnostic(const UriError& i)
{
	auto item = i;

	item.processorId = dynamic_cast<Processor*>(pwsc)->getId();

	auto ec = DiagnosticBase::DiagnosticResult::getErrorCodeFromMetadata(item.metadata);

	if (ec == DiagnosticBase::DiagnosticResult::ErrorCode::CallScopeMismatch && callscopeAnnotation.isNotEmpty())
		item.errorMessage << ". " << callscopeAnnotation;
	
	for (const auto& cs: currentCallstack)
	{
		if (cs.second.matches(item))
			continue;

		item.callstack.push_back(cs);
	}

	diagnostics->add(item);
}

HiseJavascriptEngine::ScopeTree HiseJavascriptEngine::RootObject::ObjectTypeMap::getScopeTree() const
{
	auto history = scopeHistory;
	history.push_back({ Range<int>(0, INT_MAX), scopeStack->getCurrentScopeObject() });
	return history;
}



HiseJavascriptEngine::RootObject::ObjectTypeMap::ScopedLocationSetter::ScopedLocationSetter(ObjectTypeMap& m, const Statement* s) :
	parent(m),
	prevValue(m.currentLocation)
{
	m.currentLocation = &s->location;
}

}