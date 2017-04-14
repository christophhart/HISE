/*
  ==============================================================================

    Parser.h
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED


#if INCLUDE_BUFFERS

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
		}

		return DummyReturn();
	}

	template <typename DummyReturn> static DummyReturn setSize(Buffer* b, int newSize)
	{
		b->b = new juce::VariantBuffer(newSize);

		return DummyReturn();
	}

	
};

#endif


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

	template<typename T> static T store(GlobalBase* b, T newValue)
	{
		jassert(HiseJITTypeHelpers::matchesType<T>(b->type));

		
        T* castedData = reinterpret_cast<T*>(&b->data);
        
        *castedData = newValue;

		return T();
	}

	static AddressType getBufferData(AddressType g_)
	{
		auto g = (GlobalBase*)g_;

		return reinterpret_cast<AddressType>(getBuffer(g)->b->buffer.getWritePointer(0));
	}

	static int getBufferDataSize(AddressType g_)
	{
		auto g = (GlobalBase*)g_;

		return getBuffer(g)->b->size;
	}

	template<typename T> static T get(GlobalBase* b)
	{
		jassert(HiseJITTypeHelpers::matchesType<T>(b->type));

		return *reinterpret_cast<T*>(&b->data);
	}

	static void setBufferValue(GlobalBase* b, int offset, float newValue)
	{
		b->ownedBuffer.b->setSample(offset, newValue);
	}

	TypeInfo getType() const noexcept { return type; }
	
	template <typename T> static GlobalBase* create(const Identifier& id)
	{
		return new GlobalBase(id, typeid(T));
	}

#if INCLUDE_BUFFERS
	void setBuffer(juce::VariantBuffer* b)
	{
		jassert(HiseJITTypeHelpers::matchesType<Buffer*>(type));

		ownedBuffer.setBuffer(b);
	}
	
	static Buffer* getBuffer(GlobalBase* b)
	{
		jassert(HiseJITTypeHelpers::matchesType<Buffer*>(b->type));

		return &b->ownedBuffer;
	}

	int hasOverflowError() const
	{
		if (HiseJITTypeHelpers::matchesType<Buffer*>(type))
		{
			return ownedBuffer.overflowError;
		}
		else
		{
			return -1;
		}
	}

	Buffer ownedBuffer;

#endif

	TypeInfo type;
	Identifier id;
	
	double data = 0.0;

	float* bufferData = nullptr;

	bool isConst = false;
};


#define NATIVE_JIT_ADD_C_FUNCTION_0(return_type, name) exposedFunctions.add(new TypedFunction<return_type>(Identifier(#name), (void*)(static_cast<return_type(*)()>(name)))); 
#define NATIVE_JIT_ADD_C_FUNCTION_1(return_type, name, param1_type) exposedFunctions.add(new TypedFunction<return_type, param1_type>(Identifier(#name), (void*)(static_cast<return_type(*)(param1_type)>(name)), param1_type())); 
#define NATIVE_JIT_ADD_C_FUNCTION_2(return_type, name, param1_type, param2_type) exposedFunctions.add(new TypedFunction<return_type, param1_type, param2_type>(Identifier(#name), (void*)(static_cast<return_type(*)(param1_type, param2_type)>(name)), param1_type(), param2_type())); 
#define ADD_C_FUNCTION_3(return_type, name, param1_type, param2_type, param3_type) exposedFunctions.add(new TypedFunction<return_type, param1_type, param2_type, param3_type>(Identifier(#name), (void*)(static_cast<return_type(*)(param1_type, param2_type, param3_type)>(name)), param1_type(), param2_type(), param3_type())); 

#define ADD_GLOBAL(type, name) globals.add(GlobalBase::create<type>(name));



struct FunctionInfo
{
	FunctionInfo():
		code(String::CharPointerType(nullptr)),
		lineType(typeid(void))
	{
		
	}

	Array<Identifier> parameterTypes;
	Array<Identifier> parameterNames;

	String::CharPointerType code;
	
	int length;

	Identifier id;

	String program;

	int offset = 0;

	int parameterAmount = 0;

	bool useSafeBufferFunctions = true;
	bool addVoidReturnStatement;

	TypeInfo lineType;
};


#include "Parser.cpp"


#endif  // PARSER_H_INCLUDED
