/** SNEX / Scriptnode Example code

	These code examples demonstrate the C++ API of scriptnode.

	If you export a scriptnode patch to C++ code, it will look similar to these code examples.
	Also, these examples can be compiled by the SNEX JIT compiler in order to create
	machine-code optimized nodes on runtime.

	The examples have a varying degree of complexity and focus on one particular feature.
*/

#pragma once


namespace scriptnode {
using namespace juce;
using namespace hise;

namespace examples
{

/** This example takes a previously defined node (from the hello world example) and combines
	it with a stock one pole filter and connects the filters frequency to a second macro parameter.

	You'll notice that this example is purely declarative: there is no logic code, and the entire
	node is being assembled at compile time.

*/
namespace combined_impl
{

/** We'll define the parameter to the hello_world node.
	Be aware that this is not calling hello_world_impl::processor::setParameter<0>()
	directly, but wraps it through the chain's macro parameter.
*/
using HelloParameter = parameter::plain<hello_world, 0>;

/** We'll use a frequency range from 20Hz to 2kHz. */
DECLARE_PARAMETER_RANGE(filterRange, 20.0, 2000.0);
using FilterParameter = parameter::from0to1<filters::one_pole, filters::one_pole::Frequency, filterRange>;

/** Now we'll chain these two together. */
using PType = parameter::chain<ranges::Identity, HelloParameter, FilterParameter>;

/** And we'll define a chain with two nodes and the parameter chain defined above. */
using ChainType = container::split<PType, fix<2, filters::one_pole>, hello_world>;

struct data
{
	DECLARE_SNEX_INITIALISER(combined);

	void initialise(ChainType& obj)
	{
		auto& hw = obj.get<1>();
		auto& filter = obj.get<0>();

		auto& p1 = obj.getParameter<0>();
		
		p1.connect<0>(hw);
		p1.connect<1>(filter);
	}
};

using instance = node<data, ChainType>;

}

using combined = combined_impl::instance;


}

}
