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
}



struct InstructionParsers
{
	static Result SyntaxTree(State& state)
	{
		auto isRoot = !state.currentTree.getParent().isValid();

		if (isRoot)
		{
			state.emitSingleInstruction("module", "main");

			StringArray staticSignatures, memberSignatures;

			TypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
			{
				if (v.getType() == InstructionIds::FunctionCall)
				{
					auto sig = v.getProperty("Signature").toString();

					if (!MirObject::isExternalFunction(sig))
						return false;

					if (v.getProperty("CallType").toString() == "MemberFunction")
						memberSignatures.addIfNotAlreadyThere(sig);
					else
						staticSignatures.addIfNotAlreadyThere(sig);
				}

				return false;
			});

			for (const auto& c : memberSignatures)
			{
				auto f = TypeConverters::String2FunctionData(c);
				state.addPrototype(f, true);
				state.emitSingleInstruction("import " + TypeConverters::FunctionData2MirTextLabel(f));
			}
			for (const auto& c : staticSignatures)
			{
				auto f = TypeConverters::String2FunctionData(c);
				state.addPrototype(f, false);
				state.emitSingleInstruction("import " + TypeConverters::FunctionData2MirTextLabel(f));
			}
		}


		state.processAllChildren();

		if (isRoot)
			state.emitSingleInstruction("endmodule");

		return Result::ok();
	}

	static Result Function(State& state)
	{
		auto f = TypeConverters::String2FunctionData(state.getProperty("Signature"));

		auto addObjectPtr = state.isParsingClass() && !f.returnType.isStatic();

		state.addPrototype(f, addObjectPtr);

		TextLine line(&state);
		line.label = TypeConverters::FunctionData2MirTextLabel(f);
		line.instruction = "func ";

		if (f.returnType.isValid())
			line.operands.add(TypeConverters::TypeInfo2MirTextType(f.returnType));

		if (addObjectPtr)
		{
			line.operands.add("i64:_this_");
		}

		for (auto& a : f.args)
			line.operands.add(TypeConverters::Symbol2MirTextSymbol(a));

		line.flush();

		state.isParsingFunction = true;
		state.processChildTree(0);
		state.isParsingFunction = false;

		state.emitSingleInstruction("endfunc");
		state.emitSingleInstruction("export " + line.label);

		state.localOperands.clear();

		return Result::ok();
	}

	static Result ReturnStatement(State& state)
	{
		if (state.currentTree.getNumChildren() != 0)
		{
			SimpleTypeParser p(state.getProperty("Type"));

			state.processChildTree(0);

			TextLine line(&state);
			line.instruction = "ret";

			line.operands.add(state.loadIntoRegister(0, p.getTypeInfo().isRef() ? RegisterType::Pointer : RegisterType::Value));
			line.flush();
		}
		else
		{
			state.emitSingleInstruction("ret");
		}

		return Result::ok();
	}

	static Result BinaryOp(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		TextLine line(&state);

		auto type = state.getTypeForChild(0);
		auto opType = state.getProperty("OpType");

		line.addAnonymousReg(type, RegisterType::Value);
		line.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, opType);
		line.addSelfAsValueOperand();
		line.addChildAsValueOperand(0);
		line.addChildAsValueOperand(1);
		line.flush();

		return Result::ok();
	}

	static Result VariableReference(State& state)
	{
		auto s = TypeConverters::String2Symbol(state.getProperty("Symbol"));

		auto type = TypeConverters::SymbolToMirVar(s).type;

		if (state.isParsingFunction)
		{
			auto mvn = TypeConverters::NamespacedIdentifier2MangledMirVar(s.id);

			for (auto& t : state.localOperands)
			{
				if (t.text == mvn)
				{
					auto copy = t;
					copy.v = state.currentTree;
					state.localOperands.add(copy);

					return Result::ok();
				}
			}

			for (auto& t : state.globalOperands)
			{
				if (t.text == mvn)
				{
					auto copy = t;
					copy.v = state.currentTree;
					state.globalOperands.add(copy);

					return Result::ok();
				}
			}

			auto isRef = s.typeInfo.isRef();

			state.registerCurrentTextOperand(mvn, type, isRef ? RegisterType::Pointer : RegisterType::Value);
		}
		else
		{
			state.registerCurrentTextOperand(TypeConverters::NamespacedIdentifier2MangledMirVar(s.id), type, RegisterType::Pointer);
		}

		return Result::ok();
	}

	static Result Immediate(State& state)
	{
		auto v = state.getProperty("Value");
		auto id = Types::Helpers::getTypeFromStringValue(v);
		auto type = TypeConverters::TypeInfo2MirType(TypeInfo(id, false, false));
		state.registerCurrentTextOperand(v, type, RegisterType::Value);

		return Result::ok();
	}

	static Result Cast(State& state)
	{
		state.processChildTree(0);

		auto source = TypeConverters::String2MirType(state.getProperty("Source"));
		auto target = TypeConverters::String2MirType(state.getProperty("Target"));

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

	static Result Assignment(State& state)
	{
		if (state.isParsingClass() && !state.isParsingFunction)
		{
			// Skip class definitions (they are redundant)...
			return Result::ok();
		}

		state.processChildTree(0);
		state.processChildTree(1);

		if (state.isParsingFunction)
		{
			auto opType = state.getProperty("AssignmentType");
			auto t = state.getTypeForChild(0);

			TextLine l(&state);

			if (state.getProperty("First") == "1")
			{
				auto vt = state.getTypeForChild(1);
				auto vrt = state.getRegisterTypeForChild(1);

				if (vrt == RegisterType::Pointer)
				{
					jassert(opType == JitTokens::assign_);
					jassert(state.getRegisterTypeForChild(1) == RegisterType::Pointer);

					auto name = state.getOperandForChild(1, RegisterType::Pointer);

					state.registerCurrentTextOperand(name, t, RegisterType::Pointer);
					l.localDef << "i64:" << name;
					l.instruction = "mov";

					l.addSelfAsPointerOperand();
					l.addChildAsPointerOperand(0);
					l.appendComment("Add ref");
					l.flush();

					return Result::ok();
				}

				l.localDef << TypeConverters::MirType2MirTextType(state.getTypeForChild(1));
				l.localDef << ":" << state.getOperandForChild(1, RegisterType::Raw);
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
			TextLine e(&state);
			e.instruction = "export";
			e.addOperands({ 1 }, { RegisterType::Raw });
			e.flush();
			TextLine l(&state);
			l.label = state.getOperandForChild(1, RegisterType::Raw);
			l.instruction = TypeConverters::MirType2MirTextType(state.getTypeForChild(1));
			l.instruction;
			l.addOperands({ 0 });
			l.appendComment("global def ");
			l.flush();
		}

		return Result::ok();
	}

	static Result Comparison(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		auto opType = state.getProperty("OpType");
		auto type = state.getTypeForChild(0);

		TextLine l(&state);
		l.addAnonymousReg(MIR_T_I64, RegisterType::Value);
		l.instruction = TypeConverters::MirTypeAndToken2InstructionText(type, opType);

		l.addSelfAsValueOperand();
		l.addChildAsValueOperand(0);
		l.addChildAsValueOperand(1);
		l.flush();

		return Result::ok();
	}

	static Result TernaryOp(State& state)
	{
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

		auto registerType = state.getRegisterTypeForChild(1);
		auto type = state.getTypeForChild(1);

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

	static Result IfStatement(State& state)
	{
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

	static Result LogicalNot(State& state)
	{
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

	static Result WhileLoop(State& state)
	{
		String start_label, end_label, cond_label, post_label;

		int assignIndex = -1;
		int conditionIndex = 0;
		int bodyIndex = 1;
		int postOpIndex = -1;

		if (state.getProperty("LoopType") == "For")
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

	static Result Increment(State& state)
	{
		state.processChildTree(0);

		auto isPre = state.getProperty("IsPre") == "1";
		auto isDec = state.getProperty("IsDec") == "1";

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

	static Result StatementBlock(State& state)
	{
		Array<Symbol> localSymbols;

		auto scopeId = NamespacedIdentifier::fromString(state.getProperty("ScopeId"));

		TypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
		{
			if (v.getType() == InstructionIds::Assignment && v["First"].toString() == "1")
			{
				auto c = v.getChild(1);

				if (c.getType() == InstructionIds::VariableReference)
				{
					auto vSymbol = TypeConverters::String2Symbol(c.getProperty("Symbol").toString());

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

	static Result ComplexTypeDefinition(State& state)
	{
		if (state.isParsingClass() && !state.isParsingFunction)
		{
			// Skip complex type definitions on the class level,
			// they are embedded into the init values already
			return Result::ok();
		}

		state.processAllChildren();

		SimpleTypeParser p(state.getProperty("Type"));

		if (p.getTypeInfo().isRef())
		{
			auto t = state.getTypeForChild(0);

			TextLine l(&state);

			auto name = state.getProperty("Ids").upToLastOccurrenceOf(",", false, false);

			auto vn = TypeConverters::NamespacedIdentifier2MangledMirVar(NamespacedIdentifier::fromString(name));

			state.registerCurrentTextOperand(vn, t, RegisterType::Pointer);

			l.localDef << "i64:" << vn;
			l.instruction = "mov";
			l.addSelfAsPointerOperand();
			l.addChildAsPointerOperand(0);
			l.flush();
		}
		else
		{
			if (!state.isParsingFunction)
			{
				// type copy with initialization
				auto b64 = state.getProperty("InitValues");

				InitValueParser initParser(b64);

				// global definition

				Symbol s;
				s.typeInfo = TypeInfo(Types::ID::Pointer, true, false);
				s.id = NamespacedIdentifier::fromString(state.getProperty("Ids").upToFirstOccurrenceOf(",", false, false).trim());

				uint32 lastOffset = 0;

				auto numBytes = (uint32)state.getProperty("NumBytes").getIntValue();

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
				auto numBytes = (uint32)state.getProperty("NumBytes").getIntValue();

				bool something = numBytes != 0;

				auto name = NamespacedIdentifier::fromString(state.getProperty("Ids").upToFirstOccurrenceOf(",", false, false).trim());
				auto mn = TypeConverters::NamespacedIdentifier2MangledMirVar(name);

				numBytes = state.allocateStack(mn, numBytes, true);

				auto hasDynamicInitialisation = state.currentTree.getNumChildren() != 0;

				if (hasDynamicInitialisation)
				{
					if (!state.currentTree.hasProperty("InitValues"))
					{
						// Only one expression is supported at the moment...
						jassert(state.currentTree.getNumChildren() == 1);

						state.processChildTree(0);

						auto src = state.loadIntoRegister(0, RegisterType::Pointer);

						state.emitMultiLineCopy(mn, src, numBytes);
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
					auto b64 = state.getProperty("InitValues");

					InitValueParser initParser(b64);

					uint32 lastOffset = 0;

					while (something && lastOffset < numBytes)
					{
						initParser.forEach([&](uint32 offset, Types::ID type, const VariableStorage& v)
						{
							TextLine il(&state);
							auto t = TypeConverters::TypeInfo2MirType(TypeInfo(type));
							il.instruction = TypeConverters::MirTypeAndToken2InstructionText(t, JitTokens::assign_);
							auto p = state.getOperandForChild(-1, RegisterType::Raw);
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

	static Result Subscript(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		auto t = TypeConverters::String2Symbol(state.getProperty("ElementType"));

		auto mir_t = TypeConverters::TypeInfo2MirType(t.typeInfo);
		auto tn = TypeConverters::TypeInfo2MirTextType(t.typeInfo);

		//mir_t = MIR_T_I64;

		String self, idx_reg;

		TextLine l(&state);
		self = l.addAnonymousReg(mir_t, RegisterType::Pointer);
		l.instruction = "mov";
		l.addOperands({ -1, 0 }, { RegisterType::Pointer, RegisterType::Pointer });
		String comment;
		comment << self << " = " << state.getOperandForChild(0, RegisterType::Raw) << "[" << state.getOperandForChild(1, RegisterType::Raw) << "]";
		l.appendComment(comment);
		l.flush();

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
		scaleLine.addImmOperand(state.getProperty("ElementSize").getIntValue());
		scaleLine.flush();

		TextLine offset_l(&state);
		offset_l.instruction = "add";
		offset_l.operands.add(self);
		offset_l.operands.add(self);
		offset_l.operands.add(idx_reg);
		offset_l.flush();
		

		return Result::ok();
	}

	static Result InternalProperty(State& state)
	{
		return Result::ok();
	}

	static Result ControlFlowStatement(State& state)
	{
		auto command = state.getProperty("command");

		TextLine l(&state);
		l.instruction = "jmp";
		l.operands.add(state.loopManager.getCurrentLabel(command));
		l.appendComment(command);
		l.flush();

		return Result::ok();
	}

	static Result Loop(State& state)
	{
		String startLabel, endLabel, unusedLabel;

		state.loopManager.pushLoopLabels(startLabel, endLabel, unusedLabel);
		
		// create iterator variable with pointer type
		auto iteratorSymbol = TypeConverters::String2Symbol(state.getProperty("Iterator"));
		auto type = TypeConverters::SymbolToMirVar(iteratorSymbol).type;
		auto mvn = TypeConverters::NamespacedIdentifier2MangledMirVar(iteratorSymbol.id);

		auto isRef = iteratorSymbol.typeInfo.isRef();

		state.processChildTree(0);

		state.registerCurrentTextOperand(mvn, type, isRef ? RegisterType::Pointer :
			RegisterType::Value);

		// If the iterator is not a reference, we'll create another register for the address
		String pointerReg = isRef ? mvn : state.registerManager.getAnonymousId(false);

		TextLine loadIter(&state);
		loadIter.localDef << "i64:" << pointerReg;
		loadIter.instruction = "mov";
		loadIter.operands.add(pointerReg);
		loadIter.addOperands({ 0 }, { RegisterType::Pointer });
		loadIter.flush();

		// create anonymous end address variable with pointer type
		auto endReg = state.registerManager.getAnonymousId(false);

		auto elementSize = (int)state.currentTree.getProperty("ElementSize");

		if (state.getProperty("LoopType") == "Span")
		{
			int byteSize = elementSize *
				(int)state.currentTree.getProperty("NumElements");

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
			// not implemented...
			jassertfalse;
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

		// emit body
		state.processChildTree(1);

		// bump address by element size after loop body
		TextLine bump(&state);
		bump.instruction = "add";
		bump.operands.add(pointerReg);
		bump.operands.add(pointerReg);
		bump.addImmOperand(elementSize);
		bump.flush();

		// compare addresses, jump to start_loop if iterator != end
		TextLine cmp(&state);
		cmp.instruction = "bne";
		cmp.operands.add(startLabel);
		cmp.operands.add(pointerReg);
		cmp.operands.add(endReg);
		cmp.flush();

		// emit end_loop label
		state.emitLabel(endLabel);
		state.loopManager.popLoopLabels();

		return Result::ok();
	}

	static Result FunctionCall(State& state)
	{
		state.processAllChildren();

		auto sig = TypeConverters::String2FunctionData(state.getProperty("Signature"));
		auto fid = TypeConverters::FunctionData2MirTextLabel(sig);

		if (state.hasPrototype(sig))
		{
			auto protoType = state.getPrototype(sig);

			TextLine l(&state);

			l.instruction = "call";
			l.operands.add(protoType);
			l.operands.add(fid);

			if (sig.returnType.isValid())
			{
				auto rrt = sig.returnType.isRef() ? RegisterType::Pointer : RegisterType::Value;
				l.addAnonymousReg(TypeConverters::TypeInfo2MirType(sig.returnType), rrt);
				l.operands.add(state.getOperandForChild(-1, rrt));
			}

			int argOffset = 0;

			if (state.getProperty("CallType") == "MemberFunction")
			{
				auto a = state.loadIntoRegister(0, RegisterType::Pointer);
				l.operands.add(a);
				argOffset = 1;
			}

			for (int i = 0; i < sig.args.size(); i++)
			{
				auto isRef = sig.args[i].typeInfo.isRef();
				auto isComplexType = sig.args[i].typeInfo.getType() == Types::ID::Pointer;

				if (isComplexType && !isRef)
				{
					// We need to copy the object on the stack and use the stack pointer as argument
					auto argString = state.getProperty("Signature").fromFirstOccurrenceOf("(", false, false);

					for (int j = 0; j < i; j++)
						argString = SimpleTypeParser(argString).getTrailingString().fromFirstOccurrenceOf(",", false, false);

					SimpleTypeParser p(argString);

					size_t numBytes = 0;

					for (const auto&m : state.classTypes[p.getComplexTypeId().getIdentifier()])
						numBytes = m.offset + Types::Helpers::getSizeForType(TypeConverters::MirType2TypeId(m.type));

					auto stackPtr = state.registerManager.getAnonymousId(false);

					numBytes = state.allocateStack(stackPtr, numBytes, false);
					auto source = state.loadIntoRegister(i + argOffset, RegisterType::Pointer);
					state.emitMultiLineCopy(stackPtr, source, numBytes);
					l.operands.add(stackPtr);
				}
				else
				{
					if (isRef)
					{
						auto rt = state.getRegisterTypeForChild(i + argOffset);
						auto tp = state.getTypeForChild(i + argOffset);

						if (rt == RegisterType::Value && tp != MIR_T_P)
						{
							// We need to copy the value from the register to the stack


							auto mir_t = state.getTypeForChild(i + argOffset);

							auto stackPtr = state.registerManager.getAnonymousId(false);

							state.allocateStack(stackPtr, Types::Helpers::getSizeForType(TypeConverters::MirType2TypeId(mir_t)), false);

							TextLine mv(&state);
							mv.instruction = TypeConverters::MirTypeAndToken2InstructionText(mir_t, JitTokens::assign_);

							String dst;
							dst << TypeConverters::MirType2MirTextType(mir_t);
							dst << ":(" << stackPtr << ")";

							mv.operands.add(dst);
							mv.addChildAsValueOperand(i + argOffset);
							mv.appendComment("Move register to stack");
							mv.flush();
							
							auto registerToSpill = state.getOperandForChild(i + argOffset, RegisterType::Value);

							for (auto& l : state.localOperands)
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
								auto a = state.getOperandForChild(i + argOffset, RegisterType::Pointer);
								l.operands.add(a);
							}
							else
							{
								// make sure the adress is loaded into a register
								auto a = state.loadIntoRegister(i + argOffset, RegisterType::Pointer);
								l.operands.add(a);
							}
						}
					}
					else
					{
						auto a = state.loadIntoRegister(i + argOffset, RegisterType::Value);
						l.operands.add(a);
					}


				}
			}

			l.flush();
		}
		else
		{
			SimpleTypeParser p(state.getProperty("ObjectType"));
			auto type = p.getComplexTypeId().toString();

			ValueTree dataObject;

			for (const auto& s : state.dataList)
			{
				if (s["ID"].toString() == type)
				{
					dataObject = s;
					break;
				}
			}

			auto funcData = sig.createDataLayout(true);

			// This should only happen with unresolved functions
			jassert(!(bool)funcData["IsResolved"]);

			int numArgsAvailable = state.currentTree.getNumChildren();
			int numArgsDefined = funcData.getNumChildren();

			// all defaultable parameters should be resolved by the parser
			jassert(numArgsAvailable == numArgsDefined);

			for (int i = 0; i < jmin(numArgsAvailable, numArgsDefined); i++)
			{
				SimpleTypeParser p(funcData.getChild(i).getProperty("Type").toString());
				auto t = p.getTypeInfo();
				auto isRef = t.isRef() || t.getType() == Types::ID::Pointer;
				auto registerType = isRef ? RegisterType::Pointer : RegisterType::Value;
				auto a = state.loadIntoRegister(i, registerType);

				funcData.getChild(i).setProperty("Operand", a, nullptr);
			}

			if (auto il = state.inlinerManager.getInliner(fid))
			{
				TextLine c(&state);
				c.appendComment("Inlined function " + fid);
				c.flush();

				auto returnReg = il(&state, dataObject, funcData);

				if (returnReg.text.isNotEmpty())
				{
					state.registerCurrentTextOperand(returnReg.text, returnReg.type, returnReg.registerType);
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

	static Result ClassStatement(State& state)
	{
		auto members = StringArray::fromTokens(state.getProperty("MemberInfo"), "$", "");

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

		auto x = NamespacedIdentifier::fromString(state.getProperty("Type"));
		auto className = Identifier(TypeConverters::NamespacedIdentifier2MangledMirVar(x));

		state.registerClass(className, std::move(memberInfo));

		state.numCurrentlyParsedClasses++;
		state.processAllChildren();
		state.numCurrentlyParsedClasses--;

		// Nothing to do for now, maybe we need to parse the functions, yo...
		return Result::ok();
	}

	static Result Dot(State& state)
	{
		state.processChildTree(0);

		auto memberSymbol = TypeConverters::String2Symbol(state.getCurrentChild(1)["Symbol"].toString());

		auto classId = Identifier(TypeConverters::NamespacedIdentifier2MangledMirVar(memberSymbol.id.getParent()));
		auto memberId = memberSymbol.id.getIdentifier().toString();
		auto memberType = TypeConverters::Symbol2MirTextSymbol(memberSymbol);

		for (const auto& m : state.classTypes[classId])
		{
			if (m.id == memberId)
			{
				auto ptr = state.loadIntoRegister(0, RegisterType::Pointer);

				String p;

				TextLine offset(&state);
				offset.instruction = "add";
				offset.operands.add(ptr);
				offset.operands.add(ptr);
				offset.addImmOperand((int)m.offset);
				offset.appendComment(classId + "." + m.id);
				offset.flush();

				state.registerCurrentTextOperand(ptr, m.type, RegisterType::Pointer);
				break;
			}
		}

		return Result::ok();
	}

	static Result ThisPointer(State& state)
	{
		state.registerCurrentTextOperand("_this_", MIR_T_P, RegisterType::Value);

		return Result::ok();
	}

	static Result PointerAccess(State& state)
	{
		state.processChildTree(0);

		auto n = state.getOperandForChild(0, RegisterType::Raw);
		state.registerCurrentTextOperand(n, MIR_T_P, RegisterType::Value);

		return Result::ok();
	}
};

}
}