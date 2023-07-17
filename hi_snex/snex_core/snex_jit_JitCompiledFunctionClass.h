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




class AsmJitFunctionCollection: public FunctionCollectionBase
{
public:

	AsmJitFunctionCollection(BaseScope* parentScope, const NamespacedIdentifier& classInstanceId);

	~AsmJitFunctionCollection();

	FunctionData getFunction(const NamespacedIdentifier& functionId) override;

	VariableStorage getVariable(const Identifier& id) const override;

	Array<Symbol> getAllVariables() const override;
	ComplexType::Ptr getMainObjectType() const;
	void* getVariablePtr(const Identifier& id) const override;

	size_t getMainObjectSize() const override;

	juce::String dumpTable() override;

	Array<NamespacedIdentifier> getFunctionIds() const override;

	


private:

	ClassScope* releaseClassScope()
	{
		auto c = pimpl;
		pimpl = nullptr;
		return c;
	}

	ClassScope* getClassScope()
	{
		return pimpl;
	}

	ReferenceCountedArray<DebugInformationBase> debugInformation;
	friend class ClassCompiler;
	friend class JitObject;

	ClassScope* pimpl;
};



class JitObject
{
public:

	JitObject() : functionClass(nullptr) {};

	JitObject(FunctionCollectionBase* f) :
		functionClass(f)
	{};


	template <typename T> T* getVariablePtr(const Identifier& id)
	{
		return reinterpret_cast<T*>(functionClass->getVariablePtr(id));
	}

	bool isStateless();

	FunctionData operator[](const Identifier& functionId) const;

	FunctionData operator[](const NamespacedIdentifier& functionId) const;

	Array<NamespacedIdentifier> getFunctionIds() const;

	explicit operator bool() const;;

	void* getMainObjectPtr()
	{
		return functionClass != nullptr ? functionClass->getMainObjectPtr() : nullptr;
	}

	

	juce::String dumpTable()
	{
		if(functionClass != nullptr)
			return functionClass->dumpTable();

		return {};
	}

	bool initMainObject(ObjectStorage<16, 16>& obj);

	int getNumVariables() const { return functionClass != nullptr ? functionClass->getAllVariables().size() : 0; }

	ValueTree getDataLayout(int dataIndex)
	{
		if (functionClass != nullptr)
			return functionClass->getDataLayout(dataIndex);

		return {};
	}

private:

	FunctionCollectionBase::Ptr functionClass;
};


/** This class can be used as base class to create C++ classes that call
    member functions defined in the JIT compiled code. 
	
	In order to use it, pass the code and the class identifier (either class name
	or alias) into Compiler::compileJitClass<T>(). Then subclass this class and
	add member functions that call the defined member functions of this class.
	*/
struct JitCompiledClassBase
{
	JitCompiledClassBase(JitObject&& o_, ComplexType::Ptr p) :
		o(o_),
		classType(p),
		memberFunctions(classType->getFunctionClass())
	{
		data.allocate(classType->getRequiredAlignment() + classType->getRequiredByteSize(), true);

		thisPtr = reinterpret_cast<void*>(data.get() + classType->getRequiredAlignment());

		ComplexType::InitData d;
		d.dataPointer = thisPtr;
		d.initValues = classType->makeDefaultInitialiserList();

		classType->initialise(d);
	}

protected:

	FunctionData getFunction(const Identifier& id);

	void* thisPtr = nullptr;

	JitObject o;
	ComplexType::Ptr classType;
	FunctionClass::Ptr memberFunctions;
	HeapBlock<uint8> data;
};




}
}
