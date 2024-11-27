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

namespace scriptnode { using namespace juce;

namespace conversion_logic
{
struct ms2freq
{
    double getValue(double input) const
    {
        if(input == 0.0)
            return 0.0;
        
        return 1.0 / (input * 0.001);
    }
};

struct freq2ms
{
    double getValue(double input) const
    {
        if(input == 0.0)
            return 0.0;
        
        return (1.0 / input) * 1000.0;
    }
};

struct ms2samples
{
    double getValue(double input) const
    {
        return input * 0.001 * sampleRate;
    }
    
    void prepare(PrepareSpecs ps)
    {
        sampleRate = ps.sampleRate;
    }
    
    double sampleRate = 0.0;
};

struct freq2samples
{
    double getValue(double input) const
    {
        return input > 0.001f  ? sampleRate / input : 0.0f;
    }
    
    void prepare(PrepareSpecs ps)
    {
        sampleRate = ps.sampleRate;
    }
    
    double sampleRate = 0.0;
};

struct ms2bpm
{
    double getValue(double input) const
    {
        return 60 / (hmath::max(input, 1.0) * 0.001);
    }
};

struct samples2ms
{
    double getValue(double input) const
    {
        if(sampleRate == 0.0)
            return 0.0;
        
        return input / sampleRate * 1000.0;
    }
    
    void prepare(PrepareSpecs ps)
    {
        sampleRate = ps.sampleRate;
    }
    
    double sampleRate = 0.0;
};

struct pitch2st
{
    double getValue(double input) const
    {
        return std::log2(input) * 12.0;
    }
};

struct st2pitch
{
    double getValue(double input) const
    {
        return pow(2.0, input / 12.0);
    }
};

struct cent2pitch
{
	double getValue(double input) const
	{
		return pow(2.0, input / 1200.0);
	}
};

struct pitch2cent
{
	double getValue(double input) const
	{
		return std::log2(input) * 1200.0;
	}
};

struct midi2freq
{
    double getValue(double input) const
    {
        return MidiMessage::getMidiNoteInHertz(hmath::round(input*127.0));
    }
};

struct freq2norm
{
    double getValue(double input) const
    {
        static constexpr double factor = 1.0 / 20000.0;
        return input * factor;
    }
};

struct db2gain
{
    double getValue(double input) const
    {
        return hmath::db2gain(input);
    }
};

struct gain2db
{
    double getValue(double input) const
    {
        return hmath::gain2db(input);
    }
};
};


/** Contains all different MIDI processing units. */
namespace midi_logic
{

template <int Unused> struct gate
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_INITIALISE;

    bool getMidiValue(HiseEvent& e, double& v)
    {
        if (e.isNoteOnOrOff())
        {
            v = (double)e.isNoteOn();
            return true;
        }

        return false;
    }
};

template <int Unused> struct random
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_INITIALISE;

    bool getMidiValue(HiseEvent& e, double& v)
    {
        if (e.isNoteOn())
        {
            v = r.nextDouble();
            return true;
        }

        return false;
    }
    
    Random r;
};

template <int Unused> struct velocity
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_INITIALISE;

    bool getMidiValue(HiseEvent& e, double& v)
    {
        if (e.isNoteOn())
        {
            v = e.getFloatVelocity();
            return true;
        }

        return false;
    }
};

template <int Unused> struct notenumber
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_INITIALISE;

    bool getMidiValue(HiseEvent& e, double& v)
    {
        if (e.isNoteOn())
        {
            v = (double)e.getNoteNumberIncludingTransposeAmount() / 127.0;
            return true;
        }

        return false;
    }
};

template <int Unused> struct frequency
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_INITIALISE;

    static constexpr bool IsProcessingHiseEvent() { return true; }

    bool getMidiValue(HiseEvent& e, double& v)
    {
        if (e.isNoteOn())
        {
            v = (e.getFrequency()) / 20000.0;
            return true;
        }

        return false;
    }
};
}

/** Contains all units for clone cable value calculations. */
namespace duplilogic
{

struct Helpers
{
	static bool shouldUpdateNumClones(const String& mode)
	{
        // Keep this list up to date with all nodes that return false
        // at shouldUpdateNumClones();
		static const StringArray staticModes = 
        {
            "toggle"
		};

        return !staticModes.contains(mode);
	}
};

struct spread
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int index, int numUsed, double inputValue, double gamma)
    {
        if (numUsed == 1)
            return 0.5;

        auto n = (double)index / (double)(numUsed - 1) - 0.5;
        
        if (gamma != 0.0)
        {
            auto gn = hmath::sin(double_Pi * n);
            gn *= 0.5;
            n = gn * gamma + n * (1.0 - gamma);
        }

        n *= inputValue;

        return n + 0.5;
    }
};

struct triangle
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int index, int numUsed, double inputValue, double gamma)
    {
        if (numUsed == 1)
            return 1.0;

        auto n = (double)index / (double)(numUsed - 1);

        n = hmath::abs(n - 0.5) * 2.0;

        if (gamma != 0.0)
        {
            auto gn = hmath::sin(n * double_Pi * 0.5);
            gn *= gn;

            n = gamma * gn + (1.0 - gamma) * n;
        }

        return 1.0 - inputValue * n;
    }
};



struct harmonics: public midi_logic::frequency<0>
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int index, int numUsed, double inputValue, double gamma)
    {
        return (double)(index + 1) * inputValue;
    }
};

struct nyquist: public midi_logic::frequency<0>
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int index, int numUsed, double inputValue, double gamma)
    {
        auto hvalue = harmonics().getValue(index, numUsed, inputValue, gamma);
        return hmath::smoothstep(hvalue, 1.0, jmin(0.99, gamma));
    }
};

struct fixed: public midi_logic::frequency<0>
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int /*index*/, int /*numUsed*/, double inputValue, double /*gamma*/)
    {
        return inputValue;
    }
};

struct ducker
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int /*index*/, int numUsed, double /*inputValue*/, double gamma)
    {
        auto v = 1.0 / jmax(1.0, (double)numUsed);
        
        if (gamma != 0.0)
        {
            v = std::pow(v, 1.0 - gamma);
        }

        return v;
    }
};


struct toggle
{
    static constexpr bool shouldUpdateNumClones() { return false; }

    double getValue(int index, int numUsed, double inputValue, double /*gamma*/)
    {
        auto thisIndex = (double)index / (double)numUsed;
        return (double)(thisIndex <= inputValue);
    }
};

struct random
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    Random r;

	static constexpr bool IsProcessingHiseEvent() { return true; }

	bool getMidiValue(HiseEvent& e, double& v)
	{
		if (e.isNoteOn())
		{
			// don't change the value, just return true
			return true;
		}

		return false;
	}

    double getValue(int index, int numUsed, double inputValue, double gamma)
    {
        double n;

        if (numUsed == 1)
            n = 0.5f;
        else
            n = (double)index / (double)(numUsed - 1) ;

        return jlimit(0.0, 1.0, n + (2.0 * r.nextDouble() - 1.0) * inputValue);
    }
};

struct scale
{
    static constexpr bool shouldUpdateNumClones() { return true; }

    double getValue(int index, int numUsed, double inputValue, double gamma)
    {
        if (numUsed == 1)
            return inputValue;

        auto n = (double)index / (double)(numUsed - 1) * inputValue;

        if (gamma != 1.0)
            n = hmath::pow(n, 1.0 + gamma);

        return n;
    }
};

}

namespace file_analysers
{
struct pitch
{
    double getValue(const ExternalData& d)
    {
        if (d.numSamples > 0)
        {
            block b;
            d.referBlockTo(b, 0);

            return PitchDetection::detectPitch(b.begin(), b.size(), d.sampleRate);
        }

        return 0.0;
    }
};

struct milliseconds
{
    double getValue(const ExternalData& d)
    {
        if (d.numSamples > 0 && d.sampleRate > 0.0)
        {
            return 1000.0 * (double)d.numSamples / d.sampleRate;
        }

        return 0.0;
    }
};

struct peak
{
    double getValue(const ExternalData& d)
    {
        if (d.numSamples > 0)
        {
            auto b = d.toAudioSampleBuffer();
            return (double)b.getMagnitude(0, d.numSamples);
        }

        return 0.0;
    }
};
}

namespace timer_logic
{
template <int NV> struct ping
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_RESET;

    double getTimerValue() const { return 1.0; }
};

template <int NV> struct random
{
    SN_EMPTY_PREPARE;
    SN_EMPTY_RESET;

    double getTimerValue() const
    {
        return hmath::randomDouble();
    }
};

template <int NV> struct toggle
{
    PolyData<double, NV> state;

    double getTimerValue()
    {
        double v = 0.0;

        for (auto& s : state)
        {
            s = 1.0 - s;
            v = s;
        }
        
        return v;
    }

    void prepare(PrepareSpecs ps)
    {
        state.prepare(ps);
    }

    void reset()
    {
        for (auto& s : state)
            s = 0.0;
    }
};

}

namespace faders
{
struct switcher
{
    SN_EMPTY_INITIALISE;

    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        auto numParameters = (double)(numElements);
        auto indexToActivate = jmin(numElements - 1, (int)(normalisedInput * numParameters));

        return (double)(indexToActivate == Index);
    }
};

struct overlap
{
    SN_EMPTY_INITIALISE;

    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        if (isPositiveAndBelow(Index, numElements))
        {
            switch (numElements)
            {
            case 2:
            {
                switch (Index)
                {
                case 0: return jlimit(0.0, 1.0, 2.0 - 2.0 * normalisedInput);
                case 1: return jlimit(0.0, 1.0, 2.0 * normalisedInput);
                default: return 0.0;
                }
            }
            case 3:
            {
                switch (Index)
                {
                case 0: return jlimit(0.0, 1.0, 3.0 - 3.0 * normalisedInput);
                case 1: return jlimit(0.0, 1.0, 3.0 * normalisedInput);
                default: return 0.0;
                }

            }
            case 4:
            {
                if (Index != 1)
                    return 0.0;

                auto v = 2.0 - hmath::abs(-4.0 * (normalisedInput + 0.66));

                v = jmax(0.0, v - 1.0);

                return jlimit(0.0, 1.0, v);
            }
            }
        }

        return 0.0;
    }
};

struct harmonics
{
    SN_EMPTY_INITIALISE;

    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        return normalisedInput * (double)(Index + 1);
    }
};

struct threshold
{
    SN_EMPTY_INITIALISE;
    
    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        auto tr = (double)(Index) / (double)(numElements);
        
        return (double)(normalisedInput >= tr);
    }
};

struct linear
{
    SN_EMPTY_INITIALISE;

    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        if (numElements == 1)
            return 1.0 - normalisedInput;
        else
        {
            const double u = (double)numElements - 1.0;
            const double offset = (1.0 - (double)Index) / u;
            auto v = 1.0 - Math.abs(1.0 - u * (normalisedInput + offset));
            return jlimit(0.0, 1.0, v);
        }
    }

    hmath Math;
};

struct cosine
{
    SN_EMPTY_INITIALISE;
    
    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        auto lf = linear();
        
        auto v = lf.template getFadeValue<Index>(numElements, normalisedInput);
        
        return 0.5 * (Math.sin(Math.PI * v - Math.PI / 2.0) + 1.0);
    }
    
    hmath Math;
};

struct cosine_half
{
    SN_EMPTY_INITIALISE;
    
    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        auto lf = linear();
        
        auto v = lf.template getFadeValue<Index>(numElements, normalisedInput);
        
        return Math.sin(Math.PI * 0.5 * v);
    }
    
    hmath Math;
};

struct squared
{
    SN_EMPTY_INITIALISE;

    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        auto v = lf.getFadeValue<Index>(numElements, normalisedInput);
        return v * v;
    }

    linear lf;
};

struct rms
{
    SN_EMPTY_INITIALISE;

    template <int Index> double getFadeValue(int numElements, double normalisedInput)
    {
        auto v = lf.getFadeValue<Index>(numElements, normalisedInput);
        return hmath::sqrt(v);
    }

    linear lf;
};


}


namespace smoothers
{
struct base
{
    virtual ~base() {};
    
    void setSmoothingTime(double t)
    {
        if (smoothingTimeMs != t)
        {
            smoothingTimeMs = t;
            refreshSmoothingTime();
        }
    }

	void setEnabled(double v)
	{
		enabled = v > 0.5;

		if (enabled)
			reset();
	}

    virtual float get() const = 0;
    virtual void reset() = 0;
    virtual void set(double v) = 0;
    virtual float advance() = 0;

    virtual void prepare(PrepareSpecs ps)
    {
        currentBlockRate = ps.sampleRate / (double)ps.blockSize;
        refreshSmoothingTime();
    }

    virtual void refreshSmoothingTime() = 0;

    virtual SN_EMPTY_INITIALISE;

    double currentBlockRate = 0.0;
    double smoothingTimeMs = 0.0;
	bool enabled = true;
};

template <int NV> struct no : public base
{
	static constexpr int NumVoices = NV;

    float get() const final override
    {
        return v;
    }

    void reset() final override {};

    void set(double nv) final override
    {
        v = nv;
    }

    float advance() final override
    {
        return v;
    }

    void refreshSmoothingTime() final override {};

    float v = 0.0f;
};

template <int NV> struct low_pass : public base
{
	static constexpr int NumVoices = NV;

    float get() const final override
    {
        return state.get().lastValue;
    }

	void prepare(PrepareSpecs ps) final override
	{
		base::prepare(ps);
		state.prepare(ps);
	}

    void reset() final override
    {
		for (auto& s : state)
			s.reset();
    }

    float advance() final override
    {
		return enabled ? state.get().advance() : state.get().target;
    }

    void set(double targetValue) final override
    {
		for (auto& s : state)
			s.set(targetValue);
    }

    void refreshSmoothingTime() final override
    {
		for (auto& s : state)
		{
			s.s.prepareToPlay(currentBlockRate);
			s.s.setSmoothingTime(smoothingTimeMs);
		}
    }

private:

	struct State
	{
		void reset()
		{
			isSmoothing = false;
			lastValue = target;
			s.resetToValue(target);
		}

		float advance()
		{
			if (isSmoothing)
			{
				auto thisValue = s.smooth(target);
                isSmoothing = FloatSanitizers::isNotSilence(thisValue - target);
				lastValue = thisValue;
				return thisValue;
			}

			return target;
		}

		void set(double targetValue)
		{
			auto tf = (float)targetValue;

			if (tf != target)
			{
				target = tf;
				isSmoothing = target != lastValue;
			}
		}

		
		bool isSmoothing = false;
		float lastValue = 0.0f;
		float target = 0.0f;
		Smoother s;
	};

	PolyData<State, NumVoices> state;
};

template <int NV> struct linear_ramp : public base
{
	static constexpr int NumVoices = NV;

    void reset()final override
    {
		for(auto& s: state)
			s.reset();
    }

	void prepare(PrepareSpecs ps) override
	{
		base::prepare(ps);
		state.prepare(ps);
	}

    float advance() final override
    {
        return enabled ? state.get().advance() : state.get().targetValue;
    }

    float get() const final override
    {
        return enabled ? state.get().get() : state.get().targetValue;
    }

    void set(double newValue) final override
    {
		for(auto& s: state)
			s.set(newValue);
    }

    void refreshSmoothingTime() final override
    {
		for(auto& s: state)
			s.prepare(currentBlockRate, smoothingTimeMs);
    }

private:
	
	PolyData<sdouble, NumVoices> state;
};
}


} 
