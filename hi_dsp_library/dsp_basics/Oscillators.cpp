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

namespace hise
{
using namespace juce;

template<typename T>
inline T square_number(const T &x) {
	return x * x;
}

// Adapted from "Phaseshaping Oscillator Algorithms for Musical Sound
// Synthesis" by Jari Kleimola, Victor Lazzarini, Joseph Timoney, and Vesa
// Valimaki.
// http://www.acoustics.hut.fi/publications/papers/smc2010-phaseshaping/
template <typename T> inline double blep(T t, T dt) {
	if (t < dt) {
		return -square_number(t / dt - 1);
	}
	else if (t > 1 - dt) {
		return square_number((t - 1) / dt + 1);
	}
	else {
		return 0;
	}
}

// Derived from blep().
template <typename T>inline double blamp(T t, T dt) {
	if (t < dt) {
		t = t / dt - 1;
		return -1 / 3.0 * square_number(t) * t;
	}
	else if (t > 1 - dt) {
		t = (t - 1) / dt + 1;
		return 1 / 3.0 * square_number(t) * t;
	}
	else {
		return 0;
	}
}



hise::RingBufferComponentBase* OscillatorDisplayProvider::OscillatorDisplayObject::createComponent()
{
	return new osc_display();
}

bool OscillatorDisplayProvider::OscillatorDisplayObject::validateInt(const Identifier& id, int& v) const
{
	if (id == RingBufferIds::BufferLength)
		return SimpleRingBuffer::toFixSize<256>(v);

	if (id == RingBufferIds::NumChannels)
		return SimpleRingBuffer::toFixSize<1>(v);
    
    return true;
}

void OscillatorDisplayProvider::OscillatorDisplayObject::transformReadBuffer(AudioSampleBuffer& b)
{
	if (provider != nullptr)
	{
		jassert(b.getNumChannels() == 1);
		jassert(b.getNumSamples() == 256);

		auto d = provider->uiData;
		d.uptimeDelta = 2048.0 / 256.0;

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
            default: break;
			}

			b.setSample(0, i, v);
		}
	}
}

void OscillatorDisplayProvider::OscillatorDisplayObject::initialiseRingBuffer(SimpleRingBuffer* b)
{
	b->setRingBufferSize(1, 256);
}

float OscillatorDisplayProvider::tickSaw(OscData& d)
{
	auto phase = d.tick() / 2048.0;;
	phase = std::fmod(phase + 0.5, 1.0);

	auto naiveValue = 2.0f * phase - 1.0f;

	auto dt = d.uptimeDelta / 2048.0;
	auto t = phase;

	naiveValue -= blep(t, dt);

	return naiveValue;
}

template<typename T>
inline int64_t bitwiseOrZero(const T &t) {
    return static_cast<int64_t>(t) | 0;
}

float OscillatorDisplayProvider::tickTriangle(OscData& d)
{
    auto phase = d.tick() / 2048.0;
    phase = std::fmod(phase + 0.5, 1.0);
    auto t = phase;
    
    double t1 = t + 0.25;
    t1 -= bitwiseOrZero(t1);

    double t2 = t + 0.75;
    t2 -= bitwiseOrZero(t2);

    double y = t * 4;

    if (y >= 3) {
        y -= 4;
    } else if (y > 1) {
        y = 2 - y;
    }
    
    auto dt = d.uptimeDelta / 2048.0;

    y += 4 * dt * (blamp(t1, dt) - blamp(t2, dt));

    return (float)y;
}

float OscillatorDisplayProvider::tickSine(OscData& d)
{
	return sinTable->getInterpolatedValue(d.tick());
}

float OscillatorDisplayProvider::tickSquare(OscData& d)
{
	return (float)(1 - (int)std::signbit(tickSaw(d))) * 2.0f - 1.0f;
}



OscillatorDisplayProvider::osc_display::osc_display()
{

}

void OscillatorDisplayProvider::osc_display::refresh()
{
	waveform.clear();

	if (rb != nullptr)
	{
		const auto& bf = rb->getReadBuffer();

		if(bf.getNumSamples() == 256)
		{
			auto b = getLocalBounds().reduced(10, 3).withSizeKeepingCentre(180, getHeight() - 6).toFloat();

			waveform.startNewSubPath(0.0f, 0.0f);

			for (int i = 0; i < 256; i++)
			{
				waveform.lineTo((float)i, -1.0f * bf.getSample(0, i));
			}

			waveform.lineTo(255.0f, 0.0f);

			if (!waveform.getBounds().isEmpty())
				waveform.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);
		}
	}

	repaint();
}

void OscillatorDisplayProvider::osc_display::paint(Graphics& g)
{
	auto laf = getSpecialLookAndFeel<RingBufferComponentBase::LookAndFeelMethods>();

	auto b = getLocalBounds().reduced(10, 3).withSizeKeepingCentre(180, getHeight() - 6).toFloat();

	laf->drawOscilloscopeBackground(g, *this, b.expanded(3.0f));

	Path grid;
	grid.addRectangle(b);

	laf->drawAnalyserGrid(g, *this, grid);

	if (!waveform.getBounds().isEmpty())
		laf->drawOscilloscopePath(g, *this, waveform);
}

void OscillatorDisplayProvider::osc_display::resized()
{
	refresh();
}

}
