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
		const double alpha = uptime - (double)index;
		const double invAlpha = 1.0 - alpha;
		const float currentSample = (float)invAlpha * v1 + (float)alpha * v2;

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

struct OscillatorDisplayProvider: public scriptnode::data::base
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

	struct OscillatorDisplayObject : public SimpleRingBuffer::PropertyObject
	{
		OscillatorDisplayObject(OscillatorDisplayProvider* p):
			provider(p)
		{}

		bool validateInt(const Identifier& id, int& v) const override
		{
			if (id == RingBufferIds::BufferLength)
				return SimpleRingBuffer::toFixSize<256>(v);

			if (id == RingBufferIds::NumChannels)
				return SimpleRingBuffer::toFixSize<1>(v);
		}

		void transformReadBuffer(AudioSampleBuffer& b) override
		{
			if (provider != nullptr)
			{
				jassert(b.getNumChannels() == 1);
				jassert(b.getNumSamples() == 256);

				OscData d;
				d.uptimeDelta = 2048.0 / 256.0;
				d.multiplier = provider->pitchMultiplier;

				for (int i = 0; i < 256; i++)
				{
					float v = 0.0f;

					switch (provider->currentMode)
					{
					case Mode::Sine: v = provider->tickSine(d); break;
					case Mode::Saw: v = provider->tickSaw(d); break;
					case Mode::Square: v = provider->tickSquare(d); break;
					case Mode::Triangle: v = provider->tickTriangle(d); break;
					case Mode::Noise: v = provider->tickNoise(d); break;
					}

					b.setSample(0, i, v);
				}
			}
		}

		void initialiseRingBuffer(SimpleRingBuffer* b) override
		{
			b->setRingBufferSize(1, 256);
		}

		WeakReference<OscillatorDisplayProvider> provider;
	};

	OscillatorDisplayProvider():
		modes({ "Sine", "Saw", "Triangle", "Square", "Noise" })
	{}

	OscillatorDisplayProvider(const OscillatorDisplayProvider& other) :
		modes(other.modes),
		currentMode(other.currentMode),
		sinTable(other.sinTable)
	{}

	OscillatorDisplayProvider& operator=(const OscillatorDisplayProvider& other)
	{
		currentMode = other.currentMode;
		return *this;
	}

	virtual ~OscillatorDisplayProvider() {};

	float tickNoise(OscData& d)
	{
		return r.nextFloat() * 2.0f - 1.0f;
	}

	void setExternalData(const ExternalData& d, int index)
	{
		base::setExternalData(d, index);

		if (auto rb = dynamic_cast<SimpleRingBuffer*>(d.obj))
		{
			rb->setPropertyObject(new OscillatorDisplayObject(this));
		}
	}

	float tickSaw(OscData& d)
	{
		return 2.0f * std::fmod(d.tick() / 2048.0, 1.0) - 1.0f;
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

	Random r;
	SharedResourcePointer<SineLookupTable<2048>> sinTable;
	const StringArray modes;
	Mode currentMode = Mode::Sine;
	double pitchMultiplier = 1.0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(OscillatorDisplayProvider);
};

}

