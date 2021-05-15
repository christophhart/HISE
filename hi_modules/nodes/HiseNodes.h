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

}
#endif

namespace core
{




struct OscDisplay : public ScriptnodeExtraComponent<OscillatorDisplayProvider>
{
	OscDisplay(OscillatorDisplayProvider* n, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent(n, updater)
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

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new OscDisplay(reinterpret_cast<ObjectType*>(obj), updater);
	}

	void timerCallback() override
	{
		if (getObject() == nullptr)
			return;

		auto thisMode = (int)getObject()->currentMode;

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





class hise_mod : public HiseDspBase
{
public:

	enum class Parameters
	{
		Index, 
		numParameters
	};

	enum Index
	{
		Pitch,
		Extra1,
		Extra2,
		numIndexes
	};

	SET_HISE_NODE_ID("hise_mod");
	SN_GET_SELF_AS_OBJECT(hise_mod);

	hise_mod();

	constexpr bool isPolyphonic() const { return true; }

	static constexpr bool isNormalisedModulation() { return true; }

	void initialise(NodeBase* b);
	void prepare(PrepareSpecs ps);
	
	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		if (parentProcessor != nullptr)
		{
			auto numToDo = (double)d.getNumSamples();
			auto& u = uptime.get();

			modValues.get().setModValueIfChanged(parentProcessor->getModValueForNode(modIndex, roundToInt(u)));
			u = fmod(u + numToDo * uptimeDelta, synthBlockSize);
		}
	}

	bool handleModulation(double& v);;

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto numToDo = 1.0;
		auto& u = uptime.get();

		modValues.get().setModValueIfChanged(parentProcessor->getModValueForNode(modIndex, roundToInt(u)));
		u = Math.fmod(u + 1.0 * uptimeDelta, synthBlockSize);
	}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Index, hise_mod);
	}

	void handleHiseEvent(HiseEvent& e);
	
	void createParameters(ParameterDataList& data) override;

	void setIndex(double index);

	void reset();

private:

	WeakReference<ModulationSourceNode> parentNode;
	int modIndex = -1;

	PolyData<ModValue, NUM_POLYPHONIC_VOICES> modValues;
	PolyData<double, NUM_POLYPHONIC_VOICES> uptime;
	
	double uptimeDelta = 0.0;
	double synthBlockSize = 0.0;

	hmath Math;

	WeakReference<JavascriptSynthesiser> parentProcessor;
};



} // namespace core

}
