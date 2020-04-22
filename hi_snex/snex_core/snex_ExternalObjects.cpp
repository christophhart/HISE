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
		c.addConstant(NamespacedIdentifier("NumChannels"), numChannels);
		
		auto blockType = new DynType(TypeInfo(Types::ID::Float));
		blockType->setAlias(NamespacedIdentifier("block"));
		c.registerExternalComplexType(blockType);

		auto floatType = TypeInfo(Types::ID::Float);
		auto float2 = new SpanType(floatType, numChannels);

		ComplexType::Ptr channelType = new SpanType(TypeInfo(blockType), numChannels);
		ComplexType::Ptr frameType = new DynType(TypeInfo(float2));
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

		createProcessData(c, TypeInfo(eventBufferType));
	}

	c.initInbuildFunctions();
	
	TemplateParameter ct;
	ct.constant = numChannels;
	ct.argumentId = NamespacedIdentifier("NumChannels");

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
		auto blockType = c.handler->getComplexType(NamespacedIdentifier("block"));
		auto channelType = new SpanType(TypeInfo(blockType), c.tp[0].constant);

		pType->addMember("data", TypeInfo(channelType));
		pType->addMember("events", TypeInfo(eventType));
		pType->addMember("voiceIndex", TypeInfo(Types::ID::Integer));
		pType->addMember("shouldReset", TypeInfo(Types::ID::Integer));

		auto vDefault = new InitialiserList();
		vDefault->addImmediateValue(0);

		auto sDefault = new InitialiserList();
		sDefault->addImmediateValue(0);

		pType->setDefaultValue("voiceIndex", vDefault);
		pType->setDefaultValue("shouldReset", sDefault);

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

				ComplexType::Ptr channelData = new SpanType(TypeInfo(Types::ID::Pointer, true), numChannels);
				c.handler->registerComplexTypeOrReturnExisting(channelData);

				fc->addMember("channels", TypeInfo(channelData));
				fc->addMember("frameData", TypeInfo(frameData));
				fc->addMember("limit", TypeInfo(Types::ID::Integer));
				fc->addMember("index", TypeInfo(Types::ID::Integer));

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

		pType->finaliseExternalDefinition();

		p = pType;

		return p;
	};

	c.addTemplateClass(ptc);

	
}


void SnexObjectDatabase::registerParameterTemplate(Compiler& c)
{
	using PH = ParameterBuilder::Helpers;
	using TCH = TemplateClassBuilder::Helpers;

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
			auto rangeCall = TCH::createFunctionCall(rangeType, d, "from0To1", d->args);

			if (rangeCall == nullptr)
			{
				d->location.throwError("Can't find function " + rangeType->toString() + "::from0To1(double)");
			}

			d->args[0] = rangeCall;

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
		f.addArgs("specs", TypeInfo(c.getComplexType(NamespacedIdentifier("PrepareSpecs"), {})));
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

}
}
