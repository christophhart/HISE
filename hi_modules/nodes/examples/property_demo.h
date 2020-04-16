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
using namespace snex::Types;

namespace examples
{
/** This node demonstrates how to add "heavyweight" properties to a node.

	A property in scriptnode is a more complex data that is not supposed to be controlled
	at audio rate, but rather set processing options or define external resources.

	They can be changed using the scriptnode IDE as well as with the upcoming HiseScript API
	and offer full undo support and are stored persistently in the scriptnode patch.
*/
namespace property_demo_impl
{

struct processor
{
	DECLARE_SNEX_NODE(processor);

	bool isPolyphonic() const { return false; }

	void reset() {}
	void handleHiseEvent(HiseEvent& e) {}
	void prepare(PrepareSpecs ps) {}
	void processSingle(float* data, int numChannels) {}
	void process(ProcessData& d) {}
	bool handleModulation(double& v) {}

	template <int P> void setParameter(double v) {}

	void fill(float value)
	{
		for (auto& s : data.toSimd())
			s = value;
	}

	void copyFromFile(dyn<float> fileContent)
	{
		for (int i = 0; i < 512; i++)
		{
			data[i] = fileContent[i];
		}
	}

	void referToTable(dyn<float> newTableData)
	{
		tableData = newTableData;
	}

	void referToSliderPack(dyn<float> newSliderPackData)
	{
		sliderPackData = newSliderPackData;
	}

	dyn<float> tableData;
	dyn<float> sliderPackData;

	span<float, 512> data = { 0.0f };
};

using Type = container::chain<parameter::empty, processor>;

struct initialiser
{
	DECLARE_SNEX_INITIALISER(property_demo);
	void initialise(Type& c) {}
};

/** This class provides a public property that can be changed in scriptnode. */
struct ArrayFiller
{
	using Property = properties::native<ArrayFiller>;

	/** this macro defines the name, type and default value of the property. */
	DECLARE_SNEX_NATIVE_PROPERTY(ArrayFiller, float, 0.2f);

	/** This method will be called when the property has changed. The first parameter
		will always be the root object of the instance you've created, so you can control
		multiple child nodes at once.
	*/
	void set(Type& obj, float newValue)
	{
		obj.get<0>().fill(newValue);
	}
};


struct MyFile
{
	using FileType = span<dyn<float>, 1>;
	using Property = properties::file<MyFile, 1>;
	
	DECLARE_SNEX_COMPLEX_PROPERTY(MyFileProperty);

	
	void setFile(Type& obj, double sampleRate, FileType& data)
	{
		obj.get<0>().copyFromFile(data[0]);
	}
};

struct MyTable
{
	using Property = properties::table<MyTable>;

	DECLARE_SNEX_COMPLEX_PROPERTY(MyTable);

	void setTableData(Type& obj, dyn<float> newData)
	{
		obj.get<0>().referToTable(newData);
	}
};

#if 0
struct MyPack
{
	using Property = properties::slider_pack<MyPack>;

	DECLARE_SNEX_COMPLEX_PROPERTY(MyPack);

	void setSliderPackData(Type& obj, dyn<float> newData)
	{
		obj.get<0>().referToSliderPack(newData);
	}
};
#endif

using props = properties::list<ArrayFiller::Property, MyFile::Property, MyTable::Property>;

using instance = cpp_node<initialiser, Type, props>;

}

using property_demo = property_demo_impl::instance;

}

}
