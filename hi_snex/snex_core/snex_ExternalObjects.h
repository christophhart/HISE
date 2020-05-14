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
namespace Types {
using namespace juce;

template <typename T> struct _ramp
{
	using Type = _ramp<T>;

	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(void, Type, reset);
		JIT_MEMBER_WRAPPER_1(void, Type, set, T);
		JIT_MEMBER_WRAPPER_0(T, Type, advance);
		JIT_MEMBER_WRAPPER_0(T, Type, get);
		JIT_MEMBER_WRAPPER_2(void, Type, prepare, double, double);
	};

	void reset()
	{
		stepsToDo = 0;
		value = targetValue;
		delta = T(0);
	}

	void set(T newTargetValue)
	{
		if (numSteps == 0)
		{
			value = targetValue;
			stepsToDo = 0;
		}
		else
		{
			auto d = newTargetValue - value;
			delta = d * stepDivider;
			targetValue = newTargetValue;
			stepsToDo = numSteps;
		}
	}

	T advance()
	{
		if (stepsToDo <= 0)
			return value;

		auto v = value;
		value += delta;
		stepsToDo--;

		return v;
	}

	T get()
	{
		return value;
	}

	void prepare(double samplerate, double timeInMilliseconds)
	{
		auto msPerSample = 1000.0 / samplerate;
		numSteps = timeInMilliseconds * msPerSample;

		if (numSteps > 0)
			stepDivider = T(1) / (T)numSteps;
	}

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);

	T value = T(0);
	T targetValue = T(0);
	T delta = T(0);
	T stepDivider = T(0);

	int numSteps = 0;
	int stepsToDo = 0;
};

struct EventWrapper
{
	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getNoteNumber);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getVelocity);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getChannel);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setVelocity, int);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setChannel, int);
		JIT_MEMBER_WRAPPER_1(void, HiseEvent, setNoteNumber, int);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getTimeStamp);
	};

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};

struct ScriptnodeCallbacks
{
	enum ID
	{
		ResetFunction,
		ProcessFunction,
		ProcessFrameFunction,
		PrepareFunction,
		HandleEventFunction,
		numFunctions
	};

	static Array<FunctionData> getAllPrototypes(Compiler& c, int numChannels);

	static jit::FunctionData getPrototype(Compiler& c, ID id, int numChannels);
};

template struct _ramp<float>;
template struct _ramp<double>;

using sfloat = _ramp<float>;
using sdouble = _ramp<double>;

using namespace Types;




struct PrepareSpecs
{
	double sampleRate = 0.0;
	int blockSize = 0;
	int numChannels = 0;
	int* voiceIndex = nullptr;

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};



struct OscProcessData
{
	dyn<float> data;		// 12 bytes
	double uptime = 0.0;    // 8 bytes
	double delta = 0.0;     // 8 bytes
	int voiceIndex = 0;			// 4 bytes

	static snex::ComplexType* createType(Compiler& c);
};


struct SnexObjectDatabase
{
	static void registerObjects(Compiler& c, int numChannels);
	static void createProcessData(Compiler& c, const TypeInfo& eventType);
	static void createFrameProcessor(Compiler& c);
	static void registerParameterTemplate(Compiler& c);
};



struct JitCompiledNode
{
	JitCompiledNode(Compiler& c, const String& code, const Identifier& classId, int numChannels_) :
		r(Result::ok()),
		numChannels(numChannels_)
	{
		String s;

		s << "namespace impl { " << code;
		s << "}\n";
		s << "impl::" << classId << " instance;\n";

		Types::SnexObjectDatabase::registerObjects(c, numChannels);

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

			FunctionClass::Ptr fc = c.getComplexType(impl.getChildId(classId))->getFunctionClass();

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
	}

	void addCallbackWrapper(String& s, const FunctionData& d)
	{
		s << d.getSignature({}, false) << "{ ";

		if (d.returnType != TypeInfo(Types::ID::Void))
			s << "return ";

		s << "instance." << d.id.getIdentifier() << "(";

		for (int i = 0; i < d.args.size(); i++)
		{
			s << d.args[i].id.getIdentifier();
			if (isPositiveAndBelow(i, d.args.size() - 1))
				s << ", ";
		}

		s << "); }\n";
	}

	void prepare(PrepareSpecs ps)
	{
		jassert(ps.numChannels >= numChannels);
		PrepareSpecs copy(ps);
		callbacks[Types::ScriptnodeCallbacks::PrepareFunction].callVoid(&copy);
	}

	template <typename T> void process(T& data)
	{
		jassert(data.getNumChannels() >= numChannels);
		callbacks[Types::ScriptnodeCallbacks::ProcessFunction].callVoid(&data);
	}

	void reset()
	{
		callbacks[Types::ScriptnodeCallbacks::ResetFunction].callVoid();
	}

	template <typename T> void processFrame(T& data)
	{
		jassert(data.size() >= numChannels);
		callbacks[Types::ScriptnodeCallbacks::ProcessFrameFunction].callVoid(data.begin());
	}

	void* thisPtr = nullptr;

	Result r;

private:

	int numChannels = 0;
	bool ok = false;
	FunctionData callbacks[Types::ScriptnodeCallbacks::numFunctions];

	JitObject obj;
};

}
}