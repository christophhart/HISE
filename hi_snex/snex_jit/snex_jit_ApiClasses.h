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


class ConsoleFunctions : public JitCallableObject
{
	struct WrapperInt
	{
		JIT_MEMBER_WRAPPER_1(int, ConsoleFunctions, print, int);
	};

	struct WrapperDouble
	{
		JIT_MEMBER_WRAPPER_1(double, ConsoleFunctions, print, double);
	};

	struct WrapperFloat
	{
		JIT_MEMBER_WRAPPER_1(float, ConsoleFunctions, print, float);
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
	
	int print(int value)
	{
		DBG(value);
		
		MessageManager::callAsync([this, value]()
		{
			if (gs != nullptr)
			{
				gs->logMessage(juce::String(value) + "\n");
			}
		});
			
		return value;
	}
	double print(double value)
	{
		DBG(value);
		
		MessageManager::callAsync([this, value]()
		{
			if (gs != nullptr)
			{
				gs->logMessage(juce::String(value) + "\n");
			}
		});

		return value;
	}
	float print(float value)
	{
		DBG(value);
		
		MessageManager::callAsync([this, value]()
		{
			if (gs != nullptr)
			{
				gs->logMessage(juce::String(value) + "\n");
			}
		});

		return value;
	}

	void dump()
	{
		if (gs != nullptr)
		{
			auto s = gs->getCurrentClassScope()->getRootData()->dumpTable();

			MessageManager::callAsync([this, s]()
			{
				if (gs != nullptr)
				{
					gs->logMessage(s + "\n");
				}
			});
		}
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
						Thread::getCurrentThread()->wait(100);
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

	ConsoleFunctions(GlobalScope* scope_) :
		JitCallableObject(NamespacedIdentifier("Console")),
		gs(scope_)
	{
		registerAllObjectFunctions(gs);
	};

	~ConsoleFunctions()
	{
		int x = 5;
	}

	WeakReference<BaseScope> classScope;
	WeakReference<GlobalScope> gs;
};

class MathFunctions : public FunctionClass
{
public:

	MathFunctions();;
};

}
}
