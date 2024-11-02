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

#ifndef SNEX_ARRAY_TYPES_INCLUDED
#define SNEX_ARRAY_TYPES_INCLUDED

#include <cstring>

#include <type_traits>
#include <initializer_list>
#if !JUCE_ARM
#include <nmmintrin.h>
#endif
#include <stdint.h>



namespace snex {
namespace Types {

template <typename T> struct SpanOperators
{
	struct add
	{
		using DataType = T;
		static constexpr bool shouldLoadSource() { return true; }
		static forcedinline void op(T& dst, const T& src) { dst += src; }
		static forcedinline void vOp(__m128& dst, const __m128& src) { dst = _mm_add_ps(dst, src); }
	};

	struct multiply 
	{ 
		using DataType = T;
		static constexpr bool shouldLoadSource() { return true; }
		static forcedinline void op(T& dst, const T& src) { dst *= src; } 
		static forcedinline void vOp(__m128& dst, const __m128& src) { dst = _mm_mul_ps(dst, src); }
	};

	struct assign 
	{ 
		using DataType = T;
		static constexpr bool shouldLoadSource() { return false; }
		static forcedinline void op(T& dst, const T& src) { dst = src; } 
		static forcedinline void vOp(__m128& dst, const __m128& src) { dst = src; }
	};
	
	struct divide 
	{ 
		using DataType = T;
		static constexpr bool shouldLoadSource() { return true; }
		static forcedinline void op(T& dst, const T& src) { dst /= src; } 
		static forcedinline void vOp(__m128& dst, const __m128& src) { dst = _mm_div_ps(dst, src); }
	};

	struct sub 
	{ 
		using DataType = T;
		static constexpr bool shouldLoadSource() { return true; }
		static forcedinline void op(T& dst, const T& src) { dst -= src; } 
		static forcedinline void vOp(__m128& dst, const __m128& src) { dst = _mm_sub_ps(dst, src); }
	};
};


/** A fixed-size array type for SNEX.
    @ingroup snex_containers

	The span type is an iteratable compile-time array. The elements can be accessed 
	using the []-operator or via a range-based for loop.

	In order to prevent unsafe out-of-bounds memory access, the []-operator access can
    either take a literal integer index (that will be compiled-time checked
    against the boundaries), or a index subtype with a
	defined out-of-bounds behaviour (wrapping, clamping, etc).
*/
template <class T, int Size, int Alignment=16> struct span
{
	static constexpr ArrayID ArrayType = Types::ArrayID::SpanType;
	using DataType = T;
	using Type = span<T, Size>;

	static constexpr int s = Size;

	static constexpr bool hasCompileTimeSize() { return true; }

	span()
	{
		clear();
	}

	constexpr span(const std::initializer_list<T>& l)
	{
		if (l.size() == 1)
		{
			for (int i = 0; i < Size; i++)
			{
				data[i] = *l.begin();
			}
		}
		else
			memcpy(data, l.begin(), sizeof(T)*Size);
	}

	static Type& fromExternalData(T* data, int numElements)
	{
		jassert(numElements <= Size);
		return *reinterpret_cast<Type*>(data);
	}

	static constexpr size_t getSimdSize()
	{
		if (isSimdable())
		{
			if (std::is_same<T, float>())
				return 4;
			else
				return 2;
		}
		else
			return 1;
	}

	Type& operator=(const std::initializer_list<T>& l)
	{
		*this = (float)*l.begin();
		return *this;
	}

#if 0
	Type& operator=(const T& t)
	{
		if constexpr (isSimdable())
		{
			constexpr int numLoop = Size / getSimdSize();

			if (std::is_same<DataType, float>())
			{
				auto dst = (float*)data;
				auto v = _mm_load_ps1(&t);

				for (int i = 0; i < numLoop; i++)
				{
					_mm_store_ps(dst, v);
					dst += getSimdSize();
				}
			}
		}
		else
		{
			for (auto& v : *this)
				v = t;
		}

		return *this;
	}
#endif

	operator T()
	{
		static_assert(Size == 1, "not a single element span");
		return *begin();
	}

	JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4267);

	template <typename OpType, typename OperandType> Type& performOp(const OperandType& op)
	{
		if constexpr (std::is_same<T, OperandType>())
		{
			// OperandType is a number
			//static_assert(std::is_arithmetic<T>(), "not an arithmetic type");

			if constexpr (isSimdable())
			{
				// We can use SSE instructions

				static constexpr int numLoop = Size / getSimdSize();

				if (std::is_same<DataType, float>())
				{
					auto ptr = (float*)data;
					auto v = _mm_load_ps1(&op);

					for (int i = 0; i < numLoop; i++)
					{
						__m128 dst;

						if constexpr (OpType::shouldLoadSource())
							dst = _mm_load_ps(ptr);

						OpType::vOp(dst, v);
						_mm_store_ps(ptr, dst);
						ptr += getSimdSize();
					}
				}
			}
			else
			{
				for (auto& s : *this)
					OpType::op(s, op);
			}
		}
		else if constexpr (std::is_pointer<T>())
		{
#if JUCE_WINDOWS
            using PT = typename SpanOperators<T>::assign;
            static_assert(std::is_same<OpType, PT>(), "only assignment supported");
#endif

			static_assert(std::is_same<typename OperandType::DataType, T>(), "type mismatch");

			memset(begin(), 0, sizeof(void*) * (size_t)size());
            
            int numElements = jmin((int)op.size(), (int)size());
            
            
			memcpy(begin(), op.begin(), sizeof(void*) * numElements);
		}
		else
		{
			// OperandType is a span with the same element type

			static_assert(OperandType::hasCompileTimeSize(), "not a compile time array");
			static_assert(std::is_arithmetic<T>(), "not an arithmetic type");
            static_assert(std::is_same<typename OperandType::DataType, T>(), "type mismatch");

			static constexpr int ThisSize = s;
			static constexpr int OtherSize = OperandType::s;

			if constexpr (isSimdable() && OperandType::isSimdable())
			{
				// We can use SSE instructions for both spans

				if constexpr (ThisSize == OtherSize)
				{
					// same size, simple loop
					auto p1 = begin();
					auto p2 = op.begin();

					for (int i = 0; i < size(); i += getSimdSize())
					{
						__m128 dst;

						if constexpr (OpType::shouldLoadSource())
							dst = _mm_load_ps(p1);

						__m128 src = _mm_load_ps(p2);

						OpType::vOp(dst, src);

						_mm_store_ps(p1, dst);

						p1 += getSimdSize();
						p2 += getSimdSize();
					}
				}
				else
				{
					static constexpr int MaxIndex = jmin(ThisSize, OtherSize) / (int)getSimdSize();
					

					using SSESpanType = span<span<float, (int)getSimdSize()>, ThisSize / (int)getSimdSize()>;
					using OpSSESpanType = span<span<float, (int)getSimdSize()>, OtherSize / (int)getSimdSize()>;

					auto& thisSSE = *reinterpret_cast<SSESpanType*>(this);
					auto& opSSE = *reinterpret_cast<const OpSSESpanType*>(&op);

					for (int i = 0; i < MaxIndex; i++)
					{
						auto* dPtr = thisSSE[i].begin();
						__m128 dst;

						if constexpr (OpType::shouldLoadSource())
							dst = _mm_load_ps(dPtr);

						__m128 src = _mm_load_ps(opSSE[i].begin());
						OpType::vOp(dst, src);
						_mm_store_ps(dPtr, dst);
					}

					if constexpr (ThisSize > OtherSize)
					{
						auto lastValue = _mm_load_ps1(&op[OtherSize - 1]);
						static constexpr int NumLeftOver = (ThisSize - OtherSize) / getSimdSize();

						for (int i = 0; i < NumLeftOver; i++)
						{
							auto dPtr = thisSSE[i + MaxIndex].begin();

							__m128 dst;
							
							if constexpr (OpType::shouldLoadSource())
								dst = _mm_load_ps(dPtr);

							OpType::vOp(dst, lastValue);
							_mm_store_ps(dPtr, dst);
						}
					}
				}
			}
			else
			{
				if constexpr (ThisSize == OtherSize)
				{
					for (int i = 0; i < size(); i++)
						OpType::op((*this)[i], op[i]);
				}
				else
				{
					for (int i = 0; i < size(); i++)
						OpType::op((*this)[i], op[jmin<int>(i, OtherSize)]);
				}
			}
		}

		return *this;
	}

	JUCE_END_IGNORE_WARNINGS_MSVC;

	template <typename OperandType> Type& operator+(const OperandType& op) { return this->performOp<typename SpanOperators<T>::add>(op); }
	template <typename OperandType> Type& operator+=(const OperandType& t) { return *this + t; }

	template <typename OperandType> Type& operator*(const OperandType& op) { return this->performOp<typename SpanOperators<T>::multiply>(op); }
	template <typename OperandType> Type& operator*=(const OperandType& t) { return *this * t; }

	template <typename OperandType> Type& operator/(const OperandType& op) { return this->performOp<typename SpanOperators<T>::divide>(op); }
	template <typename OperandType> Type& operator/=(const OperandType& t) { return *this / t; }

	template <typename OperandType> Type& operator-(const OperandType& op) { return this->performOp<typename SpanOperators<T>::sub>(op); }
	template <typename OperandType> Type& operator-=(const OperandType& t) { return *this - t; }

	template <typename OperandType> Type& operator=(const OperandType& t) { return this->performOp<typename SpanOperators<T>::assign>(t); };


#if 0
	Type& operator=(const Type& other)
	{
		memcpy(data, other.begin(), size() * sizeof(T));
		return *this;
	}
#endif

	constexpr bool isFloatType()
	{
		return std::is_same<float, T>() || std::is_same<double, T>();
	}

#if 0
	Type& operator* (const Type& other)
	{
		static_assert(std::is_arithmetic<T>(), "not an arithmetic type");

		const auto src = other.begin();

		for (int i = 0; i < size(); i++)
			data[i] *= src[i];

		return *this;
	}

	Type& operator *=(const Type& other)
	{
		return *this * other;
	}

	Type& operator*=(const T& scalar)
	{
		return *this * scalar;
	}

	Type& operator*(const T& scalar)
	{
		static_assert(std::is_arithmetic<T>(), "not an arithmetic type");

		for (auto& s : *this)
			s *= scalar;

		return *this;
	}
#endif

	void clear()
	{
		for (auto& s : *this)
			s = DataType();
	}

	T accumulate() const
	{
		static_assert(std::is_arithmetic<T>(), "not an arithmetic type");

		T v = T(0);

		for (const auto& s : *this)
			v += s;
		
		return v;
	}

	static constexpr bool isSimdType()
	{
		return isSimdable();
	}

	static constexpr bool isSimdable()
	{
		return (std::is_same<T, float>() && Size % 4 == 0);
	}

	bool isAlignedTo16Byte() const
	{
		return isAlignedTo16Byte(*this);
	}


	template <class InterpolatorType> DataType interpolate(InterpolatorType& i) const
	{
		return i.getFrom(*this);
	}

	template <class ObjectType> static bool isAlignedTo16Byte(ObjectType& d)
	{
		return reinterpret_cast<uint64_t>(d.begin()) % 16 == 0;
	}

	template <int ChannelAmount> span<span<T, Size / ChannelAmount>, ChannelAmount>& split()
	{
		static_assert(Size % ChannelAmount == 0, "Can't split with slice length ");

		return *reinterpret_cast<span<span<T, Size / ChannelAmount>, ChannelAmount>*>(this);
	}

	void copyTo(Type& other) const
	{
		auto src = begin();
		auto dst = other.begin();

		for (int i = 0; i < Size; i++)
			*dst++ = *src++;
	}

	template <typename IndexType> IndexType index(int initValue)
	{
		return IndexType(initValue);
	}

	void addTo(Type& other) const
	{
		auto src = begin();
		auto dst = other.begin();

		for (int i = 0; i < Size; i++)
			*dst++ += *src++;
	}

	/** Converts this float span into a SSE span with 4 float elements at once. 
		This checks at compile time whether the span can be converted.
	*/
	span<span<float, 4>, Size / 4>& toSimd()
	{
		using Type = span<span<float, 4>, Size / 4>;

		static_assert(isSimdable(), "is not SIMDable");
		jassert(isAlignedTo16Byte());

		return *reinterpret_cast<Type*>(this);
	}

	template<typename IndexType> typename std::enable_if<index::Helpers::canReturnReference<IndexType>(), const T&>::type
		operator[](const IndexType& t) const
	{
		if constexpr (std::is_integral<IndexType>())
		{
			jassert(isPositiveAndBelow((int)t, size()));
			return data[t];
		}
		else
		{
			return t.getFrom(*this);
		}
	}

	template<typename IndexType> typename std::enable_if<!index::Helpers::canReturnReference<IndexType>(), T>::type
		operator[](const IndexType& t) const
	{
		return t.getFrom(*this);
	}

	template<typename IndexType> typename std::enable_if<!index::Helpers::canReturnReference<IndexType>(), T>::type
		operator[](const IndexType& t)
	{
		return t.getFrom(*this);
	}

	template<typename IndexType> typename std::enable_if<index::Helpers::canReturnReference<IndexType>(), T&>::type
		operator[](const IndexType& t)
	{
		if constexpr (std::is_integral<IndexType>())
		{
			jassert(isPositiveAndBelow((int)t, size()));
			return data[t];
		}
		else
		{
			return *const_cast<T*>(&t.getFrom(*this));
		}
	}

	/** Morphs any pointer of the data type into this type. */
	constexpr static Type& as(T* ptr)
	{
		return *reinterpret_cast<Type*>(ptr);
	}

	/** This method allows a lean range-based for loop syntax:
	
		@code
		span<float, 512> data;
		
		for(auto& s: data)
		    s = 0.5f;
		@endcode
	*/
	T* begin() const
	{
		return const_cast<T*>(data);
	}

	T* end() const
	{
		return const_cast<T*>(data + Size);
	}

	void fill(const T& value)
	{
		for (auto& v : *this)
			v = value;
	}

	/** Returns the number of elements in this span. */
	constexpr int size() const
	{
		return Size;
	}

	static constexpr int alignment()
	{
		return Alignment;
	}

	alignas(alignment()) T data[Size];
};



/** This alias is a special type on its own as it has mathematical operators that directly translate to SSE instructions. */
using float4 = span<float, 4>;

/** The dyn template class is an array that is only referencing memory that is owned by something else.
	@ingroup snex_containers
 
	It can be freely resized, redirected and allows a fast and safe iteration and []-operator access using
    the index type.
 
    Maybe the most important usage of this class is a dyn<float> which has an alias called `block`.
*/
template <class T> struct dyn
{
	static constexpr ArrayID ArrayType = Types::ArrayID::DynType;
	using Type = dyn<T>;
	using DataType = T;

	static constexpr bool hasCompileTimeSize() { return false; }

	dyn() :
		data(nullptr),
		size_(0)
	{}

	template<class Other> dyn(Other& o) :
		data(o.begin()),
		size_((int)(o.end() - o.begin()))
	{
        static_assert(std::is_same<DataType, typename Other::DataType>(), "not same data type");
	}

	template<class Other> dyn(Other& o, size_t s_) :
		data(o.begin()),
		size_((int)s_)
	{
        static_assert(std::is_same<DataType, typename Other::DataType>(), "not same data type");
	}

	dyn(juce::HeapBlock<T>& d, size_t s_):
		data(d.get()),
		size_((int)s_)
	{}

	dyn(T* data_, size_t s_) :
		data(data_),
		size_((int)s_)
	{}

	template<class Other> dyn(Other& o, size_t s_, size_t offset) :
		data(o.begin() + offset),
		size_((int)s_)
	{
		auto fullSize = o.end() - o.begin();
		jassert(offset + size() < fullSize);
	}

	dyn<float4> toSimd() const
	{
		dyn<float4> rt;

		jassert(this->size() % 4 == 0);

		rt.data = reinterpret_cast<float4*>(begin());
		rt.size_ = size() / 4;

		return rt;
	}

	dyn<float>& asBlock()
	{
		static_assert(std::is_same<T, float>(), "not a float dyn");
		return *reinterpret_cast<dyn<float>*>(this);
	}

	dyn<float>& operator=(float s)
	{
		FloatVectorOperations::fill((float*)begin(), s, size());
		return asBlock();
	}

	template <typename OtherContainer> dyn<float>& operator=(OtherContainer& other)
	{
        static_assert(std::is_same<typename OtherContainer::DataType, float>(), "not a float container");

		// If you hit one of those, you probably wanted
		// to refer the data. Use referTo instead!
		jassert(other.size() > 0);
		jassert(size() > 0);
		jassert(begin() != nullptr);
		jassert(other.begin() != nullptr);

		memcpy(begin(), other.begin(), size() * sizeof(T));
		return *this;
	}


	dyn<float>& operator *=(float s)
	{
		FloatVectorOperations::multiply((float*)begin(), s, size());
		return asBlock();
	}

	dyn<float>& operator *=(const dyn<float>& other)
	{
		FloatVectorOperations::multiply((float*)begin(), other.begin(), size());
		return asBlock();
	}

	dyn<float>& operator +=(float s)
	{
		FloatVectorOperations::add((float*)begin(), s, size());
		return asBlock();
	}

	dyn<float>& operator +=(const dyn<float>& other)
	{
		FloatVectorOperations::add((float*)begin(), other.begin(), size());
		return asBlock();
	}

	dyn<float>& operator -=(float s)
	{
		FloatVectorOperations::add((float*)begin(), -1.0f * s, size());
		return asBlock();
	}

	dyn<float>& operator -=(const dyn<float>& other)
	{
		FloatVectorOperations::subtract((float*)begin(), other.begin(), size());
		return asBlock();
	}

	dyn<float>& operator +(float s)					{ return *this += s; }
	dyn<float>& operator +(const dyn<float>& other) { return *this += other; }
	dyn<float>& operator *(float s)					{ return *this *= s; }
	dyn<float>& operator *(const dyn<float>& other) { return *this *= other; }
	dyn<float>& operator -(float s)					{ return *this -= s; }
	dyn<float>& operator -(const dyn<float>& other) { return *this -= other; }

	template <int NumChannels> span<dyn<T>, NumChannels> split()
	{
		span<dyn<T>, NumChannels> r;

		int newSize = size() / NumChannels;

		T* d = data;

		for (auto& e : r)
		{
			e = dyn<T>(d, newSize);
			d += newSize;
		}

		return r;
	}

	void clear()
	{
		for (auto& s : *this)
			s = DataType();
	}


	template<typename IndexType> typename std::enable_if<index::Helpers::canReturnReference<IndexType>(), const T&>::type
		operator[](const IndexType& t) const
	{
		if constexpr (std::is_integral<IndexType>())
		{
			jassert(isPositiveAndBelow((int)t, size()));
			return data[t];
		}
		else
		{
			return t.getFrom(*this);
		}
	}

	template<typename IndexType> typename std::enable_if<!index::Helpers::canReturnReference<IndexType>(), T>::type
		operator[](const IndexType& t) const
	{
		return t.getFrom(*this);
	}

	template<typename IndexType> typename std::enable_if<!index::Helpers::canReturnReference<IndexType>(), T>::type
		operator[](const IndexType& t)
	{
		return t.getFrom(*this);
	}

	template<typename IndexType> typename std::enable_if<index::Helpers::canReturnReference<IndexType>(), T&>::type
		operator[](const IndexType& t)
	{
		if constexpr (std::is_integral<IndexType>())
		{
			jassert(isPositiveAndBelow((int)t, size()));
			return data[t];
		}
		else
		{
			return *const_cast<T*>(&t.getFrom(*this));
		}
	}

































#if 0
	template <class InterpolatorType> DataType interpolate(const InterpolatorType& i) const
	{
		if (isEmpty())
			return DataType(0);

		return i.getFrom(*this);
	}

	template <class IndexType> const T& operator[](const IndexType& t) const
	{
		jassert(!isEmpty());

		if constexpr (std::is_integral<IndexType>())
		{
			jassert(isPositiveAndBelow((int)t, size()));
			return data[t];
		}
		else
		{
			return t.getFrom(*this);
		}
	}

	template <class IndexType> T& operator[](const IndexType& t)
	{
		if constexpr (std::is_integral<IndexType>())
		{
			jassert(!isEmpty());
			jassert(isPositiveAndBelow((int)t, size()));
			return data[t];
		}
		else
		{
			return *const_cast<T*>(&t.getFrom(*this));
		}
	}
#endif

	template <class Other> bool valid(Other& t)
	{
		return t.valid(*this);
	}

	T* begin() const
	{
		return const_cast<T*>(data);
	}

	T* end() const
	{
		return const_cast<T*>(data) + size();
	}

	bool isSimdable() const
	{
		return reinterpret_cast<uint64_t>(begin()) % 16 == 0;
	}

	bool isEmpty() const noexcept { return size() == 0; }

	/** Returns the size of the array. Be aware that this is not a compile time constant. */
	int size() const noexcept { return size_; }

	/** Refers to a given container. */
	template <typename OtherContainer> void referTo(OtherContainer& t, int newSize=-1, int offset=0)
	{
		unused = Types::ID::Block;
		newSize = newSize >= 0 ? newSize : t.size();
		referToRawData(t.begin(), newSize, offset);
	}

	void referToNothing()
	{
		unused = Types::ID::Block;
		data = nullptr;
		size_ = 0;
	}

	/** Refers to a raw data pointer. */
	void referToRawData(T* newData, int newSize, int offset = 0)
	{
		unused = Types::ID::Block;
		jassert(newSize != 0);

		data = newData + offset;
		size_ = newSize;
	}

	template <typename OtherContainer> void copyTo(OtherContainer& t)
	{
		jassert(size() <= t.size());
		int numBytesToCopy = size() * sizeof(T);
		memcpy(t.begin(), begin(), numBytesToCopy);
	}

	template <typename OtherContainer> void copyFrom(const OtherContainer& t)
	{
		jassert(size() >= t.size());
		int numBytesToCopy = t.size() * sizeof(T);
		memcpy(begin(), t.begin(), numBytesToCopy);
	}

	int unused = Types::ID::Block;
	int size_ = 0;
	T* data;

};


template <typename T> struct heap
{
	static constexpr ArrayID ArrayType = Types::ArrayID::HeapType;
	using Type = heap<T>;
	using DataType = T;

	static constexpr bool hasCompileTimeSize() { return false; }

	int size() const noexcept { return size_; }

	bool isEmpty() const noexcept { return size() == 0; }

	void setSize(int numElements)
	{
		if (numElements != size())
		{
			if(numElements == 0)
				data.free();
			else
				data.allocate(numElements, true);

			size_ = numElements;
		}
	}

	T& operator[](int index)
	{
		return *(begin() + index);
	}

	const T& operator[](int index) const
	{
		return *(begin() + index);
	}

	T* begin() const { return data.get();  }
	T* end() const { return data + size(); }

	template <typename OtherContainer> void copyTo(OtherContainer& t)
	{
		jassert(size() <= t.size());
		int numBytesToCopy = size() * sizeof(T);
		memcpy(t.begin(), begin(), numBytesToCopy);
	}

	template <typename OtherContainer> void copyFrom(const OtherContainer& t)
	{
		jassert(size() >= t.size());
		int numBytesToCopy = t.size() * sizeof(T);
		memcpy(begin(), t.begin(), numBytesToCopy);
	}

	int unused = Types::ID::Block;
	int size_ = 0;
	juce::HeapBlock<T> data;
};

}
    
using block = Types::dyn<float>;

    
}

#endif
