/*
  ==============================================================================

    Parser.h
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED


#include <type_traits>



class BaseFunction
{
public:

	BaseFunction(const Identifier& id, void* func_): functionName(id), func(func_) {}

	virtual ~BaseFunction() {};

	virtual int getNumParameters() const = 0;

	virtual TypeInfo getReturnType() const = 0;

	TypeInfo getTypeForParameter(int index)
	{
		if (index >= 0 && index < getNumParameters())
		{
			return parameterTypes[index];
		}
		else
		{
			return typeid(void);
		}
	}

	const Identifier functionName;
	void* func;

protected:

#pragma warning (push)
#pragma warning (disable: 4100)

	template <typename P, typename... Ps> void addTypeInfo(std::vector<TypeInfo>& info, P value, Ps... otherInfo)
	{
		info.push_back(typeid(value));
		addTypeInfo(info, otherInfo...);
	}

	template<typename P> void addTypeInfo(std::vector<TypeInfo>& info, P value) { info.push_back(typeid(value)); }

#pragma warning (pop)

	void addTypeInfo(std::vector<TypeInfo>& /*info*/) {}

	std::vector<TypeInfo> parameterTypes;

private:

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseFunction)
};



struct Buffer: public ReferenceCountedObject
{
	Buffer() : buffer(new AudioSampleBuffer()) {};

	static float getSample(Buffer* b, int index)
	{
		return b->buffer->getSample(0, index);
	};

	template <typename DummyReturn> static DummyReturn setSample(Buffer* b, int index, float value)
	{
		b->buffer->setSample(0, index, value);

		return DummyReturn();
	}

	template <typename DummyReturn> static DummyReturn setSize(Buffer* b, int newSize)
	{
		b->buffer->setSize(1, newSize);

		return DummyReturn();
	}

	ScopedPointer<AudioSampleBuffer> buffer;
};


struct GlobalBase
{
	GlobalBase(const Identifier& id_, TypeInfo type_) :
		id(id_),
		type(type_)
	{
		
	};

	~GlobalBase()
	{
		
	}

	template<typename T> static T store(GlobalBase* b, T newValue)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		*reinterpret_cast<T*>(&b->value) = newValue;

		return T();
	}

	template<typename T> static T get(GlobalBase* b)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		return *reinterpret_cast<T*>(&b->value);
	}

	TypeInfo getType() const noexcept { return type; }
	
	template <typename T> static GlobalBase* create(const Identifier& id)
	{
		return new GlobalBase(id, typeid(T));
	}

	TypeInfo type;
	Identifier id;
	
	double value = 0.0;
	

	ScopedPointer<Buffer> bufferHolder;
};



template <typename T> T storeInStack(T& target, T value)
{
	target = value;
	return value;
}

double getSampleRate()
{
	return 44100.0;
}



template <typename R, typename ... Parameters> class TypedFunction : public BaseFunction
{
public:

	TypedFunction(const Identifier& id, void* func, Parameters... types) :
		BaseFunction(id, func)
	{
		addTypeInfo(parameterTypes, types...);
	};

	int getNumParameters() const override { return sizeof...(Parameters); };

	virtual TypeInfo getReturnType() const override { return typeid(R); };


};

#define NATIVE_JIT_ADD_C_FUNCTION_0(return_type, name) exposedFunctions.add(new TypedFunction<return_type>(Identifier(#name), (void*)name)); 
#define NATIVE_JIT_ADD_C_FUNCTION_1(return_type, name, param1_type) exposedFunctions.add(new TypedFunction<return_type, param1_type>(Identifier(#name), (void*)name, param1_type())); 
#define NATIVE_JIT_ADD_C_FUNCTION_2(return_type, name, param1_type, param2_type) exposedFunctions.add(new TypedFunction<return_type, param1_type, param2_type>(Identifier(#name), (void*)name, param1_type(), param2_type())); 
#define ADD_C_FUNCTION_3(return_type, name, param1_type, param2_type, param3_type) exposedFunctions.add(new TypedFunction<return_type, param1_type, param2_type, param3_type>(Identifier(#name), (void*)name, param1_type(), param2_type(), param3_type())); 

#define ADD_GLOBAL(type, name) globals.add(GlobalBase::create<type>(name));



class NativeJITScope::Pimpl : public DynamicObject
{
	friend class NativeJITScope;

public:

	Pimpl() :
		codeAllocator(32768),
		allocator(32768)
	{
		addExposedFunctions();
	}

	void addExposedFunctions()
	{
		NATIVE_JIT_ADD_C_FUNCTION_1(float, sinf, float);
		NATIVE_JIT_ADD_C_FUNCTION_0(double, getSampleRate);
		NATIVE_JIT_ADD_C_FUNCTION_2(float, powf, float, float);
	}

	BaseFunction* getExposedFunction(const Identifier& id)
	{
		for (int i = 0; i < exposedFunctions.size(); i++)
		{
			if (exposedFunctions[i]->functionName == id)
			{
				return exposedFunctions[i];
			}
		}

		return nullptr;
	}

	GlobalBase* getGlobal(const Identifier& id)
	{
		for (int i = 0; i < globals.size(); i++)
		{
			if (globals[i]->id == id) return globals[i];
		}

		return nullptr;
	}

	BaseFunction* getCompiledBaseFunction(const Identifier& id)
	{
		for (int i = 0; i < compiledFunctions.size(); i++)
		{
			if (compiledFunctions[i]->functionName == id)
			{
				return compiledFunctions[i];
			}
		}

		return nullptr;
	}

	template <typename ExpectedType> void checkTypeMatch(BaseFunction* b, int parameterIndex)
	{
		if (parameterIndex == -1)
		{
			if (!NativeJITTypeHelpers::matchesType<ExpectedType>(b->getReturnType()))
			{
				throw String("Return type of function \"" + b->functionName + "\" does not match. ") + NativeJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType>(b->getReturnType());
			}
		}
		else
		{
			if (!NativeJITTypeHelpers::matchesType<ExpectedType>(b->getTypeForParameter(parameterIndex)))
			{
				throw String("Parameter " + String(parameterIndex + 1) + " type of function \"" + b->functionName + "\" does not match. ") + NativeJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType>(b->getReturnType());
			}
		}
	}

	template <typename ReturnType> ReturnType(*getCompiledFunction1(const Identifier& /*id*/))() { return nullptr; }
	template <typename ReturnType, typename ParamType1> ReturnType(*getCompiledFunction2(const Identifier& /*id*/))(ParamType1) { return nullptr; }
	template <typename ReturnType> ReturnType(*getCompiledFunction2(const Identifier& /*id*/))() { return nullptr; }

#pragma warning(push)
#pragma warning (disable: 4127)

	template <typename ReturnType, typename... Other> ReturnType(*getCompiledFunction0(const Identifier& id))(Other...)
	{
		if (sizeof...(Other) != 0) return nullptr;

		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);

			return (ReturnType(*)(Other...))b->func;
		}

		return nullptr;
	}

	template <typename ReturnType, typename ParamType1, typename... Other> ReturnType(*getCompiledFunction1(const Identifier& id))(ParamType1, Other...)
	{
		if (sizeof...(Other) != 0) return nullptr;

		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			checkTypeMatch<ParamType1>(b, 0);

			return (ReturnType(*)(ParamType1, Other...))b->func;
		}

		return nullptr;
	}

#pragma warning (pop)

	template <typename ReturnType, typename ParamType1, typename ParamType2> ReturnType(*getCompiledFunction2(const Identifier& id))(ParamType1, ParamType2)
	{
		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			checkTypeMatch<ParamType1>(b, 0);
			checkTypeMatch<ParamType2>(b, 1);

			return (ReturnType(*)(ParamType1, ParamType2))b->func;
		}

		return nullptr;
	}

	NativeJIT::FunctionBuffer* createFunctionBuffer()
	{
		functionBuffers.add(new NativeJIT::FunctionBuffer(codeAllocator, 8192));

		return functionBuffers.getLast();
	}

	OwnedArray<GlobalBase> globals;

	OwnedArray<BaseFunction> compiledFunctions;

	OwnedArray<BaseFunction> exposedFunctions;

	Array<NativeJIT::FunctionBuffer*> functionBuffers;

	typedef ReferenceCountedObjectPtr<NativeJITScope> Ptr;

	NativeJIT::ExecutionBuffer codeAllocator;
	NativeJIT::Allocator allocator;
};



class GlobalNode
{
public:

	GlobalNode(GlobalBase* b_) :
		b(b_)
	{};

	bool matchesName(const Identifier& id)
	{
		return b->id == id;
	}

	template <typename T> void checkType(NativeJIT::Node<T>& /*n*/)
	{
		if (!NativeJITTypeHelpers::matchesType<T>(b->type))
		{
			throw String("Global Reference parsing error");
		}
	}

	template <typename T> void addNode(NativeJIT::ExpressionNodeFactory* expr, NativeJIT::Node<T>& newNode)
	{
		checkType(newNode);

		if (nodes.size() == 0)
		{
			nodes.add(&newNode);
		}
		else
		{
			auto& lastNode = getLastNode<T>();
			auto& zero = expr->template Immediate<T>(0);
			auto& zeroed = expr->Mul(lastNode, zero);
			auto& added = expr->template Add<T>(zeroed, newNode);

			nodes.add(&added);
		}
	}


	template <typename T, typename R> NativeJIT::Node<R>& callStoreFunction(NativeJIT::ExpressionNodeFactory* expr)
	{
		auto& t = getLastNode<T>();
		auto& f1 = expr->Immediate(GlobalBase::store<T>);
		auto& gb = expr->Immediate(b);
		auto& f2 = expr->Call(f1, gb, t);

		if (typeid(R) == typeid(T))
		{
			return *dynamic_cast<NativeJIT::Node<R>*>(&f2);
		}
		else
		{
			auto& cast = expr->template Cast<R, T>(f2);

			return cast;
		}
	}


	template <typename T> NativeJIT::Node<T>& getLastNode() const
	{
		auto n = dynamic_cast<NativeJIT::Node<T>*>(nodes.getLast());

		if (n == nullptr)
		{
			throw "Global Type mismatch";
		}

		return *n;
	}

	template <typename R> NativeJIT::Node<R>& getAssignmentNodeForReturnStatement(NativeJIT::ExpressionNodeFactory* expr)
	{
		if (NativeJITTypeHelpers::matchesType<float>(b->type)) return callStoreFunction<float, R>(expr);
		if (NativeJITTypeHelpers::matchesType<double>(b->type)) return callStoreFunction<double, R>(expr);
		if (NativeJITTypeHelpers::matchesType<int>(b->type)) return callStoreFunction<int, R>(expr);

		return expr->Immediate(R());
	}

protected:

	GlobalBase* b;

	Array<NativeJIT::NodeBase*> nodes;
};



struct FunctionInfo
{
	FunctionInfo():
		code(String::CharPointerType(nullptr))
	{
		
	}

	Array<Identifier> parameterTypes;
	Array<Identifier> parameterNames;

	String::CharPointerType code;
	
	int length;


	String program;

	int offset = 0;

	int parameterAmount = 0;
};


#include "Parser.cpp"


#endif  // PARSER_H_INCLUDED
