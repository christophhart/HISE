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

namespace index
{
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
		if constexpr (hasDynamicBounds())
			return hmath::wrap(input, limit);
		else
			return hmath::wrap(input, (T)UpperLimit);
	}

	template <typename ContainerType> static auto& getRedirected(ContainerType& c, int index)
	{
		jassert(isPositiveAndBelow(index, c.size()));
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
			return jlimit<T>(T(0), limit - T(1), input);
		else
			return jlimit<T>(T(0), T(getUpperLimit() - 1), input);
	}

	template <typename ContainerType> static auto& getRedirected(ContainerType& c, int index)
	{
		jassert(isPositiveAndBelow(index, c.size()));
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

	template <typename ContainerType> static auto& getRedirected(const ContainerType& c, int index)
	{
		jassert(isPositiveAndBelow(index + Offset, c.size()));
		return *(c.begin() + (index + Offset));
	}
};

template <int UpperLimit> struct looped_logic
{
	using LogicType = looped_logic;

	using WrapType = wrapped_logic<0>;

	static constexpr bool hasBoundCheck() { return true; };
	static constexpr bool hasDynamicBounds() { return UpperLimit == 0; }
	static constexpr int getUpperLimit() { return UpperLimit; }

	static String toString() { return "looped"; };

	template <typename T> T getWithDynamicLimit(T input, T limit) const
	{
		if constexpr (!hasDynamicBounds())
			limit = (T)getUpperLimit();

		jassert(length == 0 || T(length + start) <= limit);

		if (input < start)
			return hmath::max(T(0), input);

		auto v = hmath::wrap(input - (T)start, length != 0 ? (T)length : limit) + start;
		jassert(v >= T(0));
		return v;
	}

	template <typename ContainerType> auto& getRedirected(const ContainerType& c, int index) const
	{
		jassert(isPositiveAndBelow(index, c.size()));
		return *(c.begin() + (index));
	}

	void setLoopRange(Range<int> r)
	{
		start = r.getStart();
		length = r.getLength();
	}

private:

	int start = 0;
	int length = 0;
};


template <int UpperLimit, bool CheckOnAssign = false> using wrapped = integer_index<wrapped_logic<UpperLimit>, CheckOnAssign>;
template <int UpperLimit, bool CheckOnAssign = false> using clamped = integer_index<clamped_logic<UpperLimit>, CheckOnAssign>;
template <int UpperLimit, bool CheckOnAssign = false> using unsafe = integer_index<unsafe_logic<UpperLimit, 0>, false>;
template <int UpperLimit, bool CheckOnAssign = false> using previous = integer_index<unsafe_logic<UpperLimit, -1>, false>;
template <int UpperLimit, bool CheckOnAssign = false> using looped = integer_index<looped_logic<UpperLimit>, CheckOnAssign>;
template <typename FloatType, typename IntegerIndexType> using normalised = float_index<FloatType, IntegerIndexType, true>;
template <typename FloatType, typename IntegerIndexType> using unscaled = float_index<FloatType, IntegerIndexType, false>;
}

}
}
