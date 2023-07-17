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



snex::jit::Operations::FunctionCompiler& Operations::getFunctionCompiler(BaseCompiler* c)
{
	return *dynamic_cast<ClassCompiler*>(c)->asmCompiler;
}

BaseScope* Operations::findClassScope(BaseScope* scope)
{
	if (scope == nullptr)
return nullptr;

if (scope->getScopeType() == BaseScope::Class)
return scope;
else
return findClassScope(scope->getParent());
}

snex::jit::BaseScope* Operations::findFunctionScope(BaseScope* scope)
{
	if (scope == nullptr)
		return nullptr;

	if (scope->getScopeType() == BaseScope::Function)
		return scope;
	else
		return findFunctionScope(scope->getParent());
}


#if SNEX_ASMJIT_BACKEND
AsmJitRuntime* Operations::getRuntime(BaseCompiler* c)
{
	return dynamic_cast<ClassCompiler*>(c)->getRuntime();
}
#endif



snex::jit::TemplateParameter::List Operations::collectParametersFromParentClass(Statement::Ptr p, const TemplateParameter::List& instanceParameters)
{
	TemplateParameter::List list;

	if (auto fc = as<FunctionCall>(p))
	{
		if (auto p = fc->getObjectExpression())
		{
			if (auto st = p->getTypeInfo().getTypedIfComplexType<StructType>())
			{
				list.addArray(st->getTemplateInstanceParameters());
			}
		}
	}
	else
	{
		while (auto cs = findParentStatementOfType<ClassStatement>(p.get()))
		{
			list.addArray(cs->getStructType()->getTemplateInstanceParameters());
			p = cs->parent;
		}
	}

	list.addArray(instanceParameters);
	return list;
}

bool Operations::canBeReferenced(Expression::Ptr p)
{
	if (as<SymbolStatement>(p) != nullptr)
		return true;

	if (as<MemoryReference>(p) != nullptr)
		return true;

	if (as<DotOperator>(p) != nullptr)
	{
		return true;
	}

	auto isRef = p->getTypeInfo().isRef();
	return isRef;
}

snex::jit::Operations::Expression::Ptr Operations::evalConstExpr(Expression::Ptr expr)
{
	auto compiler = expr->currentCompiler;
	auto scope = expr->currentScope;

	if (compiler == nullptr)
		expr->throwError("Can't evaluate expression");

	jassert(compiler != nullptr);
	jassert(scope != nullptr);

	Random r;
	
	Statement::Ptr tr = new SyntaxTree(expr->location, compiler->namespaceHandler.createNonExistentIdForLocation({}, r.nextInt()));
	as<SyntaxTree>(tr)->addStatement(expr.get());

	BaseCompiler::ScopedPassSwitcher sp1(compiler, BaseCompiler::DataAllocation);
	compiler->executePass(BaseCompiler::DataAllocation, scope, tr.get());

	BaseCompiler::ScopedPassSwitcher sp2(compiler, BaseCompiler::PreSymbolOptimization);
	compiler->optimize(expr.get(), scope, false);

	BaseCompiler::ScopedPassSwitcher sp3(compiler, BaseCompiler::ResolvingSymbols);
	compiler->executePass(BaseCompiler::ResolvingSymbols, scope, tr.get());

	BaseCompiler::ScopedPassSwitcher sp4(compiler, BaseCompiler::PostSymbolOptimization);
	compiler->optimize(expr.get(), scope, false);

	return dynamic_cast<Operations::Expression*>(tr->getChildStatement(0).get());
}

void Operations::Expression::attachAsmComment(const juce::String& message)
{
	asmComment = message;
}

TypeInfo Operations::Expression::checkAndSetType(int offset, TypeInfo expectedType)
{
	TypeInfo exprType = expectedType;

	if (expectedType.isInvalid())
	{
		// Native types have precedence so that the complex types can call their cast operators...
		for (int i = offset; i < getNumChildStatements(); i++)
		{
			auto thisType = getChildStatement(i)->getTypeInfo();
			if (thisType.isComplexType())
				continue;

			exprType = thisType;
		}
	}

	for (int i = offset; i < getNumChildStatements(); i++)
	{
		exprType = setTypeForChild(i, exprType);
	}

	return exprType;
}




TypeInfo Operations::Expression::setTypeForChild(int childIndex, TypeInfo expectedType)
{
	auto e = getSubExpr(childIndex);

	if (e == nullptr)
		location.throwError("expected expression");

	if (auto v = dynamic_cast<VariableReference*>(e.get()))
	{
		v->tryToResolveType(currentCompiler);

		if (v->getTypeInfo().isDynamic())
		{
			currentCompiler->namespaceHandler.setTypeInfo(v->id.id, NamespaceHandler::Variable, expectedType);

			v->id = { v->id.id, expectedType };
			return expectedType;
		}

		bool isDifferentType = expectedType != Types::ID::Dynamic && expectedType != v->getConstExprValue().getType();

		if (v->isConstExpr() && isDifferentType)
		{
			// Internal cast of a constexpr variable...
			v->id.constExprValue = VariableStorage(expectedType.getType(), v->id.constExprValue.toDouble());
			return expectedType;
		}
	}

	auto thisType = e->getTypeInfo();


	if (expectedType.isInvalid())
	{
		return thisType;
	}

	if (!thisType.isComplexType() && (thisType == expectedType.getType()))
	{
		// the expected complex type can be implicitely casted to the native type
		return expectedType;
	}

	if (!expectedType.isComplexType() && expectedType == thisType.getType())
	{
		// this type can be implicitely casted to the native expected type;
		return thisType;
	}

	if (expectedType != thisType)
	{
		if (auto targetType = expectedType.getTypedIfComplexType<ComplexType>())
		{
			if (!targetType->isValidCastSource(thisType.getType(), thisType.getTypedIfComplexType<ComplexType>()))
			{
				throwError(juce::String("Can't cast ") + thisType.toString() + " to " + expectedType.toString());
			}
				
		}

		if (auto sourceType = thisType.getTypedIfComplexType<ComplexType>())
		{
			if (!sourceType->isValidCastTarget(expectedType.getType(), expectedType.getTypedIfComplexType<ComplexType>()))
			{
				if (currentCompiler->allowSmallObjectOptimisation() && sourceType->getRegisterType(true) == expectedType.getType())
				{
					return expectedType;
				}

				throwError(juce::String("Can't cast ") + thisType.toString() + " to " + expectedType.toString());
			}
				
		}

		
			

		e->logWarning("Implicit cast, possible lost of data");

		if (e->isConstExpr())
		{
			replaceChildStatement(childIndex, ConstExprEvaluator::evalCast(e.get(), expectedType.getType()).get());
		}
		else
		{
			Ptr implCast = new Operations::Cast(e->location, e, expectedType.getType());
			implCast->attachAsmComment("Implicit cast");

			replaceChildStatement(childIndex, implCast);
		}
	}

	return expectedType;
}



void Operations::Expression::replaceMemoryWithExistingReference(BaseCompiler* compiler)
{
	jassert(reg != nullptr);

	auto prevReg = compiler->registerPool.getRegisterWithMemory(reg);

	ASMJIT_ONLY(if (prevReg != reg) reg->setReferToReg(prevReg));
}

bool Operations::Expression::isAnonymousStatement() const
{
	return isStatementType<StatementBlock>(parent) ||
		isStatementType<SyntaxTree>(parent);
}



snex::VariableStorage Operations::Expression::getConstExprValue() const
{
	if (isConstExpr())
		return dynamic_cast<const Immediate*>(this)->v;

	return {};
}

bool Operations::Expression::hasSubExpr(int index) const
{
	return isPositiveAndBelow(index, getNumChildStatements());
}

snex::VariableStorage Operations::Expression::getPointerValue() const
{
	location.throwError("Can't use address of temporary register");

	RETURN_DEBUG_ONLY({});
}

Operations::Expression::Ptr Operations::Expression::getSubExpr(int index) const
{
	return Expression::Ptr(dynamic_cast<Expression*>(getChildStatement(index).get()));
}

snex::jit::Operations::RegPtr Operations::Expression::getSubRegister(int index) const
{
	// If you hit this, you have either forget to call the 
	// Statement::process() or you try to access an register 
	// way too early...
	jassert(currentPass >= BaseCompiler::Pass::RegisterAllocation);

	if (auto e = getSubExpr(index))
		return e->reg;

	// Can't find the subexpression you want
	jassertfalse;

	return nullptr;
}


SyntaxTree::SyntaxTree(ParserHelpers::CodeLocation l, const NamespacedIdentifier& ns) :
	Statement(l),
	ScopeStatementBase(ns)
{
}

int SyntaxTree::getInlinerScore()
{
	using namespace Operations;

	int score = 0;

	forEachRecursive([&score](Ptr p)
	{
		if (as<VariableReference>(p))
			score += 2;
		else if (as<IfStatement>(p))
			score += 5;
		else if (as<TernaryOp>(p))
			score += 3;
		else if (as<DotOperator>(p))
			score += 0;
		else if (as<WhileLoop>(p))
			score += 40;
		else if (as<Loop>(p))
			score += 25;
		else if (as<Subscript>(p))
			score += 0;
		else if (as<FunctionCall>(p))
			score += 2;
		else if (as<StatementWithControlFlowEffectBase>(p))
			score += 2;
		else
			score += 1;

		return false;
	}, Operations::IterationType::AllChildStatements);

	return score;
}

bool SyntaxTree::isFirstReference(Operations::Statement* v_) const
{
	SyntaxTreeWalker m(v_);

	if(auto v = m.getNextStatementOfType<Operations::VariableReference>())
	{
		return v == v_;
	}

	return false;
}


snex::jit::Operations::Statement::Ptr SyntaxTree::clone(ParserHelpers::CodeLocation l) const
{
	Statement::Ptr c;

	if (dynamic_cast<Operations::ClassStatement*>(parent.get()) != nullptr)
	{
		c = new SyntaxTree(l, getPath());
	}
	else
	{
		c = new Operations::StatementBlock(l, getPath());
		dynamic_cast<Operations::StatementBlock*>(c.get())->isInlinedFunction = true;
	}

	cloneChildren(c);
	return c;
}

Operations::Statement::Statement(Location l) :
	location(l)
{

}

String Operations::Statement::toSimpleTree() const
{
	using namespace cppgen;

	Base b(Base::OutputType::WrapInBlock);
	b << getStatementId().toString();
	b << "{";

	for (auto p : *this)
	{
		b << p->toSimpleTree();
	}

	return b.toString();
}

void Operations::Statement::throwError(const juce::String& errorMessage) const
{
	ParserHelpers::Error e(location);
	e.errorMessage = errorMessage;
	throw e;
}

void Operations::Statement::logOptimisationMessage(const juce::String& m)
{
	logMessage(currentCompiler, BaseCompiler::VerboseProcessMessage, m);
}

void Operations::Statement::logWarning(const juce::String& m)
{
	logMessage(currentCompiler, BaseCompiler::Warning, m);
}

bool Operations::Statement::forEachRecursive(const std::function<bool(Ptr)>& f, IterationType it)
{
	if (f(this))
		return true;

	if (it == IterationType::NoInlineFunctionBlocks)
	{
		if (auto sb = as<StatementBlock>(this))
		{
			if (sb->isInlinedFunction)
				return false;
		}
	}

	auto itToUse = it == IterationType::NoChildInlineFunctionBlocks ? IterationType::NoInlineFunctionBlocks : it;

	for (int i = 0; i < getNumChildStatements(); i++)
	{
		auto c = getChildStatement(i);

		if (c->forEachRecursive(f, itToUse))
			return true;
	}

	return false;
}

bool Operations::Statement::replaceIfOverloaded(Ptr objPtr, List args, FunctionClass::SpecialSymbols overloadType)
{
	if (auto cType = objPtr->getTypeInfo().getTypedIfComplexType<ComplexType>())
	{
		if (FunctionClass::Ptr fc = cType->getFunctionClass())
		{
			TypeInfo::List argTypes;

			for (auto a : args)
				argTypes.add(a->getTypeInfo());

			auto returnType = getTypeInfo();

			auto overloadedFunction = fc->getSpecialFunction(overloadType, returnType, argTypes);

			if (overloadedFunction.id.isValid())
			{
				auto path = findParentStatementOfType<ScopeStatementBase>(this)->getPath();

				auto f = new FunctionCall(location, nullptr, overloadedFunction.toSymbol(), {});

				f->setObjectExpression(objPtr->clone(location));

				for(auto a: args)
					f->addArgument(a->clone(location));

				Ptr fPtr(f);
				Ptr thisPtr(this);

				replaceInParent(fPtr);
				SyntaxTreeInlineData::processUpToCurrentPass(thisPtr, fPtr);
				return true;
			}
		}
	}

	

	return false;
}

bool Operations::Statement::isConstExpr() const
{
	return isStatementType<Immediate>(static_cast<const Statement*>(this));
}

void Operations::Statement::addStatement(Statement::Ptr b, bool addFirst/*=false*/)
{
	if (addFirst)
		childStatements.insert(0, b);
	else
		childStatements.add(b);

	b->setParent(this);
}

Operations::Statement::Ptr Operations::Statement::replaceInParent(Statement::Ptr newExpression)
{
	if (parent != nullptr)
	{
		for (int i = 0; i < parent->getNumChildStatements(); i++)
		{
			if (parent->getChildStatement(i).get() == this)
			{
				Ptr f(this);
				parent->childStatements.set(i, newExpression.get());
				newExpression->parent = parent;
				//parent = nullptr;
				return f;
			}
		}
	}

	
	return nullptr;
}


Operations::Statement::Ptr Operations::Statement::replaceChildStatement(int index, Ptr newExpr)
{
	Ptr returnExpr;

	if ((returnExpr = getChildStatement(index)))
	{
		childStatements.set(index, newExpr.get());
		newExpr->parent = this;

		if (returnExpr->parent == this)
			returnExpr->parent = nullptr;
	}
	else
		jassertfalse;

	return returnExpr;
}

void Operations::Statement::removeNoops()
{
	for (int i = 0; i < childStatements.size(); i++)
	{
		if (as<Noop>(childStatements[i]))
		{
			childStatements.remove(i--);
		}
	}
}

void Operations::Statement::logMessage(BaseCompiler* compiler, BaseCompiler::MessageType type, const juce::String& message)
{
	if (!compiler->hasLogger())
		return;

	juce::String m;

	ParserHelpers::Error e(location);
	e.errorMessage = message;

	ParserHelpers::Error::Format f = compiler->useCodeSnippetInErrorMessage() ? ParserHelpers::Error::Format::CodeExample : 
																				ParserHelpers::Error::Format::LineNumbers;

	DBG(e.toString(f));
	compiler->logMessage(type, e.toString(f));
}

void Operations::ConditionalBranch::preallocateVariableRegistersBeforeBranching(Statement::Ptr statementToSearchFor, BaseCompiler* c, BaseScope* s)
{
	auto asScope = as<ScopeStatementBase>(statementToSearchFor);

	if (asScope == nullptr)
	{
		// just grab the nearest scope
		asScope = findParentStatementOfType<ScopeStatementBase>(statementToSearchFor.get());
	}

	jassert(asScope != nullptr);
	auto path = asScope->getPath();

	auto hasChildStatementsWithLocalScope = [&](Statement::Ptr p)
	{
		if (auto v = as<VariableReference>(p))
		{
#if 0
			if (v->variableScope->getParentWithPath(path) != nullptr)
				return true;
#endif

			auto scopeId = v->id.id.getParent();
			return scopeId == path || path.isParentOf(scopeId);
		}

		return false;
	};

	statementToSearchFor->forEachRecursive([&](Statement::Ptr p)
	{
		auto d = as<DotOperator>(p) ;
		auto v = as<VariableReference>(p);

		if (p->forEachRecursive(hasChildStatementsWithLocalScope, IterationType::AllChildStatements))
			return false;

		if (d != nullptr || v != nullptr)
		{
			if (d != nullptr && as<Subscript>(d->getDotParent()))
				return false;

			if (v != nullptr && v->isClassVariable(s))
				v->forceLoadData = true;

			if (p->reg == nullptr)
				p->process(c, s);

			if (p->reg != nullptr)
			{
				auto& cc = getFunctionCompiler(c);

				if (d != nullptr || (v != nullptr && v->isClassVariable(s)))
					p->reg->loadMemoryIntoRegister(cc);

				ASMJIT_ONLY(p->reg->setWriteBackToMemory(true));
			}
		}
		
		return false;
	}, IterationType::AllChildStatements);


#if 0
	SyntaxTreeWalker w(statementToSearchFor, false);

	while (auto v = w.getNextStatementOfType<VariableReference>())
	{
		//v->forceLoadData = true;
		v->process(c, s);

		auto r = v->reg;

		// If use a class variable, we need to create the register
		// outside the loop
		if (v->isFirstReference())
		{
			
		}
			
	}
#endif
}

Operations::Statement::Ptr Operations::ScopeStatementBase::createChildBlock(Location l) const
{
	const auto& nh = dynamic_cast<const Statement*>(this)->currentCompiler->namespaceHandler;

	auto id = nh.createNonExistentIdForLocation(getPath(), l.getLine());
	
	return new StatementBlock(l, id);
}


void Operations::ScopeStatementBase::addDestructors(BaseScope* scope)
{
	Array<Symbol> destructorIds;
	auto path = getPath();

	auto asStatement = dynamic_cast<Statement*>(this);

	asStatement->forEachRecursive([path, &destructorIds, scope](Statement::Ptr p)
	{
		if (auto cd = as<ComplexTypeDefinition>(p))
		{
			if (cd->isStackDefinition(scope))
			{
				if (cd->type.getComplexType()->hasDestructor())
				{
					for (auto& id : cd->getInstanceIds())
					{
						if (path == id.getParent())
						{
							destructorIds.add(Symbol(id, cd->type));
						}
					}
				}
			}
		}

		return false;
	}, IterationType::NoChildInlineFunctionBlocks);

	for (auto dv : destructorIds)
		StatementWithControlFlowEffectBase::addDestructorToAllChildStatements(asStatement, dv);
}

void Operations::ScopeStatementBase::removeStatementsAfterReturn()
{
	bool returnCalled = false;
	bool warningCalled = false;

	auto asStatement = dynamic_cast<Statement*>(this);

	for (int i = 0; i < asStatement->getNumChildStatements(); i++)
	{
		auto c = asStatement->getChildStatement(i);

		if (as<ReturnStatement>(c))
		{
			returnCalled = true;
			continue;
		}

		if (returnCalled)
		{
			if (!warningCalled)
			{
				c->logWarning("Unreachable statement");
				warningCalled = true;
			}

			c->replaceInParent(new Operations::Noop(c->location));
		}
	}
}

void Operations::ScopeStatementBase::setNewPath(BaseCompiler* c, const NamespacedIdentifier& newPath)
{
	auto oldPath = path;

	path = newPath;

	auto asStatement = dynamic_cast<Statement*>(this);

	asStatement->forEachRecursive([c, oldPath, newPath](Statement::Ptr p)
	{
		if (auto b = as<ScopeStatementBase>(p))
		{
			auto scopePath = b->getPath();
			if (oldPath.isParentOf(scopePath))
			{
				auto newScopePath = scopePath.relocate(oldPath, newPath);
				b->path = newScopePath;
			}
		}

		if (auto l = as<Operations::Loop>(p))
		{
			if (oldPath.isParentOf(l->iterator.id))
			{
				auto newIterator = l->iterator.id.relocate(oldPath, newPath);

				l->iterator.id = newIterator;
			}
		}
		if (auto v = as<Operations::VariableReference>(p))
		{
			if (oldPath.isParentOf(v->id.id))
			{
				auto newId = v->id.id.relocate(oldPath, newPath);
				NamespaceHandler::ScopedNamespaceSetter sns(c->namespaceHandler, newId.getParent());
				c->namespaceHandler.addSymbol(newId, v->id.typeInfo, NamespaceHandler::Variable, {});
				v->id.id = newId;
			}
		}

		return false;
	}, IterationType::AllChildStatements);
}

void Operations::ClassDefinitionBase::addMembersFromStatementBlock(StructType* t, Statement::Ptr bl)
{
	for (auto s : *bl)
	{
		// A constructor is wrapped in anonymous block
		if (auto ab = dynamic_cast<AnonymousBlock*>(s))
		{
			s = ab->getSubExpr(0).get();
		}
			

		if (auto td = dynamic_cast<TypeDefinitionBase*>(s))
		{
			auto type = s->getTypeInfo();

			if (type.isDynamic())
				s->location.throwError("Can't use auto on member variables");

			for (auto& id : td->getInstanceIds())
			{
				t->addMember(id.getIdentifier(), type);

				if (type.isTemplateType() && s->getSubExpr(0) != nullptr)
				{
					InitialiserList::Ptr dv = new InitialiserList();
					dv->addImmediateValue(s->getSubExpr(0)->getConstExprValue());
					t->setDefaultValue(id.getIdentifier(), dv);
				}

				if (auto tcd = as<ComplexTypeDefinition>(s))
				{
					if(tcd->initValues != nullptr)
						t->setDefaultValue(id.getIdentifier(), tcd->initValues);
				}
			}
		}
	}

	t->finaliseAlignment();
}





Operations::TemplateParameterResolver::TemplateParameterResolver(const TemplateParameter::List& tp_) :
	tp(tp_)
{
	for (const auto& p : tp)
	{
		jassert(p.t != TemplateParameter::Empty);
		jassert(!p.isTemplateArgument());
		//jassert(p.argumentId.isValid());

		if (p.t == TemplateParameter::Type)
			jassert(p.type.isValid());
		else
			jassert(p.type.isInvalid());
	}
}

juce::Result Operations::TemplateParameterResolver::process(Statement::Ptr p)
{
	Result r = Result::ok();

	if (auto f = as<Function>(p))
	{
		r = processType(f->data.returnType);

		if (f->data.returnType.isTemplateType())
		{
			r = Result::fail("Can't resolve template return type " + f->data.returnType.getTemplateId().toString());
		}

		if (!r.wasOk())
			return r;

		for (auto& a : f->data.args)
		{
			r = processType(a.typeInfo);

			if (!r.wasOk())
				return r;
		}

		// The statement is not a "real child" so we have to 
		// call it manually...
		if (r.wasOk() && f->statements != nullptr)
			r = process(f->statements);

		if (!r.wasOk())
			return r;
	}
	if (auto fc = as<FunctionCall>(p))
	{
		auto& function = fc->function;

		auto r = process(function);

		if (r.failed())
			return r;
	}
	if (auto v = as<VariableReference>(p))
	{
		r = processType(v->id.typeInfo);

		if (!r.wasOk())
			return r;

		for (auto& p : tp)
		{
			if (p.argumentId == v->id.id)
			{
				jassert(p.t == TemplateParameter::ConstantInteger);

				auto value = VariableStorage(p.constant);
				auto imm = new Immediate(v->location, value);

				v->replaceInParent(imm);
			}
		}
	}
	if (auto cd = as<ComplexTypeDefinition>(p))
	{
		auto r = processType(cd->type);

		if (!r.wasOk())
			return r;

		if (!cd->type.isComplexType())
		{
			VariableStorage zero(cd->type.getType(), 0);

			for (auto s : cd->getSymbols())
			{
				auto v = new Operations::VariableReference(cd->location, s);
				auto imm = new Immediate(cd->location, zero);
				auto a = new Assignment(cd->location, v, JitTokens::assign_, imm, true);

				cd->replaceInParent(a);

			}

			return r;
		}
	}

	for (auto c : *p)
	{
		r = process(c);

		if (!r.wasOk())
			return r;
	}

	return r;
}

juce::Result Operations::TemplateParameterResolver::process(FunctionData& f) const
{
	auto r = resolveIds(f);

	if (r.failed())
		return r;

	r = processType(f.returnType);

	if (r.failed())
		return r;

	for (auto& a : f.args)
	{
		r = processType(a.typeInfo);

		if (r.failed())
			return r;
	}

	return r;
}

juce::Result Operations::TemplateParameterResolver::processType(TypeInfo& t) const
{
	if (auto tct = t.getTypedIfComplexType<TemplatedComplexType>())
	{
		auto r = Result::ok();
		auto newType = tct->createTemplatedInstance(tp, r);

		if (!r.wasOk())
			return r;

		auto nt = TypeInfo(newType, t.isConst(), t.isRef());
		t = nt;

		return r;
	}

	if (!t.isTemplateType())
		return Result::ok();

	for (const auto& p : tp)
	{
		if (p.argumentId == t.getTemplateId())
		{
			t = p.type.withModifiers(t.isConst(), t.isRef());
			jassert(!t.isTemplateType());
			jassert(!t.isDynamic());
			return Result::ok();
		}
	}

	return Result::fail("Can't resolve template type " + t.toString());
}


juce::Result Operations::TemplateParameterResolver::resolveIds(FunctionData& d) const
{
	auto r = resolveIdForType(d.returnType);

	if (r.failed())
		return r;

	for (auto& a : d.args)
	{
		r = resolveIdForType(a.typeInfo);

		if (r.failed())
			return r;
	}

	return Result::ok();
}

juce::Result Operations::TemplateParameterResolver::resolveIdForType(TypeInfo& t) const
{
	if (t.isTemplateType())
	{
		for (const auto& p : tp)
		{
			auto rId = t.getTemplateId();

			if (rId.getIdentifier() == p.argumentId.getIdentifier())
			{
				jassert(rId.isExplicit() || rId == p.argumentId);

				auto nt = TypeInfo(p.argumentId);
				nt = nt.withModifiers(t.isConst(), t.isRef());
				t = nt;
			}
		}
	}

	return Result::ok();
}

void Operations::StatementWithControlFlowEffectBase::addDestructorToAllChildStatements(Statement::Ptr root, const Symbol& id)
{
	Statement::List breakStatements;

	auto rootTyped = as<ScopeStatementBase>(root);

	jassert(rootTyped != nullptr);

	bool symbolWasDefined = false;

	root->forEachRecursive([&](Statement::Ptr p)
	{
		if (auto cd = as<ComplexTypeDefinition>(p))
		{
			if (cd->getInstanceIds().contains(id.id))
			{
				symbolWasDefined = true;
				return false;
			}
		}

		if (!symbolWasDefined)
			return false;

		if (auto swcfweb = as<StatementWithControlFlowEffectBase>(p))
		{
			if (swcfweb->shouldAddDestructor(rootTyped, id))
				breakStatements.add(p);
		}

		return false;
	}, IterationType::NoChildInlineFunctionBlocks);

	bool hasReturn = false; 

	for (auto s : *root)
	{
		if (as<ReturnStatement>(s) != nullptr)
			hasReturn = true;
	}

	if(!hasReturn)
		breakStatements.add(root);

	for (auto bs : breakStatements)
	{
		ComplexType::InitData d;
		ScopedPointer<SyntaxTreeInlineData> b = new SyntaxTreeInlineData(bs, rootTyped->getPath(), {});
		d.t = ComplexType::InitData::Type::Desctructor;
		d.functionTree = b.get();
		b->object = bs;

		if (id.id.getIdentifier() == Identifier("this"))
			b->expression = new Operations::ThisPointer(bs->location, id.typeInfo);
		else
			b->expression = new Operations::VariableReference(bs->location, id);

		auto r = id.typeInfo.getComplexType()->callDestructor(d);
		bs->location.test(r);
	}
		
}

}
}
