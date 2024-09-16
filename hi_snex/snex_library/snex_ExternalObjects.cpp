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
USE_ASMJIT_NAMESPACE;


void SnexObjectDatabase::registerObjects(Compiler& c, int numChannels)
{
	NamespaceHandler::InternalSymbolSetter iss(c.getNamespaceHandler());

	InbuiltTypeLibraryBuilder iBuilder(c, numChannels);
	iBuilder.registerTypes();

	//ContainerLibraryBuilder cBuilder(c, numChannels);
	//cBuilder.registerTypes();

	//ParameterLibraryBuilder pBuilder(c, numChannels);
	//pBuilder.registerTypes();

	//WrapLibraryBuilder wBuilder(c, numChannels);
	//wBuilder.registerTypes();

	DataLibraryBuilder dlBuilder(c, numChannels);
	dlBuilder.registerTypes();

#if 0
	FxNodeLibrary fBuilder(c, numChannels);
	fBuilder.registerTypes();

	CoreNodeLibrary cnBuilder(c, numChannels);
	cnBuilder.registerTypes();

	MathNodeLibrary mBuilder(c, numChannels);
	mBuilder.registerTypes();
#endif

	IndexLibrary iBuilder2(c, numChannels);
	iBuilder2.registerTypes();


#if SNEX_MIR_BACKEND
	mir::MirCompiler::setLibraryFunctions(c.getFunctionMap());
#endif
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

snex::jit::ComplexType::Ptr SampleDataJIT::createComplexType(Compiler& c, const Identifier& id)
{
	bool isMono = id == Identifier("MonoSample");

	auto st = new StructType(NamespacedIdentifier(id));

	auto blockType = c.getComplexType(NamespacedIdentifier("block"));

	

	ComplexType::Ptr spanType = new SpanType(TypeInfo(blockType, false, false), isMono ? 1 : 2);
	spanType = c.getNamespaceHandler().registerComplexTypeOrReturnExisting(spanType);

	ComplexType::Ptr loopRangeType = new SpanType(TypeInfo(Types::ID::Integer), 2);
	loopRangeType = c.getNamespaceHandler().registerComplexTypeOrReturnExisting(loopRangeType);

	st->addMember("rootNote", TypeInfo(Types::ID::Double));
	st->addMember("noteNumber", Types::ID::Double);
	st->addMember("loopRange", TypeInfo(loopRangeType));
	st->addMember("velocity", Types::ID::Integer);
	st->addMember("roundRobin", Types::ID::Integer);
	st->addMember("data", TypeInfo(spanType));

	st->setDefaultValue("rootNote", InitialiserList::makeSingleList(VariableStorage(-1)));
	st->setDefaultValue("noteNumber", InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("velocity", InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("roundRobin", InitialiserList::makeSingleList(VariableStorage(1)));
	

	st->setVisibility("rootNote", NamespaceHandler::Visibility::Public);
	st->setVisibility("noteNumber", NamespaceHandler::Visibility::Public);
	st->setVisibility("velocity", NamespaceHandler::Visibility::Public);
	st->setVisibility("roundRobin", NamespaceHandler::Visibility::Public);
	st->setVisibility("data", NamespaceHandler::Visibility::Public);
	st->setVisibility("loopRange", NamespaceHandler::Visibility::Public);

	FunctionData gpf;
	gpf.id = st->id.getChildId("getPitchFactor");
	gpf.setConst(true);
	gpf.returnType = TypeInfo(Types::ID::Double);

	gpf.inliner = Inliner::createHighLevelInliner(gpf.id, [](InlineData* b)
	{
		cppgen::Base c;
		c << "return Math.pow(2.0, (double)(this->noteNumber - this->rootNote) / 12.0);";
		return SyntaxTreeInlineParser(b, {}, c).flush();
	});

	st->addJitCompiledMemberFunction(gpf);
	
	{
		FunctionData ef;
		ef.id = st->id.getChildId("isEmpty");
		ef.setConst(true);
		ef.returnType = TypeInfo(Types::ID::Integer);

		ef.inliner = Inliner::createHighLevelInliner(ef.id, [](InlineData* b)
			{
				cppgen::Base c;
				c << "return this->data[0].size() == 0;";
				return SyntaxTreeInlineParser(b, {}, c).flush();
			});

		st->addJitCompiledMemberFunction(ef);
	}

	{
		FunctionData lr;
		lr.id = st->id.getChildId("setLoopRange");
		lr.setConst(true);
		lr.returnType = Types::ID::Void;
		lr.addArgs("idx", TypeInfo(Types::ID::Dynamic, false, true));

		lr.inliner = Inliner::createHighLevelInliner(lr.id, [](InlineData* b)
		{
			cppgen::Base c;
			c << "idx.setLoopRange(this->loopRange[0], this->loopRange[1]);";
			return SyntaxTreeInlineParser(b, {"idx"}, c).flush();
		});

		st->addJitCompiledMemberFunction(lr);
	}
	
	{
		auto eType = c.getComplexType(NamespacedIdentifier("HiseEvent"));

		FunctionData fhe;
		fhe.id = st->id.getChildId("fromHiseEvent");
		fhe.addArgs("e", TypeInfo(eType, true, true));
		fhe.returnType = TypeInfo(Types::ID::Void);

		st->addJitCompiledMemberFunction(fhe);

		if (isMono)
			st->injectMemberFunctionPointer(fhe, (void*)fromHiseEventStatic<1>);
		else
			st->injectMemberFunctionPointer(fhe, (void*)fromHiseEventStatic<2>);
	}

	{
		FunctionData cf;
		cf.id = st->id.getChildId("clear");
		cf.returnType = TypeInfo(Types::ID::Void);

		st->addJitCompiledMemberFunction(cf);

		if (isMono)
			st->injectMemberFunctionPointer(cf, (void*)clear<1>);
		else
			st->injectMemberFunctionPointer(cf, (void*)clear<2>);
	}
	


	ComplexType::Ptr frameType = new SpanType(Types::ID::Float, isMono ? 1 : 2);
	frameType = c.getNamespaceHandler().registerComplexTypeOrReturnExisting(frameType);

	FunctionData subscript;
	subscript.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::SpecialSymbols::Subscript));
	//subscript.addArgs("idx", TypeInfo(Types::ID::Dynamic, false, false));
	subscript.addArgs("idx", TypeInfo(Types::ID::Dynamic, false, false));
	subscript.returnType = TypeInfo(frameType, false, false);
	subscript.setConst(true);

	subscript.inliner = Inliner::createHighLevelInliner(subscript.id, [frameType, isMono](InlineData* b)
	{
		cppgen::Base c;
		String def;

		def << frameType->toString() << "d = { 0.0f };";
		c << def;

		c << "if(this->data[0].size() != 0)";

		{
			cppgen::StatementBlock sb(c);

				c << "d[0] = this->data[0][idx];";

			if (!isMono)
				c << "d[1] = this->data[1][idx];";
		}
		

		c << "return d;";
		
		return SyntaxTreeInlineParser(b, {"idx"}, c).flush();
	});

	st->addJitCompiledMemberFunction(subscript);


	st->finaliseExternalDefinition();

	auto originalSize = isMono ? sizeof(MonoSample) : sizeof(StereoSample);
	auto objSize = st->getRequiredByteSize();

	jassertEqual(objSize, originalSize);

	return st;
}

snex::jit::ComplexType::Ptr ExternalDataJIT::createComplexType(Compiler& c, const Identifier& id)
{
	ExternalData obj;

	auto st = new StructType(NamespacedIdentifier(id));

	st->addMember("dataType",		TypeInfo(Types::ID::Integer));
	st->addMember("numSamples",		TypeInfo(Types::ID::Integer));
	st->addMember("numChannels",	TypeInfo(Types::ID::Integer));
	st->addMember("isXYZAudioData", TypeInfo(Types::ID::Integer));
	st->addMember("data",			TypeInfo(Types::ID::Pointer, true));
	st->addMember("obj",			TypeInfo(Types::ID::Pointer, true));
	st->addMember("sampleRate",     TypeInfo(Types::ID::Double));
	
	
	st->setDefaultValue("dataType",		InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("numSamples",	InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("numChannels",	InitialiserList::makeSingleList(VariableStorage(0)));
	st->setDefaultValue("data",			InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
	st->setDefaultValue("obj",			InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
	st->setDefaultValue("sampleRate",	InitialiserList::makeSingleList(VariableStorage(0.0)));
	st->setDefaultValue("isXYZAudioData", InitialiserList::makeSingleList(VariableStorage(0)));
	
	st->setVisibility("dataType",	 NamespaceHandler::Visibility::Public);
	st->setVisibility("numSamples",	 NamespaceHandler::Visibility::Public);
	st->setVisibility("numChannels", NamespaceHandler::Visibility::Public);
	st->setVisibility("data",		 NamespaceHandler::Visibility::Public);
	st->setVisibility("obj",		 NamespaceHandler::Visibility::Public);
	st->setVisibility("sampleRate",	 NamespaceHandler::Visibility::Public);
	st->setVisibility("isXYZAudioData", NamespaceHandler::Visibility::Private);
	
	auto blockType = c.getNamespaceHandler().getComplexType(NamespacedIdentifier("block"));

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
	
	FunctionData xyz;
	xyz.id = st->id.getChildId("isXYZ");
	xyz.setConst(true);
	xyz.returnType = Types::ID::Integer;
	xyz.inliner = Inliner::createHighLevelInliner(xyz.id, [](InlineData* b)
	{
		cppgen::Base c;
		c << "return this->isXYZAudioData != 0;";
		return SyntaxTreeInlineParser(b, {}, c).flush();
	});

	st->addJitCompiledMemberFunction(xyz);







	auto eType = c.getComplexType(NamespacedIdentifier("HiseEvent"));
	
	
	{
		auto monoType = c.getComplexType(NamespacedIdentifier("MonoSample"));
		FunctionData getMono;
		getMono.id = st->id.getChildId("getMonoSample");
		getMono.returnType = Types::ID::Integer;
		getMono.addArgs("d", TypeInfo(monoType, false, true));
		getMono.addArgs("e", TypeInfo(eType, true, true));
		st->addJitCompiledMemberFunction(getMono);
		st->injectMemberFunctionPointer(getMono, (void*)ExternalData::getXYZData<1>);
	}

	{
		auto stereoType = c.getComplexType(NamespacedIdentifier("StereoSample"));
		jassert(stereoType != nullptr);
		FunctionData getStereo;
		getStereo.id = st->id.getChildId("getStereoSample");
		getStereo.returnType = Types::ID::Integer;
		getStereo.addArgs("d", TypeInfo(stereoType, false, true));
		getStereo.addArgs("e", TypeInfo(eType, true, true));
		st->addJitCompiledMemberFunction(getStereo);
		st->injectMemberFunctionPointer(getStereo, (void*)ExternalData::getXYZData<2>);
	}

	st->finaliseExternalDefinition();



	auto originalSize = sizeof(ExternalData);
	auto rSize = st->getRequiredByteSize();

	jassertEqual(rSize, originalSize);
	
	return st;
}


ScriptnodeCallbacks::ID ScriptnodeCallbacks::getCallbackId(const NamespacedIdentifier& p)
{
	// This must be handled extra because the enum is behind the numFunctions value
	// (as this is not an obligatory callback that must be defined).
	if (p.getIdentifier() == Identifier("handleModulation"))
		return ScriptnodeCallbacks::ID::HandleModulation;

	if(p.getIdentifier() == Identifier("getPlotValue"))
		return ScriptnodeCallbacks::ID::GetPlotValue;

	auto l = getIds(p.getParent());

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i] == p)
			return (ScriptnodeCallbacks::ID)i;
	}

	return ScriptnodeCallbacks::ID::OptionalOffset;
}

juce::Array<snex::NamespacedIdentifier> ScriptnodeCallbacks::getIds(const NamespacedIdentifier& p)
{
	Array<NamespacedIdentifier> ids;

	ids.add(p.getChildId("prepare"));
	ids.add(p.getChildId("reset"));
	ids.add(p.getChildId("handleHiseEvent"));
	ids.add(p.getChildId("process"));
	ids.add(p.getChildId("processFrame"));

	return ids;
}

juce::Array<snex::jit::FunctionData> ScriptnodeCallbacks::getAllPrototypes(Compiler* c, int numChannels)
{
	Array<FunctionData> f;

	for (int i = 0; i < OptionalOffset; i++)
	{
		f.add(getPrototype(c, (ID)i, numChannels));
	}

	return f;
}

snex::jit::FunctionData ScriptnodeCallbacks::getPrototype(Compiler* c, ID id, int numChannels)
{
	FunctionData f;

	ComplexType::Ptr p;
	NamespacedIdentifier nid;

	switch (id)
	{
	case PrepareFunction: 
	{
		f.id = NamespacedIdentifier("prepare");
		f.returnType = TypeInfo(Types::ID::Void);

		nid = NamespacedIdentifier("PrepareSpecs");

		if (c != nullptr)
			p = c->getComplexType(nid);
		else
			p = new StructType(nid, {});

		f.addArgs("specs", TypeInfo(p, false, false));
		break;
	}
		
	case ProcessFunction:
	{
		f.id = NamespacedIdentifier("process");
		f.returnType = TypeInfo(Types::ID::Void);

		NamespacedIdentifier pId("ProcessData");

		TemplateParameter ct(numChannels);

		ct.argumentId = pId.getChildId("NumChannels");

		if(c != nullptr)
			f.addArgs("data", TypeInfo(c->getComplexType(pId, ct), false, true));

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

		if(c != nullptr)
			f.addArgs("frame", TypeInfo(c->registerExternalComplexType(t), false, true));

		break;
	}
	case HandleEventFunction:
	{
		f.id = NamespacedIdentifier("handleHiseEvent");
		f.returnType = TypeInfo(Types::ID::Void);

		nid = NamespacedIdentifier("HiseEvent");

		if (c != nullptr)
			p = c->getComplexType(nid, {});
		else
			p = new StructType(nid);

		f.addArgs("e", TypeInfo(p, false, true));
		break;
	}
	case HandleModulation:
	{
		f.id = NamespacedIdentifier("handleModulation");
		f.returnType = TypeInfo(Types::ID::Integer);
		f.addArgs("value", TypeInfo(Types::ID::Double, false, true));
		break;
	}
	case GetPlotValue:
	{
		f.id = NamespacedIdentifier("getPlotValue");
		f.returnType = TypeInfo(Types::ID::Double);
		f.addArgs("getMagnitude", TypeInfo(Types::ID::Integer));
		f.addArgs("freqNormalised", TypeInfo(Types::ID::Double));
		break;
	}
	case SetExternalDataFunction:
	{
		f.id = NamespacedIdentifier("setExternalData");
		f.returnType = Types::ID::Void;

		nid = NamespacedIdentifier("ExternalData");

		if (c != nullptr)
			p = c->getComplexType(nid, {});
		else
			p = new StructType(nid);

		f.addArgs("data", TypeInfo(p, true, true));
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
