/*
  ==============================================================================

	hnode_DynamicType.h
	Created: 2 Sep 2018 6:48:30pm
	Author:  Christoph

  ==============================================================================
*/


namespace snex
{

using namespace Types;

#define CALC_IF_FLOAT(op) if(expectedType == Types::ID::Float && getType() == Types::ID::Float) data.f.value op (float)other;
#define CONVERT_TO_DOUBLE_WITH_OP(op) { data.d.type = Types::Double; data.d.value op other.toDouble(); }
#define CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(op) { CALC_IF_FLOAT(op) else CONVERT_TO_DOUBLE_WITH_OP(op ) }

class VariableStorage
{
public:

	VariableStorage();

	VariableStorage(const VariableStorage& other) :
		data(other.data)
	{};

	VariableStorage(Types::ID type_, const var& value);
	VariableStorage(FloatType s);
	VariableStorage(double d);
	VariableStorage(int s);
	VariableStorage(const block& b);
	VariableStorage(const HiseEvent& m);

	VariableStorage(void* ptr)
	{
		data.p.type = Types::ID::Pointer;
		data.p.data = ptr;
		data.p.size = 0;
	}

	template <class T> VariableStorage(T* ptr)
	{
		data.p.type = Types::ID::Pointer;
		data.p.data = ptr;
		data.p.size = sizeof(T);
	}

	template <class T> VariableStorage& operator =(T* ptr)
	{
		data.p.type = Types::ID::Pointer;
		data.p.data = ptr;
		data.p.size = sizeof(T);
		return *this;
	}

	VariableStorage(void* objectPointer, int objectSize);

	VariableStorage& operator=(const VariableStorage& other);
	VariableStorage& operator =(int s);
	VariableStorage& operator =(FloatType s);
	VariableStorage& operator =(double s);
	VariableStorage& operator =(const block& s);

	bool operator==(const VariableStorage& other) const;

	void setWithType(Types::ID newType, double value);
	void set(FloatType s);
	void set(double s);
	void setDouble(double newValue);
	void set(int s);;
	void set(block&& s);;
	void set(block& b);
	void set(void* objectPointer, int newSize);

	void clear();

	explicit operator FloatType() const noexcept;
	explicit operator double() const noexcept;
	explicit operator int() const;
	explicit operator block() const;
	explicit operator HiseEvent() const;
	explicit operator void* () const;

	template <Types::ID expectedType> VariableStorage& store(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value = (int)other;
else if constexpr(expectedType == Types::ID::Float || expectedType == Types::ID::Double)
CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(= )

return *this;
	}


	template <Types::ID expectedType> VariableStorage& add(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value += (int)other;
else if constexpr(expectedType == Types::ID::Float || expectedType == Types::ID::Double)
CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(+= )


return *this;
	}

	template <Types::ID expectedType> VariableStorage& and_(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value &= (int)other;

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& or_(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value |= (int)other;

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& mod(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value %= (int)other;

		return *this;
	}

	template <Types::ID expectedType> VariableStorage& sub(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value -= (int)other;
else if constexpr(expectedType == Types::ID::Float || expectedType == Types::ID::Double)
CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(-= )


return *this;
	}

	template <Types::ID expectedType> VariableStorage& div(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value /= (int)other;
else if constexpr(expectedType == Types::ID::Float || expectedType == Types::ID::Double)
CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(/= )

return *this;
	}

	template <Types::ID expectedType> VariableStorage& mul(const VariableStorage& other)
	{
		if constexpr(expectedType == Types::ID::Integer)
			data.i.value *= (int)other;
else if constexpr(expectedType == Types::ID::Float || expectedType == Types::ID::Double)
CONVERT_TO_DOUBLE_IF_REQUIRED_AND_OP(*= )


return *this;
	}

	Types::ID getType() const noexcept
	{
		auto t = getTypeValue();
		
		if (t == 0)
			return Types::ID::Void;
		else if((int)t < (int)HiseEvent::Type::numTypes)
			return Types::ID::Event;
		else
			return (Types::ID)t;
	}

	void* toPtr() const;
	double toDouble() const;
	FloatType toFloat() const;
	int toInt() const;
	block toBlock() const;
	HiseEvent toEvent() const;
	bool isVoid() const noexcept { return getType() == Types::ID::Void; }
	size_t getSizeInBytes() const noexcept;

	var toVar() const
	{
		switch(getType())
		{
		case Void: return var(); break;
		case Event: jassertfalse; break;
		case Pointer: jassertfalse; break;
		case Float: return var(toFloat());;
		case Double: return var(toDouble());;
		case Integer: return var(toInt());
		case Block: jassertfalse; //maybe a Buffer? break;
		case Dynamic: return var();;
		default: ;
		}

		return {};
	}

    VariableStorage toTypeDynamic(ID t) const
    {
        if(t == ID::Float)
            return toFloat();
        if(t == ID::Double)
            return toDouble();
        if(t == ID::Integer)
            return toInt();
		if(t == ID::Event)
			return toEvent();
        
        return *this;
    }
    
	template <ID TypeID> auto toType() const
	{
		if constexpr (TypeID == ID::Float)
			return toFloat();
		else if constexpr (TypeID == ID::Double)
			return toDouble();
	else if constexpr (TypeID == ID::Integer)
	return toInt();
	else if constexpr (TypeID == ID::Block)
	return toBlock();
	else if constexpr (TypeID == ID::Event)
		return toEvent();
	else if constexpr (TypeID == ID::Pointer)
	return toPtr();

	return 0;
	}

	void* getDataPointer() const
	{
		if (getTypeValue() == Types::ID::Pointer)
			return data.p.data;
		if (getTypeValue() == Types::ID::Float ||
			getTypeValue() == Types::ID::Double ||
			getTypeValue() == Types::ID::Integer)
		{
			return const_cast<void*>((const void*)&data.d.value);
		}
		else
			return const_cast<void*>((const void*)&data);
	}

	int getPointerSize() const;

private:

	uint8 getTypeValue() const
	{
		return reinterpret_cast<const uint8*>(&data)[0];
	}

	struct DoubleData
	{
		int type;
		int unused;
		double value;
	};

	struct IntData
	{
		int type;
		int unused;
		int64 value;
	};

	struct FloatData
	{
		int type;
		int unused;
		float value;
		int unused2;
	};

	struct PointerData
	{
		int type;
		int size;
		void* data;
	};

	union Data
	{
		Data()
		{
			e = HiseEvent();
		}

		Data(const Data& other) :
			d(other.d)
		{};


		block b;
		DoubleData d;
		IntData i;
		FloatData f;
		HiseEvent e;
		PointerData p;
	};

	Data data;
};

/* An external function map will hold a reference to various functions with the signature
 *
 * void f(void* obj, int index, const VariableStorage& value)
 *
 * an can be used to register external functions to a eventnode network.
 *
 */
struct ExternalFunctionMap
{
	typedef void(*ExternalFunction)(void*, int index, const VariableStorage&);

	struct Reference
	{
	    void init(const ExternalFunctionMap& m_, int index_)
	    {
		    parent = &m_;
	        index = index_;
	    }

	    template <typename T> void ping(const T& v) const
	    {
	        if(auto m = static_cast<const ExternalFunctionMap*>(parent))
				m->ping(index, v);
	    }

		int unused = (int)Types::ID::Pointer;
		int index = -1;
		ExternalFunctionMap const* parent = nullptr;
	};

    

	/** Register a function to be called with the index. */
    void registerFunction(int idx, void* obj, const ExternalFunction& f)
    {
        Item x = { obj, f };
	    functions[idx] = x;
    }

private:

	struct Item
	{
        operator bool() const { return f != nullptr; }

        void ping(int index, const VariableStorage& v) const
        {
	        if(*this)
                f(obj, index, v);
        }

        void* obj = nullptr;
		ExternalFunction f = nullptr;
	};

	template <typename T> void ping(int idx, const T& value) const
    {
		if(functions.find(idx) != functions.end())
		{
			VariableStorage v(value);
			functions.at(idx).ping(idx, v);
		}
    }

    std::map<int, Item> functions;
};


using ExternalFunction = ExternalFunctionMap::Reference;



}
