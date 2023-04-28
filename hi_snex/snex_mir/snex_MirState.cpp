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

String TextLine::addAnonymousReg(MIR_type_t type, RegisterType rt)
{
	auto id = state->registerManager.getAnonymousId(MIR_fp_type_p(type) && rt == RegisterType::Value);
	state->registerCurrentTextOperand(id, type, rt);

	auto t = TypeInfo(TypeConverters::MirType2TypeId(type), true);

	auto tn = TypeConverters::TypeInfo2MirTextType(t);

	if (rt == RegisterType::Pointer || type == MIR_T_P)
		tn = "i64";

	localDef << tn << ":" << id;
	return id;
}

TextLine::TextLine(State* s_):
	state(s_)
{

}

TextLine::~TextLine()
{
	jassert(flushed);
}

void TextLine::addImmOperand(const VariableStorage& value)
{
	operands.add(Types::Helpers::getCppValueString(value));
}

void TextLine::addSelfAsValueOperand()
{
	operands.add(state->getOperandForChild(-1, RegisterType::Value));
}

void TextLine::addSelfAsPointerOperand()
{
	operands.add(state->getOperandForChild(-1, RegisterType::Pointer));
}

void TextLine::addChildAsPointerOperand(int childIndex)
{
	operands.add(state->loadIntoRegister(childIndex, RegisterType::Pointer));
}

void TextLine::addChildAsValueOperand(int childIndex)
{
	operands.add(state->loadIntoRegister(childIndex, RegisterType::Value));
}

void TextLine::addOperands(const Array<int>& indexes, const Array<RegisterType>& registerTypes /*= {}*/)
{
	if (!registerTypes.isEmpty())
	{
		jassert(indexes.size() == registerTypes.size());

		for (int i = 0; i < indexes.size(); i++)
			operands.add(state->getOperandForChild(indexes[i], registerTypes[i]));
	}
	else
	{
		for (auto& i : indexes)
			operands.add(state->getOperandForChild(i, RegisterType::Value));
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

void TextLine::flush()
{
	if (!flushed)
	{
		flushed = true;
		state->lines.add(*this);
	}
}

void FunctionManager::addPrototype(State* state, const FunctionData& f, bool addObjectPointer)
{
	TextLine l(state);

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

	l.flush();

	prototypes.add(f);
}

bool FunctionManager::hasPrototype(const FunctionData& sig) const
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

String FunctionManager::getPrototype(const FunctionData& sig) const
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

void DataManager::setDataLayout(const String& b64)
{
	dataList = SyntaxTreeExtractor::getDataLayoutTrees(b64);

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
				auto offset = (size_t)(int)m["offset"];

				members.add({ id, mir_type, offset });
			}
		}

		classTypes.emplace(Identifier(l["ID"].toString()), std::move(members));
	}
}

void DataManager::registerClass(const Identifier& id, Array<MemberInfo>&& memberInfo)
{
	classTypes.emplace(id, memberInfo);
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

}
}