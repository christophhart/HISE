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

struct GlobalBase
{
	GlobalBase(const Identifier& id_, VariableStorage& t):
		id(id_),
		ownData(0.0),
		data(&ownData)
	{

	};

	~GlobalBase()
	{

	}

	void* getDataPointer() { return data->getDataPointer(); }

	template<typename T> static T store(GlobalBase* b, T newValue)
	{
#if JUCE_WINDOWS
		jassert(JITTypeHelpers::template matchesType<T>(b->type));
#endif

		T* castedData = reinterpret_cast<T*>(b->getDataPointer());
		*castedData = newValue;

		return T();
	}

	static void storeDynamic(GlobalBase* b, const VariableStorage& newValue)
	{
		*b->data = newValue;

	}


	void setExternalMemory(VariableStorage& d)
	{
		ownData = {};
		data = &d;
	}
	
	template<typename T> static T get(GlobalBase* b)
	{
#if JUCE_WINDOWS
		jassert(JITTypeHelpers::matchesType<T>(b->type));
#endif

		return *reinterpret_cast<T*>(b->getDataPointer());
	}


	template <typename T> static GlobalBase* create(GlobalScope* memoryPool, const Identifier& id)
	{
		jassertfalse;

		if (memoryPool == nullptr)
		{
			jassertfalse;
			return nullptr;
		}
		else
		{
			return nullptr;
		}
	}

	Identifier id;

	bool isConst = false;

private:

	VariableStorage ownData;
	VariableStorage* data;
};

} // end namespace jit
} // end namespace snex
