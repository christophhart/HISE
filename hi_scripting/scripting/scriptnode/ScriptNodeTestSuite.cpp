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

#if HI_RUN_UNIT_TESTS

#include "AppConfig.h"

#include  "JuceHeader.h"




namespace scriptnode
{

using namespace hise;
using namespace juce;















/** This is just a simple test node that has all functions that a node is expected to have. */
struct TestNode
{
	DECLARE_SNEX_NODE(TestNode);

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

	void processSingle(float* data, int numChannels)
	{
		data[0] = value;
	}

	void process(ProcessData& d)
	{
		for (int i = 0; i < d.size; i++)
		{
			float data[NUM_MAX_CHANNELS];

			d.copyToFrameDynamic(data);
			processSingle(data, d.numChannels);
			d.copyFromFrameAndAdvanceDynamic(data);
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













struct ScriptNodeTests : public juce::UnitTest
{
	ScriptNodeTests() :
		UnitTest("Scriptnode tests")
	{};

	void runTest() override
	{
		testRangeTemplates();
		testParameters();
		testModWrapper();
		testProperties();
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

		GET_SELF_AS_OBJECT(Dummy);

		template <int P> constexpr static void setParameter(void* obj, double v)
		{
			static_cast<Dummy*>(obj)->value[P] = v;
		}

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

		expectEquals(skewRange::from0To1(0.6), d.convertFrom0to1(0.4));
		expectEquals(skewRange::from0To1(1.0), d.convertFrom0to1(1.0));
		expectEquals(skewRange::to0To1(14.4), d.convertTo0to1(14.4));
	}

	void testParameters()
	{
		{
			beginTest("Testing simple parameter");

			// Declare a parameter range class
			DECLARE_PARAMETER_RANGE(testRange, 0.5, 0.7);

			// Create a dummy node
			Dummy d;

			// Create a parameter to the first parameter slot of 
			// the Dummy class using the range defined above
			parameter::from0to1<Dummy, Dummy::First, testRange> p;

			// Connect the parameter to the dummy node object
			p.connect(d);

			// set the parameter to 0.5
			p.call(0.5);

			expectEquals(d.value[Dummy::First], 0.6, "simple parameter test");
		}

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
			chain.connect<FirstMacro>(obj);

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
			o.connect<0>(i);

			// Connect the inner chain's parameter to the Dummy node
			i.connect<0>(d);
			
			// Set the outer parameter and it will propagate through
			// the nested parameter connections
			o.setParameter<0>(0.5);

			expectEquals(d.value[2], 0.5, "nested call");
			expectEquals(*(reinterpret_cast<double*>(&o) + 2), 0.5, "no fat");
		}

		{
			beginTest("Testing parameter chain");

			// Create three ranges that are going to be used for the parameter connections.
			DECLARE_PARAMETER_RANGE(inputRange, 0.0, 2.0);
			DECLARE_PARAMETER_RANGE(firstRange, 1.0, 3.0);
			DECLARE_PARAMETER_RANGE(secondRange, 10.0, 30.0);

			// Create an parameter chain using the input range and two connections to dummy parameters.
			using ParameterType = parameter::chain<inputRange, parameter::from0to1<Dummy, Dummy::First, firstRange>, 
														       parameter::from0to1<Dummy, Dummy::Second, secondRange>>;

			// Now we'll put that parameter into a list so it matches
			// the usual way how container parameters are declared
			using ParameterList = parameter::list<ParameterType>;

			// Create a chain with two Dummy nodes that are being controlled by one parameter
			container::chain<ParameterList, Dummy, Dummy> c;

			// get a reference to the parameter chain
			auto& p = c.getParameter<0>();

			// get the reference to the both nodes
			auto& first = c.get<0>();
			auto& second = c.get<1>();

			// connect the first connection in the chain to
			// the first node
			c.connect<0, 0>(first);

			// connect the second one to the second node
			c.connect<0, 1>(second);

			// now set the parameter and it will update both nodes
			c.setParameter<0>(1.5);

			auto fValue = first.value[Dummy::First];
			auto sValue = second.value[Dummy::Second];

			expectEquals<double>(fValue, 2.5, "first parameter");
			expectEquals<double>(sValue, 25.0, "second parameter");

		}

		{
			beginTest("Testing cpp_node template class");

			// Create an alias that will contain the structure of the inner network
			using ChainType = container::chain<parameter::plain<TestNode, 0>, TestNode>;

			// Write a class that initialises
			struct handler
			{
				void initialise(ChainType& obj)
				{
					auto& tn = obj.get<0>();

					tn.setParameter<0>(4.0);
					obj.connect<0>(tn);
				}
			};

			cpp_node<handler, ChainType> node;

			Array<HiseDspBase::ParameterData> p;

			node.initialise(nullptr);
			node.createParameters(p);

			expect(p.size() == 1, "not one parameter");
			
			if (p.size() == 1)
				p.getReference(0)(0.6);


			float d[2];

			node.processSingle(d, 2);

			expectEquals<float>(d[0], 0.6f, "parameter set not working");
		}
		
	}

	void testModWrapper()
	{
		beginTest("Testing mod wrapper");

		// Create a wrapped test node with a parameter to the second parameter
		// of a Dummy node
		wrap::mod<TestNode, parameter::plain<Dummy, Dummy::Second>> n;
		
		// Create the target
		Dummy target;

		// Connect the modulation output to the target
		n.connect<0>(target);

		float d[2];

		// Call one of the process methods in order to handle the
		// modulation
		n.processSingle(d, 2);

		
		expectEquals<float>(target.value[Dummy::Second], 0.666, "modulation connection didn't work");
	}

	struct PropertyDummy
	{
		double propertyValue = 2.0;
	};

	void testProperties()
	{
		using TestChain = container::chain<parameter::empty, TestNode, TestNode>;

		struct MyProperty
		{
			DECLARE_SNEX_NATIVE_PROPERTY(MyProperty, double, 2.0);

			void set(TestChain& n, DataType d)
			{
				n.get<0>().propertyValue = d;
				n.get<1>().propertyValue = d / 2.0;
			}
		};

		TestChain c;
		properties::list<properties::native<MyProperty>> p;

		p.initWithRoot(nullptr, nullptr, c);
		
	}
};

static ScriptNodeTests snt;

}

#endif

