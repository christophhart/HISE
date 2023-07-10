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
USE_ASMJIT_NAMESPACE;


#define HNODE_JIT_ADD_C_FUNCTION_0(rt, ptr, name) addFunction(new FunctionData(FunctionData:: template createWithoutParameters<rt>(name, reinterpret_cast<void*>(ptr))))

#define HNODE_JIT_ADD_C_FUNCTION_1(rt, ptr, argType1, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1>(name, static_cast<rt(*)(argType1)>(ptr))))

#define HNODE_JIT_ADD_C_FUNCTION_2(rt, ptr, argType1, argType2, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1, argType2>(name, static_cast<rt(*)(argType1, argType2)>(ptr))));
#define HNODE_JIT_ADD_C_FUNCTION_3(rt, ptr, argType1, argType2, argType3, name) addFunction(new FunctionData(FunctionData::template create<rt, argType1, argType2, argType3>(name, static_cast<rt(*)(argType1, argType2, argType3)>(ptr))));


class ConsoleFunctions : public JitCallableObject
{
	struct WrapperInt
	{
		JIT_MEMBER_WRAPPER_2(int, ConsoleFunctions, print, int, int);
	};

	struct WrapperDouble
	{
		JIT_MEMBER_WRAPPER_2(double, ConsoleFunctions, print, double, int);
	};

	struct WrapperFloat
	{
		JIT_MEMBER_WRAPPER_2(float, ConsoleFunctions, print, float, int);
	};

	struct WrapperEvent
	{
		JIT_MEMBER_WRAPPER_1(HiseEvent, ConsoleFunctions, print, HiseEvent);
	};

	struct WrapperStop
	{
		JIT_MEMBER_WRAPPER_1(void, ConsoleFunctions, stop, bool);
	};

	struct WrapperDump
	{
		JIT_MEMBER_WRAPPER_0(void, ConsoleFunctions, dump);
	};

	struct WrapperClear
	{
		JIT_MEMBER_WRAPPER_0(void, ConsoleFunctions, clear);
	};
	
	int print(int value, int lineNumber)
	{
		if (gs.get() != nullptr && gs->isDebugModeEnabled())
		{
			DBG(value);

			String s;
			s << "Line " << lineNumber << ": " << value;
			logAsyncIfNecessary(s);
		}

		
		return value;
	}
	double print(double value, int lineNumber)
	{
		if (gs.get() != nullptr && gs->isDebugModeEnabled())
		{
			DBG(value);
			String s;
			s << "Line " << lineNumber << ": " << value;
			logAsyncIfNecessary(s);
		}

		return value;
	}
	float print(float value, int lineNumber)
	{
		if (gs.get() != nullptr && gs->isDebugModeEnabled())
		{
			DBG(value);
			String s;
			s << "Line " << lineNumber << ": " << value;
			logAsyncIfNecessary(s);
		}
		return value;
	}

	void clear()
	{
		if (gs.get() != nullptr && gs->isDebugModeEnabled())
		{
			gs->clearDebugMessages();
		}
	}

	static void dumpObject(void* consoleObject, int dumpedObjectIndex, void* dataPtr)
	{
		auto c = static_cast<ConsoleFunctions*>(consoleObject);

		if (c->gs.get() != nullptr && c->gs->isDebugModeEnabled())
		{
			if (auto ptr = c->dumpItems[dumpedObjectIndex])
			{
				int l = 0;
				String s;

				s << "Line " << String(ptr->lineNumber) << ": ";

				if (dataPtr != nullptr)
					ptr->type->dumpTable(s, l, dataPtr, dataPtr);
				else
					s << "nullptr!";

				c->logAsyncIfNecessary(s);
			}
		}
	}

	void dump()
	{
		if (gs.get() != nullptr && gs->isDebugModeEnabled())
		{
			if (auto cs = gs->getCurrentClassScope())
			{
				auto s = cs->getRootData()->dumpTable();
				logAsyncIfNecessary(s);
			}
		}
	}

	void logAsyncIfNecessary(const juce::String& s)
	{
		jassert(gs != nullptr);
		jassert(gs->isDebugModeEnabled());

		if (gs != nullptr)
			gs->logMessage(s + "\n");

		
	}

	static void blink(void* obj, int lineNumber)
	{
		auto c = static_cast<ConsoleFunctions*>(obj);
		c->gs->sendBlinkMessage(lineNumber);
	}

	void stop(bool condition)
	{
		if (condition && gs != nullptr)
		{
			auto& handler = gs->getBreakpointHandler();

			if (handler.isActive())
			{
				handler.breakExecution();
				
				while (!handler.shouldResume())
				{
					if (handler.canWait())
						Thread::getCurrentThread()->wait(5000);
				}
			}
		}
	}

	HiseEvent print(HiseEvent e)
	{
		MessageManager::callAsync([this, e]()
		{
			juce::String s;

			s << e.getTypeAsString();
			s << " channel: " << e.getChannel();
			s << " value: " << e.getNoteNumber();
			s << " value2: " << e.getVelocity();
			s << " timestamp: " << (int)e.getTimeStamp();
			s << "\n";

			DBG(s);

			if (gs != nullptr)
			{
				gs->logMessage(s + "\n");
			}
		});

		return e;
	}

	void registerAllObjectFunctions(GlobalScope*) override;

public:

	struct DumpItem
	{
		ComplexType::Ptr type;
		NamespacedIdentifier id;
		int lineNumber;
	};

	OwnedArray<DumpItem> dumpItems;

	ConsoleFunctions(GlobalScope* scope_) :
		JitCallableObject(NamespacedIdentifier("Console")),
		gs(scope_)
	{
		registerAllObjectFunctions(gs);
	};

	WeakReference<BaseScope> classScope;
	WeakReference<GlobalScope> gs;
};

class MathFunctions : public FunctionClass
{
public:

#if SNEX_ASMJIT_BACKEND
	struct Intrinsics
	{
		static void range(x86::Compiler& cc, x86::Gp rv, x86::Gp v, x86::Gp l, x86::Gp u)
		{
			cc.lea(rv, x86::ptr(u).cloneAdjustedAndResized(0, 4));
			cc.cmp(v, u);
			cc.cmovl(rv, v);
			cc.cmp(v, l);
			cc.cmovl(rv, l);
		}
	};

	struct Inliners
	{
		static Result abs(InlineData* d);
		static Result max(InlineData* d);
		static Result min(InlineData* d);
		static Result range(InlineData* d);
		static Result sign(InlineData* b);
		static Result fmod(InlineData* b);
		static Result sin(InlineData* b);
		static Result wrap(InlineData* b);
		static Result map(InlineData* b);;
		static Result norm(InlineData* b);
		static Result sig2mod(InlineData* b);
		static Result mod2sig(InlineData* b);
	};
#endif

	MathFunctions(bool addInlinedFunctions, ComplexType::Ptr blockType);;
};

}
}
