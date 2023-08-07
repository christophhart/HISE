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
	OscData() = default;

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

	float getNyquistAttenuationGain() const noexcept
	{
		return (float)(1 - (int)((multiplier * uptimeDelta) > 1024.0));
	}

	double tick()
	{
		auto rv = uptime;
		uptime += (uptimeDelta * multiplier);
		return rv + phase;
	}

	double uptime = 0.0;
	double uptimeDelta = 0.0;
	double multiplier = 1.0;
	double phase = 0.0f;
	float gain = 1.0f;
	int enabled = 1;
};

struct OscillatorDisplayProvider: public scriptnode::data::display_buffer_base<true>
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

	struct osc_display : public ComponentWithMiddleMouseDrag,
		public RingBufferComponentBase,
		public ComponentWithDefinedSize
	{
		osc_display();

		void refresh() override;
		void paint(Graphics& g) override;
		void resized() override;
		Rectangle<int> getFixedBounds() const override { return { 0, 0, 300, 60 }; }

		Path waveform;
	};

	struct OscillatorDisplayObject : public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 9000;

		OscillatorDisplayObject(SimpleRingBuffer::WriterBase* b):
			PropertyObject(b),
			provider(getTypedBase<OscillatorDisplayProvider>())
		{}

		int getClassIndex() const override { return PropertyIndex; }

		RingBufferComponentBase* createComponent() override;;

		Path createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double ) const override;

		bool validateInt(const Identifier& id, int& v) const override;
		void transformReadBuffer(AudioSampleBuffer& b) override;
		void initialiseRingBuffer(SimpleRingBuffer* b) override;

		bool allowModDragger() const override { return false; };

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

	void registerPropertyObject(SimpleRingBuffer::Ptr rb) override
	{
		rb->registerPropertyObject<OscillatorDisplayObject>();
	}

	float tickSaw(OscData& d);

	float tickTriangle(OscData& d);

	float tickSine(OscData& d);

	float tickSquare(OscData& d);

	Random r;
	SharedResourcePointer<SineLookupTable<2048>> sinTable;
	const StringArray modes;
	Mode currentMode = Mode::Sine;

	OscData uiData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(OscillatorDisplayProvider);
};

}

