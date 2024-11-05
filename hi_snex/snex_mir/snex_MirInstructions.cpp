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
	DEFINE_ID(MemoryReference);
	DEFINE_ID(VectorOp);
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

			using SigListType = Array<std::tuple<NamespacedIdentifier, String>>;

			SigListType staticSignatures, memberSignatures;

			staticSignatures.add({ NamespacedIdentifier(), "int PolyHandler::getVoiceIndexStatic(void* voiceIndex)" });
			staticSignatures.add({ NamespacedIdentifier(), "int PolyHandler::getSizeStatic(void* voiceIndex)" });

			TypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
			{
				if (v.getType() == InstructionIds::VectorOp)
				{
					auto sig = TypeConverters::VectorOp2Signature(v);

					auto sd = TypeConverters::String2FunctionData(sig);
					auto sig2 = TypeConverters::FunctionData2MirTextLabel({}, sd);

					jassert(MirCompiler::resolve(sig2.getCharPointer().getAddress()) != nullptr);

					staticSignatures.addIfNotAlreadyThere({ NamespacedIdentifier(), sig });
				}
				if (v.getType() == InstructionIds::FunctionCall)
				{
					auto sig = v[InstructionPropertyIds::Signature].toString();
					
					if (!MirCompiler::isExternalFunction(sig))
					{
						// Check if there is a type agnostic external function with a void* argument
						auto sd = TypeConverters::String2FunctionData(sig);
						auto sig2 = sd.getSignature({}, false);

						if(MirCompiler::isExternalFunction(sig2))
							sig = sig2;
						else
							return false;
					}
						

					if (v[InstructionPropertyIds::CallType].toString() == "MemberFunction")
					{
						auto id = NamespacedIdentifier::fromString(v[InstructionPropertyIds::ObjectType].toString());
						memberSignatures.addIfNotAlreadyThere({ id, sig }); // we still need to pass in an empty object type
					}
						
					else
						staticSignatures.addIfNotAlreadyThere({ NamespacedIdentifier(), sig });
				}

				return false;
			});

			for (const auto& c : memberSignatures)
			{
				auto f = TypeConverters::String2FunctionData(std::get<1>(c));
				state.functionManager.addPrototype(&state, std::get<0>(c), f, true);
				state.emitSingleInstruction("import " + TypeConverters::FunctionData2MirTextLabel(std::get<0>(c), f));
			}
			for (const auto& c : staticSignatures)
			{
				auto f = TypeConverters::String2FunctionData(std::get<1>(c));
				auto isConsole = f.id.getParent().toString() == "Console";
				state.functionManager.addPrototype(&state, {}, f, isConsole);
				state.emitSingleInstruction("import " + TypeConverters::FunctionData2MirTextLabel(std::get<0>(c), f));
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

		auto fullSig = state[InstructionPropertyIds::Signature];

		auto f = TypeConverters::String2FunctionData(fullSig);

		auto classType = state.dataManager.getCurrentClassType();

		auto addObjectPtr = state.isParsingClass() && !f.returnType.isStatic();

		int returnBlockSize = (int)state.currentTree.getProperty(InstructionPropertyIds::ReturnBlockSize, -1);

		TextLine line(&state);

        state.functionManager.clearLocalVarDefinitions();
        
		if (state.functionManager.hasPrototype(classType, f))
		{
			line.label = state.functionManager.registerComplexTypeOverload(&state, classType, fullSig, addObjectPtr);
		}
		else
		{
			state.functionManager.addPrototype(&state, classType, f, addObjectPtr, returnBlockSize);
			line.label = TypeConverters::FunctionData2MirTextLabel(classType, f);
		}
		
		
		
		line.instruction = "func ";

		auto op = TypeConverters::TypeAndReturnBlockToReturnType(f.returnType, returnBlockSize);

		if (op.isNotEmpty())
			line.operands.add(op);

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
        
        for(const auto& a: f.args)
        {
            auto at = a.typeInfo;
            
            if(at.getType() == Types::ID::Integer &&
               !at.isRef())
            {
                // signextend 32bit integer arguments for internal
                // 64bit calculations...
                TextLine ext(&state, "ext32");
                auto id = argParent.getChildId(a.id.getIdentifier());
                
                auto aId = TypeConverters::NamespacedIdentifier2MangledMirVar(id);
                
                ext.operands.add(aId);
                ext.operands.add(aId);
                ext.flush();
            }
        }
        
        
		state.processChildTree(0);
		state.registerManager.endFunction();

		state.emitSingleInstruction("endfunc");
		state.emitSingleInstruction("export " + line.label);

		return Result::ok();
	}

	static Result ReturnStatement(State* state_)
	{
		auto& state = *state_;

		auto isVoid = state[InstructionPropertyIds::Type].contains("void"); // could be "void" or "static void"

		if (!isVoid)
		{
			auto &rm = state.registerManager;

			SimpleTypeParser p(state[InstructionPropertyIds::Type]);

			state.processAllChildren();

			TextLine line(&state);
			line.instruction = "ret";

			auto returnBlockSize = (int)state.currentTree.getProperty(InstructionPropertyIds::ReturnBlockSize, -1);

			if (returnBlockSize == -1)
			{
				auto shouldReturnPointer = p.getTypeInfo().isRef() || p.getTypeInfo().getType() == Types::ID::Pointer;

				line.operands.add(rm.loadIntoRegister(0, shouldReturnPointer ? RegisterType::Pointer : RegisterType::Value));
				line.flush();
			}
			else
			{
				state.registerManager.emitMultiLineCopy("return_block", rm.loadIntoRegister(0, RegisterType::Pointer), returnBlockSize);
				line.flush();
			}
		}
		else
		{
			state.processAllChildren(); // could still have destructors attached...
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
			
            if(state.currentTree.hasProperty("NumBytesToCopy"))
            {
                auto s = rm.loadIntoRegister(0, RegisterType::Pointer);
                auto t = rm.loadIntoRegister(1, RegisterType::Pointer);
                
                auto numBytes = (int)state.currentTree.getProperty("NumBytesToCopy", 0);
                
                rm.emitMultiLineCopy(t, s, numBytes);
                
                return Result::ok();
            }
            
			TextLine l(&state);

            auto t = rm.getTypeForChild(0);
            
			if (state[InstructionPropertyIds::First] == "1")
			{
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
                
                auto localVarName = rm.getOperandForChild(1, RegisterType::Raw);
                
                if(!state.functionManager.hasLocalVar(localVarName))
                {
                    l.localDef << TypeConverters::MirType2MirTextType(rm.getTypeForChild(1));
                    l.localDef << ":" << localVarName;
                    
                    state.functionManager.addLocalVarDefinition(localVarName);
                }
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
        
        // don't process child statements when not in a function
        // (because there is no global constructor functionality yet)
        if(state.isParsingFunction())
            state.processAllChildren();

		SimpleTypeParser p(state[InstructionPropertyIds::Type]);

		MirCodeGenerator cc(&state);

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
            auto ids = StringArray::fromTokens(state[InstructionPropertyIds::Ids], ",", "");
            
            ids.removeEmptyStrings();
            
            for(auto& id: ids)
            {
                if (!state.isParsingFunction())
                {
                    // global definition

                    Symbol s;
                    s.typeInfo = TypeInfo(Types::ID::Pointer, true, false);
                    s.id =  NamespacedIdentifier::fromString(id.trim());

                    state.dataManager.addGlobalData(s.id.toString(), state[InstructionPropertyIds::Type]);
                    
                    MemoryBlock mb;
                    mb.fromBase64Encoding(state[InstructionPropertyIds::InitValuesB64]);

                    int numQuadWords = (int)mb.getSize() / (int)sizeof(int64);

                    int64* d = reinterpret_cast<int64*>(mb.getData());

                    for (int i = 0; i < numQuadWords; i++)
                    {
                        TextLine l(&state);

                        if (i == 0)
                            l.label = s.id.toString();

                        l.instruction = "i64";
                        l.addRawOperand(String(d[i]));
                        l.flush();
                    }
                }
                else
                {
                    // stack definition
                    auto numBytes = (uint32)state[InstructionPropertyIds::NumBytes].getIntValue();
                    
                    auto name = NamespacedIdentifier::fromString(id.trim());
                    auto mn = TypeConverters::NamespacedIdentifier2MangledMirVar(name);

                    numBytes = rm.allocateStack(mn, numBytes, true);
                    auto hasDynamicInitialisation = state.currentTree.getNumChildren() != 0;

                    if (hasDynamicInitialisation)
                    {
                        if (!state.currentTree.hasProperty("InitValues"))
                        {
                            // Only one expression is supported at the moment...
                            jassert(state.currentTree.getNumChildren() == 1);
                            auto src = rm.loadIntoRegister(0, RegisterType::Pointer);
                            rm.emitMultiLineCopy(mn, src, numBytes);
                        }
                        else
                        {
                            InitValueParser p(state[InstructionPropertyIds::InitValues]);
                            
                            p.forEach([&](size_t offset, Types::ID type, const VariableStorage& f)
                            {
                                auto isExpression = type != Types::ID::Pointer && f.getType() == Types::ID::Pointer;

                                String value;
                                
                                if(type == Types::ID::Pointer && f.getType() == Types::ID::Pointer)
                                    return;
                                
                                DBG(mn);

                                if (isExpression)
                                {
                                    auto expressionIndex = reinterpret_cast<uint64_t>(f.getDataPointer());
                                    value = state.registerManager.loadIntoRegister((int)expressionIndex, RegisterType::Value);
                                }
                                else
                                {
                                    value = Types::Helpers::getCppValueString(f);
                                }

                                if (type == Types::ID::Integer)
                                    cc.mov(cc.deref<int>(mn, (int)offset), value);
                                else if (type == Types::ID::Float)
                                    cc.fmov(cc.deref<float>(mn, (int)offset), value);
                                else if (type == Types::ID::Double)
                                    cc.dmov(cc.deref<double>(mn, (int)offset), value);
                                else
                                    jassertfalse;
                            });
                        }
                    }
                    else
                    {
                        // static type copy with initialization values

                        MemoryBlock mb;
                        mb.fromBase64Encoding(state[InstructionPropertyIds::InitValuesB64]);

                        int numQuadWords = (int)mb.getSize() / (int)sizeof(int64);

                        int64* d = reinterpret_cast<int64*>(mb.getData());

                        auto p = mn;

                        for (int i = 0; i < numQuadWords; i++)
                        {
                            cc.mov(cc.deref<void*>(p, i * 8), String(d[i]));
                        }

                    }
                }
            }
            
			
		}



		return Result::ok();
	}

	static Result Subscript(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

		auto type = SimpleTypeParser(state[InstructionPropertyIds::ParentType], false).getComplexTypeId().toString();

		
		state.processChildTree(0);
		state.processChildTree(1);

		auto t = TypeConverters::String2Symbol(state[InstructionPropertyIds::ElementType]);

		auto mir_t = TypeConverters::TypeInfo2MirType(t.typeInfo);
		auto tn = TypeConverters::TypeInfo2MirTextType(t.typeInfo);

		//mir_t = MIR_T_I64;

		String self, idx_reg;

		MirCodeGenerator cc(&state);

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
		else
		{
			auto classType = SimpleTypeParser(state[InstructionPropertyIds::ParentType], true).getComplexTypeId().toString();

			auto t = state.inlinerManager.emitInliner(type + "_subscript", classType, "operator[]", { state.registerManager.getOperandForChild(0, RegisterType::Pointer), 
																									  state.registerManager.getOperandForChild(0, RegisterType::Pointer),
																									  state.registerManager.getOperandForChild(1, RegisterType::Value) });

			state.registerManager.registerCurrentTextOperand(t.text, t.type, t.registerType);

			return Result::ok();
		}

		TextLine idx(&state);
		idx_reg = idx.addAnonymousReg(MIR_T_I64, RegisterType::Value, false);
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

		state.processAllChildren(); // could have destructors attached...

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

		MirCodeGenerator cc(state_);
		

		String startLabel, endLabel, continueLabel;

		auto loopType = state[InstructionPropertyIds::LoopType];

		

		state.loopManager.pushLoopLabels(startLabel, endLabel, continueLabel);
		
		// create iterator variable with pointer type
		auto iteratorSymbol = TypeConverters::String2Symbol(state[InstructionPropertyIds::Iterator]);

		if (loopType == "CustomObject")
		{
			auto cType = state.dataManager.getDataObject(state[InstructionPropertyIds::ObjectType]);

			if (cType.isValid())
			{
				for (const auto& c : cType)
				{
					if (c.getType() == InstructionPropertyIds::Method && c[InstructionPropertyIds::ID] == "begin")
					{
						SimpleTypeParser p(c[InstructionPropertyIds::ReturnType]);

						iteratorSymbol.typeInfo = p.getTypeInfo();
						break;
					}
				}
			}
		}

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
		else if (loopType == "CustomObject")
		{
			auto objPtr = rm.loadIntoRegister(0, RegisterType::Pointer);

			auto className = state[InstructionPropertyIds::ObjectType];

			auto objectType = SimpleTypeParser(className, true).getComplexTypeId().toString();
			auto label = TypeConverters::TemplateString2MangledLabel(SimpleTypeParser(className, false).getComplexTypeId().toString());

			auto b = state.inlinerManager.emitInliner(label + "_begin_p", objectType, "begin", {objPtr});
			auto e = state.inlinerManager.emitInliner(label + "_size_i", objectType, "size", {objPtr});

			jassert(type == b.type);

			loadIter.operands.add(pointerReg);
			loadIter.operands.add(b.text);
			loadIter.appendComment("custom begin");
			loadIter.flush();

			TextLine ml(&state);
			ml.localDef << "i64:" << endReg;
			ml.instruction = "mul";
			ml.operands.add(endReg);
			ml.operands.add(e.text);
			ml.addImmOperand(elementSize);
			ml.flush();

			cc.add(endReg, endReg, pointerReg);
		}
		else
		{
			rm.registerCurrentTextOperand(mvn, type, isRef ? RegisterType::Pointer :
				RegisterType::Value);

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

			cc.add(endReg, endReg, pointerReg);
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
		cc.bge(endLabel, pointerReg, endReg);

		// emit body
		state.processChildTree(1);

        state.emitLabel(continueLabel);
        
		// bump address by element size after loop body
		cc.add(pointerReg, pointerReg, elementSize);
		cc.jmp(startLabel);

		// emit end_loop label
		cc.bind(endLabel);
		
		state.loopManager.popLoopLabels();

		return Result::ok();
	}

	static Result FunctionCall(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;
		state.processAllChildren();
		
		auto fullSig = state[InstructionPropertyIds::Signature];

		auto isBaseMethod = state.currentTree.hasProperty(InstructionPropertyIds::BaseObjectType);

		auto objectProperty = isBaseMethod ? InstructionPropertyIds::BaseObjectType :
										     InstructionPropertyIds::ObjectType;

		auto objectType = NamespacedIdentifier::fromString(state.currentTree.getProperty(objectProperty, "").toString());
		
		auto sig =  TypeConverters::String2FunctionData(fullSig);
		auto fid = state.functionManager.getIdForComplexTypeOverload(objectType, fullSig);

		if (state.functionManager.hasPrototype(objectType, sig))
		{
			auto protoType = state.functionManager.getPrototype(objectType, fullSig);

			TextLine l(&state);

			l.instruction = "inline";
			l.operands.add(protoType);
			l.operands.add(fid);

			if (sig.returnType.isValid())
			{
				auto returnBlockSize = (int)state.currentTree.getProperty(InstructionPropertyIds::ReturnBlockSize, -1);

				if (returnBlockSize == -1)
				{
					auto rrt = sig.returnType.isRef() ? RegisterType::Pointer : RegisterType::Value;
					l.addAnonymousReg(TypeConverters::TypeInfo2MirType(sig.returnType), rrt);
					l.operands.add(rm.getOperandForChild(-1, rrt));
				}
				else
				{
					MirCodeGenerator cc(state_);

					auto returnBlock = cc.alloca(returnBlockSize);

					TextLine movLine(state_, "mov");

					auto thisOp = movLine.addAnonymousReg(MIR_T_P, RegisterType::Value, true);
					
					movLine.addSelfAsValueOperand();
					movLine.addRawOperand(returnBlock);
					movLine.flush();

					l.operands.add(TypeConverters::StringOperand2ReturnBlock(thisOp, returnBlockSize));
				}
			}

			int argOffset = 0;

			auto callType = state[InstructionPropertyIds::CallType];

			if (callType.contains("MemberFunction"))
			{
				auto a = state.registerManager.loadIntoRegister(0, RegisterType::Pointer);

				if(state[InstructionPropertyIds::CallType] == ("BaseMemberFunction"))
				{
					MirCodeGenerator cc(state_);

					auto p = cc.newReg<void*>(a);

					auto offset = state[InstructionPropertyIds::BaseOffset].getIntValue();

					cc.setInlineComment("Adjust base class pointer");
					cc.add(p, a, offset);

					l.operands.add(p);
				}
				else
				{
					l.operands.add(a);
				}

				
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

					numBytes = rm.allocateStack(stackPtr, (int)numBytes, false);
					auto source = rm.loadIntoRegister(i + argOffset, RegisterType::Pointer);
					rm.emitMultiLineCopy(stackPtr, source, (int)numBytes);
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

							rm.allocateStack(stackPtr, (int)Types::Helpers::getSizeForType(TypeConverters::MirType2TypeId(mir_t)), false);

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

            if(type == "dyn<float>")
                type = "block";
            
			auto dataObject = state.dataManager.getDataObject(type);

            jassert(dataObject.isValid());
            
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

			auto fidWithoutObject = state.functionManager.getIdForComplexTypeOverload({}, fullSig);

			if (auto il = state.inlinerManager.getInliner(fidWithoutObject))
			{
				TextLine c(&state);
				c.appendComment("Inlined function " + fidWithoutObject);
				c.flush();

				funcData.setProperty(InstructionPropertyIds::Signature, fullSig, nullptr);

				auto returnReg = il(&state, dataObject, funcData);

				if (returnReg.text.isNotEmpty())
				{
					rm.registerCurrentTextOperand(returnReg.text, returnReg.type, returnReg.registerType);
				}

				state.emitLabel(state.loopManager.makeLabel(), "End of inlined function " + fid);

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
			

			auto symbol = TypeConverters::String2Symbol(m);

			auto v1 = TypeConverters::NamespacedIdentifier2MangledMirVar(symbol.id);
			auto v2 = TypeConverters::TypeInfo2MirType(symbol.typeInfo);
			auto v3 = m.fromFirstOccurrenceOf("(", false, false).getIntValue();

            memberInfo.add(MemberInfo(v1, v2, v3));
		}

		auto x = NamespacedIdentifier::fromString(state[InstructionPropertyIds::Type]);
		
		state.dataManager.startClass(x, std::move(memberInfo));
		state.processAllChildren();
		state.dataManager.endClass();
		
		// Nothing to do for now, maybe we need to parse the functions, yo...
		return Result::ok();
	}

	static Result Dot(State* state_)
	{
		auto& state = *state_;
		auto& rm = state.registerManager;

        auto isInlinedThis = [](const ValueTree& v)
        {
            return v.getType() == Identifier("InlinedParameter") &&
                   v[InstructionPropertyIds::Symbol].toString().contains(" this");
        };
        
        if(isInlinedThis(state.getCurrentChild(0)) &&
           state.getCurrentChild(1).getType() == Identifier("Dot") &&
           isInlinedThis(state.getCurrentChild(1).getChild(0)))
        {
            state.processChildTree(1);
            
            auto op = state.registerManager.getTextOperandForValueTree(state.getCurrentChild(1));
            
            state.registerManager.registerCurrentTextOperand(op.text, op.type, op.registerType);
            
            return Result::ok();
        }
        
		state.processChildTree(0);

		auto memberSymbol = TypeConverters::String2Symbol(state.getCurrentChild(1)[InstructionPropertyIds::Symbol].toString());

		auto classId = SimpleTypeParser(state[InstructionPropertyIds::ObjectType]).getComplexTypeId().toString();
		auto memberId = memberSymbol.id.getIdentifier().toString();
		auto memberType = TypeConverters::Symbol2MirTextSymbol(memberSymbol);

        const auto& members = state.dataManager.getClassType(classId);
        
		for (const auto& m : members)
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
        
        if(auto it = state.loopManager.getInlinedThis())
        {
            state.registerManager.registerCurrentTextOperand(it.text, it.type, it.registerType);
        }
        else
        {
            state.registerManager.registerCurrentTextOperand("_this_", MIR_T_P, RegisterType::Value);
        }
        
		

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
			state.loopManager.pushInlineFunction(il, mir_t, rt, name);
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
		
		auto n = state[InstructionPropertyIds::ParameterName];

		state.processChildTree(0);

		auto t = state.registerManager.getTextOperandForValueTree(state.getCurrentChild(0));
        
		state.loopManager.addInlinedArgument(n, t);

		return Result::ok();
	}

	static Result InlinedParameter(State* state_)
	{
		
		auto& state = *state_;

		auto arg = state[InstructionPropertyIds::Symbol];
		auto p = state.loopManager.getInlinedParameter(arg);

        state.registerManager.registerCurrentTextOperand(p.text, p.type, p.registerType);

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

	static Result MemoryReference(State* state_)
	{
		auto& state = *state_;

		state.processChildTree(0);

		TextLine l(&state, "mov");
		l.addSelfOperand<void*>(true); l.addChildAsPointerOperand(0);
		auto x = l.flush();

		TextLine a(&state, "add");
		a.addRawOperand(x); a.addRawOperand(x); a.addImmOperand(state[InstructionPropertyIds::Offset].getIntValue());
		a.flush();

		return Result::ok();
	}

	

	static Result VectorOp(State* state_)
	{
		MirCodeGenerator cc(state_);

		auto& state = *state_;
		state.dump();

		state.processAllChildren();

		auto fullSig = TypeConverters::VectorOp2Signature(state.currentTree);
		auto sig = TypeConverters::String2FunctionData(fullSig);
		auto fid = state.functionManager.getIdForComplexTypeOverload({}, fullSig);

		

		jassert(state.functionManager.hasPrototype({}, sig));

		auto protoType = state.functionManager.getPrototype({}, fullSig);

		

		auto dst = state.registerManager.getOperandForChild(1, RegisterType::Pointer);

		auto isSpanTarget = state.currentTree.hasProperty(InstructionPropertyIds::NumElements);

		if (isSpanTarget)
		{
			auto blockData = cc.alloca(16);
			cc.mov(cc.deref<void*>(blockData, 8), dst);
			cc.mov(cc.deref<int>(blockData, 4), state[InstructionPropertyIds::NumElements]);
			dst = blockData;
		}

		auto isScalar = state[InstructionPropertyIds::Scalar] == "1";

		auto src = state.registerManager.getOperandForChild(0, isScalar ? 
															   RegisterType::Value : 
															   RegisterType::Pointer);

		if (!isScalar && isSpanTarget)
		{
			auto blockData = cc.alloca(16);
			cc.mov(cc.deref<void*>(blockData, 8), src);
			cc.mov(cc.deref<int>(blockData, 4), state[InstructionPropertyIds::NumElements]);
			src = blockData;
		}

		cc.call<void*>({}, fullSig, { dst, src });

		return Result::ok();
	}


};

}
}
