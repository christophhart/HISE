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

namespace modulation
{

/** The base class for all modulation target nodes in scriptnode that pickup the downsampled modulation signal
 *  and expose it as sample-accurate modulation source (or signal source) within the scriptnode patch. */
template <int NV,                                    // the number of voices
	      class IndexClass,                          // the index class (from runtime_target::indexers) 
	      runtime_target::RuntimeTarget TargetType,  // the runtime target type (eg. RuntimeTarget::GlobalModulator)
	      class ConfigClass>                         // a config class from modulation::config that defines the behaviour

struct mod_base: public data::display_buffer_base<ConfigClass::shouldEnableDisplayBuffer()>,
				 public polyphonic_base,
				 public runtime_target::indexable_target<IndexClass, TargetType, SignalSource>
{
public:

	static constexpr int DisplayBufferDownsamplingFactor = 32;
	static constexpr int NumVoices = NV;
	static constexpr bool isPolyphonic() { return NumVoices > 1; };
	static constexpr bool isNormalisedModulation() { return ConfigClass::isNormalisedModulation(); };
	static constexpr bool isProcessingHiseEvent() { return true; }

	mod_base(const Identifier& id):
	  polyphonic_base(id, true)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsFixRuntimeTarget);
		
		if(!ConfigClass::isNormalisedModulation())
			cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::UseUnnormalisedModulation);
	}

	virtual ~mod_base()
	{
		this->disconnect();
	};

	enum class SignalRatio
	{
		Uninitialised,
		AudioRate,
		ControlRate,
		Incompatible
	};

	void initialise(NodeBase* n)
	{
		config.initialise(n);
	}
	
	void onConnectionChange() override {}

	void onValue(SignalSource currentModSignals) override
	{
		signal = currentModSignals;
		checkSignalRatio();
	}

	void handleHiseEvent(const HiseEvent& e)
	{
		if(e.isNoteOn())
		{
			auto& s = state.get();
			s.startVoice(e, signal, index, state.isPolyHandlerEnabled());
			modValue.setModValue(s.lastRampValue);
		}
	}

	void prepare(snex::Types::PrepareSpecs ps) override
	{
		config.prepare(ps);

		lastSpecs = ps;
		checkSignalRatio();

		if(sr == SignalRatio::Incompatible)
			scriptnode::Error::throwError(Error::SampleRateMismatch, 
									      signal.sampleRate_cr * HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR, 
									      lastSpecs.sampleRate);

		state.prepare(ps);

		for(auto& s: state)
			s.prepare(ps, sr, config.shouldProcessSignal());
		
		reset();
	}

	void checkSignalRatio()
	{
		sr = SignalRatio::Uninitialised;

		if(lastSpecs && signal)
		{
			if(lastSpecs.sampleRate == signal.sampleRate_cr)
				sr = SignalRatio::ControlRate;
			else if((lastSpecs.sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR) == signal.sampleRate_cr)
				sr = SignalRatio::AudioRate;
			else
				sr = SignalRatio::Incompatible;
		}

		ok = sr == SignalRatio::AudioRate || sr == SignalRatio::ControlRate;
	}

	bool handleModulation(double& v)
	{
		return modValue.getChangedValue(v);
	} 

	void applyModulation(float* data, int numSamples, sfloat& baseValue, sfloat& intensity)
	{
		if (config.getMode() == TargetMode::Gain)
		{
			for(int i = 0; i < numSamples; i++)
			{
				auto in = intensity.advance();
				auto bv = baseValue.advance();
				const auto a = bv - in;

				if(config.useMidPositionAsZero())
					data[i] -= 0.5f;

				data[i] = jlimit(0.0f, 1.0f, a + in * data[i] * bv);

				if(config.useMidPositionAsZero())
					data[i] += 0.5f;

			}
		}
		else if (config.getMode() == TargetMode::Bipolar)
		{
			for(int i = 0; i < numSamples; i++)
			{

				auto mv = data[i];
				mv = 2.0f * mv - 1.0;
				mv *= intensity.advance();
				data[i] = jlimit(0.0f, 1.0f, baseValue.advance() + mv);
			}
		}
		else if (config.getMode() == TargetMode::Unipolar)
		{
			for(int i = 0; i < numSamples; i++)
			{
				auto mv = data[i] * intensity.advance();
				data[i] = jlimit(0.0f, 1.0f, baseValue.advance() + mv);
			}
		}
		else if (config.getMode() == TargetMode::Pitch)
			return; 		// do nothing
		else if (config.getMode() == TargetMode::Raw)
			return;
		else if (config.getMode() == TargetMode::Aux)
		{
			for(int i = 0; i < numSamples; i++)
			{
				auto in = intensity.advance();
				auto bv = baseValue.advance();
				auto v = (1.0f - in) + in * data[i];
				data[i] = v * bv;
			}

			return;
		}

		if(numSamples == 1)
			data[0] = jlimit(0.0f, 1.0f, data[0]);
		else
			FloatVectorOperations::clip(data, data, 0.0f, 1.0f, numSamples);	
	}

	template <typename FD> void processFrame(FD& data)
	{
		if(ok)
		{
			jassert(signal);

			auto& s = state.get();
			VariableStorage mv = s.eventData.getReadPointer();

			float v = 0.0f;

			auto t = mv.getType();

			if(t == Types::ID::Block)
			{
				auto bl = mv.toBlock();

				if(sr == SignalRatio::AudioRate)
				{
					auto alpha = (float)(s.controlRateSubIndex++ / (float)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR);

					auto v1 = s.lastRampValue;
					auto v2 = bl[s.uptime];

					if(s.controlRateSubIndex == HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR)
					{
						s.controlRateSubIndex = 0;
						s.lastRampValue = bl[s.uptime];
						s.uptime++;
					}

					v = Interpolator::interpolateLinear(v1, v2, alpha);
					
				}
				else
					v = mv.toBlock()[s.uptime++];

				if(s.uptime == mv.toBlock().size())
					s.uptime = 0;

				applyModulation(&v, 1, s.baseValue, s.intensity);
			}
			else if (t == Types::ID::Float)
			{
				v = mv.toFloat();
				applyModulation(&v, 1, s.baseValue, s.intensity);
			}
			else if (t == Types::ID::Void)
				v = s.baseValue.advance();
			
			modValue.setModValue(v);

			if constexpr (ConfigClass::shouldEnableDisplayBuffer())
			{
				if(state.isFirst() && ++displayUpdateCounter == (DisplayBufferDownsamplingFactor * 2))
				{
					displayUpdateCounter = 0;
					// writing a single sample will trigger another 1024 sample watchdog...
					this->updateBuffer(v, 2); 
				}
			}
				
			if(config.shouldProcessSignal())
				data[0] = v;
		}
	}

	template <typename PD> void process(PD& data)
	{
		if constexpr (PD::hasCompileTimeSize())
		{
			if(data.getNumSamples() == 1)
			{
				constexpr static int NumChannels = PD::getNumFixedChannels();

				span<float, NumChannels> frameData;

				if(config.shouldProcessSignal())
				{
					for(int i = 0; i < NumChannels; i++)
						frameData[i] = data.getRawChannelPointers()[i][0];
				}

				this->processFrame(frameData);
				return;
			}
		}

		if(ok)
		{
			jassert(signal);

			auto& s = state.get();
			auto mv = s.eventData.getReadPointer();
			auto t = mv.getType();

			if(config.shouldProcessSignal())
			{
				auto numToCopy = data.getNumSamples();
				// the mod chain most likely only has a single channel
				block target = data[0];

				if(t == Types::ID::Float)
				{
					auto v = mv.toFloat();
					s.lastRampValue = v;
					FloatVectorOperations::fill(target.begin(), s.lastRampValue, numToCopy);
					applyModulation(target.begin(), data.getNumSamples(), s.baseValue, s.intensity);
				}
				else if (t == Types::ID::Block)
				{
					auto signal = mv.toBlock();
					const float* ptr = signal.begin() + s.uptime;
					int uptimeDelta = 0;

					if(sr == SignalRatio::ControlRate)
					{
						// We can directly copy the signal to the control rate target signal
						FloatVectorOperations::copy(target.begin(), ptr, numToCopy);
						uptimeDelta = numToCopy;
						s.lastRampValue = data[0][data.getNumSamples() - 1];
					}
					else // sr == SignalRatio::AudioRate
					{
						// Expand it using the AlignedRamper tool class
						float* target = data[0].begin();
						uptimeDelta = data.getNumSamples() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
						FloatVectorOperations::copy(target, ptr, uptimeDelta);

						if(!ModBufferExpansion::expand(target, 0, data.getNumSamples(), s.lastRampValue))
							FloatVectorOperations::fill(target, s.lastRampValue, data.getNumSamples());
					}
					
					s.uptime += uptimeDelta;

					jassert(s.uptime <= signal.size());

					if(s.uptime == signal.size())
						s.uptime = 0;

					applyModulation(target.begin(), data.getNumSamples(), s.baseValue, s.intensity);
				}
				else if (t == Types::ID::Void)
				{
					for(int i = 0; i < numToCopy; i++)
						target[i] = s.baseValue.advance();
				}

				if(ConfigClass::shouldEnableDisplayBuffer() && state.isFirst())
				{
					auto numToWrite = jmax(2, data.getNumSamples() / DisplayBufferDownsamplingFactor);
					this->updateBuffer(target[0], numToWrite);

				}
					

				modValue.setModValue(target[0]);
			}
			else
			{
				float v = 0.0f;

				if(t == Types::ID::Float)
				{
					v = mv.toFloat();
					applyModulation(&v, 1, s.baseValue, s.intensity);
				}
				else if (t == Types::ID::Block)
				{
					auto signal = mv.toBlock();
					int uptimeDelta = 0;

					if(sr == SignalRatio::ControlRate)
						uptimeDelta = data.getNumSamples();
					else // sr == SignalRatio::AudioRate
						uptimeDelta = jmax(1, data.getNumSamples() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR);

					v = signal[s.uptime];

					s.uptime += uptimeDelta;

					if(s.uptime == signal.size())
						s.uptime = 0;

					applyModulation(&v, 1, s.baseValue, s.intensity);
				}
				else if (t == Types::ID::Void)
					v = s.baseValue.advance();

				if(ConfigClass::shouldEnableDisplayBuffer() && state.isFirst())
				{
					auto numToWrite = jmax(2, data.getNumSamples() / DisplayBufferDownsamplingFactor);
					this->updateBuffer(v, numToWrite);
				}
				
				modValue.setModValue(v);
			}
		}
	}
	
	void setIndex(double newValue)
	{
		index = jlimit(-1, NumMaxModulationSources, roundToInt(newValue));
		reset();
	}

	void reset()
	{
		auto wantsPoly = state.isPolyHandlerEnabled();

		for(auto& s: state)
			s.reset(signal, index, wantsPoly);
	}

	void setProcessSignal(double newValue)
	{
		config.setProcessSignal(newValue > 0.5);
	}

	void setValue(double newValue)
	{
		for(auto& s: state)
			s.baseValue.set(jlimit(-1.0f, 1.0f, (float)newValue));
	}

	void setMode(double newValue)
	{
		auto idx = roundToInt(newValue);
		config.setMode((TargetMode)idx);
	}

	void setIntensity(double intensity)
	{
		for(auto& s: state)
			s.intensity.set(jlimit(-1.0f, 1.0f, (float)intensity));
	}

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		data::display_buffer_base<ConfigClass::shouldEnableDisplayBuffer()>::setExternalData(d, index);

		if (auto rb = dynamic_cast<SimpleRingBuffer*>(d.obj))
		{
			if (auto mp = dynamic_cast<ModPlotter::ModPlotterPropertyObject*>(rb->getPropertyObject().get()))
			{
				mp->setProperty("BufferLength", 65536 / DisplayBufferDownsamplingFactor);
			}
		}
	}

	struct Data
	{
		void startVoice(const HiseEvent& e, const SignalSource& signal, int slotIndex, bool wantsPolyphonicSignal)
		{
			eventData = signal.getEventData(slotIndex, e, wantsPolyphonicSignal);

			if(wantsPolyphonicSignal)
				uptime = e.getTimeStamp() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

			if(isPolyphonic())
			{
				intensity.reset();
				baseValue.reset();
			}

			auto v = eventData.getReadPointer();

			if(v.getType() == Types::ID::Float)
			{
				lastRampValue = v.toFloat();
			}
			else if (v.getType() == Types::ID::Block)
			{
				auto bl = v.toBlock();
				jassert(isPositiveAndBelow(uptime, bl.size()));
				lastRampValue = bl[uptime];
			}
		}

		void prepare(PrepareSpecs ps, SignalRatio r, bool process)
		{
			auto sr = ps.sampleRate;
			auto smoothingTime = 20.0;

			if(!process)
				sr /= ps.blockSize;

			baseValue.prepare(sr, smoothingTime);
			intensity.prepare(sr, smoothingTime);
		}

		void reset(const SignalSource& signal, int slotIndex, bool wantsPolyphonicSignal)
		{
			eventData = signal.getEventData(slotIndex, {}, wantsPolyphonicSignal);
			baseValue.reset();
			intensity.reset();
			lastRampValue = baseValue.get();
			uptime = 0;
			controlRateSubIndex = 0;
		}
		
		float lastRampValue = 0.0;
		sfloat baseValue;
		sfloat intensity;
		EventData eventData;
		int uptime = 0;
		int controlRateSubIndex = 0; // used for frame processing
	};

	const EventData& getCurrentEventData() const { return state.get().eventData; }

	void setUseMidPositionAsZero(bool shouldUse)
	{
		config.setUseMidPositionAsZero(shouldUse);
	}

private:

	ConfigClass config;

	int displayUpdateCounter = 0;

	PrepareSpecs lastSpecs;
	SignalSource signal;
	int index = 0;

	PolyData<Data, NV> state;
	SignalRatio sr = SignalRatio::Uninitialised;
	bool ok = false;
	ModValue modValue;
};

}

namespace core {

template <int NV, class IndexClass, class ConfigClass, runtime_target::RuntimeTarget TargetType> class indexable_mod_base:
	public modulation::mod_base<NV, IndexClass, TargetType, ConfigClass>
{
public:

	static constexpr int NumVoices = NV;

	indexable_mod_base(const Identifier& id):
	  modulation::mod_base<NV, IndexClass, TargetType, ConfigClass>(id)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::NeedsModConfig);
	};
};


template <int NV, class IndexClass, class ConfigClass> class global_mod: 
	public indexable_mod_base<NV, IndexClass, ConfigClass, runtime_target::RuntimeTarget::GlobalModulator>
{
public:

	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("global_mod");
	SN_GET_SELF_AS_OBJECT(global_mod);

	global_mod():
	  indexable_mod_base<NV, IndexClass, ConfigClass, runtime_target::RuntimeTarget::GlobalModulator>(getStaticId())
	{
		cppgen::CustomNodeProperties::addNodeIdManually(getStaticId(), PropertyIds::NeedsModConfig);
	};

	enum Parameters
	{
		Index,
		Value,
		ProcessSignal,
		Mode,
		Intensity
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Index, global_mod);
		DEF_PARAMETER(Value, global_mod);
		DEF_PARAMETER(ProcessSignal, global_mod);
		DEF_PARAMETER(Mode, global_mod);
		DEF_PARAMETER(Intensity, global_mod);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(global_mod, Index);
			p.setRange({ 0.0, 16.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(global_mod, Value);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(global_mod, ProcessSignal);
			p.setParameterValueNames({ "Disabled", "Enabled"});
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(global_mod, Mode);
			p.setParameterValueNames({ "Gain", "Unipolar", "Bipolar"});
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(global_mod, Intensity);
			p.setRange({-1.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}

	}
};


#define EIndex modulation::config::ExtraIndexer
#define EConfig modulation::config::extra_config

template <int NV, class IndexClass=EIndex, class ConfigClass=EConfig> class extra_mod: 
	public indexable_mod_base<NV, IndexClass, ConfigClass, runtime_target::RuntimeTarget::ExternalModulatorChain>
{
public:

	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("extra_mod");
	SN_GET_SELF_AS_OBJECT(extra_mod);
	
	extra_mod():
	  indexable_mod_base<NV, IndexClass, ConfigClass, runtime_target::RuntimeTarget::ExternalModulatorChain>(getStaticId())
	{
		cppgen::CustomNodeProperties::addNodeIdManually(getStaticId(), PropertyIds::NeedsModConfig);

		static_assert(std::is_same<IndexClass, EIndex>(), "must be the extra indexer");
	};

	enum Parameters
	{
		Index,
		ProcessSignal
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Index, extra_mod);
		DEF_PARAMETER(ProcessSignal, extra_mod);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(extra_mod, Index);
			p.setRange({ 0.0, 16.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(extra_mod, ProcessSignal);
			p.setParameterValueNames({ "Disabled", "Enabled"});
			data.add(std::move(p));
		}
	}
};

#undef EConfig
#undef EIndex

#define PIndex modulation::config::PitchIndexer
#define PConfig modulation::config::pitch_config

template <int NV, class Indexer=PIndex, class ConfigClass=PConfig> struct pitch_mod:
	public modulation::mod_base<NV, modulation::config::PitchIndexer, runtime_target::RuntimeTarget::ExternalModulatorChain, ConfigClass>
{
	using Base = modulation::mod_base<NV, PIndex, runtime_target::RuntimeTarget::ExternalModulatorChain, ConfigClass>;

	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("pitch_mod");
	SN_DESCRIPTION("Picks up the pitch modulation signal from the parent sound generator");
	SN_GET_SELF_AS_OBJECT(pitch_mod);
	
	pitch_mod():
	  Base(getStaticId())
	{
		static_assert(std::is_same<Indexer, PIndex>(), "must be the pitch indexer");
	};

	enum Parameters
	{
		ProcessSignal
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(ProcessSignal, pitch_mod);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(pitch_mod, ProcessSignal);
			p.setParameterValueNames({ "Disabled", "Enabled"});
			data.add(std::move(p));
		}
	}

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		Base::setExternalData(d, index);

		if (auto rb = dynamic_cast<SimpleRingBuffer*>(d.obj))
		{
			if (auto mp = dynamic_cast<ModPlotter::ModPlotterPropertyObject*>(rb->getPropertyObject().get()))
			{
				mp->transformFunction = ModBufferExpansion::pitchFactorToNormalisedRange;
			}
		}
	}

};

#undef PIndex
#undef PConfig

#define MIndex runtime_target::indexers::fix_hash<1>


template <int NV> struct matrix_mod:
	public data::display_buffer_base<true>,
	public polyphonic_base,
	public runtime_target::indexable_target<MIndex, runtime_target::RuntimeTarget::GlobalModulator, modulation::SignalSource>
{
	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("matrix_mod");
	SN_GET_SELF_AS_OBJECT(matrix_mod);
	SN_DESCRIPTION("A modulator that connects to global modulators with the feature set of the modulation matrix")

	static constexpr bool isNormalisedModulation() { return true; }
	static constexpr bool isProcessingHiseEvent() { return true; }

	matrix_mod():
	  polyphonic_base(getStaticId(), true)
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsFixRuntimeTarget);
	}

	void onValue(modulation::SignalSource value) override
	{
		sourceMod.onValue(value);
		auxMod.onValue(value);
	}

	void prepare(PrepareSpecs ps) override
	{
		lastSpecs = ps;
		sourceMod.prepare(ps);
		auxMod.prepare(ps);

		if(sourceBuffer.size() < lastSpecs.blockSize)
			sourceBuffer.setSize(ps.blockSize);
		if(auxBuffer.size() < lastSpecs.blockSize)
			auxBuffer.setSize(ps.blockSize);

		sptr = sourceBuffer.begin();
		aptr = auxBuffer.begin();
	}

	void handleHiseEvent(const HiseEvent& e)
	{
		lastVoiceIndex = lastSpecs.voiceIndex->getVoiceIndex();
		sourceMod.handleHiseEvent(e);
		auxMod.handleHiseEvent(e);
	}

	void calculateVoiceStart(float& startValue)
	{
		double dv;
		double av;
		sourceMod.handleModulation(dv);
		auxMod.handleModulation(av);
		float dv_f = (float)dv;
		float av_f = (float)av;
		applyModulation(&startValue, &dv_f, &av_f, 1);
	}

	void reset()
	{
		sourceMod.reset();
		auxMod.reset();
	}

	bool handleModulation(double& v)
	{
		return mv.getChangedValue(v);
	}

	template <typename PD> void process(PD& data)
	{
		auto sd = makeProcessData<false>(data.getNumSamples());
		auto ad = makeProcessData<true>(data.getNumSamples());
		sourceMod.process(sd);
		auxMod.process(ad);
		applyModulation(data[0].begin(), sptr, aptr, data.getNumSamples());

		if(lastSpecs.voiceIndex->getVoiceIndex() == lastVoiceIndex)
			updateBuffer(sptr, data.getNumSamples());

		mv.setModValue(sptr[0]);
	}

	template <typename FD> void processFrame(FD& data)
	{
		span<float, 1> sd;
		span<float, 1> ad;

		sourceMod.processFrame(sd);
		auxMod.processFrame(ad);
		applyModulation(data.begin(), sd.begin(), ad.begin(), 1);
		updateBuffer(sd[0], 1);
		mv.setModValue(sd[0]);
	}

	void applyModulation(float* dst, float* src, float* intensity, int numSamples)
	{
		if(inverted)
		{
			for(int i = 0; i < numSamples; i++)
				src[i] = 1.0f - src[i];
		}

		if (tm == modulation::TargetMode::Gain)
		{
			for(int i = 0; i < numSamples; i++)
			{
				auto in = intensity[i];

				auto mv = (1.0f - in) + in * src[i];

				src[i] = mv;

				dst[i] -= zeroDelta;
				dst[i] *= mv;
				dst[i] += zeroDelta;
			}
		}
		else if (tm == modulation::TargetMode::Bipolar)
		{
			for(int i = 0; i < numSamples; i++)
			{
				auto mv = src[i];
				mv = 2.0f * mv - 1.0;
				src[i] = mv * intensity[i];
				dst[i] += src[i];
			}
		}
		else if (tm == modulation::TargetMode::Unipolar)
		{
			for(int i = 0; i < numSamples; i++)
			{
				src[i] *= intensity[i]; 
				dst[i] += src[i];
			}
		}
	}

	enum Parameters
	{
		SourceIndex,
		Intensity,
		Mode,
		Inverted,
		AuxIndex,
		AuxIntensity,
		ZeroPosition
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(SourceIndex, matrix_mod);
		DEF_PARAMETER(Intensity, matrix_mod);
		DEF_PARAMETER(Mode, matrix_mod);
		DEF_PARAMETER(Inverted, matrix_mod);
		DEF_PARAMETER(AuxIndex, matrix_mod);
		DEF_PARAMETER(AuxIntensity, matrix_mod);
		DEF_PARAMETER(ZeroPosition, matrix_mod);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	void setSourceIndex(double newValue)
	{
		sourceMod.setIndex(newValue);
	}

	void setAuxIndex(double newValue)
	{
		auxMod.setIndex(newValue);
	}

	void setIntensity(double newValue)
	{
		auxMod.setValue(newValue);
	}

	void setMode(double mode)
	{
		tm = (modulation::TargetMode)roundToInt(mode);
	}

	void setInverted(double value)
	{
		inverted = value > 0.5;
	}

	void setAuxIntensity(double intensity)
	{
		auxMod.setIntensity(intensity);
	}

	void setZeroPosition(double zeroPos)
	{
		zeroDelta = zeroPos > 0.5 ? 0.5f : 0.0f;
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(matrix_mod, SourceIndex);
			p.setRange({ -1.0, (double)modulation::NumMaxModulationSources, 1.0});
			p.setDefaultValue(-1.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(matrix_mod, Intensity);
			p.setRange({ -1.0, 1.0 });
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(matrix_mod, Mode);
			p.setParameterValueNames({ "Scale", "Unipolar", "Bipolar" });
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(matrix_mod, Inverted);
			p.setParameterValueNames({ "Normal", "Inverted"});
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(matrix_mod, AuxIndex);
			p.setRange({ -1.0, (double)modulation::NumMaxModulationSources, 1.0});
			p.setDefaultValue(-1.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(matrix_mod, AuxIntensity);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(matrix_mod, ZeroPosition);
			p.setRange({ 0.0, 1.0 });
			p.setParameterValueNames({ "Left", "Center"});
			data.add(std::move(p));
		}
	}

	bool isPlaying() const
	{
		auto& ed = sourceMod.getCurrentEventData();

		if(ed.type == modulation::SourceType::Envelope)
		{
			return ed.thisBlockSize != nullptr && *ed.thisBlockSize > 0;
		}

		return true;
	}

	float zeroDelta = 0.0f;;

private:

	int lastVoiceIndex = -1;

	modulation::TargetMode tm = modulation::TargetMode::Gain;

	template <bool Aux> ProcessData<1> makeProcessData(int numSamples)
	{
		return ProcessData<1>(Aux ? &aptr : &sptr, numSamples, 1);
	}

	PrepareSpecs lastSpecs;
	heap<float> sourceBuffer;
	heap<float> auxBuffer;
	float* sptr = nullptr;
	float* aptr = nullptr;

	template <modulation::TargetMode M> struct internal_config
	{
		static constexpr bool shouldEnableDisplayBuffer() { return false; }
		static constexpr bool isNormalisedModulation() { return true; }
		static constexpr modulation::TargetMode getMode() { return M; };
		static constexpr bool shouldProcessSignal() { return true; }
		bool useMidPositionAsZero() const { return false; }
		void setUseMidPositionAsZero(bool ) {}

		SN_EMPTY_PREPARE;
		SN_EMPTY_INITIALISE;
	};

	global_mod<NV, MIndex, internal_config<modulation::TargetMode::Raw>> sourceMod;
	global_mod<NV, MIndex, internal_config<modulation::TargetMode::Aux>> auxMod;

	ModValue mv;
	bool inverted = false;
	
};

#undef MIndex

} // namespace core

}
