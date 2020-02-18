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


class ClassCompiler;

class ClassScope;

class JitCompiledFunctionClass: public ReferenceCountedObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<JitCompiledFunctionClass>;

	JitCompiledFunctionClass(BaseScope* parentScope, const Symbol& classInstanceId);

	~JitCompiledFunctionClass();

	FunctionData getFunction(const Identifier& functionId);

	VariableStorage getVariable(const Identifier& id);

	void* getVariablePtr(const Identifier& id);

	juce::String dumpTable();

	Array<Identifier> getFunctionIds() const;

	ClassScope* releaseClassScope()
	{
		auto c = pimpl;
		pimpl = nullptr;
		return c;
	}

private:

	OwnedArray<DebugInformationBase> debugInformation;
	friend class ClassCompiler;
	friend class JitObject;

	ClassScope* pimpl;
};

class JitObject: public ApiProviderBase
{
public:

	

	JitObject() : functionClass(nullptr) {};

	JitObject(JitCompiledFunctionClass* f) :
		functionClass(f)
	{};


	template <typename T> T* getVariablePtr(const Identifier& id)
	{
		return reinterpret_cast<T*>(functionClass->getVariablePtrInternal(id));
	}

	FunctionData operator[](const Identifier& functionId) const;

	Array<Identifier> getFunctionIds() const;

	explicit operator bool() const;;
	
	void rebuildDebugInformation();

	int getNumDebugObjects() const override
	{
		if(functionClass != nullptr)
			return functionClass->debugInformation.size();
		
		return 0;
	}

	DebugableObjectBase* getDebugObject(const juce::String& token) override;

	DebugInformationBase* getDebugInformation(int index)
	{
		return functionClass->debugInformation[index];
	}

	ValueTree createValueTree();

	void getColourAndLetterForType(int type, Colour& colour, char& letter) override;

	juce::String dumpTable()
	{
		if(functionClass != nullptr)
			return functionClass->dumpTable();

		return {};
	}

private:

	
	
	
	JitCompiledFunctionClass::Ptr functionClass;
};


}
}