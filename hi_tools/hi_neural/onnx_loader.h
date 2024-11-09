/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#pragma once

namespace hise {
using namespace juce;

class ONNXLoader: public ReferenceCountedObject
{
	typedef void* (*loadModel_f)(const void*, size_t);
	typedef void (*destroyModel_f)(void*);
	typedef bool (*getError_f)(void*, char*, size_t&);
	typedef bool (*run_f)(void*, const void*, size_t, int, bool);
	typedef void (*getOutput_f)(void*, float*);

public:

	using Ptr = ReferenceCountedObjectPtr<ONNXLoader>;

	ONNXLoader(const String& rootDir);

	~ONNXLoader();

	bool run(const Image& img, std::vector<float>& outputValues, bool isGreyscale);

	Result loadModel(const MemoryBlock& mb);

	Result getLastError() const;

private:

	void unloadModel();

	void setLastError();

	template <typename Func> Func getFunction(const String& name)
	{
        
		if(auto f = reinterpret_cast<Func>(data->dll->getFunction(name)))
			return f;
		else
		{
			ok = Result::fail("Can't find function " + name);
			return nullptr;
		}
	}

	struct SharedData
	{
		SharedData();

		~SharedData();

		Result initialise(const String& rootDir);

		Result ok;
		ScopedPointer<DynamicLibrary> dll;
	};

	SharedResourcePointer<SharedData> data;
	
	Result ok;
	void* currentModel = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ONNXLoader);
};

} // namespace hise
