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
namespace jit {
using namespace juce;
using namespace asmjit;


class ClassScope : public FunctionClass,
	public BaseScope
{
public:

	juce::String getDebugDataType() const override
	{
		return classTypeId.toString();
	}

	juce::String getDebugName() const override
	{
		return scopeId.toString();
	}

	Identifier getObjectName() const override
	{
		return scopeId.id;
	}

	juce::String getCategory() const override
	{
		return "Object instance";
	}

	int getTypeNumber() const override
	{
		return Types::ID::Pointer;
	}

	DebugInformationBase* createDebugInformationForChild(const Identifier& id)
	{
		jassertfalse;
		
		return nullptr;
	}

	void getAllConstants(Array<Identifier>& ids) const override
	{
		auto list = BaseScope::getAllVariables();

		for (auto l : list)
			ids.add(l.id);
	}

	ClassScope(BaseScope* parent, const Symbol& id, ComplexType::Ptr typePtr_) :
		FunctionClass(id),
		BaseScope(id, parent),
		typePtr(typePtr_)
	{
		if (auto gs = dynamic_cast<GlobalScope*>(parent))
		{
			gs->setCurrentClassScope(this);
			gs->getBufferHandler().reset();

			runtime = new asmjit::JitRuntime();
			rootData = new RootClassData();

			addFunctionClass(new MessageFunctions());
			addFunctionClass(new MathFunctions());
			addFunctionClass(new BlockFunctions());
		}

		if (id)
			scopeType = BaseScope::Class;

		jassert(scopeType == BaseScope::Class);
	};

	bool isRootClass()
	{
		return getParent()->getScopeType() == BaseScope::Global;
	}

	ClassScope* getRootClass()
	{
		if (isRootClass())
			return this;

		BaseScope* c;

		while (c->getParent()->getScopeType() != BaseScope::Global)
		{
			c = c->getParent();
		}

		auto rs = dynamic_cast<ClassScope*>(c);
		jassert(rs != nullptr);

		return rs;
	}

	ScopedPointer<asmjit::JitRuntime> runtime;
	ScopedPointer<RootClassData> rootData;
	ComplexType::Ptr typePtr;

	struct FunctionDebugInfo : public DebugInformationBase
	{
		FunctionDebugInfo(jit::FunctionData* d) :
			type(d->returnType),
			full(d->getSignature()),
			name(d->id.toString())
		{

		}

		int getType() const override
		{
			return ApiHelpers::DebugObjectTypes::LocalFunction;
		}

		juce::String getCategory() const override
		{
			return "Local function";
		}

		Types::ID type;
		juce::String name;
		juce::String full;

		virtual juce::String getTextForName() const
		{
			return full;
		}

		virtual juce::String getTextForType() const { return "function"; }

		virtual juce::String getTextForDataType() const
		{
			return Types::Helpers::getTypeName(type);
		}

		virtual juce::String getTextForValue() const
		{
			return "";
		}

		virtual juce::String getCodeToInsert() const
		{
			return full.fromFirstOccurrenceOf(" ", false, false);
		}

	};

	struct LocalVariableInfo
	{
		bool isParameter;
		Symbol id;
		Types::ID type;
	};

	struct LocalVariableDebugObject : public DebugableObjectBase
	{
		LocalVariableDebugObject(LocalVariableInfo info_, GlobalScope* s_) :
			s(s_),
			info(info_)
		{}

		GlobalScope* s = nullptr;
		LocalVariableInfo info;

		Identifier getObjectName() const override
		{
			return Identifier(info.id.toString());
		}

		int getTypeNumber() const override
		{
			return info.type;
		}

		juce::String getDebugDataType() const override
		{
			return Types::Helpers::getTypeName(info.type);
		}

		juce::String getDebugValue() const override
		{
			if (s != nullptr)
			{
				auto entry = s->getBreakpointHandler().getEntry(info.id);

				if (entry.isUsed)
					return Types::Helpers::getCppValueString(entry.currentValue);
				else
					return "unknown";
			}
		}
		juce::String getCategory() const override { return info.isParameter ? "Parameter" : "Local variable"; };
	};

	struct BufferDummy : public ManualDebugObject
	{

		BufferDummy() :
			ManualDebugObject()
		{
			childNames.add("create");
			childNames.add("getAudioFile");
			childNames.add("getTableData");
			category = "API call";
			objectName = "Buffer";
		}

		int getTypeNumber() const override { return ApiHelpers::DebugObjectTypes::ApiCall; }

		void fillInfo(SettableDebugInfo* childInfo, const juce::String& id) override
		{
			if (id == "create")
			{
				childInfo->category = "API call";
				childInfo->codeToInsert = "Buffer.create(int size)";
				childInfo->name = "Buffer.create(int size)";
				childInfo->typeValue = Types::ID::Block;
				childInfo->description.append("\nCreates a empty buffer with the given size", GLOBAL_BOLD_FONT());
			}
			if (id == "getAudioFile")
			{
				childInfo->category = "API call";
				childInfo->codeToInsert = "Buffer.getAudioFile(int fileIndex)";
				childInfo->name = "Buffer.getAudioFile(int fileIndex)";
				childInfo->typeValue = Types::ID::Block;
				childInfo->description.append("\nreturns a wrapped block for the given audio file", GLOBAL_BOLD_FONT());
			}
			if (id == "getTableData")
			{
				childInfo->category = "API call";
				childInfo->codeToInsert = "Buffer.getTableData(int tableIndex)";
				childInfo->name = "Buffer.getTableData(int tableIndex)";
				childInfo->typeValue = Types::ID::Block;
				childInfo->description.append("\nreturns a wrapped block for the given look up table", GLOBAL_BOLD_FONT());
			}
		}
	};

	Identifier classTypeId;

	void addLocalVariableInfo(const LocalVariableInfo& l)
	{
		localVariableInfo.add(l);
	}

	bool hasVariable(const Identifier& id) const override
	{
		if (auto st = dynamic_cast<StructType*>(typePtr.get()))
			return st->hasMember(id);

		return BaseScope::hasVariable(id);
	}

	bool updateSymbol(Symbol& symbolToBeUpdated) override
	{
		if (auto st = dynamic_cast<StructType*>(typePtr.get()))
		{
			if (st->updateSymbol(symbolToBeUpdated))
				return true;
		}
			

		return BaseScope::updateSymbol(symbolToBeUpdated);
	}

	juce::Array<LocalVariableInfo> localVariableInfo;

	void addRegisteredFunctionClasses(OwnedArray<DebugInformationBase>& list, int typeNumber = ApiHelpers::DebugObjectTypes::ApiCall)
	{
		for (auto f : registeredClasses)
			list.add(new ObjectDebugInformation(f, typeNumber));
	}

	void createDebugInfo(OwnedArray<DebugInformationBase>& list)
	{
		list.clear();

		auto addType = [&list](const juce::String& name, const juce::String& description, Types::ID type)
		{
			auto wblockInfo = new SettableDebugInfo();
			wblockInfo->codeToInsert = name;
			wblockInfo->name = name;
			wblockInfo->description.append("\n" + description, GLOBAL_BOLD_FONT(), Colours::black);
			wblockInfo->dataType = name;
			wblockInfo->value = "";
			wblockInfo->typeValue = type;
			wblockInfo->category = "Basic Type";

			list.add(wblockInfo);
		};

		addType("int", "A 32bit integer type", Types::ID::Integer);
		addType("bool", "A boolean type (true | false)", Types::ID::Integer);
		addType("float", "A 32bit floating point type", Types::ID::Float);
		addType("sfloat", "A smoothed floating point value ", Types::ID::Float);
		addType("double", "A 64bit floating point type", Types::ID::Double);
		addType("sdouble", "A smoothed 64bit floating point value", Types::ID::Double);
		addType("block", "A wrapper around a array of float numbers", Types::ID::Block);
		addType("wblock", "A buffer object with index wrapping", Types::ID::Block);
		addType("zblock", "A buffer object with zero wrapping", Types::ID::Block);
		addType("event", "The HISE event", Types::ID::Event);

		auto forInfo = new SettableDebugInfo();
		forInfo->codeToInsert = "for (auto& s : channel)\n\t{\n\t\t\n\t}\n\t";;
		forInfo->name = "for-loop";
		forInfo->description.append("\nA range based for loop over a block", GLOBAL_BOLD_FONT(), Colours::black);
		forInfo->dataType = "float";
		forInfo->value = "";
		forInfo->category = "Template";
		forInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		auto channelInfo = new SettableDebugInfo();
		channelInfo->codeToInsert = "void processChannel(block channel, int channelIndex)\n{\n\t\n}\n";;
		channelInfo->name = "processChannel";
		channelInfo->description.append("\nBlock based channel processing", GLOBAL_BOLD_FONT(), Colours::black);
		channelInfo->value = "";
		channelInfo->category = "Template";
		channelInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(channelInfo);

		auto sampleInfo = new SettableDebugInfo();
		sampleInfo->codeToInsert = "float processSample(float input)\n{\n\treturn input;\n}\n";;
		sampleInfo->name = "processSample";
		sampleInfo->description.append("\nMono (or stateless) sample processing", GLOBAL_BOLD_FONT(), Colours::black);
		sampleInfo->value = "";
		sampleInfo->category = "Template";
		sampleInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(sampleInfo);

		auto setInfo = new SettableDebugInfo();
		setInfo->codeToInsert = "void setParameterX(double value)\n{\n\t\n}\n";;
		setInfo->name = "setParameter";
		setInfo->description.append("\nParameter callback template", GLOBAL_BOLD_FONT(), Colours::black);
		setInfo->value = "";
		setInfo->category = "Template";
		setInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(setInfo);

		auto frameInfo = new SettableDebugInfo();
		frameInfo->codeToInsert = "void processFrame(block frame)\n{\n\t\n}\n";;
		frameInfo->name = "processFrame";
		frameInfo->description.append("\nInterleaved frame processing callback", GLOBAL_BOLD_FONT(), Colours::black);
		frameInfo->value = "";
		frameInfo->category = "Template";
		frameInfo->typeValue = ApiHelpers::DebugObjectTypes::Template;

		list.add(frameInfo);

		list.add(forInfo);

		for (auto l : localVariableInfo)
		{
			if (hasVariable(l.id.id))
				continue;

			auto lInfo = new LocalVariableDebugObject(l, dynamic_cast<GlobalScope*>(getParent()));
			list.add(new ObjectDebugInformation(lInfo));
		}

		for (auto f : functions)
		{
			list.add(new FunctionDebugInfo(f));
		}

		if (auto gs = dynamic_cast<GlobalScope*>(getParent()))
		{
			list.add(new ObjectDebugInformation(gs->getGlobalFunctionClass("Console"), ApiHelpers::DebugObjectTypes::ApiCall));
		}


		addRegisteredFunctionClasses(list);


		list.add(ManualDebugObject::create<BufferDummy>());
	}

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassScope);
};

}
}