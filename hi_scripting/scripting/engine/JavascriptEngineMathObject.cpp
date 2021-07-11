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

namespace hise { using namespace juce;

//==============================================================================
/** The Math Object. */
struct HiseJavascriptEngine::RootObject::MathClass : public ApiClass
{
	MathClass() :
	ApiClass(2)
	{
		ADD_API_METHOD_1(abs);
		ADD_API_METHOD_1(round);
		ADD_API_METHOD_0(random);
		ADD_API_METHOD_2(randInt);
		ADD_API_METHOD_2(min);
		ADD_API_METHOD_2(max);
		ADD_API_METHOD_3(range);
		ADD_API_METHOD_1(sign);
		ADD_API_METHOD_1(toDegrees);
		ADD_API_METHOD_1(toRadians);
		ADD_API_METHOD_1(sin);
		ADD_API_METHOD_1(asin);
		ADD_API_METHOD_1(sinh);
		ADD_API_METHOD_1(asinh);
		ADD_API_METHOD_1(cos);
		ADD_API_METHOD_1(acos);
		ADD_API_METHOD_1(cosh);
		ADD_API_METHOD_1(acosh);
		ADD_API_METHOD_1(tan);
		ADD_API_METHOD_1(atan);
		ADD_API_METHOD_1(tanh);
		ADD_API_METHOD_1(atanh);
		ADD_API_METHOD_1(log);
		ADD_API_METHOD_1(log10);
		ADD_API_METHOD_1(exp);
		ADD_API_METHOD_2(pow);
		ADD_API_METHOD_1(sqr);
		ADD_API_METHOD_1(sqrt);
		ADD_API_METHOD_1(ceil);
		ADD_API_METHOD_1(floor);
		ADD_API_METHOD_2(fmod);
		ADD_API_METHOD_2(wrap);

		addConstant("PI", double_Pi);
		addConstant("E", exp(1.0));
		addConstant("SQRT2", sqrt(2.0));
		addConstant("SQRT1_2", sqrt(0.5));
		addConstant("LN2", log(2.0));  
		addConstant("LN10", log(10.0));
		addConstant("LOG2E", std::log2((double)exp(1.0)));
		addConstant("LOG10E", log10(exp(1.0)));
	}

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Math"); }

	struct Wrapper
	{
		API_METHOD_WRAPPER_1(MathClass, abs);
		API_METHOD_WRAPPER_1(MathClass, round);
		API_METHOD_WRAPPER_0(MathClass, random);
		API_METHOD_WRAPPER_2(MathClass, randInt);
		API_METHOD_WRAPPER_2(MathClass, min);
		API_METHOD_WRAPPER_2(MathClass, max);
		API_METHOD_WRAPPER_3(MathClass, range);
		API_METHOD_WRAPPER_1(MathClass, sign);
		API_METHOD_WRAPPER_1(MathClass, toDegrees);
		API_METHOD_WRAPPER_1(MathClass, toRadians);
		API_METHOD_WRAPPER_1(MathClass, sin);
		API_METHOD_WRAPPER_1(MathClass, asin);
		API_METHOD_WRAPPER_1(MathClass, sinh);
		API_METHOD_WRAPPER_1(MathClass, asinh);
		API_METHOD_WRAPPER_1(MathClass, cos);
		API_METHOD_WRAPPER_1(MathClass, acos);
		API_METHOD_WRAPPER_1(MathClass, cosh);
		API_METHOD_WRAPPER_1(MathClass, acosh);
		API_METHOD_WRAPPER_1(MathClass, tan);
		API_METHOD_WRAPPER_1(MathClass, atan);
		API_METHOD_WRAPPER_1(MathClass, tanh);
		API_METHOD_WRAPPER_1(MathClass, atanh);
		API_METHOD_WRAPPER_1(MathClass, log);
		API_METHOD_WRAPPER_1(MathClass, log10);
		API_METHOD_WRAPPER_1(MathClass, exp);
		API_METHOD_WRAPPER_2(MathClass, pow);
		API_METHOD_WRAPPER_1(MathClass, sqr);
		API_METHOD_WRAPPER_1(MathClass, sqrt);
		API_METHOD_WRAPPER_1(MathClass, ceil);
		API_METHOD_WRAPPER_1(MathClass, floor);
		API_METHOD_WRAPPER_2(MathClass, fmod);
		API_METHOD_WRAPPER_2(MathClass, wrap);
	};

	/** Returns a random number between 0.0 and 1.0. */
	var random()
	{
		return Random::getSystemRandom().nextDouble();
	}

	/** Returns a random integer between the low and the high values. */
	var randInt(var low, var high)
	{
		return Random::getSystemRandom().nextInt(Range<int>((int)low, (int)high));
	}

	/** Returns the absolute (unsigned) value. */
	var abs(var value)
	{
		return value.isInt() ? var(std::abs((int)value)) :
			var(std::abs((double)value));
	}

	/** Rounds the value to the next integer. */
	var round(var value)
	{
		return value.isInt() ? var(roundToInt((int)value)) :
			var(roundToInt((double)value));
	}

	/** Returns the sign of the value. */
	var sign(var value)
	{
		return value.isInt() ? var(sign_((int)value)) :
			var(sign_((double)value));
	}

	/** Limits the value to the given range. */
	var range(var value, var lowerLimit, var upperLimit)
	{
		return value.isInt() ? var(jlimit<int>(lowerLimit, upperLimit, value)) :
			var(jlimit<double>(lowerLimit, upperLimit, value));
	}

	/** Returns the smaller number. */
	var min(var first, var second)
	{
		return (first.isInt() && second.isInt()) ? var(jmin((int)first, (int)second)) :
			var(jmin((double)first, (double)second));
	}

	/** Returns the bigger number. */
	var max(var first, var second)
	{
		return (first.isInt() && second.isInt()) ? var(jmax((int)first, (int)second)) :
			var(jmax((double)first, (double)second));
	}

	/** Converts radian (0...2*PI) to degree (0...360�). */
	var toDegrees(var value) { return radiansToDegrees((double)value); }

	/** Converts degree  (0...360�) to radian (0...2*PI). */
	var toRadians(var value) { return degreesToRadians((double)value); }

	/** Calculates the sine value (radian based). */
	var sin(var value) { return std::sin((double)value); }

	/** Calculates the asine value (radian based). */
	var asin(var value) { return std::asin((double)value); }

	/** Calculates the cosine value (radian based). */
	var cos(var value) { return std::cos((double)value); }

	/** Calculates the acosine value (radian based). */
	var acos(var value) { return std::acos((double)value); }

	/** Calculates the sinh value (radian based). */
	var sinh(var value) { return std::sinh((double)value); }

	/** Calculates the asinh value (radian based). */
	var asinh(var value) { return std::asinh((double)value); }

	/** Calculates the cosh value (radian based). */
	var cosh(var value) { return std::cosh((double)value); }

	/** Calculates the acosh value (radian based). */
	var acosh(var value) { return std::acosh((double)value); }

	/** Calculates the tan value (radian based). */
	var tan(var value) { return std::tan((double)value); }

	/** Calculates the tanh value (radian based). */
	var tanh(var value) { return std::tanh((double)value); }

	/** Calculates the atan value (radian based). */
	var atan(var value) { return std::atan((double)value); }

	/** Calculates the atanh value (radian based). */
	var atanh(var value) { return std::atanh((double)value); }

	/** Calculates the log value (with base E). */
	var log(var value) { return std::log((double)value); }

	/** Calculates the log value (with base 10). */
	var log10(var value) { return std::log10((double)value); }

	/** Calculates the exp value. */
	var exp(var value) { return std::exp((double)value); }

	/** Calculates the power of base and exponent. */
	var pow(var base, var exp) { return std::pow((double)base, (double)exp); }

	/** Calculates the square (x*x) of the value. */
	var sqr(var value) { double x = (double)value; return x * x; }

	/** Calculates the square root of the value. */
	var sqrt(var value) { return std::sqrt((double)value); }

	/** Rounds up the value. */
	var ceil(var value) { return std::ceil((double)value); }

	/** Rounds down the value. */
	var floor(var value) { return std::floor((double)value); }

	/** Returns the remainder when dividing value with limit. */
	var fmod(var value, var limit) { return hmath::fmod((double)value, (double)limit); }

	/** Wraps the value around the limit (always positive). */
	var wrap(var value, var limit) { return hmath::wrap((double)value, (double)limit); }

	template <typename Type> static Type sign_(Type n) noexcept{ return n > 0 ? (Type)1 : (n < 0 ? (Type)-1 : 0); }
};

} // namespace hise