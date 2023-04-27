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
    DEFINE_ID(Loop);
	DEFINE_ID(FunctionCall);
	DEFINE_ID(ClassStatement);
	DEFINE_ID(Dot);
	DEFINE_ID(ThisPointer);
	DEFINE_ID(PointerAccess);
}

#undef DEFINE_ID

struct SimpleTypeParser
{
	SimpleTypeParser(const String& s)
	{
		code = s;
		parse();
	}

	MIR_type_t getMirType() const
	{
		if (isRef)
			return MIR_T_P;
		else
		{
			if (t == Types::ID::Integer)
				return MIR_T_I64;
			if (t == Types::ID::Float)
				return MIR_T_F;
			if (t == Types::ID::Double)
				return MIR_T_D;

			return MIR_T_I8;
		}
	}

	TypeInfo getTypeInfo() const
	{
		return TypeInfo(t, isConst, isRef, isStatic);
	}

	String getTrailingString() const
	{
		return code;
	}

	NamespacedIdentifier parseNamespacedIdentifier()
	{
		return NamespacedIdentifier::fromString(parseIdentifier());
	}

	NamespacedIdentifier getComplexTypeId()
	{
		jassert(t == Types::ID::Pointer);
		return NamespacedIdentifier::fromString(typeId);
	}

private:

	

	void skipWhiteSpace()
	{
		auto start = code.begin();
		auto end = code.end();

		while (start != end)
		{
			if (CharacterFunctions::isWhitespace(*start))
				start++;
			else
				break;
		}

		if(start != code.begin())
			code = String(start, end);
	}

	String parseIdentifier()
	{
		skipWhiteSpace();

		auto start = code.begin();
		auto end = code.end();
		auto ptr = start;

		while (ptr != end)
		{
			auto c = *ptr;

			if (CharacterFunctions::isLetterOrDigit(c) ||
				c == ':' || c == '_' || c == '-')
				ptr++;
			else
				break;
		}

		auto id = String(start, ptr);

		code = String(ptr, end);

		return id;
	}

	bool matchIf(const char* token)
	{
		skipWhiteSpace();

		if (code.startsWith(token))
		{
			auto s = code.begin();
			auto e = code.end();

			int offset = String(token).length();

			code = String(s + offset, e);
			return true;
		}
	}

	String skipTemplate()
	{
		skipWhiteSpace();

		if (code.startsWithChar('<'))
		{
			int numOpenTemplateBrackets = 0;

			auto start = code.begin();
			auto end = code.end();
			auto ptr = start;

			while (ptr != end)
			{
				auto c = *ptr;

				if (c == '<')
					numOpenTemplateBrackets++;
				if (c == '>')
					numOpenTemplateBrackets--;

				ptr++;

				if (numOpenTemplateBrackets == 0)
					break;
			}

			code = String(ptr, end);

			jassert(numOpenTemplateBrackets == 0);
			return String(start, ptr);
		}

		return {};
	}

	void parse()
	{
		isStatic = matchIf(JitTokens::static_);
		isConst = matchIf(JitTokens::const_);
		
		typeId = parseIdentifier();

		if (typeId == "double")     t = Types::ID::Double;
		else if (typeId == "float") t = Types::ID::Float;
		else if (typeId == "int")   t = Types::ID::Integer;
		else if (typeId == "void")  t = Types::ID::Void;
		else						t = Types::ID::Pointer;

		typeId << skipTemplate();

		isRef = matchIf("&");

		skipWhiteSpace();
	}

	String code;
	String typeId;
	bool isRef = false;
	bool isConst = false;
	bool isStatic = false;
	Types::ID t = Types::ID::Void;
};

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
		SimpleTypeParser p(t);

		return p.getTypeInfo();
	}

	static MIR_type_t String2MirType(String symbol)
	{
		SimpleTypeParser p(symbol);
		return p.getMirType();
	}

	static String NamespacedIdentifier2MangledMirVar(const NamespacedIdentifier& id)
	{
		auto n = id.toString();
		return n.replaceCharacters(":", "_");
	}

	static jit::Symbol String2Symbol(const String& symbolCode)
	{
		jit::Symbol s;

		SimpleTypeParser p(symbolCode);

		s.typeInfo = p.getTypeInfo();
		s.id = p.parseNamespacedIdentifier();

		return s;
#if 0
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

			Types::ID type = Types::ID::Pointer;

			if (t == "double")	type = Types::ID::Double;
			if (t == "float")	type = Types::ID::Float;
			if (t == "int")		type = Types::ID::Integer;
			if (t == "bool")	type = Types::ID::Integer;
			if (t == "block")	type = Types::ID::Block;
			if (t == "void")	type = Types::ID::Void;

			auto ti = TypeInfo(type, isConst || type == Types::ID::Pointer, isRef);

			return Symbol(NamespacedIdentifier::fromString(n), ti);
		}
#endif
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
		SimpleTypeParser returnParser(s);

		FunctionData f;

		f.returnType = returnParser.getTypeInfo();
		f.id = returnParser.parseNamespacedIdentifier();
		
		auto rest = returnParser.getTrailingString().trim().removeCharacters("()");

		while (rest.isNotEmpty())
		{
			SimpleTypeParser a(rest);

			jit::Symbol arg;
			arg.typeInfo = a.getTypeInfo();
			arg.id = f.id.getChildId(a.parseNamespacedIdentifier().getIdentifier());

			rest = a.getTrailingString().fromFirstOccurrenceOf(",", false, false);

			f.args.add(arg);
		}
		
		return f;
	}

	static String FunctionData2MirTextLabel(const FunctionData& f)
	{
		auto label = NamespacedIdentifier2MangledMirVar(NamespacedIdentifier::fromString(f.id.toString()));

		label << "_";
		label << Types::Helpers::getCppTypeName(f.returnType.getType())[0];

		for (const auto& a : f.args)
			label << Types::Helpers::getCppTypeName(a.typeInfo.getType())[0];

		return label.replaceCharacter('~', '§');
	}

	static String MirTextLabel2FunctionSignature(const String& s)
	{
		FunctionData f;
		
		auto types = s.fromLastOccurrenceOf("_", false, false);
		



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

	static String Symbol2MirTextSymbol(const jit::Symbol& s)
	{
		String t;

		auto tn = TypeInfo2MirTextType(s.typeInfo);

		if (s.typeInfo.isRef())
			tn = "i64";

		t << tn << ":" << NamespacedIdentifier2MangledMirVar(s.id);
		return t;
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
		intOps.set(JitTokens::greaterThan,			"GTS");
		intOps.set(JitTokens::greaterThanOrEqual, 	"GES");
		intOps.set(JitTokens::lessThan,				"LTS");
		intOps.set(JitTokens::lessThanOrEqual,		"LES");
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

	void pushLoopLabels(const String& startLabel, const String& endLabel, const String& continueLabel)
	{
		labelPairs.add({ startLabel, endLabel, continueLabel });
	}

	String getCurrentLabel(const String& instructionId)
	{
		if (instructionId == "continue")
			return labelPairs.getLast().continueLabel;
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
		String continueLabel;
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

	void addPrototype(const std::string& functionName)
	{
		String s(functionName);

		auto tn = s.fromLastOccurrenceOf("_", false, false);
	}

	void addPrototype(const FunctionData& f, bool addObjectPointer)
	{
		MirTextLine l;

		l.label = "proto" + String(prototypes.size());
		l.instruction = "proto";
		
		if (f.returnType.isValid())
			l.operands.add(MirTypeConverters::TypeInfo2MirTextType(f.returnType));

		if (addObjectPointer)
		{
			l.operands.add("i64:_this_");
		}

		for (const auto& a : f.args)
			l.operands.add(MirTypeConverters::Symbol2MirTextSymbol(a));

		emitLine(l);

		prototypes.add(f);
	}

	Array<FunctionData> prototypes;

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

	struct MirMemberInfo
	{
		String id;
		MIR_type_t type;
		size_t offset;
	};

	struct MirTextOperand
	{
		ValueTree v;
		String text;
		String stackPtr;
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
			auto id = state.getAnonymousId(MIR_fp_type_p(type) && rt == RegisterType::Value);
			state.registerCurrentTextOperand(id, type, rt);

			auto t = TypeInfo(MirTypeConverters::MirType2TypeId(type), true);

            auto tn = MirTypeConverters::TypeInfo2MirTextType(t);
            
            if(rt == RegisterType::Pointer || type == MIR_T_P)
                tn = "i64";
            
			localDef << tn << ":" << id;
			return id;
		}

		void addImmOperand(const VariableStorage& value)
		{
			operands.add(Types::Helpers::getCppValueString(value));
		}

		void addSelfAsValueOperand(State& s)
		{
			operands.add(s.getOperandForChild(-1, RegisterType::Value));
		}

		void addSelfAsPointerOperand(State& s)
		{
			operands.add(s.getOperandForChild(-1, RegisterType::Pointer));
		}

		void addChildAsPointerOperand(State& s, int childIndex)
		{
			operands.add(s.loadIntoRegister(childIndex, RegisterType::Pointer));
		}

		void addChildAsValueOperand(State& s, int childIndex)
		{
			operands.add(s.loadIntoRegister(childIndex, RegisterType::Value));
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

	std::map<juce::Identifier, Array<MirMemberInfo>> classTypes;

	void registerClass(const Identifier& id, Array<MirMemberInfo>&& memberInfo)
	{
		classTypes.emplace(id, memberInfo);
	}

	void emitMultiLineCopy(const String& targetPointerReg, const String& sourcePointerReg, int numBytesToCopy)
	{
		auto use64 = numBytesToCopy % 8 == 0;

		jassert(use64);

		for (int i = 0; i < numBytesToCopy; i += 8)
		{
			MirTextLine l;
			l.instruction = "mov";
			
			auto dst = "i64:" + String(i) + "(" + targetPointerReg + ")";
			auto src = "i64:" + String(i) + "(" + sourcePointerReg + ")";

			l.operands.add(dst);
			l.operands.add(src);
			emitLine(l);
		}
	}

	int allocateStack(const String& targetName, int numBytes, bool registerAsCurrentStatementReg)
	{
		State::MirTextLine l;

		if(registerAsCurrentStatementReg)
			registerCurrentTextOperand(targetName, MIR_T_I64, State::RegisterType::Pointer);

		l.localDef << "i64:" << targetName;

		static constexpr int Alignment = 16;

		auto numBytesToAllocate = numBytes + (Alignment - numBytes % Alignment);

		l.instruction = "alloca";
		l.operands.add(targetName);
		l.addImmOperand((int)numBytesToAllocate);

		emitLine(l);

		return numBytesToAllocate;
	}

	void registerCurrentTextOperand(String n, MIR_type_t type, RegisterType rt)
	{
		if(isParsingFunction)
			localOperands.add({ currentTree, n, {}, type, rt });
		else
			globalOperands.add({ currentTree, n, {}, type, rt });
	}

	ValueTree getCurrentChild(int index)
	{
		if (index == -1)
			return currentTree;
		else
			return currentTree.getChild(index);
	}

	MirTextOperand getTextOperandForValueTree(const ValueTree& c)
	{
		for (const auto& t : localOperands)
		{
			if (t.v == c)
				return t;
		}

		for (const auto& t : globalOperands)
		{
			if (t.v == c)
				return t;
		}

		throw String("not found");
	}

	RegisterType getRegisterTypeForChild(int index)
	{
		auto t = getTextOperandForValueTree(getCurrentChild(index));
		return t.registerType;
	}

	MIR_type_t getTypeForChild(int index)
	{
		auto t = getTextOperandForValueTree(getCurrentChild(index));
		return t.type;
	}

	bool isParsingFunction = false;
	int numCurrentlyParsedClasses = 0;

	bool isParsingClass() const { return numCurrentlyParsedClasses > 0; };

	String getOperandForChild(int index, RegisterType requiredType)
	{
		auto t = getTextOperandForValueTree(getCurrentChild(index));

		if (t.registerType == RegisterType::Pointer && requiredType == RegisterType::Value)
		{
			auto x = t.text;

			if (t.stackPtr.isNotEmpty())
			{
				x = t.stackPtr;
			}
				

			auto vt = MirTypeConverters::MirType2MirTextType(t.type);


			// int registers are always 64 bit, but the 
			// address memory layout is 32bit integers so 
			// we need to use a different int type when accessing pointers
			if (vt == "i64")
				vt = "i32";

			vt << ":(" << x << ")";

			return vt;
		}
		else
		{
			return t.stackPtr.isNotEmpty() ? t.stackPtr : t.text;
		}
	}

	Array<MirTextOperand> localOperands;
	Array<MirTextOperand> globalOperands;

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

	String getPrototype(const FunctionData& sig) const
	{
		auto thisLabel = MirTypeConverters::FunctionData2MirTextLabel(sig);

		int l = 0;

		for (const auto& p : prototypes)
		{
			auto pLabel = MirTypeConverters::FunctionData2MirTextLabel(p);

			if(pLabel == thisLabel)
			{
				return "proto" + String(l);
			}

			l++;
		}

		throw String("prototype not found");
	}


	/** Use this whenever you need a child operand as a register that might be a address. 
		This will take the address, emit a load operation and return a pointer instruction using the new register. */
	String loadIntoRegister(int childIndex, RegisterType targetType)
	{
		if (getRegisterTypeForChild(childIndex) == State::RegisterType::Pointer ||
			getTypeForChild(childIndex) == MIR_T_P)
		{
			auto t = getTypeForChild(childIndex);

			State::MirTextLine load;
			load.instruction = "mov";

			auto id = "p" + String(counter++); 
			
			load.localDef << "i64:" << id;
			load.operands.add(id);
			load.addOperands(*this, { childIndex }, { State::RegisterType::Pointer });

			emitLine(load);

			if (targetType == RegisterType::Pointer)
				return id;
			else
			{
				String ptr;

				auto ptr_t = MirTypeConverters::MirType2MirTextType(t);

				if (ptr_t == "i64")
					ptr_t = "i32";

				ptr << ptr_t << ":(" << id << ")";
				return ptr;
			}
		}
		else
			return getOperandForChild(childIndex, RegisterType::Value);
	}



	



};



struct InstructionParsers
{
	static Result SyntaxTree(State& state)
	{
		auto isRoot = !state.currentTree.getParent().isValid();

		if (isRoot)
		{
			state.emitSingleInstruction("module", "main");

			StringArray staticSignatures, memberSignatures;

			MirTypeConverters::forEachChild(state.currentTree, [&](const ValueTree& v)
			{
				if (v.getType() == InstructionIds::FunctionCall)
				{
					auto sig = v.getProperty("Signature").toString();

					if (!MirObject::isExternalFunction(sig))
						return false;

					if(v.getProperty("CallType").toString() == "MemberFunction")
						memberSignatures.addIfNotAlreadyThere(sig);
					else
						staticSignatures.addIfNotAlreadyThere(sig);
				}

				return false;
			});

			for (const auto& c : memberSignatures)
			{
				auto f = MirTypeConverters::String2FunctionData(c);
				state.addPrototype(f, true);
				state.emitSingleInstruction("import " + MirTypeConverters::FunctionData2MirTextLabel(f));
			}
			for (const auto& c : staticSignatures)
			{
				auto f = MirTypeConverters::String2FunctionData(c);
				state.addPrototype(f, false);
				state.emitSingleInstruction("import " + MirTypeConverters::FunctionData2MirTextLabel(f));
			}
		}
		

		state.processAllChildren();

		if(isRoot)
			state.emitSingleInstruction("endmodule");
		
		return Result::ok();
	}

	static Result Function(State& state)
	{
		auto f = MirTypeConverters::String2FunctionData(state.getProperty("Signature"));

		auto addObjectPtr = state.isParsingClass() && !f.returnType.isStatic();

		state.addPrototype(f, addObjectPtr);

		State::MirTextLine line;
		line.label = MirTypeConverters::FunctionData2MirTextLabel(f);
		line.instruction = "func ";

		if (f.returnType.isValid())
			line.operands.add(MirTypeConverters::TypeInfo2MirTextType(f.returnType));

		if (addObjectPtr)
		{
			line.operands.add("i64:_this_");
		}

		for (auto& a : f.args)
			line.operands.add(MirTypeConverters::Symbol2MirTextSymbol(a));
		
		state.emitLine(line);

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

			State::MirTextLine line;
			line.instruction = "ret";

			line.operands.add(state.loadIntoRegister(0, p.getTypeInfo().isRef() ? State::RegisterType::Pointer : State::RegisterType::Value));

			state.emitLine(line);
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

		State::MirTextLine line;

		auto type = state.getTypeForChild(0);
		auto opType = state.getProperty("OpType");

		line.addAnonymousReg(state, type, State::RegisterType::Value);
		line.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, opType);
		line.addSelfAsValueOperand(state);
		line.addChildAsValueOperand(state, 0);
		line.addChildAsValueOperand(state, 1);
	
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
			
			state.registerCurrentTextOperand(mvn, type, isRef ? State::RegisterType::Pointer : State::RegisterType::Value);
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
		line.addSelfAsValueOperand(state);
		line.addChildAsValueOperand(state, 0);

		state.emitLine(line);

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

			State::MirTextLine l;

			if (state.getProperty("First") == "1")
			{
				auto vt = state.getTypeForChild(1);
				auto vrt = state.getRegisterTypeForChild(1);

				if (vrt == State::RegisterType::Pointer)
				{
					jassert(opType == JitTokens::assign_);
					jassert(state.getRegisterTypeForChild(1) == State::RegisterType::Pointer);

					auto name = state.getOperandForChild(1, State::RegisterType::Pointer);

					State::MirTextLine l;
					state.registerCurrentTextOperand(name, t, State::RegisterType::Pointer);
					l.localDef << "i64:" << name;
					l.instruction = "mov";

					l.addSelfAsPointerOperand(state);
					l.addChildAsPointerOperand(state, 0);
					l.appendComment("Add ref");

					state.emitLine(l);

					return Result::ok();
				}

				l.localDef << MirTypeConverters::MirType2MirTextType(state.getTypeForChild(1));
				l.localDef << ":" << state.getOperandForChild(1, State::RegisterType::Raw);
			}

			//l.addAnonymousReg(state, t, State::RegisterType::Value);

			l.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(t, opType);

			l.addChildAsValueOperand(state, 1);

			if (opType == JitTokens::assign_)
				l.addChildAsValueOperand(state, 0);
			else
			{
				l.addChildAsValueOperand(state, 1);
				l.addChildAsValueOperand(state, 0);
			}
			
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
		l.addAnonymousReg(state, MIR_T_I64, State::RegisterType::Value);
		l.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, opType);

		l.addSelfAsValueOperand(state);
		l.addChildAsValueOperand(state, 0);
		l.addChildAsValueOperand(state, 1);

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
		jumpToFalse.addChildAsValueOperand(state, 0);
		state.emitLine(jumpToFalse);

		// emit the true branch
		state.processChildTree(1);

		auto registerType = state.getRegisterTypeForChild(1);
		auto type = state.getTypeForChild(1);

		State::MirTextLine tl;
		tl.addAnonymousReg(state, type, registerType);
		state.emitLine(tl);

		State::MirTextLine movLine_t;

		if (registerType == State::RegisterType::Value)
		{
			movLine_t.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
			movLine_t.addSelfAsValueOperand(state);
			movLine_t.addChildAsValueOperand(state, 1);
		}
		else
		{
			movLine_t.instruction = "mov";
			movLine_t.addOperands(state, { -1, 1 }, { registerType, registerType });
		}

		
		state.emitLine(movLine_t);

		state.emitSingleInstruction("jmp " + endLabel);

		// emit the false branch
		state.emitLabel(falseLabel, true);
		state.processChildTree(2);

		State::MirTextLine movLine_f;

		if (registerType == State::RegisterType::Value)
		{
			movLine_f.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
			movLine_f.addSelfAsValueOperand(state);
			movLine_f.addChildAsValueOperand(state, 2);
		}
		else
		{
			movLine_f.instruction = "mov";
			movLine_f.addOperands(state, { -1, 2 }, { registerType, registerType });
		}

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
		jumpToFalse.addChildAsValueOperand(state, 0);
		//jumpToFalse.addOperands(state, { 0 }, { State::RegisterType::Value });
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
		l.addSelfAsValueOperand(state);
		l.addChildAsValueOperand(state, 0);
		l.addImmOperand(VariableStorage(0));

		state.emitLine(l);
		
		return Result::ok();
	}

	static Result WhileLoop(State& state)
	{
		auto cond_label = state.makeLabel();
		auto end_label = state.makeLabel();
		String post_label;
	
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

			post_label = state.makeLabel();

			state.loopManager.pushLoopLabels(cond_label, end_label, post_label);
		}
		else
		{
			state.loopManager.pushLoopLabels(cond_label, end_label, cond_label);
		}

		

		if (assignIndex != -1)
			state.processChildTree(assignIndex);

		state.emitLabel(cond_label, true);
		
		state.processChildTree(conditionIndex);

		State::MirTextLine jumpToEnd;
		jumpToEnd.instruction = "bf";
		jumpToEnd.operands.add(end_label);
		jumpToEnd.addChildAsValueOperand(state, conditionIndex);
		state.emitLine(jumpToEnd);

		state.processChildTree(bodyIndex);

		if (postOpIndex != -1)
		{
			state.emitLabel(post_label, true);
			state.processChildTree(postOpIndex);
		}

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
		mov_l.addSelfAsValueOperand(state);
		mov_l.addChildAsValueOperand(state, 0);
		

		State::MirTextLine add_l;
		add_l.instruction = "add";
		add_l.addChildAsValueOperand(state, 0);
		add_l.addChildAsValueOperand(state, 0);
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
            if(v.getType() == InstructionIds::Assignment && v["First"].toString() == "1")
            {
                auto c = v.getChild(1);
                
                if(c.getType() == InstructionIds::VariableReference)
                {
                    auto vSymbol = MirTypeConverters::String2Symbol(c.getProperty("Symbol").toString());

                    if (vSymbol.id.getParent() == scopeId)
                    {
                        localSymbols.add(vSymbol);
                    }
                }
                
            }
			
			return false;
		});

		State::MirTextLine local_def_l;

#if 0
		for (auto& l : localSymbols)
			local_def_l.operands.add("local " + MirTypeConverters::Symbol2MirTextSymbol(l));
#endif

		state.emitLine(local_def_l);

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

			State::MirTextLine l;
			
			auto name = state.getProperty("Ids").upToLastOccurrenceOf(",", false, false);

			auto vn = MirTypeConverters::NamespacedIdentifier2MangledMirVar(NamespacedIdentifier::fromString(name));

			state.registerCurrentTextOperand(vn, t, State::RegisterType::Pointer);

			l.localDef << "i64:" << vn;
			
			l.instruction = "mov";
			
			l.addSelfAsPointerOperand(state);
			l.addChildAsPointerOperand(state, 0);
			
			state.emitLine(l);

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
					State::MirTextLine l;

					l.label = s.id.toString();
					l.instruction = "bss";
					l.addImmOperand(jmax<int>(numBytes, numBytesInParser, 8));
					l.appendComment("Dummy Address for zero sized-class object");
					state.emitLine(l);
				}
				else
				{
					while (lastOffset < numBytes)
					{
						initParser.forEach([&](uint32 offset, Types::ID type, const VariableStorage& v)
						{
							State::MirTextLine l;

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

					if (lastOffset % 8 != 0)
					{
						State::MirTextLine l;

						l.instruction = "bss";
						l.addImmOperand(4);
						l.appendComment("pad to 8 byte alignment");
						state.emitLine(l);
					}
				}

				
			}
			else
			{
				// stack definition
				auto numBytes = (uint32)state.getProperty("NumBytes").getIntValue();

				bool something = numBytes != 0;

				auto name = NamespacedIdentifier::fromString(state.getProperty("Ids").upToFirstOccurrenceOf(",", false, false).trim());
				auto mn = MirTypeConverters::NamespacedIdentifier2MangledMirVar(name);

				numBytes = state.allocateStack(mn, numBytes, true);

				

				auto hasDynamicInitialisation = state.currentTree.getNumChildren() != 0;

				if(hasDynamicInitialisation)
				{
					if (!state.currentTree.hasProperty("InitValues"))
					{
						// Only one expression is supported at the moment...
						jassert(state.currentTree.getNumChildren() == 1);

						state.processChildTree(0);

						auto src = state.loadIntoRegister(0, State::RegisterType::Pointer);

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
							State::MirTextLine il;
							auto t = MirTypeConverters::TypeInfo2MirType(TypeInfo(type));
							il.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(t, JitTokens::assign_);
							auto p = state.getOperandForChild(-1, State::RegisterType::Raw);

							auto mir_t = MirTypeConverters::MirType2MirTextType(t);

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
								il.addChildAsValueOperand(state, childIndex);
							}
							else
								il.addImmOperand(v);

							state.emitLine(il);
						});
					}
				}

				

#if 0
				// fill with zeros
				while (lastOffset < numBytesToAllocate)
				{
					State::MirTextLine il;
					
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

        auto t = MirTypeConverters::String2Symbol(state.getProperty("ElementType"));

        auto mir_t = MirTypeConverters::TypeInfo2MirType(t.typeInfo);
        auto tn = MirTypeConverters::TypeInfo2MirTextType(t.typeInfo);

		//mir_t = MIR_T_I64;

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
		idx.addChildAsValueOperand(state, 1);

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
    
    static Result Loop(State& state)
    {
        auto startLabel = state.makeLabel();
        auto endLabel = state.makeLabel();
        
        state.loopManager.pushLoopLabels(startLabel, endLabel, startLabel);
        
        // create iterator variable with pointer type
        auto iteratorSymbol = MirTypeConverters::String2Symbol(state.getProperty("Iterator"));
        auto type = MirTypeConverters::SymbolToMirVar(iteratorSymbol).type;
        auto mvn = MirTypeConverters::NamespacedIdentifier2MangledMirVar(iteratorSymbol.id);

        auto isRef = iteratorSymbol.typeInfo.isRef();
        
        state.processChildTree(0);
        
        state.registerCurrentTextOperand(mvn, type, isRef ? State::RegisterType::Pointer :
                                                            State::RegisterType::Value);
        
        // If the iterator is not a reference, we'll create another register for the address
        String pointerReg = isRef ? mvn : state.getAnonymousId(false);
        
        State::MirTextLine loadIter;
        loadIter.localDef << "i64:" << pointerReg;
        loadIter.instruction = "mov";
        loadIter.operands.add(pointerReg);
        loadIter.addOperands(state, {0}, { State::RegisterType::Pointer });
        state.emitLine(loadIter);
        
        // create anonymous end address variable with pointer type
        auto endReg = state.getAnonymousId(false);
        
        auto elementSize = (int)state.currentTree.getProperty("ElementSize");
        
        if(state.getProperty("LoopType") == "Span")
        {
            int byteSize = elementSize *
                           (int)state.currentTree.getProperty("NumElements");
            
            State::MirTextLine ii;
            ii.localDef << "i64:" << endReg;
            ii.instruction = "add";
            ii.operands.add(endReg);
            ii.operands.add(pointerReg);
            ii.addImmOperand(byteSize);
            
            state.emitLine(ii);
        }
        else
        {
            // not implemented...
            jassertfalse;
        }
        
        // emit start_loop label
        state.emitLabel(startLabel, true);
        
        if(!isRef)
        {
            auto tn = MirTypeConverters::MirType2MirTextType(type);
            
            // Load the value into the iterator variable
            // from the address in pointerReg
            State::MirTextLine loadCopy;
            loadCopy.localDef << tn << ":" << mvn;
            loadCopy.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
            loadCopy.operands.add(mvn);
            
            String ptrAddress;
            ptrAddress << tn << ":(" << pointerReg << ")";
            
            loadCopy.operands.add(ptrAddress);
            loadCopy.appendComment("iterator load");
            
            state.emitLine(loadCopy);
        }
        
        // emit body
        state.processChildTree(1);
        
        // bump address by element size after loop body
        State::MirTextLine bump;
        bump.instruction = "add";
        bump.operands.add(pointerReg);
        bump.operands.add(pointerReg);
        bump.addImmOperand(elementSize);
        state.emitLine(bump);
        
        // compare addresses, jump to start_loop if iterator != end
        State::MirTextLine cmp;
        cmp.instruction = "bne";
        cmp.operands.add(startLabel);
        cmp.operands.add(pointerReg);
        cmp.operands.add(endReg);
        state.emitLine(cmp);
        
        // emit end_loop label
        state.emitLabel(endLabel, true);
        state.loopManager.popLoopLabels();
        
        return Result::ok();
    }

	static Result FunctionCall(State& state)
	{
		state.processAllChildren();

		auto sig = MirTypeConverters::String2FunctionData(state.getProperty("Signature"));
		auto fid = MirTypeConverters::FunctionData2MirTextLabel(sig);
		auto protoType = state.getPrototype(sig);

		State::MirTextLine l;

		l.instruction = "call";
		l.operands.add(protoType);
		l.operands.add(fid);

		if (sig.returnType.isValid())
		{
			auto rrt = sig.returnType.isRef() ? State::RegisterType::Pointer : State::RegisterType::Value;
			l.addAnonymousReg(state, MirTypeConverters::TypeInfo2MirType(sig.returnType), rrt);
			l.operands.add(state.getOperandForChild(-1, rrt));
		}

		int argOffset = 0;

		if (state.getProperty("CallType") == "MemberFunction")
		{
			auto a = state.loadIntoRegister(0, State::RegisterType::Pointer);
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
					numBytes = m.offset + Types::Helpers::getSizeForType(MirTypeConverters::MirType2TypeId(m.type));
				
				auto stackPtr = state.getAnonymousId(false);

				numBytes = state.allocateStack(stackPtr, numBytes, false);
				auto source = state.loadIntoRegister(i + argOffset, State::RegisterType::Pointer);
				state.emitMultiLineCopy(stackPtr, source, numBytes);
				l.operands.add(stackPtr);
			}
			else
			{
				if (isRef)
				{
					auto rt = state.getRegisterTypeForChild(i + argOffset);
					auto tp = state.getTypeForChild(i + argOffset);

					if (rt == State::RegisterType::Value && tp != MIR_T_P)
					{
						// We need to copy the value from the register to the stack
						

						auto mir_t = state.getTypeForChild(i + argOffset);

						auto stackPtr = state.getAnonymousId(false);

						state.allocateStack(stackPtr, Types::Helpers::getSizeForType(MirTypeConverters::MirType2TypeId(mir_t)), false);

						State::MirTextLine mv;
						mv.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(mir_t, JitTokens::assign_);

						String dst;
						dst << MirTypeConverters::MirType2MirTextType(mir_t);
						dst << ":(" << stackPtr << ")";

						mv.operands.add(dst);
						mv.addChildAsValueOperand(state, i + argOffset);
						mv.appendComment("Move register to stack");

						state.emitLine(mv);
						
						auto registerToSpill = state.getOperandForChild(i + argOffset, State::RegisterType::Value);

						for (auto& l : state.localOperands)
						{
							if (l.text == registerToSpill)
							{
								// update the operands to use the stack pointer from now on...
								l.registerType = State::RegisterType::Pointer;
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
							auto a = state.getOperandForChild(i + argOffset, State::RegisterType::Pointer);
							l.operands.add(a);
						}
						else
						{
							// make sure the adress is loaded into a register
							auto a = state.loadIntoRegister(i + argOffset, State::RegisterType::Pointer);
							l.operands.add(a);
						}
					}
				}
				else
				{
					auto a = state.loadIntoRegister(i + argOffset, State::RegisterType::Value);
					l.operands.add(a);
				}

				
			}
		}
			
		state.emitLine(l);

		return Result::ok();
	};

	static Result ClassStatement(State& state)
	{
		auto members = StringArray::fromTokens(state.getProperty("MemberInfo"), "$", "");

		Array<State::MirMemberInfo> memberInfo;

		for (const auto& m : members)
		{
			State::MirMemberInfo item;

			auto symbol = MirTypeConverters::String2Symbol(m);
			
			item.id = MirTypeConverters::NamespacedIdentifier2MangledMirVar(symbol.id);
			item.type = MirTypeConverters::TypeInfo2MirType(symbol.typeInfo);
			item.offset = m.fromFirstOccurrenceOf("(", false, false).getIntValue();
			
			memberInfo.add(item);
		}

		auto x = NamespacedIdentifier::fromString(state.getProperty("Type"));
		auto className = Identifier(MirTypeConverters::NamespacedIdentifier2MangledMirVar(x));

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

		auto memberSymbol = MirTypeConverters::String2Symbol(state.getCurrentChild(1)["Symbol"].toString());

		auto classId = Identifier(MirTypeConverters::NamespacedIdentifier2MangledMirVar(memberSymbol.id.getParent()));
		auto memberId = memberSymbol.id.getIdentifier().toString();
		auto memberType = MirTypeConverters::Symbol2MirTextSymbol(memberSymbol);

		for (const auto& m : state.classTypes[classId])
		{
			if (m.id == memberId)
			{
				auto ptr = state.loadIntoRegister(0, State::RegisterType::Pointer);

				String p;

				State::MirTextLine offset;
				offset.instruction = "add";
				offset.operands.add(ptr);
				offset.operands.add(ptr);
				offset.addImmOperand((int)m.offset);
				offset.appendComment(classId + "." + m.id);
				state.emitLine(offset);
				

				state.registerCurrentTextOperand(ptr, m.type, State::RegisterType::Pointer);
				break;
			}
		}

		return Result::ok();
	}

	static Result ThisPointer(State& state)
	{
		state.registerCurrentTextOperand("_this_", MIR_T_P, State::RegisterType::Value);

		return Result::ok();
	}

	static Result PointerAccess(State& state)
	{
		state.processChildTree(0);

		auto n = state.getOperandForChild(0, State::RegisterType::Raw);
		state.registerCurrentTextOperand(n, MIR_T_P, State::RegisterType::Value);

		return Result::ok();
	}
};



#define REGISTER_TYPE(X) currentState->registerFunction(InstructionIds::X, InstructionParsers::X);

MirBuilder::MirBuilder(MIR_context* ctx_, const ValueTree& v_) :
	root(v_)
{
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
    REGISTER_TYPE(Loop);
	REGISTER_TYPE(FunctionCall);
	REGISTER_TYPE(ClassStatement);
	REGISTER_TYPE(Dot);
	REGISTER_TYPE(ThisPointer);
	REGISTER_TYPE(PointerAccess);
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
