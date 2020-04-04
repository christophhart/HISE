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

struct ResetData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("reset"); }

	static void fillSignature(Compiler&, FunctionData& d, const TemplateParameter::List& tp)
	{
		d.returnType = TypeInfo(Types::ID::Void);
	}
};

struct ProcessSingleData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("processSingle"); }

	static void fillSignature(Compiler& c, FunctionData& d, const TemplateParameter::List& tp)
	{
		ComplexType::Ptr st = new SpanType(TypeInfo(Types::ID::Float), tp[0].constant);
		c.registerExternalComplexType(st);

		d.returnType = TypeInfo(Types::ID::Void);
		d.addArgs("frameData", TypeInfo(st, false));
	}
};



struct ProcessFunctionData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("process"); };

	static void fillSignature(Compiler& c, FunctionData& d, const TemplateParameter::List& tp)
	{
		auto pId = NamespacedIdentifier("ProcessData");
		auto st =  c.getComplexType(pId, tp);

		jassert(st != nullptr);
		

		d.returnType = TypeInfo(Types::ID::Void);
		d.addArgs("data", TypeInfo(st, false, true));
	}
};



struct PrepareData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("prepare"); }

	static void fillSignature(Compiler& c, FunctionData& d, const TemplateParameter::List& tp)
	{
		auto prepareSpecId = NamespacedIdentifier("PrepareSpecs");

		auto st = dynamic_cast<StructType*>(c.getComplexType(prepareSpecId).get());

		jassert(st != nullptr);

		d.returnType = TypeInfo(Types::ID::Void);
		d.addArgs("ps", TypeInfo(st));
	}

	
};

template <class FunctionType> struct VariadicFunctionInliner
{
	static FunctionData create(Compiler& c, const TemplateParameter::List& tp)
	{
		FunctionData d;

		d.id = getId();
		d.inliner = new Inliner(getId(), asmInline, highLevelInline);
		FunctionType::fillSignature(c, d, tp);

		return d;
	}

	static NamespacedIdentifier getId() { return NamespacedIdentifier(FunctionType::getId()); }

	static juce::Result asmInline(InlineData* d_)
	{
		auto id = FunctionType::getId();

		auto d = d_->toAsmInlineData();
		auto& cc = d->gen.cc;

		if (auto vt = VariadicTypeBase::getVariadicObjectFromInlineData(d))
		{
			auto childPtr = d->gen.registerPool->getNextFreeRegister(d->object->getScope(), TypeInfo(vt));

			if (d->object->isMemoryLocation())
				childPtr->setCustomMemoryLocation(d->object->getAsMemoryLocation(), false);
			else
			{
				auto mem = x86::ptr(PTR_REG_R(d->object));
				childPtr->setCustomMemoryLocation(mem, false);
			}

			for (int i = 0; i < vt->getNumSubTypes(); i++)
			{
				Array<FunctionData> matches;
				auto childType = vt->getSubType(i);
				childPtr->changeComplexType(childType);

				if (FunctionClass::Ptr sfc = childType->getFunctionClass())
				{
					sfc->addMatchingFunctions(matches, sfc->getClassName().getChildId(getId().id));

					if (matches.size() == 1)
					{
						auto f = matches.getFirst();

						if (!sfc->injectFunctionPointer(f))
						{
							if (!f.inliner)
								return Result::fail("Can't resolve function call");
						}

						auto r = d->gen.emitFunctionCall(d->target, f, childPtr, d->args);

						if (!r.wasOk())
							return r;

						auto childSize = childType->getRequiredByteSize();
						auto mem = childPtr->getMemoryLocationForReference().cloneAdjusted(childSize);
						childPtr->setCustomMemoryLocation(mem, false);
					}
				}

				if (matches.isEmpty())
				{
					juce::String s;
					s << childType->toString() << " does not have method " << getId().toString();
					return Result::fail(s);
				}
			}

			childPtr->flagForReuse();
		}

		return Result::ok();
	}

	static juce::Result highLevelInline(InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		if (auto vt = VariadicTypeBase::getVariadicObjectFromInlineData(b))
		{
			ScopedPointer<Operations::StatementBlock> sb = new Operations::StatementBlock(d->location, d->path);

			for (int i = 0; i < vt->getNumSubTypes(); i++)
			{
				TemplateParameter tp;
				tp.constant = i;

				auto clonedBase = d->object->clone(d->location);
				auto type = TypeInfo(vt->getSubType(i));
				auto offset = vt->getOffsetForSubType(i);

				FunctionClass::Ptr fc = type.getComplexType()->getFunctionClass();

				auto id = fc->getClassName().getChildId(getId().getIdentifier());
				TypeInfo t;


				Array<FunctionData> matches;
				fc->addMatchingFunctions(matches, id);

				if (matches.isEmpty())
				{
					juce::String s;
					s << type.toString() << " does not have a method " << getId().toString();
					return Result::fail(s);
				}
				if (matches.size() == 1)
				{
					t = matches.getFirst().returnType;
				}

				auto f = new Operations::FunctionCall(d->location, nullptr, Symbol(id, t), {});

				Operations::Expression::Ptr obj = new Operations::MemoryReference(d->location, dynamic_cast<Operations::Expression*>(clonedBase.get()), type, offset);

				f->setObjectExpression(obj);

				auto originalFunctionCall = dynamic_cast<Operations::FunctionCall*>(d->expression.get());

				jassert(originalFunctionCall != nullptr);

				for (int i = 0; i < originalFunctionCall->getNumArguments(); i++)
				{
					auto argClone = originalFunctionCall->getArgument(i)->clone(d->location);
					f->addArgument(dynamic_cast<Operations::Expression*>(argClone.get()));
				}

				sb->addStatement(f);
			}

			d->target = sb.release();
		}

		return Result::ok();
	}
};

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
		TemplateObject midi;
		auto mId = NamespacedIdentifier("wrap").getChildId("midi");
		midi.id = mId;
		midi.argList.add(TemplateParameter(mId.getChildId("T")));

		midi.makeClassType = [prototypes](const TemplateObject::ConstructData& d)
		{
			ComplexType::Ptr ptr;

			if (!d.expectTemplateParameterAmount(1))
				return ptr;

			if (!d.expectIsComplexType(0))
				return ptr;
			

			auto T = d.tp[0];
			T.argumentId = d.id.getChildId("T");

			auto st = new StructType(d.id, T);

			st->addMember("obj", d.tp[0].type, 0);
			st->finaliseExternalDefinition();

			for (auto p : prototypes)
			{
				st->addWrappedMemberMethod("obj", p);
			}
			
			ptr = st;

			return ptr;
		};

		c.addTemplateClass(midi);
	}


	{
		VariadicSubType::Ptr chainType = new VariadicSubType();
		chainType->variadicId = NamespacedIdentifier("container").getChildId("chain");

		{

			

			chainType->functions.add(VariadicFunctionInliner<ResetData>::create(c, {}));
			chainType->functions.add(VariadicFunctionInliner<ProcessSingleData>::create(c, {ct}));
			chainType->functions.add(VariadicFunctionInliner<PrepareData>::create(c, {ct}));
			chainType->functions.add(VariadicFunctionInliner<ProcessFunctionData>::create(c, {ct}));
		}

		{
			addVariadicGet(chainType);
		}


		c.registerVariadicType(chainType);
	}

	{
		auto frameType = new VariadicSubType();
		frameType->variadicId = NamespacedIdentifier("wrapp").getChildId("frame");

		frameType->functions.add(VariadicFunctionInliner<ResetData>::create(c, {}));
		frameType->functions.add(VariadicFunctionInliner<ProcessSingleData>::create(c, {ct}));
		frameType->functions.add(VariadicFunctionInliner<PrepareData>::create(c, {}));

		addVariadicGet(frameType);

		auto processFunction = FunctionData();
		processFunction.id = frameType->variadicId.getChildId("process");
		processFunction.returnType = TypeInfo(Types::ID::Void);
		processFunction.addArgs("data", TypeInfo(c.getComplexType(NamespacedIdentifier("ProcessData"), ct)));
		processFunction.inliner = Inliner::createHighLevelInliner(processFunction.id, [&c](InlineData* b)
		{
			auto d = b->toSyntaxTreeData();

			auto compiler = d->expression->currentCompiler;
			auto scope = d->expression->currentScope;
			auto root = Operations::findParentStatementOfType<Operations::ScopeStatementBase>(d->expression.get());


			auto scopeId = scope->getScopeSymbol();



			juce::String code;
			juce::String nl = "\n";

			auto channelCode = d->args[0]->toString(Operations::Statement::TextFormat::CppCode);
			auto objCode = d->object->toString(Operations::Statement::TextFormat::CppCode);

			code << "{" << nl;
			code << "    auto& fd = " << channelCode << ".toFrameData();" << nl;
			
#if 0
			code << "{" << nl;
			code << "    auto& fd = interleave(" << channelCode << ".data);" << nl;
			code << "    for(auto& f: fd)" << nl;
			code << "    {" << nl;
			code << "         " << objCode << ".get<0>().processSingle(f);" << nl;
			code << "    }" << nl;
			code << "    interleave(fd);" << nl;
			code << "}";
#endif


			CodeParser p(compiler, code.getCharPointer(), code.getCharPointer(), code.length());

			NamespaceHandler::ScopedNamespaceSetter sns(compiler->namespaceHandler, scopeId);
			BlockParser::ScopedScopeStatementSetter ssss(&p, root);

			try
			{
				d->target = p.parseStatement();

				d->target->forEachRecursive([d](Operations::Statement::Ptr s)
				{
					s->location = d->location;
					return false;
				});
			}
			catch (ParserHelpers::CodeLocation::Error& e)
			{
				jassertfalse;
			}

#if 0
			auto s = new Operations::StatementBlock(d->location, d->location.createAnonymousScopeId(scopeId));




			auto frameData = c.getComplexType(NamespacedIdentifier("FrameData"));
			auto blockData = c.getComplexType(NamespacedIdentifier("ChannelData"));

			auto fdSymbol = s->getPath().getChildId("fd");

			d->expression->currentCompiler->namespaceHandler.addSymbol(fdSymbol, TypeInfo(frameData), NamespaceHandler::Variable);

			auto def = new Operations::ComplexTypeDefinition(d->location, fdSymbol, TypeInfo(frameData));

			auto fId = Symbol(NamespacedIdentifier("interleave"), TypeInfo(blockData, false, true));
			auto f = new Operations::FunctionCall(d->location, nullptr, fId, {});

			def->addStatement(f);

			s->addStatement(def);

			d->target = s;
#endif


			return Result::ok();
		});

		frameType->functions.add(processFunction);

		c.registerVariadicType(frameType);
	}
}



void SnexObjectDatabase::addVariadicGet(VariadicSubType* variadicType)
{
	FunctionData getFunction;

	auto gId = variadicType->variadicId.getChildId("get");

	getFunction.id = gId;
	getFunction.returnType = TypeInfo(Types::ID::Dynamic, false, true);
	getFunction.templateParameters.add(TemplateParameter(gId.getChildId("Index")));

	getFunction.inliner = Inliner::createHighLevelInliner(gId, [](InlineData* b)
	{
		auto d = b->toSyntaxTreeData();

		if (auto vt = VariadicTypeBase::getVariadicObjectFromInlineData(b))
		{
			auto index = b->templateParameters[0].constant;

			auto type = TypeInfo(vt->getSubType(index));
			auto offset = (int)vt->getOffsetForSubType(index);

			auto base = dynamic_cast<Operations::Expression*>(d->object.get());

			d->target = new Operations::MemoryReference(d->location, base, type, offset);
			return Result::ok();
		}

		return Result::fail("Can't find variadic type");
	});

	getFunction.inliner->returnTypeFunction = [](InlineData* b)
	{
		auto d = dynamic_cast<ReturnTypeInlineData*>(b);

		if (d->templateParameters.size() != 1)
			return Result::fail("template amount mismatch. Expected 1");

		if (d->templateParameters[0].type.isValid())
			return Result::fail("template type mismatch. Expected integer value");

		auto index = d->templateParameters[0].constant;

		if (auto vt = VariadicTypeBase::getVariadicObjectFromInlineData(b))
		{
			if (auto st = vt->getSubType(index))
			{
				d->f.returnType = TypeInfo(st);
				return Result::ok();
			}
			else
			{
				return Result::fail("Can't find subtype with index " + juce::String(index));
			}
		}
		else
			return Result::fail("Can't call get on non-variadic templates");

		return Result::ok();
	};

	variadicType->functions.add(getFunction);
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
		auto o = 0;
		auto blockType = c.handler->getComplexType(NamespacedIdentifier("block"));
		auto channelType = new SpanType(TypeInfo(blockType), c.tp[0].constant);

		pType->addMember("data", TypeInfo(channelType), o);

		o += channelType->getRequiredByteSize();

		pType->addMember("events", TypeInfo(eventType), o);

		o += eventType.getRequiredByteSize();

		pType->addMember("voiceIndex", TypeInfo(Types::ID::Integer), o);

		o += 4;

		pType->addMember("shouldReset", TypeInfo(Types::ID::Integer), o);
		

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

#define F(channels) if (numChannels == channels) nextFrame.function = ProcessDataFix<channels>::nextFrame;

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
	case ProcessSingleFunction:
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