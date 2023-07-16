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



Operations::Assignment::Assignment(Location l, Expression::Ptr target, TokenType assignmentType_, Expression::Ptr expr, bool firstAssignment_) :
	Expression(l),
	assignmentType(assignmentType_),
	isFirstAssignment(firstAssignment_)
{
	addStatement(expr);
	addStatement(target); // the target must be evaluated after the expression
}

void Operations::Assignment::process(BaseCompiler* compiler, BaseScope* scope)
{
	/** Assignments might use the target register OR have the same symbol from another scope
		so we need to customize the execution order in these passes... */
	if (compiler->getCurrentPass() == BaseCompiler::CodeGeneration ||
		compiler->getCurrentPass() == BaseCompiler::DataAllocation)
		processBaseWithoutChildren(compiler, scope);
	else
		processBaseWithChildren(compiler, scope);

	auto e = getSubExpr(0);

	COMPILER_PASS(BaseCompiler::ComplexTypeParsing)
	{
		if (getTargetType() == TargetType::Variable && isFirstAssignment && scope->getRootClassScope() == scope)
		{
			auto typeToAllocate = getTargetVariable()->getTypeInfo();

			if (typeToAllocate.isInvalid())
			{
				typeToAllocate = getSubExpr(0)->getTypeInfo();

				if (typeToAllocate.isInvalid())
					location.throwError("Can't deduce type");

				getTargetSymbolStatement()->getSymbol().typeInfo = typeToAllocate;
			}

			scope->getRootData()->enlargeAllocatedSize(getTargetVariable()->getTypeInfo());
		}
	}

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		getSubExpr(0)->process(compiler, scope);
		auto targetType = getTargetType();

		if ((targetType == TargetType::Variable || targetType == TargetType::Reference) && isFirstAssignment)
		{
			auto type = getTargetVariable()->getType();

			if (!Types::Helpers::isFixedType(type))
			{
				getSubExpr(0)->tryToResolveType(compiler);

				type = getSubExpr(0)->getType();

				if (!Types::Helpers::isFixedType(type))
				{
					BaseCompiler::ScopedPassSwitcher rs(compiler, BaseCompiler::ResolvingSymbols);
					getSubExpr(0)->process(compiler, scope);

					BaseCompiler::ScopedPassSwitcher tc(compiler, BaseCompiler::TypeCheck);
					getSubExpr(0)->process(compiler, scope);

					type = getSubExpr(0)->getType();

					if (!Types::Helpers::isFixedType(type))
						location.throwError("Can't deduce auto type");
				}

				getTargetSymbolStatement()->getSymbol().typeInfo.setType(type);
			}

			as<VariableReference>(getTargetVariable())->isLocalDefinition = true;

			if (scope->getRootClassScope() == scope)
				scope->getRootData()->allocate(scope, getTargetSymbolStatement()->getSymbol());
		}

		getSubExpr(1)->process(compiler, scope);
	}

	COMPILER_PASS(BaseCompiler::DataInitialisation)
	{
		if (isFirstAssignment)
			initClassMembers(compiler, scope);
	}

	COMPILER_PASS(BaseCompiler::ResolvingSymbols)
	{
		switch (getTargetType())
		{
		case TargetType::Variable:
		{
			auto e = getSubExpr(1);
			auto v = getTargetSymbolStatement();
			
			jassert(v != nullptr);

			if (v->getSymbol().isConst() && !isFirstAssignment)
				throwError("Can't change constant variable");
		}
		case TargetType::Reference:
		case TargetType::ClassMember:
		case TargetType::Span:
			break;
		}
	}


	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (auto dot = dynamic_cast<DotOperator*>(getSubExpr(1).get()))
		{
			jassert(getTargetType() == TargetType::ClassMember);

			if (dot->getSubExpr(0)->getTypeInfo().isConst())
				location.throwError("Can't modify const object");
		}

		if (getTargetType() == TargetType::Reference && isFirstAssignment)
		{
			if(!canBeReferenced(getSubExpr(0)))
			{
				location.throwError("Can't assign non-reference");
			}
		}
		

		auto targetType = getSubExpr(1)->getTypeInfo();
		auto assignedType = getSubExpr(0)->getTypeInfo();

		if (targetType.isDynamic() && assignedType.isValid())
		{
			setTypeForChild(1, assignedType);
		}

		auto targetIsSimd = SpanType::isSimdType(getSubExpr(1)->getTypeInfo());

		if (targetIsSimd)
		{
			auto valueIsSimd = SpanType::isSimdType(getSubExpr(0)->getTypeInfo());

			if (!valueIsSimd)
				setTypeForChild(0, TypeInfo(Types::ID::Float));
		}
		else
		{
			List l;
			l.add(getSubExpr(0));

			if (replaceIfOverloaded(getSubExpr(1), l, FunctionClass::AssignOverload))
				return;

			checkAndSetType(0, getSubExpr(1)->getTypeInfo());
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		getSubExpr(0)->process(compiler, scope);
		getSubExpr(1)->process(compiler, scope);

		auto targetExpression = getSubExpr(1);
		auto valueExpression = getSubExpr(0);

		auto targetType = getTargetType();

		ignoreUnused(targetType);

		auto value = getSubRegister(0);
		auto tReg = getSubRegister(1);

		if (value->isSameRegisterSource(tReg))
		{
			// this prevents loading a nil value...
			return;
		}

		auto acg = CREATE_ASM_COMPILER(tReg->getType());

        auto targetIsFunction = as<FunctionCall>(getSubExpr(1));
        
		if (overloadedAssignOperator.isResolved())
		{
			AssemblyRegister::List l;
			l.add(tReg);
			l.add(value);

			auto r = acg.emitFunctionCall(tReg, overloadedAssignOperator, tReg, l);

			if (!r.wasOk())
				location.throwError(r.getErrorMessage());
		}

		else if (auto dt = getSubExpr(1)->getTypeInfo().getTypedIfComplexType<DynType>())
		{
			acg.emitStackInitialisation(tReg, dt, value, nullptr);
		}
        else if(targetIsFunction)
        {
            acg.writeRegisterToMemory(tReg, value);
        }
		else if (getTargetType() == TargetType::Reference && isFirstAssignment)
		{
            tReg->setReferToReg(value);
		}
		else
		{
			if (assignmentType == JitTokens::assign_)
			{
				if (tReg->getTypeInfo().isComplexType())
				{
					acg.emitStackInitialisation(tReg, tReg->getTypeInfo().getComplexType(), value, nullptr);
				}
				else
				{
					if (tReg != value)
					{
						if (tReg->isGlobalMemory() || tReg->hasCustomMemoryLocation())
							tReg->setWriteBackToMemory(true);

						acg.emitStore(tReg, value);
					}
				}
			}
			else
				acg.emitBinaryOp(assignmentType, tReg, value);
		}
	}
#endif
}

void Operations::Assignment::initClassMembers(BaseCompiler* compiler, BaseScope* scope)
{
	if (getSubExpr(0)->isConstExpr() && scope->getScopeType() == BaseScope::Class)
	{
		auto target = getTargetSymbolStatement()->getSymbol();
		auto initValue = getSubExpr(0)->getConstExprValue();

		if (auto st = dynamic_cast<StructType*>(dynamic_cast<ClassScope*>(scope)->typePtr.get()))
		{
			auto ok = st->setDefaultValue(target.id.getIdentifier(), InitialiserList::makeSingleList(initValue));

			if (!ok)
				throwError("Can't initialise default value");
		}
		else
		{
			// this will initialise the class members to constant values...
			auto rd = scope->getRootClassScope()->rootData.get();

			auto ok = rd->initData(scope, target, InitialiserList::makeSingleList(initValue));

			if (!ok.wasOk())
				location.throwError(ok.getErrorMessage());
		}
	}
}

Operations::Assignment::TargetType Operations::Assignment::getTargetType() const
{
	auto target = getSubExpr(1);

	// an auto statement will add a cast to the target
	if (as<Cast>(target) != nullptr)
		target = target->getSubExpr(0);

	if (auto v = dynamic_cast<SymbolStatement*>(target.get()))
	{
		return v->getSymbol().isReference() ? TargetType::Reference : TargetType::Variable;
	}
	else if (dynamic_cast<DotOperator*>(target.get()))
		return TargetType::ClassMember;
	else if (dynamic_cast<Subscript*>(target.get()))
		return TargetType::Span;
	else if (dynamic_cast<MemoryReference*>(target.get()))
		return TargetType::Reference;
	else if (dynamic_cast<StatementBlock*>(target.get()))
	{
		auto t = target->getTypeInfo();

		if(!t.isRef())
			target->throwError("Can't assign to target");

		return TargetType::Reference;
	}
    else if (dynamic_cast<FunctionCall*>(target.get()))
    {
        auto t = target->getTypeInfo();

        if(!t.isRef())
            target->throwError("Can't assign to target");

        return TargetType::Reference;
    }

	getSubExpr(1)->throwError("Can't assign to target");

	jassertfalse;
	return TargetType::numTargetTypes;
}


void Operations::Cast::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto sourceType = getSubExpr(0)->getTypeInfo();
		auto targetType = getTypeInfo();

		if (replaceIfOverloaded(getSubExpr(0), {}, FunctionClass::NativeTypeCast))
			return;

		if (sourceType.isComplexType())
			location.throwError("Can't cast " + sourceType.toString() + " to " + targetType.toString());

		if (sourceType == targetType)
		{
			replaceInParent(getSubExpr(0));
			return;
		}

	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto sourceType = getSubExpr(0)->getTypeInfo();

		if (sourceType.isComplexType())
		{
			if (compiler->getRegisterType(sourceType) == getType())
			{
				reg = getSubRegister(0);
				return;
			}

			FunctionClass::Ptr fc = sourceType.getComplexType()->getFunctionClass();
			complexCastFunction = fc->getSpecialFunction(FunctionClass::NativeTypeCast, targetType, {});
		}

		auto asg = CREATE_ASM_COMPILER(getType());
		reg = compiler->getRegFromPool(scope, getTypeInfo());

		if (complexCastFunction.isResolved())
		{
			AssemblyRegister::List l;
			auto r = asg.emitFunctionCall(reg, complexCastFunction, getSubRegister(0), l);

			if (!r.wasOk())
			{
				location.throwError(r.getErrorMessage());
			}
		}
		else
		{
			if (getSubRegister(0) == nullptr)
				location.throwError("Source register not found");

			auto sourceType = getSubExpr(0)->getType();
			
			
			asg.emitCast(reg, getSubRegister(0), sourceType);
		}
	}
#endif
}

void Operations::BinaryOp::process(BaseCompiler* compiler, BaseScope* scope)
{
	// Defer evaluation of the children for operators with short circuiting...
	bool processChildren = !(isLogicOp() && (compiler->getCurrentPass() == BaseCompiler::CodeGeneration));

	if (processChildren)
		processBaseWithChildren(compiler, scope);
	else
		processBaseWithoutChildren(compiler, scope);

	if (isLogicOp() && getSubExpr(0)->isConstExpr())
	{
		bool isOr1 = op == JitTokens::logicalOr && getSubExpr(0)->getConstExprValue().toInt() == 1;
		bool isAnd0 = op == JitTokens::logicalAnd && getSubExpr(0)->getConstExprValue().toInt() == 0;

		if (isOr1 || isAnd0)
		{
			replaceInParent(getSubExpr(0));
			return;
		}
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (op == JitTokens::logicalAnd ||
			op == JitTokens::logicalOr)
		{
			checkAndSetType(0, TypeInfo(Types::ID::Integer));
			return;
		}

		if (auto atb = getSubExpr(0)->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>())
		{
			if (atb->getElementType().getType() == Types::ID::Float)
			{
				LoopOptimiser::replaceWithVectorLoop(compiler, scope, this);
				jassertfalse;
			}
		}

		checkAndSetType(0, {});
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto asg = CREATE_ASM_COMPILER(getType());

		if (isLogicOp())
		{
			asg.emitLogicOp(this);
		}
		else
		{
			auto l = getSubRegister(0);

			if (reg == nullptr)
			{
				reg = compiler->getRegFromPool(scope, getTypeInfo());

			}

			asg.emitStore(reg, getSubRegister(0));

			auto le = getSubExpr(0);
			auto re = getSubExpr(1);

			auto r = getSubRegister(1);

			if (r == nullptr)
				getSubExpr(1)->throwError("Can't find register for right operand");

			asg.emitBinaryOp(op, reg, r);

			VariableReference::reuseAllLastReferences(getChildStatement(0));
			VariableReference::reuseAllLastReferences(getChildStatement(1));
		}
	}
#endif
}

#if SNEX_ASMJIT_BACKEND
struct Operations::VectorOp::SerialisedVectorOp : public ReferenceCountedObject
{
	enum class Type
	{
		ImmediateScalar,
		ScalarVariable,
		VectorVariable,
		VectorOpType,
		VectorFunction,
		TargetVector,
		Simd4Value,
		Simd4Op,
		Simd4Target,
		numReaderTypes
	};

	using Ptr = ReferenceCountedObjectPtr<SerialisedVectorOp>;
	using List = ReferenceCountedArray<SerialisedVectorOp>;

	SerialisedVectorOp(Statement::Ptr s, x86::Compiler& cc)
	{
		auto fc = as<FunctionCall>(s);
		String id = fc != nullptr ? fc->function.id.getIdentifier().toString() : "";

		if (getFunctionSignatureId(id, false) != 0)
		{
			readerType = Type::VectorFunction;
			opType = id.getCharPointer();
			dbgString << "func " << fc->function.getSignature();

            if (String(opType) == "abs")
            {
                auto cv = Data128::fromU32(0x7fffffff);
                immConst = cc.newConst(ConstPoolScope::kGlobal, cv.getData(), cv.size());
            }
				
		}
		
		if (s->getType() == Types::ID::Float)
		{
			if (s->isConstExpr())
			{
				readerType = Type::ImmediateScalar;
				auto immValue = s->getConstExprValue().toFloat();
				dbgString << "imm " << String(immValue);

                auto cv = Data128::fromF32(immValue);
                
                immConst = cc.newConst(ConstPoolScope::kGlobal, cv.getData(), cv.size());
			}
			else
			{
				readerType = Type::ScalarVariable;

				dbgString << "scalar " << s->toString(Statement::TextFormat::SyntaxTree);

				s->reg->loadMemoryIntoRegister(cc);
				dataReg = s->reg->getRegisterForReadOp().as<X86Xmm>();
				cc.shufps(dataReg, dataReg, 0);

				jassert(s->reg->isActive());
			}
		}
		else
		{
			auto sType = s->getTypeInfo();
			auto isSimd4 = SpanType::isSimdType(sType);

			// Must be a span or dyn
			jassert(sType.getTypedIfComplexType<ArrayTypeBase>() != nullptr);

			if (auto vop = as<VectorOp>(s))
			{
				opType = vop->opType;

				if (isSimd4)
				{
					readerType = vop->isChildOp ? Type::Simd4Op : Type::Simd4Target;
					dbgString << "f4op " << opType;

					addressReg = cc.newInt64();
					cc.mov(addressReg, 3);

				}
				else
				{
					readerType = vop->isChildOp ? Type::VectorOpType : Type::TargetVector;
					dbgString << "vop " << opType;
				}
			}
			else
			{
				if (isSimd4)
				{
					readerType = Type::Simd4Value;
					s->reg->loadMemoryIntoRegister(cc);
					dataReg = s->reg->getRegisterForReadOp().as<x86::Xmm>();
					dbgString << "float4 " << s->toString(Statement::TextFormat::CppCode);
					return;
				}
				else
				{
					readerType = Type::VectorVariable;
					dbgString << "vec " << s->toString(Statement::TextFormat::CppCode);
				}
			}

			if (isSimd4)
			{
				addressReg = cc.newGpq();

				auto reg = getRegPtr(s);

				jassert(reg->hasCustomMemoryLocation() || reg->isGlobalMemory());

				cc.lea(addressReg, reg->getMemoryLocationForReference());
			}
			else
			{
				cc.setInlineComment("Set Address");
				addressReg = cc.newGpq();

				X86Mem fatPointerAddress;

				auto reg = getRegPtr(s);

				if (reg->hasCustomMemoryLocation() || reg->isMemoryLocation())
					fatPointerAddress = reg->getMemoryLocationForReference();
				else
					fatPointerAddress = x86::ptr(PTR_REG_R(reg));


				if (auto spanType = sType.getTypedIfComplexType<SpanType>())
				{
					cc.lea(addressReg, fatPointerAddress);
					// make a better version if desired...
					sizeMem = cc.newInt32Const(ConstPoolScope::kLocal, spanType->getNumElements());
				}
				else
				{
					sizeMem = fatPointerAddress.cloneAdjustedAndResized(4, 4);
					auto objectAddress = fatPointerAddress.cloneAdjustedAndResized(8, 8);
					cc.mov(addressReg, objectAddress);
				}
			}
		}

		bool processChildOps = true;

		if (as<StatementBlock>(s) != nullptr)
			processChildOps = false;

		if (fc != nullptr && !fc->isVectorOpFunction())
			processChildOps = false;

		// don't process child statements of inlined functions as vector op...
		if (processChildOps)
		{
			for (auto c : *s)
				childOps.add(new SerialisedVectorOp(c, cc));
		}
	}

	RegPtr getAddressReg(Statement::Ptr p)
	{
		if(readerType != Type::Simd4Op)
			return getRegPtr(p);

		return childOps[0]->getAddressReg(p);
	}

	bool isOp() const
	{
		return readerType == Type::TargetVector || readerType == Type::VectorOpType || readerType == Type::VectorFunction || readerType == Type::Simd4Target || readerType == Type::Simd4Op;
	}

	static AssemblyRegister::Ptr getRegPtr(Statement::Ptr p)
	{
		if (auto vOp = as<VectorOp>(p))
		{
			if (vOp->isSimd4)
				return getRegPtr(vOp->getSubExpr(1));
		}

		if (p->reg != nullptr)
			return p->reg;

		if (auto fc = as<FunctionCall>(p))
		{
			if (getFunctionSignatureId(fc->function.id.getIdentifier().toString(), false) != 0)
				return fc->getSubRegister(0);
			else
				return fc->reg;
		}

		return nullptr;
	}

	bool isFunction() const
	{
		return readerType == Type::VectorFunction;
	}

	bool isVectorType() const
	{
		return readerType == Type::TargetVector ||
			readerType == Type::VectorVariable ||
			readerType == Type::VectorOpType ||
			readerType == Type::Simd4Target ||
			readerType == Type::Simd4Op ||
			readerType == Type::Simd4Value;
	}

	void checkAlignment(x86::Compiler& cc, x86::Gp& resultReg)
	{
		if (readerType == Type::VectorVariable)
		{
			cc.test(addressReg, 0xF);
			cc.cmovnz(resultReg, addressReg.r32());
		}

		for (auto c : childOps)
			c->checkAlignment(cc, resultReg);
	}

	void safeCheckPtrs(x86::Compiler& cc, const asmjit::Label& loopEnd)
	{
		if (isVectorType())
		{
			cc.cmp(addressReg, 0);
			cc.jz(loopEnd);
		}

		for (auto c : childOps)
			c->safeCheckPtrs(cc, loopEnd);
	}

	void process(x86::Compiler& cc, bool isSimd)
	{
		for (auto c : childOps)
			c->process(cc, isSimd);

		if (isVectorType() || isFunction())
		{
			if (readerType == Type::VectorVariable)
			{
				if (isSimd)
				{
					dataReg = cc.newXmmPs();
					cc.setInlineComment("Load data from address");
					cc.movaps(dataReg, x86::ptr(addressReg));
				}
				else
				{
					dataReg = cc.newXmm();
					cc.setInlineComment("Load data from address");
					cc.movss(dataReg, x86::ptr(addressReg));
				}
			}

			if (isOp())
			{
				emitOp(cc, isSimd);

				if (!isFunction())
				{
					if (isSimd)
						cc.movaps(x86::ptr(addressReg), getDataRegToUse());
					else
						cc.movss(x86::ptr(addressReg), getDataRegToUse());
				}
			}
		}
	}

	void incAdress(x86::Compiler& cc, bool isSimd)
	{
		if (isVectorType())
        {
            int offset = isSimd ? 4 * sizeof(float) : sizeof(float);
			cc.add(addressReg, offset);
        }

		for (auto c : childOps)
			c->incAdress(cc, isSimd);
	}

	String toString(int& intendation) const
	{
		String text;
		String tabs;
		intendation += 2;

		for (int i = 0; i < intendation; i++)
			tabs << " ";

		text << tabs << dbgString << "\n";

		for (auto c : childOps)
			text << c->toString(intendation);

		intendation -= 2;
		return text;
	}

	X86Mem getSizeMem() const
	{
		jassert(isVectorType());
		return sizeMem;
	}

private:

	x86::Xmm getDataRegToUse() const
	{
		if (readerType == Type::VectorFunction)
		{
			if (getFunctionSignatureId(opType, false))
				return childOps[0]->getDataRegToUse();
			else
				jassertfalse;
		}

		else if (isOp())
			return childOps[1]->getDataRegToUse();

		jassert(dataReg.isValid());
		jassert(dataReg.isPhysReg() || dataReg.isVirtReg());

		return dataReg;
	}

	

	void emitOp(x86::Compiler& cc, bool isSimd)
	{
		auto instId = getFunctionSignatureId(opType, isSimd);

		if (instId != 0)
		{
			auto r = childOps[0];
			jassert(r != nullptr);
			jassert(getDataRegToUse().isValid());



			if (r->readerType == Type::ImmediateScalar)
				cc.emit(instId, getDataRegToUse(), isSimd ? r->immConst : r->immConst.cloneResized(4));
			else if (readerType == Type::VectorFunction)
			{
				jassert(r->getDataRegToUse().isValid());
				bool isAbs = String(opType) == "abs";
				r = isAbs ? this : childOps[1].get();

				if (r->readerType == Type::ImmediateScalar || isAbs)
					cc.emit(instId, getDataRegToUse(), isSimd ? r->immConst : r->immConst.cloneResized(4));
				else
					cc.emit(instId, getDataRegToUse(), r->getDataRegToUse());
			}
			else
			{
				jassert(r->getDataRegToUse().isValid());
				cc.emit(instId, getDataRegToUse(), r->getDataRegToUse());
			}
		}
		else
		{
			jassertfalse;
		}

		
	}

	Type readerType;
	TokenType opType = JitTokens::void_;
	String dbgString;

	x86::Mem immConst;
	x86::Gp  addressReg;
	x86::Mem sizeMem;
	x86::Xmm dataReg;

	List childOps;
};
#endif

void Operations::VectorOp::initChildOps()
{
	if (!isChildOp)
	{
		forEachRecursive([this](Ptr p)
		{
			if (this->isChildOp)
				return true;

			if (this == p.get())
				return false;

			if (auto pt = as<VectorOp>(p))
				pt->isChildOp = true;

			return false;
		}, IterationType::NoChildInlineFunctionBlocks);
	}
}

void Operations::VectorOp::process(BaseCompiler* compiler, BaseScope* scope)
{
	initChildOps();

	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (!BlockParser::isVectorOp(opType, getSubExpr(0)))
			setTypeForChild(0, TypeInfo(Types::ID::Float));
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		// Forward the registers for all sub vector ops...
		reg = getSubRegister(1);

		if (auto fChild = as<FunctionCall>(getSubExpr(1)))
		{
			if (fChild->isVectorOpFunction())
			{
				// Pass the first argument as target vector...
				reg = fChild->getSubRegister(0);

				jassert(reg != nullptr);
				jassert(reg->getTypeInfo().getTypedIfComplexType<ArrayTypeBase>() != nullptr);
			}
		}

		jassert(reg != nullptr);

		if (!isChildOp)
			emitVectorOp(compiler, scope);
	}
#endif
}

void Operations::VectorOp::emitVectorOp(BaseCompiler* compiler, BaseScope* scope)
{
#if SNEX_ASMJIT_BACKEND
	auto& cc = getFunctionCompiler(compiler);

	if (isSimd4)
	{
		/* TODO:
		
			- make proper "dirty register" stuff
			- check references as function parameters `clearFloat(float& d)`
			- fix "left op must be vector for simd4" (check against C++)

		*/
		jassertfalse;

		SerialisedVectorOp::Ptr root = new SerialisedVectorOp(this, cc);
		root->process(cc, true);

		return;
	}
	else
	{
		SerialisedVectorOp::Ptr root = new SerialisedVectorOp(this, cc);

		int l = 0;
		DBG(root->toString(l));
		ignoreUnused(l);
			
		auto sizeReg = cc.newGpd();

		auto simdLoop = cc.newLabel();
		auto loopEnd = cc.newLabel();
		auto leftOverLoop = cc.newLabel();

		if (!scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::NoSafeChecks))
		{
			root->safeCheckPtrs(cc, loopEnd);
		}

		if (scope->getGlobalScope()->getOptimizationPassList().contains(OptimizationIds::AutoVectorisation))
		{
			static constexpr int SimdSize = 4;

			cc.xor_(sizeReg, sizeReg);
			root->checkAlignment(cc, sizeReg);

			cc.cmp(sizeReg, 0);

			// Now we can use the sizeReg as actual size...
			cc.mov(sizeReg, root->getSizeMem());

			cc.jne(leftOverLoop);

			cc.setInlineComment("Skip the SIMD loop if i < 4");
			cc.cmp(sizeReg, SimdSize);
			cc.jb(leftOverLoop);

			cc.bind(simdLoop);

			root->process(cc, true);
			root->incAdress(cc, true);

			cc.sub(sizeReg, SimdSize);
			cc.cmp(sizeReg, SimdSize);
			cc.jae(simdLoop);
		}
		else
		{
			// We used the sizeReg as alignment cache register
			// to avoid spilling...
			cc.mov(sizeReg, root->getSizeMem());
		}

		cc.bind(leftOverLoop);
		cc.test(sizeReg, sizeReg);
		cc.jz(loopEnd);

		root->process(cc, false);
		root->incAdress(cc, false);

		cc.dec(sizeReg);
		cc.jmp(leftOverLoop);
		cc.bind(loopEnd);
	}
#endif
}

juce::uint32 Operations::VectorOp::getFunctionSignatureId(const String& functionName, bool isSimd)
{
#if SNEX_ASMJIT_BACKEND
	HashMap<String, std::array<uint32, 2>> opMap;

	using namespace asmjit::x86;

	opMap.set(JitTokens::plus, { Inst::kIdAddss, Inst::kIdAddps });
	opMap.set(JitTokens::assign_, { Inst::kIdMovss, Inst::kIdMovaps });
	opMap.set(JitTokens::times, { Inst::kIdMulss, Inst::kIdMulps });
	opMap.set(JitTokens::minus, { Inst::kIdSubss, Inst::kIdSubps });
	opMap.set("min", { Inst::kIdMinss, Inst::kIdMinps });
	opMap.set("max", { Inst::kIdMaxss, Inst::kIdMaxps });
	opMap.set("abs", { Inst::kIdAndps,  Inst::kIdAndps });

	return opMap[functionName][(int)isSimd];
#else
	jassertfalse;
	return 0;
#endif
}

void Operations::Increment::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		tryToResolveType(compiler);

		auto s = getOperatorId();

		if (replaceIfOverloaded(getSubExpr(0), {}, s))
			return;

		if (dynamic_cast<Increment*>(getSubExpr(0).get()) != nullptr)
			throwError("Can't combine incrementors");

		if (compiler->getRegisterType(getSubExpr(0)->getTypeInfo()) != Types::ID::Integer)
			throwError("Can't increment non integer variables.");
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto asg = CREATE_ASM_COMPILER(getType());

		RegPtr valueReg;
		RegPtr dataReg = getSubRegister(0);

		if (isPreInc)
			valueReg = dataReg;
		else
			valueReg = compiler->getRegFromPool(scope, TypeInfo(Types::ID::Integer));

		bool done = false;

		if (getTypeInfo().isComplexType())
		{
			FunctionClass::Ptr fc = getTypeInfo().getComplexType()->getFunctionClass();
			auto f = fc->getSpecialFunction(FunctionClass::IncOverload, getTypeInfo(), { TypeInfo(Types::ID::Integer, false, true) });

			if (f.canBeInlined(false))
			{
				getOrSetIncProperties(f.templateParameters, isPreInc, isDecrement);
				AssemblyRegister::List l;

				l.add(valueReg);
				asg.emitFunctionCall(dataReg, f, nullptr, l);
				done = true;
			}
		}

		if (!done)
			asg.emitIncrement(valueReg, dataReg, isPreInc, isDecrement);

		if (isPreInc)
			reg = dataReg;
		else
			reg = valueReg;

		jassert(reg != nullptr);
	}
#endif
}

void Operations::Increment::getOrSetIncProperties(Array<TemplateParameter>& tp, bool& isPre, bool& isDec)
{
	if (tp.isEmpty())
	{
		TemplateParameter d;
		d.constant = (int)isDec;
		TemplateParameter p;
		p.constant = (int)isPre;

		tp.add(d);
		tp.add(p);
	}
	else
	{
		isDec = tp[0].constant;
		isPre = tp[1].constant;
	}
}

bool Operations::Increment::tryToResolveType(BaseCompiler* c)
{
	if (resolvedType.isValid())
		return true;

	auto s = getSubExpr(0);

	if (s->tryToResolveType(c))
	{
		resolvedType = s->getTypeInfo();

		if (resolvedType.isComplexType())
		{
			// The operator overload might return a reference
			// so we need to make sure the type has the correct
			// ref specifier

			FunctionClass::Ptr fc = resolvedType.getComplexType()->getFunctionClass();

			auto overloadedFunction = fc->getSpecialFunction(getOperatorId());

			if (!overloadedFunction.isValid())
				location.throwError(fc->getClassName().getChildId(fc->getSpecialSymbol({}, getOperatorId())).toString() + " not found");

			resolvedType = overloadedFunction.returnType;
		}

		return true;
	}

	return false;
}

snex::jit::FunctionClass::SpecialSymbols Operations::Increment::getOperatorId() const
{
	if (isPreInc && isDecrement)
		return FunctionClass::SpecialSymbols::DecOverload;
	if (isPreInc && !isDecrement)
		return FunctionClass::SpecialSymbols::IncOverload;
	if (!isPreInc && isDecrement)
		return FunctionClass::SpecialSymbols::PostDecOverload;
	if (!isPreInc && !isDecrement)
		return FunctionClass::SpecialSymbols::PostIncOverload;

	jassertfalse;
	return FunctionClass::numOperatorOverloads;
}

bool Operations::DotOperator::tryToResolveType(BaseCompiler* compiler)
{
	if (Statement::tryToResolveType(compiler))
		return true;

	if (getDotChild()->getTypeInfo().isInvalid())
	{
		if (auto st = getDotParent()->getTypeInfo().getTypedIfComplexType<StructType>())
		{
			if (auto ss = dynamic_cast<Operations::SymbolStatement*>(getDotChild().get()))
			{
				auto id = ss->getSymbol().getName();

				if (st->hasMember(id))
				{
					auto fullId = st->id.getChildId(id);

					location.test(compiler->namespaceHandler.checkVisiblity(fullId));

					resolvedType = st->getMemberTypeInfo(id);
					return true;
				}
			}
		}
	}

	return false;
}

bool Operations::Subscript::tryToResolveType(BaseCompiler* compiler)
{
	if (Statement::tryToResolveType(compiler) && subscriptType != ArrayStatementBase::Undefined)
		return true;

	auto parentType = getSubExpr(0)->getTypeInfo();

	if (auto td = parentType.getTypedIfComplexType<TemplatedComplexType>())
	{
		jassertfalse;
	}
	else if (auto ts = parentType.getTypedIfComplexType<SpanType>())
	{
		spanType = ts;
		subscriptType = Span;
		elementType = spanType->getElementType().withModifiers(parentType.isConst(), true);
		return true;
	}
	else if (auto ds = parentType.getTypedIfComplexType<DynType>())
	{
		dynType = ds;
		subscriptType = Dyn;
		elementType = dynType->elementType.withModifiers(parentType.isConst(), true);
		return true;
	}
	else if (getSubExpr(0)->getType() == Types::ID::Block)
	{
		subscriptType = Dyn;
		elementType = TypeInfo(Types::ID::Float, false, true);
		return true;
	}
	else if (auto st = parentType.getTypedIfComplexType<StructType>())
	{
		FunctionClass::Ptr fc = st->getFunctionClass();

		if (fc->hasSpecialFunction(FunctionClass::Subscript))
		{
			subscriptOperator = fc->getSpecialFunction(FunctionClass::Subscript);
			subscriptType = CustomObject;
			elementType = subscriptOperator.returnType;
			return true;
		}
	}

	return false;
}

void Operations::Subscript::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::DataAllocation)
	{
		tryToResolveType(compiler);
	}

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		tryToResolveType(compiler);
		getSubExpr(1)->tryToResolveType(compiler);

		List args;
		args.add(getSubExpr(0));

		if(subscriptType != ArrayStatementBase::CustomObject)
		{
			BaseCompiler::ScopedUnsafeBoundChecker sb(compiler);

			if (replaceIfOverloaded(getSubExpr(1), args, FunctionClass::SpecialSymbols::GetFrom))
				return;
		}
		
		auto indexType = getSubExpr(1)->getTypeInfo();

		if (indexType.getType() == Types::ID::Float || indexType.getType() == Types::ID::Double)
		{
			getSubExpr(1)->throwError("subscript index must be integer type");
		}

		if (subscriptType != ArrayStatementBase::CustomObject && dynType == nullptr && !getSubExpr(1)->isConstExpr() && !compiler->allowUnsafeIndexes())
		{
			auto idx = getSubExpr(1);
			idx->logWarning("unsafe index access.");
		}

		if (spanType != nullptr)
		{
			auto size = spanType->getNumElements();

			if (getSubExpr(1)->isConstExpr())
			{
				int index = getSubExpr(1)->getConstExprValue().toInt();

				if (!isPositiveAndBelow(index, size))
					getSubExpr(1)->throwError("constant index out of bounds");
			}
		}
		else if (dynType != nullptr)
		{
			// nothing to do here...
			return;
		}
		else if (subscriptType == CustomObject)
		{
			List l;
			l.add(getSubExpr(1));

			BaseCompiler::ScopedUnsafeBoundChecker subc(compiler);

			if (replaceIfOverloaded(getSubExpr(0), l, FunctionClass::Subscript))
				return;

			// nothing to do here, the type check will be done in the function itself...
			return;
		}
		else
		{
			if (getSubExpr(0)->getType() == Types::ID::Block)
				elementType = TypeInfo(Types::ID::Float, false, true);
			else
				getSubExpr(1)->throwError("Can't use []-operator");
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (subscriptType == Span && compiler->fitsIntoNativeRegister(getSubExpr(0)->getTypeInfo().getComplexType().get()))
		{
			reg = getSubRegister(0);
			return;
		}

		reg = compiler->registerPool.getNextFreeRegister(scope, getTypeInfo());

		auto tReg = getSubRegister(0);
		auto acg = CREATE_ASM_COMPILER(compiler->getRegisterType(getTypeInfo()));

		if (!subscriptOperator.isResolved())
		{
			auto cType = getSubRegister(0)->getTypeInfo().getTypedIfComplexType<ComplexType>();

			if (cType != nullptr)
			{
				if (FunctionClass::Ptr fc = cType->getFunctionClass())
				{
					subscriptOperator = fc->getSpecialFunction(FunctionClass::Subscript, elementType, { getSubRegister(0)->getTypeInfo(), getSubRegister(1)->getTypeInfo() });
				}
			}
		}

		if (subscriptOperator.isResolved())
		{
			AssemblyRegister::List l;
			l.add(getSubRegister(0));
			l.add(getSubRegister(1));

			acg.location = getSubExpr(1)->location;

			auto r = acg.emitFunctionCall(reg, subscriptOperator, nullptr, l);

			if (!r.wasOk())
				location.throwError(r.getErrorMessage());

			return;
		}

		RegPtr indexReg;

		indexReg = getSubRegister(1);

		jassert(indexReg->isActive() || indexReg->isMemoryLocation());
		jassert(indexReg->getType() == Types::ID::Integer);

		acg.emitSpanReference(reg, getSubRegister(0), indexReg, elementType.getRequiredByteSize());

		replaceMemoryWithExistingReference(compiler);
	}
#endif
}

void Operations::Compare::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto l = getSubExpr(0);
		auto r = getSubExpr(1);

		if (l->getType() != r->getType())
		{
			Ptr implicitCast = new Operations::Cast(location, getSubExpr(1), l->getType());
			logWarning("Implicit cast to int for comparison");
			replaceChildStatement(1, implicitCast);
		}
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			auto l = getSubExpr(0);
			auto r = getSubExpr(1);

			reg = compiler->getRegFromPool(scope, getTypeInfo());

			auto tReg = getSubRegister(0);
			auto value = getSubRegister(1);

			asg.emitCompare(useAsmFlag, op, reg, l->reg, r->reg);

			VariableReference::reuseAllLastReferences(this);

		}
	}
#endif
}

void Operations::LogicalNot::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		if (getSubExpr(0)->getType() != Types::ID::Integer)
			throwError("Wrong type for logic operation");
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		auto asg = CREATE_ASM_COMPILER(getType());
		reg = asg.emitLogicalNot(getSubRegister(0));
	}
#endif
}

void Operations::PointerAccess::process(BaseCompiler* compiler, BaseScope* s)
{
	Statement::processBaseWithChildren(compiler, s);

	COMPILER_PASS(BaseCompiler::TypeCheck)
	{
		auto t = getTypeInfo();

		if (!t.isComplexType())
			throwError("Can't dereference non-complex type");
	}

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		reg = compiler->registerPool.getNextFreeRegister(s, getTypeInfo());

		auto acg = CREATE_ASM_COMPILER(Types::ID::Pointer);

		auto obj = getSubRegister(0);

		auto ptrReg = acg.cc.newGpq();

		if (auto thisPtr = as<ThisPointer>(getSubExpr(0)))
		{
			if (obj->isMemoryLocation())
				acg.cc.lea(ptrReg, obj->getAsMemoryLocation());
			else
				acg.cc.mov(ptrReg, PTR_REG_R(obj));
				
		}
		else
		{

			auto mem = obj->getMemoryLocationForReference();

			jassert(!mem.isNone());
			acg.cc.mov(ptrReg, mem);
		}

		reg->setCustomMemoryLocation(x86::ptr(ptrReg), obj->isGlobalMemory());
	}
#endif
}

void Operations::Negation::process(BaseCompiler* compiler, BaseScope* scope)
{
	processBaseWithChildren(compiler, scope);

#if SNEX_ASMJIT_BACKEND
	COMPILER_PASS(BaseCompiler::CodeGeneration)
	{
		if (!isConstExpr())
		{
			auto asg = CREATE_ASM_COMPILER(getType());

			reg = compiler->getRegFromPool(scope, getTypeInfo());

			asg.emitNegation(reg, getSubRegister(0));

		}
		else
		{
			// supposed to be optimized away by now...
			jassertfalse;
		}
	}
#endif
}

}
}
