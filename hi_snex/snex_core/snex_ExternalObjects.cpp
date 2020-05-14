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


template <typename T>
jit::ComplexType::Ptr _ramp<T>::createComplexType(Compiler& c, const Identifier& id)
{
	Type s;

	auto obj = new StructType(NamespacedIdentifier(id));

	ADD_SNEX_STRUCT_MEMBER(obj, s, value);
	ADD_SNEX_STRUCT_MEMBER(obj, s, targetValue);
	ADD_SNEX_STRUCT_MEMBER(obj, s, delta);
	ADD_SNEX_STRUCT_MEMBER(obj, s, stepDivider);
	ADD_SNEX_STRUCT_MEMBER(obj, s, numSteps);
	ADD_SNEX_STRUCT_MEMBER(obj, s, stepsToDo);

	ADD_SNEX_STRUCT_METHOD(obj, Type, reset);
	ADD_SNEX_STRUCT_METHOD(obj, Type, set);
	ADD_SNEX_STRUCT_METHOD(obj, Type, advance);
	ADD_SNEX_STRUCT_METHOD(obj, Type, get);
	ADD_SNEX_STRUCT_METHOD(obj, Type, prepare);

	FunctionClass::Ptr fc = obj->getFunctionClass();

	ADD_INLINER(set, {
		SETUP_INLINER(T);

		auto skipSmooth = cc.newLabel();
		auto end = cc.newLabel();
		auto numSteps = cc.newGpd();
		d->args[0]->loadMemoryIntoRegister(cc);
		auto newTargetValue = FP_REG_R(d->args[0]);
		x86::Xmm dl;

		IF_(float)  dl = cc.newXmmSs();
		IF_(double) dl = cc.newXmmSd();

		cc.setInlineComment("Inlined sfloat::set");
		cc.mov(numSteps, MEMBER_PTR(numSteps));
		cc.test(numSteps, numSteps);
		cc.je(skipSmooth);

		// if(numSteps != 0)
		{
			IF_(float)
			{
				cc.movss(dl, newTargetValue);
				cc.subss(dl, MEMBER_PTR(value));
				cc.mulss(dl, MEMBER_PTR(stepDivider));
				cc.movss(MEMBER_PTR(delta), dl);
				cc.movss(MEMBER_PTR(targetValue), newTargetValue);
			}
			IF_(double)
			{
				cc.movsd(dl, newTargetValue);
				cc.subsd(dl, MEMBER_PTR(value));
				cc.mulsd(dl, MEMBER_PTR(stepDivider));
				cc.movsd(MEMBER_PTR(delta), dl);
				cc.movsd(MEMBER_PTR(targetValue), newTargetValue);
			}

			cc.mov(MEMBER_PTR(stepsToDo), numSteps);
			cc.jmp(end);
		}
		// else
		{
			cc.bind(skipSmooth);

			IF_(float) cc.movss(MEMBER_PTR(value), newTargetValue);
			IF_(double) cc.movsd(MEMBER_PTR(value), newTargetValue);

			cc.mov(MEMBER_PTR(stepsToDo), 0);
		}

		cc.bind(end);

		return Result::ok();
		});

	ADD_INLINER(get,
		{
			SETUP_INLINER(T);
			d->target->createRegister(cc);
			auto ret = FP_REG_W(d->target);

			cc.setInlineComment("inline get()");
			IF_(float) cc.movss(ret, MEMBER_PTR(value));
			IF_(double) cc.movsd(ret, MEMBER_PTR(value));

			return Result::ok();
		});

	ADD_INLINER(advance,
		{
			/*
			T advance()
			{
				auto v = value;

				if (stepsToDo <= 0)
					return v;

				value += delta;
				stepsToDo--;

				return v;
			}
			*/

			SETUP_INLINER(T);

			d->target->createRegister(cc);
			auto ret = FP_REG_W(d->target);
			auto end = cc.newLabel();
			auto stepsToDo = cc.newGpd();

			cc.setInlineComment("inline advance()");
			IF_(float) cc.movss(ret, MEMBER_PTR(value));
			IF_(double) cc.movsd(ret, MEMBER_PTR(value));

			cc.mov(stepsToDo, MEMBER_PTR(stepsToDo));
			cc.cmp(stepsToDo, 0);
			cc.jle(end);

			// if (stepsToDo > 0
			{
				IF_(float)
				{
					auto tmp = cc.newXmmSs();
					cc.movss(tmp, ret);
					cc.addss(tmp, MEMBER_PTR(delta));
					cc.movss(MEMBER_PTR(value), tmp);
				}
				IF_(double)
				{
					auto tmp = cc.newXmmSd();
					cc.movsd(tmp, ret);
					cc.addsd(tmp, MEMBER_PTR(delta));
					cc.movsd(MEMBER_PTR(value), tmp);
				}

				cc.dec(stepsToDo);
				cc.mov(MEMBER_PTR(stepsToDo), stepsToDo);
			}

			cc.bind(end);
			return Result::ok();
		});

	return obj;
}

snex::jit::ComplexType::Ptr EventWrapper::createComplexType(Compiler& c, const Identifier& id)
{
	auto obj = new StructType(NamespacedIdentifier(id));

	HiseEvent e;
	int* ptr = reinterpret_cast<int*>(&e);

	obj->addExternalMember("dword1", e, ptr[0]);
	obj->addExternalMember("dword2", e, ptr[1]);
	obj->addExternalMember("dword3", e, ptr[2]);
	obj->addExternalMember("dword4", e, ptr[3]);

	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getNoteNumber);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getVelocity);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getChannel);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, setChannel);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, setVelocity);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, setNoteNumber);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getTimeStamp);

	FunctionClass::Ptr fc = obj->getFunctionClass();

	ADD_INLINER(getChannel,
		{
			SETUP_INLINER(int);
			d->target->createRegister(cc);
			auto n = base.cloneAdjustedAndResized(0x01, 1);
			cc.movsx(INT_REG_W(d->target), n);
			return Result::ok();
		});

	ADD_INLINER(getNoteNumber,
		{
			SETUP_INLINER(int);
			d->target->createRegister(cc);
			auto n = base.cloneAdjustedAndResized(0x02, 1);
			cc.movsx(INT_REG_W(d->target), n);
			return Result::ok();
		});

	ADD_INLINER(getVelocity,
		{
			SETUP_INLINER(int);
			d->target->createRegister(cc);
			auto n = base.cloneAdjustedAndResized(0x03, 1);
			cc.movsx(INT_REG_W(d->target), n);
			return Result::ok();
		});

	ADD_INLINER(setChannel,
		{
			SETUP_INLINER(int);
			auto n = base.cloneAdjustedAndResized(0x01, 1);

			auto v = d->args[0];
			if (IS_MEM(v)) cc.mov(n, INT_IMM(v));
			else cc.mov(n, INT_REG_R(v));

			return Result::ok();
		});

	ADD_INLINER(setNoteNumber,
		{
			SETUP_INLINER(int);
			auto n = base.cloneAdjustedAndResized(0x02, 1);

			auto v = d->args[0];
			if (IS_MEM(v)) cc.mov(n, INT_IMM(v));
			else cc.mov(n, INT_REG_R(v));

			return Result::ok();
		});

	ADD_INLINER(setVelocity,
		{
			SETUP_INLINER(int);
			auto n = base.cloneAdjustedAndResized(0x03, 1);

			auto v = d->args[0];
			if (IS_MEM(v)) cc.mov(n, INT_IMM(v));
			else cc.mov(n, INT_REG_R(v));

			return Result::ok();
		});

	return obj;
}


#define REGISTER_CPP_CLASS(compiler, className) c.registerExternalComplexType(className::createComplexType(c, #className));


void SnexObjectDatabase::registerObjects(Compiler& c, int numChannels)
{

	NamespaceHandler::InternalSymbolSetter iss(c.getNamespaceHandler());

	{
		NamespacedIdentifier iId("IndexType");

		NamespaceHandler::ScopedNamespaceSetter(c.getNamespaceHandler(), iId);

		TemplateObject cf;
		cf.id = iId.getChildId("wrapped");
		auto pId = cf.id.getChildId("ArrayType");

		cf.argList.add(TemplateParameter(pId, false, jit::TemplateParameter::Single));

		cf.functionArgs = [pId]()
		{
			Array<TypeInfo> l;
			l.add(TypeInfo(pId, true, true));
			return l;
		};

		cf.makeFunction = [&c, iId](const TemplateObject::ConstructData& cd)
		{
			if (!cd.expectTemplateParameterAmount(1))
				return;

			if (!cd.expectIsComplexType(0))
				return;
			
			auto f = new FunctionData();

			f->id = cd.id;
			f->returnType = TypeInfo(Types::ID::Dynamic);
			f->templateParameters.add(cd.tp[0]);
			f->addArgs("obj", cd.tp[0].type);

			f->inliner = Inliner::createHighLevelInliner(f->id, [](InlineData* b)
			{
				auto d = b->toSyntaxTreeData();
				d->target = new Operations::Immediate(d->location, 0);



				return Result::ok();
			});

			f->inliner->returnTypeFunction = [](InlineData* b)
			{
				auto rt = dynamic_cast<ReturnTypeInlineData*>(b);
				auto& handler = rt->object->currentCompiler->namespaceHandler;
				auto ct = b->templateParameters[0].type.getTypedIfComplexType<ComplexType>();

				if (ct == nullptr)
					return Result::fail("Can't deduce index type from parameter 1");

				if (auto st = dynamic_cast<StructType*>(ct))
				{
				    auto t = handler.registerComplexTypeOrReturnExisting(new StructSubscriptIndexType(st, Identifier("wrapped")));
					rt->f.returnType = TypeInfo(t);
					return Result::ok();
				}

				SubTypeConstructData sd;
				sd.handler = &handler;
				sd.id = NamespacedIdentifier("wrapped");
				auto subType = ct->createSubType(&sd);
				subType = sd.handler->registerComplexTypeOrReturnExisting(subType);

				rt->f.returnType = TypeInfo(subType);

				if (!sd.r.wasOk())
					return sd.r;

				return Result::ok();
			};

			c.getInbuiltFunctionClass()->addFunction(f);
		};

		c.getNamespaceHandler().addTemplateFunction(cf);
	}


	{
		c.addConstant(NamespacedIdentifier("NumChannels"), numChannels);
		
		auto blockType = new DynType(TypeInfo(Types::ID::Float));
		blockType->setAlias(NamespacedIdentifier("block"));
		c.registerExternalComplexType(blockType);

		auto floatType = TypeInfo(Types::ID::Float);
		auto float2 = new SpanType(floatType, numChannels);

		ComplexType::Ptr channelType = new SpanType(TypeInfo(blockType, false, false), numChannels);
		ComplexType::Ptr frameType = new DynType(TypeInfo(float2, false, false));

		channelType->setAlias(NamespacedIdentifier("ChannelData"));
		frameType->setAlias(NamespacedIdentifier("FrameData"));

		c.registerExternalComplexType(channelType);
		c.registerExternalComplexType(frameType);
	}

	REGISTER_CPP_CLASS(c, sfloat);
	REGISTER_CPP_CLASS(c, sdouble);
	REGISTER_CPP_CLASS(c, PrepareSpecs);
	c.registerExternalComplexType(EventWrapper::createComplexType(c, "HiseEvent"));

	{
		auto dataType = c.getComplexType(NamespacedIdentifier("ChannelData"));

		auto eventType = c.getComplexType(NamespacedIdentifier("HiseEvent"));
		auto eventBufferType = new DynType(TypeInfo(eventType));

		c.registerExternalComplexType(eventBufferType);

		createFrameProcessor(c);
		createProcessData(c, TypeInfo(eventBufferType));
	}

	c.initInbuildFunctions();
	
	TemplateParameter ct;
	ct.constant = numChannels;
	ct.argumentId = NamespacedIdentifier("NumChannels");

	c.registerExternalComplexType(OscProcessData::createType(c));

	auto prototypes = ScriptnodeCallbacks::getAllPrototypes(c, numChannels);

	{
		auto mId = NamespacedIdentifier("wrap").getChildId("midi");
		TemplateClassBuilder midi(c, mId);


		midi.addTypeTemplateParameter("T");
		
		midi.setInitialiseStructFunction([prototypes](const TemplateObject::ConstructData& cd, StructType* st)
		{
			st->addMember("obj", cd.tp[0].type);

			for (auto p : prototypes)
				st->addWrappedMemberMethod("obj", p);
		});
		
		midi.flush();
	}


	{
		ContainerNodeBuilder chain(c, "chain", numChannels);
		chain.flush();

		ContainerNodeBuilder split(c, "split", numChannels);
		split.flush();


	}

	registerParameterTemplate(c);
}


void SnexObjectDatabase::createProcessData(Compiler& c, const TypeInfo& eventType)
{
	NamespacedIdentifier pId("ProcessData");

	TemplateObject ptc;
	
	ptc.id = pId;
	ptc.argList.add(TemplateParameter(pId.getChildId("NumChannels"), 0, false));

	ptc.makeClassType = [eventType](const TemplateObject::ConstructData& c)
	{
		ComplexType::Ptr p;

		if (!c.expectTemplateParameterAmount(1))
			return p;

		if (!c.expectNotIntegerValue(0, 0))
			return p;

		NamespacedIdentifier pId("ProcessData");

		TemplateParameter::List l;
		l.add(c.tp[0]);
		l.getReference(0).argumentId = pId.getChildId("NumChannels");

		jassert(l.getReference(0).argumentId.isValid());

		auto pType = new StructType(pId, l);

		pType->addMember("data", TypeInfo(Types::ID::Pointer, true));
		pType->addMember("events", TypeInfo(Types::ID::Pointer, true));
		pType->addMember("numSamples", TypeInfo(Types::ID::Integer));
		pType->addMember("numEvents", TypeInfo(Types::ID::Integer));
		pType->addMember("numChannels", TypeInfo(Types::ID::Integer));
		pType->addMember("shouldReset", TypeInfo(Types::ID::Integer));

		pType->setDefaultValue("data", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("events", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numSamples", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numEvents", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numChannels", InitialiserList::makeSingleList(c.tp[0].constant));
		pType->setDefaultValue("shouldReset", InitialiserList::makeSingleList(0));

		
		{
			FunctionData subscript;
			subscript.id = pId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::Subscript));
			subscript.returnType = TypeInfo(c.handler->getAliasType(NamespacedIdentifier("block")));
			subscript.addArgs("obj", TypeInfo(Types::ID::Pointer, true, true));
			subscript.addArgs("index", TypeInfo(Types::ID::Integer));
			subscript.inliner = Inliner::createAsmInliner(subscript.id, [pType](InlineData* b)
			{
				auto d = b->toAsmInlineData();

				auto& cc = d->gen.cc;

				auto dynObj = cc.newStack(16, 32);
				cc.mov(dynObj.cloneResized(4), 128);

				auto& dataObject = d->args[0];
				dataObject->loadMemoryIntoRegister(cc);

				auto tmp = cc.newGpq();

				auto indexReg = d->args[1];
				cc.mov(tmp, x86::ptr(PTR_REG_R(dataObject)));

				if (indexReg->isMemoryLocation())
				{
					int value = indexReg->getImmediateIntValue();
					cc.mov(tmp, x86::ptr(tmp, value * sizeof(float*), 8));
				}
				else
				{
					cc.mov(tmp, x86::ptr(tmp, INT_REG_R(indexReg), 3, 8));
				}

				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), tmp);

				int sizeOffset = pType->getMemberOffset("numSamples");

				auto size = x86::ptr(PTR_REG_R(dataObject)).cloneAdjustedAndResized(sizeOffset, sizeof(int));
				auto tmp2 = cc.newGpd();

				cc.mov(tmp2, size);
				cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp2);

				d->target->setCustomMemoryLocation(dynObj, false);

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(subscript);
		}

		auto channelPtrType = new StructType(NamespacedIdentifier("ChannelPtr"));
		channelPtrType->addMember("ptr", TypeInfo(Types::ID::Pointer, true, false));
		channelPtrType->finaliseAlignment();

		ComplexType::Ptr owned = channelPtrType;
		

		TypeInfo channelType(c.handler->registerComplexTypeOrReturnExisting(channelPtrType));

		{
			FunctionData beginF, sizeFunction;
			beginF.id = pId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::BeginIterator));
			beginF.returnType = channelType;
			sizeFunction.id = pId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::SizeFunction));
			sizeFunction.returnType = TypeInfo(Types::ID::Integer);

			beginF.inliner = Inliner::createAsmInliner(beginF.id, [](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->object->loadMemoryIntoRegister(cc);
				d->target->createRegister(cc);

				cc.mov(PTR_REG_W(d->target), x86::qword_ptr(PTR_REG_R(d->object)));
				

				return Result::ok();
			});

			sizeFunction.inliner = Inliner::createAsmInliner(sizeFunction.id, [pType](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;
				jassert(d->object->isActive());
				d->target->createRegister(cc);

				if (!d->templateParameters.isEmpty())
				{
					cc.mov(INT_REG_W(d->target), d->templateParameters[0].constant);
				}
				else
				{
					int sizeOffset = pType->getMemberOffset("numChannels");
					auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));
					cc.mov(INT_REG_W(d->target), size);
					
				}

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(beginF);
			pType->addJitCompiledMemberFunction(sizeFunction);
		}
		
		{
			FunctionData tcd;
			tcd.id = pId.getChildId("toChannelData");
			tcd.returnType = TypeInfo(c.handler->getAliasType(NamespacedIdentifier("block")));
			tcd.addArgs("channelPtr", channelType);

			tcd.inliner = Inliner::createAsmInliner(tcd.id, [pType](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->object->loadMemoryIntoRegister(cc);

				auto dynObj = cc.newStack(16, 0);

				cc.mov(dynObj.cloneResized(4), 128);

				auto data = cc.newGpq();
				cc.mov(data, x86::qword_ptr(PTR_REG_R(d->args[0])));

				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);
				int sizeOffset = pType->getMemberOffset("numSamples");
				auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

				auto tmp = cc.newGpd();

				cc.mov(tmp, size);
				cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp);

				d->target->setCustomMemoryLocation(dynObj, false);

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(tcd);
		}

		{
			FunctionData ted;
			ted.id = pId.getChildId("toEventData");

			auto eventType = c.handler->getComplexType(NamespacedIdentifier("HiseEvent"));
			auto eventBufferType = new DynType(TypeInfo(eventType));
			ted.returnType = TypeInfo(c.handler->registerComplexTypeOrReturnExisting(eventBufferType));

			ted.inliner = Inliner::createAsmInliner(ted.id, [pType](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->object->loadMemoryIntoRegister(cc);

				auto dynObj = cc.newStack(16, 0);

				cc.mov(dynObj.cloneResized(4), 128);
				auto data = cc.newGpq();
				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);

				int eventOffset = pType->getMemberOffset("events");
				auto eventDataPtr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(eventOffset, sizeof(int));

				cc.mov(data, eventDataPtr);
				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);

				int sizeOffset = pType->getMemberOffset("numEvents");
				auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

				auto tmp = cc.newGpd();

				cc.mov(tmp, size);
				cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp);

				d->target->setCustomMemoryLocation(dynObj, false);

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(ted);
		}


		{
			FunctionData tfd;
			tfd.id = pId.getChildId("toFrameData");

			int numChannels = c.tp[0].constant;

			auto frameProcessor = c.handler->createTemplateInstantiation(NamespacedIdentifier("FrameProcessor"), c.tp, *c.r);

			tfd.returnType = TypeInfo(frameProcessor);

			tfd.inliner = Inliner::createAsmInliner(tfd.id, [pType, frameProcessor, numChannels](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				auto size = frameProcessor->getRequiredByteSize();

				auto frameStackData	= cc.newStack(size, 0);

				/*
				span<float*, NumChannels>& channels; // 8 byte
				int frameLimit = 0;					 // 4 byte
				int frameIndex = 0;				     // 4 byte
				FrameType frameData;				 // sizeof(FrameData)
				*/

				cc.mov(frameStackData.cloneResized(8), PTR_REG_R(d->object));


				int sizeOffset = pType->getMemberOffset("numSamples");

				auto channelsPtrReg = cc.newGpq();
				cc.mov(channelsPtrReg, x86::qword_ptr(PTR_REG_R(d->object)));

				auto frameSize = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

				auto sizeReg = cc.newGpd();

				cc.mov(sizeReg, frameSize);
				cc.mov(frameStackData.cloneAdjustedAndResized(8, 4), sizeReg);
				cc.mov(frameStackData.cloneAdjustedAndResized(12, 4), 0);

				auto cReg = cc.newGpq();
				auto fReg = cc.newXmmSs();

				auto frameStart = frameStackData.cloneAdjustedAndResized(16, 4);

				for (int i = 0; i < numChannels; i++)
				{
					cc.mov(cReg, x86::ptr(channelsPtrReg).cloneAdjustedAndResized(i * 8, 4));
					cc.movss(fReg, x86::ptr(cReg));
					cc.movss(frameStart.cloneAdjusted(i * 4), fReg);
				}

				d->target->setCustomMemoryLocation(frameStackData, false);

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(tfd);
		}

		{
			FunctionData numChannelF;
			numChannelF.id = pType->id.getChildId("getNumChannels");
			numChannelF.returnType = TypeInfo(Types::ID::Integer);

			int numChannels = c.tp[0].constant;

			numChannelF.inliner = Inliner::createHighLevelInliner(numChannelF.id, [numChannels](InlineData* b)
			{
				auto d = b->toSyntaxTreeData();

				d->target = new Operations::Immediate(d->location, numChannels);

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(numChannelF);
		}

		{
			FunctionData numSamplesF;
			numSamplesF.id = pType->id.getChildId("getNumSamples");
			numSamplesF.returnType = TypeInfo(Types::ID::Integer);

			
			numSamplesF.inliner = Inliner::createAsmInliner(numSamplesF.id, [](InlineData* b)
			{
				jassertfalse;

				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(numSamplesF);
		}

#if 0
		{
			auto fId = pId.getChildId("FrameProcessor");
			
			auto ftp = c.tp;
			ftp.getReference(0).argumentId = pId.getChildId("NumChannels");

			ComplexType::Ptr frameClass = new StructType(fId, ftp);

			frameClass = c.handler->registerComplexTypeOrReturnExisting(frameClass);

			if (!frameClass->isFinalised())
			{
				auto fc = dynamic_cast<StructType*>(frameClass.get());


				int numChannels = ftp[0].constant;
				ComplexType::Ptr frameData = new SpanType(TypeInfo(Types::ID::Float), numChannels);
				c.handler->registerComplexTypeOrReturnExisting(frameData);

				fc->addMember("channels", TypeInfo(Types::ID::Pointer, true));
				fc->addMember("limit", TypeInfo(Types::ID::Integer));
				fc->addMember("index", TypeInfo(Types::ID::Integer));
				fc->addMember("frameData", TypeInfo(frameData));

				fc->setDefaultValue("index", InitialiserList::makeSingleList(-1));
				fc->setDefaultValue("limit", InitialiserList::makeSingleList(0));

				auto subId = fId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::Subscript));

				FunctionData subscript;
				subscript.id = subId;
				subscript.returnType = TypeInfo(Types::Float, false, true);
				subscript.addArgs("index", TypeInfo(Types::ID::Dynamic));
				subscript.inliner = Inliner::createAsmInliner(subId, [fc](InlineData* b)
				{
					auto d = b->toAsmInlineData();

					auto offset = fc->getMemberOffset("frameData");

					d->gen.emitSpanReference(d->target, d->args[0], d->args[1], sizeof(float), offset);
					
					return Result::ok();
				});

				fc->addJitCompiledMemberFunction(subscript);

				auto nextFrame = FunctionData();
				nextFrame.id = fId.getChildId("next");
				nextFrame.returnType = TypeInfo(Types::ID::Integer);

#define F(channels) if (numChannels == channels) nextFrame.function = reinterpret_cast<void*>(FrameProcessor<channels>::nextFrame);

				F(1); F(2); F(3); F(4);
				F(5); F(6); F(7); F(8);
				F(9); F(10); F(11); F(12);
				F(13); F(14); F(15); F(16);
				
#undef F
				

#if 0
				nextFrame.inliner = Inliner::createAsmInliner(nextFrame.id, [fc, numChannels](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;
					d->target->createRegister(cc);
					auto retValue = INT_REG_W(d->target);

					auto exit = cc.newLabel();
					auto L2 = cc.newLabel();
					auto L3 = cc.newLabel();
					auto mem = d->object->getAsMemoryLocation();
					auto frameOffset = fc->getMemberOffset("frameData");
					
					/* check first type
					if (frameIndex == 0)
					{
						++frameIndex;
						return frameLimit;
					}
					*/

					cc.nop();

					auto limitMem = mem.cloneAdjustedAndResized(fc->getMemberOffset("index"), 4);
					auto limit = cc.newGpd();
					cc.mov(limit, limitMem);
					cc.test(limit, limit);
					cc.jnz(L2);
					cc.inc(limit);
					cc.mov(limitMem, limit);
					cc.mov(retValue, mem.cloneAdjustedAndResized(fc->getMemberOffset("limit"), 4));
					cc.jmp(exit);
					cc.bind(L2);

					/* write last frame

					for (int i = 0; i < NumChannels; i++)
						parent.data[i][frameIndex - 1] = frameData[i];
					*/

					

					for (int i = 0; i < numChannels; i++)
					{
						auto ptr = mem.cloneAdjustedAndResized(i * 8, 8);
						auto dataPtr = cc.newGpq();
						cc.mov(dataPtr, ptr);
						cc.sub(dataPtr, 4);
						auto dst = x86::dword_ptr(dataPtr);
						auto src = mem.cloneAdjustedAndResized(frameOffset + i * 4, 4);
						auto tmp = cc.newXmmSs();
						cc.movss(tmp, src);
						cc.movss(dst, tmp);
					}

					/*
					auto ok = frameIndex < frameLimit;

					if (ok == 0)
						return ok;
					*/

					auto indexReg = cc.newGpd();
					cc.mov(indexReg, mem.cloneAdjustedAndResized(fc->getMemberOffset("index"), 4));
					cc.cmp(limitMem, indexReg);
					cc.jb(L3);
					cc.mov(retValue, 0);
					cc.jmp(exit);
					cc.bind(L3);

					/*
					for (int i = 0; i < NumChannels; i++)
						frameData[i] = parent.data[i][frameIndex];

					++frameIndex;
					return 1;
					*/

					for (int i = 0; i < numChannels; i++)
					{
						auto ptr = mem.cloneAdjustedAndResized(i * 8, 8);
						auto dataPtr = cc.newGpq();
						cc.mov(dataPtr, ptr);
						auto src = x86::dword_ptr(dataPtr);
						auto dst = mem.cloneAdjustedAndResized(frameOffset + i * 4, 4);
						auto tmp = cc.newXmmSs();
						cc.movss(tmp, src);
						cc.movss(dst, tmp);
					}

					cc.mov(retValue, 1);
					cc.bind(exit);

					return Result::ok();
				});
#endif

				fc->addExternalMemberFunction(nextFrame);

	

				fc->finaliseExternalDefinition();

				
				auto toFrame = FunctionData();
				toFrame.id = pId.getChildId("toFrameData");

				toFrame.inliner = Inliner::createAsmInliner(toFrame.id, [numChannels, fc](InlineData* b)
				{
					auto d = b->toAsmInlineData();

					auto& cc = d->gen.cc;

					auto mem = cc.newStack(fc->getRequiredByteSize(), fc->getRequiredAlignment());

					auto channelPtr = mem.cloneAdjustedAndResized(fc->getMemberOffset("channels"), 8);
					auto limit = mem.cloneAdjustedAndResized(fc->getMemberOffset("limit"), 4);
					auto index = mem.cloneAdjustedAndResized(fc->getMemberOffset("index"), 4);
					auto dst = mem.cloneAdjustedAndResized(fc->getMemberOffset("frameData"), 4);

					d->object->loadMemoryIntoRegister(cc);
					auto src = PTR_REG_R(d->object);

					for (int i = 0; i < numChannels; i++)
					{
						auto dynPtr = x86::qword_ptr(src).cloneAdjusted(8 + i * 16);
						auto c = cc.newGpq();
						cc.mov(c, dynPtr);
						cc.mov(channelPtr.cloneAdjusted(8 * i), c);
						auto d = cc.newXmmSs();
						cc.movss(d, x86::dword_ptr(c));
						cc.movss(dst.cloneAdjusted(i * 4), d);
					}

					{
						auto tmp = cc.newGpd();
						cc.mov(tmp, x86::ptr(src).cloneAdjustedAndResized(4, 4));
						cc.mov(limit, tmp);
					}

					cc.mov(index, 0);

					d->target->setCustomMemoryLocation(mem, false);

					return Result::ok();
				});

				auto handler = c.handler;

				toFrame.returnType = TypeInfo(frameClass, false, true);

				pType->addJitCompiledMemberFunction(toFrame);
			}
				

			
		}
#endif

		pType->finaliseExternalDefinition();

		p = pType;

		return p;
	};

	c.addTemplateClass(ptc);

	
}


void SnexObjectDatabase::createFrameProcessor(Compiler& c)
{
	NamespacedIdentifier pId("FrameProcessor");

	TemplateObject ftc;

	ftc.id = pId;
	ftc.argList.add(TemplateParameter(pId.getChildId("NumChannels"), 0, false));

	ftc.makeClassType = [](const TemplateObject::ConstructData& c)
	{
		ComplexType::Ptr p;

		if (!c.expectTemplateParameterAmount(1))
			return p;

		if (!c.expectNotIntegerValue(0, 0))
			return p;

		NamespacedIdentifier fId("FrameProcessor");

		TemplateParameter::List l;
		l.add(c.tp[0]);
		l.getReference(0).argumentId = fId.getChildId("NumChannels");

		jassert(l.getReference(0).argumentId.isValid());

		auto fType = new StructType(fId, l);

		int numChannels = c.tp[0].constant;

		/*
		span<float*, NumChannels>& channels; // 8 byte
		int frameLimit = 0;					 // 4 byte
		int frameIndex = 0;				     // 4 byte
		FrameType frameData;				 // sizeof(FrameData)
		*/

		fType->addMember("channels", TypeInfo(Types::ID::Pointer, true));
		fType->addMember("frameLimit", TypeInfo(Types::ID::Integer));
		fType->addMember("frameIndex", TypeInfo(Types::ID::Integer));

		auto frameDataType = new SpanType(TypeInfo(Types::ID::Float), numChannels);
		fType->addMember("frameData", TypeInfo(c.handler->registerComplexTypeOrReturnExisting(frameDataType)));

		{
			FunctionData beginF;
			beginF.id = fId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::BeginIterator));
			beginF.returnType = TypeInfo(Types::ID::Float, false, true);
			beginF.inliner = Inliner::createAsmInliner(beginF.id, [fType](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;
				auto frameDataOffset = fType->getMemberOffset("frameData");

				X86Mem framePtr;

				if (d->object->isActive())
					framePtr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(frameDataOffset, 8);
				else
					framePtr = d->object->getAsMemoryLocation().cloneAdjustedAndResized(frameDataOffset, 8);

				cc.lea(PTR_REG_W(d->target), framePtr);
				return Result::ok();
			});

			fType->addJitCompiledMemberFunction(beginF);

			FunctionData sizeF;
			sizeF.id = fId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::SizeFunction));
			sizeF.returnType = TypeInfo(Types::ID::Integer);
			sizeF.inliner = Inliner::createAsmInliner(sizeF.id, [numChannels](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				d->target->setImmediateValue(numChannels);
				return Result::ok();
			});

			fType->addJitCompiledMemberFunction(sizeF);
		}
		{
			FunctionData nextF;
			nextF.id = fId.getChildId("next");

			nextF.returnType = TypeInfo(Types::ID::Integer);
			nextF.inliner = Inliner::createAsmInliner(nextF.id, [fType, numChannels](InlineData* b)
			{
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->target->createRegister(cc);

				auto returnValue = INT_REG_W(d->target);

				X86Mem frameStack;

				if (d->object->isActive())
					frameStack = x86::ptr(PTR_REG_W(d->object));
				else
					frameStack = d->object->getAsMemoryLocation();

				auto channelPtrs = frameStack.cloneResized(8);
				auto frameLimit = frameStack.cloneAdjustedAndResized(8, 4);
				auto frameIndex = frameStack.cloneAdjustedAndResized(12, 4);
				auto frameData = frameStack.cloneAdjustedAndResized(16, 4);


				auto exit = cc.newLabel();
				auto writeLastFrame = cc.newLabel();

				cc.mov(returnValue, frameIndex);

				// if (fp->frameIndex == 0)
				cc.cmp(returnValue, 0);
				cc.jne(writeLastFrame);

				//++fp->frameIndex;
				cc.inc(frameIndex);

				//return fp->frameLimit;
				cc.mov(returnValue, frameLimit);
				cc.jmp(exit);

				cc.setInlineComment("Write the last frame");
				cc.bind(writeLastFrame);

				
				auto channelData = cc.newGpq();
				cc.mov(channelData, channelPtrs);
				cc.mov(channelData, x86::qword_ptr(channelData));
				auto tmp = cc.newXmmSs();
				auto cReg = cc.newGpq();

				for (int i = 0; i < numChannels; i++)
				{
					//channels[i][frameIndex - 1] = frameData[i];
					auto cPtr = x86::ptr(channelData).cloneAdjusted(i * 8);
					cc.mov(cReg, cPtr);

					auto src = frameData.cloneAdjusted(i * 4);
					auto dst = x86::ptr(cReg, returnValue, 2, -4, 4);
					
					cc.movss(tmp, src);
					cc.movss(dst, tmp);
				}

				
				// if(fp->frameIndex < fp->frameLimit)
				cc.cmp(returnValue, frameLimit);
				auto finished = cc.newLabel();
				cc.jnb(finished);
				cc.setInlineComment("Load the next frame");

				for (int i = 0; i < numChannels; i++)
				{
					//frameData[i] = channels[i][frameIndex];

					auto cPtr = x86::ptr(channelData).cloneAdjusted(i * 8);
					cc.mov(cReg, cPtr);

					auto src = x86::ptr(cReg, returnValue, 2, 0, 4);
					auto dst = frameData.cloneAdjusted(i * 4);
					
					cc.movss(tmp, src);
					cc.movss(dst, tmp);
				}

				//	++fp->frameIndex;
				cc.inc(frameIndex);

				//	return 1;
				cc.mov(returnValue, 1);
				cc.jmp(exit);
				cc.bind(finished);

				// return 0;
				cc.mov(returnValue, 0);
				cc.bind(exit);
				

				return Result::ok();
			});

			fType->addJitCompiledMemberFunction(nextF);

			{
				FunctionData subscript;
				subscript.id = fId.getChildId(FunctionClass::getSpecialSymbol(jit::FunctionClass::Subscript));
				subscript.returnType = TypeInfo(Types::ID::Float, false, true);
				subscript.addArgs("obj", TypeInfo(Types::ID::Pointer, true)); // break ref-count cycle...
				subscript.addArgs("index", TypeInfo(Types::ID::Integer));
				subscript.inliner = Inliner::createAsmInliner(subscript.id, [](InlineData* b)
				{
					auto d = b->toAsmInlineData();
					auto& cc = d->gen.cc;

					X86Mem frameStack;

					if (d->args[0]->isActive())
						frameStack = x86::ptr(PTR_REG_W(d->args[0]));
					else
						frameStack = d->args[0]->getAsMemoryLocation();

					auto frameData = frameStack.cloneAdjustedAndResized(16, 4);

					if (d->args[1]->isMemoryLocation())
					{
						auto offset = INT_IMM(d->args[1]);
						d->target->setCustomMemoryLocation(frameData.cloneAdjusted(offset * 4), d->args[0]->isGlobalMemory());
					}
					else
					{
						return Result::fail("Can't use non-constant index for frame []-operator");
					}

					return Result::ok();
				});

				fType->addJitCompiledMemberFunction(subscript);
			}
		}


		fType->finaliseExternalDefinition();

		p = fType;

		return p;
	};



	c.addTemplateClass(ftc);
}

void SnexObjectDatabase::registerParameterTemplate(Compiler& c)
{
	using PH = ParameterBuilder::Helpers;
	using TCH = TemplateClassBuilder::Helpers;

	auto empty = new StructType(NamespacedIdentifier("parameter").getChildId("empty"));
	
	auto emptyCallFunction = PH::createCallPrototype(empty, [](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		d->target = new Operations::Noop(d->location);

		return Result::ok();
	});

	empty->addJitCompiledMemberFunction(emptyCallFunction);
	empty->finaliseAlignment();
	
	c.getNamespaceHandler().registerComplexTypeOrReturnExisting(empty);


	auto plain = PH::createWithTP(c, "plain");
	plain.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto input = d->args[0]->clone(d->location);

			auto targetType = TCH::getStructTypeFromTemplate(st, 0);
			d->target       = PH::createSetParameterCall(targetType, d, input);

			return Result::ok();
		});
	});

	plain.flush();

	auto expr = PH::createWithTP(c, "expression");
	expr.addTypeTemplateParameter("ExpressionClass");
	expr.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto exprType =   TCH::getStructTypeFromTemplate(st, 2);
			auto exprCall =   TCH::createFunctionCall(exprType, d, "op", d->args);
			auto targetType = TCH::getStructTypeFromTemplate(st, 0);
			d->target =       PH::createSetParameterCall(targetType, d, exprCall);

			return Result::ok();
		});
	});
	expr.flush();

	auto from0To1 = PH::createWithTP(c, "from0To1");
	from0To1.addTypeTemplateParameter("RangeClass");
	from0To1.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto rangeType =   TCH::getStructTypeFromTemplate(st, 2);
			auto rangeCall =   TCH::createFunctionCall(rangeType, d, "from0To1", d->args);
			auto targetType =  TCH::getStructTypeFromTemplate(st, 0);
			d->target =        PH::createSetParameterCall(targetType, d, rangeCall);

			return Result::ok();
		});
	});
	from0To1.flush();

	auto to0To1 = PH::createWithTP(c, "to0To1");
	to0To1.addTypeTemplateParameter("RangeClass");
	to0To1.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto input = d->args[0]->clone(d->location);

			auto rangeType = TCH::getStructTypeFromTemplate(st, 2);
			auto rangeCall = TCH::createFunctionCall(rangeType, d, "to0To1", d->args);
			auto targetType = TCH::getStructTypeFromTemplate(st, 0);
			d->target = PH::createSetParameterCall(targetType, d, rangeCall);

			return Result::ok();
		});
	});
	to0To1.flush();

	ParameterBuilder chainP(c, "chain");

	chainP.addTypeTemplateParameter("InputRange");
	chainP.addVariadicTypeTemplateParameter("Parameters");
	chainP.addFunction(TemplateClassBuilder::VariadicHelpers::getFunction);
	chainP.setInitialiseStructFunction(TemplateClassBuilder::VariadicHelpers::initVariadicMembers<1>);

	chainP.setConnectFunction([](StructType* st)
	{
		auto cFunc = PH::connectFunction(st);

		cFunc.inliner = Inliner::createHighLevelInliner(cFunc.id, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();



			int parameterIndex = d->templateParameters.getFirst().constant;
			auto targetType = TCH::getStructTypeFromTemplate(st, parameterIndex + 1);
			auto newCall = TCH::createFunctionCall(targetType, d, "connect", d->args);
			TCH::addChildObjectPtr(newCall, d, st, parameterIndex);

			d->target = newCall;
			return Result::ok();
		});

		return cFunc;
	});


	chainP.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			auto rangeType = TCH::getStructTypeFromTemplate(st, 0);
			auto rangeCall = TCH::createFunctionCall(rangeType, d, "to0To1", d->args);

			if (rangeCall == nullptr)
			{
				d->location.throwError("Can't find function " + rangeType->toString() + "::to0To1(double)");
			}

			

			d->args.set(0, rangeCall);

			d->target = TemplateClassBuilder::VariadicHelpers::callEachMember(d, st, "call", 1);

			return Result::ok();
		});
	});
	chainP.flush();

	ParameterBuilder listP(c, "list");

	listP.addVariadicTypeTemplateParameter("Parameters");
	listP.addFunction(TemplateClassBuilder::VariadicHelpers::getFunction);
	listP.setInitialiseStructFunction(TemplateClassBuilder::VariadicHelpers::initVariadicMembers<0>);
	listP.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			auto value = d->templateParameters.getFirst().constant;
			auto targetType = TCH::getStructTypeFromTemplate(st, value);
			auto newCall = TCH::createFunctionCall(targetType, d, "call", d->args);
			TCH::addChildObjectPtr(newCall, d, st, value);

			d->target = newCall;

			return Result::ok();
		});
	});
	listP.flush();
}

snex::jit::ComplexType::Ptr PrepareSpecs::createComplexType(Compiler& c, const Identifier& id)
{
	PrepareSpecs obj;

	auto st = new StructType(NamespacedIdentifier(id));
	ADD_SNEX_STRUCT_MEMBER(st, obj, sampleRate);
	ADD_SNEX_STRUCT_MEMBER(st, obj, blockSize);
	ADD_SNEX_STRUCT_MEMBER(st, obj, numChannels);

	return c.registerExternalComplexType(st);
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

		auto t = new SpanType(TypeInfo(Types::ID::Float), numChannels);
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
		
	}

	return f;
}

snex::ComplexType* OscProcessData::createType(Compiler& c)
{
	OscProcessData d;

	auto st = CREATE_SNEX_STRUCT(OscProcessData);
	auto blockType = c.getNamespaceHandler().getAliasType(NamespacedIdentifier("block")).getComplexType();

	ADD_SNEX_STRUCT_COMPLEX(st, blockType, d, data);

	ADD_SNEX_STRUCT_MEMBER(st, d, uptime);
	ADD_SNEX_STRUCT_MEMBER(st, d, delta);
	ADD_SNEX_STRUCT_MEMBER(st, d, voiceIndex);

	return st;
}

}
}
