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

#pragma once

namespace snex {
namespace mir {

using namespace juce;


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

	struct LoopLabelSet
	{
		String startLabel;
		String endLabel;
		String continueLabel;
	};

	Array<LoopLabelSet> labelPairs;
};

enum class RegisterType
{
	Raw,
	Value,
	Pointer,
	numRegisterTypes
};

struct TextOperand
{
	ValueTree v;
	String text;
	String stackPtr;
	MIR_type_t type;
	RegisterType registerType = RegisterType::Value;
};

struct State
{
	using ValueTreeInliner = std::function<TextOperand(State&, const ValueTree&, const ValueTree&)>;

	State() = default;

	Array<ValueTree> dataList;

	void setDataLayout(const String& b64)
	{
		dataList = SyntaxTreeExtractor::getDataLayoutTrees(b64);

		for (const auto& l : dataList)
		{
			DBG(l.createXml()->createDocument(""));

			Array<MirMemberInfo> members;

			for (const auto& m : l)
			{
				if (m.getType() == Identifier("Member"))
				{
					auto id = m["ID"].toString();
					auto type = Types::Helpers::getTypeFromTypeName(m["type"].toString());
					auto mir_type = TypeConverters::TypeInfo2MirType(TypeInfo(type));
					auto offset = (size_t)(int)m["offset"];

					members.add({ id, mir_type, offset });
				}
			}

			classTypes.emplace(Identifier(l["ID"].toString()), std::move(members));
		}
	}

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
			l.operands.add(TypeConverters::TypeInfo2MirTextType(f.returnType));

		if (addObjectPointer)
		{
			l.operands.add("i64:_this_");
		}

		for (const auto& a : f.args)
			l.operands.add(TypeConverters::Symbol2MirTextSymbol(a));

		emitLine(l);

		prototypes.add(f);
	}

	Array<FunctionData> prototypes;

	std::map<String, ValueTreeInliner> inlinerFunctions;

	ValueTreeInliner getInliner(const FunctionData& f)
	{
		auto l = TypeConverters::FunctionData2MirTextLabel(f);
		return inlinerFunctions[l];
	}

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



	struct MirMemberInfo
	{
		String id;
		MIR_type_t type;
		size_t offset;
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

			auto t = TypeInfo(TypeConverters::MirType2TypeId(type), true);

			auto tn = TypeConverters::TypeInfo2MirTextType(t);

			if (rt == RegisterType::Pointer || type == MIR_T_P)
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

	void emitSingleInstruction(const String& instruction, const String& label = {})
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

		if (registerAsCurrentStatementReg)
			registerCurrentTextOperand(targetName, MIR_T_I64, RegisterType::Pointer);

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
		if (isParsingFunction)
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

	TextOperand getTextOperandForValueTree(const ValueTree& c)
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


			auto vt = TypeConverters::MirType2MirTextType(t.type);


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

	Array<TextOperand> localOperands;
	Array<TextOperand> globalOperands;

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

	bool hasPrototype(const FunctionData& sig) const
	{
		auto thisLabel = TypeConverters::FunctionData2MirTextLabel(sig);

		for (const auto& p : prototypes)
		{
			auto pLabel = TypeConverters::FunctionData2MirTextLabel(p);

			if (pLabel == thisLabel)
				return true;
		}

		return false;
	}

	String getPrototype(const FunctionData& sig) const
	{
		auto thisLabel = TypeConverters::FunctionData2MirTextLabel(sig);

		int l = 0;

		for (const auto& p : prototypes)
		{
			auto pLabel = TypeConverters::FunctionData2MirTextLabel(p);

			if (pLabel == thisLabel)
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
		if (getRegisterTypeForChild(childIndex) == RegisterType::Pointer ||
			getTypeForChild(childIndex) == MIR_T_P)
		{
			auto t = getTypeForChild(childIndex);

			State::MirTextLine load;
			load.instruction = "mov";

			auto id = "p" + String(counter++);

			load.localDef << "i64:" << id;
			load.operands.add(id);
			load.addOperands(*this, { childIndex }, { RegisterType::Pointer });

			emitLine(load);

			if (targetType == RegisterType::Pointer)
				return id;
			else
			{
				String ptr;

				auto ptr_t = TypeConverters::MirType2MirTextType(t);

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

}
}
