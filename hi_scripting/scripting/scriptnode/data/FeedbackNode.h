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

struct SequencerNode : public HiseDspBase
{
	SET_HISE_NODE_ID("seq");
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_EXTRA_WIDTH(512);
	SET_HISE_NODE_EXTRA_HEIGHT(100);

	struct SequencerInterface;

	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void createParameters(Array<ParameterData>& data) override;

	void initialise(ProcessorWithScriptingContent* sp) override;

	bool handleModulation(ProcessData& d, double& value)
	{
		if (changed)
		{
			value = currentValue;
			changed = false;
			return true;
		}

		return false;
	}

	void process(ProcessData& data)
	{
		if (packData != nullptr)
		{
			auto peakValue = jlimit(0.0, 1.0, DspHelpers::findPeak(data));
			auto index = roundDoubleToInt(peakValue * (double)packData->getNumSliders());


			changed = lastIndex != index;

			if (changed)
			{
				lastIndex = index;
				currentValue = packData->getValue(index);
				packData->setDisplayedIndex(index);
			}

			for (auto c : data)
				FloatVectorOperations::fill(c, currentValue, data.size);
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (packData != nullptr)
		{
			auto peakValue = jlimit(0.0, 1.0, DspHelpers::findPeak(ProcessData(&frameData, 1, numChannels)));
			auto index = roundDoubleToInt(peakValue * (double)packData->getNumSliders());
			
			changed = lastIndex != index;

			if (changed)
			{
				lastIndex = index;
				currentValue = packData->getValue(index);
			}

			FloatVectorOperations::fill(frameData, currentValue, numChannels);
		}
	};

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		
	}

	void setSliderPack(double indexAsDouble);

	WeakReference<JavascriptProcessor> jp;
	WeakReference<SliderPackData> packData;

	int lastIndex = -1;
	double currentValue = 0;
	bool changed = true;
};

struct TableNode : public HiseDspBase
{
	SET_HISE_NODE_ID("table");
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_EXTRA_WIDTH(512);
	SET_HISE_NODE_EXTRA_HEIGHT(100);

	struct TableInterface;

	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void createParameters(Array<ParameterData>& data) override;

	void initialise(ProcessorWithScriptingContent* sp) override;

	bool handleModulation(ProcessData& d, double& value)
	{
		if (changed)
		{
			value = currentValue;
			changed = false;
			return true;
		}

		return false;
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{

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
				FloatVectorOperations::fill(c, currentValue, data.size);
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

			FloatVectorOperations::fill(frameData, currentValue, numChannels);
		}
	};

	void setTable(double indexAsDouble);

	WeakReference<JavascriptProcessor> jp;
	WeakReference<SampleLookupTable> tableData;

	double currentValue = 0;
	bool changed = true;
};



using seq = SequencerNode;
using table = TableNode;

}




}
