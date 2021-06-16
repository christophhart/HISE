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
    
namespace Types {
using namespace juce;

#ifndef SNEX_WRAP_ALL_NEGATIVE_INDEXES
#define SNEX_WRAP_ALL_NEGATIVE_INDEXES 1
#endif

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
			value = t.getWithDynamicLimit(v, LogicType::getUpperLimit());

		return *this;
	}

	// Preinc
	integer_index& operator++()
	{
		if constexpr (checkBoundsOnAssign())
			value = t.getWithDynamicLimit(++value, LogicType::getUpperLimit());
		else
			++value;

		return *this;
	}

	// Postinc will always return a checked index
	int operator++(int)
	{
		if constexpr (checkBoundsOnAssign())
		{
			auto v = value;
			value = t.getWithDynamicLimit(v + 1, LogicType::getUpperLimit());
			return v;
		}
		else
		{
			auto v = t.getWithDynamicLimit(value, LogicType::getUpperLimit());
			++value;
			return v;
		}
	}

	// Preinc
	integer_index& operator--()
	{
		if constexpr (checkBoundsOnAssign())
			value = t.getWithDynamicLimit(--value, LogicType::getUpperLimit());
		else
			--value;

		return *this;
	}

	// Postinc
	Type operator--(int)
	{
		if constexpr (checkBoundsOnAssign())
		{
			auto v = value;
			value = t.getWithDynamicLimit(v - 1, LogicType::getUpperLimit());
			return v;
		}
		else
		{
			auto v = t.getWithDynamicLimit(value, LogicType::getUpperLimit());
			--value;
			return v;
		}
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

	explicit operator int() const
	{
		static_assert(!LogicType::hasDynamicBounds(), "can't use int operator with dynamic indexes");

		if constexpr (checkBoundsOnAssign())
			return value;

		return t.getWithDynamicLimit(value, LogicType::getUpperLimit());
	}

	template <typename ContainerType> auto& getFrom(const ContainerType& c) const
	{
		auto idx = value;

		if constexpr (!checkBoundsOnAssign() && LogicType::hasBoundCheck())
		{
			// We need to pass in at least 1 into the function or the wrapping might
			// lead to a division by zero exception for empty dynamic containers
			idx = t.getWithDynamicLimit(idx, jmax(1, c.size()));
		}

		return t.getRedirected(c, idx);
	}

	LogicType& getLogicType()
	{
		return t;
	}

	const LogicType& getLogicType() const
	{
		return t;
	}

	void setLoopRange(int start, int end)
	{
		getLogicType().setLoopRange({ start, end });
	}

private:

	Type value = Type(0);
	LogicType t;
};




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
		jassert(!hasDynamicBounds() || limit > 0);

		auto scaled = (int)from0To1(v, limit) + delta;
		return t.getWithDynamicLimit(scaled, jmax(1, limit));
	}

	float_index& operator=(FloatType v_)
	{
		v = v_;

		if constexpr (checkBoundsOnAssign())
		{
			auto scaled = from0To1(v, 0);
			auto checked = t.getWithDynamicLimit(scaled, FloatType(0));
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

	explicit operator FloatType() const
	{
		static_assert(!LogicType::hasDynamicBounds(), "can't use int operator with dynamic indexes");

		if constexpr (checkBoundsOnAssign())
			return v;
		else
		{
			auto scaled = from0To1(v, 0);
			auto limit = (FloatType)LogicType::getUpperLimit();
			return t.getWithDynamicLimit(scaled, limit);
		}
	}

	template <typename ContainerType> auto& getFrom(const ContainerType& c) const
	{
		auto idx = getIndex(c.size());
		return t.getRedirected(c, idx);
	}

	LogicType& getLogicType()
	{
		return t;
	}

	const LogicType& getLogicType() const
	{
		return t;
	}

	void setLoopRange(int start, int end)
	{
		getLogicType().setLoopRange({ start, end });
	}

private:

	LogicType t;
	FloatType v = FloatType(0);

	FloatType from0To1(FloatType v, int limit) const
	{
		if constexpr (IsNormalised)
		{
			if constexpr (!hasDynamicBounds())
				return v * (getUpperLimit());
			else
				return v * (FloatType)(limit);
		}
		else
			return v;
	}

	FloatType to0To1(FloatType v, int limit) const
	{
		if constexpr (IsNormalised)
		{
			if constexpr (!hasDynamicBounds())
				return v * (FloatType(1) / (getUpperLimit()));
			else
				return v / (FloatType)(limit);
		}
		else
			return v;
	}

	
};




template <typename IndexType> struct interpolator_base
{
	static constexpr bool canReturnReference() { return false; }

	static constexpr bool redirectOnFailure() { return IndexType::LogicType::redirectOnFailure(); }

	static constexpr bool hasBoundCheck() { return IndexType::hasBoundCheck(); };

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
		return interpolator_base((FloatType)idx + t);
	}

	interpolator_base operator-(FloatType t) const
	{
		return interpolator_base((FloatType)idx - t);
	}

	void setLoopRange(int start, int end)
	{
		getLogicType().setLoopRange({ start, end });
	}

	const LogicType& getLogicType() const
	{
		return idx.getLogicType();
	}

	LogicType& getLogicType()
	{
		return idx.getLogicType();
	}

	explicit operator Type() const
	{
		return (Type)idx;
	}

	IndexType idx;
};

template <typename IndexType> struct lerp: public interpolator_base<IndexType>
{
    using LogicType = typename IndexType::LogicType;
    using Type = typename IndexType::Type;
    
	lerp(Type initValue=Type(0)):
		interpolator_base<IndexType>(initValue)
	{}

	lerp& operator=(Type v)
	{
		idx = v;
		return *this;
	}

	static String toString()
	{
		String s;
		s << "index::lerp";
		s << "<" << IndexType::toString() << ">";
		return s;
	}

	template <typename ContainerType> auto getFrom(const ContainerType& t) const
	{
		auto i1 = this->idx.getIndex(t.size(), 0);
		auto i2 = this->idx.getIndex(t.size(), 1);
		auto alpha = this->idx.getAlpha(t.size());

		return getFromImpl(t, i1, i2, alpha, std::is_floating_point<typename ContainerType::DataType>());
	}

	

private:

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i1, int i2, Type alpha, std::true_type) const
	{
		using ElementType = typename ContainerType::DataType;

		const LogicType& lt = this->getLogicType();

        auto v1 = lt.getRedirected(t, i1);
        auto v2 = lt.getRedirected(t, i2);

		return interpolate(v1, v2, (ElementType)alpha);
	}

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i1, int i2, Type alpha, std::false_type) const
	{
		using ElementType = typename ContainerType::DataType;

		ElementType d;

		index::unsafe<0> j;

		for (auto& s : d)
		{
			auto x0 = t[i1][j];
			auto x1 = t[i2][j];
			++j;

			s = interpolate(x0, x1, (typename ElementType::DataType)alpha);
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
    using LogicType = typename IndexType::LogicType;
    using Type = typename IndexType::Type;
    
	hermite(Type initValue = Type(0)) :
		interpolator_base<IndexType>(initValue)
	{}

	hermite& operator=(Type v)
	{
		idx = v;
		return *this;
	}

	static String toString()
	{
		String s;
		s << "index::hermite";
		s << "<" << IndexType::toString() << ">";
		return s;
	}

	template <typename ContainerType> auto getFrom(const ContainerType& t) const
	{
		auto i0 = this->idx.getIndex(t.size(), -1);
		auto i1 = this->idx.getIndex(t.size(), 0);
		auto i2 = this->idx.getIndex(t.size(), 1);
		auto i3 = this->idx.getIndex(t.size(), 2);
		auto alpha = this->idx.getAlpha(t.size());

		return getFromImpl(t, i0, i1, i2, i3, alpha, std::is_floating_point<typename ContainerType::DataType>());
	}

private:

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i0, int i1, int i2, int i3, Type alpha, std::true_type) const
	{
		using ElementType = typename ContainerType::DataType;

		const auto v0 = this->getLogicType().getRedirected(t, i0);
		const auto v1 = this->getLogicType().getRedirected(t, i1);
		const auto v2 = this->getLogicType().getRedirected(t, i2);
		const auto v3 = this->getLogicType().getRedirected(t, i3);

		return interpolate(v0, v1, v2, v3, (ElementType)alpha);
	}

	template <typename ContainerType> auto getFromImpl(const ContainerType& t, int i0, int i1, int i2, int i3, Type alpha, std::false_type) const
	{
		using ElementType = typename ContainerType::DataType;

		ElementType d;
		index::unsafe<0> j;

		for(auto& s: d)
		{
			const auto v0 = t[i0][j];
			const auto v1 = t[i1][j];
			const auto v2 = t[i2][j];
			const auto v3 = t[i3][j];

			++j;
			s = interpolate(v0, v1, v2, v3, (typename ElementType::DataType)alpha);
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

}
}
