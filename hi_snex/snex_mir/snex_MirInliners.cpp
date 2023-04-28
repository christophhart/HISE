namespace snex {
namespace mir {
using namespace juce;

struct InlineState
{
	InlineState(State& state_, const ValueTree& d_, const ValueTree& f_) :
		state(state_),
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

	String deref(const String& pointerOperand, MIR_type_t type, int offset = 0) const
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

		if (offset != 0)
			s << String(offset);

		s << "(" << pointerOperand << ")";
		return s;
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
					return deref(argOp(0), mir_t, (int)m["offset"]);
				}
				else
				{
					State::MirTextLine l;
					auto id = state.getAnonymousId(false);
					l.localDef << "i64:" << id;
					l.instruction = "add";
					l.operands.add(id);
					l.operands.add(argOp(0));
					l.operands.add(m["offset"].toString());
					l.appendComment(data["ID"].toString() + "." + memberId);
					state.emitLine(l);

					return id;
				}
			}
		}

		throw String("member " + memberId + " not found");
	}

	State& state;
	ValueTree data;
	ValueTree function;
};

struct InlinerFunctions
{
	static TextOperand dyn__referTo_ppii(State& state, const ValueTree& data, const ValueTree& function)
	{
		InlineState obj(state, data, function);

		obj.dump();

		auto offset = obj.argOp(3).getIntValue();

		auto offsetReg = state.getAnonymousId(false);

		State::MirTextLine scale;
		scale.localDef << "i64:" << offsetReg;
		scale.instruction = "mul";
		scale.operands.add(offsetReg);
		scale.operands.add(obj.argOp(3));
		scale.operands.add(data.getProperty("ElementSize"));
		scale.appendComment("scale with element size");
		state.emitLine(scale);


		// Assign the data pointer with the offset
		State::MirTextLine dl;
		dl.instruction = "add";
		dl.operands.add(obj.memberOp("data"));
		dl.operands.add(obj.argOp(1));
		dl.operands.add(offsetReg);
		dl.appendComment("dyn.data");
		state.emitLine(dl);

		// set the size

		auto size = obj.argOp(2);

		if (size == "-1")
		{
			jassertfalse;
		}

		State::MirTextLine sl;
		sl.instruction = "mov";
		sl.operands.add(obj.memberOp("size"));
		sl.operands.add(size);
		sl.appendComment("dyn.size");
		state.emitLine(sl);

		return obj.flush(obj.argOp(0), RegisterType::Value);
	};

	static TextOperand dyn__size_i(State& state, const ValueTree& data, const ValueTree& function)
	{
		InlineState obj(state, data, function);

		auto s = obj.memberOp("size", RegisterType::Pointer);
		return obj.flush(s, RegisterType::Pointer);
	};
};

}
}