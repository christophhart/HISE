namespace snex {
namespace mir {
using namespace juce;

#define INSTRUCTION2(x) template <typename T1> void x(const String& op1, const T1& op2) { emit(#x, _operands(op1, op2)); }
#define INSTRUCTION3(x) template <typename T1, typename T2> void x(const String& op1, const T1& op2, const T2& op3) { emit(#x, _operands(op1, op2, op3)); }

struct InlineState
{
	InlineState(State* state_, const ValueTree& d_, const ValueTree& f_) :
		state(*state_),
		rm(state_->registerManager),
		data(d_),
		function(f_)
	{};

	void dump() const
	{
		DBG("DATA:");
		DBG(data.createXml()->createDocument(""));
		DBG("FUNCTION");
		DBG(function.createXml()->createDocument(""));
	}

	String argOp(int index) const
	{
		return function.getChild(index).getProperty("Operand").toString();
	}

	TextOperand flush(const String& s, RegisterType rt) const
	{
		TextOperand ret;
		ret.text = s;
		ret.type = SimpleTypeParser(function["ReturnType"].toString()).getMirType(false);
		ret.registerType = rt;
		return ret;
	}

	int templateConstant(const String& templateId) const
	{
		for (const auto& d : data)
		{
			if (d.getType() == Identifier("TemplateParameter") &&
				d["ID"] == templateId)
			{
				jassert(d["ParameterType"].toString() == "Integer");
				return (int)d["Value"];
			}
		}

		jassertfalse;
		return -1;
	}

	template <typename T, typename OffsetType = int> String deref(const String& pointerOperand, int displacement = 0, const OffsetType& offset = {})
	{
		constexpr bool indexIsRegister = std::is_integral<OffsetType>::value;

		auto t = TypeConverters::getMirTypeFromT<T>();

		if constexpr (indexIsRegister)
		{
			return derefInternal(pointerOperand, t, displacement + sizeof(T) * offset, "");
		}
		else
		{
			return derefInternal(pointerOperand, t, displacement, offset, sizeof(T));
		}

		return {};
	}

	String derefInternal(const String& pointerOperand, MIR_type_t type, int displacement = 0, const String& offset = {}, int elementSize = 0) const
	{
		String s;

		if (type == MIR_T_I64)
			s << "i32:";
		if (type == MIR_T_F)
			s << "f:";
		if (type == MIR_T_D)
			s << "d:";
		if (type == MIR_T_P)
			s << "i64:";

		if (displacement != 0)
			s << String(displacement);

		s << "(" << pointerOperand;

		if (offset.isNotEmpty())
			s << ", " << offset;

		if (elementSize != 0)
			s << ", " << elementSize;

		s << ")";
		return s;
	}

	String dataProperty(const String& id)
	{
		return data.getProperty(id).toString();
	}

	String memberOp(const String& memberId, RegisterType rt = RegisterType::Value) const
	{
		for (const auto& m : data)
		{
			if (m.getType() == Identifier("Member") &&
				m["ID"].toString() == memberId)
			{
				auto mir_t = SimpleTypeParser(m["type"]).getMirType();

				if (rt == RegisterType::Value)
				{
					return derefInternal(argOp(0), mir_t, (int)m["offset"]);
				}
				else
				{
					TextLine l(&state);
					auto id = rm.getAnonymousId(false);
					l.localDef << "i64:" << id;
					l.instruction = "add";
					l.operands.add(id);
					l.operands.add(argOp(0));
					l.operands.add(m["offset"].toString());
					l.appendComment(data["ID"].toString() + "." + memberId);
					l.flush();

					return id;
				}
			}
		}

		throw String("member " + memberId + " not found");
	}

	String alloca(size_t numBytes)
	{
		auto blockReg = rm.getAnonymousId(false);
		rm.allocateStack(blockReg, numBytes, false);
		return blockReg;
	}

	void emit(const String& instruction, const StringArray& operands)
	{
		TextLine tl(&state, instruction);
		tl.operands = operands;

		if (nextComment.isNotEmpty())
		{
			tl.appendComment(nextComment);
			nextComment = {};
		}

		tl.flush();
	}

	template <typename T> String newReg(const String& source)
	{
		TextLine s1(&state, "mov");
		s1.addSelfOperand<T>(); s1.addRawOperand(source);
		return s1.flush();
	}

	void bind(const String& label, const String& comment = {})
	{
		state.emitLabel(label, comment);
	}

	void jmp(const String& op)
	{
		state.emitSingleInstruction("jmp " + op);
	}

	INSTRUCTION2(fmov);
	INSTRUCTION2(mov);
	INSTRUCTION3(mul);
	INSTRUCTION3(add);
	INSTRUCTION3(bne);
	INSTRUCTION3(bge);

	State& state;
	RegisterManager& rm;
	ValueTree data;
	ValueTree function;

	void setInlineComment(const String& s)
	{
		nextComment = s;
	}

	private:

	String nextComment;

	template <typename T1> StringArray _operands(const String& op1, const T1& op2)
	{
		StringArray sa;
		sa.add(op1);

		if constexpr (std::is_same<T1, juce::String>()) sa.add(op2);
		else										    sa.add(Types::Helpers::getCppValueString(VariableStorage(op2)));

		return sa;
	}

	template <typename T1, typename T2> StringArray _operands(const String& op1, const T1& op2, const T2& op3)
	{
		StringArray sa;
		sa.add(op1);

		if constexpr (std::is_same<T1, juce::String>()) sa.add(op2);
		else										    sa.add(Types::Helpers::getCppValueString(VariableStorage(op2)));

		if constexpr (std::is_same<T2, juce::String>()) sa.add(op3);
		else										    sa.add(Types::Helpers::getCppValueString(VariableStorage(op3)));

		return sa;
	}
};

#undef INSTRUCTION2;
#undef INSTRUCTION3;

#define DEFINE_INLINER(x) static TextOperand x(State* state, const ValueTree& data, const ValueTree& function)

struct InlinerFunctions
{
	DEFINE_INLINER(dyn_referTo_ppii)
	{
		InlineState cc(state, data, function);
		
		auto offset = cc.newReg<int>(cc.argOp(3));
		cc.mul(offset, offset, cc.dataProperty("ElementSize"));
		cc.add(cc.memberOp("data"), cc.argOp(1), offset);
		cc.mov(cc.memberOp("size"), cc.argOp(2));

		return cc.flush(cc.argOp(0), RegisterType::Pointer);
	};

	DEFINE_INLINER(dyn_size_i)
	{
		InlineState cc(state, data, function);

		auto s = cc.memberOp("size", RegisterType::Pointer);

		return cc.flush(s, RegisterType::Pointer);
	};

    DEFINE_INLINER(ProcessData_toChannelData_pp)
    {
        InlineState cc(state, data, function);
        
        auto blockReg = cc.alloca(16);
		cc.mov(cc.deref<int>(blockReg), 128);
		cc.mov(cc.deref<int>(blockReg, 4), cc.memberOp("numSamples"));
		cc.mov(cc.deref<void*>(blockReg, 8), cc.argOp(1));
        
        return cc.flush(blockReg, RegisterType::Pointer);
    }
    
	DEFINE_INLINER(ProcessData_toEventData_p)
	{
		InlineState cc(state, data, function);

		auto blockReg = cc.alloca(16);
		cc.mov(cc.deref<int>(blockReg), 128);
		cc.mov(cc.deref<int>(blockReg, 4), cc.memberOp("numEvents"));
		cc.mov(cc.deref<void*>(blockReg, 8), cc.memberOp("events"));

		return cc.flush(blockReg, RegisterType::Pointer);
	}
	
	DEFINE_INLINER(ProcessData_begin_p)
	{
		InlineState cc(state, data, function);

		auto reg = cc.newReg<void*>(cc.deref<void*>(cc.argOp(0)));

		return cc.flush(reg, RegisterType::Pointer);
	}

	DEFINE_INLINER(ProcessData_size_i)
	{
		InlineState cc(state, data, function);

		auto s = cc.memberOp("numChannels", RegisterType::Value);

		return cc.flush(s, RegisterType::Value);
	}

	DEFINE_INLINER(ProcessData_subscript)
	{
		InlineState cc(state, data, function);

		auto blockReg = cc.alloca(16);
		cc.mov(cc.deref<int>(blockReg),    128);
		cc.mov(cc.deref<int>(blockReg, 4), cc.memberOp("numSamples"));
		auto offset = cc.newReg<int>(cc.argOp(2));
		cc.mul(offset, offset, 8);
		auto dataPtr = cc.newReg<float**>(cc.memberOp("data"));
		cc.add(dataPtr, dataPtr, offset);
		cc.mov(cc.deref<void*>(blockReg, 8), cc.deref<void*>(dataPtr));
		
		return cc.flush(blockReg, RegisterType::Pointer);
	}

	DEFINE_INLINER(ProcessData_toFrameData_p)
	{
		InlineState cc(state, data, function);

		/* DATA LAYOUT FOR FRAME_PROCESSOR:
		span<float*, NumChannels>& channels; // 8 byte
		int frameLimit = 0;					 // 4 byte
		int frameIndex = 0;				     // 4 byte
		FrameType frameData;				 // sizeof(FrameData)
		*/

		auto NumChannels = cc.templateConstant("NumChannels");
		auto numBytes = 8 + 4 + 4 + sizeof(float) * NumChannels;

		auto fp = cc.alloca(numBytes);

		cc.setInlineComment("fp.channels");
		cc.mov(cc.deref<void*>(fp), cc.deref<void*>(cc.memberOp("data", RegisterType::Pointer)));
		
		cc.setInlineComment("fp.frameLimit");
		cc.mov(cc.deref<int>(fp, 8), cc.memberOp("numSamples"));

		cc.setInlineComment("fp.frameIndex");
		cc.mov(cc.deref<int>(fp, 12), 0);

		// float** channelData = &fp.channels;
		auto channelData = cc.newReg<float**>(cc.deref<void*>(fp));

		for (int i = 0; i < NumChannels; i++)
		{
			auto channel = cc.newReg<float*>(cc.deref<float*>(channelData, 0, i));

			cc.setInlineComment("frameData[ " + String(i) + "]");
			cc.fmov(cc.deref<float>(fp, 16, i), cc.deref<float>(channel));
		}

		return cc.flush(fp, RegisterType::Pointer);
	}

	DEFINE_INLINER(FrameProcessor_next_i)
	{
		InlineState cc(state, data, function);

		int numChannels = cc.templateConstant("NumChannels");

		// const int frameLimit = fp.frameLimit;
		auto frameLimit = cc.memberOp("frameLimit", RegisterType::Value);
		// int* frameIndex = &fp.frameIndex;
		auto frameIndexPtr = cc.memberOp("frameIndex", RegisterType::Pointer);
		auto frameIndex = cc.deref<int>(frameIndexPtr);

		// float* frameData = fp.frameData.begin();
		auto frameData = cc.memberOp("frameData", RegisterType::Pointer);

		auto exit = cc.state.loopManager.makeLabel();
		auto writeLastFrame = cc.state.loopManager.makeLabel();
		auto finished = cc.state.loopManager.makeLabel();

		// int returnValue = *frameIndex;
		auto returnValue = cc.newReg<int>(frameIndex);

		// if (fp->frameIndex != 0) goto writeLastFrame
		cc.bne(writeLastFrame, returnValue, 0);

		//++fp->frameIndex;
		cc.add(frameIndex, frameIndex, 1);

		//return fp->frameLimit;
		cc.mov(returnValue, frameLimit);

		cc.jmp(exit);
		cc.bind(writeLastFrame, "Write last frame");

		// float** channelPtrs = fp.channels;
		auto channelPtrs = cc.newReg<float**>(cc.memberOp("channels"));

		for (int i = 0; i < numChannels; i++)
		{
			auto channel = cc.newReg<float*>(cc.deref<float**>(channelPtrs, 0, i));
			
			cc.fmov(cc.deref<float>(channel, -4, returnValue), 
				    cc.deref<float>(frameData, 0, i));
		}
		
		// if(fp->frameIndex < fp->frameLimit) goto finished
		cc.setInlineComment("Load the next frame");
		cc.bge(finished, returnValue, frameLimit);

		for (int i = 0; i < numChannels; i++)
		{
			auto channel = cc.newReg<float*>(cc.deref<float**>(channelPtrs, 0, i));
			cc.fmov(cc.deref<float>(frameData, 0, i), cc.deref<float>(channel, 0, returnValue));
		}

		//	++fp->frameIndex;
		cc.add(frameIndex, frameIndex, 1);

		cc.mov(returnValue, 1);
		cc.jmp(exit);
		cc.bind(finished, "finished");

		cc.mov(returnValue, 0);
		cc.bind(exit, "exit");

		return cc.flush(returnValue, RegisterType::Value);
	}

	DEFINE_INLINER(FrameProcessor_begin_p)
	{
		InlineState cc(state, data, function);

		auto reg = cc.newReg<void*>(cc.memberOp("frameData", RegisterType::Pointer));
		return cc.flush(reg, RegisterType::Pointer);
	}

	DEFINE_INLINER(FrameProcessor_size_i)
	{
		InlineState cc(state, data, function);
		auto reg = cc.newReg<int>(String(cc.templateConstant("NumChannels")));
		return cc.flush(reg, RegisterType::Value);
	}
	
	DEFINE_INLINER(FrameProcessor_subscript)
	{
		InlineState cc(state, data, function);

		cc.dump();
		
		auto reg = cc.newReg<void*>(cc.argOp(1));

		cc.add(reg, reg, 16);

		auto idx = cc.newReg<int>(cc.argOp(2));

		cc.mul(idx, idx, 4);

		cc.add(reg, reg, idx);

		return cc.flush(reg, RegisterType::Pointer);
	}
};

#undef DEFINE_INLINER

}
}
