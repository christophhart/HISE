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

template <int NV> class seq_impl : public HiseDspBase
{
public:

	static constexpr int NumVoices = NV;

	SET_HISE_POLY_NODE_ID("seq");
	GET_SELF_AS_OBJECT(seq_impl);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_EXTRA_WIDTH(512);
	SET_HISE_NODE_EXTRA_HEIGHT(100);

	Component* createExtraComponent(PooledUIUpdater* updater) override;
	void createParameters(Array<ParameterData>& data) override;
	void initialise(NodeBase* n) override;
	bool handleModulation(double& value);
	void reset();
	void process(ProcessData& data);
	void processSingle(float* frameData, int numChannels);;
	void prepare(PrepareSpecs ps);

	void setSliderPack(double indexAsDouble);

	WeakReference<SliderPackProcessor> sp;
	WeakReference<SliderPackData> packData;

	PolyData<int, NumVoices> lastIndex = -1;
	PolyData<ModValue, NumVoices> modValue;
};


DEFINE_EXTERN_NODE_TEMPLATE(seq, seq_poly, seq_impl);


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
	bool handleModulation(double& value);
	void prepare(PrepareSpecs);
	void reset() noexcept;
	void process(ProcessData& data);
	void processSingle(float* frameData, int numChannels);;
	void setTable(double indexAsDouble);

	WeakReference<LookupTableProcessor> tp;
	WeakReference<SampleLookupTable> tableData;

	double currentValue = 0;
	bool changed = true;
};

namespace core
{

struct file_player : public AudioFileNodeBase
{
	SET_HISE_NODE_ID("file_player");
	GET_SELF_AS_OBJECT(file_player);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);
	SET_HISE_NODE_EXTRA_WIDTH(256);
	SET_HISE_NODE_EXTRA_HEIGHT(100);

	file_player();;

	void prepare(PrepareSpecs specs) override;
	void reset();
	bool handleModulation(double& modValue);
	void process(ProcessData& d);
	void processSingle(float* frameData, int numChannels);
	void updatePosition();

private:

	float getSample(double pos, int channelIndex);

	double uptime = 0.0;
	double uptimeDelta = 1.0;
};

}

namespace core
{

using table = TableNode;

}




}
