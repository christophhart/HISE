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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace core
{

class tempo_sync : public HiseDspBase,
				   public TempoListener
{
public:

	SET_HISE_NODE_ID("tempo_sync");
	GET_SELF_AS_OBJECT(tempo_sync);
	SET_HISE_NODE_EXTRA_HEIGHT(24);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	void initialise(NodeBase* n) override
	{
		mc = n->getScriptProcessor()->getMainController_();
		mc->addTempoListener(this);
	}

	~tempo_sync()
	{
		mc->removeTempoListener(this);
	}

	struct TempoDisplay : public ModulationSourceBaseComponent
	{
		TempoDisplay(PooledUIUpdater* updater, tempo_sync* p_):
			ModulationSourceBaseComponent(updater),
			p(p_)
		{

		}

		void timerCallback() override
		{
			auto thisValue = p->currentTempoMilliseconds;

			if (thisValue != lastValue)
			{
				lastValue = thisValue;
				repaint();
			}
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white);
			g.setFont(GLOBAL_FONT());
			
			String n = String((int)lastValue) + " ms";

			g.drawText(n, getLocalBounds().toFloat(), Justification::centred);
		}

		double lastValue = 0.0;
		tempo_sync* p;
	};

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return new TempoDisplay(updater, this);
	}

	void createParameters(Array<ParameterData>& data) override
	{
		{
			ParameterData p("Tempo");

			StringArray sa;

			for (int i = 0; i < TempoSyncer::numTempos; i++)
				sa.add(TempoSyncer::getTempoName(i));

			p.setParameterValueNames(sa);
			p.db = std::bind(&tempo_sync::updateTempo, this, std::placeholders::_1);

			data.add(std::move(p));
		}
	}

	void tempoChanged(double newTempo) override
	{
		bpm = newTempo;
		updateTempo((double)(int)currentTempo);
	}

	void updateTempo(double newTempoIndex)
	{
		currentTempo = (TempoSyncer::Tempo)(int)newTempoIndex;
		currentTempoMilliseconds = TempoSyncer::getTempoInMilliSeconds(this->bpm, currentTempo);
	}

	void prepare(PrepareSpecs)
	{

	}

	void reset()
	{

	}

	void process(ProcessData& )
	{

	}

	void processSingle(float*, int)
	{

	}

	bool handleModulation(double& max)
	{
		if (lastTempoMs != currentTempoMilliseconds)
		{
			lastTempoMs = currentTempoMilliseconds;
			max = currentTempoMilliseconds;

			return true;
		}

		return false;
	}

	double currentTempoMilliseconds = 500.0;
	double lastTempoMs = 0.0;
	double bpm = 120.0;
	TempoSyncer::Tempo currentTempo;

	MainController* mc = nullptr;
};

class peak : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("peak");
	GET_SELF_AS_OBJECT(peak);
	SET_HISE_NODE_EXTRA_HEIGHT(60);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	void prepare(PrepareSpecs)
	{}

	bool handleModulation(double& value)
	{
		value = max;

		return true;
	}

	Component* createExtraComponent(PooledUIUpdater* updater) 
	{ 
		return new ModulationSourcePlotter(updater);
	};

	void createParameters(Array<ParameterData>&)
	{

	}

	forcedinline void reset() noexcept { max = 0.0; };


	void process(ProcessData& data)
	{
		max = DspHelpers::findPeak(data);
	}

	void processSingle(float* frameData, int numChannels)
	{
		max = DspHelpers::findPeak(ProcessData(&frameData, 1, numChannels));
	}

	double max = 0.0;
};


class simple_saw : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("simple_saw");
	GET_SELF_AS_OBJECT(simple_saw);
	SET_HISE_NODE_EXTRA_HEIGHT(60);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	

	simple_saw()
	{
		setPeriodTime(100.0);
	}

	bool handleModulation(double& v)
	{ 
		v = currentValue;
		return true;
	};

	void prepare(PrepareSpecs ps)
	{
		sr = ps.sampleRate;
        setPeriodTime(periodTime);
	}

	void process(ProcessData& d)
	{
		for (int c = 0; c < d.numChannels; c++)
		{
			double thisUptime = uptime;

			for (int i = 0; i < d.size; i++)
			{
				auto nextValue = fmod(thisUptime, 1.0);
				thisUptime += uptimeDelta;

				d.data[c][i] += (float)nextValue;
			}
		}

		uptime += uptimeDelta * (double)d.size;
		currentValue = fmod(uptime, 1.0);
	}

	forcedinline void reset() noexcept { uptime = 0.0; };

	void processSingle(float* frameData, int numChannels)
	{
		currentValue = fmod(uptime, 1.0);
		uptime += uptimeDelta;

		FloatVectorOperations::add(frameData, (float)currentValue, numChannels);
	}

	Component* createExtraComponent(PooledUIUpdater* updater)
	{
		return new ModulationSourcePlotter(updater);
	};

	void createParameters(Array<ParameterData>& data) override
	{
		{
			ParameterData p("PeriodTime");

			p.range = { 0.1, 1000.0, 0.1 };
			p.defaultValue = 100.0;

			p.db = std::bind(&simple_saw::setPeriodTime, this, std::placeholders::_1);

			data.add(std::move(p));
		}
	}

	void setPeriodTime(double periodTimeMs)
	{
        periodTime = periodTimeMs;
        
		if (sr > 0.0)
		{
			auto s = periodTime * 0.001;
			auto inv = 1.0 / jmax(0.00001, s);

			uptimeDelta = jmax(0.0000001, inv / sr);
		}
	}

	double sr = 44100.0;
	double uptime = 0.0;
	double uptimeDelta = 0.001;
	double currentValue = 0.0;
    double periodTime = 500.0;
};


template <int NV> class oscillator_impl: public PolyDspBase<NV>
{
public:

	constexpr static int NumVoices = NV;

	SET_HISE_NODE_EXTRA_HEIGHT(50);
	SET_HISE_POLY_NODE_ID("oscillator");

	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	GET_SELF_AS_OBJECT(oscillator_impl);

	StringArray modes;

	oscillator_impl()
	{
		modes = { "Sine", "Saw", "Noise", "Square" };
	}

	void createParameters(Array<HiseDspBase::ParameterData>& data)
	{
		{
			HiseDspBase::ParameterData p("Mode");
			p.setParameterValueNames(modes);
			p.db = std::bind(&oscillator_impl::setMode, this, std::placeholders::_1);
			data.add(std::move(p));
		}
		{
			HiseDspBase::ParameterData p("Frequency");
			p.range = { 20.0, 20000.0, 0.1 };
			p.defaultValue = 220.0;
			p.range.setSkewForCentre(1000.0);
			p.db = std::bind(&oscillator_impl::setFrequency, this, std::placeholders::_1);
			data.add(std::move(p));
		}
	}

	void setMode(double newMode)
	{
		currentMode = (int)newMode;
	}

	void setFrequency(double newFrequency)
	{
		if (voiceData.isVoiceRenderingActive())
		{
			voiceData.get().uptimeDelta = (double)(newFrequency / sr * double_Pi * 2.0);
		}
		else
		{
			auto newUptimeDelta = (double)(newFrequency / sr * double_Pi * 2.0);

			voiceData.forEachVoice([newUptimeDelta](OscData& d)
			{
				d.uptimeDelta = newUptimeDelta;
			});
		}
	}

	forcedinline void reset() noexcept { voiceData.get().reset(); };

	void prepare(PrepareSpecs ps)
	{
		voiceData.prepare(ps);

		if (sr != 0.0 && ps.sampleRate != 0.0)
		{
			auto factor = sr / ps.sampleRate;

			if (factor != 1.0)
			{
				voiceData.forEachVoice([factor](OscData& d)
				{
					d.uptimeDelta *= factor;
				});
			}
		}

		sr = ps.sampleRate;
	}

	bool handleModulation(double&) noexcept { return false; };

	void process(ProcessData& data)
	{
		auto stackData = (float*)alloca(data.size * sizeof(float));
		auto signal = stackData;

		int numSamples = data.size;

		while (--numSamples >= 0)
		{
			*stackData++ = (float)std::sin(voiceData.get().tick());
		}

		for (auto c : data)
		{
			FloatVectorOperations::add(c, signal, data.size);
		}
	}

	void processSingle(float* data, int numChannels)
	{
		auto newValue = (float)std::sin(voiceData.get().tick());

		for (int i = 0; i < numChannels; i++)
			data[i] += newValue;
 	}

	struct Display : public HiseDspBase::ExtraComponent<oscillator_impl>
	{
		Display(oscillator_impl* n, PooledUIUpdater* updater) :
			HiseDspBase::ExtraComponent<oscillator_impl>(n, updater)
		{
			p = f.createPath("sine");
			this->setSize(0, 50);
		};

		void paint(Graphics& g) override
		{
            auto h = this->getHeight();
			auto b = this->getLocalBounds().withSizeKeepingCentre(h * 2, h).toFloat();
			p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), true);
			GlobalHiseLookAndFeel::fillPathHiStyle(g, p, h * 2, h, false);
		}

		void timerCallback() override
		{
			auto thisMode = this->getObject()->currentMode;

			if (currentMode != thisMode)
			{
				currentMode = thisMode;

				auto pId = MarkdownLink::Helpers::getSanitizedFilename(this->getObject()->modes[currentMode]);
				p = f.createPath(pId);
				this->repaint();
			}
		}

		int currentMode = 0;
		WaveformComponent::WaveformFactory f;
		Path p;
	};

	Component* createExtraComponent(PooledUIUpdater* updater)
	{
		return new Display(this, updater);
	}

	double sr = 44100.0;
	PolyData<OscData, NumVoices> voiceData;

	int currentMode = 0;
};

using oscillator = oscillator_impl<1>;
using oscillator_poly = oscillator_impl<NUM_POLYPHONIC_VOICES>;

class gain : public HiseDspBase
{
public:

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_NODE_ID("gain");
	GET_SELF_AS_OBJECT(gain);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void prepare(PrepareSpecs ps)
	{
		sr = ps.sampleRate;

		if (ps.numChannels != gainers.size())
		{
			gainers.clear();
			for (int i = 0; i < ps.numChannels; i++)
			{
				gainers.add(new LinearSmoothedValue<float>());
			}
		}

		setSmoothingTime(smoothingTime * 1000.0);
	}

	void process(ProcessData& d)
	{
		auto channelToUse = d.data;

		for (auto g : gainers)
			g->applyGain(*channelToUse++, d.size);
	}

	forcedinline void reset() noexcept 
	{ 
		for (auto g : gainers)
		{
			g->reset(sr, smoothingTime);
			g->setValueWithoutSmoothing((float)gainValue);
		}
	};

	void processSingle(float* numFrames, int numChannels)
	{
		auto nextValue = gainers[0]->getNextValue();
		FloatVectorOperations::multiply(numFrames, nextValue, numChannels);
	}
	
	bool handleModulation(double&) noexcept { return false; };

	void createParameters(Array<ParameterData>& data) override
	{
		{
			ParameterData p("Gain");

			p.range = { -100.0, 0.0, 0.1 };
			p.range.setSkewForCentre(-12.0);
			p.defaultValue = 0.0;
			p.db = std::bind(&gain::setGain, this, std::placeholders::_1);
			
			data.add(std::move(p));
		}
		{
			ParameterData p("Smoothing");
			p.range = { 0.0, 1000.0, 0.1 };
			p.range.setSkewForCentre(100.0);
			p.defaultValue = 20.0;
			p.db = std::bind(&gain::setSmoothingTime, this, std::placeholders::_1);

			data.add(std::move(p));
		}
	}

	void setGain(double newValue)
	{
		gainValue = Decibels::decibelsToGain(newValue);

		for (auto g: gainers)
			g->setValue((float)gainValue);
	}

	void setSmoothingTime(double smoothingTimeMs)
	{
		smoothingTime = smoothingTimeMs * 0.001;
		
		reset();
			
	}

	double gainValue = 1.0;
	double sr = 0.0;
	double smoothingTime = 0.02;
	OwnedArray<LinearSmoothedValue<float>> gainers;
};

template <int V> class ramp_envelope_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = V;

	SET_HISE_NODE_EXTRA_HEIGHT(0);
	SET_HISE_POLY_NODE_ID("ramp_envelope");
	GET_SELF_AS_OBJECT(ramp_envelope_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	void prepare(PrepareSpecs ps)
	{
		gainer.prepare(ps);
		sr = ps.sampleRate;
		setRampTime(attackTimeSeconds);
	}

	void processSingle(float* data, int numChannels)
	{

	}

	void process(ProcessData& d)
	{
		auto& thisG = gainer.get();

		if (thisG.isSmoothing())
		{
			for (int i = 0; i < d.size; i++)
			{
				auto v = thisG.getNextValue();

				for (int c = 0; c < d.numChannels; c++)
					d.data[c][i] *= v;
			}

			d.shouldReset = false;
		}
		else
		{
			for (int c = 0; c < d.numChannels; c++)
			{
				FloatVectorOperations::multiply(d.data[c], thisG.getTargetValue(), d.size);
			}

			d.shouldReset = thisG.getCurrentValue() == 0.0;
		}
	}

	bool handleModulation(double&) { return false; }

	void handleHiseEvent(HiseEvent& e) final override
	{
		if (e.isNoteOnOrOff())
			gainer.get().setTargetValue(e.isNoteOn() ? 1.0f : 0.0f);
	}

	void setGate(double newValue)
	{
		gainer.getCurrentOrFirst().setTargetValue(newValue > 0.5 ? 1.0f : 0.0f);
	}

	void reset()
	{
		if (gainer.isVoiceRenderingActive())
			gainer.get().setValueWithoutSmoothing(0.0f);
		else
		{
			gainer.forEachVoice([](LinearSmoothedValue<float>& v)
			{
				v.setValueWithoutSmoothing(0.0f);
			});
		}
	}

	void setRampTime(double newAttackTimeMs)
	{
		attackTimeSeconds = newAttackTimeMs * 0.001;

		if (sr > 0.0)
		{
			if (gainer.isVoiceRenderingActive())
			{
				gainer.get().reset(sr, attackTimeSeconds);
			}
			else
			{
				auto sr_ = sr;
				auto as = attackTimeSeconds;
				gainer.forEachVoice([sr_, as](LinearSmoothedValue<float>& v)
				{
					v.reset(sr_, as);
				});
			}
		}
	}

	void createParameters(Array<ParameterData>& data) override
	{
		{
			ParameterData p("Gate");
			p.setParameterValueNames({ "On", "Off" });
			p.db = BIND_MEMBER_FUNCTION_1(ramp_envelope_impl::setGate);
			
			data.add(std::move(p));
		}

		{
			ParameterData p("Ramp Time");
			p.range = { 0.0, 2000.0, 0.1 };
			p.db = BIND_MEMBER_FUNCTION_1(ramp_envelope_impl::setRampTime);

			data.add(std::move(p));
		}
	}

	double attackTimeSeconds = 0.01;
	double sr = 0.0;
	PolyData<LinearSmoothedValue<float>, NumVoices> gainer;
};

using ramp_envelope = ramp_envelope_impl<1>;
using ramp_envelope_poly = ramp_envelope_impl<NUM_POLYPHONIC_VOICES>;

}

namespace math
{
	struct Operations
	{
#define SET_ID(x) static Identifier getId() { RETURN_STATIC_IDENTIFIER(#x); }
#define SET_DEFAULT(x) static constexpr float defaultValue = x;

		struct mul
		{
			SET_ID(mul); SET_DEFAULT(1.0f);

			static void op(ProcessData& d, float value)
			{
				for (int i = 0; i < d.numChannels; i++)
					FloatVectorOperations::multiply(d.data[i], value, d.size);
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
					*frameData++ *= value;
			}
		};
		
		struct add
		{
			SET_ID(add); SET_DEFAULT(0.0f);

			static void op(ProcessData& d, float value)
			{
				for (int i = 0; i < d.numChannels; i++)
					FloatVectorOperations::add(d.data[i], value, d.size);
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
					*frameData++ += value;
			}
		};
		
		struct clear
		{
			SET_ID(clear); SET_DEFAULT(0.0f);

			static void op(ProcessData& d, float)
			{
				for (int i = 0; i < d.numChannels; i++)
					FloatVectorOperations::clear(d.data[i], d.size);
			}

			static void opSingle(float* frameData, int numChannels, float)
			{
				for (int i = 0; i < numChannels; i++)
					*frameData++ = 0.0f;
			}
		};

		struct sub
		{
			SET_ID(sub); SET_DEFAULT(0.0f);

			static void op(ProcessData& d, float value)
			{
				for (int i = 0; i < d.numChannels; i++)
					FloatVectorOperations::add(d.data[i], -value, d.size);
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
					*frameData++ -= value;
			}
		};

		struct div
		{
			SET_ID(div); SET_DEFAULT(1.0f);

			static void op(ProcessData& d, float value)
			{
				auto factor = value > 0.0f ? 1.0f / value : 0.0f;
				for (int i = 0; i < d.numChannels; i++)
					FloatVectorOperations::multiply(d.data[i], factor, d.size);
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
					*frameData++ /= value;
			}
		};
		
		struct tanh
		{
			SET_ID(tanh); SET_DEFAULT(1.0f);

			static void op(ProcessData& d, float value)
			{
				for (int i = 0; i < d.numChannels; i++)
				{
					auto ptr = d.data[i];

					for (int j = 0; j < d.size; j++)
						ptr[i] = std::tanhf(ptr[i] * value);
				}
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
                    frameData[i] = std::tanhf(frameData[i] * value);
			}
		};

		struct pi
		{
			SET_ID(pi); SET_DEFAULT(2.0f);

			static void op(ProcessData& d, float value)
			{
				for (auto ptr : d)
					FloatVectorOperations::multiply(ptr, float_Pi * value, d.size);
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
					*frameData++ *= float_Pi * value;
			}
		};

		struct sin
		{
			SET_ID(sin); SET_DEFAULT(2.0f);

			static void op(ProcessData& d, float)
			{
				for (auto ptr : d)
				{
					for (int i = 0; i < d.size; i++)
						ptr[i] = std::sin(ptr[i]);
				}
			}

			static void opSingle(float* frameData, int numChannels, float)
			{
				for (int i = 0; i < numChannels; i++)
					frameData[i] = std::sin(frameData[i]);
			}
		};

		struct sig2mod
		{
			SET_ID(sig2mod); SET_DEFAULT(0.0f);

			static void op(ProcessData& d, float)
			{
				for (auto& ch: d.channels())
					for (auto& s : ch)
						s = s * 0.5f + 0.5f;
			}

			static void opSingle(float* frameData, int numChannels, float)
			{
				for (int i = 0; i < numChannels; i++)
					frameData[i] = frameData[i] * 0.5f + 0.5f;
			}
		};
		
		struct clip
		{
			SET_ID(clip); SET_DEFAULT(1.0f);

			static void op(ProcessData& d, float value)
			{
				for (auto ptr: d)
					FloatVectorOperations::clip(ptr, ptr, -value, value, d.size);
			}

			static void opSingle(float* frameData, int numChannels, float value)
			{
				for (int i = 0; i < numChannels; i++)
					frameData[i] = jlimit(-value, value, frameData[i]);
			}
		};

		struct abs
		{
			SET_ID(abs); SET_DEFAULT(0.0f);

			static void op(ProcessData& d, float)
			{
				for (int i = 0; i < d.numChannels; i++)
					FloatVectorOperations::abs(d.data[i], d.data[i], d.size);
			}

			static void opSingle(float* frameData, int numChannels, float)
			{
				for (int i = 0; i < numChannels; i++)
					frameData[i] = frameData[i] > 0.0f ? frameData[i] : frameData[i] * -1.0f;
			}
		};
	};


	template <class OpType> class OpNode : public HiseDspBase
	{
	public:

		SET_HISE_NODE_ID(OpType::getId());
		SET_HISE_NODE_EXTRA_HEIGHT(0);
		GET_SELF_AS_OBJECT(OpNode<OpType>);
		SET_HISE_NODE_IS_MODULATION_SOURCE(false);

		bool handleModulation(double&) noexcept { return false; };

		void process(ProcessData& d)
		{
			OpType::op(d, value);
		}

		void processSingle(float* frameData, int numChannels)
		{
			OpType::opSingle(frameData, numChannels, value);
		}

		forcedinline void reset() noexcept {}

		void prepare(PrepareSpecs)
		{}

		void createParameters(Array<ParameterData>& data) override
		{
			ParameterData p("Value");
			p.range = { 0.0, 1.0, 0.01 };
			p.defaultValue = OpType::defaultValue;

			p.db = [this](double newValue)
			{
				value = (float)newValue;
			};

			data.add(std::move(p));
		}

		float value = OpType::defaultValue;
	};

	using mul = OpNode<Operations::mul>;
	using add = OpNode<Operations::add>;
	using sub = OpNode<Operations::sub>;
	using div = OpNode<Operations::div>;
	using tanh = OpNode<Operations::tanh>;
	using clip = OpNode<Operations::clip>;
	using sin = OpNode<Operations::sin>;
	using pi = OpNode<Operations::pi>;
	using sig2mod = OpNode<Operations::sig2mod>;
	using abs = OpNode<Operations::abs>;
	using clear = OpNode<Operations::clear>;

	class MathFactory : public NodeFactory
	{
	public:

		MathFactory(DspNetwork* n) :
			NodeFactory(n)
		{
			registerNode<HiseDspNodeBase<math::mul>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::add>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::clear>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::sub>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::div>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::tanh>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::clip>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::sin>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::pi>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::sig2mod>>>();
			registerNode<HiseDspNodeBase<OpNode<Operations::abs>>>();
		};

		Identifier getId() const override { return "math"; }
	};
}

}
