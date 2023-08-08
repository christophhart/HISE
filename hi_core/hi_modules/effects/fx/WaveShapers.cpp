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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#include <math.h>

namespace hise {
using namespace juce;




struct ShapeFX::ShapeFunctions
{
	struct Linear { static float shape(float input) { return input; }; };
	struct Atan { static float shape(float input) { return atanf(input); }; };
	struct Tanh { static float shape(float input) { return std::tanh(input); }; };

	struct Sin { static float shape(float input) { return sinf(input); }; };
	struct Asinh { static float shape(float input) { return std::asinh(input); }; };
	struct TanCos { static float shape(float input) { return atanf(2.0f * input) * cosf(input / 2.0f); } };

#define POW_3(x) x * x * x
#define POW_5(x) x * x * x * x * x
#define POW_7(x) x * x * x * x * x * x * x

	struct Chebichev1 { static float shape(float input) { float x = input * 0.25f; return (4.0f * POW_3(x) - 3.0f * x); }; };
	struct Chebichev2 { static float shape(float input) { return 16.0f * POW_5(input) - 20.0f * POW_3(input)  + 5.0f * input; }; };
	struct Chebichev3 { static float shape(float input) { return 64.0f * POW_7(input) - 112.0f * POW_5(input) + 56.0f * POW_3(input) - 7.0f * input; }; };

#undef POW_3
#undef POW_5
#undef POW_7

	struct Square
	{
		static float shape(float input)
		{
			auto sign = (0.f < input) - (input < 0.0f);
			return jlimit<float>(-1.0f, 1.0f, (float)sign * input * input);
		};
	};

	struct SquareRoot
	{
		static float shape(float input)
		{
			auto v = fabsf(input);
			auto sign = (0.f < input) - (input < 0.0f);

			return (float)sign * sqrt(v);
		};
	};
};

class PolyshapeFX::PolytableShaper : public ShapeFX::ShaperBase
{
public:

	PolytableShaper(PolyshapeFX& p):
		table(static_cast<SampleLookupTable*>(p.getTableUnchecked(0)))
	{}

	static String getName() { return "Curve"; };

	void processBlock(float* l, float* r, int numSamples) override
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = get(l[i]);
			r[i] = get(r[i]);
		}
	}

	float getSingleValue(float input) override { return get(input); };

	SampleLookupTable* table;

	~PolytableShaper() { table = nullptr; }

private:

	forcedinline float get(float input)
	{
		auto sign = (0.f < input) - (input < 0.0f);

		auto v = jlimit<float>(0.0f, 511.0f, fabsf(input) * 512.0f);

		const float i1 = floor(v);
		const float i2 = jmin<float>((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f, i1 + 1.0f);

		const float delta = v - i1;

		auto t = table->getReadPointer();

		return (float)sign * Interpolator::interpolateLinear(t[(int)i1], t[(int)i2], delta);
	}
};

class PolyshapeFX::PolytableAsymetricalShaper : public ShapeFX::ShaperBase
{
public:

	PolytableAsymetricalShaper(PolyshapeFX& p) :
		table(static_cast<SampleLookupTable*>(p.getTableUnchecked(1)))
	{}

	static String getName() { return "Asymetrical Curve"; };

	void processBlock(float* l, float* r, int numSamples) override
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = get(l[i]);
			r[i] = get(r[i]);
		}
	}

	float getSingleValue(float input) override { return get(input); };

	SampleLookupTable* table;

	~PolytableAsymetricalShaper() { table = nullptr; }

private:

	forcedinline float get(float input)
	{
		auto v = jlimit<float>(0.0f, 511.0f, (input + 1.0f) * 256.0f);

		const float i1 = floor(v);

		const float delta = v - i1;

		const int index1 = (int)i1 % SAMPLE_LOOKUP_TABLE_SIZE;
		const int index2 = (index1+1) % SAMPLE_LOOKUP_TABLE_SIZE;

		auto t = table->getReadPointer();

		auto value = 2.0f * Interpolator::interpolateLinear(t[index1], t[index2], delta) - 1.0f;

		return value;
	}
};

class ShapeFX::TableShaper : public ShapeFX::ShaperBase
{
public:

	TableShaper(ShapeFX& p) :
		table(dynamic_cast<SampleLookupTable*>(p.getTableUnchecked(0)))
	{};

	~TableShaper()
	{
		table = nullptr;
	}

	static String getName() { return "Curve"; };

	void processBlock(float* l, float* r, int numSamples) override
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = get(l[i]);
			r[i] = get(r[i]);
		}
	}

	float getSingleValue(float input) override { return get(input); };

	SampleLookupTable* table;

private:

	forcedinline float get(float input)
	{
		auto sign = (0.f < input) - (input < 0.0f);
		auto v = jlimit<float>(0.0f, 1.0f, fabsf(input)) * ((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f);

		const float i1 = floor(v);
		const float i2 = jmin<float>((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f, i1 + 1.0f);

		const float delta = v - i1;

		auto t = table->getReadPointer();

		return (float)sign * Interpolator::interpolateLinear(t[(int)i1], t[(int)i2], delta);
	}


};

#if HI_USE_SHAPE_FX_SCRIPTING
class ShapeFX::ScriptShaper : public ShapeFX::ShaperBase
{
public:

	ScriptShaper() :
		shapeResult(Result::ok())
	{}

	static String getName() { return "Script"; };

	void processBlock(float* l, float* r, int numSamples) override
	{
		SpinLock::ScopedLockType sl(scriptLock);

		static const Identifier g("gain");
		parent->scriptEngine->getRootObject()->setProperty(g, parent->gain);

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = calculateSingleValue(l[i]);
			r[i] = calculateSingleValue(r[i]);
		}
	}

	float getSingleValue(float input) override
	{
		SpinLock::ScopedLockType sl(scriptLock);

		static const Identifier g("gain");

		parent->scriptEngine->getRootObject()->setProperty(g, parent->gain);
		return calculateSingleValue(input);
	}

	ShapeFX* parent = nullptr;
	Result shapeResult;
	SpinLock scriptLock;

private:

	float calculateSingleValue(float input)
	{
		float value = 0.0f;

		

		parent->scriptEngine->setCallbackParameter(0, 0, input);
		value = (float)parent->scriptEngine->executeCallback(0, &shapeResult);

		value = jlimit<float>(-1024.0f, 1024.0f, value);
		value = FloatSanitizers::sanitizeFloatNumber(value);

		return value;
	}
};


class ShapeFX::CachedScriptShaper : public ShapeFX::ShaperBase
{
public:

	CachedScriptShaper() :
		shapeResult(Result::ok())
	{
		memset(cachedValues, 0, sizeof(float) * SAMPLE_LOOKUP_TABLE_SIZE);
	};

	static String getName() { return "Cached Script"; };

	float getSingleValue(float input) override
	{
		jassert(parent != nullptr);

		return calculateSingleValue(input);
	}

	ShapeFX* parent = nullptr;

	void processBlock(float* l, float* r, int numSamples) override
	{
		SpinLock::ScopedLockType sl(tableLock);

		jassert(parent != nullptr);

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = getCached(l[i] / gain);
			r[i] = getCached(r[i] / gain);
		}
	}

	void updateLookupTable(const float* newValues, float newGain)
	{
		SpinLock::ScopedLockType sl(tableLock);

		FloatVectorOperations::copy(cachedValues, newValues, SAMPLE_LOOKUP_TABLE_SIZE);
		gain = newGain;
	}

	float cachedValues[SAMPLE_LOOKUP_TABLE_SIZE];

	float gain = 1.0f;

private:

	float calculateSingleValue(float input)
	{
		float value = 0.0f;

		static const Identifier g("gain");
		parent->scriptEngine->getRootObject()->setProperty(g, parent->gain);

		parent->scriptEngine->setCallbackParameter(0, 0, input);
		value = (float)parent->scriptEngine->executeCallback(0, &shapeResult);

		value = jlimit<float>(-1024.0f, 1024.0f, value);
		value = FloatSanitizers::sanitizeFloatNumber(value);

		return value;
	}

	float getCached(float normalizedIndex)
	{
		auto v = jlimit<float>(0.0, 511.0f, (normalizedIndex + 1.0f) / 2.0f * (float)SAMPLE_LOOKUP_TABLE_SIZE);

		const float i1 = floor(v);
		const float i2 = jmin<float>((float)SAMPLE_LOOKUP_TABLE_SIZE - 1.0f, i1 + 1.0f);

		const float delta = v - i1;

		return Interpolator::interpolateLinear(cachedValues[(int)i1], cachedValues[(int)i2], delta);
	}

	SpinLock tableLock;
	Result shapeResult;
};
#endif

class ShapeFX::InternalSaturator : public ShapeFX::ShaperBase
{
public:

	InternalSaturator(ShapeFX& p)
	{

	}

	static String getName() { return "Saturate"; }

	void updateAmount(float gain)
	{
		s.setSaturationAmount(jmap(Decibels::gainToDecibels(gain), 0.0f, 60.0f, 0.0f, 0.99f));
	}

	float getSingleValue(float input) override { return s.getSaturatedSample(input); };

	void processBlock(float* l, float* r, int numSamples) override
	{
		for (int i = 0; i < numSamples; i++)
		{
			l[i] = s.getSaturatedSample(l[i]);
			r[i] = s.getSaturatedSample(r[i]);
		}
	}

	Saturator s;
};


#define REGISTER_BASIC_SHAPE_FUNCTION(id) shapers.set((int)ShapeFX::ShapeMode::id, new ShapeFX::FuncShaper<ShapeFX::ShapeFunctions::id>()); shapeNames.set((int)ShapeFX::ShapeMode::id, #id);
#define REGISTER_SHAPER_CLASS(enumId, ClassName) shapers.set((int)ShapeFX::ShapeMode::enumId, new ClassName(*this)); shapeNames.set((int)ShapeFX::ShapeMode::enumId, ClassName::getName());

void ShapeFX::initShapers()
{
	for (int i = 0; i < ShapeMode::numModes; i++)
	{
		shapers.add(new FuncShaper<ShapeFunctions::Linear>());
		shapeNames.add("unused");
	}

	REGISTER_BASIC_SHAPE_FUNCTION(Linear);
	REGISTER_BASIC_SHAPE_FUNCTION(Atan);
	REGISTER_BASIC_SHAPE_FUNCTION(Tanh);
	REGISTER_BASIC_SHAPE_FUNCTION(Sin);
	REGISTER_BASIC_SHAPE_FUNCTION(Asinh);
	REGISTER_SHAPER_CLASS(Saturate, InternalSaturator);
	REGISTER_BASIC_SHAPE_FUNCTION(Square);
	REGISTER_BASIC_SHAPE_FUNCTION(SquareRoot);
	REGISTER_SHAPER_CLASS(Curve, TableShaper);

#if HI_USE_SHAPE_FX_SCRIPTING
	REGISTER_SHAPER_CLASS(CachedScript, CachedScriptShaper);
	REGISTER_SHAPER_CLASS(Script, ScriptShaper);

	static_cast<ScriptShaper*>(shapers[Script])->parent = this;
	static_cast<CachedScriptShaper*>(shapers[CachedScript])->parent = this;
#endif
};

void PolyshapeFX::initShapers()
{
	for (int i = 0; i < ShapeFX::ShapeMode::numModes; i++)
	{
		shapers.add(new ShapeFX::FuncShaper<ShapeFX::ShapeFunctions::Linear>());
		shapeNames.add("unused");
	}

	REGISTER_BASIC_SHAPE_FUNCTION(Linear);
	REGISTER_BASIC_SHAPE_FUNCTION(Atan);
	REGISTER_BASIC_SHAPE_FUNCTION(Sin);
	REGISTER_BASIC_SHAPE_FUNCTION(TanCos);
	REGISTER_BASIC_SHAPE_FUNCTION(Asinh);
	REGISTER_BASIC_SHAPE_FUNCTION(Chebichev1);
	REGISTER_BASIC_SHAPE_FUNCTION(Chebichev2);
	REGISTER_BASIC_SHAPE_FUNCTION(Chebichev3);
	REGISTER_SHAPER_CLASS(Curve, PolytableShaper);
	REGISTER_SHAPER_CLASS(AsymetricalCurve, PolytableAsymetricalShaper);
}



#undef REGISTER_BASIC_SHAPE_FUNCTION

}