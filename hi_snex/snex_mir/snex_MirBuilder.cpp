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



#include "src/mir.h"
#include "src/mir-gen.h"

namespace snex {
namespace jit {
using namespace juce;

#define DEFINE_ID(x) static const Identifier x(#x);

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
}

#undef DEFINE_ID

struct MirTypeConverters
{
	static bool forEachChild(const ValueTree& v, const std::function<bool(const ValueTree&)>& f)
	{
		if (f(v))
			return true;

		for (auto c : v)
		{
			if (forEachChild(c, f))
				return true;
		}

		return false;
	}

	static NamespacedIdentifier String2NamespacedIdentifier(const String& symbol)
	{
		return NamespacedIdentifier::fromString(symbol.fromFirstOccurrenceOf(" ", false, false));
	}

	static Types::ID MirType2TypeId(MIR_type_t t)
	{
		if (t == MIR_T_I64)
			return Types::ID::Integer;
		if (t == MIR_T_F)
			return Types::ID::Float;
		if (t == MIR_T_D)
			return Types::ID::Double;
		if (t == MIR_T_P)
			return Types::ID::Pointer;

		return Types::ID::Void;
	}

	static MIR_type_t TypeInfo2MirType(const TypeInfo& t)
	{
		auto m = t.getType();

		if (m == Types::ID::Integer)
			return MIR_T_I64;
		if (m == Types::ID::Float)
			return MIR_T_F;
		if (m == Types::ID::Double)
			return MIR_T_D;
		if (m == Types::ID::Pointer)
			return MIR_T_P;

		jassertfalse;
		return MIR_T_I8;
	}

	static TypeInfo String2TypeInfo(String t)
	{
		t = t.trim();

		auto isConst = t.startsWith("const");

		if (isConst)
			t = t.fromFirstOccurrenceOf("const", false, false).trim();

		auto isRef = t.endsWithChar('&');

		if (isRef)
			t = t.upToFirstOccurrenceOf("&", false, false).trim();

		auto type = Types::Helpers::getTypeFromTypeName(t);

		return TypeInfo(type, isConst, isRef);

	}

	static MIR_type_t String2MirType(String symbol)
	{
		if (symbol.contains(" "))
			symbol = symbol.upToFirstOccurrenceOf(" ", false, false);
		
		symbol = symbol.trim();

		if (symbol.containsAnyOf("*&"))
			return MIR_T_P;
		if (symbol == "int")
			return MIR_T_I64;
		if (symbol == "float")
			return MIR_T_F;
		if (symbol == "double")
			return MIR_T_D;

		jassertfalse;
		return MIR_T_I8;
	}

	static String NamespacedIdentifier2MangledMirVar(const NamespacedIdentifier& id)
	{
		auto n = id.toString();
		return n.replaceCharacters(":", "_");
	}

	static jit::Symbol String2Symbol(String symbolCode)
	{
		auto isConst = symbolCode.startsWith("const ");

		if (isConst)
			symbolCode = symbolCode.fromFirstOccurrenceOf("const ", false, false).trim();

		if (symbolCode.containsChar('>'))
		{
			
			symbolCode = symbolCode.fromLastOccurrenceOf(">", false, false).trim();

			auto isRef = symbolCode.startsWithChar('&');

			if (isRef)
				symbolCode = symbolCode.substring(1).trim();

			Symbol s;
			s.id = NamespacedIdentifier::fromString(symbolCode);
			s.typeInfo = TypeInfo(Types::ID::Pointer, true, isRef);
			return s;
		}
		else
		{
			auto isConst = symbolCode.trim().startsWith("const ");

			if (isConst)
				symbolCode = symbolCode.fromFirstOccurrenceOf("const ", false, false).trim();

			auto isRef = symbolCode.containsChar('&');

			if (isRef)
				symbolCode = symbolCode.removeCharacters("&").trim();

			auto t = symbolCode.upToFirstOccurrenceOf(" ", false, false);
			auto n = symbolCode.fromFirstOccurrenceOf(" ", false, false);
			auto ti = TypeInfo(Types::Helpers::getTypeFromTypeName(t));

			return Symbol(NamespacedIdentifier::fromString(n), ti);
		}
	}

	static MIR_var SymbolToMirVar(const jit::Symbol& s)
	{
		MIR_var t;
		t.name = s.id.getIdentifier().toString().getCharPointer().getAddress();
		t.size = -1;
		t.type = TypeInfo2MirType(s.typeInfo);
		return t;
	}

	static FunctionData String2FunctionData(String s)
	{
		auto retString = s.upToFirstOccurrenceOf(" ", false, false);
		auto rest = s.fromFirstOccurrenceOf(" ", false, false);

		auto name = rest.upToFirstOccurrenceOf("(", false, false);
		rest = rest.fromFirstOccurrenceOf("(", false, false);

		FunctionData f;
		f.id = NamespacedIdentifier(name);
		f.returnType = Types::Helpers::getTypeFromTypeName(retString);
		
		auto args = StringArray::fromTokens(rest.upToLastOccurrenceOf(")", false, false), ",", "");

		for (auto a : args)
		{
			auto s = String2Symbol(a);
			
			f.addArgs(s.id.getIdentifier(), s.typeInfo);
		}

		return f;
	}

	static int64_t getHash(const ValueTree& v)
	{
		auto ptr = v.getPropertyPointer(Identifier("Line"));

		jassert(ptr != nullptr);

		return reinterpret_cast<int64_t>(ptr);
	}

	static int64_t getHash(const NamespacedIdentifier& id)
	{
		return id.toString().hashCode64();
	}

	static String MirType2MirTextType(const MIR_type_t& t)
	{
		if (t == MIR_T_I64)
			return "i64";
		if (t == MIR_T_P)
			return "i64";
		if (t == MIR_T_D)
			return "d";
		if (t == MIR_T_F)
			return "f";

		throw String("Unknown type");
	}

	static String TypeInfo2MirTextType(const TypeInfo& t)
	{
		if (t.getType() == Types::ID::Integer)
			return "i64";
		if (t.getType() == Types::ID::Double)
			return "d";
		if (t.getType() == Types::ID::Float)
			return "f";
		if (t.getType() == Types::ID::Pointer ||
			t.getType() == Types::ID::Block)
			return "p";

		throw String("Unknown type");
	}

	static String MirTypeAndToken2InstructionText(MIR_type_t type, const String& token)
	{
		static StringPairArray intOps, fltOps, dblOps;

		intOps.set(JitTokens::assign_,				"MOV");
		intOps.set(JitTokens::plus,					"ADD");
		intOps.set(JitTokens::minus,				"SUB");
		intOps.set(JitTokens::times,				"MUL");
		intOps.set(JitTokens::divide,				"DIV");
		intOps.set(JitTokens::modulo,				"MOD");
		intOps.set(JitTokens::greaterThan,			"GT");
		intOps.set(JitTokens::greaterThanOrEqual, 	"GE");
		intOps.set(JitTokens::lessThan,				"LT");
		intOps.set(JitTokens::lessThanOrEqual,		"LE");
		intOps.set(JitTokens::equals,				"EQ");
		intOps.set(JitTokens::notEquals,			"NE");

		fltOps.set(JitTokens::assign_,				"FMOV");
		fltOps.set(JitTokens::plus,					"FADD");
		fltOps.set(JitTokens::minus,				"FSUB");
		fltOps.set(JitTokens::times,				"FMUL");
		fltOps.set(JitTokens::divide,				"FDIV");
		fltOps.set(JitTokens::greaterThan,			"FGT");
		fltOps.set(JitTokens::greaterThanOrEqual,	"FGE");
		fltOps.set(JitTokens::lessThan,				"FLT");
		fltOps.set(JitTokens::lessThanOrEqual,		"FLE");
		fltOps.set(JitTokens::equals,				"FEQ");
		fltOps.set(JitTokens::notEquals,			"FNE");
		
		dblOps.set(JitTokens::assign_,				"DMOV");
		dblOps.set(JitTokens::plus,					"DADD");
		dblOps.set(JitTokens::minus,				"DSUB");
		dblOps.set(JitTokens::times,				"DMUL");
		dblOps.set(JitTokens::divide,				"DDIV");
		dblOps.set(JitTokens::greaterThan,			"DGT");
		dblOps.set(JitTokens::greaterThanOrEqual,	"DGE");
		dblOps.set(JitTokens::lessThan,				"DLT");
		dblOps.set(JitTokens::lessThanOrEqual,		"DLE");
		dblOps.set(JitTokens::equals,				"DEQ");
		dblOps.set(JitTokens::notEquals,			"DNE");

		if (type == MIR_T_I64)	return intOps.getValue(token, "").toLowerCase();
		if (type == MIR_T_F)	return fltOps.getValue(token, "").toLowerCase();
		if (type == MIR_T_D)	return dblOps.getValue(token, "").toLowerCase();
		
		jassertfalse;
		return "";
	}
};

struct LoopManager
{
	~LoopManager()
	{
		jassert(labelPairs.isEmpty());
	}

	void pushLoopLabels(const String& startLabel, const String& endLabel)
	{
		labelPairs.add({ startLabel, endLabel });
	}

	String getCurrentLabel(const String& instructionId)
	{
		if (instructionId == "continue")
			return labelPairs.getLast().startLabel;
		else if (instructionId == "break")
			return labelPairs.getLast().endLabel;

		jassertfalse;
		return "";
	}

	void popLoopLabels()
	{
		labelPairs.removeLast();
	}

private:

	struct LoopLabelPair
	{
		String startLabel;
		String endLabel;
	};

	Array<LoopLabelPair> labelPairs;
};

struct State
{
	State() = default;

	mutable int counter = 0;
	mutable int labelCounter = 0;

	String makeLabel() const
	{
		return "L" + String(labelCounter++);
	}

	String getAnonymousId(bool isFloat) const
	{
		String s;
		s << (isFloat ? "xmm" : "reg") << String(counter++);
		return s;
	}

	using ValueTreeFuncton = std::function<Result(State& state)>;

	MIR_context_t ctx;
	MIR_module_t currentModule = nullptr;
	ValueTree currentTree;

	String getProperty(const Identifier& id) const
	{
		if (!currentTree.hasProperty(id))
		{
			dump();
			throw String("No property " + id.toString());
		}

		return currentTree[id].toString();
	}

	void dump() const
	{
		DBG(currentTree.createXml()->createDocument(""));
	}

	enum class RegisterType
	{
		Raw,
		Value,
		Pointer,
		numRegisterTypes
	};

	struct MirTextOperand
	{
		ValueTree v;
		String text;
		MIR_type_t type;
		RegisterType registerType = RegisterType::Value;
	};

	

	struct MirTextLine
	{
		String label;
		String localDef;
		String instruction;
		StringArray operands;
		String comment;

		String addAnonymousReg(State& state, MIR_type_t type, RegisterType rt)
		{
			auto id = state.getAnonymousId(MIR_fp_type_p(type));
			state.registerCurrentTextOperand(id, type, rt);

			auto t = TypeInfo(MirTypeConverters::MirType2TypeId(type), true);

			localDef << MirTypeConverters::TypeInfo2MirTextType(t) << ":" << id;
			return id;
		}

		void addImmOperand(const VariableStorage& value)
		{
			operands.add(Types::Helpers::getCppValueString(value));
		}

		void addOperands(State& s, const Array<int>& indexes, const Array<RegisterType>& registerTypes = {})
		{
			if (!registerTypes.isEmpty())
			{
				jassert(indexes.size() == registerTypes.size());

				for (int i = 0; i < indexes.size(); i++)
					operands.add(s.getOperandForChild(indexes[i], registerTypes[i]));
			}
			else
			{
				for (auto& i : indexes)
					operands.add(s.getOperandForChild(i, RegisterType::Value));
			}

		}

		int getLabelLength() const
		{
			return label.isEmpty() ? -1 : (label.length() + 2);
		}

		String toLine(int maxLabelLength) const
		{
			String s;

			if (label.isNotEmpty())
				s << label << ":" << " ";

			auto numToAdd = maxLabelLength - s.length();

			for (int i = 0; i < numToAdd; i++)
				s << " ";

			if (!localDef.isEmpty())
				s << "local " << localDef << "; ";

			s << instruction;

			if (!operands.isEmpty())
			{
				s << " ";

				auto numOps = operands.size();
				auto index = 0;

				for (auto& o : operands)
				{
					s << o;

					if (++index < numOps)
						s << ", ";
				}
			}

			if (comment.isNotEmpty())
				s << " # " << comment;

			return s;
		}

		void appendComment(const String& c) { comment = c; }
	};

	Array<MirTextLine> lines;

	void emitSingleInstruction(const String& instruction, const String& label={})
	{
		MirTextLine l;
		l.instruction = instruction;
		l.label = label;
		lines.add(l);
	}

	LoopManager loopManager;

	String nextLabel;

	/** emmit noop will add a dummy op that will prevent a compile error if the next statement has a local definition. */
	void emitLabel(const String& label, bool emitNoop)
	{
		nextLabel = label;

		if (emitNoop)
		{
			MirTextLine noop;
			noop.instruction = "bt";
			noop.operands.add(label);
			noop.addImmOperand(0);
			noop.appendComment("noop");
			emitLine(noop);
		}
	}

	void emitLine(MirTextLine& l)
	{
		if (nextLabel.isNotEmpty())
		{
			l.label = nextLabel;
			nextLabel = "";
		}
		lines.add(l);
	}

	String toString(bool addTabs)
	{
		String s;

		if (!addTabs)
		{
			for (const auto& l : lines)
				s << l.toLine(-1) << "\n";
		}
		else
		{
			int maxLabelLength = -1;

			for (const auto& l : lines)
				maxLabelLength = jmax(maxLabelLength, l.getLabelLength());

			for (const auto& l : lines)
				s << l.toLine(maxLabelLength) << "\n";
		}

		return s;
	}

	void registerCurrentTextOperand(const String& n, MIR_type_t type, RegisterType rt)
	{
		textOperands.add({ currentTree, n, type, rt });
	}

	ValueTree getCurrentChild(int index)
	{
		if (index == -1)
			return currentTree;
		else
			return currentTree.getChild(index);
	}

	RegisterType getRegisterTypeForChild(int index)
	{
		auto c = getCurrentChild(index);

		for (const auto& t : textOperands)
		{
			if (t.v == c)
				return t.registerType;
		}

		throw String("not found");
	}

	MIR_type_t getTypeForChild(int index)
	{
		auto c = getCurrentChild(index);

		for (const auto& t : textOperands)
		{
			if (t.v == c)
				return t.type;
		}

		throw String("not found");
	}

	bool isParsingFunction = false;

	String getOperandForChild(int index, RegisterType requiredType)
	{
		auto c = getCurrentChild(index);

		for (const auto& t : textOperands)
		{
			if (t.v == c)
			{
				if (t.registerType == RegisterType::Pointer && requiredType == RegisterType::Value)
				{
					MirTextLine load;

					load.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(t.type, JitTokens::assign_);
					
					auto l = load.addAnonymousReg(*this, t.type, RegisterType::Value);

					load.operands.add(l);
					load.operands.add(t.text);

					auto vt = MirTypeConverters::MirType2MirTextType(t.type);

					emitLine(load);

					return vt + ":(" + l + ")";
				}
					
				else
					return t.text;
			}
		}

		throw String("not found");
	}

	Array<MirTextOperand> textOperands;

	bool skipChildren = false;

	void registerFunction(const Identifier& id, const ValueTreeFuncton& f)
	{
		functions.emplace(id, f);
	}

	void processChildTree(int childIndex)
	{
		ScopedValueSetter<ValueTree> svs(currentTree, currentTree.getChild(childIndex));
		auto ok = processTreeElement(currentTree);

		if (ok.failed())
			throw ok.getErrorMessage();
	}

	void processAllChildren()
	{
		for (int i = 0; i < currentTree.getNumChildren(); i++)
			processChildTree(i);
	}

	Result processTreeElement(const ValueTree& v)
	{
		try
		{
			if (auto f = functions[v.getType()])
			{
				currentTree = v;
				return f(*this);
			}
		}
		catch (String& e)
		{
			return Result::fail(e);
		}

		return Result::fail("unknown value tree type " + v.getType());
	}

	std::map<juce::Identifier, ValueTreeFuncton> functions;
};


struct InstructionParsers
{
	static Result SyntaxTree(State& state)
	{
		auto isRoot = !state.currentTree.getParent().isValid();

		if (isRoot)
			state.emitSingleInstruction("module", "main");

		state.processAllChildren();

		if(isRoot)
			state.emitSingleInstruction("endmodule");
		
		return Result::ok();
	}

	static Result Function(State& state)
	{
		auto f = MirTypeConverters::String2FunctionData(state.getProperty("Signature"));

		State::MirTextLine line;
		line.label = f.id.getIdentifier().toString();
		line.instruction = "func ";
		line.instruction << MirTypeConverters::TypeInfo2MirTextType(f.returnType) << ", ";

		for (auto& a : f.args)
		{
			String arg;
			arg << MirTypeConverters::TypeInfo2MirTextType(a.typeInfo) << ":" <<
				MirTypeConverters::NamespacedIdentifier2MangledMirVar(a.id);

			line.operands.add(arg);
		}

		state.emitLine(line);

		state.isParsingFunction = true;
		state.processChildTree(0);
		state.isParsingFunction = false;

		state.emitSingleInstruction("endfunc");

		return Result::ok();
	}

	static Result ReturnStatement(State& state)
	{
		state.processChildTree(0);

		State::MirTextLine line;
		line.instruction = "ret";
		line.addOperands(state, { 0 }, { State::RegisterType::Value });

		state.emitLine(line);

		return Result::ok();
	}

	static Result BinaryOp(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		State::MirTextLine line;

		auto type = state.getTypeForChild(0);
		auto opType = state.getProperty("OpType");

		line.addAnonymousReg(state, type, State::RegisterType::Value);
		line.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, opType);
		line.addOperands(state, { -1, 0, 1 });

		state.emitLine(line);

		return Result::ok();
	}

	static Result VariableReference(State& state)
	{
		auto s = MirTypeConverters::String2Symbol(state.getProperty("Symbol"));

		auto type = MirTypeConverters::SymbolToMirVar(s).type;

		if (state.isParsingFunction)
		{
			auto mvn = MirTypeConverters::NamespacedIdentifier2MangledMirVar(s.id);

			for (auto& t : state.textOperands)
			{
				if (t.text == mvn)
				{
					auto copy = t;
					copy.v = state.currentTree;
					state.textOperands.add(copy);

					return Result::ok();
				}
			}

			state.registerCurrentTextOperand(mvn, type, State::RegisterType::Value);
		}
		else
		{
			state.registerCurrentTextOperand(MirTypeConverters::NamespacedIdentifier2MangledMirVar(s.id), type, State::RegisterType::Pointer);
		}

		return Result::ok();
	}

	static Result Immediate(State& state)
	{
		auto v = state.getProperty("Value");
		auto id = Types::Helpers::getTypeFromStringValue(v);
		auto type = MirTypeConverters::TypeInfo2MirType(TypeInfo(id, false, false));
		state.registerCurrentTextOperand(v, type, State::RegisterType::Value);

		return Result::ok();
	}

	static Result Cast(State& state)
	{
		state.processChildTree(0);

		auto source = MirTypeConverters::String2MirType(state.getProperty("Source"));
		auto target = MirTypeConverters::String2MirType(state.getProperty("Target"));

		String x;

		if (source == MIR_T_I64 && target == MIR_T_F)	   x = "I2F";
		else if (source == MIR_T_F && target == MIR_T_I64) x = "F2I";
		else if (source == MIR_T_I64 && target == MIR_T_D) x = "I2D";
		else if (source == MIR_T_D && target == MIR_T_I64) x = "D2I";
		else if (source == MIR_T_D && target == MIR_T_F)   x = "D2F";
		else if (source == MIR_T_F && target == MIR_T_D)   x = "F2D";

		State::MirTextLine line;

		line.addAnonymousReg(state, target, State::RegisterType::Value);
		line.instruction = x.toLowerCase();
		line.addOperands(state, { -1, 0 });

		state.emitLine(line);

		return Result::ok();
	}

	static Result Assignment(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		if (state.isParsingFunction)
		{
			auto opType = state.getProperty("AssignmentType");
			auto t = state.getTypeForChild(0);

			State::MirTextLine l;

			if (state.getProperty("First") == "1")
			{
				l.localDef << MirTypeConverters::MirType2MirTextType(state.getTypeForChild(1));
				l.localDef << ":" << state.getOperandForChild(1, State::RegisterType::Raw);
			}

			//l.addAnonymousReg(state, t, State::RegisterType::Value);

			l.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(t, opType);

			if (opType == JitTokens::assign_)
				l.addOperands(state, { 1, 0 });
			else
				l.addOperands(state, { 1, 1, 0 });

			state.emitLine(l);
		}
		else
		{
			State::MirTextLine e;
			e.instruction = "export";
			e.addOperands(state, { 1 }, { State::RegisterType::Raw }); 
			state.emitLine(e);
			State::MirTextLine l;
			l.label = state.getOperandForChild(1, State::RegisterType::Raw);
			l.instruction = MirTypeConverters::MirType2MirTextType(state.getTypeForChild(1));
			l.instruction;
			l.addOperands(state, { 0 });
			l.appendComment("global def ");

			state.emitLine(l);
		}

		return Result::ok();
	}

	static Result Comparison(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		auto opType = state.getProperty("OpType");
		auto type = state.getTypeForChild(0);

		State::MirTextLine l;
		l.addAnonymousReg(state, type, State::RegisterType::Value);
		l.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, opType);
		l.addOperands(state, { -1, 0, 1 });

		state.emitLine(l);

		return Result::ok();
	}

	static Result TernaryOp(State& state)
	{
		auto trueLabel = state.makeLabel();
		auto falseLabel = state.makeLabel();
		auto endLabel = state.makeLabel();

		// emit the condition
		state.processChildTree(0);

		// jump to false if condition == 0
		State::MirTextLine jumpToFalse;
		jumpToFalse.instruction = "bf";
		jumpToFalse.operands.add(falseLabel);
		jumpToFalse.addOperands(state, { 0 }, { State::RegisterType::Value });
		state.emitLine(jumpToFalse);

		// emit the true branch
		state.processChildTree(1);

		auto registerType = state.getRegisterTypeForChild(1);
		auto type = state.getTypeForChild(1);

		State::MirTextLine tl;
		tl.addAnonymousReg(state, type, registerType);
		state.emitLine(tl);

		State::MirTextLine movLine_t;
		movLine_t.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
		movLine_t.addOperands(state, { -1, 1 }, { registerType, registerType });
		state.emitLine(movLine_t);

		state.emitSingleInstruction("jmp " + endLabel);

		// emit the false branch
		state.emitLabel(falseLabel, true);
		state.processChildTree(2);

		State::MirTextLine movLine_f;
		movLine_f.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
		movLine_f.addOperands(state, { -1, 2 }, { registerType, registerType });
		state.emitLine(movLine_f);

		state.emitLabel(endLabel, true);

		return Result::ok();
	}

	static Result IfStatement(State& state)
	{
		auto hasFalseBranch = state.currentTree.getNumChildren() == 3;

		auto falseLabel = hasFalseBranch ? state.makeLabel() : "";

		auto endLabel = state.makeLabel();

		state.processChildTree(0);

		// jump to false if condition == 0
		State::MirTextLine jumpToFalse;
		jumpToFalse.instruction = "bf";
		jumpToFalse.operands.add(hasFalseBranch ? falseLabel : endLabel);
		jumpToFalse.addOperands(state, { 0 }, { State::RegisterType::Value });
		state.emitLine(jumpToFalse);

		state.processChildTree(1);

		if (hasFalseBranch)
		{
			state.emitSingleInstruction("jmp " + endLabel);
			state.emitLabel(falseLabel, true);
			state.processChildTree(2);
		}

		state.emitLabel(endLabel, true);

		return Result::ok();
	}

	static Result LogicalNot(State& state)
	{
		state.processChildTree(0);

		State::MirTextLine l;
		l.addAnonymousReg(state, MIR_T_I64, State::RegisterType::Value);
		l.instruction = "eq";
		l.addOperands(state, { -1, 0 });
		l.addImmOperand(VariableStorage(0));

		state.emitLine(l);
		
		return Result::ok();
	}

	static Result WhileLoop(State& state)
	{
		auto cond_label = state.makeLabel();
		auto end_label = state.makeLabel();
		
		state.loopManager.pushLoopLabels(cond_label, end_label);

		state.emitLabel(cond_label, true);
		
		state.processChildTree(0);

		State::MirTextLine jumpToEnd;
		jumpToEnd.instruction = "bf";
		jumpToEnd.operands.add(end_label);
		jumpToEnd.addOperands(state, { 0 }, { State::RegisterType::Value });
		state.emitLine(jumpToEnd);

		state.processChildTree(1);

		state.emitSingleInstruction("jmp " + cond_label);

		state.emitLabel(end_label, true);

		state.loopManager.popLoopLabels();

		return Result::ok();
	}

	static Result Increment(State& state)
	{
		state.processChildTree(0);

		auto isPre = state.getProperty("IsPre") == "1";
		auto isDec = state.getProperty("IsDec") == "1";

		State::MirTextLine mov_l;
		mov_l.addAnonymousReg(state, MIR_T_I64, State::RegisterType::Value);
		mov_l.instruction = "mov";
		mov_l.addOperands(state, { -1, 0 });

		State::MirTextLine add_l;
		add_l.instruction = "add";
		add_l.addOperands(state, { 0, 0 });
		add_l.addImmOperand(VariableStorage(isDec ? -1 : 1));

		if (isPre)
		{
			state.emitLine(add_l);
			state.emitLine(mov_l);
		}
		else
		{
			state.emitLine(mov_l);
			state.emitLine(add_l);
		}

		return Result::ok();
	}

	static Result StatementBlock(State& state)
	{
		Array<Symbol> localSymbols;

		auto scopeId = NamespacedIdentifier::fromString(state.getProperty("ScopeId"));

		MirTypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
		{
			if (v.getType() == InstructionIds::VariableReference)
			{
				auto vSymbol = MirTypeConverters::String2Symbol(v.getProperty("Symbol").toString());

				if (vSymbol.id.getParent() == scopeId)
				{
					localSymbols.add(vSymbol);
				}
			}

			return false;
		});

		State::MirTextLine local_def_l;

		for (auto& l : localSymbols)
		{
			auto type = MirTypeConverters::SymbolToMirVar(l).type;
			auto id = MirTypeConverters::NamespacedIdentifier2MangledMirVar(l.id);

			String ld;
			ld << "local " << MirTypeConverters::MirType2MirTextType(type) << ":" << id;

			local_def_l.operands.add(ld);
		}

		state.emitLine(local_def_l);

		state.processAllChildren();

		return Result::ok();
	}

	static Result ComplexTypeDefinition(State& state)
	{
		state.processAllChildren();

		auto b64 = state.getProperty("InitValues");

		InitValueParser initParser(b64);

		if (!state.isParsingFunction)
		{
			// global definition

			Symbol s;
			s.typeInfo = TypeInfo(Types::ID::Pointer, true, false);
			s.id = NamespacedIdentifier::fromString(state.getProperty("Ids").upToFirstOccurrenceOf(",", false, false).trim());

			initParser.forEach([&](uint32 offset, Types::ID type, const VariableStorage& v)
			{
				State::MirTextLine l;

				if (offset == 0)
				{
					l.label = s.id.toString();
				}

				l.instruction = MirTypeConverters::TypeInfo2MirTextType(TypeInfo(type));

				// use 4 bytes for integer initialisation...
				if (l.instruction == "i64")
					l.instruction = "i32";

				if (v.getType() == Types::ID::Pointer)
				{
					auto childIndex = (int)reinterpret_cast<int64_t>(v.getDataPointer());
					l.addOperands(state, { childIndex }, { State::RegisterType::Value });
				}
				else
					l.addImmOperand(v);

				state.emitLine(l);
			});
		}
		else
		{
			// stack definition

			State::MirTextLine l;

			auto name = NamespacedIdentifier::fromString(state.getProperty("Ids").upToFirstOccurrenceOf(",", false, false).trim());

			auto mn = MirTypeConverters::NamespacedIdentifier2MangledMirVar(name);

			state.registerCurrentTextOperand(mn, MIR_T_I64, State::RegisterType::Pointer);

			l.localDef << "i64:" << mn;

			l.instruction = "alloca";
			l.addOperands(state, { -1 }, { State::RegisterType::Raw });
			l.addImmOperand((int)initParser.getNumBytesRequired());

			state.emitLine(l);

			initParser.forEach([&](uint32 offset, Types::ID type, const VariableStorage& v)
			{
				State::MirTextLine il;
				auto t = MirTypeConverters::TypeInfo2MirType(TypeInfo(type));
				il.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(t, JitTokens::assign_);

				auto p = state.getOperandForChild(-1, State::RegisterType::Raw);

				String op;
				op << MirTypeConverters::MirType2MirTextType(t) << ":";
				if (offset != 0)
					op << String(offset);
				op << "(" << p << ")";

				il.operands.add(op);

				if (v.getType() == Types::ID::Pointer)
				{
					auto childIndex = (int)reinterpret_cast<int64_t>(v.getDataPointer());
					il.addOperands(state, { childIndex }, { State::RegisterType::Value });
				}
				else
					il.addImmOperand(v);

				state.emitLine(il);
			});
		}

		return Result::ok();
	}

	static Result Subscript(State& state)
	{
		state.processChildTree(0);
		state.processChildTree(1);

		auto t = MirTypeConverters::String2Symbol(state.getProperty("ElementType"));

		auto mir_t = MirTypeConverters::TypeInfo2MirType(t.typeInfo);

		if (mir_t == MIR_T_P)
			mir_t = MIR_T_I64;

		State::MirTextLine l;
		auto self = l.addAnonymousReg(state, mir_t, State::RegisterType::Pointer);
		l.instruction = "mov";

		l.addOperands(state, { -1, 0 }, { State::RegisterType::Pointer, State::RegisterType::Pointer });

		String comment;
		comment << self << " = " << state.getOperandForChild(0, State::RegisterType::Raw) << "[" << state.getOperandForChild(1, State::RegisterType::Raw) << "]";

		l.appendComment(comment);

		state.emitLine(l);

		State::MirTextLine idx;

		auto idx_reg = idx.addAnonymousReg(state, MIR_T_I64, State::RegisterType::Value);

		idx.instruction = "mov";
		idx.operands.add(idx_reg);
		idx.addOperands(state, { 1 });

		auto scale = state.getProperty("ElementSize").getIntValue();

		state.emitLine(idx);

		State::MirTextLine scaleLine;

		scaleLine.instruction = "mul";
		scaleLine.operands.add(idx_reg);
		scaleLine.operands.add(idx_reg);
		scaleLine.addImmOperand(scale);

		state.emitLine(scaleLine);

		State::MirTextLine offset_l;
		offset_l.instruction = "add";
		offset_l.operands.add(self);
		offset_l.operands.add(self);
		offset_l.operands.add(idx_reg);

		state.emitLine(offset_l);

		return Result::ok();
	}

	static Result InternalProperty(State& state)
	{
		return Result::ok();
	}

	static Result ControlFlowStatement(State& state)
	{
		auto command = state.getProperty("command");

		auto jumpLabel = state.loopManager.getCurrentLabel(command);

		State::MirTextLine l;
		l.instruction = "jmp";
		l.operands.add(jumpLabel);
		l.appendComment(command);

		state.emitLine(l);

		return Result::ok();
	}
};



#define REGISTER_TYPE(X) currentState->registerFunction(InstructionIds::X, InstructionParsers::X);

MirBuilder::MirBuilder(MIR_context* ctx_, const ValueTree& v_) :
	root(v_)
{
	/** Rework the logic:
	
	- allow better control over execution of child statements:

	if(state.isPre())
	{
		// do something;

		state.emitChild(0);
		
		state.emitInstruction("jnz L2");
		

		// do somethingElse

		state.emitChild(1);

		state.emitInstruction("jmp L3");
		state.emitLabel("L2);

		state.appendLabel(1);

		state.emitChild(2);

		state.emitLabel("L3");
	}
	
	*/
	
	currentState = new State();

	currentState->ctx = ctx_;

	REGISTER_TYPE(SyntaxTree);
	REGISTER_TYPE(Function);
	REGISTER_TYPE(ReturnStatement);
	REGISTER_TYPE(BinaryOp);
	REGISTER_TYPE(VariableReference);
	REGISTER_TYPE(Immediate);
	REGISTER_TYPE(Cast);
	REGISTER_TYPE(Assignment);
	REGISTER_TYPE(Comparison);
	REGISTER_TYPE(TernaryOp);
	REGISTER_TYPE(IfStatement);
	REGISTER_TYPE(LogicalNot);
	REGISTER_TYPE(WhileLoop);
	REGISTER_TYPE(Increment);
	REGISTER_TYPE(StatementBlock);
	REGISTER_TYPE(ComplexTypeDefinition);
	REGISTER_TYPE(Subscript);
	REGISTER_TYPE(InternalProperty);
	REGISTER_TYPE(ControlFlowStatement);
}

#undef REGISTER_TYPE

MirBuilder::~MirBuilder()
{
	delete currentState;
}

juce::Result MirBuilder::parse()
{
	return parseInternal(root);
}

MIR_module* MirBuilder::getModule() const
{
	return currentState->currentModule;
}

String MirBuilder::getMirText() const
{
	auto text = currentState->toString(true);
	DBG(text);

	return text;
}

juce::Result MirBuilder::parseInternal(const ValueTree& v)
{
	return currentState->processTreeElement(v);
}

bool MirBuilder::checkAndFinish(const Result& r)
{
	if (r.failed())
	{
		return true;
	}

	return r.failed();
}

}
}
