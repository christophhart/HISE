namespace snex {
namespace mir {
using namespace juce;

namespace InstructionIds
{
	DEFINE_ID(SyntaxTree);
	DEFINE_ID(Function);
	DEFINE_ID(ReturnStatement);
	DEFINE_ID(BinaryOp);
	DEFINE_ID(VariableReference);
	DEFINE_ID(Immediate);
	DEFINE_ID(Cast);
	DEFINE_ID(Assignment);
	DEFINE_ID(Comparison);
	DEFINE_ID(TernaryOp);
	DEFINE_ID(IfStatement);
	DEFINE_ID(LogicalNot);
	DEFINE_ID(WhileLoop);
	DEFINE_ID(Increment);
	DEFINE_ID(StatementBlock);
	DEFINE_ID(ComplexTypeDefinition);
	DEFINE_ID(Subscript);
	DEFINE_ID(InternalProperty);
	DEFINE_ID(ControlFlowStatement);
	DEFINE_ID(Loop);
	DEFINE_ID(FunctionCall);
	DEFINE_ID(ClassStatement);
	DEFINE_ID(Dot);
	DEFINE_ID(ThisPointer);
	DEFINE_ID(PointerAccess);
	DEFINE_ID(TemplatedFunction);
	DEFINE_ID(TemplateDefinition);
	DEFINE_ID(Noop);
	DEFINE_ID(AnonymousBlock);
	DEFINE_ID(InlinedFunction);
	DEFINE_ID(InlinedParameter);
	DEFINE_ID(InlinedReturnValue);
	DEFINE_ID(InlinedArgument);
}

namespace InstructionPropertyIds
{
	DEFINE_ID(AssignmentType);
	DEFINE_ID(CallType);
	DEFINE_ID(command);
	DEFINE_ID(ElementSize);
	DEFINE_ID(ElementType);
	DEFINE_ID(First);
	DEFINE_ID(Ids);
	DEFINE_ID(InitValues);
	DEFINE_ID(IsDec);
	DEFINE_ID(IsPre);
	DEFINE_ID(Iterator);
	DEFINE_ID(LoopType);
	DEFINE_ID(MemberInfo);
	DEFINE_ID(NumElements);
	DEFINE_ID(NumBytes);
	DEFINE_ID(ObjectType);
	DEFINE_ID(Operand);
	DEFINE_ID(OpType);
	DEFINE_ID(ParentType);
	DEFINE_ID(ParameterName);
	DEFINE_ID(ReturnType);
	DEFINE_ID(ScopeId);
	DEFINE_ID(Signature);
	DEFINE_ID(Source);
	DEFINE_ID(Symbol);
	DEFINE_ID(Target);
	DEFINE_ID(Type);
	DEFINE_ID(Value);
}

struct InstructionParsers
{
	static Result SyntaxTree(State* state_)
	{
		auto& state = *state_;
		auto isRoot = !state.currentTree.getParent().isValid();

		if (isRoot)
		{
			state.emitSingleInstruction("module", "main");

			state.emitSingleInstruction("import Console");

			StringArray staticSignatures, memberSignatures;

			TypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
			{
				if (v.getType() == InstructionIds::FunctionCall)
				{
					auto sig = v[InstructionPropertyIds::Signature].toString();

					if (!MirCompiler::isExternalFunction(sig))
						return false;

					if (v[InstructionPropertyIds::CallType].toString() == "MemberFunction")
						memberSignatures.addIfNotAlreadyThere(sig);
					else
						staticSignatures.addIfNotAlreadyThere(sig);
				}

				return false;
			});

			for (const auto& c : memberSignatures)
			{
				auto f = TypeConverters::String2FunctionData(c);
				state.functionManager.addPrototype(&state, f, true);
				state.emitSingleInstruction("import " + TypeConverters::FunctionData2MirTextLabel(f));
			}
			for (const auto& c : staticSignatures)
			{
				auto f = TypeConverters::String2FunctionData(c);
				auto isConsole = f.id.getParent().toString() == "Console";
				state.functionManager.addPrototype(&state, f, isConsole);
				state.emitSingleInstruction("import " + TypeConverters::FunctionData2MirTextLabel(f));
			}
		}

		state.processAllChildren();

		if (isRoot)
			state.emitSingleInstruction("endmodule");

		return Result::ok();
	}

	static Result Function(State* state_)
	{
		auto& state = *state_;
		auto f = TypeConverters::String2FunctionData(state[InstructionPropertyIds::Signature]);

		auto addObjectPtr = state.isParsingClass() && !f.returnType.isStatic();

		state.functionManager.addPrototype(&state, f, addObjectPtr);

		TextLine line(&state);
		line.label = TypeConverters::FunctionData2MirTextLabel(f);
		line.instruction = "func ";

		if (f.returnType.isValid())
			line.operands.add(TypeConverters::TypeInfo2MirTextType(f.returnType));

		if (addObjectPtr)
		{
			line.operands.add("i64:_this_");
		}

		// We need to strip the template arguments from the local parameter variable names
		// so that the VariableReference child elements can be resolved correctly
		auto argParent = SimpleTypeParser(state[InstructionPropertyIds::Signature]).parseNamespacedIdentifier();

		for (auto& a : f.args)
		{
			jit::Symbol withoutTemplateArguments;
			withoutTemplateArguments.id = argParent.getChildId(a.id.getIdentifier());
			withoutTemplateArguments.typeInfo = a.typeInfo;
			line.operands.add(TypeConverters::Symbol2MirTextSymbol(withoutTemplateArguments));
		}
			

		line.flush();

		state.registerManager.startFunction();
		state.processChildTree(0);
		state.registerManager.endFunction();

		state.emitSingleInstruction("endfunc");
		state.emitSingleInstruction("export " + line.label);

		return Result::ok();
	}

	static Result ReturnStatement(State* state_)
	{
		auto& state = *state_;

		if (state.currentTree.getNumChildren() != 0)
		{
			auto &rm = state.registerManager;

			SimpleTypeParser p(state[InstructionPropertyIds::Type]);

			state.processChildTree(0);

			TextLine line(&state);
			line.instruction = "ret";

			line.operands.add(rm.loadIntoRegister(0, p.getTypeInfo().isRef() ? RegisterType::Pointer : RegisterType::Value));
			line.flush();
		}
		else
		{
			state.emitSingleInstruction("ret");
		}

		return Result::ok();
	}

	static Result BinaryOp(State* state_)
	{
		auto& state = *state_;

		auto& rm = state.registerManager;

		state.processChildTree(0);
		state.processChildTree(1);

		TextLine line(&state);

		auto type = rm.getTypeForChild(0);
		auto opType = state[InstructionPropertyIds::OpType];

		line.addAnonymousReg(type, RegisterType::Value);
		line.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, opType);
		line.addSelfAsValueOperand();
		line.addChildAsValueOperand(0);
		line.addChildAsValueOperand(1);
		line.flush();

		return Result::ok();
	}

	static Result VariableReference(State* state_)
	{
		auto& state = *state_;
		auto s = TypeConverters::String2Symbol(state[InstructionPropertyIds::Symbol]);

		auto type = TypeConverters::SymbolToMirVar(s).type;
		auto& rm = state.registerManager;

		auto mvn = TypeConverters::NamespacedIdentifier2MangledMirVar(s.id);

		if (state.isParsingFunction())
		{
			for (auto& t : rm.localOperands)
			{
				if (t.text == mvn)
				{
					auto copy = t;
					copy.v = state.currentTree;
					rm.localOperands.add(copy);

					return Result::ok();
				}
			}

			for (auto& t : rm.globalOperands)
			{
				if (t.text == mvn)
				{
					auto copy = t;
					copy.v = state.currentTree;
					rm.globalOperands.add(copy);

					return Result::ok();
				}
			}

			auto isRef = s.typeInfo.isRef();

			rm.registerCurrentTextOperand(mvn, type, isRef ? RegisterType::Pointer : RegisterType::Value);
		}
		else
		{
			rm.registerCurrentTextOperand(mvn, type, RegisterType::Pointer);
		}

		return Result::ok();
	}

	static Result Immediate(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

		auto v = state[InstructionPropertyIds::Value];
		auto id = Types::Helpers::getTypeFromStringValue(v);
		auto type = TypeConverters::TypeInfo2MirType(TypeInfo(id, false, false));
		rm.registerCurrentTextOperand(v, type, RegisterType::Value);

		return Result::ok();
	}

	static Result Cast(State* state_)
	{
		auto& state = *state_;
		state.processChildTree(0);

		auto source = TypeConverters::String2MirType(state[InstructionPropertyIds::Source]);
		auto target = TypeConverters::String2MirType(state[InstructionPropertyIds::Target]);

		String x;

		if (source == MIR_T_I64 && target == MIR_T_F)	   x = "I2F";
		else if (source == MIR_T_F && target == MIR_T_I64) x = "F2I";
		else if (source == MIR_T_I64 && target == MIR_T_D) x = "I2D";
		else if (source == MIR_T_D && target == MIR_T_I64) x = "D2I";
		else if (source == MIR_T_D && target == MIR_T_F)   x = "D2F";
		else if (source == MIR_T_F && target == MIR_T_D)   x = "F2D";

		TextLine line(&state);

		line.addAnonymousReg(target, RegisterType::Value);
		line.instruction = x.toLowerCase();
		line.addSelfAsValueOperand();
		line.addChildAsValueOperand(0);
		line.flush();

		return Result::ok();
	}

	static Result Assignment(State* state_)
	{
		auto& state = *state_;

		if (state.isParsingClass() && !state.isParsingFunction())
		{
			// Skip class definitions (they are redundant)...
			return Result::ok();
		}

		auto& rm = state.registerManager;
		state.processChildTree(0);
		state.processChildTree(1);

		if (state.isParsingFunction())
		{
			auto opType = state[InstructionPropertyIds::AssignmentType];
			auto t = rm.getTypeForChild(0);

			TextLine l(&state);

			if (state[InstructionPropertyIds::First] == "1")
			{
				auto vt = rm.getTypeForChild(1);
				auto vrt = rm.getRegisterTypeForChild(1);

				if (vrt == RegisterType::Pointer)
				{
					jassert(opType == JitTokens::assign_);
					jassert(rm.getRegisterTypeForChild(1) == RegisterType::Pointer);

					auto name = rm.getOperandForChild(1, RegisterType::Pointer);

					rm.registerCurrentTextOperand(name, t, RegisterType::Pointer);
					l.localDef << "i64:" << name;
					l.instruction = "mov";

					l.addSelfAsPointerOperand();
					l.addChildAsPointerOperand(0);
					l.appendComment("Add ref");
					l.flush();

					return Result::ok();
				}

				l.localDef << TypeConverters::MirType2MirTextType(rm.getTypeForChild(1));
				l.localDef << ":" << rm.getOperandForChild(1, RegisterType::Raw);
			}

			//l.addAnonymousReg(state, t, State::RegisterType::Value);

			l.instruction = TypeConverters::MirTypeAndToken2InstructionText(t, opType);

			l.addChildAsValueOperand(1);

			if (opType == JitTokens::assign_)
				l.addChildAsValueOperand(0);
			else
			{
				l.addChildAsValueOperand(1);
				l.addChildAsValueOperand(0);
			}

			l.flush();
		}
		else
		{
            //state.dataManager.addGlobalData(s.id.toString(), state[InstructionPropertyIds::Type]);
            
			TextLine e(&state);
			e.instruction = "export";
			e.addOperands({ 1 }, { RegisterType::Raw });
			e.flush();
			TextLine l(&state);
			l.label = rm.getOperandForChild(1, RegisterType::Raw);
			l.instruction = TypeConverters::MirType2MirTextType(rm.getTypeForChild(1));
			l.instruction;
			l.addOperands({ 0 });
			l.appendComment("global def ");
			l.flush();
            
            auto id = l.label;
            auto type = Types::Helpers::getCppTypeName(TypeConverters::MirType2TypeId(rm.getTypeForChild(1)));
            
            state.dataManager.addGlobalData(id, type);
		}

		return Result::ok();
	}

	static Result Comparison(State* state_)
	{
		auto& state = *state_;

		auto& rm = state.registerManager;
		state.processChildTree(0);
		state.processChildTree(1);

		auto opType = state[InstructionPropertyIds::OpType];
		auto type = rm.getTypeForChild(0);

		TextLine l(&state);
		l.addAnonymousReg(MIR_T_I64, RegisterType::Value);
		l.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, opType);

		l.addSelfAsValueOperand();
		l.addChildAsValueOperand(0);
		l.addChildAsValueOperand(1);
		l.flush();

		return Result::ok();
	}

	static Result TernaryOp(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

		auto trueLabel = state.loopManager.makeLabel();
		auto falseLabel = state.loopManager.makeLabel();
		auto endLabel = state.loopManager.makeLabel();

		// emit the condition
		state.processChildTree(0);

		// jump to false if condition == 0
		TextLine jumpToFalse(&state);
		jumpToFalse.instruction = "bf";
		jumpToFalse.operands.add(falseLabel);
		jumpToFalse.addChildAsValueOperand(0);
		jumpToFalse.flush();

		// emit the true branch
		state.processChildTree(1);

		auto registerType = rm.getRegisterTypeForChild(1);
		auto type = rm.getTypeForChild(1);

		TextLine tl(&state);
		tl.addAnonymousReg(type, registerType);
		tl.flush();

		TextLine movLine_t(&state);

		if (registerType == RegisterType::Value)
		{
			movLine_t.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
			movLine_t.addSelfAsValueOperand();
			movLine_t.addChildAsValueOperand(1);
		}
		else
		{
			movLine_t.instruction = "mov";
			movLine_t.addOperands({ -1, 1 }, { registerType, registerType });
		}

		movLine_t.flush();
		state.emitSingleInstruction("jmp " + endLabel);

		// emit the false branch
		state.emitLabel(falseLabel);
		state.processChildTree(2);

		TextLine movLine_f(&state);

		if (registerType == RegisterType::Value)
		{
			movLine_f.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
			movLine_f.addSelfAsValueOperand();
			movLine_f.addChildAsValueOperand(2);
		}
		else
		{
			movLine_f.instruction = "mov";
			movLine_f.addOperands({ -1, 2 }, { registerType, registerType });
		}

		movLine_f.flush();
		state.emitLabel(endLabel);

		return Result::ok();
	}

	static Result IfStatement(State* state_)
	{
		auto& state = *state_;
		auto hasFalseBranch = state.currentTree.getNumChildren() == 3;

		auto falseLabel = hasFalseBranch ? state.loopManager.makeLabel() : "";

		auto endLabel = state.loopManager.makeLabel();

		state.processChildTree(0);

		// jump to false if condition == 0
		TextLine jumpToFalse(&state);
		jumpToFalse.instruction = "bf";
		jumpToFalse.operands.add(hasFalseBranch ? falseLabel : endLabel);
		jumpToFalse.addChildAsValueOperand(0);
		jumpToFalse.flush();

		state.processChildTree(1);

		if (hasFalseBranch)
		{
			state.emitSingleInstruction("jmp " + endLabel);
			state.emitLabel(falseLabel);
			state.processChildTree(2);
		}

		state.emitLabel(endLabel);

		return Result::ok();
	}

	static Result LogicalNot(State* state_)
	{
		auto& state = *state_;
		state.processChildTree(0);

		TextLine l(&state);
		l.addAnonymousReg(MIR_T_I64, RegisterType::Value);
		l.instruction = "eq";
		l.addSelfAsValueOperand();
		l.addChildAsValueOperand(0);
		l.addImmOperand(VariableStorage(0));
		l.flush();

		return Result::ok();
	}

	static Result WhileLoop(State* state_)
	{
		auto& state = *state_;
		String start_label, end_label, cond_label, post_label;

		int assignIndex = -1;
		int conditionIndex = 0;
		int bodyIndex = 1;
		int postOpIndex = -1;

		if (state[InstructionPropertyIds::LoopType] == "For")
		{
			assignIndex = 0;
			conditionIndex = 1;
			bodyIndex = 2;
			postOpIndex = 3;

			state.loopManager.pushLoopLabels(cond_label, end_label, post_label);
		}
		else
			state.loopManager.pushLoopLabels(cond_label, end_label, cond_label);

		if (assignIndex != -1)
			state.processChildTree(assignIndex);

		state.emitLabel(cond_label);
		state.processChildTree(conditionIndex);

		TextLine jumpToEnd(&state);
		jumpToEnd.instruction = "bf";
		jumpToEnd.operands.add(end_label);
		jumpToEnd.addChildAsValueOperand(conditionIndex);
		jumpToEnd.flush();

		state.processChildTree(bodyIndex);

		if (postOpIndex != -1)
		{
			state.emitLabel(post_label);
			state.processChildTree(postOpIndex);
		}

		state.emitSingleInstruction("jmp " + cond_label);
		state.emitLabel(end_label);

		state.loopManager.popLoopLabels();

		return Result::ok();
	}

	static Result Increment(State* state_)
	{
		auto& state = *state_;
		state.processChildTree(0);

		auto isPre = state[InstructionPropertyIds::IsPre] == "1";
		auto isDec = state[InstructionPropertyIds::IsDec] == "1";

		TextLine mov_l(&state);
		mov_l.addAnonymousReg(MIR_T_I64, RegisterType::Value);
		mov_l.instruction = "mov";
		mov_l.addSelfAsValueOperand();
		mov_l.addChildAsValueOperand(0);

		TextLine add_l(&state);
		add_l.instruction = "add";
		add_l.addChildAsValueOperand(0);
		add_l.addChildAsValueOperand(0);
		add_l.addImmOperand(VariableStorage(isDec ? -1 : 1));

		if (isPre)
		{
			add_l.flush();
			mov_l.flush();
		}
		else
		{
			mov_l.flush();
			add_l.flush();
		}

		return Result::ok();
	}

	static Result StatementBlock(State* state_)
	{
		auto& state = *state_;
		Array<Symbol> localSymbols;

		auto scopeId = NamespacedIdentifier::fromString(state[InstructionPropertyIds::ScopeId]);

		TypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
		{
			if (v.getType() == InstructionIds::Assignment && v[InstructionPropertyIds::First].toString() == "1")
			{
				auto c = v.getChild(1);

				if (c.getType() == InstructionIds::VariableReference)
				{
					auto vSymbol = TypeConverters::String2Symbol(c[InstructionPropertyIds::Symbol].toString());

					if (vSymbol.id.getParent() == scopeId)
					{
						localSymbols.add(vSymbol);
					}
				}

			}

			return false;
		});

		state.processAllChildren();

		return Result::ok();
	}

	static Result ComplexTypeDefinition(State* state_)
	{
		auto& state = *state_;

		if (state.isParsingClass() && !state.isParsingFunction())
		{
			// Skip complex type definitions on the class level,
			// they are embedded into the init values already
			return Result::ok();
		}

		auto& rm = state.registerManager;
		state.processAllChildren();

		SimpleTypeParser p(state[InstructionPropertyIds::Type]);

		if (p.getTypeInfo().isRef())
		{
			auto t = rm.getTypeForChild(0);

			TextLine l(&state);

			auto name = state[InstructionPropertyIds::Ids].upToLastOccurrenceOf(",", false, false);

			auto vn = TypeConverters::NamespacedIdentifier2MangledMirVar(NamespacedIdentifier::fromString(name));

			rm.registerCurrentTextOperand(vn, t, RegisterType::Pointer);

			l.localDef << "i64:" << vn;
			l.instruction = "mov";
			l.addSelfAsPointerOperand();
			l.addChildAsPointerOperand(0);
			l.flush();
		}
		else
		{
			if (!state.isParsingFunction())
			{
				// type copy with initialization
				auto b64 = state[InstructionPropertyIds::InitValues];

				InitValueParser initParser(b64);

				// global definition

				Symbol s;
				s.typeInfo = TypeInfo(Types::ID::Pointer, true, false);
				s.id = NamespacedIdentifier::fromString(state[InstructionPropertyIds::Ids].upToFirstOccurrenceOf(",", false, false).trim());

				uint32 lastOffset = 0;

                state.dataManager.addGlobalData(s.id.toString(), state[InstructionPropertyIds::Type]);
                
				auto numBytes = (uint32)state[InstructionPropertyIds::NumBytes].getIntValue();

				auto numBytesInParser = initParser.getNumBytesRequired();

				if (numBytes == 0 || numBytesInParser == 0)
				{
					TextLine l(&state);

					l.label = s.id.toString();
					l.instruction = "bss";
					l.addImmOperand(jmax<int>(numBytes, numBytesInParser, 8));
					l.appendComment("Dummy Address for zero sized-class object");
					l.flush();
				}
				else
				{
					while (lastOffset < numBytes)
					{
						initParser.forEach([&](uint32 offset, Types::ID type, const VariableStorage& v)
						{
							TextLine l(&state);

							if (lastOffset == 0)
							{
								l.label = s.id.toString();
							}

							auto numBytesForEntry = v.getSizeInBytes();

							if (lastOffset % numBytesForEntry != 0)
							{
								auto alignment = numBytesForEntry - lastOffset % numBytesForEntry;

								lastOffset += alignment;

								String sl = "bss " + String(alignment);
								state.emitSingleInstruction(sl);
							}

							lastOffset += numBytesForEntry;

							l.instruction = TypeConverters::TypeInfo2MirTextType(TypeInfo(type));

							// use 4 bytes for integer initialisation...
							if (l.instruction == "i64")
								l.instruction = "i32";

							if (v.getType() == Types::ID::Pointer)
							{
								auto childIndex = (int)reinterpret_cast<int64_t>(v.getDataPointer());

								l.addOperands({ childIndex }, { RegisterType::Value });
							}
							else
								l.addImmOperand(v);

							l.flush();
						});
					}

					if (lastOffset % 8 != 0)
					{
						TextLine l(&state);
						l.instruction = "bss";
						l.addImmOperand(4);
						l.appendComment("pad to 8 byte alignment");
						l.flush();
					}
				}
			}
			else
			{
				// stack definition
				auto numBytes = (uint32)state[InstructionPropertyIds::NumBytes].getIntValue();
				auto numOriginalBytes = numBytes;

				bool something = numBytes != 0;

				auto name = NamespacedIdentifier::fromString(state[InstructionPropertyIds::Ids].upToFirstOccurrenceOf(",", false, false).trim());
				auto mn = TypeConverters::NamespacedIdentifier2MangledMirVar(name);

				numBytes = rm.allocateStack(mn, numBytes, true);

				auto hasDynamicInitialisation = state.currentTree.getNumChildren() != 0;

				if (hasDynamicInitialisation)
				{
					if (!state.currentTree.hasProperty("InitValues"))
					{
						// Only one expression is supported at the moment...
						jassert(state.currentTree.getNumChildren() == 1);

						state.processChildTree(0);

						auto src = rm.loadIntoRegister(0, RegisterType::Pointer);

						rm.emitMultiLineCopy(mn, src, numBytes);
					}
					else
					{
						// mix expression with init values, not implemented...
						jassertfalse;
					}
				}
				else
				{
					// static type copy with initialization values
					auto b64 = state[InstructionPropertyIds::InitValues];

					InitValueParser initParser(b64);

					uint32 lastOffset = 0;

					

					while (something && lastOffset < numBytes)
					{
						int numToPad = numBytes - numOriginalBytes;

						initParser.forEach([&](uint32 offset, Types::ID type, const VariableStorage& v)
						{
							TextLine il(&state);
							auto t = TypeConverters::TypeInfo2MirType(TypeInfo(type));
							il.instruction = TypeConverters::MirTypeAndToken2InstructionText(t, JitTokens::assign_);
							auto p = rm.getOperandForChild(-1, RegisterType::Raw);
							auto mir_t = TypeConverters::MirType2MirTextType(t);

							// use 4 bytes for integer initialisation...

							String op;
							op << mir_t << ":";
							if (lastOffset != 0)
								op << String(lastOffset);
							op << "(" << p << ")";

							il.operands.add(op);

							lastOffset += v.getSizeInBytes();

							if (v.getType() == Types::ID::Pointer)
							{
								auto childIndex = (int)reinterpret_cast<int64_t>(v.getDataPointer());
								il.addChildAsValueOperand(childIndex);
							}
							else
								il.addImmOperand(v);

							il.flush();
						});

						while(numToPad > 0)
						{
							TextLine il(&state);

							il.instruction = "mov";

							auto p = rm.getOperandForChild(-1, RegisterType::Raw);
							auto mir_t = "i32";

							String op;
							op << mir_t << ":";
							op << String(lastOffset);
							op << "(" << p << ")";

							il.operands.add(op);
							il.addImmOperand(0);
							il.flush();

							lastOffset += 4;
							numToPad -= 4;
						}
					}
				}

#if 0
				// fill with zeros
				while (lastOffset < numBytesToAllocate)
				{
					TextLine il;

					il.instruction = "mov";

					auto p = state.getOperandForChild(-1, State::RegisterType::Raw);
					auto mir_t = "i32";

					String op;
					op << mir_t << ":";
					op << String(lastOffset);
					op << "(" << p << ")";

					il.operands.add(op);
					il.addImmOperand(0);
					state.emitLine(il);

					lastOffset += 4;
				}
#endif
			}
		}



		return Result::ok();
	}

	static Result Subscript(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

		auto type = SimpleTypeParser(state[InstructionPropertyIds::ParentType]).getComplexTypeId().toString().upToFirstOccurrenceOf("<", false, false);

		
		state.processChildTree(0);
		state.processChildTree(1);

		auto t = TypeConverters::String2Symbol(state[InstructionPropertyIds::ElementType]);

		auto mir_t = TypeConverters::TypeInfo2MirType(t.typeInfo);
		auto tn = TypeConverters::TypeInfo2MirTextType(t.typeInfo);

		//mir_t = MIR_T_I64;

		String self, idx_reg;

		

		if (type == "span")
		{
            TextLine l(&state);
            self = l.addAnonymousReg(mir_t, RegisterType::Pointer);
			l.instruction = "mov";
			l.addOperands({ -1, 0 }, { RegisterType::Pointer, RegisterType::Pointer });
			String comment;
			comment << "span " << self << " = " << rm.getOperandForChild(0, RegisterType::Raw) << "[" << rm.getOperandForChild(1, RegisterType::Raw) << "]";
			l.appendComment(comment);
			l.flush();
		}
		else if (type == "dyn")
		{
            TextLine l(&state);
            self = l.addAnonymousReg(mir_t, RegisterType::Pointer);
			l.instruction = "mov";

			l.operands.add(self);
			l.operands.add("i64:8(" + rm.loadIntoRegister(0, RegisterType::Pointer) + ")");
			String comment;

			

			comment << "dyn " << self << " = " << rm.getOperandForChild(0, RegisterType::Raw) << "[" << rm.getOperandForChild(1, RegisterType::Raw) << "]";
			l.appendComment(comment);
			l.flush();
		}

		

		TextLine idx(&state);
		idx_reg = idx.addAnonymousReg(MIR_T_I64, RegisterType::Value);
		idx.instruction = "mov";
		idx.operands.add(idx_reg);
		idx.addChildAsValueOperand(1);
		idx.flush();

		TextLine scaleLine(&state);
		scaleLine.instruction = "mul";
		scaleLine.operands.add(idx_reg);
		scaleLine.operands.add(idx_reg);
		scaleLine.addImmOperand(state[InstructionPropertyIds::ElementSize].getIntValue());
		scaleLine.flush();

		TextLine offset_l(&state);
		offset_l.instruction = "add";
		offset_l.operands.add(self);
		offset_l.operands.add(self);
		offset_l.operands.add(idx_reg);
		offset_l.flush();


		return Result::ok();
		
	}

	static Result InternalProperty(State* state_)
	{
		return Result::ok();
	}

	static Result ControlFlowStatement(State* state_)
	{
		auto& state = *state_;
		auto command = state[InstructionPropertyIds::command];

		TextLine l(&state);
		l.instruction = "jmp";
		l.operands.add(state.loopManager.getCurrentLabel(command));
		l.appendComment(command);
		l.flush();

		return Result::ok();
	}

	static Result Loop(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;
		String startLabel, endLabel, unusedLabel;

		auto loopType = state[InstructionPropertyIds::LoopType];

		state.loopManager.pushLoopLabels(startLabel, endLabel, unusedLabel);
		
		// create iterator variable with pointer type
		auto iteratorSymbol = TypeConverters::String2Symbol(state[InstructionPropertyIds::Iterator]);
		auto type = TypeConverters::SymbolToMirVar(iteratorSymbol).type;
		auto mvn = TypeConverters::NamespacedIdentifier2MangledMirVar(iteratorSymbol.id);

		auto isRef = iteratorSymbol.typeInfo.isRef();

		state.processChildTree(0);

		rm.registerCurrentTextOperand(mvn, type, isRef ? RegisterType::Pointer :
			RegisterType::Value);

		// If the iterator is not a reference, we'll create another register for the address
		String pointerReg = isRef ? mvn : state.registerManager.getAnonymousId(false);

		TextLine loadIter(&state);
		loadIter.localDef << "i64:" << pointerReg;
		loadIter.instruction = "mov";

		// create anonymous end address variable with pointer type
		String endReg = state.registerManager.getAnonymousId(false);

		auto elementSize = state[InstructionPropertyIds::ElementSize].getIntValue();

		if (loopType == "Span")
		{
			loadIter.operands.add(pointerReg);
			loadIter.addOperands({ 0 }, { RegisterType::Pointer });
			loadIter.flush();

			int byteSize = elementSize * state[InstructionPropertyIds::NumElements].getIntValue();

			TextLine ii(&state);
			ii.localDef << "i64:" << endReg;
			ii.instruction = "add";
			ii.operands.add(endReg);
			ii.operands.add(pointerReg);
			ii.addImmOperand(byteSize);
			ii.flush();
		}
		else
		{
			loadIter.operands.add(pointerReg);

			auto dynPtr = rm.loadIntoRegister(0, RegisterType::Pointer);

			String op("i64:8(" + dynPtr + ")");
			loadIter.operands.add(op);
			loadIter.flush();

			TextLine ii(&state);
			ii.localDef << "i64:" << endReg;
			ii.instruction = "mul";
			ii.operands.add(endReg);
			ii.operands.add("i32:4  (" + dynPtr + ")");
			ii.addImmOperand(elementSize);
			ii.flush();

			//pointerReg = "i64:8(" + pointerReg + ")";

			TextLine ii2(&state);

			ii2.instruction = "add";
			ii2.operands.add(endReg);
			ii2.operands.add(endReg);
			ii2.operands.add(pointerReg);
			ii2.flush();
		}

		// emit start_loop label
		state.emitLabel(startLabel);

		if (!isRef)
		{
			auto tn = TypeConverters::MirType2MirTextType(type);

			// Load the value into the iterator variable
			// from the address in pointerReg
			TextLine loadCopy(&state);
			loadCopy.localDef << tn << ":" << mvn;
			loadCopy.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
			loadCopy.operands.add(mvn);

			String ptrAddress;
			ptrAddress << tn << ":(" << pointerReg << ")";

			loadCopy.operands.add(ptrAddress);
			loadCopy.appendComment("iterator load");
			loadCopy.flush();
		}

		// compare addresses, jump to start_loop if iterator < end
		TextLine cmp(&state);
		cmp.instruction = "bge";
		cmp.operands.add(endLabel);
		cmp.operands.add(pointerReg);
		cmp.operands.add(endReg);
		
		cmp.flush();

		// emit body
		state.processChildTree(1);

		// bump address by element size after loop body
		TextLine bump(&state);
		bump.instruction = "add";
		bump.operands.add(pointerReg);
		bump.operands.add(pointerReg);
		bump.addImmOperand(elementSize);
		bump.flush();

		TextLine jmp(&state);
		jmp.instruction = "jmp";
		jmp.operands.add(startLabel);
		jmp.flush();


		// emit end_loop label
		state.emitLabel(endLabel);
		state.loopManager.popLoopLabels();

		return Result::ok();
	}

	static Result FunctionCall(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;
		state.processAllChildren();

		auto sig = TypeConverters::String2FunctionData(state[InstructionPropertyIds::Signature]);
		auto fid = TypeConverters::FunctionData2MirTextLabel(sig);

		if (state.functionManager.hasPrototype(sig))
		{
			auto protoType = state.functionManager.getPrototype(sig);

			TextLine l(&state);

			l.instruction = "call";
			l.operands.add(protoType);
			l.operands.add(fid);

			if (sig.returnType.isValid())
			{
				auto rrt = sig.returnType.isRef() ? RegisterType::Pointer : RegisterType::Value;
				l.addAnonymousReg(TypeConverters::TypeInfo2MirType(sig.returnType), rrt);
				l.operands.add(rm.getOperandForChild(-1, rrt));
			}

			int argOffset = 0;

			if (state[InstructionPropertyIds::CallType] == "MemberFunction")
			{
				auto a = state.registerManager.loadIntoRegister(0, RegisterType::Pointer);
				l.operands.add(a);
				argOffset = 1;
			}

			if (fid.startsWith("Console_"))
			{
				l.operands.add("Console");
				
			}

			for (int i = 0; i < sig.args.size(); i++)
			{
				auto isRef = sig.args[i].typeInfo.isRef();
				auto isComplexType = sig.args[i].typeInfo.getType() == Types::ID::Pointer;

				if (isComplexType && !isRef)
				{
					// We need to copy the object on the stack and use the stack pointer as argument
					auto argString = state[InstructionPropertyIds::Signature].fromFirstOccurrenceOf("(", false, false);

					for (int j = 0; j < i; j++)
						argString = SimpleTypeParser(argString).getTrailingString().fromFirstOccurrenceOf(",", false, false);

					SimpleTypeParser p(argString);

					auto numBytes = state.dataManager.getNumBytesRequired(p.getComplexTypeId().getIdentifier());

					auto stackPtr = state.registerManager.getAnonymousId(false);

					numBytes = rm.allocateStack(stackPtr, numBytes, false);
					auto source = rm.loadIntoRegister(i + argOffset, RegisterType::Pointer);
					rm.emitMultiLineCopy(stackPtr, source, numBytes);
					l.operands.add(stackPtr);
				}
				else
				{
					if (isRef)
					{
						auto rt = rm.getRegisterTypeForChild(i + argOffset);
						auto tp = rm.getTypeForChild(i + argOffset);

						if (rt == RegisterType::Value && tp != MIR_T_P)
						{
							// We need to copy the value from the register to the stack


							auto mir_t = rm.getTypeForChild(i + argOffset);

							auto stackPtr = rm.getAnonymousId(false);

							rm.allocateStack(stackPtr, Types::Helpers::getSizeForType(TypeConverters::MirType2TypeId(mir_t)), false);

							TextLine mv(&state);
							mv.instruction = TypeConverters::MirTypeAndToken2InstructionText(mir_t, JitTokens::assign_);

							String dst;
							dst << TypeConverters::MirType2MirTextType(mir_t);
							dst << ":(" << stackPtr << ")";

							mv.operands.add(dst);
							mv.addChildAsValueOperand(i + argOffset);
							mv.appendComment("Move register to stack");
							mv.flush();
							
							auto registerToSpill = rm.getOperandForChild(i + argOffset, RegisterType::Value);

							for (auto& l : rm.localOperands)
							{
								if (l.text == registerToSpill)
								{
									// update the operands to use the stack pointer from now on...
									l.registerType = RegisterType::Pointer;
									l.stackPtr = stackPtr;
								}
							}

							l.operands.add(stackPtr);
						}
						else
						{
							if (tp == MIR_T_P)
							{
								// it's already loaded into a register
								auto a = rm.getOperandForChild(i + argOffset, RegisterType::Pointer);
								l.operands.add(a);
							}
							else
							{
								// make sure the adress is loaded into a register
								auto a = rm.loadIntoRegister(i + argOffset, RegisterType::Pointer);
								l.operands.add(a);
							}
						}
					}
					else
					{
						auto a = rm.loadIntoRegister(i + argOffset, RegisterType::Value);
						l.operands.add(a);
					}
				}
			}

			l.flush();
		}
		else
		{
			SimpleTypeParser p(state[InstructionPropertyIds::ObjectType]);
			auto type = p.getComplexTypeId().toString();

			auto dataObject = state.dataManager.getDataObject(type);

			auto funcData = sig.createDataLayout(true);

			// This should only happen with unresolved functions
			jassert(!(bool)funcData["IsResolved"]);

			int numArgsAvailable = state.currentTree.getNumChildren();
			int numArgsDefined = funcData.getNumChildren();

			// all defaultable parameters should be resolved by the parser
			jassert(numArgsAvailable == numArgsDefined);

			for (int i = 0; i < jmin(numArgsAvailable, numArgsDefined); i++)
			{
				SimpleTypeParser p(funcData.getChild(i)[InstructionPropertyIds::Type].toString());
				auto t = p.getTypeInfo();
				auto isRef = t.isRef() || t.getType() == Types::ID::Pointer;
				auto registerType = isRef ? RegisterType::Pointer : RegisterType::Value;
				auto a = rm.loadIntoRegister(i, registerType);

				funcData.getChild(i).setProperty(InstructionPropertyIds::Operand, a, nullptr);
			}

			if (auto il = state.inlinerManager.getInliner(fid))
			{
				TextLine c(&state);
				c.appendComment("Inlined function " + fid);
				c.flush();

				auto returnReg = il(&state, dataObject, funcData);

				if (returnReg.text.isNotEmpty())
				{
					rm.registerCurrentTextOperand(returnReg.text, returnReg.type, returnReg.registerType);
				}

				return Result::ok();
			}
			else
			{
				throw String("inliner for " + fid + " not found");
			}
		}

		return Result::ok();
	};

	static Result ClassStatement(State* state_)
	{
		auto& state = *state_;
		auto members = StringArray::fromTokens(state[InstructionPropertyIds::MemberInfo], "$", "");

		Array<MemberInfo> memberInfo;

		for (const auto& m : members)
		{
			MemberInfo item;

			auto symbol = TypeConverters::String2Symbol(m);

			item.id = TypeConverters::NamespacedIdentifier2MangledMirVar(symbol.id);
			item.type = TypeConverters::TypeInfo2MirType(symbol.typeInfo);
			item.offset = m.fromFirstOccurrenceOf("(", false, false).getIntValue();

			memberInfo.add(item);
		}

		auto x = NamespacedIdentifier::fromString(state[InstructionPropertyIds::Type]);
		auto className = Identifier(TypeConverters::NamespacedIdentifier2MangledMirVar(x));

		state.dataManager.startClass(className, std::move(memberInfo));
		state.processAllChildren();
		state.dataManager.endClass();
		
		// Nothing to do for now, maybe we need to parse the functions, yo...
		return Result::ok();
	}

	static Result Dot(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

		state.processChildTree(0);

		auto memberSymbol = TypeConverters::String2Symbol(state.getCurrentChild(1)[InstructionPropertyIds::Symbol].toString());

		auto classId = Identifier(TypeConverters::NamespacedIdentifier2MangledMirVar(memberSymbol.id.getParent()));
		auto memberId = memberSymbol.id.getIdentifier().toString();
		auto memberType = TypeConverters::Symbol2MirTextSymbol(memberSymbol);

		for (const auto& m : state.dataManager.getClassType(classId))
		{
			if (m.id == memberId)
			{
				auto ptr = rm.loadIntoRegister(0, RegisterType::Pointer);

				String p;

				TextLine offset(&state);
				offset.instruction = "add";
				offset.operands.add(ptr);
				offset.operands.add(ptr);
				offset.addImmOperand((int)m.offset);
				offset.appendComment(classId + "." + m.id);
				offset.flush();

				rm.registerCurrentTextOperand(ptr, m.type, RegisterType::Pointer);
				break;
			}
		}

		return Result::ok();
	}

	static Result ThisPointer(State* state_)
	{
		auto& state = *state_;
		state.registerManager.registerCurrentTextOperand("_this_", MIR_T_P, RegisterType::Value);

		return Result::ok();
	}

	static Result PointerAccess(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;
		state.processChildTree(0);

		auto n = rm.getOperandForChild(0, RegisterType::Raw);
		rm.registerCurrentTextOperand(n, MIR_T_P, RegisterType::Value);

		return Result::ok();
	}

	static Result TemplatedFunction(State* state)
	{
		state->processAllChildren();
		return Result::ok();
	}

	static Result TemplateDefinition(State* state)
	{
		state->processAllChildren();
		return Result::ok();
	}

	static Result AnonymousBlock(State* state)
	{
		state->processAllChildren();
		return Result::ok();
	}

	static Result Noop(State* state)
	{
		return Result::ok();
	}

	static Result InlinedFunction(State* state_)
	{
		auto& state = *state_;

		String il;

		auto returnType = SimpleTypeParser(state[InstructionPropertyIds::ReturnType]).getTypeInfo();

		

		if (returnType.isValid())
		{
			auto mir_t = TypeConverters::TypeInfo2MirType(returnType);
			auto rt = returnType.isRef() ? RegisterType::Pointer : RegisterType::Value;

			TextLine ml(state_);
			ml.addAnonymousReg(mir_t, rt);
			ml.flush();

			auto name = state.registerManager.getOperandForChild(-1, rt);
			state.loopManager.pushInlineFunction(il, mir_t, name);
		}
		else
		{
			state.loopManager.pushInlineFunction(il);
		}

		

		state.processAllChildren();

		state.emitLabel(il);
		state.loopManager.popInlineFunction();

		return Result::ok();
	}

	static Result InlinedArgument(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

		auto n = state[InstructionPropertyIds::ParameterName];

		state.processChildTree(0);

		auto r = rm.getOperandForChild(0, RegisterType::Raw);

		state.loopManager.addInlinedArgument(n, r);

		return Result::ok();
	}

	static Result InlinedParameter(State* state_)
	{
		
		auto& state = *state_;

		auto arg = state[InstructionPropertyIds::Symbol];
		auto p = state.loopManager.getInlinedParameter(arg);

		state.registerManager.registerCurrentTextOperand(p, MIR_T_I64, RegisterType::Value);

		return Result::ok();
	}

	static Result InlinedReturnValue(State* state_)
	{
		auto& state = *state_;

        if(state.currentTree.getNumChildren() > 0)
            state.processChildTree(0);

		state.loopManager.emitInlinedReturn(state_);

		return Result::ok();
	}
};

}
}
