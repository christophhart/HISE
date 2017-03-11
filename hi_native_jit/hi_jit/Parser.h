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



struct BufferHolder
{
	BufferHolder(juce::VariantBuffer* buffer) :
		b(buffer)
	{};

	BufferHolder():
		b(nullptr)
	{}

	BufferHolder(int size) :
		b(new juce::VariantBuffer(size))
	{}

	~BufferHolder()
	{
		b = nullptr;
	}

	void setBuffer(juce::VariantBuffer* newBuffer)
	{
		b = newBuffer;
	}

	juce::VariantBuffer::Ptr b;

	int overflowError = -1;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BufferHolder);
};

typedef BufferHolder Buffer;


struct BufferOperations
{
	static float getSample(Buffer* b, int index)
	{
		if (b->b.get() == nullptr)
			return 0.0f;

		auto& vb = *b->b.get();

		if (isPositiveAndBelow(index, vb.size))
		{
			return vb.buffer.getReadPointer(0)[index];
		}
		else
		{
			b->overflowError = index;
			return 0.0f;
		}
	};

	static float getSampleRaw(Buffer*b, int index)
	{
		return b->b->buffer.getReadPointer(0)[index];
	}

	template <typename DummyReturn> static DummyReturn setSampleRaw(Buffer* b, int index, float value)
	{
		b->b->buffer.getWritePointer(0)[index] = value;
		return DummyReturn();
	}

	template <typename DummyReturn> static DummyReturn setSample(Buffer* b, int index, float value)
	{
		if (b->b.get() == nullptr)
			return DummyReturn();

		auto& vb = *b->b.get();

		if (isPositiveAndBelow(index, vb.size))
		{
			vb.buffer.getWritePointer(0)[index] = value;
		}
		else
		{
			b->overflowError = index;
			return DummyReturn();
		}
	}

	template <typename DummyReturn> static DummyReturn setSize(Buffer* b, int newSize)
	{
		b->b = new juce::VariantBuffer(newSize);

		return DummyReturn();
	}

	
};

constexpr size_t getGlobalDataSize()
{
	return sizeof(double) > sizeof(juce::VariantBuffer::Ptr) ? sizeof(double) : sizeof(juce::VariantBuffer::Ptr);
}

struct GlobalBase
{
	

	GlobalBase(const Identifier& id_, TypeInfo type_) :
		id(id_),
		type(type_),
		data(0.0)
	{
		
	};

	~GlobalBase()
	{
		
	}

    template<typename T> static T returnSameValue(T value)
    {
        return value;
    }
    
	template<typename T> static T store(GlobalBase* b, T newValue)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		jassert(typeid(T) != typeid(Buffer*));

        T* castedData = reinterpret_cast<T*>(&b->data);
        
        *castedData = newValue;

		return T();
	}

	template<typename T> static T get(GlobalBase* b)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		return *reinterpret_cast<T*>(&b->data);
	}

	TypeInfo getType() const noexcept { return type; }
	
	template <typename T> static GlobalBase* create(const Identifier& id)
	{
		return new GlobalBase(id, typeid(T));
	}

	void setBuffer(juce::VariantBuffer* b)
	{
		jassert(NativeJITTypeHelpers::matchesType<Buffer*>(type));

		ownedBuffer.setBuffer(b);
	}
	
	static Buffer* getBuffer(GlobalBase* b)
	{
		jassert(NativeJITTypeHelpers::matchesType<Buffer*>(b->type));

		return &b->ownedBuffer;
	}

	int hasOverflowError() const
	{
		if (NativeJITTypeHelpers::matchesType<Buffer*>(type))
		{
			return ownedBuffer.overflowError;
		}
		else
		{
			return -1;
		}
	}

	TypeInfo type;
	Identifier id;
	
	double data = 0.0;

	Buffer ownedBuffer;

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
		allocator(32768)
	{
		codeAllocators.add(new NativeJIT::ExecutionBuffer(32768));

		addExposedFunctions();
	}

	~Pimpl()
	{
		functionBuffers.clear();

		for (int i = 0; i < codeAllocators.size(); i++)
		{
			codeAllocators[i]->Reset();
		}

		codeAllocators.clear();

		allocator.Reset();
	}

	void addExposedFunctions()
	{
		NATIVE_JIT_ADD_C_FUNCTION_1(float, sinf, float);
		NATIVE_JIT_ADD_C_FUNCTION_0(double, getSampleRate);
		NATIVE_JIT_ADD_C_FUNCTION_2(float, powf, float, float);
		NATIVE_JIT_ADD_C_FUNCTION_1(float, fabsf, float);
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

	void setGlobalVariable(const juce::Identifier& id, juce::var value)
	{
		if (auto g = getGlobal(id))
		{
			TypeInfo t = g->getType();

			if (value.isInt64() || value.isDouble() || value.isInt())
			{
				if (NativeJITTypeHelpers::matchesType<float>(t)) GlobalBase::store<float>(g, (float)value);
				else if (NativeJITTypeHelpers::matchesType<double>(t)) GlobalBase::store<double>(g, (double)value);
				else if (NativeJITTypeHelpers::matchesType<int>(t)) GlobalBase::store<int>(g, (int)value);
				else throw String(id.toString() + " - var type mismatch: " + value.toString());
			}
			else if (value.isBuffer() && NativeJITTypeHelpers::matchesType<Buffer*>(t))
			{
				g->setBuffer(value.getBuffer());
			}
			else
			{
				throw String(id.toString() + " - var type mismatch: " + value.toString());
			}
		}
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
		try
		{
			functionBuffers.add(new NativeJIT::FunctionBuffer(*codeAllocators.getLast(), 8192));
		}
		catch (std::runtime_error e)
		{
			codeAllocators.add(new NativeJIT::ExecutionBuffer(32768));

			functionBuffers.add(new NativeJIT::FunctionBuffer(*codeAllocators.getLast(), 8192));
		}
		
		return functionBuffers.getLast();
	}

	OwnedArray<GlobalBase> globals;

	OwnedArray<BaseFunction> compiledFunctions;

	OwnedArray<BaseFunction> exposedFunctions;

	OwnedArray<NativeJIT::FunctionBuffer> functionBuffers;

	typedef ReferenceCountedObjectPtr<NativeJITScope> Ptr;

	OwnedArray<NativeJIT::ExecutionBuffer> codeAllocators;
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

	template <typename T> void checkType()
	{
		if (!NativeJITTypeHelpers::matchesType<T>(b->type))
		{
			throw String("Global Reference parsing error");
		}
	}

	template <typename T> void addNode(NativeJIT::ExpressionNodeFactory* expr, NativeJIT::NodeBase* newNode)
	{
		checkType<T>();

		auto typed = dynamic_cast<NativeJIT::Node<T>*>(newNode);

		if (nodes.size() == 0)
		{
			nodes.add(newNode);
		}
		else
		{
			auto& lastNode = getLastNode<T>();
			auto& zero = expr->template Immediate<T>(0);
			auto& zeroed = expr->Mul(lastNode, zero);
			auto& added = expr->template Add<T>(zeroed, *typed);

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

	NativeJIT::NodeBase* getLastNodeUntyped() const
	{
		if (nodes.size() != 0)
		{
			return nodes.getLast();
		}
		else
		{
			throw "Global Node not found";
		}

		return nodes.getLast();
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

	bool useSafeBufferFunctions = true;
};


#include "Parser.cpp"


#endif  // PARSER_H_INCLUDED
