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

/** AST analysis pass that validates method calls on const var objects and collects
	DiagnosticPlaceholder nodes left by the parser's diagnostic-mode recovery sites.

	Tier 2 (exact): FunctionCall + DotOperator + ConstReference → resolve ConstScriptingObject,
					validate method existence, argument count, and literal argument types.
	Tier 3 (greedy): FunctionCall + DotOperator without ConstReference → check method name
					 against ALL known API classes via the enrichment ValueTree.

	This is a read-only pass — never replaces statements. */
struct ApiValidationAnalyzer
{
	using RO = HiseJavascriptEngine::RootObject;
	using Statement = RO::Statement;
	using ApiDiagnostic = RO::ApiDiagnostic;
	using CS = ApiClass::DiagnosticResult::Classification;
	using SV = ApiClass::DiagnosticResult::Severity;
	using DR = ApiClass::DiagnosticResult;

	void setDiagnosticTarget(Array<ApiDiagnostic>* target, RO::HiseSpecialData* hsd);

	/** Run the QueryFunction for a method (if registered) and return the result.
		Does NOT emit — the caller coalesces with other checks before emitting. */
	DR getQueryDiagnostic(DiagnosticBase* c, Statement* s, const Identifier& methodName);

	DR checkContentAddFunction(RO::FunctionCall* funcCall)
	{
		auto numArgs = funcCall->arguments.size();

		if (numArgs != 1)
		{
			String msg;
			msg << "omit the x, y arguments to avoid repositioning UI components after recompilation.";
			return DR::fail(msg);
		}

		return DR::ok();
	}

	

	/** Emit a single diagnostic with "ClassName.methodName - message" format. */
	void emitDiagnostic(const RO::CodeLocation& loc, const String& className,
		const Identifier& methodName, const DR& dr)
	{
		String msg;
		msg << className << "." << methodName.toString();

		auto errorMsg = dr.getErrorMessage();
		if (errorMsg.isNotEmpty())
			msg << " - " << errorMsg;

		addDiagnostic(loc, msg, dr.getSuggestions(), dr.getSeverity(), dr.getClassification());
	}

	/** Emit method-not-found with fuzzy suggestion. */
	void emitMethodNotFound(RO::FunctionCall* funcCall, ConstScriptingObject* cso,
		const String& className, const Identifier& methodName)
	{
		StringArray candidates;
		Array<Identifier> funcIds;
		cso->getAllFunctionNames(funcIds);
		for (auto& id : funcIds)
			candidates.add(id.toString());

		String msg = className + " has no method '" + methodName.toString() + "'";

		StringArray suggestions;
		auto suggestion = FuzzySearcher::suggestCorrection(methodName.toString(), candidates, 0.6);
		if (suggestion.isNotEmpty())
			suggestions.add(suggestion);

		addDiagnostic(funcCall->location, msg, suggestions, SV::Error, CS::ApiValidation);
	}

	RO::ObjectTypeMap* getOrCreateObjectTypeMap() const
	{
		return hiseSpecialData->root->objectTypeMap.get();
	}

	String getArgIdFromExpression(RO::Expression* e) const;

	String getConstExpression(RO::ConstVarStatement* cas) const
	{
		String exp;

		if (cas->ns != nullptr && cas->ns != hiseSpecialData)
			exp << cas->ns->id.toString();

		if (exp.isNotEmpty())
			exp << ".";

		exp << cas->name;

		return exp;
	}

	String getConstExpression(RO::ConstReference* cref) const
	{
		String exp;

		if (cref->ns != nullptr && cref->ns != hiseSpecialData)
			exp << cref->ns->id.toString();

		if (exp.isNotEmpty())
			exp << ".";

		exp << cref->getVariableName();

		return exp;
	}

	var evaluateIfNoSideEffect(RO::Expression* e) const
	{
		if (!e->hasSideEffects())
		{
			try
			{
				auto v = e->getResult(RO::Scope(nullptr, hiseSpecialData->root, hiseSpecialData->root));

				if (!v.isUndefined())
					return v;
			}
			catch (RO::Error&) {}
		}

		auto expString = e->getProfileName();
		return getOrCreateObjectTypeMap()->getScopedConstant(expString);
	}

	bool validateExpression(RO::FunctionCall* funcCall, RO::DotOperator* dot, const String& expression)
	{
		if (expression == "Content")
		{
			RO::ObjectTypeMap::ObjectTypeItem ti;
			ti.expression = expression;
			ti.typeInfo = expression;
			ti.obj = getOrCreateObjectTypeMap()->getApiClassFromMap(expression);

			validateWithKnownTypeId(funcCall, dot, ti);
			return true;
		}

		auto objType = getOrCreateObjectTypeMap()->getObjectTypeFromTypeMap(expression);
		validateWithKnownTypeId(funcCall, dot, objType);
		return objType.obj.get() != nullptr;
	}

	void process(Statement* s);

	void processAssignment(const String& expression, RO::Expression* initialiser);

	void processInternal(RO::VarStatement* vs)
	{
		processAssignment(vs->name.toString(), vs->initialiser.get());
	}

	void processInternal(RO::ConstVarStatement* cas)
	{
		processAssignment(getConstExpression(cas), cas->initialiser.get());
	}

	void processInternal(RO::LocalVarStatement* ls)
	{
		processAssignment(ls->name.toString(), ls->initialiser.get());
	}

	void processInternal(RO::Assignment* as)
	{
		processAssignment(as->target->getVariableName().toString(), as->newValue.get());
	}

	void processInternal(RO::RegisterVarStatement* rv)
	{
		processAssignment(rv->name.toString(), rv->initialiser.get());
	}

	void processInternal(RO::DotOperator* dot);

	// --- FunctionCall: check for DotOperator pattern ---
	void processInternal(RO::FunctionCall* funcCall);

	void processInternal(RO::InlineFunction::FunctionCall* ifc);

	void processInternal(RO::LiteralValue* lit);

	void processInternal(RO::AnonymousFunctionWithCapture* anon);

	void processInternal(RO::ReturnStatement* rs)
	{
		if (rs->returnValue != nullptr)
		{
			auto rv = evaluateIfNoSideEffect(rs->returnValue).toString();

			if (rv.isEmpty())
				rv = rs->returnValue->getProfileName();

			returnItem = getOrCreateObjectTypeMap()->getObjectTypeFromTypeMap(rv);
		}
	}

	// --- Tier 1: ApiCall (global API classes like Synth, Engine, Console) ---
	void processInternal(RO::ApiCall* ac);

private:

	void validateTier2(RO::FunctionCall* funcCall, RO::DotOperator* dot,
		RO::ConstReference* constRef)
	{
		if (constRef->ns == nullptr)
			return;

		auto constValue = constRef->ns->constObjects.getValueAt(constRef->index);
		auto* cso = dynamic_cast<ConstScriptingObject*>(constValue.getObject());

		String exp;

		if (constRef->ns != hiseSpecialData)
			exp << constRef->ns->id.toString();

		if (exp.isNotEmpty())
			exp << ".";

		exp << constRef->getVariableName();

		auto typeMapType = getOrCreateObjectTypeMap()->getObjectTypeFromTypeMap(exp);

		validateWithKnownTypeId(funcCall, dot, typeMapType);
	}

	void validateWithKnownTypeId(RO::FunctionCall* funcCall, RO::DotOperator* dot,
		const RO::ObjectTypeMap::ObjectTypeItem& resolvedType)
	{
		if (resolvedType.obj.get() == nullptr)
			return;


		auto methodName = dot->child.toString();

		auto argDr = resolvedType.obj->checkArgCount(methodName, funcCall->arguments.size());
		auto queryDr = getQueryDiagnostic(resolvedType.obj.get(), funcCall, methodName);

		auto best = DR::max(queryDr, argDr);

		if (best.shouldReport())
			emitDiagnostic(funcCall->location, resolvedType.typeInfo.toString(), methodName, best);
	}

	void validateTier3(RO::FunctionCall* funcCall, RO::DotOperator* dot)
	{
		jassertfalse;

#if 0
		auto methodName = dot->child.toString();

		auto& methodMap = getGreedyApiMethodMap();
		auto info = methodMap.find(methodName);

		if (info == nullptr)
			return; // Method not found in any API class — can't validate without knowing the object type

		// Method exists somewhere — check arg count if unambiguous
		if (info->ambiguousArgCount)
			return; // Different classes have different arg counts — can't validate

		auto objectKey = dot->parent->getProfileName();
		DR queryDr = DR::ok();

		if (auto* proto = getOrCreatePrototype(info->className, objectKey))
		{
			proto->markMethodTouched(Identifier(methodName));
			queryDr = getQueryDiagnostic(proto, funcCall, Identifier(methodName));
		}

		auto isContentAddFunction = info->className == Identifier("Content")
			&& methodName.startsWith("add")
			&& methodName != "addVisualGuide";

		auto argDr = isContentAddFunction ? checkContentAddFunction(funcCall) : checkArgCount(funcCall, info->numArgs);

		// Tier 3 arg count is Warning (not Error) — less certainty without type info
		if (argDr.shouldReport())
			argDr = argDr.withSeverity(DR::Severity::Warning);



		auto best = DR::max(queryDr, argDr);

		if (best.shouldReport())
		{
			if (info->className.isValid())
			{
				emitDiagnostic(funcCall->location, info->className.toString(),
					Identifier(methodName), best);
			}
			else
			{
				// Ambiguous class — fall back to generic format
				String msg = "Method '" + methodName + "'";
				auto errorMsg = best.getErrorMessage();
				if (errorMsg.isNotEmpty())
					msg << " - " << errorMsg;

				addDiagnostic(funcCall->location, msg, best.getSuggestions(),
					best.getSeverity(), best.getClassification());
			}
		}
#endif

		// No type checking for Tier 3 — we don't know the concrete class
	}

	void checkLiteralArgTypes(RO::FunctionCall* funcCall, ConstScriptingObject* cso,
		int functionIndex, int numArgs,
		const String& className, const String& methodName)
	{
		auto types = cso->getForcedParameterTypes(functionIndex, numArgs);

		for (int i = 0; i < numArgs; i++)
		{
			auto expectedType = types[i];

			if (expectedType == VarTypeChecker::Undefined)
				continue; // No type constraint for this parameter

			var argValue;
			bool hasKnownType = false;

			if (auto* literal = dynamic_cast<RO::LiteralValue*>(funcCall->arguments[i]))
			{
				argValue = literal->value;
				hasKnownType = true;
			}
			else if (auto* constRef = dynamic_cast<RO::ConstReference*>(funcCall->arguments[i]))
			{
				if (constRef->ns != nullptr)
				{
					argValue = constRef->ns->constObjects.getValueAt(constRef->index);
					hasKnownType = true;
				}
			}

			if (!hasKnownType)
				continue; // Dynamic expression — skip type check

			auto actualType = VarTypeChecker::getType(argValue);

			if ((actualType & expectedType) == 0 && actualType != VarTypeChecker::Undefined)
			{
				auto expectedName = VarTypeChecker::getTypeName(expectedType);
				auto actualName = VarTypeChecker::getTypeName(actualType);

				String msg = className + "." + methodName + "() parameter " + String(i + 1)
					+ " requires " + expectedName + ", got " + actualName;

				addDiagnostic(funcCall->arguments[i]->location, msg, {}, SV::Error, CS::TypeCheck);
			}
		}
	}

	void addDiagnostic(const RO::CodeLocation& loc, const String& message,
		const StringArray& suggestions, SV severity, CS source)
	{
		ApiDiagnostic d;
		loc.fillColumnAndLines(d.col, d.line, d.charIndex);
		d.fileName = loc.externalFile;
		d.message = message;
		d.suggestions = suggestions;
		d.severity = severity;
		d.classification = source;
		diagnostics->add(d);
	}

	// ==================== Greedy API Method Map (Tier 3) ====================

	struct GreedyMethodInfo
	{
		int numArgs = 0;           // Consistent arg count (if not ambiguous)
		bool ambiguousArgCount = false;  // True if different classes disagree on arg count
		Identifier className;
	};

	struct GreedyApiMethodMap
	{
		HashMap<String, GreedyMethodInfo> map;
		StringArray allMethodNames;  // For fuzzy search
		bool built = false;

		GreedyMethodInfo* find(const String& methodName)
		{
			if (map.contains(methodName))
				return &map.getReference(methodName);
			return nullptr;
		}

		void buildFromTree(const ValueTree& apiTree)
		{
			for (const auto& classNode : apiTree)
			{
				auto parentClassName = classNode.getType();

				for (const auto& methodNode : classNode)
				{
					auto name = methodNode.getProperty("name").toString();

					if (name.isEmpty())
						continue;

					auto argStr = methodNode.getProperty("arguments").toString();
					int argCount = countArgsFromString(argStr);

					if (map.contains(name))
					{
						auto& existing = map.getReference(name);

						if (existing.className != parentClassName)
							existing.className = {};

						if (!existing.ambiguousArgCount && existing.numArgs != argCount)
							existing.ambiguousArgCount = true;
					}
					else
					{
						GreedyMethodInfo info;
						info.numArgs = argCount;
						info.className = parentClassName;
						allMethodNames.add(name);
						map.set(name, info);
					}
				}
			}

			built = true;
		}

		/** Parse an argument string like "(int x, double y)" to count parameters. */
		static int countArgsFromString(const String& argStr)
		{
			auto trimmed = argStr.trim();

			if (trimmed.isEmpty() || trimmed == "()")
				return 0;

			// Remove outer parens
			auto inner = trimmed.substring(1, trimmed.length() - 1).trim();

			if (inner.isEmpty())
				return 0;

			// Count commas + 1
			int count = 1;

			for (int i = 0; i < inner.length(); i++)
			{
				if (inner[i] == ',')
					count++;
			}

			return count;
		}
	};

	static GreedyApiMethodMap& getGreedyApiMethodMap()
	{
		static GreedyApiMethodMap instance;

		if (!instance.built)
			instance.buildFromTree(ApiHelpers::getApiTree());

		return instance;
	}

	DiagnosticBase* getOrCreatePrototype(const Identifier& className, const String& expression)
	{
		std::pair<Identifier, String> key = { className, expression };

		if (prototypeCache.find(key) != prototypeCache.end())
			return prototypeCache.at(key).get();

		auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(hiseSpecialData->processor);
		jassert(pwsc != nullptr);

		auto n = ConstScriptingObject::createDiagnosticPrototype(className, pwsc);

		if (n != nullptr)
			prototypeCache[key] = n;

		return n.get();
	}

	void clearAllPrototypeTouchedState()
	{
		for (auto& pair : prototypeCache)
			pair.second->clearTouchedMethods();
	}

	Array<ApiDiagnostic>* diagnostics = nullptr;
	RO::HiseSpecialData* hiseSpecialData = nullptr;
	// Contains prototypes for all tier 3 function call parents that can be resolved unambiguously
	// by their method name
	std::map<std::pair<Identifier, String>, ReferenceCountedObjectPtr<DiagnosticBase>> prototypeCache;

	struct FunctionBody
	{
		std::vector<String> parameterNames;
		Statement* body;
	};

	RO::ObjectTypeMap::ObjectTypeItem returnItem;

	std::map<String, FunctionBody> functionBodies;

};

}