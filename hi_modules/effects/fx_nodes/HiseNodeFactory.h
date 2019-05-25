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



namespace core
{



struct frame2_block2 : public HiseDspBase, public HardcodedNode
{
	// Node Definitions ============================================================

	// Member Nodes ================================================================

	struct dry_wet_
	{
		// Node Definitions ============================================================
		SET_HISE_NODE_ID("dry_wet");
		SET_HISE_NODE_IS_MODULATION_SOURCE(false);

		// Member Nodes ================================================================

		struct comp_analyser_
		{
			// Node Definitions ============================================================
			SET_HISE_NODE_ID("comp_analyser");
			SET_HISE_NODE_IS_MODULATION_SOURCE(false);

			// Member Nodes ================================================================
			dynamics::comp compressor;
			math::clear clearer;

			// Interface methods === ========================================================
			void initialise(ProcessorWithScriptingContent* sp)
			{
				compressor.initialise(sp);
				clearer.initialise(sp);
			}

			void prepare(int numChannels, double sampleRate, int blockSize)
			{
				compressor.prepare(numChannels, sampleRate, blockSize);
				clearer.prepare(numChannels, sampleRate, blockSize);
			}

			void process(ProcessData& data)
			{
				compressor.process(data);
				clearer.process(data);
			}

			void processSingle(float* frameData, int numChannels)
			{
				compressor.processSingle(frameData, numChannels);
				clearer.processSingle(frameData, numChannels);
			}

			bool handleModulation(ProcessData& data, double& value)
			{
			}

			void createParameters(Array<ParameterData>& data)
			{
				Array<ParameterData> ip;
				ip.addArray(createParametersT(&compressor, "compressor"));
				ip.addArray(createParametersT(&clearer, "clearer"));

				// Parameter Initalisation =================================================
				Array<ParameterInitValue> iv;
				iv.add({ "compressor.Threshhold", -90.0 });
				iv.add({ "compressor.Attack", 0.7 });
				iv.add({ "compressor.Release", 6.6 });
				iv.add({ "compressor.Ratio", 1.1 });
				iv.add({ "clearer.Value", 0.0 });
				initParameterData(ip, iv);

				data.addArray(ip);
			}

			// Private Members =============================================================
		} comp_analyser;

		filters::svf filter;

		// Interface methods ===========================================================
		void initialise(ProcessorWithScriptingContent* sp)
		{
			comp_analyser.initialise(sp);
			filter.initialise(sp);
		}

		void prepare(int numChannels, double sampleRate, int blockSize)
		{
			comp_analyser.prepare(numChannels, sampleRate, blockSize);
			filter.prepare(numChannels, sampleRate, blockSize);

			DspHelpers::increaseBuffer(splitBuffer, numChannels * 2, blockSize);
		}

		void process(ProcessData& data)
		{
			auto original = data.copyTo(splitBuffer, 0);

			{
				comp_analyser.process(data);
			}
			{
				auto wd = original.copyTo(splitBuffer, 1);
				filter.process(wd);
				data += wd;
			}
		}

		void processSingle(float* frameData, int numChannels)
		{
		}

		bool handleModulation(ProcessData& data, double& value)
		{
		}

		void createParameters(Array<ParameterData>& data)
		{
			Array<ParameterData> ip;
			ip.addArray(createParametersT(&comp_analyser, ""));
			ip.addArray(createParametersT(&filter, "filter"));

			// Parameter Initalisation =================================================
			Array<ParameterInitValue> iv;
			iv.add({ "filter.Frequency", 464.0 });
			iv.add({ "filter.Q", 6.0 });
			iv.add({ "filter.Gain", 14.7 });
			iv.add({ "filter.Smoothing", 0.0 });
			iv.add({ "filter.Mode", 0.0 });
			initParameterData(ip, iv);

			data.addArray(ip);
		}

		// Private Members =============================================================
		AudioSampleBuffer splitBuffer;
	} dry_wet;


	// Interface methods ===========================================================
	void initialise(ProcessorWithScriptingContent* sp) override
	{
		dry_wet.initialise(sp);
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		blockSize = 1;
		dry_wet.prepare(numChannels, sampleRate, blockSize);
	}

	void process(ProcessData& data)
	{
		static constexpr int NumChannels = 2;

		int numToDo = data.size;
		float frame[NumChannels];

		while (--numToDo >= 0)
		{
			data.copyToFrame<NumChannels>(frame);
			dry_wet.processSingle(frame, NumChannels);
			data.copyFromFrameAndAdvance<NumChannels>(frame);
		}
	}

	void processSingle(float* frameData, int numChannels)
	{
		dry_wet.processSingle(frameData, numChannels);
	}

	bool handleModulation(ProcessData& data, double& value)
	{
	}

	void createParameters(Array<ParameterData>& data)
	{
		Array<ParameterData> ip;
		ip.addArray(createParametersT(&dry_wet, ""));

		// Parameter Initalisation =================================================
		Array<ParameterInitValue> iv;
		initParameterData(ip, iv);

		// Parameter Callbacks =====================================================
		{
			ParameterData p("Amount");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto comp2_Threshhold = getParameter(ip, "comp2.Threshhold");

			p.db = [comp2_Threshhold, rangeCopy](float newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				comp2_Threshhold.db(comp2_Threshhold.range.convertFrom0to1(normalised));
			};

			data.add(std::move(p));
		}
	}

	// Private Members =============================================================
};




struct sin_lfo : public HiseDspBase, public HardcodedNode
{
	// Node Definitions ============================================================
	SET_HISE_NODE_ID("sin_lfo");
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_EXTRA_COMPONENT(60, ModulationSourcePlotter);

	// Member Nodes ================================================================
	core::oscillator oscillator1;
	math::sig2mod sig2mod1;
	core::peak peak1;

	// Interface methods ===========================================================
	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		sampleRate /= (double)HISE_EVENT_RASTER;
		blockSize /= HISE_EVENT_RASTER;
		numChannels = 1;

		oscillator1.prepare(numChannels, sampleRate, blockSize);
		sig2mod1.prepare(numChannels, sampleRate, blockSize);
		peak1.prepare(numChannels, sampleRate, blockSize);
	}

	void process(ProcessData& data)
	{
		int numToProcess = data.size / HISE_EVENT_RASTER;

		auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
		CLEAR_FLOAT_ARRAY(d, numToProcess);
		ProcessData modData(&d, 1, numToProcess);

		oscillator1.process(modData);
		sig2mod1.process(modData);
		peak1.process(modData);

		modValue = DspHelpers::findPeak(modData);
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (--singleCounter > 0) return;

		singleCounter = HISE_EVENT_RASTER;
		float value = 0.0f;

		oscillator1.processSingle(&value, 1);
		sig2mod1.processSingle(&value, 1);
		peak1.processSingle(&value, 1);
	}

	bool handleModulation(ProcessData& data, double& value)
	{
		value = modValue;
		return true;
	}

	void createParameters(Array<ParameterData>& data)
	{
		Array<ParameterData> ip;
		ip.addArray(createParametersT(&oscillator1, "oscillator1"));
		ip.addArray(createParametersT(&sig2mod1, "sig2mod1"));
		ip.addArray(createParametersT(&peak1, "peak1"));

		// Parameter Initalisation =================================================
		Array<ParameterInitValue> iv;
		// Parameter Callbacks =====================================================
		iv.add({ "oscillator1.Mode", 0.0 });
		iv.add({ "oscillator1.Frequency", 2.07406 });
		iv.add({ "sig2mod1.Value", 0.0 });
		initParameterData(ip, iv);

		{
			ParameterData p("Frequency");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto oscillator1_Frequency = getParameter(ip, "oscillator1.Frequency");

			p.db = [oscillator1_Frequency, rangeCopy](float newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				oscillator1_Frequency.db(oscillator1_Frequency.range.convertFrom0to1(normalised));
			};

			data.add(std::move(p));
		}
	}

	// Private Members =============================================================
	int singleCounter = 0;
	double modValue = 0.0;
};



struct mod1 : public HiseDspBase, public HardcodedNode
{
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_ID("mod1");
	core::oscillator oscillator1;
	math::sig2mod sig2mod1;
	core::peak peak2;

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		sampleRate /= (double)HISE_EVENT_RASTER;
		blockSize /= HISE_EVENT_RASTER;
		numChannels = 1;

		oscillator1.prepare(numChannels, sampleRate, blockSize);
		sig2mod1.prepare(numChannels, sampleRate, blockSize);
		peak2.prepare(numChannels, sampleRate, blockSize);
	}

	void process(ProcessData& data)
	{
		int numToProcess = data.size / HISE_EVENT_RASTER;

		auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
		CLEAR_FLOAT_ARRAY(d, numToProcess);
		ProcessData modData(&d, 1, numToProcess);

		oscillator1.process(modData);
		sig2mod1.process(modData);
		peak2.process(modData);

		modValue = DspHelpers::findPeak(modData);
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (--singleCounter > 0) return;

		singleCounter = HISE_EVENT_RASTER;
		float value = 0.0f;

		oscillator1.processSingle(&value, 1);
		sig2mod1.processSingle(&value, 1);
		peak2.processSingle(&value, 1);
	}

	bool handleModulation(ProcessData& data, double& value)
	{
		value = modValue;
		return true;
	}

	void createParameters(Array<ParameterData>& data)
	{
		Array<ParameterData> ip;
		ip.addArray(createParametersT(&oscillator1, "oscillator1"));
		ip.addArray(createParametersT(&sig2mod1, "sig2mod1"));
		ip.addArray(createParametersT(&peak2, "peak2"));

	}

	int singleCounter = 0;
	double modValue = 0.0;
};




struct seq_lfo : public HiseDspBase, public HardcodedNode
{
	// Node Definitions ============================================================
	SET_HISE_NODE_ID("seq_lfo");
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_EXTRA_COMPONENT(60, ModulationSourcePlotter);

	// Member Nodes ================================================================
	core::simple_saw simple_saw2;
	core::seq seq2;
	core::peak peak1;

	// Interface methods ===========================================================
	void initialise(ProcessorWithScriptingContent* sp)override
	{
		simple_saw2.initialise(sp);
		seq2.initialise(sp);
		peak1.initialise(sp);
	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		sampleRate /= (double)HISE_EVENT_RASTER;
		blockSize /= HISE_EVENT_RASTER;
		numChannels = 1;

		simple_saw2.prepare(numChannels, sampleRate, blockSize);
		seq2.prepare(numChannels, sampleRate, blockSize);
		peak1.prepare(numChannels, sampleRate, blockSize);
	}

	void process(ProcessData& data)
	{
		int numToProcess = data.size / HISE_EVENT_RASTER;

		auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
		CLEAR_FLOAT_ARRAY(d, numToProcess);
		ProcessData modData(&d, 1, numToProcess);

		simple_saw2.process(modData);
		seq2.process(modData);
		peak1.process(modData);

		modValue = DspHelpers::findPeak(modData);
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (--singleCounter > 0) return;

		singleCounter = HISE_EVENT_RASTER;
		float value = 0.0f;

		simple_saw2.processSingle(&value, 1);
		seq2.processSingle(&value, 1);
		peak1.processSingle(&value, 1);
	}

	bool handleModulation(ProcessData& data, double& value)
	{
		value = modValue;
		return true;
	}

	void createParameters(Array<ParameterData>& data)
	{
		Array<ParameterData> ip;
		ip.addArray(createParametersT(&simple_saw2, "simple_saw2"));
		ip.addArray(createParametersT(&seq2, "seq2"));
		ip.addArray(createParametersT(&peak1, "peak1"));

		// Parameter Initalisation =================================================
		Array<ParameterInitValue> iv;
		// Parameter Callbacks =====================================================
		iv.add({ "simple_saw2.PeriodTime", 1000.0 });
		iv.add({ "seq2.SliderPack", 0.0 });
		initParameterData(ip, iv);

		{
			ParameterData p("Pack");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto seq2_SliderPack = getParameter(ip, "seq2.SliderPack");

			p.db = [seq2_SliderPack, rangeCopy](float newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				seq2_SliderPack.db(seq2_SliderPack.range.convertFrom0to1(normalised));
			};

			data.add(std::move(p));
		}
		{
			ParameterData p("Frequency");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto simple_saw2_PeriodTime = getParameter(ip, "simple_saw2.PeriodTime");

			p.db = [simple_saw2_PeriodTime, rangeCopy](float newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				simple_saw2_PeriodTime.db(simple_saw2_PeriodTime.range.convertFrom0to1(normalised));
			};

			data.add(std::move(p));
		}
	}

	// Private Members =============================================================
	int singleCounter = 0;
	double modValue = 0.0;
};



struct mod2 : public HiseDspBase, public HardcodedNode
{
	// Node Definitions ============================================================
	SET_HISE_NODE_ID("mod2");
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_EXTRA_COMPONENT(60, ModulationSourcePlotter);

	// Member Nodes ================================================================
	math::add add2;
	dynamics::envelope_follower envelope_follower1;
	core::peak peak1;

	// Interface methods ===========================================================
	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		sampleRate /= (double)HISE_EVENT_RASTER;
		blockSize /= HISE_EVENT_RASTER;
		numChannels = 1;

		add2.prepare(numChannels, sampleRate, blockSize);
		envelope_follower1.prepare(numChannels, sampleRate, blockSize);
		peak1.prepare(numChannels, sampleRate, blockSize);
	}

	void process(ProcessData& data)
	{
		int numToProcess = data.size / HISE_EVENT_RASTER;

		auto d = ALLOCA_FLOAT_ARRAY(numToProcess);
		CLEAR_FLOAT_ARRAY(d, numToProcess);
		ProcessData modData(&d, 1, numToProcess);

		add2.process(modData);
		envelope_follower1.process(modData);
		peak1.process(modData);

		modValue = DspHelpers::findPeak(modData);
	}

	void processSingle(float* frameData, int numChannels)
	{
		if (--singleCounter > 0) return;

		singleCounter = HISE_EVENT_RASTER;
		float value = 0.0f;

		add2.processSingle(&value, 1);
		envelope_follower1.processSingle(&value, 1);
		peak1.processSingle(&value, 1);
	}

	bool handleModulation(ProcessData& data, double& value)
	{
		value = modValue;
		return true;
	}

	// Parameter Initalisation =================================================
	// Parameter Callbacks =====================================================
	void createParameters(Array<ParameterData>& data)
	{
		Array<ParameterData> ip;
		ip.addArray(createParametersT(&add2, "add2"));
		ip.addArray(createParametersT(&envelope_follower1, "envelope_follower1"));
		ip.addArray(createParametersT(&peak1, "peak1"));

		Array<ParameterInitValue> iv;
		iv.add({ "add2.Value", 0.73 });
		iv.add({ "envelope_follower1.Attack", 0.9 });
		iv.add({ "envelope_follower1.Release", 398.0 });
		initParameterData(ip, iv);

		{
			ParameterData p("Value");
			p.range = { 0.0, 1.0, 0.0, 1.0 };
			auto rangeCopy = p.range;

			auto add2_Value = getParameter(ip, "add2.Value");

			p.db = [add2_Value, rangeCopy](float newValue)
			{
				auto normalised = rangeCopy.convertTo0to1(newValue);
				add2_Value.db(add2_Value.range.convertFrom0to1(normalised));
			};

			data.add(std::move(p));
		}
	}

	// Private Members =============================================================
	int singleCounter = 0;
	double modValue = 0.0;
};






}

class HiseFxNodeFactory : public NodeFactory
{
public:

	HiseFxNodeFactory(DspNetwork* network);;
	Identifier getId() const override { return "core"; }
};

}
