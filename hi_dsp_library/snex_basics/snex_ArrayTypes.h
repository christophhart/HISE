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
#include <nmmintrin.h>
#include <stdint.h>

namespace snex {
namespace Types {


struct DSP
{
	template <class WrapType, class T> static float interpolate(T& data, float v)
	{
		static_assert(std::is_same<T, typename WrapType::ParentType>(), "parent type mismatch ");

		auto floorValue = (int)v;

		WrapType lower = { (int)v };
		WrapType upper = { lower.value + 1 };

		int lowerIndex = lower.get(data);
		int upperIndex = upper.get(data);

		auto ptr = data.begin();
		auto lowerValue = ptr[lowerIndex];
		auto upperValue = ptr[upperIndex];
		auto alpha = v - (float)floorValue;
		const float invAlpha = 1.0f - alpha;
		return invAlpha * lowerValue + alpha * upperValue;
	}
};

namespace index
{

struct Helpers
{
	template <typename C> static int getLowestBounds(const C& first)
	{
		return first.size();
	}

	template <typename C, typename... Cs> static int getLowestBounds(const C& first, const Cs&... others)
	{
		return jmin<int>(first.size(), getLowestBounds(others...));
	}

	template <typename IndexType> constexpr static bool canReturnReference()
	{
		if constexpr (std::is_integral<IndexType>())
			return true;
		else
			return IndexType::canReturnReference();
	}

	template <typename LogicType> static constexpr int getUpperLimitOrMaxInt()
	{
		constexpr int v = LogicType::getUpperLimit();
		return v == 0 ? INT_MAX : v;
	}
};



/** The wrapped index type can be used for an integer index that will wrap around
	the boundaries in order to implement eg. ring buffers.

	The template argument specifies the compile time boundary. If it is zero, you can
	use it dynamically with the getWithDynamicLimit() method

	You can either create them directly or use the IndexType helper class
	for a leaner syntax:

	@code
	span<float, 19> data = { 2.0f };

	span<float, 19>::wrapped idx2 = data;

	// Just pass the span into the function
	auto idx1 = IndexType::wrapped(data);
	@endcode
	*/
template <int UpperLimit> struct wrapped_logic
{
	using LogicType = wrapped_logic<UpperLimit>;

	static constexpr bool hasBoundCheck() { return true; };
	static constexpr bool hasDynamicBounds() { return UpperLimit == 0; }
	static constexpr int getUpperLimit() { return UpperLimit; }
	static constexpr bool redirectOnFailure() { return false; }

	static String toString() { return "wrapped"; };

	template <typename T> static T getWithDynamicLimit(T input, T limit)
	{
		if constexpr (std::is_floating_point<T>())
		{
			if constexpr (hasDynamicBounds())
				return hmath::fmod(input, limit);
			else
				return hmath::fmod(input, (T)UpperLimit);
		}
		else
		{
			if constexpr (hasDynamicBounds())
				return (input >= 0) ? input % limit : (limit - abs(input) % limit) % limit;
			else
				return (input >= 0) ? input % UpperLimit : (UpperLimit - abs(input) % UpperLimit) % UpperLimit;
		}
	}

	template <typename ContainerType> static auto& getRedirected(ContainerType& c, int index)
	{
		return *(c.begin() + index);
	}
};



/** An index type that will clamp the value to the limits, so that its zero for negative input and `size-1` for values outside the boundary.

		This is useful for look up tables etc.
*/
template <int UpperLimit> struct clamped_logic
{
	using LogicType = clamped_logic<UpperLimit>;

	static constexpr bool hasBoundCheck() { return true; };
	static constexpr bool hasDynamicBounds() { return UpperLimit == 0; }
	static constexpr int getUpperLimit() { return UpperLimit; }
	static constexpr bool redirectOnFailure() { return false; }

	static String toString() { return "clamped"; };

	template <typename T> static T getWithDynamicLimit(T input, T limit)
	{
		if constexpr (hasDynamicBounds())
			return hmath::range(input, T(0), limit - T(1));
		else
			return hmath::range(input, T(0), T(getUpperLimit() - 1));
	}

	template <typename ContainerType> static auto& getRedirected(ContainerType& c, int index)
	{
		return *(c.begin() + index);
	}
};

/** An index type that is not performing any bounds-check at all. Use it at your own risk! */
template <int UpperLimit, int Offset> struct unsafe_logic
{
	using LogicType = unsafe_logic;

	static constexpr bool hasBoundCheck() { return false; };
	static constexpr bool hasDynamicBounds() { return UpperLimit == 0; }
	static constexpr int getUpperLimit() { return UpperLimit; }

	static String toString() { return "unsafe"; };

	template <typename T> static T getWithDynamicLimit(T input, T limit)
	{
		ignoreUnused(limit);
		return input;
	}

	template <typename ContainerType> static auto& getRedirected(ContainerType& c, int index)
	{
		jassert(isPositiveAndBelow(index + Offset, c.size()));
		return *(c.begin() + (index + Offset));
	}
};

/** An index type that is not performing any bounds-check at all. Use it at your own risk! */
template <int UpperLimit> struct safe_logic
{
	using LogicType = safe_logic;

	static constexpr bool hasBoundCheck() { return false; };
	static constexpr bool hasDynamicBounds() { return UpperLimit == 0; }
	static constexpr int getUpperLimit() { return UpperLimit; }
	
	static String toString() { return "safe"; };

	template <typename T> static T getWithDynamicLimit(T input, T limit)
	{
		ignoreUnused(limit);
		return input;
	}

	template <typename ContainerType> static auto& getRedirected(ContainerType& c, int index)
	{
		using T = typename ContainerType::DataType;
		static T zeroValue = T();
		T* ptrs[2] = { &zeroValue, c.begin() + index };
		auto offset = (int)(index < c.size());
		return *ptrs[offset];
	}
};


/** The base class for integer indexes. */
template <typename LT, bool CheckOnAssign> struct integer_index
{
	using Type = int;
	using LogicType = LT;

	static constexpr bool checkBoundsOnAssign() { return !LogicType::hasDynamicBounds() && CheckOnAssign; }
	static constexpr bool canReturnReference() { return true; }
	static constexpr bool isExceptionSafe() { return LogicType::hasBoundCheck() || LogicType::redirectOnFailure(); }

	static String toString()
	{
		String s;
		s << "index::" << LogicType::toString();
		s << "<" << LogicType::getUpperLimit() << ", ";
		s << (checkBoundsOnAssign() ? "true" : "false") << ">";
		return s;
	}

	integer_index(Type initValue=Type(0))
	{
		*this = initValue;
	};

	integer_index& operator=(Type v)
	{ 
		value = v; 

		if constexpr (checkBoundsOnAssign())
			value = LogicType::getWithDynamicLimit(v, LogicType::getUpperLimit());

		return *this;
	}

	// Preinc
	integer_index& operator++()
	{
		if constexpr (checkBoundsOnAssign())
			value = LogicType::getWithDynamicLimit(++value, LogicType::getUpperLimit());
		else
			++value;

		return *this;
	}

	// Postinc will always return a checked index
	int operator++(int)
	{
		auto v = value;
		value = LogicType::getWithDynamicLimit(v + 1, LogicType::getUpperLimit());
		return v;
	}

	// Preinc
	integer_index& operator--()
	{
		if constexpr (checkBoundsOnAssign())
			value = LogicType::getWithDynamicLimit(--value, LogicType::getUpperLimit());
		else
			--value;

		return *this;
	}

	// Postinc
	Type operator--(int)
	{
		auto v = value;
		value = LogicType::getWithDynamicLimit(v - 1, LogicType::getUpperLimit());
		return v;
	}

	integer_index operator+(int delta) const
	{
		return integer_index(value + delta);
	}

	integer_index operator-(int delta) const
	{
		return integer_index(value - delta);
	}

	bool next() const
	{
		static_assert(LogicType::getUpperLimit() > 0, "can't check dynamic index without container arguments");
		static_assert(!LogicType::hasBoundCheck(), "wrapped indexes are always inside the boundaries");

		auto ok = isPositiveAndBelow(value, LogicType::getUpperLimit());
		operator++();
		return ok;
	}

	template <typename... Cs> bool next(const Cs&... containers)
	{
		static_assert(!LogicType::hasBoundCheck(), "wrapped indexes are always inside the boundaries");

		auto minSize = Helpers::getLowestBounds(containers...);

		if constexpr (LogicType::getUpperLimit() > 0)
			minSize = jmin(minSize, LogicType::getUpperLimit());

		auto ok = isPositiveAndBelow(value, minSize);
		operator++();
		return ok;
	}

	

	operator int() const
	{
		static_assert(!LogicType::hasDynamicBounds(), "can't use int operator with dynamic indexes");

		if constexpr (checkBoundsOnAssign())
			return value;

		return LogicType::getWithDynamicLimit(value, LogicType::getUpperLimit());
	}

	template <typename ContainerType> auto& getFrom(const ContainerType& c) const
	{
		auto idx = value;

		if constexpr (!checkBoundsOnAssign() && LogicType::hasBoundCheck())
			idx = LogicType::getWithDynamicLimit(idx, c.size());

		return LogicType::getRedirected(c, idx);
	}

private:

	Type value = Type(0);
};



template <int UpperLimit, bool CheckOnAssign=false> using wrapped = integer_index<wrapped_logic<UpperLimit>, CheckOnAssign>;
template <int UpperLimit, bool CheckOnAssign=false> using clamped = integer_index<clamped_logic<UpperLimit>, CheckOnAssign>;
template <int UpperLimit, bool CheckOnAssign=false> using unsafe = integer_index<unsafe_logic<UpperLimit, 0>, false>;

template <int UpperLimit, bool CheckOnAssign = false> using previous = integer_index<unsafe_logic<UpperLimit, -1>, false>;

template <int UpperLimit, bool CheckOnAssign = false> using safe = integer_index<safe_logic<UpperLimit>, false>;

template <typename FloatType, typename IntegerIndexType, bool IsNormalised> struct float_index
{
	using Type = FloatType;
	using LogicType = typename IntegerIndexType::LogicType;

	static constexpr bool canReturnReference() { return true; }

	static String toString()
	{
		String s;
		s << "index::" << (IsNormalised ? "normalised" : "unscaled");
		s << "<" << Types::Helpers::getTypeNameFromTypeId<Type>() << ", ";
		s << IntegerIndexType::toString() << ">";
		return s;
	}

	float_index(FloatType initValue = 0)
	{
		*this = initValue;
	}

	static constexpr bool hasDynamicBounds() { return LogicType::hasDynamicBounds(); }

	static constexpr Type getUpperLimit()
	{
		static_assert(!LogicType::hasDynamicBounds(), "can't use upper limit on dynamic bounds");
		return (FloatType)LogicType::getUpperLimit();
	}

	static constexpr bool hasBoundCheck() { return LogicType::hasBoundCheck(); };
	static constexpr bool checkBoundsOnAssign() { return !hasDynamicBounds() && IntegerIndexType::checkBoundsOnAssign(); }

	FloatType getAlpha(int limit) const
	{
		auto f = from0To1(v, limit);
		return f - (FloatType)((int)f);
	}

	int getIndex(int limit, int delta=0) const
	{
		auto scaled = (int)from0To1(v, limit) + delta;
		return LogicType::getWithDynamicLimit(scaled, limit);
	}

	float_index& operator=(FloatType t)
	{
		v = t;

		if constexpr (checkBoundsOnAssign())
		{
			auto scaled = from0To1(v, 0);
			auto checked = LogicType::getWithDynamicLimit(scaled, FloatType(0));
			v = to0To1(checked, 0);
		}

		return *this;
	}

	float_index operator+(FloatType t) const
	{
		return float_index(v + t);
	}

	float_index operator-(FloatType t) const
	{
		return float_index(v - t);
	}

	operator FloatType() const
	{
		static_assert(!LogicType::hasDynamicBounds(), "can't use int operator with dynamic indexes");

		if constexpr (checkBoundsOnAssign())
			return v;

		auto scaled = from0To1(v, 0);
		auto limit = (FloatType)LogicType::getUpperLimit();
		return LogicType::getWithDynamicLimit(scaled, limit);
	}

	template <typename ContainerType> auto& getFrom(const ContainerType& c) const
	{
		auto idx = getIndex(c.size());
		return LogicType::getRedirected(c, idx);
	}

private:

	FloatType v = FloatType(0);

	FloatType from0To1(FloatType v, int limit) const
	{
		if constexpr (IsNormalised)
		{
			if constexpr (!hasDynamicBounds())
				return v * getUpperLimit();
			else
				return v * (FloatType)limit;
		}
		else
			return v;
	}

	FloatType to0To1(FloatType v, int limit) const
	{
		if constexpr (IsNormalised)
		{
			if constexpr (!hasDynamicBounds())
				return v * (FloatType(1) / getUpperLimit());
			else
				return v / (FloatType)limit;
		}
		else
			return v;
	}
};

template <typename FloatType, typename IntegerIndexType> using normalised = float_index<FloatType, IntegerIndexType, true>;

template <typename FloatType, typename IntegerIndexType> using unscaled = float_index<FloatType, IntegerIndexType, false>;


template <typename IndexType> struct interpolator_base
{
	static constexpr bool canReturnReference() { return false; }

	static constexpr bool redirectOnFailure() { return IndexType::LogicType::redirectOnFailure(); }

	using LogicType = typename IndexType::LogicType;
	using Type = typename IndexType::Type;

	interpolator_base(Type initValue = Type(0)):
		idx(initValue)
	{
		static_assert(std::is_floating_point<Type>(), "index type must be float for linear interpolation");
	}

	interpolator_base& operator=(Type v)
	{
		idx = v;
		return *this;
	}

	interpolator_base operator+(FloatType t) const
	{
		return interpolator_base(v + t);
	}

	interpolator_base operator-(FloatType t) const
	{
		return interpolator_base(v - t);
	}

protected:

	IndexType idx;
};

template <typename IndexType> struct lerp: public interpolator_base<IndexType>
{
	lerp(Type initValue=Type(0)):
		interpolator_base<IndexType>(initValue)
	{}

	template <typename ContainerType> auto getFrom(const ContainerType& t) const
	{
		auto i1 = idx.getIndex(t.size(), 0);
		auto i2 = idx.getIndex(t.size(), 1);
		auto alpha = idx.getAlpha(t.size());

		return getFromImpl(t, i1, i2, alpha, std::is_floating_point<ContainerType::DataType>());
	}

private:

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i1, int i2, Type alpha, std::true_type) const
	{
		using ElementType = typename ContainerType::DataType;

		auto v1 = LogicType::getRedirected(t, i1);
		auto v2 = LogicType::getRedirected(t, i2);

		return interpolate(v1, v2, (ElementType)alpha);
	}

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i1, int i2, Type alpha, std::false_type) const
	{
		using ElementType = typename ContainerType::DataType;

		ElementType d;

		for(int i = 0; i < t.size(); j++)
		{
			auto v1 = LogicType::getRedirected(t, i1)[j];
			auto v2 = LogicType::getRedirected(t, i2)[j];
			
			d[j] = interpolate(v1, v2, (typename ElementType::DataType)alpha);
		}

		return d;
	}

	static Type interpolate(Type x0, Type x1, Type alpha)
	{
		return x0 + (x1 - x0) * alpha;
	}
};

template <typename IndexType> struct hermite : public interpolator_base<IndexType>
{
	hermite(Type initValue = Type(0)) :
		interpolator_base<IndexType>(initValue)
	{}

	template <typename ContainerType> auto getFrom(const ContainerType& t) const
	{
		auto i0 = idx.getIndex(t.size(), -1);
		auto i1 = idx.getIndex(t.size(), 0);
		auto i2 = idx.getIndex(t.size(), 1);
		auto i3 = idx.getIndex(t.size(), 2);
		auto alpha = idx.getAlpha(t.size());

		return getFromImpl(t, i0, i1, i2, i3, alpha, std::is_floating_point<ContainerType::DataType>());
	}

private:

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i0, int i1, int i2, int i3, Type alpha, std::true_type) const
	{
		using ElementType = typename ContainerType::DataType;

		const auto v0 = LogicType::getRedirected(t, i0);
		const auto v1 = LogicType::getRedirected(t, i1);
		const auto v2 = LogicType::getRedirected(t, i2);
		const auto v3 = LogicType::getRedirected(t, i3);

		return interpolate(v0, v1, v2, v3, (ElementType)alpha);
	}

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i0, int i1, int i2, int i3, Type alpha, std::false_type) const
	{
		using ElementType = typename ContainerType::DataType;

		ElementType d;

		for (int j = 0; j < d.size(); j++)
		{
			const auto v0 = LogicType::getRedirected(t, i0)[j];
			const auto v1 = LogicType::getRedirected(t, i1)[j];
			const auto v2 = LogicType::getRedirected(t, i2)[j];
			const auto v3 = LogicType::getRedirected(t, i3)[j];

			d[j] = interpolate(v0, v1, v2, v3, (typename ElementType::DataType)alpha);
		}

		return d;
	}

	static Type interpolate(Type x0, Type x1, Type x2, Type x3, Type alpha)
	{
		Type a = ((Type(3) * (x1 - x2)) - x0 + x3) * Type(0.5);
		Type b = x2 + x2 + x0 - (Type(5) *x1 + x3) * Type(0.5);
		Type c = (x2 - x0) * Type(0.5);
		return ((a*alpha + b)*alpha + c)*alpha + x1;
	};
};

}



/** A fixed-size array type for SNEX. 

	The span type is an iteratable compile-time array. The elements can be accessed 
	using the []-operator or via a range-based for loop.

	Note that the []-operator access can either take a literal integer index (that 
	is compiled-time checked against the boundaries), or a index subtype with a 
	defined out-of-bounds behaviour (wrapping, clamping, etc).

	A special version of the span is the span<float, 4> type, which will use optimized
	SSE instructions for common operations. Also any span type with floats and a 
	length that is a multiple of 4 will use SSE instructions when optimizing loops.
*/
template <class T, int Size> struct span
{
	static constexpr ArrayID ArrayType = Types::ArrayID::SpanType;
	using DataType = T;
	using Type = span<T, Size>;

	static constexpr int s = Size;

	using wrapped = index::wrapped<Size, false>;
	using clamped = index::clamped<Size, false>;

	struct zeroed
	{
		operator int() const
		{
			if (isPositiveAndBelow(this->value, Size - 1))
				return this->value;

			return 0;
		}
	};

	

	span()
	{
		clear();
	}

	span(const std::initializer_list<T>& l)
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
		if (isSimdable())
		{
			constexpr int numLoop = Size / getSimdSize();

			if (std::is_same<DataType, float>())
			{
				float t_ = (float)*l.begin();

				auto dst = (float*)data;

				auto v = _mm_load_ps1(&t_);

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
				v = *l.begin();
		}

		return *this;

	}

	Type& operator=(const T& t)
	{
		for (auto& v : *this)
			v = t;

		return *this;
	}

	operator T()
	{
		static_assert(Size == 1, "not a single elemnet span");
		return *begin();
	}

	Type& operator+=(const T& scalar)
	{
		return *this + scalar;
	}

	Type& operator+(const T& scalar)
	{
		static_assert(std::is_arithmetic<T>(), "not an arithmetic type");
		
		for (auto& s : *this)
			s += scalar;

		return *this;
	}

	Type& operator=(const Type& other)
	{
		memcpy(data, other.begin(), size() * sizeof(T));
		return *this;
	}

	Type& operator+ (const Type& other)
	{
		static_assert(std::is_arithmetic<T>(), "not an arithmetic type");

		for (int i = 0; i < size(); i++)
			data[i] += other.data[i];

		return *this;
	}

	Type& operator +=(const Type& other)
	{
		return *this + other;
	}

	constexpr bool isFloatType()
	{
		return std::is_same<float, T>() || std::is_same<double, T>();
	}

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
		return (std::is_same<T, float>() && Size == 4) ||
			(std::is_same<T, double>() && Size == 2);
	}

	static constexpr bool isSimdable()
	{
		return (std::is_same<T, float>() && Size % 4 == 0) ||
			(std::is_same<T, double>() && Size % 2 == 0);
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
		jassertfalse;
		return t.getFrom(*this);
	}

	template<typename IndexType> typename std::enable_if<!index::Helpers::canReturnReference<IndexType>(), T>::type
		operator[](const IndexType& t)
	{
		jassertfalse;
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
		return 16;
	}

	alignas(alignment()) T data[Size];
};



/** This alias is a special type on its own as it has mathematical operators that directly translate to SSE instructions. */
using float4 = span<float, 4>;

/** The dyn template is a typed array with a dynamic amount of elements. 
	
	The memory used by this type will be allocated on the heap and it can be resized to fit a new size limit.

*/
template <class T> struct dyn
{
	static constexpr ArrayID ArrayType = Types::ArrayID::DynType;
	using Type = dyn<T>;
	using DataType = T;

	using wrapped = index::wrapped<0, false>;
	using clamped = index::clamped<0, false>;
	using unsafe = index::unsafe<0, false>;

	dyn() :
		data(nullptr),
		size_(0)
	{}

	template<class Other> dyn(Other& o) :
		data(o.begin()),
		size_(o.end() - o.begin())
	{
        static_assert(std::is_same<DataType, typename Other::DataType>(), "not same data type");
	}

	template<class Other> dyn(Other& o, size_t s_) :
		data(o.begin()),
		size_(s_)
	{
        static_assert(std::is_same<DataType, typename Other::DataType>(), "not same data type");
	}

	dyn(juce::HeapBlock<T>& d, size_t s_):
		data(d.get()),
		size_(s_)
	{}

	dyn(T* data_, size_t s_) :
		data(data_),
		size_(s_)
	{}

	template<class Other> dyn(Other& o, size_t s_, size_t offset) :
		data(o.begin() + offset),
		size_(s_)
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
		jassert(!isEmpty());

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

	int size() const noexcept { return size_; }

	bool isEmpty() const noexcept { return size() == 0; }

	void setSize(int numElements)
	{
		if (numElements != size())
		{
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

namespace Interleaver
{


static void interleave(float* src, int numFrames, int numChannels)
{
	size_t numBytes = sizeof(float) * numChannels * numFrames;

	float* dst = (float*)alloca(numBytes);

	for (int i = 0; i < numFrames; i++)
	{
		for (int j = 0; j < numChannels; j++)
		{
			auto targetIndex = i * numChannels + j;
			auto sourceIndex = j * numFrames + i;

			dst[targetIndex] = src[sourceIndex];
		}
	}

	memcpy(src, dst, numBytes);
}


static void interleaveRaw(float* src, int numFrames, int numChannels)
{
	interleave(src, numFrames, numChannels);
}

template <class T, int NumChannels> static auto interleave(span<dyn<T>, NumChannels>& t)
{
	jassert(isContinousMemory(t));

	int numFrames = t[0].size;

	static_assert(std::is_same<float, T>(), "must be float");

	// [ [ptr, size], [ptr, size] ]
	// => [ ptr, size ] 

	using FrameType = span<T, NumChannels>;


	auto src = reinterpret_cast<float*>(t[0].begin());
	interleaveRaw(src, numFrames, NumChannels);

	return dyn<FrameType>(reinterpret_cast<FrameType*>(src), numFrames);
}

/** Interleaves the float data from a dynamic array of frames.

	dyn_span<T>(numChannels) => span<dyn_span<T>, NumChannels>
*/
template <class T, int NumChannels> static auto interleave(dyn<span<T, NumChannels>>& t)
{
	jassert(isContinousMemory(t));

	int numFrames = t.size;

	span<dyn<T>, NumChannels> d;

	float* src = t.begin()->begin();


	interleave(src, NumChannels, numFrames);

	for (auto& r : d)
	{
		r = dyn<T>(src, numFrames);
		src += numFrames;
	}

	return d;
}

template <class T, int NumFrames, int NumChannels> static auto& interleave(span<span<T, NumFrames>, NumChannels>& t)
{
	static_assert(std::is_same<float, T>(), "must be float");
	jassert(isContinousMemory(t));

	using ChannelType = span<T, NumChannels>;

	int s1 = sizeof(span<ChannelType, NumFrames>);
	int s2 = sizeof(span<span<T, NumFrames>, NumChannels>);



	auto src = reinterpret_cast<float*>(t.begin());
	interleave(src, NumFrames, NumChannels);
	return *reinterpret_cast<span<ChannelType, NumFrames>*>(&t);
}

template <class T> static bool isContinousMemory(const T& t)
{
	using ElementType = typename T::DataType;

	auto ptr = t.begin();
	auto e = t.end();
	auto elementSize = sizeof(ElementType);
	auto size = (e - ptr) * elementSize;
	auto realSize = reinterpret_cast<uint64_t>(e) - reinterpret_cast<uint64_t>(ptr);
	return realSize == size;
}


template <class DataType> static dyn<DataType> slice(const dyn<DataType>& src, int size = -1, int start=0)
{
	dyn<DataType> c;
	c.referTo(src, size, start);
	return c;
}

template <class DataType, int Size> static dyn<DataType> slice(const span<DataType, Size>& src, int size = -1, int start=0)
{
	dyn<DataType> d;
	d.referToRawData(src.begin(), size, start);
	return d;
}


};




}
}

#endif
