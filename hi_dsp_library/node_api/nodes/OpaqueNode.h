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

namespace scriptnode { 
using namespace juce;
using namespace hise;

using namespace snex;
using namespace Types;



/** A mysterious wrapper that will use a rather old-school, plain C API for the callbacks. 

	It holds a number of typed function pointers to the callbacks of the scriptnode system
	and allocates an object of the wrapped node. This allows both external nodes via DLLs and
	JIT compiled nodes as well as avoid huge template sizes for each subtype. 
	
	In order to use this node, just create it, and then call the templated create() function and
	it will setup all function pointers using the prototypes namespace and allocate the bytes
	required for the object and call its constructor / destructor. 

	Depending on the object size it will allocate on the heap or use a internal buffer to keep the cache
	pressure as low as possible.
	*/
struct OpaqueNode
{
	static constexpr int NumMaxParameters = 16;
	static constexpr int SmallObjectSize = 128;

	using MonoFrame = span<float, 1>;
	using StereoFrame = span<float, 2>;

	SN_GET_SELF_AS_OBJECT(OpaqueNode);

	OpaqueNode();

	OpaqueNode(OpaqueNode&& other) = default;
	OpaqueNode& operator=(OpaqueNode&& other) = default;

	OpaqueNode(const OpaqueNode& other) = default;

	virtual ~OpaqueNode();

	template <typename T> void create()
	{
		callDestructor();
		allocateObjectSize(sizeof(T));

		destructFunc = prototypes::static_wrappers<T>::destruct;
		prepareFunc = prototypes::static_wrappers<T>::prepare;
		resetFunc = prototypes::static_wrappers<T>::reset;
		eventFunc = prototypes::static_wrappers<T>::handleHiseEvent;
		processFunc = prototypes::static_wrappers<T>::template process<ProcessDataDyn>;
		monoFrame = prototypes::static_wrappers<T>::template processFrame<MonoFrame>;
		stereoFrame = prototypes::static_wrappers<T>::template processFrame<StereoFrame>;
		initFunc = prototypes::static_wrappers<T>::initialise;

		auto t = prototypes::static_wrappers<T>::create(getObjectPtr());
		isPoly = t->isPolyphonic();

		if constexpr (prototypes::check::setExternalData<T>::value)
			externalDataFunc = prototypes::static_wrappers<T>::setExternalData;
		else
			externalDataFunc = prototypes::noop::setExternalData;

		if constexpr (prototypes::check::handleModulation<T>::value)
		{
			modFunc = prototypes::static_wrappers<T>::handleModulation;

			if constexpr (prototypes::check::isNormalisedModulation<T::WrappedObjectType>::value)
				isNormalised = T::WrappedObjectType::isNormalisedModulation();
		}
		else
			modFunc = prototypes::noop::handleModulation;

		ParameterDataList pList;

		t->createParameters(pList);

		numParameters = pList.size();

		for (int i = 0; i < numParameters; i++)
		{
			parameters[i] = pList[i].info;
			parameterFunctions[i] = pList[i].callback.getFunction();
			parameterObjects[i] = pList[i].callback.getObjectPtr();
			parameterNames[i] = pList[i].parameterNames;
		}
	}

	template <typename T> T& as()
	{
		return *static_cast<T*>(getObjectPtr());
	}

	template <typename T> const T& as() const
	{
		return *static_cast<T*>(this->getObjectPtr());
	}

	void initialise(NodeBase* n)
	{
		if (initFunc)
			initFunc(this->getObjectPtr(), n);
	}

	bool isPolyphonic() const { return isPoly; };

	void prepare(PrepareSpecs ps);

	void process(ProcessDataDyn& data);

	void processFrame(MonoFrame& d);

	void processFrame(StereoFrame& d);

	void reset();

	void handleHiseEvent(HiseEvent& e);

	void setExternalData(const ExternalData& b, int index);

	void createParameters(ParameterDataList& l);

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		// should always be forwarded directly...
		jassertfalse;
	}

	void initExternalData(ExternalDataHolder* externalDataHolder);

	void setExternalPtr(void* externPtr);

	void callDestructor();

	bool handleModulation(double& d);

	void* getObjectPtr() const { return this->object.getObjectPtr(); }

	bool hasComplexData() const
	{
		int numData = 0;

		for (int i = 0; i < (int)ExternalData::DataType::numDataTypes; i++)
			numData += numDataObjects[i];

		return numData > 0;
	}

private:

	void allocateObjectSize(int numBytes);

	hise::ObjectStorage<SmallObjectSize, 16> object;

	bool isPoly = false;

	prototypes::handleHiseEvent eventFunc = nullptr;
	prototypes::destruct destructFunc = nullptr;
	prototypes::prepare prepareFunc = nullptr;
	prototypes::reset resetFunc = nullptr;
	prototypes::process<ProcessDataDyn> processFunc = nullptr;
	prototypes::processFrame<MonoFrame> monoFrame = nullptr;
	prototypes::processFrame<StereoFrame> stereoFrame = nullptr;
	prototypes::initialise initFunc = nullptr;
	prototypes::setExternalData externalDataFunc = nullptr;
	prototypes::handleModulation modFunc;

public:

	bool isNormalised = false;
	span<parameter::pod, NumMaxParameters> parameters;
	span<prototypes::setParameter, NumMaxParameters> parameterFunctions;
	span<void*, NumMaxParameters> parameterObjects;
	span<StringArray, NumMaxParameters> parameterNames;

	int numParameters = 0;
	int numDataObjects[(int)ExternalData::DataType::numDataTypes];
};



namespace dll
{
	/** A reference counted object around a dynamic library and functions
	    for creating / querying node specs. */
	struct ProjectDll : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<ProjectDll>;

		typedef int(*GetNumNodesFunc)();
		typedef size_t(*GetNodeIdFunc)(int, char*);
		typedef void(*InitNodeFunc)(scriptnode::OpaqueNode*, int);
		typedef void(*DeleteNodeFunc)(void* obj);
		typedef int(*GetNumDataObjects)(int, int);

		int getNumNodes() const;

		int getNumDataObjects(int nodeIndex, int dataTypeAsInt) const;

		String getNodeId(int index) const;

		bool initNode(OpaqueNode* n, int index);

		ProjectDll(const File& f);

		~ProjectDll();

		operator bool() const
		{
			return ok;
		}

		int getHash() const
		{
			return hash;
		}

	private:

		int hash = 0;
		bool ok = false;

		GetNumNodesFunc gnnf;
		GetNodeIdFunc gnif;
		InitNodeFunc inf;
		DeleteNodeFunc dnf;
		GetNumDataObjects gndo;

		ScopedPointer<DynamicLibrary> dll;
	};

	/** The base class for both factory types. */
	struct FactoryBase
	{
		virtual ~FactoryBase() {};
		virtual int getNumNodes() const = 0;
		virtual String getId(int index) const = 0;
		virtual bool initOpaqueNode(OpaqueNode* n, int index) = 0;
		virtual int getNumDataObjects(int index, int dataTypeAsInt) const = 0;

	};

	/** A Factory that initialises the nodes using the templated OpaqueNode::create function.
	
		This will be used inside the project DLL (or the compiled plugin when the project nodes
		are embedded into the code. 
	*/
	struct PluginFactory : public FactoryBase
	{
		struct Item
		{
			String id;
			std::function<void(scriptnode::OpaqueNode* n)> f;
			int numDataObjects[(int)ExternalData::DataType::numDataTypes];
		};

		Array<Item> items;

		String getId(int index) const override;
		int getNumNodes() const override;
		bool initOpaqueNode(scriptnode::OpaqueNode* n, int index) override;

		int getNumDataObjects(int index, int dataTypeAsInt) const override;

		template <typename T> void registerNode()
		{
			Item i;
			i.id = T::MetadataClass::getStaticId().toString();
			i.f = [](scriptnode::OpaqueNode* n) { n->create<T>(); };

			i.numDataObjects[(int)ExternalData::DataType::Table] = T::NumTables;
			i.numDataObjects[(int)ExternalData::DataType::SliderPack] = T::NumSliderPacks;
			i.numDataObjects[(int)ExternalData::DataType::AudioFile] = T::NumAudioFiles;
			i.numDataObjects[(int)ExternalData::DataType::FilterCoefficients] = T::NumFilters;
			i.numDataObjects[(int)ExternalData::DataType::DisplayBuffer] = T::NumDisplayBuffers;

			items.add(i);
		}
	};

	/** The factory that creates the nodes on the host side.
	
		It will call into the provided dll and initialise the given opaque node using
		the DLL functions. 

		It uses a reference counted pointer to the given DLL object, so creating and
		using these is very lightweight.
	*/
	struct HostFactory : public FactoryBase
	{
		HostFactory(ProjectDll::Ptr dll_);

		~HostFactory();

		int getNumNodes() const override;

		String getId(int index) const override;

		bool initOpaqueNode(scriptnode::OpaqueNode* n, int index) override;

		int getNumDataObjects(int index, int dataTypeAsInt) const override;

	private:

		ProjectDll::Ptr projectDll;
	};

}

} 
