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

#pragma once;

namespace scriptnode {
using namespace juce;
using namespace hise;

struct DynamicHelpers
{
	static Identifier getId(chunkware_simple::SimpleGate* ptr)
	{
		RETURN_STATIC_IDENTIFIER("gate");
	}

	static Identifier getId(chunkware_simple::SimpleComp* ptr)
	{
		RETURN_STATIC_IDENTIFIER("comp");
	}

	static Identifier getId(chunkware_simple::SimpleCompRms* ptr)
	{
		RETURN_STATIC_IDENTIFIER("comp_rms");
	}

	static Identifier getId(chunkware_simple::SimpleLimit* ptr)
	{
		RETURN_STATIC_IDENTIFIER("limiter");
	}
};

template <class DynamicProcessorType> class DynamicsNodeBase : public HiseDspBase
{
public:

	static Identifier getStaticId() 
	{
		DynamicProcessorType* t = nullptr;
		return DynamicHelpers::getId(t);
	}

	SET_HISE_NODE_EXTRA_HEIGHT(30);
	GET_SELF_AS_OBJECT(DynamicsNodeBase<DynamicProcessorType>);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	DynamicsNodeBase()
	{
	}

	void createParameters(Array<ParameterData>& data)
	{
		{
			ParameterData p("Threshhold");
			p.range = { -100.0, 0.0, 0.1 };
			p.range.setSkewForCentre(-12.0);
			p.defaultValue = 0.0;

			p.db = std::bind(&DynamicProcessorType::setThresh, &obj, std::placeholders::_1);

			data.add(std::move(p));
		}

		{
			ParameterData p("Attack");
			p.range = { 0.0, 250.0, 0.1 };
			p.range.setSkewForCentre(50.0);
			p.defaultValue = 50.0;

			p.db = std::bind(&DynamicProcessorType::setAttack, &obj, std::placeholders::_1);

			data.add(std::move(p));
		}

		{
			ParameterData p("Release");
			p.range = { 0.0, 250.0, 0.1 };
			p.range.setSkewForCentre(50.0);
			p.defaultValue = 50.0;

			p.db = std::bind(&DynamicProcessorType::setRelease, &obj, std::placeholders::_1);

			data.add(std::move(p));
		}

		{
			ParameterData p("Ratio");
			p.range = { 1.0, 32.0, 0.1 };
			p.range.setSkewForCentre(4.0);
			p.defaultValue = 1.0;

			p.db = [this](double newValue)
			{
				auto ratio = (newValue != 0.0) ? 1.0 / newValue : 1.0;
				obj.setRatio(ratio);
			};

			data.add(std::move(p));
		}
	}

#if 0
	struct GainReductionMeter : public ExtraComponent<DynamicsNodeBase>
	{
		GainReductionMeter(DynamicsNodeBase* n, PooledUIUpdater* updater) :
			ExtraComponent(n, updater)
		{
			addAndMakeVisible(gainReductionMeter);
			
			gainReductionMeter.setType(VuMeter::Type::MonoHorizontal);
			gainReductionMeter.setColour(VuMeter::backgroundColour, Colour(0xFF333333));
			gainReductionMeter.setColour(VuMeter::ledColour, Colours::lightgrey);
			gainReductionMeter.setColour(VuMeter::outlineColour, Colour(0x45FFFFFF));
			gainReductionMeter.setInvertMode(true);
			setSize(0, 30);
		};

		void paint(Graphics& g) override
		{

		}

		void resized() override
		{
			gainReductionMeter.setBounds(getLocalBounds().removeFromTop(15));
			
		}

		void timerCallback() override
		{
			auto reduction = 1.0f - getObject()->obj.getGainReduction();

			gainReductionMeter.setPeak(reduction, reduction);
			repaint();
		}

		VuMeter gainReductionMeter;
	};
#endif

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return new ModulationSourcePlotter(updater);
	}

	bool handleModulation(double& max) noexcept 
	{
		max = jlimit(0.0, 1.0, 1.0 - obj.getGainReduction());
		return true;
	};

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		obj.setSampleRate(sampleRate);
	}

	void reset() noexcept
	{
		obj.initRuntime();
	}

	void process(ProcessData& d)
	{
		if(d.numChannels >= 2)
		{
			for (int i = 0; i < d.size; i++)
			{
				double v[2] = { d.data[0][i], d.data[1][i] };
				obj.process(v[0], v[1]);
				d.data[0][i] = v[0];
				d.data[1][i] = v[1];
			}
		}
		else if (d.numChannels == 1)
		{
			for (int i = 0; i < d.size; i++)
			{
				double v[2] = { d.data[0][i], d.data[0][i] };
				obj.process(v[0], v[1]);
				d.data[0][i] = v[0];
				
			}
		}
	}

	void processSingle(float* data, int numChannels)
	{
		if (numChannels == 2)
		{
			double values[2] = { data[0], data[1] };
			obj.process(values[0], values[1]);
			data[0] = (float)values[0];
			data[1] = (float)values[1];
		}
		else if (numChannels == 1)
		{
			double values[2] = { data[0], data[0] };
			obj.process(values[0], values[1]);
			data[0] = (float)values[0];
		}
	}


	DynamicProcessorType obj;
};

class EnvelopeFollowerNode : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("envelope_follower");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	GET_SELF_AS_OBJECT(EnvelopeFollowerNode);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	EnvelopeFollowerNode():
		envelope(20.0, 50.0)
	{}

	bool handleModulation(double&) noexcept { return false; };

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		envelope.setSampleRate(sampleRate);
	}

	forcedinline void reset() noexcept
	{
		envelope.reset();
	}

	void process(ProcessData& d)
	{
		for (int i = 0; i < d.size; i++)
		{
			float input = 0.0;

			for (int c = 0; c < d.numChannels; c++)
				input = jmax(std::abs(d.data[c][i]), input);

			input = envelope.calculateValue(input);

			for (int c = 0; c < d.numChannels; c++)
				d.data[c][i] = input;
		}
	}

	void processSingle(float* data, int numChannels)
	{
		float input = 0.0;
		
		for (int i = 0; i < numChannels; i++)
			input = jmax(std::abs(data[i]), input);
		
		input = envelope.calculateValue(input);

		FloatVectorOperations::fill(data, input, numChannels);
	}

	void createParameters(Array<ParameterData>& data)
	{
		{
			ParameterData p("Attack");
			p.range = { 0.0, 1000.0, 0.1 };
			p.range.setSkewForCentre(50.0);
			p.defaultValue = 20.0;

			p.db = std::bind(&EnvelopeFollower::AttackRelease::setAttackDouble, &envelope, std::placeholders::_1);

			data.add(std::move(p));
		}

		{
			ParameterData p("Release");
			p.range = { 0.0, 1000.0, 0.1 };
			p.range.setSkewForCentre(50.0);
			p.defaultValue = 50.0;

			p.db = std::bind(&EnvelopeFollower::AttackRelease::setReleaseDouble, &envelope, std::placeholders::_1);

			data.add(std::move(p));
		}
	}

	EnvelopeFollower::AttackRelease envelope;
};

namespace dynamics
{
using gate = DynamicsNodeBase<chunkware_simple::SimpleGate>;
using comp = DynamicsNodeBase<chunkware_simple::SimpleComp>;
using limiter = DynamicsNodeBase<chunkware_simple::SimpleLimit>;
using envelope_follower = EnvelopeFollowerNode;

struct Factory : public NodeFactory
{
	Factory(DspNetwork* network) :
		NodeFactory(network)
	{
		registerNode<HiseDspNodeBase<gate>>();
		registerNode<HiseDspNodeBase<comp>>();
		registerNode<HiseDspNodeBase<limiter>>();
		registerNode<HiseDspNodeBase<envelope_follower>>();
	};

	Identifier getId() const override { return "dynamics"; }

};
}



}