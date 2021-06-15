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



namespace core
{

template <int NV> struct file_player : public data::base
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

	static constexpr bool isPolyphonic() { return NumVoices > 1; }

	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_MOD;
	SN_DESCRIPTION("A simple file player with multiple playback modes");

	file_player() = default;

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
		
	}

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		if (auto b = dynamic_cast<MultiChannelAudioBuffer*>(d.obj))
			b->setDisabledXYZProviders({ scriptnode::data::XYZSampleMapProvider::getStaticId(), scriptnode::data::XYZSFZProvider::getStaticId() });

		base::setExternalData(d, index);

		if(lastSpecs)
			prepare(lastSpecs);

		for (OscData& s : state)
		{
			s.uptime = 0.0;
			s.uptimeDelta = 0.0;
		}
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
					
					//externalData.setDisplayedValue((double)(int)idx);

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
		if (mode != PlaybackModes::SignalInput)
		{
			auto& s = state.get();

			if (e.isNoteOn())
			{
				auto& cd = getCurrentAudioSample();

				if (this->externalData.getStereoSample(cd, e))
					s.uptimeDelta = cd.getPitchFactor();

				if (mode == PlaybackModes::MidiFreq)
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

}

}
