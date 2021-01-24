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
namespace Types {
using namespace juce;
using namespace asmjit;




static void funkyWasGeht(void* obj, void* data)
{
	jassertfalse;
}

void SnexObjectDatabase::registerObjects(Compiler& c, int numChannels)
{
	NamespaceHandler::InternalSymbolSetter iss(c.getNamespaceHandler());

	InbuiltTypeLibraryBuilder iBuilder(c, numChannels);
	iBuilder.registerTypes();

	ContainerLibraryBuilder cBuilder(c, numChannels);
	cBuilder.registerTypes();

	ParameterLibraryBuilder pBuilder(c, numChannels);
	pBuilder.registerTypes();

	WrapLibraryBuilder wBuilder(c, numChannels);
	wBuilder.registerTypes();

	FxNodeLibrary fBuilder(c, numChannels);
	fBuilder.registerTypes();

	CoreNodeLibrary cnBuilder(c, numChannels);
	cnBuilder.registerTypes();

	MathNodeLibrary mBuilder(c, numChannels);
	mBuilder.registerTypes();
}


snex::jit::ComplexType::Ptr PrepareSpecsJIT::createComplexType(Compiler& c, const Identifier& id)
{
	PrepareSpecs obj;

	auto st = new StructType(NamespacedIdentifier(id));
	ADD_SNEX_STRUCT_MEMBER(st, obj, sampleRate);
	ADD_SNEX_STRUCT_MEMBER(st, obj, blockSize);
	ADD_SNEX_STRUCT_MEMBER(st, obj, numChannels);

	auto ptr = (void*)obj.voiceIndex;

	st->addExternalMember("voiceIndex", obj, ptr, NamespaceHandler::Visibility::Private);

	st->finaliseExternalDefinition();

	return st;
}


snex::jit::ComplexType::Ptr ExternalDataJIT::createComplexType(Compiler& c, const Identifier& id)
{
	ExternalData obj;

	auto st = new StructType(NamespacedIdentifier(id));

	st->addMember("dataType",		TypeInfo(Types::ID::Integer));
	st->addMember("numSamples",		TypeInfo(Types::ID::Integer));
	st->addMember("numChannels",	TypeInfo(Types::ID::Integer));
	st->addMember("data",			TypeInfo(Types::ID::Pointer, true));
	st->addMember("obj",			TypeInfo(Types::ID::Pointer, true));
	st->addMember("f",				TypeInfo(Types::ID::Pointer, true));
	
	st->setDefaultValue("dataType",		InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("numSamples",	InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("numChannels",	InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("data",			InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
	st->setDefaultValue("obj",			InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
	st->setDefaultValue("f",			InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
	
	st->setVisibility("dataType",	 NamespaceHandler::Visibility::Public);
	st->setVisibility("numSamples",	 NamespaceHandler::Visibility::Public);
	st->setVisibility("numChannels", NamespaceHandler::Visibility::Public);
	st->setVisibility("data",		 NamespaceHandler::Visibility::Private);
	st->setVisibility("obj",		 NamespaceHandler::Visibility::Private);
	st->setVisibility("f",			 NamespaceHandler::Visibility::Private);
	
	auto blockType = c.getNamespaceHandler().getComplexType(NamespacedIdentifier("block"));

	FunctionData constructor;
	constructor.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor));
	constructor.addArgs("obj", TypeInfo(Types::ID::Dynamic));
	constructor.returnType = TypeInfo(Types::ID::Void);

	constructor.inliner = Inliner::createAsmInliner(constructor.id, [](InlineData* b)
	{
		auto d = b->toAsmInlineData();

		if (auto argType = d->args[0]->getTypeInfo().getTypedIfComplexType<StructType>())
		{
			if (auto dataType = argType->getMemberComplexType("data"))
			{
				if (auto st = dynamic_cast<SpanType*>(dataType.get()))
				{
					auto& cc = d->gen.cc;

					auto thisType = d->object->getTypeInfo().getComplexType();
					auto thisSize = thisType->getRequiredByteSize();
					auto thisAlign = thisType->getRequiredAlignment();

					auto thisMem = cc.newStack(thisSize, thisAlign);

					int sizeToUse = st->getNumElements();
					
					auto spanTarget = d->args[0];
					auto spanOffset = argType->getMemberOffset("data");

					cc.mov(thisMem.cloneResized(4), (int)ExternalData::DataType::ConstantLookUp);
					cc.mov(thisMem.cloneAdjustedAndResized(4, 4), sizeToUse);
					cc.mov(thisMem.cloneAdjustedAndResized(8, 4), 1);

					
					auto spanLocReg = cc.newGpq();

					if (spanTarget->isMemoryLocation())
					{
						auto mem = spanTarget->getMemoryLocationForReference().cloneAdjustedAndResized(spanOffset, 8);
						cc.lea(spanLocReg, mem);
					}
					else
					{
						auto mem = x86::ptr(PTR_REG_R(spanTarget)).cloneAdjustedAndResized(spanOffset, 8);
						cc.lea(spanLocReg, mem);
					}

					cc.mov(thisMem.cloneAdjustedAndResized(16, 8), spanLocReg);
					cc.mov(thisMem.cloneAdjustedAndResized(24, 8), 0);
					cc.mov(thisMem.cloneAdjustedAndResized(32, 8), 0);

					d->object->setCustomMemoryLocation(thisMem, false);

					return Result::ok();
				}
				else
					return Result::fail(argType->id.getChildId("data").toString() + " is not a span<float, x> type");

				
			}
			else
				return Result::fail(argType->id.getChildId("data").toString() + " is not defined");
		}
		else
		{
			return Result::fail("ExternalData constructor requires object argument");
		}

		
	});

	st->addJitCompiledMemberFunction(constructor);

	FunctionData rf;
	rf.id = st->id.getChildId("referBlockTo");
	rf.addArgs("b", TypeInfo(blockType, false, true));
	rf.addArgs("index", TypeInfo(Types::ID::Integer));
	rf.returnType = TypeInfo(Types::ID::Void);
	
	st->addJitCompiledMemberFunction(rf);
	st->injectMemberFunctionPointer(rf, (void*)referTo);

	FunctionData df;
	df.id = st->id.getChildId("setDisplayedValue");
	df.addArgs("value", TypeInfo(Types::ID::Double));
	df.returnType = TypeInfo(Types::ID::Void);

	st->addJitCompiledMemberFunction(df);
	st->injectMemberFunctionPointer(df, (void*)setDisplayValueStatic);

	st->finaliseExternalDefinition();

	return st;
}


ScriptnodeCallbacks::ID ScriptnodeCallbacks::getCallbackId(const NamespacedIdentifier& p)
{
	auto l = getIds(p.getParent());

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i] == p)
			return (ScriptnodeCallbacks::ID)i;
	}

	return ScriptnodeCallbacks::ID::numFunctions;
}

juce::Array<snex::NamespacedIdentifier> ScriptnodeCallbacks::getIds(const NamespacedIdentifier& p)
{
	Array<NamespacedIdentifier> ids;

	ids.add(p.getChildId("prepare"));
	ids.add(p.getChildId("reset"));
	ids.add(p.getChildId("handleEvent"));
	ids.add(p.getChildId("process"));
	ids.add(p.getChildId("processFrame"));

	return ids;
}

juce::Array<snex::jit::FunctionData> ScriptnodeCallbacks::getAllPrototypes(Compiler& c, int numChannels)
{
	Array<FunctionData> f;

	for (int i = 0; i < numFunctions; i++)
	{
		f.add(getPrototype(c, (ID)i, numChannels));
	}

	return f;
}

snex::jit::FunctionData ScriptnodeCallbacks::getPrototype(Compiler& c, ID id, int numChannels)
{
	FunctionData f;

	switch (id)
	{
	case PrepareFunction: 
		f.id = NamespacedIdentifier("prepare");
		f.returnType = TypeInfo(Types::ID::Void);
		f.addArgs("specs", TypeInfo(c.getComplexType(NamespacedIdentifier("PrepareSpecs"), {}), false, false));
		break;
	case ProcessFunction:
	{
		f.id = NamespacedIdentifier("process");
		f.returnType = TypeInfo(Types::ID::Void);

		NamespacedIdentifier pId("ProcessData");

		TemplateParameter ct(numChannels);

		ct.argumentId = pId.getChildId("NumChannels");
		f.addArgs("data", TypeInfo(c.getComplexType(pId, ct), false, true));
		break;
	}
	case ResetFunction:
		f.id = NamespacedIdentifier("reset");
		f.returnType = TypeInfo(Types::ID::Void);
		break;
	case ProcessFrameFunction:
	{
		f.id = NamespacedIdentifier("processFrame");
		f.returnType = TypeInfo(Types::ID::Void);

		

		ComplexType::Ptr t = new SpanType(TypeInfo(Types::ID::Float), numChannels);

		

		f.addArgs("frame", TypeInfo(c.registerExternalComplexType(t), false, true));
		break;
	}
	case HandleEventFunction:
	{
		f.id = NamespacedIdentifier("handleEvent");
		f.returnType = TypeInfo(Types::ID::Void);
		f.addArgs("e", TypeInfo(c.getComplexType(NamespacedIdentifier("HiseEvent"), {}), false, true));
		break;
	}
	case HandleModulation:
	{
		f.id = NamespacedIdentifier("handleModulation");
		f.returnType = TypeInfo(Types::ID::Integer);
		f.addArgs("value", TypeInfo(Types::ID::Double, false, true));
		break;
	}
	case SetExternalDataFunction:
	{
		f.id = NamespacedIdentifier("setExternalData");
		f.returnType = Types::ID::Void;
		f.addArgs("data", TypeInfo(c.getComplexType(NamespacedIdentifier("ExternalData")), true, true));
		f.addArgs("index", Types::ID::Integer);
		break;
	}
	}

	return f;
}

#if 0
snex::jit::Inliner::Ptr SnexNodeBase::createInliner(const NamespacedIdentifier& id, const Array<void*>& functions)
{
	return Inliner::createAsmInliner(id, [id, functions](InlineData* b)
	{
		auto d = b->toAsmInlineData();

		FunctionData f;
		f.returnType = TypeInfo(Types::ID::Void);
		f.id = id;
		auto tp = d->templateParameters[0];

		int c = 2;

		f.function = functions[c];

		d->gen.emitFunctionCall(d->target, f, d->object, d->args);

		return Result::ok();
	});
}





TypeInfo SnexNodeBase::Wrappers::createFrameType(const SnexTypeConstructData& cd)
{
	ComplexType::Ptr st = new SpanType(TypeInfo(Types::ID::Float), cd.numChannels);

	return TypeInfo(cd.c.getNamespaceHandler().registerComplexTypeOrReturnExisting(st), false, true);
}

#endif




}
}
