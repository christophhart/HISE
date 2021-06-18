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
	DECLARE_NODE(processor);

	static const int NumChannels = 2;

	bool isPolyphonic() const { return false; }

	void reset() {}
	void handleHiseEvent(HiseEvent& e) {}
	void prepare(PrepareSpecs ps) {}
	void processFrame(span<float, NumChannels>& d) {}
	void process(ProcessData<NumChannels>& d) {}
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
	// This alias will be used below and wraps this class into a
	// properties::native template that can be used if the property is
	// a native type (int, double or float)...
	using Property = properties::native<ArrayFiller>;

	/** this macro defines the name, type and default value of the property. */
	DECLARE_SNEX_NATIVE_PROPERTY(ArrayFiller, float, 0.2f);

	/** This method will be called when the property has changed. The first parameter
		will always be the root object of the instance you've created, so you can control
		multiple child nodes at once.

		The second parameter will be the native value type defined by the
		DECLARE_SNEX_NATIVE_PROPERTY() macro.
	*/
	void set(Type& obj, float newValue)
	{
		obj.get<0>().fill(newValue);
	}
};



/** This property can be used to provide access to external files. */
struct MyFile
{
	/** You need to supply a number of channels that this property expects.
		Channel mismatches will be converted automatically if possible. */
	static const int NumChannels = 1;

	/** This alias will be used later, but it will be also used in the 
		setFile method to shorten the second argument. */
	using Property = properties::file<MyFile, NumChannels>;
	
	DECLARE_SNEX_COMPLEX_PROPERTY(MyFileProperty);

	void setFile(Type& obj, Property::type& file)
	{
		obj.get<0>().copyFromFile(file.data[0]);
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


struct SnexTableProperty
{
	void setTableData(ComplexType::Ptr propType, ComplexType::Ptr dataType, void* node)
	{
		ComplexType::InitData d;
		d.dataPointer = node;
		d.initValues = dataType->makeDefaultInitialiserList();
		dataType->initialise(d);
	}

	
}

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

using instance = node<initialiser, Type, props>;

}

using property_demo = property_demo_impl::instance;

}

}
