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


void ApiValidationAnalyzer::setDiagnosticTarget(Array<ApiDiagnostic>* target, RO::HiseSpecialData* hsd)
{
	diagnostics = target;
	hiseSpecialData = hsd;
}

/** Run the QueryFunction for a method (if registered) and return the result.
Does NOT emit — the caller coalesces with other checks before emitting. */
ApiClass::DiagnosticResult ApiValidationAnalyzer::getQueryDiagnostic(DiagnosticBase* c, Statement* s, const Identifier& methodName)
{
	if (!c->hasDiagnosticCheck(methodName))
		return DR::ok();

	Array<var> diagnosticArgs;

	if (auto fc = dynamic_cast<RO::FunctionCall*>(s))
	{
		for (auto a : fc->arguments)
			diagnosticArgs.add(evaluateIfNoSideEffect(a));
	}
	if (auto ac = dynamic_cast<RO::ApiCall*>(s))
	{
		for (int i = 0; i < ac->expectedNumArguments; i++)
		{
			auto a = ac->argumentList[i].get();
			diagnosticArgs.add(evaluateIfNoSideEffect(a));
		}
	}

	if (auto lw = dynamic_cast<LightweightDiagnostics*>(c))
		lw->currentParentValidator = this;

	return c->performDiagnostic(methodName, diagnosticArgs);
}

String ApiValidationAnalyzer::getArgIdFromExpression(RO::Expression* e) const
{
	if (auto ac = dynamic_cast<RO::ApiCall*>(e))
	{
		return evaluateIfNoSideEffect(ac->argumentList[0].get()).toString();
	}

	return {};
}

void ApiValidationAnalyzer::process(Statement* s)
{
	if (diagnostics == nullptr)
		return;

	if (auto fc = dynamic_cast<RO::FunctionObject*>(s))
	{
		jassertfalse;
		return;
	}

	else if (auto lit = dynamic_cast<RO::LiteralValue*>(s))
		processInternal(lit);
	else if (auto anon = dynamic_cast<RO::AnonymousFunctionWithCapture*>(s))
		processInternal(anon);
	else if (auto dot = dynamic_cast<RO::DotOperator*>(s))
		processInternal(dot);
	else if (auto ifc = dynamic_cast<RO::InlineFunction::FunctionCall*>(s))
		processInternal(ifc);
	else if (auto ac = dynamic_cast<RO::ApiCall*>(s))
		processInternal(ac);
	else if (auto cas = dynamic_cast<RO::ConstVarStatement*>(s))
		processInternal(cas);
	else if (auto vs = dynamic_cast<RO::VarStatement*>(s))
		processInternal(vs);
	else if (auto ra = dynamic_cast<RO::RegisterVarStatement*>(s))
		processInternal(ra);
	else if (auto ll = dynamic_cast<RO::LocalVarStatement*>(s))
		processInternal(ll);
	else if (auto as = dynamic_cast<RO::Assignment*>(s))
		processInternal(as);
	else if (auto funcCall = dynamic_cast<RO::FunctionCall*>(s))
		processInternal(funcCall);
	else if (auto rt = dynamic_cast<RO::ReturnStatement*>(s))
		processInternal(rt);

	int idx = 0;

	auto tv = returnItem;

	while (auto c = s->getChildStatement(idx))
	{
		process(c);
		idx++;
	}

	// If the returnItem has changed we need to reevaluate the expression
	// and store the object type in the type map after the function call has been evaluated...
	if (tv.obj != returnItem.obj)
	{
		if (auto cas = dynamic_cast<RO::ConstVarStatement*>(s))
			processInternal(cas);
		else if (auto vs = dynamic_cast<RO::VarStatement*>(s))
			processInternal(vs);
		else if (auto ra = dynamic_cast<RO::RegisterVarStatement*>(s))
			processInternal(ra);
		else if (auto ll = dynamic_cast<RO::LocalVarStatement*>(s))
			processInternal(ll);
	}
}

void ApiValidationAnalyzer::processAssignment(const String& expression, RO::Expression* initialiser)
{
	if (returnItem)
	{
		getOrCreateObjectTypeMap()->addReference(expression, returnItem);
		returnItem = {};
		return;
	}

	if (auto lit = dynamic_cast<RO::LiteralValue*>(initialiser))
	{
		if (auto* fo = dynamic_cast<RO::FunctionObject*>(lit->value.getDynamicObject()))
		{
			if (fo->body != nullptr)
			{
				FunctionBody fb;
				fb.body = fo->body.get();

				for (auto p : fo->parameters)
					fb.parameterNames.push_back(p.toString());

				functionBodies[expression] = fb;
				return;
			}
		}
	}

	auto setFromApiCall = [&](const String& expression, RO::Expression* e)
	{
		auto rt = e->getObjectTypeInformation(hiseSpecialData->root);

		if (rt.isValid())
		{
			getOrCreateObjectTypeMap()->setObjectTypeInTypeMap(expression, rt, getArgIdFromExpression(e));
			return true;
		}
		else
			return getOrCreateObjectTypeMap()->removeObjectTypeInTypeMap(expression);

		return false;
	};

	setFromApiCall(expression, initialiser);

	if (auto arc = dynamic_cast<RO::ArrayDeclaration*>(initialiser))
	{
		int idx = 0;

		for (auto c : arc->values)
		{
			String id;
			id << expression << "[" + String(idx++) << "]";
			setFromApiCall(id, c);
		}
	}
}

void ApiValidationAnalyzer::processInternal(RO::DotOperator* dot)
{
	auto pName = dot->parent->getProfileName();

	if (auto to = getOrCreateObjectTypeMap()->getObjectTypeFromTypeMap(pName))
	{
		if (auto lw = dynamic_cast<LightweightDiagnostics*>(to.obj.get()))
		{
			if (!lw->checkProperty(dot->child))
			{
				addDiagnostic(dot->location, "Missing property: " + dot->child.toString(), {}, SV::Error, CS::ApiValidation);
			}
		}
	}
}

void ApiValidationAnalyzer::processInternal(RO::FunctionCall* funcCall)
{
	if (auto* dot = dynamic_cast<RO::DotOperator*>(funcCall->object.get()))
	{
		// try the most generic resolution through the variable name
		if (validateExpression(funcCall, dot, dot->parent->getVariableName().toString()))
			return;

		auto validateArrayElement = [&](RO::ConstReference* cs, int idx)
		{
			String exp;

			if (cs->ns != nullptr && cs->ns != hiseSpecialData)
				exp << cs->ns->id.toString() << ".";

			exp << cs->getVariableName() << "[" << String(idx) << "]";

			return validateExpression(funcCall, dot, exp);
		};

		auto methodName = dot->child.toString();



		if (auto* constRef = dynamic_cast<RO::ConstReference*>(dot->parent.get()))
		{
			// --- Tier 2: Exact resolution via ConstReference ---
			validateTier2(funcCall, dot, constRef);
		}
		else if (auto ar = dynamic_cast<RO::ArraySubscript*>(dot->parent.get()))
		{
			if (auto cs = dynamic_cast<RO::ConstReference*>(ar->object.get()))
			{
				if (ar->index->isConstant())
				{
					auto idx = (int)ar->index->getResult(RO::Scope(nullptr, hiseSpecialData->root, new DynamicObject()));
					validateArrayElement(cs, idx);
				}
			}
		}
		else if (auto lp = dynamic_cast<RO::LoopStatement::IteratorName*>(dot->parent.get()))
		{
			if (auto cs = dynamic_cast<RO::ConstReference*>(lp->loopToUse->currentIterator.get()))
			{
				int idx = 0;

				while (validateArrayElement(cs, idx))
					idx++;
			}
		}
		else
		{
			// --- Tier 3: Greedy lookup across all API classes ---
			validateTier3(funcCall, dot);
		}
	}

	auto funcId = funcCall->object->getVariableName().toString();

	if (functionBodies.find(funcId) != functionBodies.end())
	{
		const auto& f = functionBodies.at(funcId);

		int idx = 0;

		RO::ObjectTypeMap::TypeScope s(*hiseSpecialData->root->objectTypeMap);

		for (auto a : funcCall->arguments)
		{
			auto id = a->getProfileName();
			auto pName = f.parameterNames[idx++];


			if (s.addReference(id, pName))
				continue;

			auto value = evaluateIfNoSideEffect(a);
			s.setConstantScopedValue(pName, value);
		}

		process(f.body);
	}
	
}

void ApiValidationAnalyzer::processInternal(RO::InlineFunction::FunctionCall* ifc)
{
	auto functionName = ifc->f->name;
	auto pNames = ifc->f->parameterNames;
	auto& pExpressions = ifc->parameterExpressions;

	RO::ObjectTypeMap::TypeScope s(*hiseSpecialData->root->objectTypeMap);

	for (int i = 0; i < pExpressions.size(); i++)
	{
		auto e = pExpressions[i]->getProfileName();

		if (s.addReference(e, pNames[i].id.toString()))
			continue;

		auto value = evaluateIfNoSideEffect(pExpressions[i]);
		s.setConstantScopedValue(pNames[i].id, value);

	}

	returnItem = {};

	process(ifc->f->body);
}

void ApiValidationAnalyzer::processInternal(RO::LiteralValue* lit)
{
	// --- Walk into anonymous function bodies ---
	// LiteralValue and AnonymousFunctionWithCapture wrap FunctionObject in a var,
	// making getChildStatement() return nullptr and blocking the tree walker.
	// We extract the FunctionObject and recursively walk its body so that
	// g.fillRect(...) etc. inside paint routines get validated.
	if (auto* fo = dynamic_cast<RO::FunctionObject*>(lit->value.getDynamicObject()))
	{
		if (fo->body != nullptr)
		{
			clearAllPrototypeTouchedState();
			//process(fo->body.get());
		}
	}
}

void ApiValidationAnalyzer::processInternal(RO::AnonymousFunctionWithCapture* anon)
{
	if (auto* fo = dynamic_cast<RO::FunctionObject*>(anon->function.getDynamicObject()))
	{
		if (fo->body != nullptr)
		{
			clearAllPrototypeTouchedState();
			process(fo->body);
		}
	}
}

// --- Tier 1: ApiCall (global API classes like Synth, Engine, Console) ---
void ApiValidationAnalyzer::processInternal(RO::ApiCall* ac)
{
	auto className = ac->apiClass->getObjectName();
	auto methodName = ac->functionName;

	auto returnType = getOrCreateObjectTypeMap()->getReturnTypeFromApiMap(className, methodName);

	auto diagnosticObj = getOrCreateObjectTypeMap()->getApiClassFromMap(className);

	auto queryDr = getQueryDiagnostic(diagnosticObj, ac, methodName);
	ac->setObjectTypeInformation(returnType);

	if (queryDr.shouldReport())
	{
		auto className = ac->apiClass->getObjectName().toString();
		emitDiagnostic(ac->location, className, ac->functionName, queryDr);
	}
}

}