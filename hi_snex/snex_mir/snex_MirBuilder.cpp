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
		static std::map<MIR_type_t, std::map<String, String>> codes;

		codes.emplace(MIR_T_I64, std::map<String, String>());
		codes.emplace(MIR_T_F, std::map<String, String>());
		codes.emplace(MIR_T_D, std::map<String, String>());

		codes[MIR_T_I64].emplace(JitTokens::assign_,			"MOV");
		codes[MIR_T_I64].emplace(JitTokens::plus,				"ADD");
		codes[MIR_T_I64].emplace(JitTokens::minus,				"SUB");
		codes[MIR_T_I64].emplace(JitTokens::times,				"MUL");
		codes[MIR_T_I64].emplace(JitTokens::divide,				"DIV");
		codes[MIR_T_I64].emplace(JitTokens::modulo,				"MOD");
		codes[MIR_T_I64].emplace(JitTokens::greaterThan,		"GT");
		codes[MIR_T_I64].emplace(JitTokens::greaterThanOrEqual, "GE");
		codes[MIR_T_I64].emplace(JitTokens::lessThan,			"LT");
		codes[MIR_T_I64].emplace(JitTokens::lessThanOrEqual,	"LE");
		codes[MIR_T_I64].emplace(JitTokens::equals,				"EQ");
		codes[MIR_T_I64].emplace(JitTokens::notEquals,			"NE");
		codes[MIR_T_F].emplace(JitTokens::assign_,				"FMOV");
		codes[MIR_T_F].emplace(JitTokens::plus,					"FADD");
		codes[MIR_T_F].emplace(JitTokens::minus,				"FSUB");
		codes[MIR_T_F].emplace(JitTokens::times,				"FMUL");
		codes[MIR_T_F].emplace(JitTokens::divide,				"FDIV");
		codes[MIR_T_F].emplace(JitTokens::greaterThan,			"FGT");
		codes[MIR_T_F].emplace(JitTokens::greaterThanOrEqual,	"FGE");
		codes[MIR_T_F].emplace(JitTokens::lessThan,				"FLT");
		codes[MIR_T_F].emplace(JitTokens::lessThanOrEqual,		"FLE");
		codes[MIR_T_F].emplace(JitTokens::equals,				"FEQ");
		codes[MIR_T_F].emplace(JitTokens::notEquals,			"FNE");
		codes[MIR_T_D].emplace(JitTokens::assign_,				"DMOV");
		codes[MIR_T_D].emplace(JitTokens::plus,					"DADD");
		codes[MIR_T_D].emplace(JitTokens::minus,				"DSUB");
		codes[MIR_T_D].emplace(JitTokens::times,				"DMUL");
		codes[MIR_T_D].emplace(JitTokens::divide,				"DDIV");
		codes[MIR_T_D].emplace(JitTokens::greaterThan,			"DGT");
		codes[MIR_T_D].emplace(JitTokens::greaterThanOrEqual,	"DGE");
		codes[MIR_T_D].emplace(JitTokens::lessThan,				"DLT");
		codes[MIR_T_D].emplace(JitTokens::lessThanOrEqual,		"DLE");
		codes[MIR_T_D].emplace(JitTokens::equals,				"DEQ");
		codes[MIR_T_D].emplace(JitTokens::notEquals,			"DNE");

		return codes[type][token].toLowerCase();
	}

	static MIR_insn_code_t MirTypeAndToken2InstructionCode(MIR_type_t type, const String& token)
	{
		static std::map<MIR_type_t, std::map<String, MIR_insn_code_t>> codes;

		codes.emplace(MIR_T_I64, std::map<String, MIR_insn_code_t>());
		codes.emplace(MIR_T_F, std::map<String, MIR_insn_code_t>());
		codes.emplace(MIR_T_D, std::map<String, MIR_insn_code_t>());

		// INT =================================================================

		codes[MIR_T_I64].emplace(JitTokens::assign_, MIR_MOV);
		codes[MIR_T_I64].emplace(JitTokens::plus, MIR_ADD);
		codes[MIR_T_I64].emplace(JitTokens::minus, MIR_SUB);
		codes[MIR_T_I64].emplace(JitTokens::times, MIR_MUL);
		codes[MIR_T_I64].emplace(JitTokens::divide, MIR_DIV);
		codes[MIR_T_I64].emplace(JitTokens::modulo, MIR_MOD);

		codes[MIR_T_I64].emplace(JitTokens::greaterThan,		MIR_GT);
		codes[MIR_T_I64].emplace(JitTokens::greaterThanOrEqual, MIR_GE);
		codes[MIR_T_I64].emplace(JitTokens::lessThan,			MIR_LT);
		codes[MIR_T_I64].emplace(JitTokens::lessThanOrEqual,	MIR_LE);
		codes[MIR_T_I64].emplace(JitTokens::equals,				MIR_EQ);
		codes[MIR_T_I64].emplace(JitTokens::notEquals,			MIR_NE);

		// FLOAT =================================================================

		codes[MIR_T_F].emplace(JitTokens::assign_, MIR_FMOV);
		codes[MIR_T_F].emplace(JitTokens::plus,   MIR_FADD);
		codes[MIR_T_F].emplace(JitTokens::minus,  MIR_FSUB);
		codes[MIR_T_F].emplace(JitTokens::times,  MIR_FMUL);
		codes[MIR_T_F].emplace(JitTokens::divide, MIR_FDIV);
		
		codes[MIR_T_F].emplace(JitTokens::greaterThan,			MIR_FGT);
		codes[MIR_T_F].emplace(JitTokens::greaterThanOrEqual,	MIR_FGE);
		codes[MIR_T_F].emplace(JitTokens::lessThan,				MIR_FLT);
		codes[MIR_T_F].emplace(JitTokens::lessThanOrEqual,		MIR_FLE);
		codes[MIR_T_F].emplace(JitTokens::equals,				MIR_FEQ);
		codes[MIR_T_F].emplace(JitTokens::notEquals,			MIR_FNE);

		// DOUBLE =================================================================

		codes[MIR_T_D].emplace(JitTokens::assign_, MIR_DMOV);
		codes[MIR_T_D].emplace(JitTokens::plus,   MIR_DADD);
		codes[MIR_T_D].emplace(JitTokens::minus,  MIR_DSUB);
		codes[MIR_T_D].emplace(JitTokens::times,  MIR_DMUL);
		codes[MIR_T_D].emplace(JitTokens::divide, MIR_DDIV);

		codes[MIR_T_D].emplace(JitTokens::greaterThan,			MIR_DGT);
		codes[MIR_T_D].emplace(JitTokens::greaterThanOrEqual,	MIR_DGE);
		codes[MIR_T_D].emplace(JitTokens::lessThan,				MIR_DLT);
		codes[MIR_T_D].emplace(JitTokens::lessThanOrEqual,		MIR_DLE);
		codes[MIR_T_D].emplace(JitTokens::equals,				MIR_DEQ);
		codes[MIR_T_D].emplace(JitTokens::notEquals,			MIR_DNE);

		return codes[type][token];
	}
};





struct State
{
	enum class CallOrder
	{
		Pre,
		Post,
		numCallOrders
	};

	bool isPre() const {
		return order == CallOrder::Pre;
	}

	struct MirDataPool
	{
		MirDataPool(State& parent_) :
			parent(parent_)
		{};

		void add(const jit::Symbol& symbol, const VariableStorage& value)
		{
			

			

			auto ni = new MirDataPool::Item();

			ni->symbol = MirTypeConverters::SymbolToMirVar(symbol);
			ni->value = value;

			if (ni->symbol.type == MIR_T_P)
			{
				auto numBytes = value.getPointerSize();
				ni->item = MIR_new_data(parent.ctx, ni->symbol.name, MIR_T_I8, numBytes, ni->value.getDataPointer());
			}
			else
			{
				ni->item = MIR_new_data(parent.ctx, ni->symbol.name, ni->symbol.type, 1, ni->value.getDataPointer());
			}

			ni->ptr = ni->item->u.data->u.els;
			items.add(ni);
		}

		int64_t getAddress(const MIR_var_t& s)
		{
			for (const auto i : items)
			{
				if (strcmp(s.name, i->symbol.name) == 0)
					return reinterpret_cast<int64_t>(i->ptr);
			}

			return 0;
		}

	private:

		State& parent;

		struct Item
		{
			MIR_var_t symbol;
			MIR_item_t item;
			VariableStorage value;
			void* ptr;
			bool dirty;
			MIR_reg_t assignedRegister;
		};

		OwnedArray<Item> items;
	} dataPool;

	struct RegisterDatabase
	{
		RegisterDatabase(State& parent_) :
			parent(parent_)
		{};

		template <typename T> void addImmediate(const T& v, const VariableStorage& value)
		{
			Item n;

			n.hashes.add(MirTypeConverters::getHash(v));
			n.value = value;
			n.type.type = MirTypeConverters::TypeInfo2MirType(TypeInfo(value.getType()));
			n.isImm = true;

			items.add(n);
		}

		template <typename T> void add(const T& v, MIR_reg_t reg, MIR_var_t type)
		{
			auto h = MirTypeConverters::getHash(v);

			if (auto existing = get(h))
				return;

			Item n;

			n.hashes.add(h);

			n.reg = reg;
			n.type = type;

			items.add(n);
		}

		template <typename T> MIR_reg_t getRegisterIndex(const T& v, bool throwIfFailure = true)
		{
			if (auto item = get(MirTypeConverters::getHash(v)))
			{
				if (item->isImm)
				{
					// Load the imm into a register...

					auto op = MirTypeConverters::MirTypeAndToken2InstructionCode(item->type.type, JitTokens::assign_);
					auto id = getAnonymousId(MIR_fp_type_p(item->type.type));
					item->reg = MIR_new_func_reg(parent.ctx, parent.currentFunc->u.func, item->type.type, id.getCharPointer().getAddress());

					auto x = createImmOp(item->value);

					auto in = MIR_new_insn(parent.ctx, op, MIR_new_reg_op(parent.ctx, item->reg), x);

					parent.emitInstruction(in);

					item->isImm = false;
				}
				

				return item->reg;
			}


			throw String("Can't find register");
		}

		template <typename T> void setMemoryOp(const T& v, MIR_op_t memOp)
		{
			if (auto item = get(MirTypeConverters::getHash(v)))
			{
				item->memOp = memOp;
				item->useMem = true;
				return;
			}

			throw String("Can't find register");
		}

		template <typename ExistingType, typename NewType> bool addHash(const ExistingType& e, const NewType& n)
		{
			auto ehash = MirTypeConverters::getHash(e);
			auto nhash = MirTypeConverters::getHash(n);

			if (auto existing = get(ehash))
			{
				existing->hashes.addIfNotAlreadyThere(nhash);
				return true;
			}

			return false;
		}

		template <typename T> bool contains(const T& v)
		{
			return get(MirTypeConverters::getHash(v)) != nullptr;
		}

		template <typename T> VariableStorage getRegisterValue(const T& v)
		{
			if (auto existing = get(MirTypeConverters::getHash(v)))
				return existing->value;

			throw String("Can't find register");
		}

		template <typename T> MIR_var_t getRegisterType(const T& v)
		{
			if (auto existing = get(MirTypeConverters::getHash(v)))
				return existing->type;

			throw String("Can't find register");
		}

		template <typename T> MIR_op_t createOp(const T& v)
		{
			if (auto existing = get(MirTypeConverters::getHash(v)))
			{
				if (existing->useMem)
					return existing->memOp;

				else if (!existing->isImm)
					return MIR_new_reg_op(parent.ctx, getRegisterIndex(v));
				else
				{
					return createImmOp(existing->value);
				}
			}

			throw String("Can't find register");
		}

		String getAnonymousId(bool isFloat) const
		{
			String s;
			s << (isFloat ? "xmm" : "reg") << String(counter++);
			return s;
		}

	private:

		mutable int counter = 0;

		MIR_op_t createImmOp(const VariableStorage& imm) const
		{
			if (imm.getType() == Types::ID::Integer)
				return MIR_new_int_op(parent.ctx, imm.toInt());
			if (imm.getType() == Types::ID::Float)
				return MIR_new_float_op(parent.ctx, imm.toFloat());
			if (imm.getType() == Types::ID::Double)
				return MIR_new_double_op(parent.ctx, imm.toDouble());
		}

		struct Item
		{
			Array<int64> hashes;
			MIR_reg_t reg;
			MIR_var_t type;
			VariableStorage value;
			MIR_op_t memOp;
			bool useMem = false;
			bool isImm = false;
		};

		Item* get(int64_t hash)
		{
			for (auto& i : items)
			{
				if (i.hashes.contains(hash))
					return &i;
			}

			return nullptr;
		}

		const Item* get(int64_t hash) const
		{
			for (auto& i : items)
			{
				if (i.hashes.contains(hash))
					return &i;
			}

			return nullptr;
		}

		Array<Item> items;

		State& parent;
	} registers;

	State() :
		dataPool(*this),
		registers(*this)
	{};

	

	using ValueTreeFuncton = std::function<Result(State& state)>;

	MIR_context_t ctx;
	MIR_module_t currentModule = nullptr;
	MIR_item_t currentFunc = nullptr;
	const ValueTree* currentTree = nullptr;
	CallOrder order = CallOrder::numCallOrders;

	using LabelPair = snex::span<MIR_insn_t, 2>;

	Array<LabelPair> currentLoopLabels;
	 
	LabelPair pushLoop()
	{
		auto loop_begin = MIR_new_label(ctx);
		auto loop_end = MIR_new_label(ctx);

		currentLoopLabels.add({ loop_begin, loop_end });
		return currentLoopLabels.getLast();
	}

	LabelPair popLoop()
	{
		return currentLoopLabels.removeAndReturn(currentLoopLabels.size() - 1);
	}

	struct Instruction
	{
		ValueTree v;
		MIR_insn_t instruction;
	};

	Array<Instruction> instructions;

	MIR_insn_t getLastInstruction(int childIndex)
	{
		auto v = currentTree->getChild(childIndex);

		for (const auto& i : instructions)
		{
			if (v == i.v)
			{
				return i.instruction;
			}
		}

		return nullptr;
	}

	void emitInstruction(MIR_insn_t newInstruction)
	{
		instructions.add({ *currentTree, newInstruction });
		MIR_append_insn(ctx, currentFunc, newInstruction);
	}

	void emitInstructionAfter(MIR_insn_t newInstruction, MIR_insn_t previousInstruction)
	{
		// Update the last instruction...
		for (auto& c : instructions)
		{
			if (c.instruction == previousInstruction)
			{
				c.instruction = newInstruction;
				break;
			}
		}

		MIR_insert_insn_after(ctx, currentFunc, previousInstruction, newInstruction);
	}

	MIR_type_t createTempRegisterLikeChild(int index=0)
	{
		auto type =  registers.getRegisterType(currentTree->getChild(index));
		auto id = registers.getAnonymousId(MIR_fp_type_p(type.type));
		type.name = id.getCharPointer().getAddress();
		
		createVariable(type);
		return type.type;
	}

	MIR_type_t createTempRegisterWithType(Types::ID typeId, bool addToPool=true)
	{
		MIR_var_t type;

		auto id = registers.getAnonymousId(MIR_fp_type_p(type.type));
		type.name = id.getCharPointer().getAddress();
		type.type = MirTypeConverters::TypeInfo2MirType(TypeInfo(typeId, true));

		createVariable(type, addToPool);
		return type.type;
	}
	
	MIR_reg_t createVariable(MIR_var_t type, bool addToPool=true)
	{
		auto tToUse = type.type;

		if (tToUse == MIR_T_P)
			tToUse = MIR_T_I64;

		auto r = MIR_new_func_reg(ctx, currentFunc->u.func, tToUse, type.name);

		if(addToPool)
			registers.add(*currentTree, r, type);

		return r;
	}

	String getProperty(const Identifier& id) const
	{
		if (!currentTree->hasProperty(id))
		{
			dump();
			throw String("No property " + id.toString());
		}
			

		return currentTree->getProperty(id).toString();
	}

	

	void dump() const
	{
		DBG(currentTree->createXml()->createDocument(""));
	}

	MIR_insn_t createInstruction(MIR_insn_code_t code, const Array<int>& indexes)
	{
		if (indexes.size() == 1)
			return MIR_new_insn(ctx, code, createOpForCurrentChild(indexes[0]));
		if(indexes.size() == 2)
			return MIR_new_insn(ctx, code, createOpForCurrentChild(indexes[0]),
										   createOpForCurrentChild(indexes[1]));
		if(indexes.size() == 3)
			return MIR_new_insn(ctx, code, createOpForCurrentChild(indexes[0]),
										   createOpForCurrentChild(indexes[1]),
										   createOpForCurrentChild(indexes[2]));

		jassertfalse;
		throw String("illegal num operands");
	}

	MIR_var_t getTypeOfFirstChild()
	{
		return registers.getRegisterType(currentTree->getChild(0));
	}

	MIR_op_t createOpForCurrentChild(int index)
	{
		ValueTree c;

		if (index == -1)
			c = *currentTree;
		else
			c = currentTree->getChild(index);

		return registers.createOp(c);
	}

	

	enum class RegisterType
	{
		Raw,
		Value,
		Pointer,
		numRegisterTypes
	};

	struct MirTextLabel
	{
		ValueTree v;
		String label;
	};

	Array<MirTextLabel> labels;

	void registerTextLabel(int childIndex)
	{
		MirTextLabel l;

		if (childIndex == -1)
			l.v = *currentTree;
		else
			l.v = currentTree->getChild(childIndex);

		l.label = "L" + String(labels.size()+1);
		labels.add(l);
	}

	String getLabel(int childIndex)
	{
		ValueTree v;
		if (childIndex == -1)
			v = *currentTree;
		else
			v = currentTree->getChild(childIndex);

		for (auto& l : labels)
		{
			if (l.v == v)
				return l.label;
		}

		return {};
	}

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
			auto id = state.registers.getAnonymousId(MIR_fp_type_p(type));
			state.registerCurrentTextOperand(id, type, rt);

			auto t = TypeInfo(MirTypeConverters::MirType2TypeId(type), true);

			localDef << MirTypeConverters::TypeInfo2MirTextType(t) << ":" << id;
			return id;
		}

		void attachLabel(State& state, int childIndex)
		{
			auto s = state.getLabel(childIndex);

			if (s.isNotEmpty())
				label = s;
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

	void appendLine(MirTextLine& l, bool attachLabelForCurrentTree)
	{
		if (attachLabelForCurrentTree)
			l.attachLabel(*this, -1);

		lines.add(l);
	}

	void insertLine(const MirTextLine& l, const String& label, bool insertBefore)
	{
		for (int i = 0; i < lines.size(); i++)
		{
			if (lines[i].label == label)
			{
				lines.insert(insertBefore ? i : i + 1, l);
				return;
			}
		}

		throw String("label not found");
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
		textOperands.add({ *currentTree, n, type, rt });
	}

	MIR_type_t getTypeForChild(int index)
	{
		ValueTree c;

		if (index == -1)
			c = *currentTree;
		else
			c = currentTree->getChild(index);

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
		ValueTree c;

		if (index == -1)
			c = *currentTree;
		else
			c = currentTree->getChild(index);

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

					appendLine(load, false);

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
};

static int64 funky = 1205;



struct InstructionParsers
{
	static Result SyntaxTree(State& state)
	{
		if (!state.currentTree->getParent().isValid())
		{
#if CREATE_MIR_TEXT
			if (state.isPre())
			{
				State::MirTextLine l;
				l.label = "main";
				l.instruction = "module";
				state.appendLine(l, false);
			}
			else
			{
				State::MirTextLine l;
				l.instruction = "endmodule";
				state.appendLine(l, false);
			}
#else
			if (state.isPre())
				state.currentModule = MIR_new_module(state.ctx, "main");
			else
				MIR_finish_module(state.ctx);
#endif
		}

		return Result::ok();
	}

	static Result Function(State& state)
	{
#if CREATE_MIR_TEXT
		if (state.isPre())
		{
			auto sig = state.currentTree->getProperty("Signature").toString();

			auto f = MirTypeConverters::String2FunctionData(sig);
			
			state.isParsingFunction = true;

#if 0
			auto ftype = MirTypeConverters::SymbolToMirVar(jit::Symbol(f.id, f.returnType));
			auto numReturn = (int)f.returnType.getType() != Types::ID::Void;
			String funcDef;
			funcDef << f.id.getIdentifier() << ": func ";

			funcDef << MirTypeConverters::TypeInfo2MirTextType(f.returnType);


			for (auto& a : f.args)
			{
				funcDef << ", " << MirTypeConverters::TypeInfo2MirTextType(a.typeInfo) << ":" <<
								   MirTypeConverters::NamespacedIdentifier2MangledMirVar(a.id);
			}
#endif

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

			state.appendLine(line, false);
		}
		else
		{
			state.isParsingFunction = false;
			State::MirTextLine l;
			l.instruction = "endfunc";
			state.appendLine(l, false);
		}

		return Result::ok();

#else
		if (state.isPre())
		{
			auto sig = state.currentTree->getProperty("Signature").toString();

			auto f = MirTypeConverters::String2FunctionData(sig);
			auto ftype = MirTypeConverters::SymbolToMirVar(jit::Symbol(f.id, f.returnType));
			auto numReturn = (int)f.returnType.getType() != Types::ID::Void;

			Array<MIR_var_t> args;
			
			for (const auto& a : f.args)
				args.add(MirTypeConverters::SymbolToMirVar(a));

			state.currentFunc = MIR_new_func_arr(state.ctx, 
												 ftype.name,
												 numReturn, 
												 &ftype.type, 
												 args.size(), 
												 args.getRawDataPointer());

			for (const auto& a : args)
			{
				auto s = MIR_reg(state.ctx, a.name, state.currentFunc->u.func);
				auto id = NamespacedIdentifier(ftype.name).getChildId(a.name);
				state.registers.add(id, s, a);
			}
		}
		else
		{
			MIR_finish_func(state.ctx);
		}

		return Result::ok();
#endif
	}

	static Result ReturnStatement(State& state)
	{
#if CREATE_MIR_TEXT
		if (!state.isPre())
		{
			State::MirTextLine line;

			line.instruction = "ret";

			line.addOperands(state, { 0 }, { State::RegisterType::Value });

			state.appendLine(line, true);
		}

#else
		if (!state.isPre())
		{
			auto op = MIR_new_ret_insn(state.ctx, 1, state.createOpForCurrentChild(0));

			state.emitInstruction(op);
		}
#endif

		return Result::ok();
	}

	static Result BinaryOp(State& state)
	{
#if CREATE_MIR_TEXT
		

		if (!state.isPre())
		{
			State::MirTextLine line;

			auto type = state.getTypeForChild(0);
			auto opType = state.getProperty("OpType");

			line.addAnonymousReg(state, type, State::RegisterType::Value);
			line.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, opType);
			line.addOperands(state, { -1, 0, 1 });
			
			state.appendLine(line, true);
		}
#else
		if (!state.isPre())
		{
			auto type = state.createTempRegisterLikeChild();
			auto opType = state.getProperty("OpType");
			auto x = MirTypeConverters::MirTypeAndToken2InstructionCode(type, opType);
			auto inst = state.createInstruction(x, { -1, 0, 1 });
			state.emitInstruction(inst);
		}
#endif

		return Result::ok();
	}

	static Result VariableReference(State& state)
	{
#if CREATE_MIR_TEXT

		if (state.isPre())
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
						copy.v = *state.currentTree;
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
		}

#else
		if (state.isPre())
		{
			if (state.currentFunc != nullptr)
			{
				auto id = MirTypeConverters::String2NamespacedIdentifier(state.getProperty("Symbol"));

				if (state.registers.contains(id))
				{
					// We need to register the variable with the current value tree again...
					state.registers.addHash(id, *state.currentTree);
					
				}
				else
				{
					// We need to register a new variable to the valuetree and the namespaced ID

					auto symb = state.getProperty("Symbol");

					auto type = MirTypeConverters::String2Symbol(symb);
					auto v = MirTypeConverters::SymbolToMirVar(type);
					auto r = state.createVariable(v);

					state.registers.addHash(*state.currentTree, type.id);
					
					// Load the global data pointer to a memory operand
					auto ptr = state.dataPool.getAddress(v);
					auto anon = state.registers.getAnonymousId(MIR_fp_type_p(v.type));



					auto b = r;
					auto load = MIR_new_insn(state.ctx, MIR_MOV, MIR_new_reg_op(state.ctx, b), MIR_new_int_op(state.ctx, ptr));

					state.emitInstruction(load);

					if (v.type != MIR_T_P)
					{
						auto src = MIR_new_mem_op(state.ctx, v.type, 0, b, 0, Types::Helpers::getSizeForType(MirTypeConverters::MirType2TypeId(v.type)));
						state.registers.setMemoryOp(type.id, src);
					}
				}
			}
			else
			{
				// nothing to do here, it will get created in the parent assignment
			}
		}
#endif

		return Result::ok();
	}

	static Result Immediate(State& state)
	{
#if CREATE_MIR_TEXT

		if (state.isPre())
		{
			auto v = state.currentTree->getProperty("Value");

			auto id = Types::Helpers::getTypeFromStringValue(v);
			auto type = MirTypeConverters::TypeInfo2MirType(TypeInfo(id, false, false));
			state.registerCurrentTextOperand(v, type, State::RegisterType::Value);
		}

#else
		if (state.isPre())
		{
			auto v = state.currentTree->getProperty("Value");

			auto v2 = VariableStorage(Types::Helpers::getTypeFromStringValue(v.toString()), v);
			state.registers.addImmediate(*state.currentTree, v2);
		}
#endif

		return Result::ok();
	}

	static Result Cast(State& state)
	{
#if CREATE_MIR_TEXT
		if (!state.isPre())
		{
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

			state.appendLine(line, true);
		}


#else
		if (!state.isPre())
		{
			auto source = MirTypeConverters::String2MirType(state.getProperty("Source"));
			auto target = MirTypeConverters::String2MirType(state.getProperty("Target"));

			MIR_insn_code_t x;

			if(		 source == MIR_T_I64 && target == MIR_T_F) x = MIR_I2F;
			else if (source == MIR_T_F && target == MIR_T_I64) x = MIR_F2I;
			else if (source == MIR_T_I64 && target == MIR_T_D) x = MIR_I2D;
			else if (source == MIR_T_D && target == MIR_T_I64) x = MIR_D2I;
			else if (source == MIR_T_D && target == MIR_T_F)   x = MIR_D2F;
			else if (source == MIR_T_F && target == MIR_T_D)   x = MIR_F2D;

			state.createTempRegisterWithType(MirTypeConverters::MirType2TypeId(target));
			auto in = state.createInstruction(x, { -1, 0 });
			state.emitInstruction(in);
		}
#endif
		return Result::ok();
	}

	static Result Assignment(State& state)
	{
#if CREATE_MIR_TEXT

		if (!state.isPre())
		{
			if (state.isParsingFunction)
			{
				auto opType = state.getProperty("AssignmentType");
				auto t = state.getTypeForChild(0);



				State::MirTextLine l;

				l.addAnonymousReg(state, t, State::RegisterType::Value);

				l.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(t, opType);

				if (opType == JitTokens::assign_)
					l.addOperands(state, { 1, 0 });
				else
					l.addOperands(state, { 1, 1, 0 });

				state.appendLine(l, true);
			}
			else
			{
				State::MirTextLine e;
				e.instruction = "export";
				e.addOperands(state, { 1 }, { State::RegisterType::Raw }); 
				state.appendLine(e, false);
				State::MirTextLine l;
				l.label = state.getOperandForChild(1, State::RegisterType::Raw);
				l.instruction = MirTypeConverters::MirType2MirTextType(state.getTypeForChild(1));
				l.instruction;
				l.addOperands(state, { 0 });
				l.appendComment("global def ");

				state.appendLine(l, false);
			}
			
		}
#else
		if (!state.isPre())
		{
			if (state.currentFunc != nullptr)
			{
				auto opType = state.getProperty("AssignmentType");
				auto t = state.getTypeOfFirstChild();

				auto code = MirTypeConverters::MirTypeAndToken2InstructionCode(t.type, opType);

				MIR_insn_t in;

				if (opType == JitTokens::assign_)
					in = state.createInstruction(code, { 1, 0 });
				else
					in = state.createInstruction(code, { 1, 1, 0 });

				state.emitInstruction(in);
			}
			else
			{
				auto value = state.registers.getRegisterValue(state.currentTree->getChild(0));
				auto symbol = MirTypeConverters::String2Symbol(state.currentTree->getChild(1).getProperty("Symbol"));

				state.dataPool.add(symbol, value);

				
			}
		}
#endif

		return Result::ok();
	}

	static Result Comparison(State& state)
	{
#if CREATE_MIR_TEXT
		if (!state.isPre())
		{
			auto opType = state.getProperty("OpType");
			auto type = state.getTypeForChild(0);
			
			State::MirTextLine l;
			l.addAnonymousReg(state, type, State::RegisterType::Value);
			l.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, opType);
			l.addOperands(state, { -1, 0, 1 });

			state.appendLine(l, true);
		}
#else
		if (!state.isPre())
		{
			state.dump();
			auto opType = state.getProperty("OpType");
			auto type = state.getTypeOfFirstChild();

			auto t2 = state.registers.getRegisterType(state.currentTree->getChild(1));

			auto code = MirTypeConverters::MirTypeAndToken2InstructionCode(type.type, opType);
			state.createTempRegisterWithType(Types::ID::Integer);
			auto in = state.createInstruction(code, { -1, 0, 1 });
			state.emitInstruction(in);
		}
#endif
		

		return Result::ok();
	}

	static Result TernaryOp(State& state)
	{
#if CREATE_MIR_TEXT
		if (state.isPre())
		{
			auto type = state.getTypeForChild(1);

			State::MirTextLine l;
			l.addAnonymousReg(state, type, State::RegisterType::Value);
			state.appendLine(l, false);

			state.registerTextLabel(1);
			state.registerTextLabel(2);
		}
		if (!state.isPre())
		{
			auto tl = state.getLabel(1);
			auto fl = state.getLabel(2);

			auto type = state.getTypeForChild(1);

			{
				// add the move from true line
				State::MirTextLine tb;

				tb.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
				tb.addOperands(state, { -1, 0 });

				state.insertLine(tb, fl, true);
			}
			{
				// add the jump to end line
				// add the move from true line
				State::MirTextLine jmp;

				jmp.instruction = "jmp";
				jmp.operands.add("FUNKY");

				state.insertLine(jmp, fl, true);
				
			}
			{
				// add the move from false line
				State::MirTextLine tb;

				tb.instruction = MirTypeConverters::MirTypeAndToken2InstructionText(type, JitTokens::assign_);
				tb.addOperands(state, { -1, 0 });

				state.insertLine(tb, fl, true);
			}
			{
				// add the end label line;
				State::MirTextLine l;

				state.registerTextLabel(-1);

				auto label = state.getLabel(-1);

				l.instruction = "mov";
				l.addOperands(state, { 0, 0 }, { State::RegisterType::Raw, State::RegisterType::Raw });
				
				state.appendLine(l, true);
			}

			
		}

		return Result::ok();

#else
		if (!state.isPre())
		{
			auto type = state.createTempRegisterLikeChild(1);

			auto false_l = MIR_new_label(state.ctx);
			auto end_l = MIR_new_label(state.ctx);

			auto cond = state.getLastInstruction(0);
			auto true_b = state.getLastInstruction(1);
			auto false_b = state.getLastInstruction(2);
			
			auto assignOp = MirTypeConverters::MirTypeAndToken2InstructionCode(type, JitTokens::assign_);

			auto jumpToFalse = MIR_new_insn(state.ctx, MIR_BF, MIR_new_label_op(state.ctx, false_l), state.createOpForCurrentChild(0));
			auto jumpToEnd = MIR_new_insn(state.ctx, MIR_JMP, MIR_new_label_op(state.ctx, end_l));

			// Attach the jump to end label after the condition
			state.emitInstructionAfter(jumpToFalse, cond);

			// If the true branch has no instructions, we start after the jump to false
			if (!true_b)
				true_b = jumpToFalse;

			// Attach the store true to the true branch
			auto storeTrue = state.createInstruction(assignOp, { -1, 1 });
			state.emitInstructionAfter(storeTrue, true_b);
			state.emitInstructionAfter(jumpToEnd, storeTrue);
			state.emitInstructionAfter(false_l, jumpToEnd);

			// if the false branch has no instructions, we start after the jump to end
			if (!false_b)
				false_b = false_l;

			// Attach the store false value
			auto storeFalse = state.createInstruction(assignOp, { -1, 2 });
			state.emitInstructionAfter(storeFalse, false_b);

			// Attach the end label
			state.emitInstruction(end_l);
		}

		return Result::ok();
#endif
	}

	static Result IfStatement(State& state)
	{
		if (!state.isPre())
		{
			auto false_l = MIR_new_label(state.ctx);
			auto end_l = MIR_new_label(state.ctx);

			auto cond = state.getLastInstruction(0);
			auto true_b = state.getLastInstruction(1);
			auto false_b = state.getLastInstruction(2);

			auto hasFalseBranch = false_b != nullptr;

			if (!hasFalseBranch)
			{
				auto jumpToEnd = MIR_new_insn(state.ctx, MIR_BF, MIR_new_label_op(state.ctx, end_l), state.createOpForCurrentChild(0));
				state.emitInstructionAfter(jumpToEnd, cond);
				state.emitInstructionAfter(end_l, true_b);
			}
			else
			{
				auto jumpToFalse = MIR_new_insn(state.ctx, MIR_BF, MIR_new_label_op(state.ctx, false_l), state.createOpForCurrentChild(0));

				state.emitInstructionAfter(jumpToFalse, cond);

				auto jumpToEnd = MIR_new_insn(state.ctx, MIR_JMP, MIR_new_label_op(state.ctx, end_l));
				state.emitInstructionAfter(jumpToEnd, true_b);
				state.emitInstructionAfter(false_l, jumpToEnd);
				state.emitInstructionAfter(end_l, false_b);
			}

			
		}
		
		return Result::ok();
	}

	static Result LogicalNot(State& state)
	{
#if CREATE_MIR_TEXT
		if (!state.isPre())
		{
			State::MirTextLine l;
			l.addAnonymousReg(state, MIR_T_I64, State::RegisterType::Value);
			l.instruction = "eq";
			l.addOperands(state, { -1, 0 });
			l.addImmOperand(VariableStorage(0));
			
			state.appendLine(l, true);
		}
#else
		if (!state.isPre())
		{
			state.createTempRegisterWithType(Types::ID::Integer);
			auto in = MIR_new_insn(state.ctx, MIR_EQ, state.createOpForCurrentChild(-1), state.createOpForCurrentChild(0), MIR_new_int_op(state.ctx, 0));
			state.emitInstruction(in);
		}
#endif
		
		return Result::ok();
	}

	static Result WhileLoop(State& state)
	{
		if (state.isPre())
		{
			auto loop_markers = state.pushLoop();

			state.emitInstruction(loop_markers[0]);
		}

		if (!state.isPre())
		{
			state.dump();
			auto cond = state.getLastInstruction(0);
			auto body = state.getLastInstruction(1);

			auto loop_markers = state.popLoop();

			auto jumpToEnd = MIR_new_insn(state.ctx, MIR_BF, MIR_new_label_op(state.ctx, loop_markers[1]), state.createOpForCurrentChild(0));
			auto jumpToBegin = MIR_new_insn(state.ctx, MIR_JMP, MIR_new_label_op(state.ctx, loop_markers[0]));

			state.emitInstructionAfter(jumpToEnd, cond);
			state.emitInstructionAfter(jumpToBegin, body);
			state.emitInstructionAfter(loop_markers[1], jumpToBegin);
		}

		return Result::ok();
	}

	static Result Increment(State& state)
	{
#if CREATE_MIR_TEXT
		if (!state.isPre())
		{
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
				state.appendLine(add_l, true);
				state.appendLine(mov_l, false);
			}
			else
			{
				state.appendLine(mov_l, true);
				state.appendLine(add_l, false);
			}
		}
#else
		if (!state.isPre())
		{
			auto isPre = state.getProperty("IsPre") == "1";
			auto isDec = state.getProperty("IsDec") == "1";

			state.createTempRegisterLikeChild(0);

			auto delta = MIR_new_int_op(state.ctx, isDec ? -1 : 1);

			auto t = state.createOpForCurrentChild(0);
			auto self = state.createOpForCurrentChild(-1);

			if (isPre)
			{
				state.emitInstruction(MIR_new_insn(state.ctx, MIR_ADD, t, t, delta));
				state.emitInstruction(MIR_new_insn(state.ctx, MIR_MOV, self, t));
			}
			else
			{
				state.emitInstruction(MIR_new_insn(state.ctx, MIR_MOV, self, t));
				state.emitInstruction(MIR_new_insn(state.ctx, MIR_ADD, t, t, delta));			
			}
		}
#endif

		return Result::ok();
	}

	static Result StatementBlock(State& state)
	{
		// Nothing to do here (until we need the scope of a block...)
#if CREATE_MIR_TEXT
		if (state.isPre())
		{
			state.dump();

			Array<Symbol> localSymbols;

			auto scopeId = NamespacedIdentifier::fromString(state.getProperty("ScopeId"));

			MirTypeConverters::forEachChild(*state.currentTree, [&](const ValueTree& v)
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

			state.appendLine(local_def_l, true);
		}
#else

		if (state.isPre())
		{
			
		}

		if (!state.isPre())
		{
			// We need to store the last instruction of the child list as last instruction of itself.
			auto lastInstruction = state.getLastInstruction(state.currentTree->getNumChildren() - 1);
			state.instructions.add({ *state.currentTree, lastInstruction });
		}
#endif

		return Result::ok();
	}

	static Result ComplexTypeDefinition(State& state)
	{
#if CREATE_MIR_TEXT

		if (!state.isPre())
		{
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
					
					state.appendLine(l, false);
				});

				jassertfalse;
#if 0
				auto useI64 = (mis.getNumBytesRemaining() % 8 == 0);

				l.instruction = useI64 ? "i64" : "i32";

				while(!mis.isExhausted())
					l.operands.add(String(useI64 ? mis.readInt64() : mis.readInt()));
#endif
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
				
				state.appendLine(l, true);

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
					{
						il.addImmOperand(v);
					}

					

					state.appendLine(il, false);
				});

				
					

				

				
				
			}
		}

#else
		if (state.isPre())
		{
			MemoryBlock mb;
			mb.fromBase64Encoding(state.getProperty("InitValues"));

			if (state.currentFunc == nullptr)
			{
				auto ptr = (int*)mb.getData();
				
				Symbol s;
				s.typeInfo = TypeInfo(Types::ID::Pointer, true, false);
				s.id = NamespacedIdentifier::fromString(state.getProperty("Ids").upToFirstOccurrenceOf(",", false, false).trim());

				state.dataPool.add(s, VariableStorage(mb.getData(), mb.getSize()));

				// global definition
			}
			else
			{
				// stack definition
			}

		}
#endif

		return Result::ok();
	}

	static Result Subscript(State& state)
	{
#if CREATE_MIR_TEXT
		if (!state.isPre())
		{
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

			state.appendLine(l, true);

			State::MirTextLine idx;

			auto idx_reg = idx.addAnonymousReg(state, MIR_T_I64, State::RegisterType::Value);

			idx.instruction = "mov";
			idx.operands.add(idx_reg);
			idx.addOperands(state, { 1 });

			auto scale = state.getProperty("ElementSize").getIntValue();
			
			state.appendLine(idx, false);

			State::MirTextLine scaleLine;

			scaleLine.instruction = "mul";
			scaleLine.operands.add(idx_reg);
			scaleLine.operands.add(idx_reg);
			scaleLine.addImmOperand(scale);

			state.appendLine(scaleLine, false);

			State::MirTextLine offset_l;
			offset_l.instruction = "add";
			offset_l.operands.add(self);
			offset_l.operands.add(self);
			offset_l.operands.add(idx_reg);

			state.appendLine(offset_l, false);

		}

#else
		if (!state.isPre())
		{
			auto t = MirTypeConverters::String2Symbol(state.getProperty("ElementType"));

			auto mir_t = MirTypeConverters::TypeInfo2MirType(t.typeInfo);

			state.createTempRegisterWithType(t.typeInfo.getType());
			auto scale = state.getProperty("ElementSize").getIntValue();
			
			
			auto index = state.registers.getRegisterIndex(state.currentTree->getChild(1));
			
			if (mir_t == MIR_T_P)
			{
				auto offset_reg = state.createTempRegisterWithType(Types::ID::Integer, false);
				auto offset_op = MIR_new_reg_op(state.ctx, offset_reg);

				auto scale_op = MIR_new_int_op(state.ctx, scale);

				state.emitInstruction(MIR_new_insn(state.ctx, MIR_MOV, offset_op, MIR_new_reg_op(state.ctx, index)));
				state.emitInstruction(MIR_new_insn(state.ctx, MIR_MUL, offset_op, offset_op, scale_op));

				state.emitInstruction(MIR_new_insn(state.ctx, MIR_ADD, state.createOpForCurrentChild(-1), state.createOpForCurrentChild(0), offset_op));
			}
			else
			{
				auto base = state.registers.getRegisterIndex(state.currentTree->getChild(0));

				auto src = MIR_new_mem_op(state.ctx, mir_t, 0, base, index, scale);
				auto op = MirTypeConverters::MirTypeAndToken2InstructionCode(mir_t, JitTokens::assign_);
				auto in = MIR_new_insn(state.ctx, op, state.createOpForCurrentChild(-1), src);
				state.emitInstruction(in);
			}			
		}
#endif

		return Result::ok();
	}

	static Result InternalProperty(State& state)
	{
		return Result::ok();
	}
};

struct InstructionCollection
{
	void registerFunction(const Identifier& id, const State::ValueTreeFuncton& f)
	{
		functions.emplace(id, f);
	}

	std::map<juce::Identifier, State::ValueTreeFuncton> functions;
};

#define REGISTER_TYPE(X) functions->registerFunction(InstructionIds::X, InstructionParsers::X);

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
	jassertfalse;

	functions = new InstructionCollection();
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
}

#undef REGISTER_TYPE

MirBuilder::~MirBuilder()
{
	delete functions;
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

juce::Result MirBuilder::pre(const ValueTree& p)
{
	if (auto f = functions->functions[p.getType()])
	{
		currentState->order = State::CallOrder::Pre;
		currentState->currentTree = &p;
		return f(*currentState);
	}
	
	DBG(p.createXml()->createDocument(""));

	return Result::fail("unknown tree " + p.getType());
}

juce::Result MirBuilder::post(const ValueTree& p)
{
	if (auto f = functions->functions[p.getType()])
	{
		currentState->order = State::CallOrder::Post;
		currentState->currentTree = &p;

		return f(*currentState);
	}
	
	return Result::fail("unknown tree " + p.getType());
}

juce::Result MirBuilder::parseInternal(const ValueTree& v)
{
	auto ok = pre(v);

	if (checkAndFinish(ok))
		return ok;

	for (const auto& c : v)
	{
		ok = parseInternal(c);
		if (checkAndFinish(ok))
			return ok;
	}

	ok = post(v);

	if (checkAndFinish(ok))
		return ok;

	return ok;
}

bool MirBuilder::checkAndFinish(const Result& r)
{
	if (r.failed())
	{
		if (currentState->currentFunc)
		{
			MIR_finish_func(currentState->ctx);
			currentState->currentFunc = nullptr;
		}

		if (currentState->currentModule)
		{
			MIR_finish_module(currentState->ctx);
			currentState->currentModule = nullptr;
		}
		
		return true;
	}

	return r.failed();
}

}
}
