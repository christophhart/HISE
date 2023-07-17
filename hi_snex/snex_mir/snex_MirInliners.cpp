namespace snex {
namespace mir {
using namespace juce;

struct InlineCodeGenerator: public MirCodeGenerator
{
	InlineCodeGenerator(State* state_, const ValueTree& d_, const ValueTree& f_) :
		MirCodeGenerator(state_),
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

	String templateParameter(const String& templateId) const
	{
		for (const auto& d : data)
		{
			if (d.getType() == Identifier("TemplateParameter") &&
				d["ID"] == templateId)
			{
				jassert(d["ParameterType"].toString() == "Type");
				return d["Type"].toString();
			}
		}

		jassertfalse;
		return "";
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

	ValueTree data;
	ValueTree function;
};



#define DEFINE_INLINER(x) static TextOperand x(State* state, const ValueTree& data, const ValueTree& function)

struct InlinerFunctions
{
	/* dyn<T> ============================================================================================ */

	DEFINE_INLINER(dyn_referTo_ppii)
	{
		InlineCodeGenerator cc(state, data, function);
		
		auto fullSig = function[InstructionPropertyIds::Signature].toString();

		auto firstArg = fullSig.fromFirstOccurrenceOf("(", false, false);

		SimpleTypeParser p(firstArg);

		auto sourceType = p.getComplexTypeId().toString();
		
		auto isDyn = sourceType.startsWith("dyn");

		auto offset = cc.newReg<int>(cc.argOp(3));
		cc.mul(offset, offset, cc.dataProperty("ElementSize"));

		String source;

		if (isDyn)
			source = cc.deref<void*>(cc.argOp(1), 8);
		else
			source = cc.argOp(1);

		cc.add(cc.memberOp("data"), source, offset);
		cc.mov(cc.memberOp("size"), cc.argOp(2));

		return cc.flush(cc.argOp(0), RegisterType::Pointer);
	};

	DEFINE_INLINER(dyn_size_i)
	{
		InlineCodeGenerator cc(state, data, function);

		auto s = cc.memberOp("size", RegisterType::Pointer);

		return cc.flush(s, RegisterType::Pointer);
	};

	/* ProcessData<NumChannels> ============================================================================================ */

    DEFINE_INLINER(ProcessData_toChannelData_pp)
    {
        InlineCodeGenerator cc(state, data, function);
        
        auto blockReg = cc.alloca(16);
		cc.mov(cc.deref<int>(blockReg), 128);
		cc.mov(cc.deref<int>(blockReg, 4), cc.memberOp("numSamples"));
		cc.mov(cc.deref<void*>(blockReg, 8), cc.argOp(1));
        
        return cc.flush(blockReg, RegisterType::Pointer);
    }
    
	DEFINE_INLINER(ProcessData_toEventData_p)
	{
		InlineCodeGenerator cc(state, data, function);

		auto blockReg = cc.alloca(16);
		cc.mov(cc.deref<int>(blockReg), 128);
		cc.mov(cc.deref<int>(blockReg, 4), cc.memberOp("numEvents"));
		cc.mov(cc.deref<void*>(blockReg, 8), cc.memberOp("events"));

		return cc.flush(blockReg, RegisterType::Pointer);
	}
	
	DEFINE_INLINER(ProcessData_begin_p)
	{
		InlineCodeGenerator cc(state, data, function);

		auto reg = cc.newReg<void*>(cc.deref<void*>(cc.argOp(0)));

		return cc.flush(reg, RegisterType::Pointer);
	}

	DEFINE_INLINER(ProcessData_size_i)
	{
		InlineCodeGenerator cc(state, data, function);

		auto s = cc.memberOp("numChannels", RegisterType::Value);

		return cc.flush(s, RegisterType::Value);
	}

	DEFINE_INLINER(ProcessData_subscript)
	{
		InlineCodeGenerator cc(state, data, function);

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
		InlineCodeGenerator cc(state, data, function);

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

	/*  FrameProcessor<NumChannels> ============================================================================================ 
	
		Memory Layout

		span<float*, NumChannels>& channels; // void* 8 byte
		int frameLimit = 0;					 // int 4 byte
		int frameIndex = 0;				     // int 4 byte
		FrameType frameData;				 // span<float, NumChannels>
	*/
	DEFINE_INLINER(FrameProcessor_next_i)
	{
		InlineCodeGenerator cc(state, data, function);

		int numChannels = cc.templateConstant("NumChannels");

		// const int frameLimit = fp.frameLimit;
		auto frameLimit = cc.memberOp("frameLimit", RegisterType::Value);
		// int* frameIndex = &fp.frameIndex;
		auto frameIndexPtr = cc.memberOp("frameIndex", RegisterType::Pointer);
		auto frameIndex = cc.deref<int>(frameIndexPtr);

		// float* frameData = fp.frameData.begin();
		auto frameData = cc.memberOp("frameData", RegisterType::Pointer);

		auto exit = cc.newLabel();
		auto writeLastFrame = cc.newLabel();
		auto finished = cc.newLabel();

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
		InlineCodeGenerator cc(state, data, function);

		auto reg = cc.newReg<void*>(cc.memberOp("frameData", RegisterType::Pointer));
		return cc.flush(reg, RegisterType::Pointer);
	}

	DEFINE_INLINER(FrameProcessor_size_i)
	{
		InlineCodeGenerator cc(state, data, function);

		auto reg = cc.newReg<int>(String(cc.templateConstant("NumChannels")));

		return cc.flush(reg, RegisterType::Value);
	}
	
	DEFINE_INLINER(FrameProcessor_subscript)
	{
		InlineCodeGenerator cc(state, data, function);

		auto reg = cc.newReg<void*>(cc.argOp(1));
		cc.add(reg, reg, 16);
		auto idx = cc.newReg<int>(cc.argOp(2));
		cc.mul(idx, idx, 4);
		cc.add(reg, reg, idx);

		return cc.flush(reg, RegisterType::Pointer);
	}

	/*	PolyData<T, NumVoices> ============================================================================================ 

		Memory Layout

		voiceIndex		0, void*
		lastVoiceIndex  8, int
		unused,         12, int
		data,           16, span<T, NumVoices>
	*/

	DEFINE_INLINER(PolyData_get_i) { return PolyData_get_a(state, data, function); };
	DEFINE_INLINER(PolyData_get_f) { return PolyData_get_a(state, data, function); };
	DEFINE_INLINER(PolyData_get_d) { return PolyData_get_a(state, data, function); };
	DEFINE_INLINER(PolyData_get_p) { return PolyData_get_a(state, data, function); };

	DEFINE_INLINER(PolyData_get_a)
	{
		InlineCodeGenerator cc(state, data, function);

		auto dataOffset = 16;
		auto numVoices = cc.templateConstant("NumVoices");
		auto elementSize = (cc.dataProperty("NumBytes").getIntValue() - dataOffset) / numVoices;

		auto reg = cc.newReg<void*>(cc.memberOp("data", RegisterType::Pointer));

		if (numVoices != 1)
		{
			auto voicePtr = cc.newReg<PolyHandler*>(cc.memberOp("voiceIndex"));
			auto lastVoiceIndex = cc.call<int>({}, "int PolyHandler::getVoiceIndexStatic(void*)", { voicePtr });
			cc.mul(lastVoiceIndex, lastVoiceIndex, elementSize);
			cc.add(reg, reg, lastVoiceIndex);
		}

		return cc.flush(reg, RegisterType::Pointer);
	}

	DEFINE_INLINER(PolyData_prepare_vp)
	{
		InlineCodeGenerator cc(state, data, function);
		
		auto voicePtr = cc.deref<void*>(cc.argOp(1), offsetof(PrepareSpecs, voiceIndex));
		cc.mov(cc.memberOp("voiceIndex"), voicePtr);

		return cc.flush("", RegisterType::Pointer);
	}

	DEFINE_INLINER(PolyData_begin_p)
	{
		return PolyData_get_p(state, data, function);
	}

	DEFINE_INLINER(PolyData_size_i)
	{
		InlineCodeGenerator cc(state, data, function);

		auto numVoices = cc.templateConstant("NumVoices");
		auto voicePtr = cc.newReg<PolyHandler*>(cc.memberOp("voiceIndex"));
		auto size = cc.call<int>({}, "int PolyHandler::getSizeStatic(void*)", { voicePtr });
		cc.mul(size, size, numVoices-1);
		cc.add(size, size, 1);
		
		return cc.flush(size, RegisterType::Value);
	}
};

#undef DEFINE_INLINER

}
}
