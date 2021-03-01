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

	JitCompiledFunctionClass(BaseScope* parentScope, const NamespacedIdentifier& classInstanceId);

	~JitCompiledFunctionClass();

	FunctionData getFunction(const NamespacedIdentifier& functionId);

	VariableStorage getVariable(const Identifier& id);

	ComplexType::Ptr getMainObjectType();
	void* getMainObjectPtr();

	void* getVariablePtr(const Identifier& id);

	juce::String dumpTable();

	Array<NamespacedIdentifier> getFunctionIds() const;

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


private:

	static NamespacedIdentifier getMainId();

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
		return reinterpret_cast<T*>(functionClass->getVariablePtr(id));
	}

	bool isStateless();

	FunctionData operator[](const Identifier& functionId) const;

	FunctionData operator[](const NamespacedIdentifier& functionId) const;

	Array<NamespacedIdentifier> getFunctionIds() const;

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

	ComplexType::Ptr getMainObjectType()
	{
		return functionClass->getMainObjectType();
	}

	void* getMainObjectPtr()
	{
		return functionClass->getMainObjectPtr();
	}

	ClassScope* getClassScope() { return functionClass->getClassScope(); };

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





struct SnexStructSriptingWrapper : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<SnexStructSriptingWrapper>;
	using List = ReferenceCountedArray<SnexStructSriptingWrapper>;
	using WeakPtr = WeakReference<SnexStructSriptingWrapper>;
	using WeakList = Array<WeakPtr>;

	SnexStructSriptingWrapper(GlobalScope& s, const String& code, const Identifier& classId_);

	var create(const var::NativeFunctionArgs& args);

	Identifier classId;
	GlobalScope& scope;
	JitObject obj;
	ComplexType::Ptr ptr;
	Result r;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexStructSriptingWrapper);
};

class SnexComplexVarObject : public DynamicObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<SnexComplexVarObject>;

	static var make(SnexStructSriptingWrapper::Ptr p, const var::NativeFunctionArgs& args)
	{
		return new SnexComplexVarObject(p, args);
	}

	void setProperty(const Identifier& propertyName, const var& newValue) override
	{
		for (auto& p : dynamicProps)
		{
			if (p.id == propertyName)
			{
				VariableStorage v(p.type, newValue);
				ComplexType::writeNativeMemberType(p.dataMember, 0, v);
			}
		}
	}

	const var& getProperty(const Identifier& propertyName) const override
	{
		for (const auto& p : dynamicProps)
		{
			if (p.id == propertyName)
				return p.toVar();
		}

		static var empty = {};
		return empty;
	}

	bool hasMethod(const Identifier& id) const override
	{
		for (auto& f : functions)
			if (f == id)
				return true;

		return false;
	}

	var invokeMethod(Identifier id, const var::NativeFunctionArgs& a) override
	{
		for (auto& f : functions)
		{
			if (f == id)
				return f.call(a);
		}

		return {};
	}

	var toVar()
	{
		return var(this);
	}

	void* getPointer() const
	{
		return dataPtr;
	}

	static VariableStorage toArgs(const var& s, const TypeInfo& requestedType)
	{

	}

private:

	struct DynamicProperty
	{
		Types::ID type;
		Identifier id;
		void* dataMember = nullptr;
		mutable var externalVar;

		const var& toVar() const
		{
			switch (type)
			{
			case Types::ID::Integer:
			{
				auto v = *reinterpret_cast<int*>(dataMember);
				externalVar = { v };
				break;
			}
			case Types::ID::Float:
			{
				auto v = *reinterpret_cast<float*>(dataMember);
				externalVar = { v };
				break;
			}
			case Types::ID::Double:
			{
				auto v = *reinterpret_cast<double*>(dataMember);
				externalVar = { v };
				break;
			}
			default:
				break;
			}

			return externalVar;
		}
	};

	struct DynamicFunction
	{
		DynamicFunction(const FunctionData& f_);;

		bool operator==(const Identifier& id_) const
		{
			return id_ == id;
		}

		int getNumArgs() const
		{
			return f.args.size();
		}

		var call(const var::NativeFunctionArgs& vArgs);;

		struct ComplexArgument
		{
			static constexpr int MaxChannelAmount = 16;

			enum SpecialType
			{
				Native,
				BlockType,
				ProcessData,
				FrameData,
				Unknown,
				numSpecialTypes
			};

			bool isSpecial() const { return specialType != Native; }

			int numElements = 0;
			Types::ID nativeType;
			SpecialType specialType = Native;
		};

		static ComplexArgument::SpecialType getSpecialType(const TypeInfo& t);

		static int getNumElements(const TypeInfo& t);

		VariableStorage args[4];
		ComplexArgument cArgs[4];

		FunctionData f;
		Identifier id;
		Types::ID returnType;
	};

	Array<DynamicFunction> functions;
	Array<DynamicProperty> dynamicProps;

	SnexComplexVarObject(SnexStructSriptingWrapper::Ptr p, const var::NativeFunctionArgs& constructorArgs);

	snex::jit::SnexStructSriptingWrapper::WeakPtr parent;
	ComplexType::Ptr ptr;
	HeapBlock<uint8> ownedData_;
	void* dataPtr;
};





}
}
