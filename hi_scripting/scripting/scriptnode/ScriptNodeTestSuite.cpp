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


#include "AppConfig.h"




#if HI_RUN_UNIT_TESTS

#include  "JuceHeader.h"

namespace scriptnode
{

using namespace hise;
using namespace juce;
using namespace snex;
using namespace snex::Types;










/** This is just a simple test node that has all functions that a node is expected to have. */
struct TestNode
{
	SNEX_NODE(TestNode);

	static const int NumChannels = 2;

	/** Returns whether the node is polyphonic. */
	bool isPolyphonic() const { return false; }

	void reset()
	{
		resetted = true;
	}

	void handleHiseEvent(HiseEvent& e) {}

	void prepare(PrepareSpecs ps)
	{
		prepared = true;
	}

	void processFrame(span<float, NumChannels>& data)
	{
		data[0] = value;
	}

	void process(ProcessData<NumChannels>& d)
	{
		auto frameData = d.toFrameData();

		while (frameData.next())
		{
			processFrame(frameData);
		}
	}

	bool handleModulation(double& v)
	{
		v = modValue;
		return true;
	}

	template <int P> void setParameter(double v) 
	{ 
		if(P == 0)
			value = v;
	}

	double propertyValue = 0.0;

	double value = 0.0;
	bool initialised = false;
	bool prepared = false;
	bool resetted = false;
	const double modValue = 0.666;
};

namespace TestOps
{
using ProcessDataType = snex::Types::ProcessData<2>;

struct Add
{
	constexpr static float getDefaultValue() { return 0.0f; }
	
	static float op(float op1, float op2)
	{
		return op1 + op2;
	}
};

struct Mul
{
	constexpr static float getDefaultValue() { return 1.0f; }

	static float op(float op1, float op2)
	{
		return op1 * op2;
	}
};

template <class OpClass, int C> struct Node
{
	SNEX_NODE(Node);

	static const int NumChannels = C;

	bool isPolyphonic() const { return false; }

	void reset() {}
	void handleHiseEvent(HiseEvent& e) {}

	void prepare(PrepareSpecs ps) {}

	void processFrame(span<float, NumChannels>& d)
	{
		for (auto& s : d)
			s = OpClass::op(s, op2);
	}

	void process(ProcessData<NumChannels>& d)
	{
		for (auto& c : d)
		{
			for (auto& s : d.toChannelData(c))
			{
				auto value = OpClass::op(s, op2);
				s = value;
			}
		}
	}

	bool handleModulation(double& v) { return false; }

	template <int P> void setParameter(double v)
	{
		op2 = (float)v;
	}

	float op2 = OpClass::getDefaultValue();
};
}


namespace matrixtypes
{
template <int C> struct identity
{
	static constexpr bool createDisplayValues() { return false; }
	void handleDisplayValues(bool, ProcessDataDyn& ) {}

	static constexpr bool isFixedChannelMatrix() { return true; }
	static constexpr int getNumChannels() { return C; }
	static constexpr int getChannel(int index) { return index; }
	static constexpr int getSendChannel(int index) { return -1; }
	static constexpr bool hasSendChannels() { return false; }
};

template <int Index, int C> struct one2all
{
	static constexpr bool createDisplayValues() { return false; }
	void handleDisplayValues(bool, ProcessDataDyn& ) {}

	static constexpr bool isFixedChannelMatrix() { return true; }
	static constexpr int getNumChannels() { return C; }
	static constexpr int getChannel(int index) { return Index; }
	static constexpr int getSendChannel(int index) { return -1; }
	static constexpr bool hasSendChannels() { return false; }
};





}

struct right_to_left : public routing::static_matrix<2, right_to_left, false>
{
	static constexpr int channels[2] =
	{
		0, 0
	};
};

struct ScriptNodeTests : public juce::UnitTest
{
	ScriptNodeTests() :
		UnitTest("Scriptnode tests")
	{};

	template <int N> void testContainerWithNumSamples()
	{
		beginTest("Testing containers with " + String(N) + " samples");

		testContainers<1, N>();
		testContainers<2, N>();
		testContainers<3, N>();
		testContainers<4, N>();
		testContainers<5, N>();
		testContainers<6, N>();
		testContainers<7, N>();
		testContainers<8, N>();
		testContainers<13, N>();
		testContainers<16, N>();
	}

	void runTest() override
	{
		testSendReceive();
		testMatrixNode();

		testContainerWithNumSamples<1>();
		testContainerWithNumSamples<2>();
		testContainerWithNumSamples<8>();
		testContainerWithNumSamples<128>();
		testContainerWithNumSamples<37>();

		testProcessData<2>(2);
		testProcessData<3>(7);
		testProcessData<1>(1);
		testProcessData<1>(2);
		testProcessData<2>(1);
		testProcessData<2>(2);
		testProcessData<3>(1);
		testProcessData<3>(2);
		testProcessData<9>(32);
		testProcessData<11>(31);
		testProcessData<6>(941);
		testProcessData<12>(204);

		testRangeTemplates();
		testParameters();
		testModWrapper();
	}

	struct Dummy
	{
		enum Parameters
		{
			First,
			Second,
			Third,
			numParameters
		};

		Dummy()
		{
			memset(value, 0, sizeof(double) * numParameters);
		}

		SN_GET_SELF_AS_OBJECT(Dummy);

		static const int NumChannels = 2;

		template <int P> void setParameter(double v)
		{
			value[P] = v;
		}

		SN_FORWARD_PARAMETER_TO_MEMBER(Dummy);

		double value[numParameters];
	};

	

	void testRangeTemplates()
	{
		beginTest("Testing range templates");

		DECLARE_PARAMETER_RANGE(testRange, 0.5, 0.7);

		expectEquals(testRange::from0To1(0.5), 0.6, "midpoint from");
		expectEquals(testRange::from0To1(0.0), 0.5, "start from");
		expectEquals(testRange::from0To1(1.0), 0.7, "endpoint from");

		expectEquals(testRange::to0To1(0.6), 0.5, "midpoint to");
		expectEquals(testRange::to0To1(0.7), 1.0, "end to");
		expectEquals(testRange::to0To1(0.5), 0.0, "start to");

		DECLARE_PARAMETER_EXPRESSION(exprRange, Math.abs(input * 2.0 - 3.0));

		expectEquals(exprRange::op(2.0), hmath::abs(2.0 * 2.0 - 3.0), "expr 1");
		expectEquals(exprRange::op(-12.0), hmath::abs(-12.0 * 2.0 - 3.0), "expr 2");
		expectEquals(exprRange::op(1000.0), hmath::abs(1000.0 * 2.0 - 3.0), "expr 3");

		DECLARE_PARAMETER_RANGE_SKEW(skewRange, 12.0, 19.0, 1.5);

		NormalisableRange<double> d(12.0, 19.0, 0.0, 1.5);

		
		expectEquals(skewRange::from0To1(1.0), d.convertFrom0to1(1.0));
		expectEquals(skewRange::to0To1(14.4), d.convertTo0to1(14.4));
	}

	void testParameters()
	{
		

		{
			beginTest("Testing macro parameters in a chain");

			// Create an alias for a list with one parameter to the second
			// slot of the Dummy node
			using parameters = parameter::list<parameter::plain<Dummy, Dummy::Third>>;

			static constexpr int FirstMacro = 0;

			// Create a chain with the previously defined parameter set
			// and one Dummy node
			container::chain<parameters, Dummy> chain;

			// Get a reference to the Dummy node in the chain
			auto& obj = chain.get<0>();

			// Connect the chain's macro parameter to the dummy node
			chain.getParameter<FirstMacro>().connect<0>(obj);

			// Call the macro parameter of the chain
			chain.setParameter<FirstMacro>(0.5);

			expectEquals(obj.value[Dummy::Third], 0.5, "chain test");
		}

		{
			beginTest("Testing nested chain parameters");

			// Create an alias for the inner chain
			using inner = container::chain<parameter::plain<Dummy, 2>, Dummy>;

			// Create an alias for the outer chain, using the inner chain
			// and a parameter that will be connected to the inner chain's parameter
			using outer = container::chain<parameter::plain<inner, 0>, inner>;

			// Create an instance of the outer chain
			outer o;

			// Get the references to the inner chain and the most inner object
			auto& i = o.get<0>();
			auto& d = i.get<0>();

			// Connect the outer chain to the inner chain's parameter
			o.getParameter<0>().connect<0>(i);

			// Connect the inner chain's parameter to the Dummy node
			i.getParameter<0>().connect<0>(d);
			
			// Set the outer parameter and it will propagate through
			// the nested parameter connections
			o.setParameter<0>(0.5);

			expectEquals(d.value[2], 0.5, "nested call");
		}

		{
			beginTest("Testing parameter chain");

			// Create three ranges that are going to be used for the parameter connections.
			DECLARE_PARAMETER_RANGE(inputRange, 0.0, 2.0);
			DECLARE_PARAMETER_RANGE(firstRange, 1.0, 3.0);
			DECLARE_PARAMETER_RANGE(secondRange, 10.0, 30.0);

			// Create an parameter chain using the input range and two connections to dummy parameters.
			using ParameterType = parameter::chain<inputRange, scriptnode::parameter::from0To1<Dummy, Dummy::First, firstRange>, 
														       parameter::from0To1<Dummy, Dummy::Second, secondRange>>;

			// Now we'll put that parameter into a list so it matches
			// the usual way how container parameters are declared
			using ParameterList = parameter::list<ParameterType>;

			// Create a chain with two Dummy nodes that are being controlled by one parameter
			container::chain<ParameterList, Dummy, Dummy> c;

			// get a reference to the parameter chain
			auto& p = c.getParameter<0>();

            ignoreUnused(p);
            
			// get the reference to the both nodes
			auto& first = c.get<0>();
			auto& second = c.get<1>();

			// connect the first connection in the chain to
			// the first node
			c.getParameter<0>().connect<0>(first);

			// connect the second one to the second node
			c.getParameter<0>().connect<1>(second);

			// now set the parameter and it will update both nodes
			c.setParameter<0>(1.5);

			auto fValue = first.value[Dummy::First];
			auto sValue = second.value[Dummy::Second];

			expectEquals<double>(fValue, 2.5, "first parameter");
			expectEquals<double>(sValue, 25.0, "second parameter");

		}

		
		
	}

	void testModWrapper()
	{
		beginTest("Testing mod wrapper");

		// Create a wrapped test node with a parameter to the second parameter
		// of a Dummy node
		wrap::mod<parameter::plain<Dummy, Dummy::Second>, TestNode> n;
		
		// Create the target
		Dummy target;

		// Connect the modulation output to the target
		n.connect<0>(target);

		snex::Types::span<float, 2> d;

		// Call one of the process methods in order to handle the
		// modulation
		n.processFrame(d);

		
		expectEquals<double>(target.value[Dummy::Second], 0.666, "modulation connection didn't work");
	}

	template <typename S, typename R> void initSendReceive(S& sender, R& receiver, int numChannels, int blockSize)
	{
		receiver.setFeedback(0.5f);
		sender.connect(receiver);

		PrepareSpecs ps;
		ps.numChannels = numChannels;
		ps.blockSize = blockSize;
		ps.sampleRate = 44100.0;

		sender.prepare(ps);
		receiver.prepare(ps);
	}

	template <typename R, typename S, int NumChannels> auto* createSendReceiveChain(int numSamples)
	{
		using rType = wrap::fix<NumChannels, R>;
		using sType = S;
		using ChainType = container::chain<parameter::empty, rType, math::add<1>, sType>;

		auto* chain = new ChainType();

		auto& receiver = chain->template get<0>();
		auto& adder = chain->template get<1>();
		auto& sender = chain->template get<2>();

		adder.setValue(1.0f);
		receiver.setFeedback(0.5f);
		sender.connect(receiver);

		PrepareSpecs ps;
		ps.numChannels = NumChannels;
		ps.blockSize = numSamples;
		ps.sampleRate = 44100.0;

		try
		{
			chain->prepare(ps);
		}
		catch (scriptnode::Error& )
		{
			jassertfalse;
		}
		

		return chain;
	}

	template <typename CableType, int NumChannels> void testSendReceiveFrame()
	{
		using SpanType = span<float, NumChannels>;
		using SenderType = routing::send<CableType>;
		using ReceiverType = routing::receive<CableType>;
		
		{
			SenderType sender;
			ReceiverType receiver;

			initSendReceive(sender, receiver, NumChannels, 1);

			
			SpanType original, originalE;
            typename CableType::FrameType d(original);

			fillWithInc(d);

			for (auto&v : d)
				v += 1.0f;

            typename CableType::FrameType e(originalE);

			receiver.processFrame(e);
			expectEquals<float>(e[0], 0.0f, "unconnected receiver start value");
			sender.connect(receiver);

			receiver.processFrame(e);
			expectEquals<float>(e[0], 0.0f, "receiver start value");

			sender.processFrame(d);
			receiver.processFrame(e);
			expectEquals<float>(sum(e), sum(d) * 0.5f, "receive doesn't work");

			clear(e);

			sender.reset();
			receiver.processFrame(e);

			expectEquals<float>(e[0], 0.0f, "sender::reset() doesn't work");
		}
		{
			auto chain = createSendReceiveChain<ReceiverType, SenderType, NumChannels>(1);

			SpanType data;
			chain->processFrame(data);
			expectEquals(sum(data), NumChannels * 1.0f, "chain processing 1");
			clear(data);
			chain->processFrame(data);
			expectEquals(sum(data), NumChannels * 1.5f, "chain processing 2");

			// ugly, but we don't care about a proper copy constructor for nodes...
			delete chain;
		}
	}

	template <typename CableType, int NumChannels> void testSendReceiveBlock(int numSamples)
	{
		heap<float> buffer;

		buffer.setSize(NumChannels * numSamples);

		using SenderType = routing::send<CableType>;
		using ReceiverType = routing::receive<CableType>;
		

		{
			auto chain = createSendReceiveChain<ReceiverType, SenderType, NumChannels>(numSamples);

			for (auto& v : buffer)
				v = 3.0f;

			auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(buffer, numSamples);
			ProcessData<NumChannels> data(cd.begin(), numSamples);

			chain->process(data);

			float expected;

			const float bufferSize = (float)buffer.size();

			expected = bufferSize * (3.0f + 1.0f);
			
			expectEquals(sum(buffer), expected, "first round");

			for (auto& v : buffer)
				v = 9.0f;

			chain->process(data);

			expected = bufferSize * (9.0f + 2.0f + 1.0f);
			

			expectEquals(sum(buffer), expected, "second round");

			delete chain;
		}
		
		clear(buffer);
		fillWithInc(buffer);

		{
			using sType = wrap::fix<NumChannels, SenderType>;
			using rType = ReceiverType;

			using ChainType = container::chain<parameter::empty, sType, math::clear<1>, wrap::fix_block<16, rType>>;

			ChainType chain;

			auto& sender = chain.template get<0>();
			auto& receiver = chain.template get<2>();

			sender.connect(receiver);
			receiver.setFeedback(1.0f);
			
			PrepareSpecs ps;
			ps.numChannels = NumChannels;
			ps.blockSize = numSamples;
			ps.sampleRate = 44100.0;

			chain.prepare(ps);

			auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(buffer, numSamples);
			ProcessData<NumChannels> data(cd.begin(), numSamples);

			chain.process(data);

			float i = 0.0f;

			for (auto& s : buffer)
			{
				if (s != i)
				{
					expect(false, "mismatch");
					break;
				}

				i += 1.0f;
			}

			
		}
	}

	void testSendReceive()
	{
		beginTest("Testing send / receive connections");

		testSendReceiveBlock<cable::block<1>, 1>(512);
		testSendReceiveBlock<cable::block<1>, 1>(512);
		testSendReceiveBlock<cable::block<2>, 2>(300);
		testSendReceiveBlock<cable::block<2>, 2>(491);
		//testSendReceiveBlock<cable::dynamic, 1>(491);
		//testSendReceiveBlock<cable::dynamic, 2>(491);
		testSendReceiveFrame<cable::frame<1>, 1>();
		testSendReceiveFrame<cable::frame<2>, 2>();
		testSendReceiveFrame<cable::frame<7>, 7>();
		//testSendReceiveFrame<cable::dynamic, 1>();
		//testSendReceiveFrame<cable::dynamic, 2>();
		//testSendReceiveFrame<cable::dynamic, 7>();
	}

	void testMatrixNode()
	{
		constexpr int NumChannels = 2;
		int numSamples = 512;

		beginTest("Testing matrix nodes");

		using namespace snex::Types;

		PrepareSpecs ps;
		ps.blockSize = numSamples;
		ps.numChannels = NumChannels;

		heap<float> buffer;
		DspHelpers::increaseBuffer(buffer, ps);
		
		auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(buffer, numSamples);
		ProcessData<NumChannels> d(cd.begin(), numSamples, NumChannels);

		clear(buffer);

		for (auto& s : d[0])
			s = 2.0f;

		{
			routing::matrix<matrixtypes::identity<NumChannels>> matrix;

			matrix.process(d);

			auto frame = d.toFrameData();

			while (frame.next())
				expect(frame[0] = 2.0f && frame[1] == 0.0f, "mismatch");
		}

		clear(buffer);

		for (auto& s : d[0])
			s = 1.5f;

		for (auto& s : d[1])
			s = 2.5f;

		{
			

			routing::matrix<right_to_left> matrix;

			matrix.process(d);
			
			auto frame = d.toFrameData();

			while (frame.next())
			{
				expect(frame[0] = 4.0f && frame[1] == 0.0f, "mismatch");
			}
		}
	}

	template <typename T> static void fillWithInc(T& data)
	{
		float v = 0.0f;

		for (auto& s : data)
		{
			s = v;
			v += 1.0f;
		}
	}

	template <typename T> static void clear(const T& data)
	{
		for (auto& s : data)
			s = 0.0f;
	}

	template <typename T> static float sum(const T& data)
	{
		float sum = 0.0f;
		
		for (auto& s : data)
			sum += s;

		return sum;
	}

	template <typename ProcessDataType, typename FrameDataType> float testProcessData(ProcessDataType& pd, FrameDataType& f, const String& typeName)
	{
		int numChannels = pd.getNumChannels();
		int numSamples = pd.getNumSamples();

		using SpanType = typename FrameDataType::FrameType;

		expectEquals(numChannels, ((SpanType)f).size(), "frame type mismatch");

		beginTest("Testing " + typeName + "<" + String(numChannels) + "> with " + String(numSamples) + " samples");

		for (auto c : pd)
			expectEquals(pd.toChannelData(c).size(), numSamples, "iterator block size");

		float v = 0.0f;

		int index = 0;

		while (f.next())
		{
			auto firstElementInt = (int)f[0];

			expect(firstElementInt == index++, "frame processing not interleaved");

			for (auto& s : f)
				v += s;
		}

		expectEquals(index, numSamples, "frame processing didn't use all samples");

		return v;
	}

	template <int NumChannels> void testProcessData(int numSamples)
	{
		heap<float> data;
		data.setSize(NumChannels * numSamples);
		

		testProcessDataFix<NumChannels>(data);
		testProcessDataDyn<NumChannels>(data);
	}
	
	template <int NumChannels> void testProcessDataFix(heap<float>& data)
	{
		auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(data, -1);
		int numSamples = ProcessDataHelpers<NumChannels>::getNumSamplesForConsequentData(data, -1);

		ProcessData<NumChannels> pd(cd.begin(), numSamples);

		{
			fillWithInc(data);

			auto f = pd.toFrameData();
			auto v = testProcessData(pd, f, "ProcessData");

			expectEquals(pd.getNumSamples(), numSamples, "sample divider");
			expectEquals(v, sum(data), "frame processing doesn't work");

			heap<float> copy;
			copy.setSize(data.size());
			ProcessDataHelpers<NumChannels>::copyTo(pd, copy);

			expectEquals<float>(sum(copy), sum(data), "copyTo doesn't work");
		}

		{
			fillWithInc(data);

			auto& dpd = pd.template as<ProcessDataDyn>();

			auto f = dpd.template toFrameData<NumChannels>();
			auto v = testProcessData(pd, f, "ProcessData -> Dyn");

			expectEquals(dpd.getNumSamples(), numSamples, "sample divider");
			expectEquals(v, sum(data), "frame processing doesn't work");
		}
	}

	template <int NumChannels> void testProcessDataDyn(heap<float>& data)
	{
		auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(data, -1);
		int numSamples = ProcessDataHelpers<NumChannels>::getNumSamplesForConsequentData(data, -1);

		ProcessDataDyn pd(cd.begin(), numSamples, NumChannels);

		{
			fillWithInc(data);

			auto f = pd.toFrameData<NumChannels>();
			auto v = testProcessData(pd, f, "ProcessDataDyn");

			expectEquals(pd.getNumSamples(), numSamples, "sample divider");
			expectEquals(v, sum(data), "frame processing doesn't work");
		}

		{
			fillWithInc(data);

			auto& fpd = pd.as<ProcessData<NumChannels>>();
			auto f = fpd.toFrameData();

			auto v = testProcessData(fpd, f, "ProcessDataDyn -> Fix");

			expectEquals(fpd.getNumSamples(), numSamples, "sample divider");
			expectEquals(v, sum(data), "frame processing doesn't work");
		}
	}

	template <int NumSamples, typename T, typename RefData> void expectContainerWorks(T& obj, const String& name, RefData& refData)
	{
		constexpr int NumChannels = T::getNumChannels();
		using ProcessType = ProcessData<NumChannels>;

		heap<float> data;

		{
			data.setSize(NumChannels * NumSamples);
			fillWithInc(data);

			auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(data, NumSamples);
			ProcessType d(cd.begin(), NumSamples);
			obj.process(d);
		}

		heap<float> singleData;

		{
			singleData.setSize(NumChannels * NumSamples);
			fillWithInc(singleData);
			auto cd = ProcessDataHelpers<NumChannels>::makeChannelData(singleData, NumSamples);
			ProcessType d(cd.begin(), NumSamples);

			auto frames = d.toFrameData();

			while (frames.next())
				obj.processFrame(frames);
		}

		int numElements = data.size();

		jassert(refData.size() == numElements);

		String errorName;
		
		errorName << String(NumChannels) + " channels: " + name;

		for (int i = 0; i < numElements; i++)
		{
			auto pOk = refData[i] == data[i];
			auto fOk = refData[i] == singleData[i];

			if (!pOk)
			{
				expect(false, errorName + "::process doesn't work");
				break;
			}

			if (!fOk)
			{
				expect(false, errorName + "::processFrame doesn't work");
				break;
			}
		}
	}

	template <class T, int NumSamples> static T createAndConnect(float firstValue, float secondValue, float thirdValue)
	{
		T c;

		c.template get<0>().op2 = firstValue;
		c.template get<1>().op2 = secondValue;
		c.template get<2>().op2 = thirdValue;

		PrepareSpecs ps;
		ps.blockSize = NumSamples;
		ps.numChannels = c.getNumChannels();
		ps.sampleRate = 44100.0;

		c.prepare(ps);
		
		return c;
	}

	template <int C, int S> span<float, C * S> makeTestSpan()
	{
		constexpr int NumElements = C * S;
		span<float, NumElements> d;
		fillWithInc(d);
		return d;
	}

	template <int NumChannels, int NumSamples> void testContainers()
	{
		using AddType = TestOps::Node<TestOps::Add, NumChannels>;
		using AddParam = parameter::plain<AddType, 0>;
		using MulType = TestOps::Node<TestOps::Mul, NumChannels>;
		using MulParam = parameter::plain<MulType, 0>;
		using PType = parameter::chain<ranges::Identity, AddParam, MulParam>;

		Random r;

		auto v1 = r.nextFloat();
		auto v2 = r.nextFloat();
		auto v3 = r.nextFloat();

		{
			auto c = createAndConnect<container::chain<PType, AddType, MulType, AddType>, NumSamples>(v1, v2, v3);
			auto v = makeTestSpan<NumChannels, NumSamples>();

			for (auto& s : v)
			{
				s += v1;
				s *= v2;
				s += v3;
			}

			expectContainerWorks<NumSamples>(c, "container::chain", v);
		}

		{
			auto c = createAndConnect<container::split<PType, AddType, MulType, AddType>, NumSamples>(v1, v2, v3);
			auto v = makeTestSpan<NumChannels, NumSamples>();
			
			for (auto& s : v)
			{
				auto copy = s;
				s =  copy + v1;
				s += copy * v2;
				s += copy + v3;
			}

			expectContainerWorks<NumSamples>(c, "container::split", v);
		}

		{
			auto c = createAndConnect<container::multi<PType, AddType, MulType, AddType>, NumSamples>(v1, v2, v3);
			auto v = makeTestSpan<NumChannels * 3, NumSamples>();
			
			constexpr int NumMultis = 3;

			for(int s = 0; s < NumSamples; s++)
			{
				for (int c = 0; c < NumChannels; c++)
				{
					int cOffset = NumSamples * c;

					for (int m = 0; m < NumMultis; m++)
					{
						int mOffset = NumSamples * NumChannels * m;
						int posInData = s + mOffset + cOffset;
						auto& value = v[posInData];

						if (m == 0) value += v1;
						if (m == 1) value *= v2;
						if (m == 2) value += v3;
					}
				}
			}

			expectContainerWorks<NumSamples>(c, "container::multi", v);
		}
	}

	struct PropertyDummy
	{
		double propertyValue = 2.0;
	};

};

static ScriptNodeTests snt;

}

#endif

