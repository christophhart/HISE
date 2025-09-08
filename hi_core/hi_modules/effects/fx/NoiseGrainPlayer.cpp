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

namespace hise { using namespace juce;

#if USE_BACKEND
struct NoiseGrainPlayerEditor: public ProcessorEditorBody
{
	NoiseGrainPlayerEditor(ProcessorEditor* parent):
	  ProcessorEditorBody(parent),
	  tableIndex("Position"),
	  mixSlider("Mix"),
	  whiteNoise("WhiteNoise"),
	  grainSize("GrainSize")
	{
		auto np = dynamic_cast<NoiseGrainPlayer*>(parent->getProcessor());
		thumbnail.setAudioFile(&np->getBuffer());

		tableIndex.setup(np, NoiseGrainPlayer::Parameters::Position, "Position");
		tableIndex.setMode(HiSlider::Mode::NormalizedPercentage);
		tableIndex.setRange(0.0, 1.0, 0.0);
		tableIndex.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
		tableIndex.setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);

		mixSlider.setup(np, NoiseGrainPlayer::Parameters::Mix, "Mix");
		mixSlider.setMode(HiSlider::Mode::NormalizedPercentage);
		mixSlider.setRange(0.0, 1.0, 0.0);
		mixSlider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
		mixSlider.setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);

		whiteNoise.setup(np, NoiseGrainPlayer::Parameters::WhiteNoise, "WhiteNoise");
		whiteNoise.setMode(HiSlider::Mode::NormalizedPercentage);
		whiteNoise.setRange(0.0, 1.0, 0.0);
		whiteNoise.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
		whiteNoise.setTextBoxStyle (Slider::TextBoxRight, false, 80, 20);

		grainSize.setup(np, NoiseGrainPlayer::Parameters::GrainSize, "GrainSize");
		grainSize.addItemList({ "256 samples", "512 samples", "1024 samples", "2048 samples", "4096 samples", "8192 Samples" }, 1);

		addAndMakeVisible(tableIndex);
		addAndMakeVisible(mixSlider);
		addAndMakeVisible(whiteNoise);
		addAndMakeVisible(thumbnail);
		addAndMakeVisible(grainSize);
	}

	int getBodyHeight() const override
	{
		return 250;
	}

	void updateGui() override
	{
		tableIndex.updateValue();
		mixSlider.updateValue();
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(20);
		auto sb = b.removeFromLeft(128);

		tableIndex.setBounds(sb.removeFromTop(48));
		sb.removeFromTop(10);
		mixSlider.setBounds(sb.removeFromTop(48));
		sb.removeFromTop(10);
		whiteNoise.setBounds(sb.removeFromTop(48));

		sb.removeFromTop(10);
		grainSize.setBounds(sb.removeFromTop(28));
		b.removeFromLeft(10);
		thumbnail.setBounds(b);
	}

	MultiChannelAudioBufferDisplay thumbnail;
	HiSlider tableIndex, mixSlider, whiteNoise;
	HiComboBox grainSize;
};
#endif

NoiseGrainPlayer::NoiseGrainPlayer(MainController* mc, const String& uid, int numVoices):
  VoiceEffectProcessor(mc, uid, numVoices),
  AudioSampleProcessor(mc)
{
	getBuffer().addListener(this);

	modChains += {this, "Table Index Modulation", ModulatorChain::ModulationType::Normal, Modulation::GainMode};
	modChains += {this, "Table Index Bipolar", ModulatorChain::ModulationType::Normal, Modulation::OffsetMode};
	
	finaliseModChains();

	modChains[InternalChains::PositionBipolar].setIncludeMonophonicValuesInVoiceRendering(true);
	modChains[InternalChains::PositionBipolar].setIncludeMonophonicValuesInVoiceRendering(true);

	parameterNames.add("Position");
	parameterNames.add("Mix");
	parameterNames.add("WhiteNoise");
	parameterNames.add("GrainSize");
	updateParameterSlots();
}

ProcessorEditorBody *NoiseGrainPlayer::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new NoiseGrainPlayerEditor(parentEditor);
#else
	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

void NoiseGrainPlayer::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	if(auto sl = hise::SimpleReadWriteLock::ScopedTryReadLock(grainLock))
	{
		auto t = jlimit(0.0f, 1.0f, tableIndex);
		auto m = mixGain;

		auto tg = modChains[InternalChains::PositionChain].getOneModulationValue(startSample);
		auto tb = modChains[InternalChains::PositionBipolar].getOneModulationValue(startSample);

		t *= tg;
		t += tb;

		auto g1 = scriptnode::faders::overlap().getFadeValue<0>(2, jlimit(0.0, 1.0, (double)m));
		auto g2 = scriptnode::faders::overlap().getFadeValue<1>(2, jlimit(0.0, 1.0, (double)m));

		b.applyGain(startSample, numSamples, g1);

		voiceState[voiceIndex].render(b, startSample, numSamples, uptimeDelta, noiseGrains, t, g2);

		if(voiceIndex == lastStartedVoiceIndex)
		{
			getBuffer().sendDisplayIndexMessage(t * getBuffer().getCurrentRange().getLength());
		}
	}
}

void NoiseGrainPlayer::Grain::reset()
{
	grainData = nullptr;
	uptime = 0.0;
}

bool NoiseGrainPlayer::Grain::start(const AudioSampleBuffer& gd, double noiseAmp_)
{
	grainData = &gd;
	uptime = 0.0;
	noiseAmp = noiseAmp_ * 10.0;
	
	return isActive();
}

int NoiseGrainPlayer::Grain::apply(AudioSampleBuffer& b, int startSample, int numSamples, double uptimeDelta, float gain)
{
	if(!isActive())
		return 0;

	for(int i = 0; i < numSamples; i++)
	{
		auto tUptime = uptime + noiseAmp * (r.nextDouble() - 0.5);

		tUptime = hmath::wrap(tUptime, (double)grainData->getNumSamples() -1);

		int lo = (int)tUptime;
		int hi = lo + 1;
		float alpha = (float)tUptime - (float)lo;

		if(uptime >= grainData->getNumSamples())
		{
			grainData = nullptr;
			return i;
		}

		for(int c = 0; c < b.getNumChannels(); c++)
		{
			auto gc = c % grainData->getNumChannels();
			auto v1 = grainData->getSample(gc, lo);
			auto v2 = grainData->getSample(gc, hi);
			auto v = Interpolator::interpolateLinear(v1, v2, alpha);
			b.addSample(c, i + startSample, gain * v);
		}

		uptime += uptimeDelta;
	}

	return 0;
}

void NoiseGrainPlayer::VoiceState::reset()
{
	lastHopIndex = -1;
	lastStartedIndex = -1;
	
	for(auto& g: grains)
		g.reset();
}

void NoiseGrainPlayer::VoiceState::render(AudioSampleBuffer& b, int startSample, int numSamples, double uptimeDelta,
                                          const NoiseGrainCollection& nc, double tableIndex, float gain)
{
	if(hopSize != 0.0)
	{
		auto thisHopIndex = (int)(uptime / hopSize);

		if(thisHopIndex != lastHopIndex)
		{
			auto numGrains = (int)nc.size();

			auto grainIndex = jlimit(0, numGrains-1, roundToInt(tableIndex * (double)(numGrains-1)));

			if(grainIndex == lastStartedIndex)
			{
				grainIndex = jlimit(0, numGrains-1, grainIndex + Random::getSystemRandom().nextInt({-5, 5}));
			}

			for(auto& g: grains)
			{
				if(!g.isActive())
				{
					g.start(nc[grainIndex], noiseAmp);
					break;
				}
			}

			lastHopIndex = thisHopIndex;
			lastStartedIndex = grainIndex;
		}

		for(auto& g: grains)
			g.apply(b, startSample, numSamples, uptimeDelta, gain);

		uptime += (double)(numSamples) * uptimeDelta;
	}
	
}

void NoiseGrainPlayer::recalculate()
{
	if(dirty)
		return;

	dirty = true;

	auto f = [](Processor* p)
	{
		auto typed = static_cast<NoiseGrainPlayer*>(p);

		SiTraNoConverter::ConfigData cd;
		cd.fastFFTOrder = 7 + typed->grainSize;
		SiTraNoConverter converter(typed->getBuffer().sampleRate, cd);

		converter.process(typed->getAudioSampleBuffer());

		auto newGrains = converter.getNoiseGrains();

		SimpleReadWriteLock::ScopedWriteLock sl(typed->grainLock);
		std::swap(typed->noiseGrains, newGrains);
		typed->hopSize = typed->noiseGrains.empty() ? 0 : typed->noiseGrains.front().getNumSamples() / 4;
		typed->recalculateUptimeDelta();
		typed->dirty = false;

		return SafeFunctionCall::OK;
	};

	getMainController()->getKillStateHandler().killVoicesAndCall(this, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
}
} // namespace hise
