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

	ADD_SNEX_PRIVATE_MEMBER(obj, s, value);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, targetValue);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, delta);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, stepDivider);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, numSteps);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, stepsToDo);

	ADD_SNEX_STRUCT_METHOD(obj, Type, reset);
	
	ADD_SNEX_STRUCT_METHOD(obj, Type, set);
	SET_SNEX_PARAMETER_IDS(obj, "newValue");

	ADD_SNEX_STRUCT_METHOD(obj, Type, advance);
	ADD_SNEX_STRUCT_METHOD(obj, Type, get);
	ADD_SNEX_STRUCT_METHOD(obj, Type, prepare);
	SET_SNEX_PARAMETER_IDS(obj, "sampleRate", "fadeTimeMilliSeconds");

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

	obj->addExternalMember("dword1", e, ptr[0], NamespaceHandler::Visibility::Private);
	obj->addExternalMember("dword2", e, ptr[1], NamespaceHandler::Visibility::Private);
	obj->addExternalMember("dword3", e, ptr[2], NamespaceHandler::Visibility::Private);
	obj->addExternalMember("dword4", e, ptr[3], NamespaceHandler::Visibility::Private);

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





template <class Node> struct JitNodeWrapper
{
	using T = scriptnode::core::fix_delay;

	struct Wrapper
	{
		static void prepare(void* obj, PrepareSpecs specs)
		{
			static_cast<T*>(obj)->prepare(specs);
		}

		static void reset(void* obj)
		{
			static_cast<T*>(obj)->reset();
		};

		template <int NumChannels> static void process(void* obj, ProcessData<NumChannels>& data)
		{
			static_cast<T*>(obj)->process(data);
		}

		template <int NumChannels> static void processFrame(void* obj, span<float, NumChannels>& data)
		{
			static_cast<T*>(obj)->processFrame(data);
		}

		static void handleHiseEvent(void* obj, HiseEvent& e)
		{
			static_cast<T*>(obj)->handleHiseEvent(e);
		}

		static void construct(void* target)
		{
			volatile T* dst = new (target)T();
			jassert(dst != nullptr);
		}
	};

	static ComplexType::Ptr create(Compiler& c, int numChannels, const Identifier& factoryId)
	{
		auto id = NamespacedIdentifier(factoryId).getChildId(T::getStaticId());
		auto st = new StructType(id);
		
		auto prepaFunction = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::PrepareFunction, numChannels);
		auto eventFunction = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::HandleEventFunction, numChannels);
		auto resetFunction = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::ResetFunction, numChannels);

		st->addJitCompiledMemberFunction(prepaFunction);
		st->addJitCompiledMemberFunction(eventFunction);
		st->addJitCompiledMemberFunction(resetFunction);

		auto addProcessCallbacks = [&](int i)
		{
			auto pi = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::ProcessFunction, i);
			auto fi = ScriptnodeCallbacks::getPrototype(c, ScriptnodeCallbacks::ProcessFrameFunction, i);

			st->addJitCompiledMemberFunction(pi);
			st->addJitCompiledMemberFunction(fi);

			if (i == 1)
			{
				pi.function = (void*)Wrapper::process<1>;
				fi.function = (void*)Wrapper::processFrame<1>;
			}

			if (i == 2)
			{
				pi.function = (void*)Wrapper::process<2>;
				fi.function = (void*)Wrapper::processFrame<2>;
			}

			st->injectMemberFunctionPointer(pi, pi.function);
			st->injectMemberFunctionPointer(fi, fi.function);
		};

		addProcessCallbacks(2);
		addProcessCallbacks(1);
		
		st->injectMemberFunctionPointer(prepaFunction, Wrapper::prepare);
		st->injectMemberFunctionPointer(eventFunction, Wrapper::handleHiseEvent);
		st->injectMemberFunctionPointer(resetFunction, Wrapper::reset);

		T object;
		scriptnode::ParameterDataList list;
		object.createParameters(list);

		for (int i = 0; i < list.size(); i++)
		{
			auto p = list.getReference(i);

			FunctionData f;

			f.id = st->id.getChildId("setParameter");
			f.templateParameters.add(TemplateParameter(f.id.getChildId("P"), i, true, jit::TemplateParameter::Single));
			f.returnType = TypeInfo(Types::ID::Void);
			f.addArgs("value", TypeInfo(Types::ID::Double));

			st->addJitCompiledMemberFunction(f);
			f.function = p.dbNew.getFunction();
			st->injectMemberFunctionPointer(f, p.dbNew.getFunction());
		}

		{
			FunctionData constructor;
			constructor.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, jit::FunctionClass::Constructor));

			constructor.returnType = TypeInfo(Types::ID::Void);

			st->addJitCompiledMemberFunction(constructor);
			st->injectMemberFunctionPointer(constructor, Wrapper::construct);
		}

		st->setSizeFromObject(object);

		return st;
	}
};


static void funkyWasGeht(void* obj, void* data)
{
	jassertfalse;
}

void SnexObjectDatabase::registerObjects(Compiler& c, int numChannels)
{
	NamespaceHandler::InternalSymbolSetter iss(c.getNamespaceHandler());

	{
		NamespacedIdentifier iId("IndexType");

		NamespaceHandler::ScopedNamespaceSetter(c.getNamespaceHandler(), iId);

		TemplateObject cf({ iId.getChildId("wrapped"), {} });
		auto pId = cf.id.id.getChildId("ArrayType");

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

			f->id = cd.id.id;
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
		
		auto blockType = c.getNamespaceHandler().getComplexType(NamespacedIdentifier("block"));

		auto floatType = TypeInfo(Types::ID::Float);
		auto float2 = new SpanType(floatType, numChannels);

		ComplexType::Ptr channelType = new SpanType(TypeInfo(blockType, false, false), numChannels);
		ComplexType::Ptr frameType = new DynType(TypeInfo(c.registerExternalComplexType(float2), false, false));

		channelType->setAlias(NamespacedIdentifier("ChannelData"));
		frameType->setAlias(NamespacedIdentifier("FrameData"));

		c.registerExternalComplexType(channelType);
		c.registerExternalComplexType(frameType);
	}

	REGISTER_CPP_CLASS(c, sfloat);
	REGISTER_CPP_CLASS(c, sdouble);
	
	c.registerExternalComplexType(PrepareSpecsJIT::createComplexType(c, "PrepareSpecs"));
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


		midi.addTypeTemplateParameter("NodeType");
		
		midi.setInitialiseStructFunction([prototypes](const TemplateObject::ConstructData& cd, StructType* st)
		{
			st->addMember("obj", cd.tp[0].type);

			for (auto p : prototypes)
				st->addWrappedMemberMethod("obj", p);
		});

		midi.addFunction(WrapBuilder::Helpers::constructorFunction);

		midi.flush();
	}


	{
		ContainerNodeBuilder chain(c, "chain", numChannels);
		chain.setDescription("Processes all nodes serially");

		chain.flush();

		ContainerNodeBuilder split(c, "split", numChannels);
		split.setDescription("Copies the signal, processes all nodes parallel and sums up the processed signal at the end");
		split.flush();


	}

	{
		WrapBuilder init(c, "init", numChannels);
		init.addTypeTemplateParameter("InitialiserClass");

		init.addInitFunction([](const TemplateObject::ConstructData& cd, StructType* st)
		{
			auto ic = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 1);

			st->addMember("initialiser", TypeInfo(ic, false, false));
			InitialiserList::Ptr di = new InitialiserList();
			di->addChild(new InitialiserList::MemberPointer("obj"));
			st->setDefaultValue("initialiser", di);
		});

		init.addFunction([](StructType* st)
		{
			FunctionData f;

			f.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor));
			f.returnType = TypeInfo(Types::ID::Void);

			f.inliner = Inliner::createHighLevelInliner(f.id, [st](InlineData* b)
			{
				auto d = b->toSyntaxTreeData();

				auto ic = st->getMemberComplexType(Identifier("initialiser"));

				FunctionClass::Ptr fc = ic->getFunctionClass();

				auto icf = fc->getSpecialFunction(FunctionClass::Constructor);

				auto nc = new Operations::FunctionCall(d->location, nullptr, Symbol(icf.id, TypeInfo(Types::ID::Void)), icf.templateParameters);

				auto initRef = new Operations::MemoryReference(d->location, d->object, TypeInfo(ic, false), st->getMemberOffset(1));
				auto objRef = new Operations::MemoryReference(d->location, d->object, st->getMemberTypeInfo("obj").withModifiers(false, true, false), 0);

				nc->setObjectExpression(initRef);
				nc->addArgument(objRef);

				if (icf.canBeInlined(true))
				{
					SyntaxTreeInlineData sd(nc, {});
					sd.object = initRef->clone(d->location);
					sd.path = d->path;
					sd.templateParameters = d->templateParameters;
					auto r = icf.inlineFunction(&sd);

					if (!r.wasOk())
						return r;

					d->target = sd.target;
				}
				else
				{
					d->target = nc;
				}

				return Result::ok();
			});

			return f;
		});

		init.flush();
	}

	{
		WrapBuilder os(c, "fix", "NumChannels", numChannels);

		os.injectExternalFunction("process", funkyWasGeht);

		os.flush();
	}

	{
		WrapBuilder fb(c, "fix_block", "BlockSize", numChannels);

		fb.mapToExternalTemplateFunction(ScriptnodeCallbacks::PrepareFunction, [](const TemplateParameter::List& tp)
		{
			HashMap<int, void*> map;

#define INSERT(b) map.set({b}, (void*)scriptnode::wrap::static_functions::fix_block<b>::prepare);
			INSERT(16);
			INSERT(32);
			INSERT(64);
			INSERT(128);
			INSERT(256);
			INSERT(512);
#undef INSERT

			return map[tp[0].constant];
		});

		fb.mapToExternalTemplateFunction(ScriptnodeCallbacks::ProcessFunction, [](const TemplateParameter::List& tp)
		{
			struct Key
			{
				Key(int b, int c) : blocksize(b), channelAmount(c) { }
				String toString() const { return String(blocksize << 16 | channelAmount); }
				int blocksize; int channelAmount;
			};

			HashMap<String, void*> map;

#define INSERT(b, c) map.set(Key(b, c).toString(), (void*)scriptnode::wrap::static_functions::fix_block<b>::process<Types::ProcessData<c>>);
			INSERT(16, 1);  INSERT(16, 2);
			INSERT(32, 1);  INSERT(32, 2);
			INSERT(64, 1);  INSERT(64, 2);
			INSERT(128, 1); INSERT(128, 2);
			INSERT(256, 1); INSERT(256, 2);
			INSERT(512, 1); INSERT(512, 2);
#undef INSERT
			
			return map[Key(tp[0].constant, WrapBuilder::Helpers::getChannelFromFixData(tp[2].type)).toString()];
		});
		
		fb.flush();
	}

	registerParameterTemplate(c);

	c.registerExternalComplexType(JitNodeWrapper<scriptnode::core::fix_delay>::create(c, numChannels, "core"));
}


void SnexObjectDatabase::createProcessData(Compiler& c, const TypeInfo& eventType)
{
	NamespacedIdentifier pId("ProcessData");

	TemplateObject ptc({ pId, {} });
	ptc.argList.add(TemplateParameter(pId.getChildId("NumChannels"), 0, false));
	ptc.description = "An object containing the data for the multichannel processing";

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

		pType->addMember("data", TypeInfo(Types::ID::Pointer, true), "the pointer to the channel values");
		pType->addMember("events", TypeInfo(Types::ID::Pointer, true), "a list containing all events for the current block");
		pType->addMember("numSamples", TypeInfo(Types::ID::Integer), "the number of samples to process");
		pType->addMember("numEvents", TypeInfo(Types::ID::Integer), "the number of events in this block");
		pType->addMember("numChannels", TypeInfo(Types::ID::Integer), "the number of channels to process");
		pType->addMember("shouldReset", TypeInfo(Types::ID::Integer), "flag that checks if the processing should be reseted");

		pType->setDefaultValue("data", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("events", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numSamples", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numEvents", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numChannels", InitialiserList::makeSingleList(c.tp[0].constant));
		pType->setDefaultValue("shouldReset", InitialiserList::makeSingleList(0));

		
		{
			FunctionData subscript;
			subscript.id = pId.getChildId(FunctionClass::getSpecialSymbol(pId, jit::FunctionClass::Subscript));
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
			beginF.id = pId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::BeginIterator));
			beginF.returnType = channelType;
			sizeFunction.id = pId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::SizeFunction));
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
				d->args[0]->loadMemoryIntoRegister(cc);

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

			auto frameProcessor = c.handler->createTemplateInstantiation({ NamespacedIdentifier("FrameProcessor"), {} }, c.tp, *c.r);

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

	TemplateObject ftc({ pId,{} });
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
			beginF.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::BeginIterator));
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
			sizeF.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::SizeFunction));
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
				subscript.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::Subscript));
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
	plain.setDescription("A plain parameter connection to a certain parameter index of a given node type. The value will be passed to the target without any conversion.");
	plain.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto input = d->args[0]->clone(d->location);

			auto targetType = TCH::getSubTypeFromTemplate(st, 0);
			d->target       = PH::createSetParameterCall(targetType, d, input);

			return Result::ok();
		});
	});

	plain.flush();

	auto expr = PH::createWithTP(c, "expression");
	expr.setDescription("A parameter with an expression that is evaluated before sending the value to the destination.  \nThe expression class must have a function `static double ExpressionClass::op(double input);`");
	expr.addTypeTemplateParameter("ExpressionClass");
	expr.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto exprType =   TCH::getSubTypeFromTemplate(st, 2);
			auto exprCall =   TCH::createFunctionCall(exprType, d, "op", d->args);
			auto targetType = TCH::getSubTypeFromTemplate(st, 0);
			d->target =       PH::createSetParameterCall(targetType, d, exprCall);

			return Result::ok();
		});
	});
	expr.flush();

	auto from0To1 = PH::createWithTP(c, "from0To1");
	from0To1.addTypeTemplateParameter("RangeClass");
	from0To1.setDescription("A parameter that converts a normalised value to a given range before sending it to the destination. The RangeClass must have a `static double from0To1(double input);` method.");

	from0To1.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto rangeType =   TCH::getSubTypeFromTemplate(st, 2);
			auto rangeCall =   TCH::createFunctionCall(rangeType, d, "from0To1", d->args);
			auto targetType =  TCH::getSubTypeFromTemplate(st, 0);
			d->target =        PH::createSetParameterCall(targetType, d, rangeCall);

			return Result::ok();
		});
	});
	from0To1.flush();

	auto to0To1 = PH::createWithTP(c, "to0To1");
	to0To1.addTypeTemplateParameter("RangeClass");
	to0To1.setDescription("A parameter connection that sends a normalised value to the target. The input value will be scaled based on the Range class which needs a `static double to0To1(double input);` method.");
	to0To1.addFunction([](StructType* st)
	{
		return PH::createCallPrototype(st, [st](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();
			auto input = d->args[0]->clone(d->location);

			auto rangeType = TCH::getSubTypeFromTemplate(st, 2);
			auto rangeCall = TCH::createFunctionCall(rangeType, d, "to0To1", d->args);
			auto targetType = TCH::getSubTypeFromTemplate(st, 0);
			d->target = PH::createSetParameterCall(targetType, d, rangeCall);

			return Result::ok();
		});
	});
	to0To1.flush();

	ParameterBuilder chainP(c, "chain");

	chainP.addTypeTemplateParameter("InputRange");
	chainP.setDescription("A parameter connection to multiple targets. The `Parameters` argument can be a list of other parameter classes.  \nThe input value will be normalised using the `InputRange` class, so you most probably want to scale the values back using `parameter::from0To1` (or a custom scaling using `parameter::expression`).");
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
			auto targetType = TCH::getSubTypeFromTemplate(st, parameterIndex + 1);
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

			auto rangeType = TCH::getSubTypeFromTemplate(st, 0);
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
			auto targetType = TCH::getSubTypeFromTemplate(st, value);
			auto newCall = TCH::createFunctionCall(targetType, d, "call", d->args);
			TCH::addChildObjectPtr(newCall, d, st, value);

			d->target = newCall;

			return Result::ok();
		});
	});
	listP.flush();
}

snex::jit::ComplexType::Ptr PrepareSpecsJIT::createComplexType(Compiler& c, const Identifier& id)
{
	PrepareSpecs obj;

	auto st = new StructType(NamespacedIdentifier(id));
	ADD_SNEX_STRUCT_MEMBER(st, obj, sampleRate);
	ADD_SNEX_STRUCT_MEMBER(st, obj, blockSize);
	ADD_SNEX_STRUCT_MEMBER(st, obj, numChannels);

	return c.registerExternalComplexType(st);
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

JitCompiledNode::JitCompiledNode(Compiler& c, const String& code, const String& classId, int numChannels_, const CompilerInitFunction& cf) :
	r(Result::ok()),
	numChannels(numChannels_)
{
	String s;

	auto implId = NamespacedIdentifier::fromString("impl::" + classId);

	s << "namespace impl { " << code;
	s << "}\n";
	s << implId.toString() << " instance;\n";

	cf(c, numChannels);
	
	Array<Identifier> fIds;

	for (auto f : Types::ScriptnodeCallbacks::getAllPrototypes(c, numChannels))
	{
		addCallbackWrapper(s, f);
		fIds.add(f.id.getIdentifier());
	}

	obj = c.compileJitObject(s);

	r = c.getCompileResult();

	if (r.wasOk())
	{
		NamespacedIdentifier impl("impl");

		if (instanceType = c.getComplexType(implId))
		{
			if (auto libraryNode = dynamic_cast<SnexNodeBase*>(instanceType.get()))
			{
				parameterList = libraryNode->getParameterList();
			}
			if (auto st = dynamic_cast<StructType*>(instanceType.get()))
			{
				auto pId = st->id.getChildId("Parameters");
				auto pNames = c.getNamespaceHandler().getEnumValues(pId);

				if (!pNames.isEmpty())
				{
					for (int i = 0; i < pNames.size(); i++)
						addParameterMethod(s, pNames[i], i);
				}

				cf(c, numChannels);

				obj = c.compileJitObject(s);
				r = c.getCompileResult();

				instanceType = c.getComplexType(implId);

				for (auto& n : pNames)
				{
					auto f = obj[Identifier("set" + n)];

					OpaqueSnexParameter osp;
					osp.name = n;
					osp.function = f.function;
					parameterList.add(osp);
				}
			}

			FunctionClass::Ptr fc = instanceType->getFunctionClass();

			thisPtr = obj.getMainObjectPtr();
			ok = true;

			for (int i = 0; i < fIds.size(); i++)
			{
				callbacks[i] = obj[fIds[i]];

				Array<FunctionData> matches;

				fc->addMatchingFunctions(matches, fc->getClassName().getChildId(fIds[i]));

				FunctionData wrappedFunction;

				if (matches.size() == 1)
					wrappedFunction = matches.getFirst();
				else
				{
					for (auto m : matches)
					{
						if (m.matchesArgumentTypes(callbacks[i]))
						{
							wrappedFunction = m;
							break;
						}
					}
				}

				if (!wrappedFunction.matchesArgumentTypes(callbacks[i]))
				{
					r = Result::fail(wrappedFunction.getSignature({}, false) + " doesn't match " + callbacks[i].getSignature({}, false));
					ok = false;
					break;
				}

				if (callbacks[i].function == nullptr)
					ok = false;
			}

			
		}
		else
		{
			jassertfalse;
		}
	}
}

}
}
