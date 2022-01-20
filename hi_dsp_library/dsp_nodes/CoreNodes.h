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
using namespace snex;
using namespace snex::Types;


#if 0
namespace container
{

template <class P, typename... Ts> using frame1_block = wrap::frame<1, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame2_block = wrap::frame<2, container::chain<P, Ts...>>;
template <class P, typename... Ts> using frame4_block = wrap::frame<4, container::chain<P, Ts...>>;
template <class P, typename... Ts> using framex_block = wrap::frame_x< container::chain<P, Ts...>>;
template <class P, typename... Ts> using oversample2x = wrap::oversample<2,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample4x = wrap::oversample<4,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample8x = wrap::oversample<8,   container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using oversample16x = wrap::oversample<16, container::chain<P, Ts...>, init::oversample>;
template <class P, typename... Ts> using modchain = wrap::control_rate<chain<P, Ts...>>;

template <class P, typename... Ts> using oversample = wrap::oversample<-1,   container::chain<P, Ts...>, init::oversample>;

}
#endif



namespace core
{



struct table: public scriptnode::data::base
{
	SET_HISE_NODE_ID("table");
	SN_GET_SELF_AS_OBJECT(table);
	SN_DESCRIPTION("a (symmetrical) lookup table based waveshaper");

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_SET_PARAMETER;
	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_CREATE_PARAM;

	using TableSpanType = span<float, SAMPLE_LOOKUP_TABLE_SIZE>;

	bool isPolyphonic() const { return false; };
	static constexpr bool isNormalisedModulation() { return true; }

	bool handleModulation(double& value)
	{
		return currentValue.getChangedValue(value);
	}

	void prepare(PrepareSpecs ps)
	{
		smoothedValue.prepare(ps.sampleRate, 20.0);
	}

	void reset() noexcept
	{
		currentValue.setModValue(0.0);
		smoothedValue.reset();
	}

	TableSpanType* getTableData()
	{
		return reinterpret_cast<TableSpanType*>(externalData.data);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
	{
		DataReadLock l(this);

		if (!tableData.isEmpty())
		{
			float v = 0.0f;

			for (auto& ch : data)
			{
				for (auto& s : data.toChannelData(ch))
				{
					processFloat(s);
				}
			}

			externalData.setDisplayedValue(v);
		}
	}

	void processFloat(float& s)
	{
		InterpolatorType ip(hmath::abs(s));
		s *= tableData[ip];
	}

	void setExternalData(const ExternalData& d, int) override
	{
		base::setExternalData(d, 0);
		d.referBlockTo(tableData, 0);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
	{
		DataReadLock l(this);

		if (!tableData.isEmpty())
		{
			for(auto& s: data)
				processFloat(s);
		}
	}

	sfloat smoothedValue;
	ModValue currentValue;
	block tableData;

	using TableClampType = index::clamped<SAMPLE_LOOKUP_TABLE_SIZE, false>;
	using InterpolatorType = index::lerp<index::normalised<float, TableClampType>>;

	JUCE_DECLARE_WEAK_REFERENCEABLE(table);
};

class peak: public data::display_buffer_base<true>
{
public:

	SET_HISE_NODE_ID("peak");
	SN_GET_SELF_AS_OBJECT(peak);
	SN_DESCRIPTION("create a modulation signal from the input peak");

	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;

	bool isPolyphonic() const { return false; }

	bool handleModulation(double& value)
	{
		value = max;
		return true;
	}

	void reset() noexcept;;

	static constexpr bool isNormalisedModulation() { return true; }

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		snex::hmath Math;

		max = 0.0f;

		for (auto& ch : data)
		{
			auto range = FloatVectorOperations::findMinAndMax(data.toChannelData(ch).begin(), data.getNumSamples());
			max = jmax<float>(max, Math.abs(range.getStart()), Math.abs(range.getEnd()));
		}

		updateBuffer(max, data.getNumSamples());
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		snex::hmath Math;

		max = 0.0;

		for (auto& s : data)
			max = Math.max(max, Math.abs((double)s));

		updateBuffer(max, 1);
	}

	// This is no state variable, so we don't need it to be polyphonic...
	double max = 0.0;
};

class recorder: public data::base
{
public:

	enum class RecordingState
	{
		Idle,
		Recording,
		WaitingForStop,
		numStates
	};

	enum class Parameters
	{
		State,
		RecordingLength
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(State, recorder);
		DEF_PARAMETER(RecordingLength, recorder);
	}
	

	SET_HISE_NODE_ID("recorder");
	SN_GET_SELF_AS_OBJECT(recorder);
	SN_DESCRIPTION("Record the signal input into a audio file slot");

	static constexpr bool isPolyphonic() { return false; }

	HISE_EMPTY_MOD;
	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_INITIALISE;

	void reset()
	{
		recordingIndex = 0;
	}

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		if (updater == nullptr)
		{
			if (auto gu = d.obj->getUpdater().getGlobalUIUpdater())
				updater = new InternalUpdater(*this, gu);
		}

		base::setExternalData(d, index);
	}

	void prepare(PrepareSpecs ps)
	{
		lastSpecs = ps;

		if (updater != nullptr)
			updater->resizeFlag.store(true);
	}

	void setState(double state)
	{
		auto thisState = (state > 0.5) ? RecordingState::Recording : RecordingState::Idle;

		if (currentState != thisState)
		{
			currentState = thisState;
			recordingIndex = 0;
		}
	}

	void setRecordingLength(double lengthInMilliseconds)
	{
		bufferSize = lengthInMilliseconds / 1000.0 * lastSpecs.sampleRate;

		if (updater != nullptr)
			updater->resizeFlag.store(true);
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(recorder, State);
			p.setParameterValueNames({ "On", "Off" });
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(recorder, RecordingLength);
			p.setRange({ 0.0, 2000.0, 0.1 });
			data.add(std::move(p));
		}
	}

	void flush()
	{
		SimpleReadWriteLock::ScopedReadLock l(bufferLock);

		if (auto af = dynamic_cast<MultiChannelAudioBuffer*>(externalData.obj))
		{
			af->loadBuffer(recordingBuffer, lastSpecs.sampleRate);
		}

		currentState = RecordingState::Idle;
	}

	void rebuildBuffer()
	{
		AudioSampleBuffer newBuffer(lastSpecs.numChannels, bufferSize);
		newBuffer.clear();

		{
			SimpleReadWriteLock::ScopedWriteLock l(bufferLock);
			std::swap(newBuffer, recordingBuffer);
			recordingIndex = 0;
		}
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (currentState == RecordingState::Recording)
		{
			SimpleReadWriteLock::ScopedReadLock l(bufferLock);

			if (d.getNumChannels() == 2)
				FrameConverters::forwardToFrameStereo(this, d);
			if (d.getNumChannels() == 1)
				FrameConverters::forwardToFrameMono(this, d);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		int numSamplesInBuffer = recordingBuffer.getNumSamples();

		if (currentState == RecordingState::Recording && isPositiveAndBelow(recordingIndex, numSamplesInBuffer))
		{
			for (int i = 0; i < data.size(); i++)
				recordingBuffer.setSample(i, recordingIndex, data[i]);

			recordingIndex++;
		}


		if (recordingIndex++ >= numSamplesInBuffer)
		{
			recordingIndex = 0;
			currentState = RecordingState::WaitingForStop;

			if (updater != nullptr)
				updater->flushFlag.store(true);
		}
	}

private:

	struct InternalUpdater : public PooledUIUpdater::SimpleTimer
	{
		InternalUpdater(recorder& p, PooledUIUpdater* u) :
			SimpleTimer(u),
			parent(p)
		{};

		void timerCallback() override
		{
			if (resizeFlag)
				refreshBufferSize();

			if (flushFlag)
				flushBuffer();
		}

		void flushBuffer()
		{
			parent.flush();
			flushFlag.store(false);
		}

		void refreshBufferSize()
		{
			parent.rebuildBuffer();
			resizeFlag.store(false);
		}

		std::atomic<bool> resizeFlag = false;
		std::atomic<bool> flushFlag = false;

		recorder& parent;
	};

	ScopedPointer<InternalUpdater> updater;

	int recordingIndex = 0;
	int bufferSize = 0;
	RecordingState currentState = RecordingState::Idle;
	PrepareSpecs lastSpecs;
	SimpleReadWriteLock bufferLock;
	AudioSampleBuffer recordingBuffer;
};

class mono2stereo : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("mono2stereo");
	SN_GET_SELF_AS_OBJECT(mono2stereo);
	SN_DESCRIPTION("converts a mono signal to a stereo signal (`L->L+R`)");

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_RESET;
	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_SET_PARAMETER;
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (data.getNumChannels() >= 2)
        {
            auto dst = data[1];
            Math.vcopy(dst, data[0]);
        }
			
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (data.size() >= 2)
			data[1] = data[0];
	}

	hmath Math;
};

class empty : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("empty");
	SN_GET_SELF_AS_OBJECT(empty);

	HISE_EMPTY_PREPARE;
	HISE_EMPTY_CREATE_PARAM;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_RESET;
	HISE_EMPTY_HANDLE_EVENT;
};



template <class ShaperType> struct snex_shaper
{
	SET_HISE_NODE_ID("snex_shaper");
	SN_GET_SELF_AS_OBJECT(snex_shaper);
	SN_DESCRIPTION("A custom waveshaper using SNEX");

	HISE_EMPTY_HANDLE_EVENT;
	
	snex_shaper()
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::TemplateArgumentIsPolyphonic);
	}

	void prepare(PrepareSpecs ps)
	{
		shaper.prepare(ps);
	}

	void reset()
	{
		shaper.reset();
	};

	void initialise(NodeBase* n)
	{
		if constexpr (prototypes::check::initialise<ShaperType>::value)
			shaper.initialise(n);
	}

	static constexpr bool isPolyphonic() { return false; }

	ShaperType shaper;

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		shaper.process(data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		shaper.processFrame(data);
	}

	void setExternalData(const ExternalData& d, int index)
	{
		if constexpr (prototypes::check::setExternalData<ShaperType>::value)
			shaper.setExternalData(d, index);
	}

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto t = static_cast<snex_shaper<ShaperType>*>(obj);
		t->shaper.template setParameter<P>(v);
	}
    PARAMETER_MEMBER_FUNCTION;

	HISE_EMPTY_CREATE_PARAM;
};

template <int NV, bool UseRingBuffer=false> class ramp : public data::display_buffer_base<UseRingBuffer>
{
public:

	enum class Parameters
	{
		PeriodTime,
		LoopStart,
		Gate,
		numParameters
	};

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("ramp");
	SN_GET_SELF_AS_OBJECT(ramp);
	SN_DESCRIPTION("Creates a ramp signal that can be used as modulation source");

	ramp()
	{
		setPeriodTime(100.0);

		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsPolyphonic);
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UseRingBuffer);
	}

	void prepare(PrepareSpecs ps)
	{
		state.prepare(ps);

		sr = ps.sampleRate;
		setPeriodTime(periodTime);
	}

	void reset() noexcept
	{
		for (auto& s : state)
		{
			s.data.reset();
		}
	}

	HISE_EMPTY_INITIALISE;

	static constexpr bool isNormalisedModulation() { return true; };

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(PeriodTime, ramp);
		DEF_PARAMETER(LoopStart, ramp);
		DEF_PARAMETER(Gate, ramp);
	}
	PARAMETER_MEMBER_FUNCTION;

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& thisState = state.get();

		if (thisState.enabled)
		{
			double thisUptime = thisState.data.uptime;
			double thisDelta = thisState.data.uptimeDelta;

			for (auto c : d)
			{
				thisUptime = thisState.data.uptime;

				for (auto& s : d.toChannelData(c))
				{
					if (thisUptime > 1.0)
						thisUptime = thisState.loopStart;

					s += (float)thisUptime;

					thisUptime += thisDelta;
				}
			}

			thisState.data.uptime = thisUptime;
			thisState.modValue.setModValue(thisUptime);
		}

		this->updateBuffer(thisState.data.uptime, d.getNumSamples());
	}

	bool handleModulation(double& v)
	{
		return state.get().modValue.getChangedValue(v);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto& s = state.get();

		if (s.enabled)
		{
			auto newValue = s.data.tick();

			if (newValue > 1.0)
			{
				newValue = s.loopStart;
				s.data.uptime = newValue;
			}

			for (auto& s : data)
				s += (float)newValue;
			
			s.modValue.setModValue(newValue);

			this->updateBuffer(newValue, 1);
		}
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(ramp, PeriodTime);
			p.setRange({ 0.1, 1000.0, 0.1 });
			p.setDefaultValue(100.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(ramp, LoopStart);
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(ramp, Gate);
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
	}

	HISE_EMPTY_HANDLE_EVENT;

	void setGate(double onOffValue)
	{
		bool shouldBeOn = onOffValue > 0.5;

		for (auto& s : state)
		{
			if (s.enabled != shouldBeOn)
			{
				s.enabled = shouldBeOn;
				s.data.uptime = 0.0;
			}
		}
	}

	void setPeriodTime(double periodTimeMs)
	{
		if (periodTimeMs > 0.0)
		{
			periodTime = periodTimeMs;

			if (sr > 0.0)
			{
				auto s = periodTime * 0.001;
				auto inv = 1.0 / jmax(0.00001, s);

				auto newUptimeDelta = jmax(0.0000001, inv / sr);

				for (auto& s : state)
					s.data.uptimeDelta = newUptimeDelta;
			}
		}
	}

	void setLoopStart(double newLoopStart)
	{
		auto v = jlimit(0.0, 1.0, newLoopStart);

		for (auto& d : state)
			d.loopStart = v;
	}

private:

	struct State
	{
		OscData data;
		double loopStart = 0.0;
		bool enabled = false;
		ModValue modValue;
	};

	double sr = 44100.0;
	double periodTime = 500.0;
	PolyData<State, NumVoices> state;
};


template <int NV> class oscillator: public OscillatorDisplayProvider,
								    public polyphonic_base
{
public:

	enum class Parameters
	{
		Mode,
		Frequency,
		PitchMultiplier,
		Gate,
		Phase,
		Gain,
		numParameters
	};

	constexpr static int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("oscillator");
	SN_GET_SELF_AS_OBJECT(oscillator);
	SN_DESCRIPTION("A tone generator with multiple waveforms");

	HISE_EMPTY_INITIALISE;

	oscillator(): polyphonic_base(getStaticId()) {}

	constexpr bool isProcessingHiseEvent() const { return true; }

	void reset()
	{
		for (auto& s : voiceData)
			s.reset();
	}

	void prepare(PrepareSpecs ps);
	
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		currentVoiceData = &voiceData.get();

		if (currentVoiceData->enabled == 0)
			return;

		if (data.getNumChannels() == 2)
		{
			auto fd = data.template as<ProcessData<2>>().toFrameData();
			while (fd.next())
				processFrame(fd.toSpan());
		}
		else
		{
			for (auto& s : data[0])
			{
				auto asSpan = reinterpret_cast<span<float, 1>*>(&s);
				processFrame(*asSpan);
			}
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		if (currentVoiceData == nullptr)
			currentVoiceData = &voiceData.get();

		if (currentVoiceData->enabled == 0)
			return;

        float v = 0.0f;

		auto g = currentVoiceData->gain;

		switch (currentMode)
		{
		case Mode::Sine:	 v = g * tickSine(*currentVoiceData); break;
		case Mode::Triangle: v = g * tickTriangle(*currentVoiceData); break;
		case Mode::Saw:		 v = g * tickSaw(*currentVoiceData); break;
		case Mode::Square:	 v = g * tickSquare(*currentVoiceData); break;
		case Mode::Noise:	 v = g * (Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
        default: break;
		}
        
		for (auto& s : data)
			s += v;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			setFrequency(e.getFrequency());
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(oscillator, Mode);
			p.setParameterValueNames(modes);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(oscillator, Frequency);
			p.setRange({ 20.0, 20000.0, 0.1 });
			p.setDefaultValue(220.0);
			p.setSkewForCentre(1000.0);
			data.add(std::move(p));
		}
		{
			parameter::data p("Freq Ratio");
			p.setRange({ 1.0, 16.0, 1.0 });
			p.setDefaultValue(1.0);
			p.callback = parameter::inner<oscillator, (int)Parameters::PitchMultiplier>(*this);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(oscillator, Gate);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(oscillator, Phase);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(oscillator, Gain);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
	}

	void setMode(double newMode)
	{
		currentMode = (Mode)(int)newMode;

		if (auto o = this->externalData.obj)
			o->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
	}

	void setFrequency(double newFrequency)
	{
		freqValue = newFrequency;

		if (sr > 0.0)
		{
			auto newUptimeDelta = (double)(newFrequency / sr * (double)sinTable->getTableSize());

			uiData.uptimeDelta = newUptimeDelta;

			for (auto& d : voiceData)
				d.uptimeDelta = newUptimeDelta;
		}
	}

	void setGate(double v)
	{
		auto shouldBeOn = (int)(v > 0.5);

		for (auto& d : voiceData)
		{
			auto shouldReset = shouldBeOn && !d.enabled;

			if (shouldReset)
				d.uptime = 0.0;

			d.enabled = shouldBeOn;
		}
	}

	void setPhase(double v)
	{
		v *= (double)sinTable->getTableSize();

		uiData.phase = v;

		for (auto& s : voiceData)
			s.phase = v;

		if (auto o = this->externalData.obj)
			o->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
	}

	void setGain(double gain)
	{
		uiData.gain = gain;

		for (auto& s : voiceData)
			s.gain = gain;

		if (auto o = this->externalData.obj)
			o->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
	}

	void setPitchMultiplier(double newMultiplier)
	{
		auto pitchMultiplier = jlimit(0.001, 100.0, newMultiplier);

		for (auto& d : voiceData)
			d.multiplier = pitchMultiplier;

		uiData.multiplier = pitchMultiplier;

		if (auto o = this->externalData.obj)
			o->getUpdater().sendDisplayChangeMessage(0.0f, sendNotificationAsync, true);
	}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Mode, oscillator);
		DEF_PARAMETER(Frequency, oscillator);
		DEF_PARAMETER(PitchMultiplier, oscillator);
		DEF_PARAMETER(Gate, oscillator);
		DEF_PARAMETER(Gain, oscillator);
		DEF_PARAMETER(Phase, oscillator);
	}

	PARAMETER_MEMBER_FUNCTION;

	double sr = 44100.0;
	PolyData<OscData, NumVoices> voiceData;

	OscData* currentVoiceData = nullptr;

	double freqValue = 220.0;
	
};

template class oscillator<1>;
template class oscillator<NUM_POLYPHONIC_VOICES>;

template <int NV> struct file_player : public data::base,
                                       public polyphonic_base
{
    static constexpr int NumVoices = NV;

    SET_HISE_NODE_ID("file_player");
    SN_GET_SELF_AS_OBJECT(file_player);

    enum class PlaybackModes
    {
        Static,
        SignalInput,
        MidiFreq
    };

    enum class Parameters
    {
        PlaybackMode,
        Gate,
        RootFrequency,
        FreqRatio
    };

    DEFINE_PARAMETERS
    {
        DEF_PARAMETER(PlaybackMode, file_player);
        DEF_PARAMETER(Gate, file_player);
        DEF_PARAMETER(RootFrequency, file_player);
        DEF_PARAMETER(FreqRatio, file_player);
    }
    PARAMETER_MEMBER_FUNCTION;

    static constexpr bool isPolyphonic() { return NumVoices > 1; }

    HISE_EMPTY_INITIALISE;
    HISE_EMPTY_MOD;
    SN_DESCRIPTION("A simple file player with multiple playback modes");

    file_player(): polyphonic_base(getStaticId()) {};
    
    void prepare(PrepareSpecs specs)
    {
        lastSpecs = specs;

        auto fileSampleRate = this->externalData.sampleRate;

        if (lastSpecs.sampleRate > 0.0)
            globalRatio = fileSampleRate / lastSpecs.sampleRate;

        state.prepare(specs);
        currentXYZSample.prepare(specs);


        reset();
    }

    void reset()
    {
        for (auto& s : state)
        {
            if (mode != PlaybackModes::MidiFreq)
            {
                auto& cd = *currentXYZSample.begin();

                HiseEvent e(HiseEvent::Type::NoteOn, 64, 1, 1);

                if (this->externalData.getStereoSample(cd, e))
                    s.uptimeDelta = cd.getPitchFactor();

                

                s.uptime = 0.0;
            }
        }
    }

    void setExternalData(const snex::ExternalData& d, int index) override
    {
        base::setExternalData(d, index);

        if(lastSpecs)
            prepare(lastSpecs);

        for (OscData& s : state)
        {
            s.uptime = 0.0;
            s.uptimeDelta = 0.0;
        }

        reset();
    }

    PolyData<StereoSample, NUM_POLYPHONIC_VOICES> currentXYZSample;

    StereoSample& getCurrentAudioSample()
    {
        return currentXYZSample.get();
    }

    template <int C> void processFix(ProcessData<C>& data)
    {
        if (auto dt = DataTryReadLock(this))
        {
            auto& s = getCurrentAudioSample();

            if (!externalData.isEmpty() && !s.data[0].isEmpty())
            {
                auto fd = data.toFrameData();

                
                auto maxIndex = (double)s.data[0].size();

                if (mode == PlaybackModes::SignalInput)
                {
                    auto pos = jlimit(0.0, 1.0, (double)data[0][0]);
                    externalData.setDisplayedValue(pos * maxIndex);

                    while (fd.next())
                        processWithSignalInput(fd.toSpan());
                }
                else
                {

                    using IndexType = index::unscaled<double, index::looped<0>>;

                    IndexType i(state.get().uptime);
                    i.setLoopRange(s.loopRange[0], s.loopRange[1]);
                    auto uptime = i.getIndex(maxIndex, 0);
                    externalData.setDisplayedValue(uptime);

                    while (fd.next())
                        processWithPitchRatio(fd.toSpan());
                }
            }
            else if (mode == PlaybackModes::SignalInput)
            {
                for (auto& ch : data)
                {
                    auto b = data.toChannelData(ch);
                        FloatVectorOperations::clear(b.begin(), b.size());
                }
            };
        }
    }

    template <typename ProcessDataType> void process(ProcessDataType& data) noexcept
    {
        if constexpr (!ProcessDataType::hasCompileTimeSize())
        {
            if (data.getNumChannels() == 2)
                processFix(data.template as<ProcessData<2>>());
            if (data.getNumChannels() == 1)
                processFix(data.template as<ProcessData<1>>());
        }
        else
        {
            processFix(data);
        }
    }

    template <int C> void processWithSignalInput(span<float, C>& data)
    {
        using IndexType = index::normalised<float, index::clamped<0, true>>;
        using InterpolatorType = index::lerp<IndexType>;

        auto s = data[0];

        InterpolatorType ip(s);

        auto fd = getCurrentAudioSample()[ip];

        for (int i = 0; i < data.size(); i++)
            data[i] = fd[i];
    }

    template <int C> void processWithPitchRatio(span<float, C>& data)
    {
        using IndexType = index::unscaled<double, index::looped<0>>;
        using InterpolatorType = index::lerp<IndexType>;

        OscData& s = state.get();

        if (s.uptimeDelta != 0)
        {
            auto uptime = s.tick();

            auto& cs = getCurrentAudioSample();

            InterpolatorType ip(uptime * globalRatio);
            ip.setLoopRange(cs.loopRange[0], cs.loopRange[1]);

            auto fd = cs[ip];

            data += fd;
        }
    }

    template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept
    {
        if (auto dt = DataTryReadLock(this))
        {
            auto& cd = getCurrentAudioSample().data;
            int numSamples = cd[0].size();

            switch (mode)
            {
            case PlaybackModes::SignalInput:
            {
                if (numSamples == 0)
                    data = 0.0f;
                else
                {
                    if (frameUpdateCounter++ >= 1024)
                    {
                        frameUpdateCounter = 0;
                        auto pos = jlimit(0.0, 1.0, (double)data[0]);
                        externalData.setDisplayedValue(pos * (double)(numSamples));
                    }

                    processWithSignalInput(data);
                }

                break;
            }
            case PlaybackModes::Static:
            case PlaybackModes::MidiFreq:
            {
                if (frameUpdateCounter++ >= 1024)
                {
                    frameUpdateCounter = 0;
                    externalData.setDisplayedValue(hmath::fmod(state.get().uptime * globalRatio, (double)numSamples));
                }

                processWithPitchRatio(data);
                break;
            }
            }
        }
    }

    void handleHiseEvent(HiseEvent& e)
    {
        if (mode == PlaybackModes::MidiFreq)
        {
            auto& s = state.get();

            if (e.isNoteOn())
            {
                auto& cd = getCurrentAudioSample();

                if (this->externalData.getStereoSample(cd, e))
                    s.uptimeDelta = cd.getPitchFactor();
                else
                    s.uptimeDelta = e.getFrequency() / rootFreq;

                s.uptime = 0.0;
            }
        }
    }

    void setPlaybackMode(double v)
    {
        mode = (PlaybackModes)(int)v;

        reset();
    }

    void setRootFrequency(double rv)
    {
        rootFreq = jlimit(20.0, 8000.0, rv);
    }

    void setFreqRatio(double fr)
    {
        for (OscData& s : state)
            s.multiplier = fr;
    }

    void setGate(double v)
    {
        bool on = v > 0.5;

        if (on)
        {
            for (OscData& s : state)
            {
                s.uptimeDelta = 1.0;
                s.uptime = 0.0;
            }
        }
        else
        {
            for (OscData& s : state)
                s.uptimeDelta = 0.0;
        }
    }

    void createParameters(ParameterDataList& d)
    {
        {
            DEFINE_PARAMETERDATA(file_player, PlaybackMode);
            p.setParameterValueNames({ "Static", "Signal in", "MIDI" });
            d.add(p);
        }
        {
            DEFINE_PARAMETERDATA(file_player, Gate);
            p.setRange({ 0.0, 1.0, 1.0 });
            p.setDefaultValue(1.0f);
            d.add(p);
        }
        {
            DEFINE_PARAMETERDATA(file_player, RootFrequency);
            p.setRange({ 20.0, 2000.0 });
            p.setDefaultValue(440.0);
            d.add(p);
        }
        {
            DEFINE_PARAMETERDATA(file_player, FreqRatio);
            p.setRange({ 0.0, 2.0, 0.01 });
            p.setDefaultValue(1.0f);
            d.add(p);
        }
    }

private:

    double globalRatio = 1.0;
    double rootFreq = 440.0;

    int frameUpdateCounter = 0;
    PlaybackModes mode;

    PolyData<OscData, NumVoices> state;
    PrepareSpecs lastSpecs;
};

class fm : public HiseDspBase
{
public:

	enum class Parameters
	{
		Frequency,
		Modulator,
		FreqMultiplier,
		Gate
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, fm);
		DEF_PARAMETER(Modulator, fm);
		DEF_PARAMETER(FreqMultiplier, fm);
		DEF_PARAMETER(Gate, fm);
	}
	PARAMETER_MEMBER_FUNCTION;

	SET_HISE_NODE_ID("fm");
	SN_GET_SELF_AS_OBJECT(fm);
	SN_DESCRIPTION("A FM oscillator that uses the signal input as FM source");

	constexpr bool isProcessingHiseEvent() const { return true; }

	bool isPolyphonic() const { return true; }

	void prepare(PrepareSpecs ps);
	void reset();

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		if (oscData.get().enabled == 0)
			return;

		FrameConverters::forwardToFrameMono(this, data);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& od = oscData.get();

		if (!od.enabled)
			return;

		double modValue = (double)d[0];
		d[0] = sinTable->getInterpolatedValue(od.tick());
		od.uptime += modGain.get() * modValue;
	}

	void createParameters(ParameterDataList& data) override;

	void handleHiseEvent(HiseEvent& e);
	
	void setFreqMultiplier(double input);
	void setModulator(double newGain);
	void setFrequency(double newFrequency);
	void setGate(double v);

private:

	double sr = 0.0;
	double freqMultiplier = 1.0;

	PolyData<OscData, NUM_POLYPHONIC_VOICES> oscData;
	PolyData<double, NUM_POLYPHONIC_VOICES> modGain;

	SharedResourcePointer<SineLookupTable<2048>> sinTable;
};

template <int V> class gain_impl : public HiseDspBase
{
public:

	enum class Parameters
	{
		Gain,
		Smoothing,
		ResetValue
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Gain, gain_impl);
		DEF_PARAMETER(Smoothing, gain_impl);
		DEF_PARAMETER(ResetValue, gain_impl);
	}
	PARAMETER_MEMBER_FUNCTION;

	static constexpr int NumVoices = V;

	SET_HISE_POLY_NODE_ID("gain");
	SN_GET_SELF_AS_OBJECT(gain_impl);
	SN_DESCRIPTION("A gain module with decibel range and parameter smoothing");

	void prepare(PrepareSpecs ps)
	{
		gainer.prepare(ps);
		sr = ps.sampleRate;

		setSmoothing(smoothingTime);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto gainFactor = gainer.get().advance();

		for (auto& s : data)
			s *= gainFactor;
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& thisGainer = gainer.get();

		if (thisGainer.isActive())
			FrameConverters::forwardToFrame16(this, data);
		else
		{
			auto gainFactor = thisGainer.get();

			for (auto ch : data)
				data.toChannelData(ch) *= gainFactor;
		}
	}

	void reset() noexcept
	{
		if (sr == 0.0)
			return;

		for (auto& g : gainer)
		{
			g.set(resetValue);
			g.reset();
			g.set((float)gainValue);
		}
	}

	void createParameters(ParameterDataList& data) override
	{
		{
			DEFINE_PARAMETERDATA(gain_impl, Gain);
			p.setRange({ -100.0, 0.0, 0.1 });
			p.setSkewForCentre(-12.0);
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(gain_impl, Smoothing);
			p.setRange({ 0.0, 1000.0, 0.1 });
			p.setSkewForCentre(100.0);
			p.setDefaultValue(20.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(gain_impl, ResetValue);
			p.setRange({ -100.0, 0.0, 0.1 });
			p.setSkewForCentre(-12.0);
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
	}


	void setGain(double newValue)
	{
		gainValue = Decibels::decibelsToGain(newValue);

		float gf = (float)gainValue;

		for (auto& g : gainer)
			g.set(gf);
	}

	void setSmoothing(double smoothingTimeMs)
	{
		smoothingTime = smoothingTimeMs;

		if (sr <= 0.0)
			return;

		for (auto& g : gainer)
			g.prepare(sr, smoothingTime);
	}

	void setResetValue(double newResetValue)
	{
		resetValue = Decibels::decibelsToGain(newResetValue);
	}

	double gainValue = 1.0;
	double sr = 0.0;
	double smoothingTime = 0.02;
	double resetValue = 0.0;

	PolyData<sfloat, NumVoices> gainer;
};

DEFINE_EXTERN_NODE_TEMPLATE(gain, gain_poly, gain_impl);



template <int NV> class smoother: public mothernode
{
public:

	enum class Parameters
	{
		SmoothingTime,
		DefaultValue
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(SmoothingTime, smoother);
		DEF_PARAMETER(DefaultValue, smoother);
	}
	PARAMETER_MEMBER_FUNCTION;

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("smoother");
	SN_GET_SELF_AS_OBJECT(smoother);
	SN_DESCRIPTION("Smoothes the input signal using a low pass filter");

	smoother() {};

	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_MOD;
    
	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(smoother, DefaultValue);
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(smoother, SmoothingTime);
			p.setRange({ 0.0, 2000.0, 0.1 });
			p.setSkewForCentre(100.0);
			p.setDefaultValue(100.0);
			data.add(std::move(p));
		}
	}

	void prepare(PrepareSpecs ps) 
	{
		smoothers.prepare(ps);
		auto sr = ps.sampleRate;
		auto sm = smoothingTimeMs;

		for (auto& s : smoothers)
		{
			s.prepareToPlay(sr);
			s.setSmoothingTime((float)sm);
		}
	}

	void reset()
	{
		auto d = defaultValue;

		for (auto& s : smoothers)
			s.resetToValue(d, 0.0);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		data[0] = smoothers.get().smooth(data[0]);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		smoothers.get().smoothBuffer(data[0].data, data.getNumSamples());
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			reset();
	}

	void setSmoothingTime(double newSmoothingTime)
	{
		smoothingTimeMs = newSmoothingTime;

		for (auto& s : smoothers)
			s.setSmoothingTime((float)newSmoothingTime);
	}

	void setDefaultValue(double newDefaultValue)
	{
		defaultValue = (float)newDefaultValue;

		auto d = defaultValue; auto sm = smoothingTimeMs;

		for (auto& s : smoothers)
			s.resetToValue((float)d, (float)sm);
	}

	double smoothingTimeMs = 100.0;
	float defaultValue = 0.0f;
	PolyData<hise::Smoother, NumVoices> smoothers;
};


template <typename T> struct snex_osc_base: public mothernode
{
	void initialise(NodeBase* n)
	{
		if constexpr (prototypes::check::initialise<T>::value)
			oscType.initialise(n);
	}

	void setExternalData(const ExternalData& d, int index)
	{
		if constexpr (prototypes::check::setExternalData<T>::value)
			oscType.setExternalData(d, index);
	}

	void prepare(PrepareSpecs ps)
	{
		if constexpr (prototypes::check::prepare<T>::value)
			oscType.prepare(ps);
	}

	T oscType;
};

template <int NV, typename T> struct snex_osc : public snex_osc_base<T>,
												public polyphonic_base
{
	enum class Parameters
	{
		Frequency,
		PitchMultiplier
	};

	static constexpr int NumVoices = NV;

	constexpr bool isProcessingHiseEvent() const { return true; }

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Frequency, snex_osc);
		DEF_PARAMETER(PitchMultiplier, snex_osc);

		if (P > 1)
		{
			auto typed = static_cast<snex_osc*>(obj);
			typed->oscType.template setParameter<P - 2>(value);
		}
	}

	PARAMETER_MEMBER_FUNCTION;

	SET_HISE_POLY_NODE_ID("snex_osc");
	SN_GET_SELF_AS_OBJECT(snex_osc);
	SN_DESCRIPTION("A custom oscillator node using SNEX");

	snex_osc() :
		polyphonic_base(getStaticId(), true)
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::TemplateArgumentIsPolyphonic);
	};

	void prepare(PrepareSpecs ps)
	{
		snex_osc_base<T>::prepare(ps);
		sampleRate = ps.sampleRate;
		oscData.prepare(ps);
		reset();
		setFrequency(lastFreq);
	}

	void reset()
	{
		for (auto& o : oscData)
			o.reset();
	}

	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		auto& thisData = oscData.get();
		auto uptime = thisData.tick();
		data[0] += this->oscType.tick(uptime);
	}

	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		auto& thisData = oscData.get();

		OscProcessData op;

		op.data.referToRawData(data.getRawDataPointers()[0], data.getNumSamples());
		op.uptime = thisData.uptime;
		op.delta = thisData.uptimeDelta * thisData.multiplier;

		this->oscType.process(op);
		thisData.uptime += op.delta * (double)data.getNumSamples();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			setFrequency(e.getFrequency());
	}

	void setFrequency(double newValue)
	{
		lastFreq = newValue;

		if (sampleRate > 0.0)
		{
			auto cyclesPerSecond = newValue;
			auto cyclesPerSample = cyclesPerSecond / sampleRate;

			for (auto& o : oscData)
				o.uptimeDelta = cyclesPerSample;
		}
	}

	void setPitchMultiplier(double newMultiplier)
	{
		newMultiplier = jlimit(0.01, 100.0, newMultiplier);

		for (auto& o : oscData)
			o.multiplier = newMultiplier;
	}

	double sampleRate = 0.0;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(snex_osc, Frequency);
			p.setRange({ 20.0, 20000.0, 0.1 });
			p.setSkewForCentre(1000.0);
			p.setDefaultValue(220.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(snex_osc, PitchMultiplier);
			p.setRange({ 1.0, 16.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}
	}

	double lastFreq = 0.0;
	PolyData<OscData, NumVoices> oscData;
};

} // namespace core




}
