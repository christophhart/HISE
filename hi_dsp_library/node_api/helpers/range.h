/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

/** This namespace contains template classes that define compile-time range objects which can be passed
	into parameter connections. It has:

	- Identity - no conversion
	- SNEX Expression range - conversion using a custom formula
	- Complex range conversions with skew factor and step sizes

	You will most likely use the DECLARE_XXX macros instead of using the classes directly for better readability.
*/
namespace ranges
{
using namespace snex;




/** The base class for a connection with a custom SNEX expression.

	You will most likely never use this class directly, but create the parameter
	using the

	DECLARE_PARAMETER_EXPRESSION()

	macro that just contains the formula.
*/
struct SnexExpressionBase
{
	static constexpr bool isRange() { return true; }

	/** Allows `Math.min(a, b)` syntax... */
	static hmath Math;

	/** There's no range conversion going on so we return the identity. */
	static NormalisableRange<double> createNormalisableRange() { return NormalisableRange<double>(0.0, 1.0); };
};


/** @internal A helper class that contains the actual range calculations.

	It is used by the various range macro definitions below to create classes with compile time range conversions. */
template <typename T> struct RangeBase
{
	static constexpr T from0To1(T min, T max, T value) { return hmath::map(value, min, max); }
	static constexpr T to0To1(T min, T max, T value) { return (value - min) / (max - min); }

	static constexpr T from0To1Skew(T min, T max, T skew, T value) 
	{ 
		return min + (max - min) * hmath::exp(hmath::log(value) / skew);
	}

	static constexpr T to0To1Skew(T min, T max, T skew, T value) 
	{
		return hmath::pow(to0To1(min, max, value), skew);
	}

	static constexpr T to0To1Step(T min, T max, T step, T value) { return to0To1(min, max, value - hmath::fmod(value, step)); }
	static constexpr T to0To1StepSkew(T min, T max, T step, T skew, T value) { return to0To1(min, max, skew, value - hmath::fmod(value, step)); }

	static constexpr T from0To1Step(T min, T max, T step, T value)
	{
		auto v = from0To1(min, max, value);
		return v - hmath::fmod(v, step);
	}
	static constexpr T from0To1StepSkew(T min, T max, T step, T skew, T value)
	{
		auto v = from0To1(min, max, skew, value);
		return v - hmath::fmod(v, step);
	}
};

/** A range without any conversion.*/
struct Identity
{
	static constexpr bool isRange() { return true; }
	static constexpr double from0To1(double input) { return input; };
	static constexpr double to0To1(double input) { return input; };
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(0.0, 1.0); };
};

struct InvertedIdentity
{
	static constexpr bool isRange() { return true; }
	static constexpr double from0To1(double input) { return 1.0 - input; };
	static constexpr double to0To1(double input) { return 1.0 - input; };
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(0.0, 1.0).inverted(); };
};


/** Declares a identity range. */
#define DECLARE_IDENTITY_RANGE(name) struct name: public Identity {};

/** A shortcut to declare an expression parameter */
#define DECLARE_PARAMETER_EXPRESSION(name, x) struct name : public scriptnode::ranges::SnexExpressionBase { \
static double op(double input) { return x; } }; 

#define RANGE_BASE scriptnode::ranges::RangeBase<double>

/** Declares a linear range with a custom range. */
#define DECLARE_PARAMETER_RANGE(name, min, max) struct name { \
	static constexpr bool isRange() { return true; } \
	static constexpr double to0To1(double input) { return RANGE_BASE::to0To1(min, max, input); } \
	static constexpr double from0To1(double input){ return RANGE_BASE::from0To1(min, max, input); };\
	static constexpr std::array<double, 2> getSimpleRange() { return { (double)min, (double)max }; } \
	static InvertableParameterRange createNormalisableRange() { return {min, max}; } };

#define DECLARE_PARAMETER_RANGE_INV(name, min, max) struct name { \
	static constexpr bool isRange() { return true; } \
	static constexpr double to0To1(double input) { return 1.0 - RANGE_BASE::to0To1(min, max, input); } \
	static constexpr double from0To1(double input){ return RANGE_BASE::from0To1(min, max, 1.0 - input); };\
	static constexpr std::array<double, 2> getSimpleRange() { return { (double)min, (double)max }; } \
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(min, max).inverted(); } };

/** Declares a linear range with discrete steps. */
#define DECLARE_PARAMETER_RANGE_STEP(name, min, max, step) struct name {\
	static constexpr bool isRange() { return true; } \
	static constexpr double to0To1(double input) {  return RANGE_BASE::to0To1Step(min, max, step, input); } \
	static constexpr double from0To1(double input){ return RANGE_BASE::from0To1Step(min, max, step, input);} \
	static constexpr std::array<double, 2> getSimpleRange() { return { (double)min, (double)max }; } \
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(min, max, step); } };

#define DECLARE_PARAMETER_RANGE_STEP_INV(name, min, max, step) struct name {\
	static constexpr bool isRange() { return true; } \
	static constexpr double to0To1(double input) {  return 1.0 - RANGE_BASE::to0To1Step(min, max, step, input); } \
	static constexpr double from0To1(double input){ return RANGE_BASE::from0To1Step(min, max, step, 1.0 - input);} \
	static constexpr std::array<double, 2> getSimpleRange() { return { (double)min, (double)max }; } \
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(min, max, step).inverted(); } };

/** Declares a skewed range with a settable skew factor. */
#define DECLARE_PARAMETER_RANGE_SKEW(name, min, max, skew) struct name {\
	static constexpr bool isRange() { return true; } \
	static constexpr double to0To1(double input) {  return RANGE_BASE::to0To1Skew(min, max, skew, input); }\
	static constexpr double from0To1(double input){ return RANGE_BASE::from0To1Skew(min, max, skew, input);} \
	static constexpr std::array<double, 2> getSimpleRange() { return { (double)min, (double)max }; } \
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(min, max, 0.0, skew); } };

#define DECLARE_PARAMETER_RANGE_SKEW_INV(name, min, max, skew) struct name {\
	static constexpr bool isRange() { return true; } \
	static constexpr double to0To1(double input) {  return 1.0 - RANGE_BASE::to0To1Skew(min, max, skew, input); }\
	static constexpr double from0To1(double input){ return RANGE_BASE::from0To1Skew(min, max, skew, 1.0 - input);} \
	static constexpr std::array<double, 2> getSimpleRange() { return { (double)min, (double)max }; } \
	static InvertableParameterRange createNormalisableRange() { return InvertableParameterRange(min, max, 0.0, skew).inverted(); } };

}

}
