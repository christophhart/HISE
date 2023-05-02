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
	ValueTree v;
	String text;
	String stackPtr;
	MIR_type_t type;
	RegisterType registerType = RegisterType::Value;
};

struct MemberInfo
{
	String id;
	MIR_type_t type;
	size_t offset;
};

struct State;

struct TextLine
{
	TextLine(State* s);

	~TextLine();

	void addImmOperand(const VariableStorage& value);

	String addAnonymousReg(MIR_type_t type, RegisterType rt);
	void addSelfAsValueOperand();
	void addSelfAsPointerOperand();
	void addChildAsPointerOperand(int childIndex);
	void addChildAsValueOperand(int childIndex);
	void addOperands(const Array<int>& indexes, const Array<RegisterType>& registerTypes = {});

	void appendComment(const String& c) { comment = c; }

	int getLabelLength() const;
	String toLine(int maxLabelLength) const;

	void flush();

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

	void pushInlineFunction(String& l, MIR_type_t type = MIR_T_I8, const String& returnRegName={});

	void popInlineFunction()
	{
		inlineFunctionData.removeLast(1);
	}

	void addInlinedArgument(const String& argumentId, const String& variableName)
	{
		inlineFunctionData.getReference(inlineFunctionData.size() - 1).arguments.emplace(argumentId, variableName);
	}

	String getInlinedParameter(const String& argumentId)
	{
		return inlineFunctionData.getReference(inlineFunctionData.size() - 1).arguments[argumentId];
	}

	void emitInlinedReturn(State* s);

private:

	mutable int labelCounter = 0;

	struct InlineFunctionData
	{
		String endLabel;
		String returnReg;
		MIR_type_t type;
		std::map<String, String> arguments;
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
	void addPrototype(State* state, const FunctionData& f, bool addObjectPointer);

	bool hasPrototype(const FunctionData& sig) const;

	String getPrototype(const FunctionData& sig) const;

private:

	Array<FunctionData> prototypes;
};

struct DataManager
{
	void setDataLayout(const String& b64);

	void startClass(const Identifier& id, Array<MemberInfo>&& memberInfo);

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
                auto id = Types::Helpers::getTypeFromTypeName(k.second);
                
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

private:

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
	void emitLabel(const String& label);

	bool isParsingClass() const { return dataManager.isParsingClass(); };
	bool isParsingFunction() const { return registerManager.isParsingFunction(); }

	ValueTree getCurrentChild(int index);
	void processChildTree(int childIndex);
	void processAllChildren();
	Result processTreeElement(const ValueTree& v);

	String toString(bool addTabs);
};

}
}
