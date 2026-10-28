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

#pragma once

namespace hise {
using namespace juce;

struct HiseJavascriptEngine::RootObject::ObjectTypeMap : public ReferenceCountedObject,
	public DiagnosticBase::ShadowParserBase
{
	using Ptr = ReferenceCountedObjectPtr<ObjectTypeMap>;

	using Strictness = WeakCallbackHolder::CallableObject::StrictnessLevel;
	using ScopeSafety = WeakCallbackHolder::CallableObject::CallScope;

	struct TypeScope
	{
		TypeScope(ObjectTypeMap& ro, Range<int> codeLocation_, Strictness s = Strictness::Unset, ScopeSafety safety = ScopeSafety::Unknown);

		void setThisObject(LightweightDiagnostics::Ptr to)
		{
			prevThisObject = parent.currentThisObject;
			parent.currentThisObject = to;
		}

		~TypeScope();

		DiagnosticBase::DiagnosticResult getCallscopeReport() { return callScopeReport; }

		Scope* getCurrentScope() const;
		DynamicObject* getCurrentScopeObject() const;

		ObjectTypeMap& parent;

		LightweightDiagnostics::Ptr prevThisObject;
		Range<int> codeLocation;

		DiagnosticBase::DiagnosticResult callScopeReport;
	};

	ObjectTypeMap(RootObject& parent, Array<ApiDiagnostic>* target);

	~ObjectTypeMap();

	Identifier getReturnTypeFromApiMap(const Identifier& className, const Identifier& methodName) const
	{
		return lightweightDiagnostics.getReturnTypeFromApiMap(className, methodName);
	}

	LightweightDiagnostics::Ptr getApiClassFromMap(const Identifier& apiClassName)
	{
		return lightweightDiagnostics.createLightweightPrototype(pwsc, apiClassName, {});
	}

	LightweightDiagnostics::Ptr getLightweighDiagnosticsFromVar(const var& obj) const;

	LightweightDiagnostics::Ptr createAnonoymousDiagnosticObject(const Identifier& className, const String& argId)
	{
		return lightweightDiagnostics.createLightweightPrototype(pwsc, className, argId);
	}

	LightweightDiagnostics* getCurrentThisObject() const { return currentThisObject.get(); }

	Scope& getCurrentScope();

	void shadowParse(Statement* s);

	void addDiagnostic(const ApiDiagnostic& i) override;

	ScopeTree getScopeTree() const;

	ScopeSafety getMinimumRequiredScopeSafety() const;

	Strictness getStrictnessForCallback(const Identifier& callbackId) const;


	struct ScopedCallStack
	{
		ScopedCallStack(ShadowParserBase* parser, const Statement* s, const String& customStackName = String());

		~ScopedCallStack();

	private:

		ObjectTypeMap& parent;
	};



private:

	struct ScopeStack;

	Array<DiagnosticBase::DiagnosticResult::CallStackItem> currentCallstack;

	RootObject& root;
	ProcessorWithScriptingContent* pwsc;

	ScopeStack* scopeStack;

	ScopeTree scopeHistory;

	CriticalSection lock;

	LightweightDiagnostics::Factory lightweightDiagnostics;
	LightweightDiagnostics::Ptr currentThisObject;
	Array<ApiDiagnostic>* diagnostics;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ObjectTypeMap);
};

using ShadowParser = HiseJavascriptEngine::RootObject::ObjectTypeMap;

}