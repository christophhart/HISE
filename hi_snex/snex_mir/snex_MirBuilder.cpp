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

#include "mir.h"
#include "mir-gen.h"

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
}

#undef DEFINE_ID

struct MirTypeConverters
{
	

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
		auto m = t.toMirType();

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

	static jit::Symbol String2Symbol(const String& symbolCode)
	{
		auto t = symbolCode.upToFirstOccurrenceOf(" ", false, false);
		auto n = symbolCode.fromFirstOccurrenceOf(" ", false, false);
		auto ti = TypeInfo(Types::Helpers::getTypeFromTypeName(t));

		return Symbol(NamespacedIdentifier::fromString(n), ti);
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

struct RegisterDatabase
{
	template <typename T> void addImmediate(const T& v, const VariableStorage& value)
	{
		Item n;

		n.hashes.add(MirTypeConverters::getHash(v));
		n.value = value;
		n.type.type = MirTypeConverters::TypeInfo2MirType(TypeInfo(value.getType()));
		
		items.add(n);
	}

	template <typename T> void add(const T& v, MIR_reg_t reg, MIR_var_t type)
	{
		auto h = MirTypeConverters::getHash(v);

		if (auto existing = get(h))
		{
			return;
		}

		Item n;

		n.hashes.add(h);

		n.reg = reg;
		n.type = type;

		items.add(n);
	}

	template <typename T> MIR_reg_t getRegisterIndex(const T& v, bool throwIfFailure = true)
	{
		if (auto item = get(MirTypeConverters::getHash(v)))
			return item->reg;

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

	template <typename T> MIR_op_t createOp(MIR_context_t ctx, const T& v)
	{
		if (auto existing = get(MirTypeConverters::getHash(v)))
		{
			if (existing->useMem)
				return existing->memOp;

			VariableStorage imm = getRegisterValue(v);

			if (imm.isVoid())
				return MIR_new_reg_op(ctx, getRegisterIndex(v));
			else
			{
				if (imm.getType() == Types::ID::Integer)
					return MIR_new_int_op(ctx, imm.toInt());
				if (imm.getType() == Types::ID::Float)
					return MIR_new_float_op(ctx, imm.toFloat());
				if (imm.getType() == Types::ID::Double)
					return MIR_new_double_op(ctx, imm.toDouble());
			}
		}

		throw String("Can't find register");
	}

	String getAnonymousId() const
	{
		String s;
		s << "anon" << String(items.size());
		return s;
	}

private:

	int counter = 0;

	struct Item
	{
		Array<int64> hashes;
		MIR_reg_t reg;
		MIR_var_t type;
		VariableStorage value;
		MIR_op_t memOp;
		bool useMem = false;
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
			auto numBytes = Types::Helpers::getSizeForType(value.getType());

			auto ni = new MirDataPool::Item();

			ni->symbol = MirTypeConverters::SymbolToMirVar(symbol);
			ni->value = value;

			ni->item = MIR_new_data(parent.ctx, ni->symbol.name, ni->symbol.type, 1, ni->value.getDataPointer());
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

	State() :
		dataPool(*this)
	{};

	RegisterDatabase registers;

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
		auto id = registers.getAnonymousId();
		type.name = id.getCharPointer().getAddress();
		
		createVariable(type);
		return type.type;
	}

	MIR_type_t createTempRegisterWithType(Types::ID typeId)
	{
		MIR_var_t type;

		auto id = registers.getAnonymousId();
		type.name = id.getCharPointer().getAddress();
		type.type = MirTypeConverters::TypeInfo2MirType(TypeInfo(typeId));

		createVariable(type);
		return type.type;
	}
	
	MIR_reg_t createVariable(MIR_var_t type)
	{
		auto r = MIR_new_func_reg(ctx, currentFunc->u.func, type.type, type.name);
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

		return registers.createOp(ctx, c);
	}

	
};

static int64 funky = 1205;

struct InstructionParsers
{
	static Result SyntaxTree(State& state)
	{
		if (!state.currentTree->getParent().isValid())
		{
			if (state.isPre())
				state.currentModule = MIR_new_module(state.ctx, "main");
			else
				MIR_finish_module(state.ctx);
		}

		return Result::ok();
	}

	static Result Function(State& state)
	{
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
	}

	static Result ReturnStatement(State& state)
	{
		if (!state.isPre())
		{
			auto op = MIR_new_ret_insn(state.ctx, 1, state.createOpForCurrentChild(0));

			state.emitInstruction(op);
		}

		return Result::ok();
	}

	static Result BinaryOp(State& state)
	{
		if (!state.isPre())
		{
			auto type = state.createTempRegisterLikeChild();
			auto opType = state.getProperty("OpType");
			auto x = MirTypeConverters::MirTypeAndToken2InstructionCode(type, opType);
			auto inst = state.createInstruction(x, { -1, 0, 1 });
			state.emitInstruction(inst);
		}

		return Result::ok();
	}

	static Result VariableReference(State& state)
	{
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
					auto type = MirTypeConverters::String2Symbol(state.getProperty("Symbol"));
					auto v = MirTypeConverters::SymbolToMirVar(type);
					auto r = state.createVariable(v);

					state.registers.addHash(*state.currentTree, type.id);
					
					// Load the global data pointer to a memory operand
					auto ptr = state.dataPool.getAddress(v);
					auto anon = state.registers.getAnonymousId();
					auto b = MIR_new_func_reg(state.ctx, state.currentFunc->u.func, MIR_T_I64, anon.getCharPointer().getAddress());
					auto load = MIR_new_insn(state.ctx, MIR_MOV, MIR_new_reg_op(state.ctx, b), MIR_new_int_op(state.ctx, ptr));

					state.emitInstruction(load);

					auto src = MIR_new_mem_op(state.ctx, v.type, 0, b, 0, Types::Helpers::getSizeForType(MirTypeConverters::MirType2TypeId(v.type))); 
		
					state.registers.setMemoryOp(type.id, src);
				}
			}
			else
			{
				// nothing to do here, it will get created in the parent assignment
			}
		}
		return Result::ok();
	}

	static Result Immediate(State& state)
	{
		if (state.isPre())
		{
			auto v = state.currentTree->getProperty("Value");

			auto v2 = VariableStorage(Types::Helpers::getTypeFromStringValue(v.toString()), v);
			state.registers.addImmediate(*state.currentTree, v2);
		}

		return Result::ok();
	}

	static Result Cast(State& state)
	{
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

		return Result::ok();
	}

	static Result Assignment(State& state)
	{
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

		return Result::ok();
	}

	static Result Comparison(State& state)
	{
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
		

		return Result::ok();
	}

	static Result TernaryOp(State& state)
	{
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
		if (!state.isPre())
		{
			state.createTempRegisterWithType(Types::ID::Integer);
			auto in = MIR_new_insn(state.ctx, MIR_EQ, state.createOpForCurrentChild(-1), state.createOpForCurrentChild(0), MIR_new_int_op(state.ctx, 0));
			state.emitInstruction(in);
		}
		
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

		return Result::ok();
	}

	static Result StatementBlock(State& state)
	{
		// Nothing to do here (until we need the scope of a block...)

		if (!state.isPre())
		{
			// We need to store the last instruction of the child list as last instruction of itself.
			auto lastInstruction = state.getLastInstruction(state.currentTree->getNumChildren() - 1);
			state.instructions.add({ *state.currentTree, lastInstruction });
		}

		return Result::ok();
	}

	static Result ComplexTypeDefinition(State& state)
	{
		if (state.isPre())
		{
			auto initData = state.getProperty("InitData");
		}

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