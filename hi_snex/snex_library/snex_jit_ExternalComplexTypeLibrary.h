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




template <typename T> struct RampWrapper
{
	using Type = pimpl::_ramp<T>;

	RampWrapper() {};

	struct Wrapper
	{
		JIT_MEMBER_WRAPPER_0(void, Type, reset);
		JIT_MEMBER_WRAPPER_1(void, Type, set, T);
		JIT_MEMBER_WRAPPER_0(T, Type, advance);
		JIT_MEMBER_WRAPPER_0(T, Type, get);
		JIT_MEMBER_WRAPPER_2(void, Type, prepare, double, double);
		JIT_MEMBER_WRAPPER_0(int, Type, isActive);
	};


	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};


template struct RampWrapper<float>;
template struct RampWrapper<double>;

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
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, isNoteOn);
		JIT_MEMBER_WRAPPER_0(double, HiseEvent, getFrequency);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, isEmpty);
		JIT_MEMBER_WRAPPER_0(void, HiseEvent, clear);
		JIT_MEMBER_WRAPPER_0(int, HiseEvent, getEventId);


#define JIT_MEMBER_WRAPPER_0(R, C, N)					  static R N(void* o) { return static_cast<C*>(o)->N(); };
	};

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};

struct PrepareSpecsJIT : public PrepareSpecs
{
	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};

struct DataReadLockJIT
{
	struct Wrappers
	{
		static void constructor(void* obj, void* externalData, int tryRead)
		{
			if (auto thisPtr = static_cast<DataReadLockJIT*>(obj))
			{
				auto ed = static_cast<ExternalData*>(externalData);
				auto cd = ed->obj;

				thisPtr->complexDataPtr = cd;

				if (cd != nullptr)
				{
					if(tryRead)
						thisPtr->holdsLock = (int)cd->getDataLock().enterTryReadLock();
					else
						thisPtr->holdsLock = (int)cd->getDataLock().enterReadLock();
				}
			}
		}

		static void destructor(void* obj)
		{
			if (auto thisPtr = static_cast<DataReadLockJIT*>(obj))
			{
				if (auto cd = static_cast<ComplexDataUIBase*>(thisPtr->complexDataPtr))
				{
					bool holdsLock = thisPtr->holdsLock > 0;
					cd->getDataLock().exitReadLock(holdsLock);
				}
			}
		}
        
        static int isLocked(void* obj)
        {
            if (auto thisPtr = static_cast<DataReadLockJIT*>(obj))
            {
                return thisPtr->holdsLock;
            }
            
            return false;
        }
	};

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);

	void* complexDataPtr = nullptr;
	int holdsLock = 0;
};

struct SampleDataJIT : public MonoSample
{
	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);

	template <int C> static void clear(void* obj)
	{
		auto typed = static_cast<SampleData<C>*>(obj);
		typed->clear();
	}

	template <int C> static void fromHiseEventStatic(void* obj, HiseEvent* e)
	{
		auto typed = static_cast<SampleData<C>*>(obj);
		typed->fromHiseEvent(*e);
	}
};

struct ExternalDataJIT : public ExternalData
{
	static void referTo(void* obj, block& b, int index)
	{
		static_cast<ExternalData*>(obj)->referBlockTo(b, index);
	}

	static void setDisplayValueStatic(void* obj, double value)
	{
		static_cast<ExternalData*>(obj)->setDisplayedValue(value);
	}

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};

struct ModValueJit : public ModValue
{
	struct Wrapper
	{
		static int getChangedValue(void* obj, double* d)
		{
			auto typed = static_cast<ModValue*>(obj);
			return (int)typed->getChangedValue(*d);
		}

		static double getModValue(void *obj)
		{ 
			auto typed = static_cast<ModValue*>(obj);
			return (double)typed->modValue; 
		}

		static void setModValue(void* obj, double newValue)
		{
			auto typed = static_cast<ModValue*>(obj);
			typed->setModValue(newValue);
		}
		
		static void setModValueIfChanged(void* obj, double newValue)
		{
			auto typed = static_cast<ModValue*>(obj);
			typed->setModValueIfChanged(newValue);
		}
	};

	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};


struct OscProcessDataJit : public OscProcessData
{
	static ComplexType::Ptr createComplexType(Compiler& c, const Identifier& id);
};

struct DataLibraryBuilder: public LibraryBuilderBase
{
	DataLibraryBuilder(Compiler& c, int numChannels) :
		LibraryBuilderBase(c, numChannels)
	{};

	Identifier getFactoryId() const override { RETURN_STATIC_IDENTIFIER("data"); }

	Result registerTypes() override;
	
};

struct InbuiltTypeLibraryBuilder : public LibraryBuilderBase
{
	InbuiltTypeLibraryBuilder(Compiler& c, int numChannels) :
		LibraryBuilderBase(c, numChannels)
	{

	}

	Identifier getFactoryId() const override { return Identifier(); }



	Result registerTypes() override;

private:

	void createExternalDataTemplates();

	void createProcessData(const TypeInfo& eventType);

	void createFrameProcessor();

	void registerBuiltInFunctions();

	void registerRangeFunctions();

	FunctionData* createRangeFunction(const Identifier& id, int numArgs, Inliner::InlineType type, const Inliner::Func& inliner);
	void addRangeFunction(FunctionClass* fc, const Identifier& id, const StringArray& parameters, const String& code);

};


}
}
