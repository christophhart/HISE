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

struct PolyDataBuilder : public TemplateClassBuilder
{
	static bool isPolyphonyEnabled(StructType* st)
	{
		auto numVoices = st->getTemplateInstanceParameters()[1].constant;
		auto enabled = st->getCompiler()->getGlobalScope().getPolyHandler()->isEnabled();

		return numVoices > 1 && enabled;
	}

	PolyDataBuilder(Compiler& c) :
		TemplateClassBuilder(c, NamespacedIdentifier("PolyData"))
	{
		addTypeTemplateParameter("ElementType");
		addIntTemplateParameter("NumVoices");

		setInitialiseStructFunction(Functions::init);

		addFunction(Functions::prepareFunction);
		addFunction(Functions::beginFunction);
		addFunction(Functions::sizeFunction);
		addFunction(Functions::getFunction);
		addFunction(Functions::getVoiceIndexForData);



	};

private:

	

	struct Functions
	{
		static void init(const TemplateObject::ConstructData& cd, StructType* st)
		{
			auto dataType = cd.handler->registerComplexTypeOrReturnExisting(new SpanType(cd.tp[0].type, cd.tp[1].constant));

			st->addMember("voiceIndex", TypeInfo(Types::ID::Pointer, true));
			st->addMember("lastVoiceIndex", TypeInfo(Types::ID::Integer));
			st->addMember("unused", TypeInfo(Types::ID::Integer));

			st->addMember("data", TypeInfo(dataType));

			st->setDefaultValue("voiceIndex", InitialiserList::makeSingleList(VariableStorage(nullptr, 0)));
			st->setDefaultValue("lastVoiceIndex", InitialiserList::makeSingleList(VariableStorage(-1)));
			st->setDefaultValue("unused", InitialiserList::makeSingleList(VariableStorage(0)));
		};

		

#if SNEX_ASMJIT_BACKEND
		static Result prepareInliner(InlineData* b)
		{
			auto d = b->toAsmInlineData();
			auto& cc = d->gen.cc;

			auto prepareType = d->args[0]->getTypeInfo().getTypedComplexType<StructType>();
			auto polyDataType = d->object->getTypeInfo().getTypedComplexType<StructType>();

			auto sourceOffset = prepareType->getMemberOffset("voiceIndex");
			auto targetOffset = polyDataType->getMemberOffset("voiceIndex");

			jassertEqual(targetOffset, 0);
			
			auto r = cc.newGpq();

			d->object->loadMemoryIntoRegister(cc);

			cc.mov(r, x86::ptr(PTR_REG_R(d->args[0])).cloneAdjustedAndResized(sourceOffset, 8));
			cc.mov(x86::ptr(PTR_REG_R(d->object)), r);

			return Result::ok();
		}

		

		static Result getVoiceIndexForDataInliner(InlineData* b)
		{
			auto d = b->toAsmInlineData();
			
			auto& cc = d->gen.cc;

			d->target->createRegister(d->gen.cc);
			d->object->loadMemoryIntoRegister(d->gen.cc);

			auto polyDataType = d->object->getTypeInfo().getTypedComplexType<StructType>();
			auto dataOffset = polyDataType->getMemberOffset("data");

			auto targetMem = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(dataOffset, 8);

			AsmCodeGenerator::TemporaryRegister voiceReg(d->gen, d->object->getScope(), Types::ID::Integer);

			cc.lea(INT_REG_W(voiceReg.tempReg), targetMem);
			cc.lea(PTR_REG_W(d->target), d->args[0]->getMemoryLocationForReference());
			cc.sub(PTR_REG_W(d->target), INT_REG_R(voiceReg.tempReg));
			cc.shr(PTR_REG_W(d->target), 3);

			return Result::ok();
		}

		static Result getInliner(InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto polyDataType = d->object->getTypeInfo().getTypedComplexType<StructType>();

			auto dataOffset = polyDataType->getMemberOffset("data");
			auto dataType = polyDataType->getTemplateInstanceParameters()[0].type.withModifiers(false, true);
			auto elementSize = dataType.getRequiredByteSizeNonZero();

			if (isPolyphonyEnabled(polyDataType))
			{
				auto& cc = d->gen.cc;
				d->target->createRegister(cc);
				AsmCodeGenerator::TemporaryRegister voiceReg(d->gen, d->object->getScope(), Types::ID::Integer);
				TypeInfo ptrType(Types::ID::Pointer, true);

				d->object->loadMemoryIntoRegister(cc);

				auto& tPtr = PTR_REG_W(d->target);
				auto& vPtr = INT_REG_W(voiceReg.tempReg);
				auto& oPtr = PTR_REG_R(d->object);
				
				cc.mov(tPtr, x86::ptr(oPtr));

				asmGetVoiceIndex(d->gen, d->target, voiceReg.tempReg);

				// If voice index is -1, make it zero...
				auto tmp = cc.newGpd();
				cc.mov(tmp, vPtr);
				cc.xor_(vPtr, vPtr);
				cc.test(tmp, tmp);
				cc.cmovns(vPtr, tmp);

				cc.mov(tPtr, oPtr);
				cc.imul(vPtr, elementSize);
				cc.add(tPtr, (int)dataOffset);
				cc.add(tPtr, vPtr.r64());
				
				return Result::ok();
			}
			else
			{
				d->target->createRegister(d->gen.cc);
				d->object->loadMemoryIntoRegister(d->gen.cc);

				auto targetMem = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(dataOffset, 8);
				d->gen.cc.lea(PTR_REG_W(d->target), targetMem);
				return Result::ok();
			}
		}


		static void asmGetVoiceIndex(AsmCodeGenerator& gen, AssemblyRegister::Ptr objReg, AssemblyRegister::Ptr retReg)
		{
			auto& cc = gen.cc;

			retReg->createRegister(cc);
			objReg->loadMemoryIntoRegister(cc);

			auto& o = PTR_REG_R(objReg);
			auto& r = INT_REG_W(retReg);

			auto threadIsZero = cc.newLabel();
			auto branchEnd = cc.newLabel();

			cc.xor_(r, r);
			cc.cmp(o, 0);

			auto tmp = cc.newGpd();
			cc.mov(tmp, -1);
			cc.cmove(r, tmp);

			cc.jz(branchEnd);
			cc.cmp(x86::ptr(o).cloneResized(8), 0);
			cc.jz(threadIsZero);

			FunctionData f;
			f.returnType = Types::ID::Integer;
			f.function = (void*)PolyHandler::getVoiceIndexStatic;
			f.setConst(true);
			AssemblyRegister::List l;
			auto ok = gen.emitFunctionCall(retReg, f, objReg, l);

			cc.imul(r, x86::ptr(o).cloneAdjustedAndResized(12, 4));
			cc.jmp(branchEnd);
			cc.bind(threadIsZero);
			cc.mov(r, x86::ptr(o).cloneAdjustedAndResized(8, 4));
			cc.imul(r, x86::ptr(o).cloneAdjustedAndResized(12, 4));
			cc.bind(branchEnd);

		}
#else

		static Result prepareInliner(InlineData* b) { jassertfalse; return Result::ok(); };
		static Result getVoiceIndexForDataInliner(InlineData* b) { jassertfalse; return Result::ok(); };
		static Result getInliner(InlineData* b) { jassertfalse; return Result::ok(); };
		static Result asmGetVoiceIndex(InlineData* b) { jassertfalse; return Result::ok(); };

#endif

		static FunctionData prepareFunction(StructType* st)
		{
			auto f = ScriptnodeCallbacks::getPrototype(st->getCompiler(), ScriptnodeCallbacks::PrepareFunction, 2);

			f.id = st->id.getChildId(f.id.getIdentifier());

			f.inliner = Inliner::createAsmInliner({}, prepareInliner);

			return f;
		}

		static FunctionData getVoiceIndexForData(StructType* st)
		{
			FunctionData f;

			f.id = st->id.getChildId("getVoiceIndexForData");
			f.returnType = Types::ID::Integer;
			f.setConst(true);
			
			auto argType = st->getTemplateInstanceParameters()[0].type.withModifiers(true, true, false);

			f.addArgs("data", argType);

			f.inliner = Inliner::createAsmInliner({}, getVoiceIndexForDataInliner);

			return f;
		}

		static FunctionData beginFunction(StructType* st)
		{
			FunctionData f;

			f.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::BeginIterator));
			f.returnType = st->getTemplateInstanceParameters()[0].type.withModifiers(false, true, false);
			f.inliner = Inliner::createAsmInliner({}, getInliner);

			return f;
		}

		static FunctionData getFunction(StructType* st)
		{
			FunctionData f;
			f.id = st->id.getChildId("get");
			f.returnType = st->getTemplateInstanceParameters()[0].type.withModifiers(false, true);
			f.inliner = Inliner::createAsmInliner({}, getInliner);
			return f;
		}


		static FunctionData sizeFunction(StructType* st)
		{
			FunctionData f;

			f.id = st->id.getChildId(FunctionClass::getSpecialSymbol({}, FunctionClass::SizeFunction));
			f.returnType = TypeInfo(Types::ID::Integer);


			f.inliner = Inliner::createAsmInliner({}, [](InlineData* b)
			{
#if SNEX_ASMJIT_BACKEND
				auto d = b->toAsmInlineData();

				auto polyDataType = d->object->getTypeInfo().getTypedComplexType<StructType>();

				if (!isPolyphonyEnabled(polyDataType))
					d->target->setImmediateValue(1);
				else
				{
					auto& cc = d->gen.cc;

					AsmCodeGenerator::TemporaryRegister polyPtrReg(d->gen, d->object->getScope(), TypeInfo(Types::ID::Pointer, true));

					d->object->loadMemoryIntoRegister(cc);
					cc.mov(polyPtrReg.get(), x86::ptr(PTR_REG_R(d->object)));

					asmGetVoiceIndex(d->gen, polyPtrReg.tempReg, d->target);

					// if the voice index is -1, make it numVoices
					// otherwise make it 1
					auto& tPtr = INT_REG_W(d->target);
					auto tmp = cc.newGpd();
					
					cc.cmp(tPtr, -1);
					cc.mov(tmp, 18);
					cc.mov(tPtr, 1);
					cc.cmove(tPtr, tmp);
				}
#endif
				return Result::ok();
			});


			return f;
		}
	};
};


#define REGISTER_CPP_CLASS(compiler, className) c.registerExternalComplexType(className::createComplexType(c, #className));

#define REGISTER_ORIGINAL_CPP_CLASS(compiler, className, originalClass) c.registerExternalComplexType(className::createComplexType(c, #originalClass));


template <typename T>
jit::ComplexType::Ptr RampWrapper<T>::createComplexType(Compiler& c, const Identifier& id)
{
	Type s;

	auto obj = new StructType(NamespacedIdentifier(id));

	ADD_SNEX_PRIVATE_MEMBER(obj, s, value);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, targetValue);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, delta);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, stepDivider);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, numSteps);
	ADD_SNEX_PRIVATE_MEMBER(obj, s, stepsToDo);

	ADD_SNEX_STRUCT_METHOD(obj, RampWrapper<T>, reset);

	ADD_SNEX_STRUCT_METHOD(obj, RampWrapper<T>, set);
	SET_SNEX_PARAMETER_IDS(obj, "newValue");

	ADD_SNEX_STRUCT_METHOD(obj, RampWrapper<T>, advance);
	ADD_SNEX_STRUCT_METHOD(obj, RampWrapper<T>, get);
	ADD_SNEX_STRUCT_METHOD(obj, RampWrapper<T>, prepare);
	SET_SNEX_PARAMETER_IDS(obj, "sampleRate", "fadeTimeMilliSeconds");
	ADD_SNEX_STRUCT_METHOD(obj, RampWrapper<T>, isActive);

	FunctionClass::Ptr fc = obj->getFunctionClass();

	obj->injectInliner("advance", Inliner::HighLevel, [](InlineData* b)
	{
		using namespace cppgen;

		Base c(Base::OutputType::AddTabs);

		cppgen::StatementBlock b1(c);
		c << "if (this->stepsToDo <= 0)";
		c << "	  return this->value;";
		c << "else";
		{
			cppgen::StatementBlock b(c);
			c << "auto v = this->value;";
			c << "this->value += this->delta;";
			c << "this->stepsToDo -= 1;";
			c << "return v;";
		}

		return SyntaxTreeInlineParser(b, {}, c).flush();
	});

	obj->injectInliner("reset", Inliner::HighLevel, [](InlineData* b)
	{
		using namespace cppgen;

		Base c(Base::OutputType::StatementListWithoutSemicolon);

		c << "stepsToDo = 0";
		c << "value = targetValue";

		auto t = Types::Helpers::getTypeName(Types::Helpers::getTypeFromTypeId<T>());
		String l; l << "delta = (" << t << ")0"; c << l;

		return SyntaxTreeInlineParser(b, {}, c).flush();
	});

	obj->injectInliner("set", Inliner::HighLevel, [](InlineData* b)
	{
		using namespace cppgen;

		Base c(Base::OutputType::AddTabs);

		c << "if (this->numSteps == 0)";
		{
			cppgen::StatementBlock sb(c);
			c << "this->targetValue = newTargetValue;";
			c << "this->reset();";
		}
		c << "else";
		{
			cppgen::StatementBlock sb(c);
			c << "auto d = newTargetValue - this->value;";
			c << "this->delta = d * this->stepDivider;";
			c << "this->targetValue = newTargetValue;";
			c << "this->stepsToDo = this->numSteps;";
		}

		return SyntaxTreeInlineParser(b, {"newTargetValue"}, c).flush();
	});

#if SNEX_ASMJIT_BACKEND
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
#endif


	return obj->finaliseAndReturn();
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
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, isNoteOn);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getFrequency);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, isEmpty);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, getEventId);
	ADD_SNEX_STRUCT_METHOD(obj, EventWrapper, clear);

	obj->setCustomDumpFunction([](void* ptr)
	{
		auto e = static_cast<HiseEvent*>(ptr);

		String s;

		s << "| HiseEvent { " << e->toDebugString() + " }\n";
		return s;
	});

	

	FunctionClass::Ptr fc = obj->getFunctionClass();

#if SNEX_ASMJIT_BACKEND
	ADD_INLINER(getChannel,
	{
		SETUP_INLINER(int);
		d->target->createRegister(cc);
		auto n = base.cloneAdjustedAndResized(0x01, 1);
		cc.movsx(INT_REG_W(d->target), n);
		return Result::ok();
	});

	ADD_INLINER(isEmpty,
	{
		SETUP_INLINER(int);
		d->target->createRegister(cc);

		auto n1 = base.cloneAdjustedAndResized(0, 8);
		auto n2 = base.cloneAdjustedAndResized(1, 8);
		cc.or_(INT_REG_W(d->target), n1);
		cc.or_(INT_REG_W(d->target), n2);
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
		if (IS_MEM(v)) cc.mov(n, (int64_t)INT_IMM(v));
		else cc.mov(n, INT_REG_R(v));

		return Result::ok();
	});

	ADD_INLINER(setNoteNumber,
	{
		SETUP_INLINER(int);
		auto n = base.cloneAdjustedAndResized(0x02, 1);

		auto v = d->args[0];
		if (IS_MEM(v)) cc.mov(n, (int64_t)INT_IMM(v));
		else cc.mov(n, INT_REG_R(v));

		return Result::ok();
	});

	ADD_INLINER(setVelocity,
	{
		SETUP_INLINER(int);
		auto n = base.cloneAdjustedAndResized(0x03, 1);

		auto v = d->args[0];
		if (IS_MEM(v)) cc.mov(n, (int64_t)INT_IMM(v));
		else cc.mov(n, INT_REG_R(v));

		return Result::ok();
	});
#endif

	return obj->finaliseAndReturn();
}


snex::jit::ComplexType::Ptr ModValueJit::createComplexType(Compiler& c, const Identifier& id)
{
	ModValue d;

	auto st = CREATE_SNEX_STRUCT(ModValue);

	ADD_SNEX_STRUCT_MEMBER(st, d, changed);
	ADD_SNEX_STRUCT_MEMBER(st, d, modValue);

	ADD_SNEX_STRUCT_METHOD(st, ModValueJit, getChangedValue);
	ADD_SNEX_STRUCT_METHOD(st, ModValueJit, getModValue);
	ADD_SNEX_STRUCT_METHOD(st, ModValueJit, setModValue);
	ADD_SNEX_STRUCT_METHOD(st, ModValueJit, setModValueIfChanged);

	return st->finaliseAndReturn();
}


snex::jit::ComplexType::Ptr DataReadLockJIT::createComplexType(Compiler& c, const Identifier& id)
{
	DataReadLockJIT l;

	auto st = CREATE_SNEX_STRUCT(DataReadLock);

	ADD_SNEX_STRUCT_MEMBER(st, l, complexDataPtr);
	ADD_SNEX_STRUCT_MEMBER(st, l, holdsLock);

	st->setVisibility("complexDataPtr", NamespaceHandler::Visibility::Private);
	st->setVisibility("holdsLock", NamespaceHandler::Visibility::Private);

	ComplexType::Ptr ed = c.getComplexType(NamespacedIdentifier("ExternalData"));

	FunctionData cf;
	cf.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor));
	cf.addArgs("data", TypeInfo(ed, false, true));
	cf.addArgs("tryRead", Types::ID::Integer);
	cf.setDefaultParameter("tryRead", VariableStorage(0));
	cf.returnType = Types::ID::Void;
	cf.function = (void*)Wrappers::constructor;
	st->addExternalMemberFunction(cf);

	FunctionData df;
	df.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Destructor));
	df.returnType = Types::ID::Void;
	df.function = (void*)Wrappers::destructor;
	st->addExternalMemberFunction(df);

	FunctionData lf;
	lf.id = st->id.getChildId("isLocked");
	lf.returnType = Types::ID::Integer;
    lf.function = (void*)Wrappers::isLocked;
	lf.setConst(true);
	lf.inliner = Inliner::createHighLevelInliner(lf.id, [](InlineData* b)
	{
		cppgen::Base c;
		c << "return this->holdsLock;";
		return SyntaxTreeInlineParser(b, {}, c).flush();
	});

	st->addExternalMemberFunction(lf);

	return st->finaliseAndReturn();
}

snex::jit::ComplexType::Ptr OscProcessDataJit::createComplexType(Compiler& c, const Identifier& id)
{
	OscProcessData d;

	auto st = CREATE_SNEX_STRUCT(OscProcessData);
	auto blockType = c.getNamespaceHandler().getAliasType(NamespacedIdentifier("block")).getComplexType();

	ADD_SNEX_STRUCT_COMPLEX(st, blockType, d, data);

	ADD_SNEX_STRUCT_MEMBER(st, d, uptime);
	ADD_SNEX_STRUCT_MEMBER(st, d, delta);

	return st->finaliseAndReturn();
}

struct ExternalDataTemplateBuilder: public TemplateClassBuilder
{
	ComplexType::Ptr getExternalDataType()
	{
		auto t = c.getComplexType(NamespacedIdentifier("ExternalData"));
		jassert(t != nullptr);
		return t;
	}

	static Result createEmbeddedSetExternalData(InlineData* b)
	{
#if 0
		auto d = b->toAsmInlineData();

		auto st = d->args[0]->getTypeInfo().getTypedComplexType<StructType>();

		auto comp = st->getCompiler();

		jassert(comp != nullptr);

		auto ed = comp->getComplexType(NamespacedIdentifier("ExternalData"));
		
		FunctionClass::Ptr fc = st->getFunctionClass();
		auto f = fc->getNonOverloadedFunction(fc->getClassName().getChildId("setExternalData"));

		jassert(f.function != nullptr);

		

		AsmCodeGenerator::TemporaryRegister edReg(d->gen, d->object->getScope(), TypeInfo(ed, true, true));
		AsmCodeGenerator::TemporaryRegister indexReg(d->gen, d->object->getScope(), Types::ID::Integer);

		auto& cc = d->gen.cc;

		auto mem = cc.newStack(ed->getRequiredByteSize(), ed->getRequiredAlignment());
		edReg.tempReg->setCustomMemoryLocation(mem, true);

		/*
		DataType dataType; 4
		int numSamples;    4
		int numChannels;   4
		void* data;        8
		void* obj;         8
		UpdateFunction f;  8
		*/

		d->gen.emitStackInitialisation(edReg.tempReg, ed, nullptr, ed->makeDefaultInitialiserList());

		auto objType = d->object->getTypeInfo().getTypedComplexType<StructType>();

		int size = -1;

		if (auto dataType = objType->getMemberTypeInfo("embeddedData").getTypedComplexType<StructType>()->getMemberTypeInfo("data").getTypedIfComplexType<SpanType>())
		{
			size = dataType->getNumElements();
		}
		else
		{
			return Result::fail("expected span type");
		}

		cc.mov(mem.cloneAdjustedAndResized(0, 4), (int)ExternalData::DataType::ConstantLookUp);
		cc.mov(mem.cloneAdjustedAndResized(4, 4), (int)size);
		cc.mov(mem.cloneAdjustedAndResized(8, 4), 1);
		cc.mov(mem.cloneAdjustedAndResized(12, 4), 0);

		d->object->loadMemoryIntoRegister(cc);

		cc.mov(mem.cloneAdjustedAndResized(16, 8), PTR_REG_R(d->object));

		indexReg.tempReg->setImmediateValue(0);

		AssemblyRegister::List innerArgs;
		innerArgs.add(edReg.tempReg);
		innerArgs.add(indexReg.tempReg);

		d->gen.emitFunctionCall(d->target, f, d->args[0], innerArgs);

		return Result::ok();

#else
		cppgen::Base c;

		c << "ExternalData d(this->embeddedData);";
		c << "n.setExternalData(d, 0);";

		return SyntaxTreeInlineParser(b, { "n", "b", "index" }, c).flush();
#endif
	}

	static Result createExternalSetExternalData(InlineData* b)
	{
		cppgen::Base c;

		auto d = b->toSyntaxTreeData()->object->getTypeInfo().getTypedComplexType<StructType>()->getTemplateInstanceParameters()[0].constant;

		String cond;

		cond << "if (index == " << String(d) << ")";

		c << cond;
		c << "    n.setExternalData(b, 0);";

		return SyntaxTreeInlineParser(b, { "n", "b", "index" }, c).flush();
	}

	ExternalDataTemplateBuilder(Compiler& c, bool isEmbedded, ExternalData::DataType d) :
		TemplateClassBuilder(c, getId(isEmbedded, d))
	{
		if (isEmbedded)
			addTypeTemplateParameter("DataClass");
		else
			addIntTemplateParameter("Index");

		setInitialiseStructFunction([d, isEmbedded](const TemplateObject::ConstructData& data, StructType* st)
		{
			ExternalData::forEachType([st, d](ExternalData::DataType t)
			{
				auto id = ExternalData::getNumIdentifier(t);
				st->setInternalProperty(id, d == t ? 1 : 0);
			});

			if (isEmbedded)
			{
				auto t = Helpers::getSubTypeFromTemplate(st, 0);

				st->addMember("embeddedData", TypeInfo(Helpers::getSubTypeFromTemplate(st, 0), false, false));
				st->setDefaultValue("embeddedData", t->makeDefaultInitialiserList());
			}
		});

		auto ed = getExternalDataType();

		addFunction([](StructType* st)
		{
			FunctionData cf;
			cf.id = st->id.getChildId(st->getConstructorId());
			cf.addArgs("obj", TypeInfo(Types::ID::Dynamic, false, true));
			

			

			cf.returnType = Types::ID::Void;

			cf.inliner = Inliner::createAsmInliner({}, [](InlineData* b)
			{
				return Result::ok();
			});

			return cf;
		});

		addFunction([ed, isEmbedded](StructType* st)
		{
			FunctionData f;
			f.id = st->id.getChildId("setExternalData");

			f.addArgs("obj", TypeInfo(Types::ID::Dynamic, true, true));
			f.addArgs("data", TypeInfo(ed, true, true));
			f.addArgs("index", Types::ID::Integer);
			f.returnType = Types::ID::Void;

			if (isEmbedded)
				f.inliner = Inliner::createHighLevelInliner({}, createEmbeddedSetExternalData);
			else
				f.inliner = Inliner::createHighLevelInliner({}, createExternalSetExternalData);

			return f;
		});
	}

	static NamespacedIdentifier getId(bool isEmbedded, ExternalData::DataType d)
	{
		NamespacedIdentifier s("data");
		if (isEmbedded)
			s = s.getChildId("embedded");
		else
			s = s.getChildId("external");

		s = s.getChildId(ExternalData::getDataTypeName(d).toLowerCase());

		return s;
	}
};

Result DataLibraryBuilder::registerTypes()
{
	auto fid = NamespacedIdentifier(getFactoryId());

	auto st = new StructType(fid.getChildId("base"));

	c.registerExternalComplexType(st);

	auto st2 = new StructType(fid.getChildId("filter_node_base"));
	
	c.registerExternalComplexType(st2);

	return Result::ok();
}

void InbuiltTypeLibraryBuilder::createExternalDataTemplates()
{
	ExternalData::forEachType([this](ExternalData::DataType t)
	{
		ExternalDataTemplateBuilder embed(c, true, t);
		ExternalDataTemplateBuilder ext(c, false, t);
		embed.flush();
		ext.flush();
	});
}

void InbuiltTypeLibraryBuilder::createProcessData(const TypeInfo& eventType)
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

		auto blockType = TypeInfo(c.handler->getAliasType(NamespacedIdentifier("block")));

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

		auto numChannels = c.tp[0].constant;

		pType->setDefaultValue("data", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("events", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numSamples", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numEvents", InitialiserList::makeSingleList(0));
		pType->setDefaultValue("numChannels", InitialiserList::makeSingleList(numChannels));
		pType->setDefaultValue("shouldReset", InitialiserList::makeSingleList(0));

		pType->setCustomDumpFunction([numChannels](void* obj)
		{
			String s;

			auto d = static_cast<ProcessDataDyn*>(obj);

			s << "| ProcessData<" << numChannels << ">\t{ ";
			s << "numSamples: " << d->getNumSamples() << " }\n";


			for (int i = 0; i < numChannels; i++)
			{
				s << "|   Channel " << numChannels << "\t{ }\n";

				auto chPtr = d->getRawChannelPointers()[i];

				for (int j = 0; j < d->getNumSamples(); j++)
				{
					String t;
					t << "d[" << i << "][" << j << "]";

					Types::Helpers::dumpNativeData(s, 2, t, chPtr, chPtr + i, sizeof(float), Types::ID::Float);
				}
			}

			return s;
		});

		{
			FunctionData subscript;
			subscript.id = pId.getChildId(FunctionClass::getSpecialSymbol(pId, jit::FunctionClass::Subscript));
			subscript.returnType = blockType;
			subscript.addArgs("obj", TypeInfo(Types::ID::Pointer, true, true));
			subscript.addArgs("index", TypeInfo(Types::ID::Integer));

			subscript.inliner = Inliner::createAsmInliner(subscript.id, [pType](InlineData* b)
			{
#if SNEX_ASMJIT_BACKEND
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				auto dynObj = cc.newStack(16, 32);
				cc.mov(dynObj.cloneResized(4), 128);

				auto dataObject = d->args[0];
				dataObject->loadMemoryIntoRegister(cc);

				auto tmp = cc.newGpq();

				auto indexReg = d->args[1];
				cc.mov(tmp, x86::ptr(PTR_REG_R(dataObject)));

				if (indexReg->isMemoryLocation())
				{
					int value = indexReg->getImmediateIntValue();

					auto numChannels = pType->getTemplateInstanceParameters()[0].constant;

					if (value >= numChannels)
						return Result::fail("channel index out of bounds");

					cc.mov(tmp, x86::ptr(tmp, value * sizeof(float*), 8));
				}
				else
				{
					cc.mov(tmp, x86::ptr(tmp, INT_REG_R(indexReg), 3, 8));
				}

				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), tmp);

				auto sizeOffset = (int64_t)pType->getMemberOffset("numSamples");

				auto size = x86::ptr(PTR_REG_R(dataObject)).cloneAdjustedAndResized(sizeOffset, sizeof(int));
				auto tmp2 = cc.newGpd();

				cc.mov(tmp2, size);
				cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp2);

				d->target->setCustomMemoryLocation(dynObj, false);
#endif
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
#if SNEX_ASMJIT_BACKEND

				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->object->loadMemoryIntoRegister(cc);
				d->target->createRegister(cc);

				cc.mov(PTR_REG_W(d->target), x86::qword_ptr(PTR_REG_R(d->object)));
#endif
				return Result::ok();
			});

			sizeFunction.inliner = Inliner::createAsmInliner(sizeFunction.id, [pType](InlineData* b)
			{
#if SNEX_ASMJIT_BACKEND
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
					auto sizeOffset = (int64_t)pType->getMemberOffset("numChannels");
					auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));
					cc.mov(INT_REG_W(d->target), size);

				}

#endif
				return Result::ok();
			});

			pType->addJitCompiledMemberFunction(beginF);
			pType->addJitCompiledMemberFunction(sizeFunction);
		}

		{
			FunctionData tcd;
			tcd.id = pId.getChildId("toChannelData");
			tcd.returnType = blockType;
			tcd.addArgs("channelPtr", channelType);


			tcd.inliner = Inliner::createAsmInliner(tcd.id, [pType](InlineData* b)
			{
#if SNEX_ASMJIT_BACKEND
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->object->loadMemoryIntoRegister(cc);
				d->args[0]->loadMemoryIntoRegister(cc);

				auto dynObj = cc.newStack(16, 0);

				cc.mov(dynObj.cloneResized(4), 128);

				auto data = cc.newGpq();
				cc.mov(data, x86::qword_ptr(PTR_REG_R(d->args[0])));

				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);
				auto sizeOffset = (int64_t)pType->getMemberOffset("numSamples");
				auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

				auto tmp = cc.newGpd();

				cc.mov(tmp, size);
				cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp);

				d->target->setCustomMemoryLocation(dynObj, false);
#endif
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
#if SNEX_ASMJIT_BACKEND
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				d->object->loadMemoryIntoRegister(cc);

				auto dynObj = cc.newStack(16, 0);

				cc.mov(dynObj.cloneResized(4), 128);
				auto data = cc.newGpq();
				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);

				auto eventOffset = (int64_t)pType->getMemberOffset("events");
				auto eventDataPtr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(eventOffset, sizeof(int));

				cc.mov(data, eventDataPtr);
				cc.mov(dynObj.cloneAdjustedAndResized(8, 8), data);

				auto sizeOffset = (int64_t)pType->getMemberOffset("numEvents");
				auto size = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(sizeOffset, sizeof(int));

				auto tmp = cc.newGpd();

				cc.mov(tmp, size);
				cc.mov(dynObj.cloneAdjustedAndResized(4, 4), tmp);

				d->target->setCustomMemoryLocation(dynObj, false);
#endif
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
#if SNEX_ASMJIT_BACKEND
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;

				auto size = frameProcessor->getRequiredByteSize();

				auto frameStackData = cc.newStack((uint32_t)size, 0);

				/*
				span<float*, NumChannels>& channels; // 8 byte
				int frameLimit = 0;					 // 4 byte
				int frameIndex = 0;				     // 4 byte
				FrameType frameData;				 // sizeof(FrameData)
				*/

				cc.mov(frameStackData.cloneResized(8), PTR_REG_R(d->object));

				auto channelsPtrReg = cc.newGpq();
				cc.mov(channelsPtrReg, x86::qword_ptr(PTR_REG_R(d->object)));

				auto sizeOffset = (int64_t)pType->getMemberOffset("numSamples");
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

#endif
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


		pType->finaliseExternalDefinition();

		p = pType;

		return p;
	};

	c.addTemplateClass(ptc);
}

void InbuiltTypeLibraryBuilder::createFrameProcessor()
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

		static const NamespacedIdentifier fId("FrameProcessor");

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

		auto frameDataType_ = new SpanType(TypeInfo(Types::ID::Float), numChannels);
		auto frameType = TypeInfo(c.handler->registerComplexTypeOrReturnExisting(frameDataType_));
		fType->addMember("frameData", frameType);

		{
			FunctionData beginF;
			beginF.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::BeginIterator));
			beginF.returnType = TypeInfo(Types::ID::Float, false, true);


			beginF.inliner = Inliner::createAsmInliner(beginF.id, [fType](InlineData* b)
			{
#if SNEX_ASMJIT_BACKEND
				auto d = b->toAsmInlineData();
				auto& cc = d->gen.cc;
				auto frameDataOffset = fType->getMemberOffset("frameData");

				X86Mem framePtr;

				if (d->object->isActive())
					framePtr = x86::ptr(PTR_REG_R(d->object)).cloneAdjustedAndResized(frameDataOffset, 8);
				else
					framePtr = d->object->getAsMemoryLocation().cloneAdjustedAndResized(frameDataOffset, 8);

				cc.lea(PTR_REG_W(d->target), framePtr);
#endif
				return Result::ok();
			});


			fType->addJitCompiledMemberFunction(beginF);

			FunctionData sizeF;
			sizeF.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::SizeFunction));
			sizeF.returnType = TypeInfo(Types::ID::Integer);

			sizeF.inliner = Inliner::createHighLevelInliner(sizeF.id, [numChannels](InlineData* b)
			{
				auto d = b->toSyntaxTreeData();
				d->target = new Operations::Immediate(d->location, numChannels);
				return Result::ok();
			});

			fType->addJitCompiledMemberFunction(sizeF);
		}
		{
			FunctionData nextF;
			nextF.id = fId.getChildId("next");

			nextF.returnType = TypeInfo(Types::ID::Integer);

#if SNEX_ASMJIT_BACKEND
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
#endif

			fType->addJitCompiledMemberFunction(nextF);

			{
				FunctionData subscript;
				subscript.id = fId.getChildId(FunctionClass::getSpecialSymbol({}, jit::FunctionClass::Subscript));
				subscript.returnType = TypeInfo(Types::ID::Float, false, true);
				subscript.addArgs("obj", TypeInfo(Types::ID::Pointer, true)); // break ref-count cycle...
				subscript.addArgs("index", TypeInfo(Types::ID::Integer));

#if SNEX_ASMJIT_BACKEND
				subscript.inliner = Inliner::createAsmInliner(subscript.id, [](InlineData* b)
				{
					auto d = b->toAsmInlineData();

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
#endif

				fType->addJitCompiledMemberFunction(subscript);
			}

			{
				FunctionData ts;
				ts.id = fId.getChildId("toSpan");
				ts.returnType = frameType.withModifiers(false, true, false);
				ts.inliner = Inliner::createHighLevelInliner({}, [](InlineData* b)
				{
					auto d = b->toSyntaxTreeData();

					auto fType = d->object->getTypeInfo().getTypedComplexType<StructType>();

					auto mt = fType->getMemberTypeInfo("frameData");
					auto mo = (int)fType->getMemberOffset("frameData");

					d->target = new Operations::MemoryReference(d->location, d->object->clone(d->location), mt, mo);

					return Result::ok();
				});

				fType->addJitCompiledMemberFunction(ts);
			}
		}


		fType->finaliseExternalDefinition();

		p = fType;

		return p;
	};

	c.addTemplateClass(ftc);
}

void InbuiltTypeLibraryBuilder::registerBuiltInFunctions()
{
	{
		c.addConstant(NamespacedIdentifier("NumChannels"), numChannels);

		auto blockType = c.getNamespaceHandler().getComplexType(NamespacedIdentifier("block"));

		jassert(blockType != nullptr);

		auto floatType = TypeInfo(Types::ID::Float);
		auto float2 = new SpanType(floatType, numChannels);

		ComplexType::Ptr channelType = new SpanType(TypeInfo(blockType, false, false), numChannels);
		ComplexType::Ptr frameType = new DynType(TypeInfo(c.registerExternalComplexType(float2), false, false));

		channelType->setAlias(NamespacedIdentifier("ChannelData"));
		frameType->setAlias(NamespacedIdentifier("FrameData"));

		c.registerExternalComplexType(channelType);
		c.registerExternalComplexType(frameType);
	}

	c.initInbuildFunctions();

	registerRangeFunctions();
}

void InbuiltTypeLibraryBuilder::registerRangeFunctions()
{
	FunctionClass::Ptr bf = c.getInbuiltFunctionClass();

	{
		{
			auto st = new StructType(NamespacedIdentifier::fromString("ranges::Identity"));

			

			FunctionData to0To1;
			to0To1.id = st->id.getChildId("to0To1");
			to0To1.returnType = TypeInfo(Types::ID::Double, false, false, true);
			to0To1.addArgs("input", Types::ID::Double);

			st->addJitCompiledMemberFunction(to0To1);

			st->injectInliner("to0To1", Inliner::HighLevel, [](InlineData* b)
			{
				cppgen::Base c;
				c << "return value;";
				return SyntaxTreeInlineParser(b, { "value" }, c).flush();
			});

			c.registerExternalComplexType(st);
		}

		// static constexpr T from0To1(T min, T max, T value) { return hmath::map(value, min, max); }
		addRangeFunction(bf.get(), "from0To1", { "min", "max", "value" }, 
							 "{ return Math.map(value, min, max); }");

		// static constexpr T to0To1(T min, T max, T value) { return (value - min) / (max - min); }
		addRangeFunction(bf.get(), "to0To1", { "min", "max", "value" },
							 "{ return (value - min) / (max - min); }");

		// static constexpr T from0To1Skew(T min, T max, T skew, T value) { return from0To1(min, max, hmath::pow(value, skew)); }
		addRangeFunction(bf.get(), "from0To1Skew", { "min", "max", "skew", "value" },
							 "{ return min + (max - min) * Math.exp(Math.log(value) / skew); }");

		// static constexpr T to0To1Skew(T min, T max, T skew, T value) { return hmath::pow(to0To1(min, max, value), skew); }
		addRangeFunction(bf.get(), "to0To1Skew", { "min", "max", "skew", "value" },
							 "{ return Math.pow(to0To1(min, max, value), skew); }");

		// static constexpr T to0To1Step(T min, T max, T step, T value) { return to0To1(min, max, value - hmath::fmod(value, step)); }
		addRangeFunction(bf.get(), "to0To1Step", { "min", "max", "step", "value" },
							 "{ return to0To1(min, max, value - Math.fmod(value, step)); }");

		// static constexpr T from0To1Step(T min, T max, T step, T value) { auto v = from0To1(min, max, value); return v - hmath::fmod(v, step); }
		addRangeFunction(bf.get(), "from0To1Step", { "min", "max", "step", "value" },
			"{ return from0To1(min, max, value) - Math.fmod(from0To1(min, max, value), step); }");
	}
}

snex::jit::FunctionData* InbuiltTypeLibraryBuilder::createRangeFunction(const Identifier& id, int numArgs, Inliner::InlineType type, const Inliner::Func& inliner)
{
	auto f = new FunctionData();

	f->id = NamespacedIdentifier("ranges").getChildId(id);
	f->returnType = TypeInfo(Types::ID::Double);

	for (int i = 0; i < numArgs; i++)
	{
		Identifier id("a" + String(i + 1));
		f->addArgs(id, TypeInfo(Types::ID::Double));
	}

	f->inliner = Inliner::createFromType({}, type, inliner);

	NamespaceHandler::ScopedNamespaceSetter sns(c.getNamespaceHandler(), f->id.getParent());

	c.getNamespaceHandler().addSymbol(f->id, f->returnType, NamespaceHandler::Function, {});

	return f;
}

void InbuiltTypeLibraryBuilder::addRangeFunction(FunctionClass* fc, const Identifier& id, const StringArray& parameters, const String& code)
{
	fc->addFunction(createRangeFunction(id, parameters.size(), Inliner::HighLevel, [code, parameters](InlineData* b)
	{
		using namespace cppgen;

		Base c(Base::OutputType::WrapInBlock);
		c << code;

		SyntaxTreeInlineParser p(b, parameters, c);
		return p.flush();
	}));
}

juce::Result InbuiltTypeLibraryBuilder::registerTypes()
{
	registerBuiltInFunctions();

	REGISTER_ORIGINAL_CPP_CLASS(c, RampWrapper<float>, sfloat);
	REGISTER_ORIGINAL_CPP_CLASS(c, RampWrapper<double>, sdouble);
	REGISTER_ORIGINAL_CPP_CLASS(c, OscProcessDataJit, OscProcessData);
	REGISTER_ORIGINAL_CPP_CLASS(c, PrepareSpecsJIT, PrepareSpecs);
	REGISTER_ORIGINAL_CPP_CLASS(c, EventWrapper, HiseEvent);

	REGISTER_ORIGINAL_CPP_CLASS(c, SampleDataJIT, MonoSample);
	REGISTER_ORIGINAL_CPP_CLASS(c, SampleDataJIT, StereoSample);

	REGISTER_ORIGINAL_CPP_CLASS(c, ExternalDataJIT, ExternalData);
	REGISTER_ORIGINAL_CPP_CLASS(c, DataReadLockJIT, DataReadLock);

	REGISTER_ORIGINAL_CPP_CLASS(c, ModValueJit, ModValue);

	c.registerExternalComplexType(new StructType(NamespacedIdentifier("ExternalFunctionMap")));

	REGISTER_ORIGINAL_CPP_CLASS(c, ExternalFunctionJIT, ExternalFunction);

	

	auto eventType = c.getComplexType(NamespacedIdentifier("HiseEvent"));
	auto eventBufferType = new DynType(TypeInfo(eventType));
	eventBufferType->setAlias(NamespacedIdentifier("HiseEventBuffer"));

	c.registerExternalComplexType(eventBufferType);
	createFrameProcessor();
	createProcessData(TypeInfo(eventBufferType));

	createExternalDataTemplates();

	PolyDataBuilder polyDataBuilder(c);
	
	polyDataBuilder.flush();

	return Result::ok();
}






}
}
