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




enum class RegisterType
{
	Raw,
	Value,
	Pointer,
	numRegisterTypes
};

struct TextOperand
{
    operator bool() const
    {
        return v.isValid() && text.isNotEmpty();
    }
    
	ValueTree v;
	String text;
	String stackPtr;
	MIR_type_t type;
	RegisterType registerType = RegisterType::Value;
};

struct MemberInfo
{
    MemberInfo(const String& id_, const MIR_type_t t_, size_t o_):
      id(id_),
      type(t_),
      offset(o_)
    {};
    
	const String id;
	const MIR_type_t type;
	const size_t offset;
};

struct State;

struct TextLine
{
	TextLine(State* s, const String& instruction = {});

	~TextLine();

	void addImmOperand(const VariableStorage& value);

	void addRawOperand(const String& op)
	{
		operands.add(op);
	}

	template <typename T> void addSelfOperand(bool registerAsCurrent=false)
	{
		auto t = TypeConverters::getMirTypeFromT<T>();

		auto s = addAnonymousReg(t, RegisterType::Value, registerAsCurrent);
		operands.add(s);
	}

	String addAnonymousReg(MIR_type_t type, RegisterType rt, bool registerAsCurrentOp=true);
	void addSelfAsValueOperand();
	void addSelfAsPointerOperand();
	void addChildAsPointerOperand(int childIndex);
	void addChildAsValueOperand(int childIndex);
	void addOperands(const Array<int>& indexes, const Array<RegisterType>& registerTypes = {});

	void appendComment(const String& c) { comment = c; }

	int getLabelLength() const;
	String toLine(int maxLabelLength) const;

	String flush();

	bool flushed = false;
	State* state;
	String label;
	String localDef;
	String instruction;
	StringArray operands;
	String comment;
};

struct LoopManager
{
	~LoopManager();

	void pushLoopLabels(String& startLabel, String& endLabel, String& continueLabel);
	String getCurrentLabel(const String& instructionId);
	void popLoopLabels();
	String makeLabel() const;

    void pushInlineFunction(String& l, MIR_type_t type = MIR_T_I8, RegisterType rType=RegisterType::Value, const String& returnRegName={});

	void popInlineFunction()
	{
		inlineFunctionData.removeLast(1);
	}

	void addInlinedArgument(const String& symbol, const TextOperand& op)
	{
        inlineFunctionData.getReference(inlineFunctionData.size() - 1).args.add({symbol, op});
	}

    TextOperand getInlinedThis() const
    {
        for(int i = inlineFunctionData.size()-1; i >= 0; i--)
        {
            const auto& s = inlineFunctionData.getReference(i);
            
            for(const auto& a: s.args)
            {
                if(a.symbol == "unresolved this")
                    return a.op;
            }
        }
        
        return {};
    }
    
	TextOperand getInlinedParameter(const String& argumentId)
	{
        for(int i = inlineFunctionData.size()-1; i >= 0; i--)
        {
            const auto& s = inlineFunctionData.getReference(i);
            
            for(auto& a: s.args)
            {
                if(a.symbol == argumentId)
                    return a.op;
            }
        }
        
        jassertfalse;
        return {};
	}

	void emitInlinedReturn(State* s);

private:

	mutable int labelCounter = 0;

    struct InlineArgument
    {
        String symbol;
        TextOperand op;
    };
    
	struct InlineFunctionData
	{
		String endLabel;
		String returnReg;
        RegisterType registerType;
		MIR_type_t type;
        Array<InlineArgument> args;
	};

	struct LoopLabelSet
	{
		String startLabel;
		String endLabel;
		String continueLabel;
	};

	Array<InlineFunctionData> inlineFunctionData;

	Array<LoopLabelSet> labelPairs;
};

struct InlinerManager
{
	InlinerManager(State* s) :
		state(s)
	{};

	using ValueTreeInliner = std::function<TextOperand(State*, const ValueTree&, const ValueTree&)>;

	ValueTreeInliner getInliner(String fid)
	{
		return inlinerFunctions[fid];
	}

	void registerInliner(const String& fid, const ValueTreeInliner& f)
	{
		inlinerFunctions.emplace(fid, f);
	}

	TextOperand emitInliner(const String& inlinerLabel, const String& className, const String& methodName, const StringArray& operands);


private:

	State* state;
	std::map<String, ValueTreeInliner> inlinerFunctions;
};

struct RegisterManager
{
	RegisterManager(State* state);

	String getAnonymousId(bool isFloat) const;

	void emitMultiLineCopy(const String& targetPointerReg, const String& sourcePointerReg, int numBytesToCopy);

	int allocateStack(const String& targetName, int numBytes, bool registerAsCurrentStatementReg);

	void registerCurrentTextOperand(String n, MIR_type_t type, RegisterType rt);

	/** Use this whenever you need a child operand as a register that might be a address.
		This will take the address, emit a load operation and return a pointer instruction using the new register. */
	String loadIntoRegister(int childIndex, RegisterType targetType);

	TextOperand getTextOperandForValueTree(const ValueTree& c);

	RegisterType getRegisterTypeForChild(int index);

	MIR_type_t getTypeForChild(int index);

	String getOperandForChild(int index, RegisterType requiredType);

    bool hasOperandWithName(const String& name) const
    {
        for(const auto& l: localOperands)
        {
            if(l.text == name)
                return true;
        }
        
        return false;
    }
    
	void startFunction()
	{
		currentlyParsingFunction = true;
	}

	void endFunction()
	{
		currentlyParsingFunction = false;
		localOperands.clear();
	}

	Array<TextOperand> localOperands;
	Array<TextOperand> globalOperands;

	bool isParsingFunction() const { return currentlyParsingFunction; };

private:

	mutable int counter = 0;
	State* state;
	bool currentlyParsingFunction = false;
};

struct FunctionManager
{
	void addPrototype(State* state, const NamespacedIdentifier& objectType, const FunctionData& f, bool addObjectPointer, int returnBlockSize=-1);

	bool hasPrototype(const NamespacedIdentifier& objectType, const FunctionData& sig) const;

	String getPrototype(const NamespacedIdentifier& objectType, const String& fullSignature) const;

	String getIdForComplexTypeOverload(const NamespacedIdentifier& objectType, const String& fullSignature) const
	{
		for (const auto& f : specialOverloads)
		{
			if (f.fullSignature == fullSignature)
				return f.mangledId;
		}

		auto sig = TypeConverters::String2FunctionData(fullSignature);
		return TypeConverters::FunctionData2MirTextLabel(objectType, sig);
	}

	String registerComplexTypeOverload(State* state, const NamespacedIdentifier& objectType, const String& fullSignature, bool addObjectPtr);

    void addLocalVarDefinition(const String& lv)
    {
        localVarDefinitions.add(lv);
    }
    
    void clearLocalVarDefinitions()
    {
        localVarDefinitions.clear();
    }
    
    
    bool hasLocalVar(const String& lv) const
    {
        return localVarDefinitions.contains(lv);
    }
    
private:

    StringArray localVarDefinitions;
    
	struct ComplexTypeOverload
	{
		ComplexTypeOverload(const NamespacedIdentifier& objectType_, const String& fullSignature_);;

		NamespacedIdentifier objectType;
		String fullSignature;
		String mangledId;
		String prototype;
	};

	Array<ComplexTypeOverload> specialOverloads;

	Array<std::tuple<NamespacedIdentifier, FunctionData>> prototypes;
};

struct DataManager
{
	void setDataLayout(const Array<ValueTree>& dl);

	void startClass(const NamespacedIdentifier& nid,Array<MemberInfo>&& memberInfo);

	void endClass();

	void registerClass(const Identifier& id, Array<MemberInfo>&& memberInfo);

	ValueTree getDataObject(const String& type) const;

    void addGlobalData(const String& id, const String& type)
    {
        globalObjects.emplace(id, type);
    }
    
    ValueTree getGlobalData()
    {
        ValueTree v("GlobalData");
        
        for(auto& k: globalObjects)
        {
            auto t = getDataObject(k.second).createCopy();
            
            if(t.isValid())
            {
                t.setProperty("ObjectId", k.first, nullptr);
                v.addChild(t, -1, nullptr);
            }
            else
            {
                ValueTree nt("NativeType");
                nt.setProperty("ObjectId", k.first, nullptr);
                nt.setProperty("type", k.second, nullptr);
                v.addChild(nt, -1, nullptr);
            }
        }
        
        return v;
    }
    
	size_t getNumBytesRequired(const Identifier& id);

	const Array<MemberInfo>& getClassType(const Identifier& id);

	bool isParsingClass() const { return numCurrentlyParsedClasses > 0; }

	NamespacedIdentifier getCurrentClassType() const
	{
		auto id = NamespacedIdentifier::fromString(currentClassTypes.getFirst());

		for (int i = 1; i < currentClassTypes.size(); i++)
			id = id.getChildId(currentClassTypes[i]);
		
		return id;
	}

private:

	Array<String> currentClassTypes;

    std::map<String, String> globalObjects; // ID => type
    
	int numCurrentlyParsedClasses = 0;
	std::map<juce::Identifier, Array<MemberInfo>> classTypes;
	Array<ValueTree> dataList;
};

struct InstructionManager
{
	using ValueTreeFuncton = std::function<Result(State* state)>;

	void registerInstruction(const Identifier& id, const ValueTreeFuncton& f)
	{
		instructions.emplace(id, f);
	}

	Result perform(State* state, const ValueTree& v);

private:

	std::map<juce::Identifier, ValueTreeFuncton> instructions;
};

struct State
{
	State() :
		registerManager(this),
		inlinerManager(this)
	{};

	LoopManager loopManager;
	InlinerManager inlinerManager;
	RegisterManager registerManager;
	FunctionManager functionManager;
	DataManager dataManager;
	InstructionManager instructionManager;

	MIR_context_t ctx;
	MIR_module_t currentModule = nullptr;
	ValueTree currentTree;
	
	Array<TextLine> lines;

	String operator[](const Identifier& id) const;

	void dump() const;
	void emitSingleInstruction(const String& instruction, const String& label = {});
	void emitLabel(const String& label, const String& optionalComment = {});

	bool isParsingClass() const { return dataManager.isParsingClass(); };
	bool isParsingFunction() const { return registerManager.isParsingFunction(); }

	ValueTree getCurrentChild(int index);
	void processChildTree(int childIndex);
	void processAllChildren();
	Result processTreeElement(const ValueTree& v);

	String toString(bool addTabs);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(State);
};

#define INSTRUCTION2(x) template <typename T1> void x(const String& op1, const T1& op2) { emit(#x, _operands(op1, op2)); }
#define INSTRUCTION3(x) template <typename T1, typename T2> void x(const String& op1, const T1& op2, const T2& op3) { emit(#x, _operands(op1, op2, op3)); }

struct MirCodeGenerator
{
	MirCodeGenerator(mir::State* state_) :
		state(*state_),
		rm(state_->registerManager)
	{};

	virtual ~MirCodeGenerator() {}

	template <typename T, typename OffsetType = int> String deref(const String& pointerOperand, int displacement = 0, const OffsetType& offset = {})
	{
		constexpr bool indexIsRegister = std::is_integral<OffsetType>::value;

		auto t = TypeConverters::getMirTypeFromT<T>();

		if constexpr (indexIsRegister)
			return derefInternal(pointerOperand, t, displacement + sizeof(T) * offset, "");
		else
			return derefInternal(pointerOperand, t, displacement, offset, sizeof(T));
	}

	template <typename T> String newReg(const String& source)
	{
		TextLine s1(&state, "mov");
		s1.addSelfOperand<T>(); s1.addRawOperand(source);
		return s1.flush();
	}

	void emit(const String& instruction, const StringArray& operands);

	String alloca(size_t numBytes);
	void bind(const String& label, const String& comment = {});
	String newLabel();
	void jmp(const String& op);

	template <typename ReturnType> String call(const NamespacedIdentifier& objectType, const String& functionId, const StringArray& args)
	{
		TextLine s1(&state, "call");

		auto fd = TypeConverters::String2FunctionData(functionId);

		auto prototype = state.functionManager.getPrototype(objectType, functionId);

		s1.addRawOperand(prototype);

		s1.addRawOperand(TypeConverters::FunctionData2MirTextLabel(objectType, fd));

		s1.addSelfOperand<ReturnType>();

		for (const auto& a : args)
			s1.operands.add(a);

		s1.flush();

		return s1.operands[2];
	}

	INSTRUCTION2(fmov);
	INSTRUCTION2(dmov);
	INSTRUCTION2(mov);
	INSTRUCTION3(mul);
	INSTRUCTION3(add);
	INSTRUCTION3(bne);
	INSTRUCTION3(bge);

	void setInlineComment(const String& s);

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

protected:

	String derefInternal(const String& pointerOperand, MIR_type_t type, int displacement = 0, const String& offset = {}, int elementSize = 0) const;

	mir::State& state;
	RegisterManager& rm;

};

#undef INSTRUCTION2
#undef INSTRUCTION3

}
}
