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
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise
{

namespace tests
{


using namespace juce;
using namespace scriptnode;
using namespace snex;
using namespace snex::Types;

struct node_test
{
	using ExpectedValueList = std::vector<std::tuple<int, int, float>>;

	node_test() :
		dynData(nullptr, 0, 0)
	{};

	virtual ~node_test() {};

	void setEventBufferInitialiser(const std::function<void(HiseEventBuffer&)>& f)
	{
		eventBufferInitialiser = f;
	}

	Result expectValues(const String& testName, const ExpectedValueList& l)
	{
		try
		{
			for (auto t : l)
				expect(std::get<0>(t), std::get<1>(t), std::get<2>(t));

			return Result::ok();
		}
		catch (String& e)
		{
			return Result::fail(testName + " - " + e);
		}
	}

	template <int ChannelAmount, typename T> void callObjectWithFix(T& obj, int numSamples)
	{
		setup(ChannelAmount, numSamples);

		obj.prepare(ps);
		obj.reset();

		auto& fixData = dynData.as<ProcessData<ChannelAmount>>();

		obj.process(fixData);
	}

	template <typename T> void callObjectWithDyn(T& obj, int numChannels, int numSamples)
	{
		setup(numChannels, numSamples);

		obj.prepare(ps);
		obj.reset();
		obj.process(dynData);
	}

private:

	void setup(int numChannels, int numSamples)
	{
		b.setSize(numChannels, numSamples);
		b.clear();
		ps.numChannels = numChannels;
		ps.blockSize = numSamples;
		ps.sampleRate = 44100.0;

		dynData.referTo(b.getArrayOfWritePointers(), numChannels, numSamples);

		eb.clear();

		if (eventBufferInitialiser)
			eventBufferInitialiser(eb);

		dynData.setEventBuffer(eb);
	}

	void expect(int channelIndex, int sampleIndex, float expectedValue)
	{
		auto f = b.getSample(channelIndex, sampleIndex);

		if (std::abs(f - expectedValue) > 0.0001f)
		{
			String e;

			wrongChannel.reserve(b.getNumSamples());

			for (int i = 0; i < b.getNumSamples(); i++)
				wrongChannel.push_back(b.getSample(channelIndex, i));

			e << "Expected " << String(expectedValue) << " at data[" << String(channelIndex) << "][" << String(sampleIndex) << "]";
			e << ", Actual value: " << String(f);

			jassertfalse;

			throw e;
		}
	}

	AudioSampleBuffer b;
	PrepareSpecs ps;
	HiseEventBuffer eb;
	ProcessDataDyn dynData;

	std::vector<float> wrongChannel;

	std::function<void(HiseEventBuffer&)> eventBufferInitialiser;
};

/** A bunch of helper nodes that might come in handy to verify the processing of wrapper nodes / containers. */
struct helper_nodes
{
	struct dc
	{
		DECLARE_NODE(dc);

		SN_EMPTY_PREPARE;
		SN_EMPTY_RESET;
		
		void handleHiseEvent(HiseEvent& e)
		{

		}
		
		template <int P> void setParameter(double v)
		{
			dcValue = (float)v;
		}

		template <typename ProcessDataType> void process(ProcessDataType& data)
		{
			for (auto& ch : data)
			{
				for (auto& s : data.toChannelData(ch))
					s += dcValue;
			}
		}

		template <typename FrameDataType> void processFrame(FrameDataType& d)
		{
			for (auto& s : d)
				s += dcValue;
		}

		float dcValue = 1.0f;
	};

	/** Converts the velocity of a hise event to a DC offset applied to all channels. */
	struct event2dc
	{
		DECLARE_NODE(event2dc);

		SN_EMPTY_PREPARE;
		SN_EMPTY_RESET;
		
        template <int P> void setParameter(double v)
        {
            
        };
        
		template <typename FrameDataType> void processFrame(FrameDataType& data)
		{
			for (auto& s : data)
				s = v;
		}

		template <typename ProcessDataType> void process(ProcessDataType& data)
		{
			FrameConverters::forwardToFrame16(this, data);
		}

		void handleHiseEvent(HiseEvent& e)
		{
			v += e.getFloatVelocity();
		}

		float v = 0.0f;
	};

	template <int Default7BitValue> struct event_initialiser
	{
		event_initialiser(event2dc& e)
		{
			e.v = (float)Default7BitValue / 128.0f;
		}
	};
};

struct WrapperTests : public UnitTest
{
	WrapperTests() :
		UnitTest("Testing node wrappers", "node_tests")
	{}

	void testOpaqueNodes()
	{
		beginTest("Testing opaque nodes");

		node_test n;

		testOpaqueNode<core::mono2stereo>(n, "mono2stereo");
		testOpaqueNode<core::oscillator<1>>(n, "oscillator");
	}

	template <typename T> void testOpaqueNode(node_test& t, const String& v)
	{
		OpaqueNode obj;
		obj.create<T>();

		t.callObjectWithDyn(obj, 2, 512);
	}


	template <typename T> void testEventWrapper(node_test& t, const String& v)
	{
		T obj;

		//t.callObjectWithDyn<T>(obj, 2, 128);

		t.callObjectWithFix<2>(obj, 128);

		auto r = t.expectValues(v,
			{ {0, 0, 0.0f}, {0, 63, 0.0f}, {0, 64, 1.0f}, {0, 127, 1.0f} });
		expect(r.wasOk(), r.getErrorMessage());
	}

	void testEventWrappers()
	{
		tests::node_test t;

		t.setEventBufferInitialiser([](HiseEventBuffer& b)
		{
			HiseEvent e(HiseEvent::Type::NoteOn, 64, 127, 1);
			e.setTimeStamp(64);
			b.addEvent(e);
		});

		beginTest("Testing wrap::event");

		testEventWrapper<wrap::event<wrap::frame<2, helper_nodes::event2dc>>>
			(t, "Testing wrap::event in frame<2>");

		testEventWrapper<wrap::event<helper_nodes::event2dc>>
			(t, "Testing wrap::event");

		testEventWrapper<wrap::fix_block<32, wrap::event<helper_nodes::event2dc>>>
			(t, "Testing wrap::event in fix_block<32>");

		//testEventWrapper<wrap::init<wrap::event<helper_nodes::event2dc>, helper_nodes::event_initialiser<32>>>(t, "Testing init");
	}

	

	void runTest() override
	{
		
		testOpaqueNodes();

		//testBypassWrappers();
		

		//testEventWrappers();
	}


	void testBypassWrappers()
	{
		beginTest("Testing bypass wrappers");

		node_test t;

		using BypassNodeType = bypass::smoothed<helper_nodes::dc>;
		
		BypassNodeType obj;

		parameter::bypass<BypassNodeType> p;

		p.connect<0>(obj);

		p.call(1.0);

		t.callObjectWithFix<1>(obj, 44100);

		t.expectValues("smoothed_test", { {0, 0, 1.0}, {0, 22050, 0.0f } });
	}
};

static WrapperTests wrapperTest;


}


}
