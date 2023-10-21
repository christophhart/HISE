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
		VariableStorage(HiseEvent& m);

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

        VariableStorage toTypeDynamic(ID t) const
        {
            if(t == ID::Float)
                return toFloat();
            if(t == ID::Double)
                return toDouble();
            if(t == ID::Integer)
                return toInt();
            
            return *this;
        }
        
		template <ID TypeID> auto toType() const
		{
			if (TypeID == ID::Float)
				return toFloat();
			else if (TypeID == ID::Double)
				return toDouble();
		else if(TypeID == ID::Integer)
		return toInt();
		else if(TypeID == ID::Block)
		return toBlock();
		else if(TypeID == ID::Pointer)
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






}
