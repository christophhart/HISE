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

template <int NV> struct seq_impl : public HiseDspBase
{
	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("seq");
	GET_SELF_AS_OBJECT(seq_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_EXTRA_WIDTH(512);
	SET_HISE_NODE_EXTRA_HEIGHT(100);

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return new SequencerInterface(updater, packData.get());
	}

	void createParameters(Array<ParameterData>& data) override
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

	void initialise(NodeBase* n) override
	{
		sp = dynamic_cast<SliderPackProcessor*>(n->getScriptProcessor());
	}

	bool handleModulation(double& value)
	{
		return modValue.get().getChangedValue(value);
	}

	void reset()
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

	void process(ProcessData& data)
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

	void processSingle(float* frameData, int numChannels)
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
	};

	void prepare(PrepareSpecs ps)
	{
		lastIndex.prepare(ps);
		modValue.prepare(ps);
	}

	void setSliderPack(double indexAsDouble)
	{
		jassert(sp != nullptr);
		auto index = (int)indexAsDouble;
		packData = sp.get()->getSliderPackData(index);
	}

	WeakReference<SliderPackProcessor> sp;
	WeakReference<SliderPackData> packData;

	PolyData<int, NumVoices> lastIndex = -1;
	PolyData<ModValue, NumVoices> modValue;
};

DEFINE_EXTERN_NODE_TEMPLATE(seq, seq_poly, seq_impl);
DEFINE_EXTERN_NODE_TEMPIMPL(seq_impl);

struct TableNode : public HiseDspBase
{
	SET_HISE_NODE_ID("table");
	GET_SELF_AS_OBJECT(TableNode);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_EXTRA_WIDTH(512);
	SET_HISE_NODE_EXTRA_HEIGHT(100);

	struct TableInterface;

	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void createParameters(Array<ParameterData>& data) override;

	void initialise(NodeBase* n) override;

	bool handleModulation(double& value)
	{
		if (changed)
		{
			value = currentValue;
			changed = false;
			return true;
		}

		return false;
	}

	void prepare(PrepareSpecs)
	{

	}

	forcedinline void reset() noexcept
	{
		currentValue = 0.0;
		changed = true;
	}

	void process(ProcessData& data)
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

	void processSingle(float* frameData, int numChannels)
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
	};

	void setTable(double indexAsDouble);

	WeakReference<LookupTableProcessor> tp;
	WeakReference<SampleLookupTable> tableData;

	double currentValue = 0;
	bool changed = true;
};

namespace core
{

using table = TableNode;

}




}
