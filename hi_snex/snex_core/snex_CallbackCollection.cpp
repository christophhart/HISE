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
using namespace juce;




CallbackCollection::CallbackCollection()
{
	bestCallback[FrameProcessing] = CallbackTypes::Inactive;
	bestCallback[BlockProcessing] = CallbackTypes::Inactive;
}

juce::String CallbackCollection::getBestCallbackName(int processType) const
{
	auto cb = bestCallback[processType];

	if (cb == CallbackTypes::Channel) return "Channel";
	if (cb == CallbackTypes::Frame) return "Frame";
	if (cb == CallbackTypes::Sample) return "Sample";

	return "Inactive";
}

void CallbackCollection::setupCallbacks()
{
	StringArray cIds = { "processChannel", "processFrame", "processSample" };

	using namespace snex::Types;

	prepareFunction = obj["prepare"];

	if (!prepareFunction.matchesNativeArgumentTypes(ID::Void, { ID::Double, ID::Integer, ID::Integer }))
		prepareFunction = {};

	resetFunction = obj["reset"];

	if (!resetFunction.matchesNativeArgumentTypes(ID::Void, {}))
		resetFunction = {};

	eventFunction = obj["handleEvent"];

	callbacks[CallbackTypes::Sample] = obj[cIds[CallbackTypes::Sample]];

	if (!callbacks[CallbackTypes::Sample].matchesNativeArgumentTypes(ID::Float, { ID::Float }))
		callbacks[CallbackTypes::Sample] = {};

	callbacks[CallbackTypes::Frame] = obj[cIds[CallbackTypes::Frame]];

	if (!callbacks[CallbackTypes::Frame].matchesNativeArgumentTypes(ID::Void, { ID::Block }))
		callbacks[CallbackTypes::Frame] = {};

	callbacks[CallbackTypes::Channel] = obj[cIds[CallbackTypes::Channel]];

	if (!callbacks[CallbackTypes::Channel].matchesNativeArgumentTypes(ID::Void, { ID::Block, ID::Integer }))
		callbacks[CallbackTypes::Channel] = {};

	bestCallback[FrameProcessing] = getBestCallback(FrameProcessing);
	bestCallback[BlockProcessing] = getBestCallback(BlockProcessing);

	parameters.clear();

	auto parameterNames = ParameterHelpers::getParameterNames(obj);

	for (auto n : parameterNames)
	{
		auto pFunction = ParameterHelpers::getFunction(n, obj);

		if (!pFunction.matchesNativeArgumentTypes(ID::Void, { ID::Double }))
			pFunction = {};

		parameters.add({ n, pFunction });
	}

	if (listener != nullptr)
		listener->initialised(*this);
}

int CallbackCollection::getBestCallback(int processType) const
{
	if (processType == FrameProcessing)
	{
		if (callbacks[CallbackTypes::Frame])
			return CallbackTypes::Frame;
		if (callbacks[CallbackTypes::Sample])
			return CallbackTypes::Sample;
		if (callbacks[CallbackTypes::Channel])
			return CallbackTypes::Channel;

		return CallbackTypes::Inactive;
	}
	else
	{
		if (callbacks[CallbackTypes::Channel])
			return CallbackTypes::Channel;
		if (callbacks[CallbackTypes::Frame])
			return CallbackTypes::Frame;
		if (callbacks[CallbackTypes::Sample])
			return CallbackTypes::Sample;

		return CallbackTypes::Inactive;
	}
}

void CallbackCollection::prepare(double sampleRate, int blockSize, int numChannels)
{
	if (sampleRate == -1 || blockSize == 0 || numChannels == 0)
		return;

	if (prepareFunction)
		prepareFunction.callVoid(sampleRate, blockSize, numChannels);

	if (resetFunction)
		resetFunction.callVoid();

	if (listener != nullptr)
		listener->prepare(sampleRate, blockSize, numChannels);
}

void CallbackCollection::setListener(Listener* l)
{
	listener = l;
}

snex::jit::FunctionData ParameterHelpers::getFunction(const juce::String& parameterName, JitObject& obj)
{
	Identifier id("set" + parameterName);

	auto f = obj[id];

	if (f.matchesNativeArgumentTypes(Types::ID::Void, { Types::ID::Double }))
		return f;

	return {};
}

juce::StringArray ParameterHelpers::getParameterNames(JitObject& obj)
{
	auto ids = obj.getFunctionIds();
	StringArray sa;

	for (int i = 0; i < ids.size(); i++)
	{
		auto fName = ids[i].toString();

		if (fName.startsWith("set"))
			sa.add(fName.fromFirstOccurrenceOf("set", false, false));
	}

	return sa;
}

JitExpression::JitExpression(const juce::String& s, DebugHandler* handler) :
	memory(0)
{
	juce::String code = "double get(double input){ return " + s + ";}";



	snex::jit::Compiler c(memory);
	obj = c.compileJitObject(code);

	if (c.getCompileResult().wasOk())
	{
		f = obj["get"];

		// Add this after the compilation, we don't want to spam the logger
		// with compilation messages
		if (handler != nullptr)
			memory.addDebugHandler(handler);
	}
	else
		errorMessage = c.getCompileResult().getErrorMessage();
}

double JitExpression::getValue(double input) const
{
	if (f)
		return getValueUnchecked(input);
	else
		return input;
}

double JitExpression::getValueUnchecked(double input) const
{
	return f.callUncheckedWithCopy<double>(input);
}

juce::String JitExpression::getErrorMessage() const
{
	return errorMessage;
}

bool JitExpression::isValid() const
{
	return (bool)f;
}

juce::String JitExpression::convertToValidCpp(juce::String input)
{
	return input.replace("Math.", "hmath::");
}



template <typename T>
ComplexType::Ptr snex::_ramp<T>::createComplexType(const Identifier& id)
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

snex::jit::ComplexType::Ptr EventWrapper::createComplexType(const Identifier& id)
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
		if (IS_MEM(v)) cc.mov(n, INT_IMM((int)v));
		else cc.mov(n, INT_REG_R(v));

		return Result::ok();
	});

	ADD_INLINER(setNoteNumber,
	{
		SETUP_INLINER(int);
		auto n = base.cloneAdjustedAndResized(0x02, 1);

		auto v = d->args[0];
		if (IS_MEM(v)) cc.mov(n, INT_IMM((int)v));
		else cc.mov(n, INT_REG_R(v));

		return Result::ok();
	});

	ADD_INLINER(setVelocity,
	{
		SETUP_INLINER(int);
		auto n = base.cloneAdjustedAndResized(0x03, 1);

		auto v = d->args[0];
		if (IS_MEM(v)) cc.mov(n, INT_IMM((int)v));
		else cc.mov(n, INT_REG_R(v));

		return Result::ok();
	});

	return obj;
}

struct ResetData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("reset"); }

	static void fillSignature(Compiler& , FunctionData& d)
	{
		d.returnType = TypeInfo(Types::ID::Void);
	}
};

struct ProcessSingleData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("processSingle"); }

	static void fillSignature(Compiler& c, FunctionData& d)
	{
		auto st = c.getComplexType(NamespacedIdentifier("span<float, 2"));

		if (st == nullptr)
		{
			st = new SpanType(TypeInfo(Types::ID::Float), 2);
			c.registerExternalComplexType(st);
		}

		d.returnType = TypeInfo(Types::ID::Void);
		d.addArgs("frameData", TypeInfo(st, false));
	}
};

struct PrepareSpecs
{
	static ComplexType::Ptr createComplexType(const Identifier& id)
	{
		PrepareSpecs obj;

		auto st = new StructType(NamespacedIdentifier(id));
		ADD_SNEX_STRUCT_MEMBER(st, obj, sampleRate);
		ADD_SNEX_STRUCT_MEMBER(st, obj, blockSize);
		ADD_SNEX_STRUCT_MEMBER(st, obj, numChannels);

		return st;
	}

	double sampleRate = 0.0;
	int blockSize = 0;
	int numChannels = 0;
};

struct PrepareData
{
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("prepare"); }

	static void fillSignature(Compiler& c, FunctionData& d)
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
	static FunctionData create(Compiler& c)
	{
		FunctionData d;

		d.id = getId();
		d.inliner = new Inliner(getId(), asmInline, highLevelInline);
		FunctionType::fillSignature(c, d);

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

#define REGISTER_CPP_CLASS(compiler, className) c.registerExternalComplexType(className::createComplexType(#className));

void SnexObjectDatabase::registerObjects(Compiler& c)
{
	REGISTER_CPP_CLASS(c, sfloat);
	REGISTER_CPP_CLASS(c, sdouble);
	REGISTER_CPP_CLASS(c, PrepareSpecs);

	c.registerExternalComplexType(EventWrapper::createComplexType("HiseEvent"));

	VariadicSubType::Ptr chainType = new VariadicSubType();
	chainType->variadicId = NamespacedIdentifier("container").getChildId("chain");

	{
		chainType->functions.add(VariadicFunctionInliner<ResetData>::create(c));
		chainType->functions.add(VariadicFunctionInliner<ProcessSingleData>::create(c));
		chainType->functions.add(VariadicFunctionInliner<PrepareData>::create(c));
	}

	{
		FunctionData getFunction;
		getFunction.id = chainType->variadicId.getChildId("get");
		getFunction.returnType = TypeInfo(Types::ID::Dynamic, false, true);

		getFunction.inliner = Inliner::createHighLevelInliner(getFunction.id, [](InlineData* b)
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

		chainType->functions.add(getFunction);
	}


	c.registerVariadicType(chainType);


}



}
