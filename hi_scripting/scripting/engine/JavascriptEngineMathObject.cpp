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
		ADD_INLINEABLE_API_METHOD_1(abs);
		ADD_INLINEABLE_API_METHOD_1(round);
		ADD_API_METHOD_0(random);
		ADD_API_METHOD_2(randInt);
		ADD_INLINEABLE_API_METHOD_2(min);
		ADD_INLINEABLE_API_METHOD_2(max);
		ADD_INLINEABLE_API_METHOD_3(range);
		ADD_INLINEABLE_API_METHOD_1(sign);
		ADD_INLINEABLE_API_METHOD_1(toDegrees);
		ADD_INLINEABLE_API_METHOD_1(toRadians);
		ADD_INLINEABLE_API_METHOD_1(sin);
		ADD_INLINEABLE_API_METHOD_1(asin);
		ADD_INLINEABLE_API_METHOD_1(sinh);
		ADD_INLINEABLE_API_METHOD_1(asinh);
		ADD_INLINEABLE_API_METHOD_1(cos);
		ADD_INLINEABLE_API_METHOD_1(acos);
		ADD_INLINEABLE_API_METHOD_1(cosh);
		ADD_INLINEABLE_API_METHOD_1(acosh);
		ADD_INLINEABLE_API_METHOD_1(tan);
		ADD_INLINEABLE_API_METHOD_1(atan);
		ADD_INLINEABLE_API_METHOD_1(tanh);
		ADD_INLINEABLE_API_METHOD_1(atanh);
		ADD_INLINEABLE_API_METHOD_1(log);
		ADD_INLINEABLE_API_METHOD_1(log10);
		ADD_INLINEABLE_API_METHOD_1(exp);
		ADD_INLINEABLE_API_METHOD_2(pow);
		ADD_INLINEABLE_API_METHOD_1(sqr);
		ADD_INLINEABLE_API_METHOD_1(sqrt);
		ADD_INLINEABLE_API_METHOD_1(ceil);
		ADD_INLINEABLE_API_METHOD_1(floor);
		ADD_INLINEABLE_API_METHOD_2(fmod);
		ADD_INLINEABLE_API_METHOD_3(smoothstep);
		ADD_INLINEABLE_API_METHOD_2(wrap);
		ADD_INLINEABLE_API_METHOD_2(from0To1);
		ADD_INLINEABLE_API_METHOD_2(to0To1);
		ADD_INLINEABLE_API_METHOD_3(skew);

		ADD_INLINEABLE_API_METHOD_1(isinf);
		ADD_INLINEABLE_API_METHOD_1(isnan);
		ADD_INLINEABLE_API_METHOD_1(sanitize);

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
		API_METHOD_WRAPPER_3(MathClass, smoothstep);
		API_METHOD_WRAPPER_2(MathClass, wrap);
		API_METHOD_WRAPPER_3(MathClass, skew);
		API_METHOD_WRAPPER_2(MathClass, from0To1);
		API_METHOD_WRAPPER_2(MathClass, to0To1);

		API_METHOD_WRAPPER_1(MathClass, isinf);
		API_METHOD_WRAPPER_1(MathClass, isnan);
		API_METHOD_WRAPPER_1(MathClass, sanitize);
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

	/** Checks for infinity. */
	var isinf(var value)
	{
		return var(std::isinf((double)value));
	}

	/** Checks for NaN (invalid floating point value). */
	var isnan(var value)
	{
		return var(std::isnan((double)value));
	}

	/** Sets infinity & NaN floating point numbers to zero. */
	var sanitize(var value)
	{
		auto v = (double)value;
		return var(hmath::sanitize(v));
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

	/** Converts radian (0...2*PI) to degree (0...360°). */
	var toDegrees(var value) { return radiansToDegrees((double)value); }

	/** Converts degree  (0...360°) to radian (0...2*PI). */
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

	/** Returns the skew factor for the given mid point. */
	var skew(var start, var end, var midPoint)
	{
		NormalisableRange<double> rng(start, end); rng.setSkewForCentre(midPoint); return rng.skew;
	}

	/** Calculates a smooth transition between the lower and the upper value. */
	var smoothstep(var input, var lower, var upper)
	{
		return var(upper > lower ? hmath::smoothstep((double)input, (double)lower, (double)upper) : 0.0);
	}

	/** Converts a normalised value (between 0 and 1) to a range defined by the JSON data in rangeObj. */
	var from0To1(var value, var rangeObj)
	{
		return getRange(rangeObj).convertFrom0to1((double)value, true);
	}

	/** Converts a value inside a range defined by the JSON data in range obj to a normalised value. */
	var to0To1(var value, var rangeObj)
	{
		return getRange(rangeObj).convertTo0to1((double)value, true);
	}

	template <typename Type> static Type sign_(Type n) noexcept{ return n > 0 ? (Type)1 : (n < 0 ? (Type)-1 : 0); }

	static scriptnode::InvertableParameterRange getRange(var rangeObj)
	{
		InvertableParameterRange r;

		if(auto fo = dynamic_cast<fixobj::ObjectReference*>(rangeObj.getObject()))
		{
			auto hash = fo->typeHash;
			auto floatPtr = reinterpret_cast<float*>(fo->data);
			const bool requiresMiddle = hash == 1468876904 || hash == -1419086716;

			auto setSkew = [requiresMiddle, &r](float v) { if(requiresMiddle) r.rng.setSkewForCentre(v); else r.rng.skew = v; };
			
			switch(hash)
			{
			case 1207537023:  // { MinValue, MaxValue, SkewFactor, StepSize, Inverted(float) }
			case -1419086716: // { min, max, middlePosition, stepSize, Inverted(float) }
			case -748746349:  // { Start, End, Skew, Interval, Inverted }
			{
				r.rng.start = floatPtr[0];
				r.rng.end = floatPtr[1];
				setSkew(floatPtr[2]);
				r.rng.interval = floatPtr[3];
				r.inv = floatPtr[4] > 0.5;
				break;
			}
			case -575529029: // { MinValue, MaxValue, SkewFactor }
			case 1468876904: // { min, max, middlePosition }
			case 2138798677: // { Start, End, Skew }
			{
				r.rng.start = floatPtr[0];
				r.rng.end = floatPtr[1];
				setSkew(floatPtr[2]);
				break;
			}
			case -1567604795: // { MinValue, MaxValue, StepSize }
			case -1126239209: // { min, max, stepSize }
			case 1610048532:  // { Start, End, Interval }
			{
				r.rng.start = floatPtr[0];
				r.rng.end = floatPtr[1];
				r.rng.interval = floatPtr[2];
				break;
			}
			default:
				throw String("unknown type layout " + JSON::toString(rangeObj, true));
			}
		}
		else if (auto dyn = rangeObj.getDynamicObject())
		{
			const auto& set = dyn->getProperties();

			r.inv = set.getWithDefault(PropertyIds::Inverted, false);
			
			if (set.contains(PropertyIds::MaxValue)) // scriptnode range object
			{
				r.rng.start = set.getWithDefault(PropertyIds::MinValue, 0.0);
				r.rng.end = set.getWithDefault(PropertyIds::MaxValue, 1.0);
				r.rng.interval = set.getWithDefault(PropertyIds::StepSize, 0.0);
				r.rng.skew = set.getWithDefault(PropertyIds::SkewFactor, 1.0);
			}
			else if (set.contains("max")) // UI Component properties
			{
				r.rng.start = set.getWithDefault("min", 0.0);
				r.rng.end = set.getWithDefault("max", 1.0);
				r.rng.interval = set.getWithDefault("stepSize", 0.0);

				if (set.contains("middlePosition"))
					r.rng.setSkewForCentre(set["middlePosition"]);
			}
			else if (set.contains("Start")) // MIDI Automation object
			{
				r.rng.start = set.getWithDefault("Start", 0.0);
				r.rng.end = set.getWithDefault("End", 1.0);
				r.rng.interval = set.getWithDefault(PropertyIds::StepSize, 0.0);
				r.rng.skew = set.getWithDefault("Skew", 1.0);
			}
		}

		return r;
	}
};

} // namespace hise