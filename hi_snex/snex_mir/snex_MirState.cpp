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


namespace snex {
namespace mir {
using namespace juce;


LoopManager::~LoopManager()
{
	jassert(labelPairs.isEmpty());
}


void LoopManager::pushLoopLabels(String& startLabel, String& endLabel, String& continueLabel)
{
	startLabel = makeLabel();
	endLabel = makeLabel();
	continueLabel = makeLabel();

	labelPairs.add({ startLabel, endLabel, continueLabel });
}

String LoopManager::getCurrentLabel(const String& instructionId)
{
	if (instructionId == "inlined-return")
		return inlineFunctionData.getLast().endLabel;
	if (instructionId == "continue")
		return labelPairs.getLast().continueLabel;
	else if (instructionId == "break")
		return labelPairs.getLast().endLabel;

	jassertfalse;
	return "";
}

void LoopManager::popLoopLabels()
{
	labelPairs.removeLast();
}

String LoopManager::makeLabel() const
{
	return "L" + String(labelCounter++);
}

void LoopManager::pushInlineFunction(String& l, MIR_type_t type, RegisterType rtype, const String& returnRegName)
{
	jassert((type != MIR_T_I8) == returnRegName.isNotEmpty());

	l = makeLabel();

	InlineFunctionData d;
	d.endLabel = l;
	d.type = type;
	d.returnReg = returnRegName;
    d.registerType = rtype;

	inlineFunctionData.add(d);
}

void LoopManager::emitInlinedReturn(State* s)
{
	auto l = inlineFunctionData.getLast();

	if (l.returnReg.isNotEmpty())
	{
        TextLine store(s);

		auto regType = l.registerType;

		if (l.type == MIR_T_P)
			regType = RegisterType::Pointer;

        auto returnValue = s->registerManager.loadIntoRegister(0, regType);

        auto at = l.type;
        
        if(l.registerType == RegisterType::Pointer)
            at = MIR_T_I64;
        
		store.instruction = TypeConverters::MirTypeAndToken2InstructionText(at, JitTokens::assign_);
		store.operands.add(l.returnReg);
		store.operands.add(returnValue);
		store.flush();
	}
	
	TextLine jmp(s);
	jmp.instruction = "jmp";
	jmp.operands.add(l.endLabel);
	jmp.flush();
}

String TextLine::addAnonymousReg(MIR_type_t type, RegisterType rt, bool registerAsCurrentOp)
{
	auto& rm = state->registerManager;

	auto id = rm.getAnonymousId(MIR_fp_type_p(type) && rt == RegisterType::Value);

	if(registerAsCurrentOp)
		rm.registerCurrentTextOperand(id, type, rt);

	auto t = TypeInfo(TypeConverters::MirType2TypeId(type), true);

	auto tn = TypeConverters::TypeInfo2MirTextType(t);

	if (rt == RegisterType::Pointer || type == MIR_T_P)
		tn = "i64";

	localDef << tn << ":" << id;
	return id;
}

TextLine::TextLine(State* s_, const String& instruction_):
	state(s_),
	instruction(instruction_)
{
	
}

TextLine::~TextLine()
{
	jassert(flushed);
}

void TextLine::addImmOperand(const VariableStorage& value)
{
	if (value.getType() == Types::ID::Pointer)
	{
		auto x = String(reinterpret_cast<int64>(value.getDataPointer()));

		operands.add(x);
	}
	else
	{
		operands.add(Types::Helpers::getCppValueString(value));
	}

	
}

void TextLine::addSelfAsValueOperand()
{
	operands.add(state->registerManager.getOperandForChild(-1, RegisterType::Value));
}

void TextLine::addSelfAsPointerOperand()
{
	operands.add(state->registerManager.getOperandForChild(-1, RegisterType::Pointer));
}

void TextLine::addChildAsPointerOperand(int childIndex)
{
	operands.add(state->registerManager.loadIntoRegister(childIndex, RegisterType::Pointer));
}

void TextLine::addChildAsValueOperand(int childIndex)
{
	operands.add(state->registerManager.loadIntoRegister(childIndex, RegisterType::Value));
}

void TextLine::addOperands(const Array<int>& indexes, const Array<RegisterType>& registerTypes /*= {}*/)
{
	if (!registerTypes.isEmpty())
	{
		jassert(indexes.size() == registerTypes.size());

		for (int i = 0; i < indexes.size(); i++)
			operands.add(state->registerManager.getOperandForChild(indexes[i], registerTypes[i]));
	}
	else
	{
		for (auto& i : indexes)
			operands.add(state->registerManager.getOperandForChild(i, RegisterType::Value));
	}
}

int TextLine::getLabelLength() const
{
	return label.isEmpty() ? -1 : (label.length() + 2);
}

String TextLine::toLine(int maxLabelLength) const
{
	String s;

	if (label.isNotEmpty())
		s << label << ":" << " ";

	auto numToAdd = maxLabelLength - s.length();

	for (int i = 0; i < numToAdd; i++)
		s << " ";

	if (!localDef.isEmpty())
    {
        s << "local " << localDef << "\n";
        
        for (int i = 0; i < maxLabelLength; i++)
            s << " ";
    }


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

String TextLine::flush()
{
	if (!flushed)
	{
		flushed = true;
		state->lines.add(*this);
	}

	return operands[0];
}

FunctionManager::ComplexTypeOverload::ComplexTypeOverload(const NamespacedIdentifier& objectType_, const String& fullSignature_) :
	fullSignature(fullSignature_),
	objectType(objectType_)
{
	auto f = TypeConverters::String2FunctionData(fullSignature);

	auto hash = fullSignature.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false);
	hash = hash.removeCharacters("<>&, ");

	auto newid = f.id.id.toString() + "_" + hash;

	f.id.id = Identifier(newid);

	mangledId = TypeConverters::FunctionData2MirTextLabel(objectType, f);
}



void FunctionManager::addPrototype(State* state, const NamespacedIdentifier& objectType, const FunctionData& f, bool addObjectPointer, int returnBlockSize)
{
	if (hasPrototype(objectType, f))
	{
		// must be caught with registerComplexTypeOverload
		jassertfalse;
	}

	TextLine l(state);

	l.label = "proto" + String(prototypes.size());
	l.instruction = "proto";

	auto op = TypeConverters::TypeAndReturnBlockToReturnType(f.returnType, returnBlockSize);

	if (op.isNotEmpty())
		l.operands.add(op);
	

	if (addObjectPointer)
	{
		l.operands.add("i64:_this_");
	}

	for (const auto& a : f.args)
		l.operands.add(TypeConverters::Symbol2MirTextSymbol(a));

	l.flush();

	prototypes.add({objectType, f});
}

bool FunctionManager::hasPrototype(const NamespacedIdentifier& objectType, const FunctionData& sig) const
{
	auto thisLabel = TypeConverters::FunctionData2MirTextLabel(objectType, sig);

	for (const auto& p : prototypes)
	{
		auto pLabel = TypeConverters::FunctionData2MirTextLabel(std::get<0>(p), std::get<1>(p));

		if (pLabel == thisLabel)
			return true;
	}

	return false;
}

String FunctionManager::getPrototype(const NamespacedIdentifier& objectType, const String& fullSignature) const
{
	for (auto& f : specialOverloads)
	{
		if (f.fullSignature == fullSignature &&
			f.objectType == objectType)
		{
			return f.prototype;
		}
	}

	auto sig = TypeConverters::String2FunctionData(fullSignature);

	auto thisLabel = TypeConverters::FunctionData2MirTextLabel(objectType, sig);

	int l = 0;

	for (const auto& p : prototypes)
	{
		auto pLabel = TypeConverters::FunctionData2MirTextLabel(std::get<0>(p), std::get<1>(p));

		if (pLabel == thisLabel)
		{
			return "proto" + String(l);
		}

		l++;
	}

	throw String("prototype not found");
}

String FunctionManager::registerComplexTypeOverload(State* state, const NamespacedIdentifier& objectType, const String& fullSignature, bool addObjectPtr)
{
	ComplexTypeOverload t(objectType, fullSignature);

	t.prototype = "proto" + String(prototypes.size());

	auto f = TypeConverters::String2FunctionData(fullSignature);

	f.id = NamespacedIdentifier(t.mangledId);

	addPrototype(state, objectType, f, addObjectPtr);

	specialOverloads.add(t);
	return t.mangledId;
}

void DataManager::setDataLayout(const Array<ValueTree>& dataList_)
{
	dataList = dataList_;

	for (const auto& l : dataList)
	{
		Array<MemberInfo> members;

		for (const auto& m : l)
		{
			if (m.getType() == Identifier("Member"))
			{
				auto id = m["ID"].toString();
				auto type = Types::Helpers::getTypeFromTypeName(m["type"].toString());
				auto mir_type = TypeConverters::TypeInfo2MirType(TypeInfo(type));
                size_t offset = (size_t)(int)m["offset"];

				members.add(MemberInfo(id, mir_type, offset));
			}
		}

		classTypes.emplace(Identifier(l["ID"].toString()), std::move(members));
	}
}

void DataManager::startClass(const NamespacedIdentifier& nid, Array<MemberInfo>&& memberInfo)
{
	auto className = Identifier(TypeConverters::NamespacedIdentifier2MangledMirVar(nid));

	currentClassTypes.add(nid.getIdentifier().toString());
    classTypes.emplace(className, memberInfo);
	numCurrentlyParsedClasses++;
}

void DataManager::endClass()
{
	currentClassTypes.removeLast();
	numCurrentlyParsedClasses--;
}

juce::ValueTree DataManager::getDataObject(const String& type) const
{
	for (const auto& s : dataList)
	{
		if (s["ID"].toString() == type)
		{
			return s;
		}
	}

	return {};
}

size_t DataManager::getNumBytesRequired(const Identifier& id)
{
	size_t numBytes = 0;

	for (const auto&m : classTypes[id])
		numBytes = m.offset + Types::Helpers::getSizeForType(TypeConverters::MirType2TypeId(m.type));

	return numBytes;
}

const juce::Array<snex::mir::MemberInfo>& DataManager::getClassType(const Identifier& id)
{
	return classTypes[id];
}

RegisterManager::RegisterManager(State* state_):
	state(state_)
{

}

String RegisterManager::getAnonymousId(bool isFloat) const
{
	String s;
	s << (isFloat ? "xmm" : "reg") << String(counter++);
	return s;
}

void RegisterManager::emitMultiLineCopy(const String& targetPointerReg, const String& sourcePointerReg, int numBytesToCopy)
{
	auto use64 = numBytesToCopy % 8 == 0;

    jassert(use64); ignoreUnused(use64);

	for (int i = 0; i < numBytesToCopy; i += 8)
	{
		TextLine l(state);
		l.instruction = "mov";

		auto dst = "i64:" + String(i) + "(" + targetPointerReg + ")";
		auto src = "i64:" + String(i) + "(" + sourcePointerReg + ")";

		l.operands.add(dst);
		l.operands.add(src);
		l.flush();
	}
}

int RegisterManager::allocateStack(const String& targetName, int numBytes, bool registerAsCurrentStatementReg)
{
    jassert(targetName.isNotEmpty());
    
	TextLine l(state);

	if (registerAsCurrentStatementReg)
		registerCurrentTextOperand(targetName, MIR_T_I64, RegisterType::Pointer);

	l.localDef << "i64:" << targetName;

	static constexpr int Alignment = 16;

	auto numBytesToAllocate = numBytes;

	if(numBytesToAllocate % Alignment != 0)
		numBytesToAllocate = numBytes + (Alignment - numBytes % Alignment);

	l.instruction = "alloca";
	l.operands.add(targetName);
	l.addImmOperand((int)numBytesToAllocate);
	l.flush();

	return numBytesToAllocate;
}

void RegisterManager::registerCurrentTextOperand(String n, MIR_type_t type, RegisterType rt)
{
	if (isParsingFunction())
		localOperands.add({ state->currentTree, n, {}, type, rt });
	else
		globalOperands.add({ state->currentTree, n, {}, type, rt });
}

String RegisterManager::loadIntoRegister(int childIndex, RegisterType targetType)
{
	if (getRegisterTypeForChild(childIndex) == RegisterType::Pointer ||
		getTypeForChild(childIndex) == MIR_T_P)
	{
		auto t = getTypeForChild(childIndex);

		TextLine load(state);
		load.instruction = "mov";

		auto id = "p" + String(counter++);

		load.localDef << "i64:" << id;
		load.operands.add(id);
		load.addOperands({ childIndex }, { RegisterType::Pointer });
		load.flush();

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

snex::mir::TextOperand RegisterManager::getTextOperandForValueTree(const ValueTree& c)
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

	state->dump();

	throw String(2);
}

snex::mir::RegisterType RegisterManager::getRegisterTypeForChild(int index)
{
	auto t = getTextOperandForValueTree(state->getCurrentChild(index));
	return t.registerType;
}

MIR_type_t RegisterManager::getTypeForChild(int index)
{
	auto t = getTextOperandForValueTree(state->getCurrentChild(index));
	return t.type;
}

String RegisterManager::getOperandForChild(int index, RegisterType requiredType)
{
	auto t = getTextOperandForValueTree(state->getCurrentChild(index));

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



String State::operator[](const Identifier& id) const
{
	if (!currentTree.hasProperty(id))
	{
		dump();
		throw String("No property " + id.toString());
	}

	return currentTree[id].toString();
}

void State::dump() const
{
	DBG(currentTree.createXml()->createDocument(""));
}

void State::emitSingleInstruction(const String& instruction, const String& label /*= {}*/)
{
	TextLine l(this);
	l.instruction = instruction;
	l.label = label;
	l.flush();
}

void State::emitLabel(const String& label, const String& optionalComment)
{
	TextLine noop(this);
	noop.label = label;
	noop.instruction = "bt";
	noop.operands.add(label);
	noop.addImmOperand(0);

	if (optionalComment.isEmpty())
		noop.appendComment("noop");
	else
		noop.appendComment(optionalComment);

	noop.flush();
}

String State::toString(bool addTabs)
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

juce::ValueTree State::getCurrentChild(int index)
{
	if (index == -1)
		return currentTree;
	else
		return currentTree.getChild(index);
}

void State::processChildTree(int childIndex)
{
	ScopedValueSetter<ValueTree> svs(currentTree, currentTree.getChild(childIndex));
	auto ok = processTreeElement(currentTree);

	if (ok.failed())
		throw ok.getErrorMessage();
}

void State::processAllChildren()
{
	for (int i = 0; i < currentTree.getNumChildren(); i++)
		processChildTree(i);
}

juce::Result State::processTreeElement(const ValueTree& v)
{
	try
	{
		currentTree = v;
		return instructionManager.perform(this, v);
	}
	catch (String& e)
	{
		return Result::fail(e);
	}
}

juce::Result InstructionManager::perform(State* state, const ValueTree& v)
{
	if (auto f = instructions[v.getType()])
	{
		return f(state);
	}

	state->dump();

	throw String("no instruction found for type " + v.getType());
}

snex::mir::TextOperand InlinerManager::emitInliner(const String& inlinerLabel, const String& className, const String& methodName, const StringArray& operands)
{
	auto dTree = state->dataManager.getDataObject(className);
	ValueTree fTree;

	for (const auto& m : dTree)
	{
		if (m.getType() == Identifier("Method") && m["ID"] == methodName)
		{
			fTree = m.createCopy();
			break;
		}
	}
	  
	jassert(fTree.isValid());
	jassert(fTree.getNumChildren() == operands.size());

	for(int i = 0; i < operands.size(); i++)
		fTree.getChild(i).setProperty("Operand", operands[i], nullptr);

	auto contains = inlinerFunctions.count(inlinerLabel) > 0;

	if (!contains)
	{
		throw String("Can't find inliner with label " + inlinerLabel);
	}

	return inlinerFunctions[inlinerLabel](state, dTree, fTree);
}

String MirCodeGenerator::derefInternal(const String& pointerOperand, MIR_type_t type, int displacement /*= 0*/, const String& offset /*= {}*/, int elementSize /*= 0*/) const
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

String MirCodeGenerator::newLabel()
{
	return state.loopManager.makeLabel();
}

void MirCodeGenerator::jmp(const String& op)
{
	state.emitSingleInstruction("jmp " + op);
}

String MirCodeGenerator::alloca(size_t numBytes)
{
	auto blockReg = rm.getAnonymousId(false);
	rm.allocateStack(blockReg, (int)numBytes, false);
	return blockReg;
}

void MirCodeGenerator::emit(const String& instruction, const StringArray& operands)
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

void MirCodeGenerator::bind(const String& label, const String& comment /*= {}*/)
{
	state.emitLabel(label, comment);
}

void MirCodeGenerator::setInlineComment(const String& s)
{
	nextComment = s;
}

}
}
