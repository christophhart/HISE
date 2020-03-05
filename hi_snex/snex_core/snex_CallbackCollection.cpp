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

	if (!eventFunction.matchesNativeArgumentTypes(ID::Void, { ID::Event }))
		eventFunction = {};

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
	
	auto obj = new StructType(Symbol::createRootSymbol(Identifier(id)));

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
	auto obj = new StructType(Symbol::createRootSymbol(Identifier(id)));

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


void SnexObjectDatabase::registerObjects(Compiler& c)
{
	c.registerExternalComplexType(sfloat::createComplexType("sfloat"));
	c.registerExternalComplexType(sdouble::createComplexType("sdouble"));
	c.registerExternalComplexType(EventWrapper::createComplexType("HiseEvent"));
}



}