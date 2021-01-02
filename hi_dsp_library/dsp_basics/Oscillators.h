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

namespace hise { using namespace juce;


template <int TableSize> class SineLookupTable
{
public:

	SineLookupTable()
	{
		for (int i = 0; i < TableSize; i++)
		{
			sinTable[i] = sinf(i * float_Pi / (float)(TableSize / 2));
		}
	}

	constexpr int getTableSize() const { return TableSize; };

	float getInterpolatedValue(double uptime) const
	{
		int index = (int)uptime;

		const float v1 = getWrappedValue(index);
		const float v2 = getWrappedValue(index + 1);
		const float alpha = float(uptime) - (float)index;
		const float invAlpha = 1.0f - alpha;
		const float currentSample = invAlpha * v1 + alpha * v2;

		return currentSample;
	}

private:

	float getWrappedValue(int index) const noexcept
	{
		return sinTable[index & (TableSize - 1)];
	}

	float sinTable[TableSize];
};

struct OscData
{
	void reset()
	{
		uptime = 0.0;
	}

	void setFrequency(HiseEvent& e, double sampleRate)
	{
		if (sampleRate != 0.0)
		{
			auto freq = e.getFrequency();
			uptimeDelta = freq / sampleRate;
		}
	}

	double tick()
	{
		auto rv = uptime;
		uptime += (uptimeDelta * multiplier);
		return rv;
	}

	double uptime = 0.0;
	double uptimeDelta = 0.0;
	double multiplier = 1.0;
};

struct OscillatorDisplayProvider
{
	enum class Mode
	{
		Sine,
		Saw,
		Triangle,
		Square,
		Noise,
		numModes
	};

	OscillatorDisplayProvider()
	{
		modes = { "Sine", "Saw", "Triangle", "Square", "Noise" };
	}

	virtual ~OscillatorDisplayProvider() {};

	float tickNoise(OscData& d)
	{
		return r.nextFloat() * 2.0f - 1.0f;
	}

	float tickSaw(OscData& d)
	{
		return 2.0f * std::fmodf(d.tick() / sinTable->getTableSize(), 1.0f) - 1.0f;
	}

	float tickTriangle(OscData& d)
	{
		return (1.0f - std::abs(tickSaw(d))) * 2.0f - 1.0f;
	}

	float tickSine(OscData& d)
	{
		return sinTable->getInterpolatedValue(d.tick());
	}

	float tickSquare(OscData& d)
	{
		return (float)(1 - (int)std::signbit(tickSaw(d))) * 2.0f - 1.0f;
	}

	bool useMidi = false;

	Random r;
	SharedResourcePointer<SineLookupTable<2048>> sinTable;
	StringArray modes;
	Mode currentMode = Mode::Sine;

	JUCE_DECLARE_WEAK_REFERENCEABLE(OscillatorDisplayProvider);
};

} 