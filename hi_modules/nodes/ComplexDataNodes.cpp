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

namespace scriptnode
{
using namespace juce;
using namespace hise;


struct SequencerInterface : public ScriptnodeExtraComponent<SliderPackData>
{
	SequencerInterface(PooledUIUpdater* updater, SliderPackData* s) :
		ScriptnodeExtraComponent(s, updater),
		pack(s),
		dragger(updater)
	{
		addAndMakeVisible(pack);
		addAndMakeVisible(dragger);
		setSize(512, 100);
		stop(); // hammertime
	}

	void timerCallback() override {};

	void resized() override
	{
		auto b = getLocalBounds();
		pack.setBounds(b.removeFromTop(80));
		dragger.setBounds(b);
	}

	ModulationSourceBaseComponent dragger;
	hise::SliderPack pack;
};

template <int NV>
void scriptnode::seq_impl<NV>::setSliderPack(double indexAsDouble)
{
	jassert(sp != nullptr);
	auto index = (int)indexAsDouble;
	packData = sp.get()->getSliderPackData(index);
}

template <int NV>
void scriptnode::seq_impl<NV>::prepare(PrepareSpecs ps)
{
	lastIndex.prepare(ps);
	modValue.prepare(ps);
}



template <int NV>
void scriptnode::seq_impl<NV>::reset()
{
	lastIndex.setAll(-1);

	for (auto& mv : modValue)
		mv.setModValue(0.0);
}

template <int NV>
bool scriptnode::seq_impl<NV>::handleModulation(double& value)
{
	return modValue.get().getChangedValue(value);
}

template <int NV>
void scriptnode::seq_impl<NV>::initialise(NodeBase* n)
{
	sp = dynamic_cast<SliderPackProcessor*>(n->getScriptProcessor());
}

template <int NV>
void scriptnode::seq_impl<NV>::createParameters(ParameterDataList& data)
{
	jassert(sp != nullptr);

	{
		DEFINE_PARAMETERDATA(seq_impl, SliderPack);
		double maxPacks = (double)sp.get()->getNumSliderPacks();
		p.setRange({ 0.0, jmax(1.0, maxPacks), 1.0 });
		data.add(std::move(p));
	}
}

DEFINE_EXTERN_NODE_TEMPIMPL(seq_impl);


core::file_player::file_player()
{

}

void core::file_player::prepare(PrepareSpecs specs)
{
	uptimeDelta = audioFile->getSampleRate() / specs.sampleRate;
}

void core::file_player::reset()
{
	uptime = 0.0;
}

bool core::file_player::handleModulation(double& )
{
	return false;
}

void core::file_player::updatePosition()
{
	auto newPos = fmod(uptime / (double)currentBuffer->sampleRange.getLength(), 1.0);
	audioFile->setPosition(newPos);
}

float core::file_player::getSample(double pos, int channelIndex)
{
	auto prev = (int)pos;
	auto next = prev + 1;

	auto delta = (float)fmod(pos, 1.0);

	int numSamples = currentBuffer->sampleRange.getLength();
	int numChannels = currentBuffer->all.getNumChannels();

	next %= numSamples;
	prev %= numSamples;
	channelIndex %= numChannels;

	return Interpolator::interpolateLinear(currentBuffer->range.getSample(channelIndex, prev),
		currentBuffer->range.getSample(channelIndex, next),
		delta);
}

}

