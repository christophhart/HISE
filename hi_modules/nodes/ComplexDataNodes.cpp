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


struct SequencerInterface : public HiseDspBase::ExtraComponent<SliderPackData>
{
	SequencerInterface(PooledUIUpdater* updater, SliderPackData* s) :
		ExtraComponent(s, updater),
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
void scriptnode::seq_impl<NV>::processSingle(float* frameData, int numChannels)
{
	if (packData != nullptr)
	{
		auto peakValue = jlimit(0.0, 0.999999, DspHelpers::findPeak(ProcessData(&frameData, 1, numChannels)));
		auto index = int(peakValue * (double)packData->getNumSliders());

		if (lastIndex.get() != index)
		{
			lastIndex.get() = index;
			modValue.get().setModValue(packData->getValue(index));
		}

		FloatVectorOperations::fill(frameData, (float)modValue.get().getModValue(), numChannels);
	}
}

template <int NV>
void scriptnode::seq_impl<NV>::process(ProcessData& data)
{
	if (packData != nullptr)
	{
		auto peakValue = jlimit(0.0, 0.999999, DspHelpers::findPeak(data));
		auto index = int(peakValue * (double)(packData->getNumSliders()));

		if (lastIndex.get() != index)
		{
			lastIndex.get() = index;


			modValue.get().setModValue((double)packData->getValue(index));



			packData->setDisplayedIndex(index);
		}

		for (auto c : data)
			FloatVectorOperations::fill(c, (float)modValue.get().getModValue(), data.size);

		String s;


	}
}

template <int NV>
void scriptnode::seq_impl<NV>::reset()
{
	if (lastIndex.isVoiceRenderingActive())
	{
		lastIndex.get() = -1;
		modValue.get().setModValue(0.0);
	}
	else
	{
		lastIndex.setAll(-1);
		modValue.forEachVoice([](ModValue& mv) { mv.setModValue(0.0); });
	}
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
void scriptnode::seq_impl<NV>::createParameters(Array<ParameterData>& data)
{
	jassert(sp != nullptr);

	{
		ParameterData p("SliderPack");

		double maxPacks = (double)sp.get()->getNumSliderPacks();

		p.range = { 0.0, jmax(1.0, maxPacks), 1.0 };
		p.db = BIND_MEMBER_FUNCTION_1(seq_impl::setSliderPack);

		data.add(std::move(p));
	}
}


template <int NV>
Component* scriptnode::seq_impl<NV>::createExtraComponent(PooledUIUpdater* updater)
{
	return new SequencerInterface(updater, packData.get());
}

DEFINE_EXTERN_NODE_TEMPIMPL(seq_impl);

struct TableNode::TableInterface : public HiseDspBase::ExtraComponent<TableNode>
{
	TableInterface(PooledUIUpdater* updater, TableNode* t) :
		ExtraComponent(t, updater),
		editor(nullptr, t->tableData),
		dragger(updater)
	{
		addAndMakeVisible(editor);
		addAndMakeVisible(dragger);
		setSize(512, 90);
	}

	void timerCallback() override
	{
		if (editor.getEditedTable() != getObject()->tableData)
		{
			editor.setEditedTable(getObject()->tableData);
		}
	};

	void resized() override
	{
		auto b = getLocalBounds();

		editor.setBounds(b.removeFromTop(80));
		dragger.setBounds(b);
	}

	ModulationSourceBaseComponent dragger;
	hise::TableEditor editor;
	ComboBox selector;
};

juce::Component* TableNode::createExtraComponent(PooledUIUpdater* updater)
{
	return new TableInterface(updater, this);
}

void TableNode::createParameters(Array<ParameterData>& data)
{
	if (tp != nullptr)
	{
		ParameterData p("TableIndex");

		double maxPacks = (double)tp.get()->getNumTables();

		p.range = { 0.0, jmax(1.0, maxPacks), 1.0 };
		p.db = BIND_MEMBER_FUNCTION_1(TableNode::setTable);

		data.add(std::move(p));
	}
}


void TableNode::initialise(NodeBase* n)
{
	tp = dynamic_cast<LookupTableProcessor*>(n->getScriptProcessor());
}

bool TableNode::handleModulation(double& value)
{
	if (changed)
	{
		value = currentValue;
		changed = false;
		return true;
	}

	return false;
}

void TableNode::prepare(PrepareSpecs)
{

}

void TableNode::reset() noexcept
{
	currentValue = 0.0;
	changed = true;
}

void TableNode::process(ProcessData& data)
{
	if (tableData != nullptr)
	{
		auto peakValue = jlimit(0.0, 1.0, DspHelpers::findPeak(data));
		auto value = tableData->getInterpolatedValue(peakValue * SAMPLE_LOOKUP_TABLE_SIZE);

		changed = currentValue != value;

		if (changed)
			currentValue = value;

		for (auto c : data)
			FloatVectorOperations::fill(c, (float)currentValue, data.size);
	}
}

void TableNode::processSingle(float* frameData, int numChannels)
{
	if (tableData != nullptr)
	{
		auto peakValue = jlimit(0.0, 1.0, DspHelpers::findPeak(ProcessData(&frameData, 1, numChannels)));
		auto value = tableData->getInterpolatedValue(peakValue * SAMPLE_LOOKUP_TABLE_SIZE);

		changed = currentValue != value;

		if (changed)
			currentValue = value;

		FloatVectorOperations::fill(frameData, (float)currentValue, numChannels);
	}
}

void TableNode::setTable(double indexAsDouble)
{
	jassert(tp != nullptr);
	auto index = (int)indexAsDouble;
	tableData = dynamic_cast<SampleLookupTable*>(tp.get()->getTable(index));
}



core::file_player::file_player()
{

}

void core::file_player::prepare(PrepareSpecs specs)
{
	uptimeDelta = specs.sampleRate / audioFile->getSampleRate();
}

void core::file_player::reset()
{
	uptime = 0.0;
}

bool core::file_player::handleModulation(double& modValue)
{
	return false;
}

void core::file_player::process(ProcessData& d)
{
	SpinLock::ScopedLockType sl(audioFile->getLock());

	if (currentBuffer->clear)
		return;

	for (int c = 0; c < d.numChannels; c++)
	{
		auto thisUptime = uptime;

		for (int i = 0; i < d.size; i++)
		{
			d.data[c][i] += getSample(thisUptime, c);
			thisUptime += uptimeDelta;
		}
	}

	uptime += (double)d.size * uptimeDelta;

	updatePosition();
}

void core::file_player::processSingle(float* frameData, int numChannels)
{
	SpinLock::ScopedLockType sl(lock);

	if (currentBuffer->clear)
		return;

	for (int i = 0; i < numChannels; i++)
		frameData[i] += getSample(uptime, i);

	uptime += uptimeDelta;

	updatePosition();
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

