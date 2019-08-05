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


#define HNODE_JIT_ADD_C_FUNCTION_0(rt, ptr, name) addFunction(new FunctionData(FunctionData:: template createWithoutParameters<rt>(name, reinterpret_cast<void*>(ptr))))

#define HNODE_JIT_ADD_C_FUNCTION_1(rt, ptr, argType1, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1>(name, static_cast<rt(*)(argType1)>(ptr))))

#define HNODE_JIT_ADD_C_FUNCTION_2(rt, ptr, argType1, argType2, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1, argType2>(name, static_cast<rt(*)(argType1, argType2)>(ptr))));
#define HNODE_JIT_ADD_C_FUNCTION_3(rt, ptr, argType1, argType2, argType3, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1, argType2, argType3>(name, static_cast<rt(*)(argType1, argType2, argType3)>(ptr))));


class ConsoleFunctions : public FunctionClass
{
	static void print(int value) 
	{ 
		DBG(value); 
		if (currentDebugHandler != nullptr)
			currentDebugHandler->logMessage(String(value) + "\n");
	}
	static void print(double value)
	{ 
		DBG(value);
		if (currentDebugHandler != nullptr)
			currentDebugHandler->logMessage(String(value) + "\n");
	}
	static void print(float value) 
	{
		DBG(value);
		if (currentDebugHandler != nullptr)
			currentDebugHandler->logMessage(String(value) + "\n");
	}
	static void print(HiseEvent e)
	{
		String s;

		s << e.getTypeAsString();
		s << " channel: " << e.getChannel();
		s << " value: " << e.getNoteNumber();
		s << " value2: " << e.getVelocity();
		s << " timestamp: " << (int)e.getTimeStamp();
		s << "\n";

		DBG(s);
		if (currentDebugHandler != nullptr)
			currentDebugHandler->logMessage(s);
	}


	static Compiler::DebugHandler* currentDebugHandler;


public:

	static void setDebugHandler(Compiler::DebugHandler* handler)
	{
		currentDebugHandler = handler;
	}

	ConsoleFunctions() :
		FunctionClass("Console")
	{
		HNODE_JIT_ADD_C_FUNCTION_1(void, print, int, "print");
		HNODE_JIT_ADD_C_FUNCTION_1(void, print, float, "print");
		HNODE_JIT_ADD_C_FUNCTION_1(void, print, double, "print");
		HNODE_JIT_ADD_C_FUNCTION_1(void, print, HiseEvent, "print");
	}
};

Compiler::DebugHandler* ConsoleFunctions::currentDebugHandler = nullptr;

class BlockFunctions : public FunctionClass
{
public:

	struct Wrapper
	{
		static float getSample(block b, int index)
		{
			if(isPositiveAndBelow(index, b.size()))
               return b[index];
               
            return 0.0f;
		}

		static void setSample(block b, int index, float newValue)
		{
            if(isPositiveAndBelow(index, b.size()))
                b[index] = newValue;
		}

		static AddressType getWritePointer(block b)
		{
			return reinterpret_cast<AddressType>(b.getData());
		}

		static int size(block b)
		{
			return b.size();
		}

	};

public:

	BlockFunctions() :
		FunctionClass("Block")
	{
		HNODE_JIT_ADD_C_FUNCTION_2(float, Wrapper::getSample, block, int, "getSample");
		HNODE_JIT_ADD_C_FUNCTION_3(void, Wrapper::setSample, block, int, float, "setSample");
		HNODE_JIT_ADD_C_FUNCTION_1(AddressType, Wrapper::getWritePointer, block, "getWritePointer");
		HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::size, block, "size");
	}
};

class MessageFunctions : public FunctionClass
{
public:

	struct Wrapper
	{
#define WRAP_MESSAGE_GET_FUNCTION(x) static int x(HiseEvent e) { return (int)e.x(); };
#define WRAP_MESSAGE_SET_FUNCTION(x) static void x(HiseEvent e, int v) {e.x(v); };

		WRAP_MESSAGE_GET_FUNCTION(getNoteNumber);
		WRAP_MESSAGE_GET_FUNCTION(getVelocity);
		WRAP_MESSAGE_GET_FUNCTION(getChannel);
		WRAP_MESSAGE_GET_FUNCTION(isNoteOn);
		WRAP_MESSAGE_GET_FUNCTION(isNoteOnOrOff);
		
		static void setNoteNumber(HiseEvent& e, int noteNumber)
		{
			e.setNoteNumber(noteNumber);
		}


#undef WRAP_MESSAGE_FUNCTION
	};

public:

	MessageFunctions() :
		FunctionClass("Message")
	{
		HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getNoteNumber, HiseEvent, "getNoteNumber");
		
		auto nf = new FunctionData();
		nf->returnType = Types::ID::Void;
		nf->args.add(Types::ID::Event);
		nf->args.add(Types::ID::Integer);
		nf->function = reinterpret_cast<void*>(Wrapper::setNoteNumber);
		nf->id = "setNoteNumber";

		addFunction(nf);
	};
};

class MathFunctions : public FunctionClass
{
public:

	MathFunctions() :
		FunctionClass("Math")
	{
		
		//exposedFunctions.add(new FunkyFunctionData(FunkyFunctionData::create<float, float>("sin", static_cast<float(*)(float)>(hmath::sin))));
		//exposedFunctions.add(new FunkyFunctionData(FunkyFunctionData::create<double, double>("sin", static_cast<double(*)(double)>(hmath::sin))));

		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sin, double, "sin");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::asin, double, "asin");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::cos, double, "cos");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::acos, double, "acos");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sinh, double, "sinh");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::cosh, double, "cosh");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::tan, double, "tan");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::tanh, double, "tanh");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::atan, double, "atan");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::atanh, double, "atanh");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::log, double, "log");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::log10, double, "log10");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::exp, double, "exp");
		HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::pow, double, double, "pow");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sqr, double, "sqr");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sqrt, double, "sqrt");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::ceil, double, "ceil");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::floor, double, "floor");

		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sign, double, "sign");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::abs, double, "abs");
		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::round, double, "round");
		HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::range, double, double, double, "range");
		HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::min, double, double, "min");
		HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::max, double, double, "max");
		HNODE_JIT_ADD_C_FUNCTION_0(double, hmath::randomDouble, "randomDouble");


		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sin, float, "sin");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::asin, float, "asin");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::cos, float, "cos");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::acos, float, "acos");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sinh, float, "sinh");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::cosh, float, "cosh");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::tan, float, "tan");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::tanh, float, "tanh");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::atan, float, "atan");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::atanh, float, "atanh");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::log, float, "log");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::log10, float, "log10");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::exp, float, "exp");
		HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::pow, float, float, "pow");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sqr, float, "sqr");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sqrt, float, "sqrt");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::ceil, float, "ceil");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::floor, float, "floor");

		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sign, float, "sign");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::abs, float, "abs");
		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::round, float, "round");
		HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::range, float, float, float, "range");
		HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::min, float, float, "min");
		HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::max, float, float, "max");
		HNODE_JIT_ADD_C_FUNCTION_0(float, hmath::random, "random");


#if 0
		HNODE_JIT_ADD_C_FUNCTION_1(float, cosf, float, "cos");
		HNODE_JIT_ADD_C_FUNCTION_1(double, cos, double);
		HNODE_JIT_ADD_C_FUNCTION_1(float, tanf, float);
		HNODE_JIT_ADD_C_FUNCTION_1(double, tan, double);
		HNODE_JIT_ADD_C_FUNCTION_1(float, atanf, float);
		HNODE_JIT_ADD_C_FUNCTION_1(double, atan, double);
		HNODE_JIT_ADD_C_FUNCTION_1(float, atanhf, float);
		HNODE_JIT_ADD_C_FUNCTION_1(double, atanh, double);
		HNODE_JIT_ADD_C_FUNCTION_2(float, powf, float, float);
		HNODE_JIT_ADD_C_FUNCTION_2(double, pow, double, double);
		HNODE_JIT_ADD_C_FUNCTION_1(double, sqrt, double);
		HNODE_JIT_ADD_C_FUNCTION_1(float, sqrtf, float);

		HNODE_JIT_ADD_C_FUNCTION_0(float, hmath::random, "random");
		HNODE_JIT_ADD_C_FUNCTION_0(double, hmath::randomDouble, "random");

		HNODE_JIT_ADD_C_FUNCTION_1(float, tanhf, float);
		HNODE_JIT_ADD_C_FUNCTION_1(double, tanh, double);

		HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::abs, double, "abs");
		HNODE_JIT_ADD_C_FUNCTION_1(int, hmath::abs, int, "abs");

		HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::abs, float, "abs");
		HNODE_JIT_ADD_C_FUNCTION_1(double, exp, double);
		HNODE_JIT_ADD_C_FUNCTION_1(float, expf, float);
#endif

	};
};

#if 0
class JITScope::Pimpl : public DynamicObject,
						public FunctionClass,
					    public BaseScope
{
	friend class JITScope;

public:

	Pimpl(GlobalScope* globalMemoryPool_ = nullptr):
		FunctionClass({}),
		BaseScope({}, globalMemoryPool_)
	{
		runtime = new asmjit::JitRuntime();

		exposedFunctionClasses.add(new MathFunctions());
	}

	~Pimpl()
	{
		exposedFunctionClasses.clear();

		runtime = nullptr;
	}

	bool hasFunction(const Identifier& classId, const Identifier& functionId) const override
	{
		// Check compiled functions

		if (FunctionClass::hasFunction(classId, functionId)) 
			return true;

		// Check object functions
		if (globalMemoryPool != nullptr)
		{
			if (globalMemoryPool->hasFunction(classId, functionId))
				return true;
		}

		// Check registered function classes
		for (auto c : exposedFunctionClasses)
		{
			if (c->hasFunction(classId, functionId))
				return true;
		}
		
		return false;
	}

	void addMatchingFunctions(Array<FunkyFunctionData>& matches, const Identifier& classId, const Identifier& functionId) const override
	{
		FunctionClass::addMatchingFunctions(matches, classId, functionId);

		if (globalMemoryPool != nullptr)
			globalMemoryPool->addMatchingFunctions(matches, classId, functionId);

		for (auto c : exposedFunctionClasses)
			c->addMatchingFunctions(matches, classId, functionId);
	}

#if INCLUDE_GLOBALS

#if 0
	GlobalBase* getGlobal(const Identifier& id, bool lookForExternals=true)
	{
		for (auto g: globals)
		{
			if (g->id == id) return g;
		}

		if (lookForExternals)
		{
			for (auto eg : externalGlobals)
			{
				if (eg->id == id) return eg;
			}
		}

		return nullptr;
	}

	void setGlobalVariable(int globalIndex, const var& value)
	{
		if (auto g = globals[globalIndex])
		{
			TypeInfo t = g->getType();

			if (value.isInt64() || value.isDouble() || value.isInt())
			{
				if (JITTypeHelpers::matchesType<float>(t)) GlobalBase::store<float>(g, (float)value);
				else if (JITTypeHelpers::matchesType<double>(t)) GlobalBase::store<double>(g, (double)value);
				else if (JITTypeHelpers::matchesType<int>(t)) GlobalBase::store<int>(g, (int)value);
				else if (JITTypeHelpers::matchesType<BooleanType>(t)) GlobalBase::store<BooleanType>(g, (int)value > 0 ? 1 : 0);
				else throw String(g->id.toString() + " - var type mismatch: " + value.toString());
			}
#if INCLUDE_BUFFERS
			else if (value.isBuffer() && JITTypeHelpers::matchesType<Buffer*>(t))
			{
				g->setBuffer(value.getBuffer());
			}
#endif
			else
			{
				throw String(g->id.toString() + " - var type mismatch: " + value.toString());
			}
		}
	}

	void addGlobalVariable(const Identifier& id, const VariableStorage& newValue)
	{
		auto g = GlobalBase::create<int>(globalMemoryPool.get(), id);

		GlobalBase::storeDynamic(g, newValue);

		globals.add(g);
	}

	void setGlobalVariable(const juce::Identifier& id, const juce::var& value)
	{
		setGlobalVariable(getIndexForGlobal(id), value);
	}

	int getIndexForGlobal(const juce::Identifier& id) const
	{
		for (int i = 0; i < globals.size(); i++)
		{
			if (globals[i]->id == id) return i;
		}

		return -1;
	}
#endif
#endif

	Result getCompiledFunction(FunkyFunctionData& functionToSearch)
	{
		for (auto f : functions)
		{
			if (f->id == functionToSearch.id)
			{
				if (functionToSearch.matchesArgumentTypes(*f))
				{
					functionToSearch.function = f->function;
					return Result::ok();
				}
				else
				{
					return Result::fail("Function signature mismatch");
				}
			}
		}

		return Result::fail("Function not found");
	}


	String dumpAssembly()
	{
		return assembly;
	}

#if INCLUDE_GLOBALS
	OwnedArray<GlobalBase> globals;


	OwnedArray<GlobalBase> externalGlobals; // from memory pool...

#endif

	String assembly;

	OwnedArray<FunctionClass> exposedFunctionClasses;

	ScopedPointer<asmjit::JitRuntime> runtime;

	typedef ReferenceCountedObjectPtr<JITScope> Ptr;

	WeakReference<GlobalScope> globalMemoryPool;
};
#endif


} // end namespace jit
} // end namespace snex
