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






/** mother of all nodes. */
struct mothernode
{
	using DataProvider = scriptnode::data::pimpl::provider_base;

    mothernode& operator=(const mothernode& other)
    {
        data_provider = other.data_provider;
        return *this;
    };
    
	virtual ~mothernode() {};

	template <typename T> static constexpr bool isBaseOf()
	{
		return std::is_base_of<mothernode, typename T::WrappedObjectType>();
	}

	template <typename T> static mothernode* getAsBase(T& obj)
	{
		jassert(isBaseOf<T>());
		return dynamic_cast<mothernode*>(&obj.getWrappedObject());
	}

	void setDataProvider(DataProvider* dp)
	{
		data_provider = dp;
	}

	DataProvider* getDataProvider() const { return data_provider; }

private:

	DataProvider* data_provider = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(mothernode);
};



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
	

	OpaqueNode(const OpaqueNode& other) = default;

	virtual ~OpaqueNode();

	template <typename T> static String getDescription(const T& t)
	{
		if constexpr (prototypes::check::getDescription<typename T::WrappedObjectType>::value)
			return t.getWrappedObject().getDescription();
		else 
			return {};
	}

	template <typename T> void create()
	{
		callDestructor();
		allocateObjectSize(sizeof(T));

		destructFunc = prototypes::static_wrappers<T>::destruct;
		prepareFunc = prototypes::static_wrappers<T>::prepare;
		resetFunc = prototypes::static_wrappers<T>::reset;

		processFunc = prototypes::static_wrappers<T>::template process<ProcessDataDyn>;
		monoFrame = prototypes::static_wrappers<T>::template processFrame<MonoFrame>;
		stereoFrame = prototypes::static_wrappers<T>::template processFrame<StereoFrame>;
		initFunc = prototypes::static_wrappers<T>::initialise;
		eventFunc = prototypes::static_wrappers<T>::handleHiseEvent;

		auto t = prototypes::static_wrappers<T>::create(getObjectPtr());
		isPoly = t->isPolyphonic();

#if !HISE_NO_GUI_TOOLS
		description = getDescription(*t);
#endif

		if constexpr (mothernode::isBaseOf<T>())
			mnPtr = mothernode::getAsBase(*static_cast<T*>(getObjectPtr()));

		if constexpr (prototypes::check::setExternalData<T>::value)
			externalDataFunc = prototypes::static_wrappers<T>::setExternalData;
		else
			externalDataFunc = prototypes::noop::setExternalData;

		if constexpr (prototypes::check::isProcessingHiseEvent<T>::value)
			shouldProcessHiseEvent = t->isProcessingHiseEvent();

        if constexpr (prototypes::check::connectToRuntimeTarget<T>::value)
            connectRuntimeFunc = prototypes::static_wrappers<T>::connectToRuntimeTarget;
        
		if constexpr (prototypes::check::hasTail<T>::value)
			hasTail_ = t->hasTail();

		if constexpr (prototypes::check::isSuspendedOnSilence<T>::value)
			canBeSuspended_ = t->isSuspendedOnSilence();

		if constexpr (prototypes::check::getFixChannelAmount<typename T::ObjectType>::value)
			numChannels = T::ObjectType::getFixChannelAmount();
		else
			numChannels = -1;

		if constexpr (prototypes::check::handleModulation<T>::value)
		{
			modFunc = prototypes::static_wrappers<T>::handleModulation;

			if constexpr (prototypes::check::isNormalisedModulation<typename T::WrappedObjectType>::value)
				isNormalised = T::WrappedObjectType::isNormalisedModulation();
		}
		else
			modFunc = prototypes::noop::handleModulation;

		ParameterDataList pList;

		t->createParameters(pList);

		fillParameterList(pList);

		
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

    void connectToRuntimeTarget(bool add, const runtime_target::connection& c);
    
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

	void* getObjectPtr() const
    {
        auto obj = this->object.getObjectPtr();
        jassert(obj != nullptr);
        return obj;
    }

	mothernode* getObjectAsMotherNode() { return mnPtr; }

	bool hasComplexData() const
	{
		int numData = 0;

		for (int i = 0; i < (int)ExternalData::DataType::numDataTypes; i++)
			numData += numDataObjects[i];

		return numData > 0;
	}

	bool isProcessingHiseEvent() const { return shouldProcessHiseEvent; }

	void setCanBePolyphonic()
	{
		isPolyPossible = true;
	}

	bool canBePolyphonic() const
	{
		return isPolyPossible;
	}

	void fillParameterList(ParameterDataList& d);

	String getDescription() const { return description; }

	bool hasTail() const { return hasTail_; }

	bool isSuspendedOnSilence() const { return canBeSuspended_; }

private:

	String description;

	mothernode* mnPtr = nullptr;

	void allocateObjectSize(int numBytes);

	hise::ObjectStorage<SmallObjectSize, 16> object;

	bool isPoly = false;
	bool isPolyPossible = false;

	bool hasTail_ = true;
	bool canBeSuspended_ = false;

	prototypes::handleHiseEvent eventFunc = nullptr;
	prototypes::destruct destructFunc = nullptr;
	prototypes::prepare prepareFunc = nullptr;
	prototypes::reset resetFunc = nullptr;
	prototypes::process<ProcessDataDyn> processFunc = nullptr;
	prototypes::processFrame<MonoFrame> monoFrame = nullptr;
	prototypes::processFrame<StereoFrame> stereoFrame = nullptr;
	prototypes::initialise initFunc = nullptr;
	prototypes::setExternalData externalDataFunc = nullptr;
    prototypes::connectRuntimeTarget connectRuntimeFunc = nullptr;
	prototypes::handleModulation modFunc;

	Array<parameter::data> parameters;

public:

	bool shouldProcessHiseEvent = false;
	bool isNormalised = false;

	parameter::data* getParameter(int index)
	{
		if (isPositiveAndBelow(index, numParameters))
		{
			return parameters.getRawDataPointer() + index;
		}

		return nullptr;
	}

	struct ParameterIterator
	{
		ParameterIterator(OpaqueNode& on_) :
			on(on_)
		{};

		parameter::data* begin() const
		{
			return on.parameters.begin();
		}

		parameter::data* end() const
		{
			return on.parameters.end();
		}

		OpaqueNode& on;
	};

	int numChannels = -1;
	int numParameters = 0;
	int numDataObjects[(int)ExternalData::DataType::numDataTypes];
};



namespace dll
{
	/** The base class for both factory types. */
	struct FactoryBase
	{
		virtual ~FactoryBase() {};
		virtual int getNumNodes() const = 0;
		virtual String getId(int index) const = 0;
		virtual bool initOpaqueNode(OpaqueNode* n, int index, bool polyphonicIfPossible) = 0;
		virtual int getNumDataObjects(int index, int dataTypeAsInt) const = 0;
		virtual int getWrapperType(int index) const = 0;

		virtual int getHash(int index) const = 0;

		virtual Error getError() const = 0;
		virtual void clearError() const = 0;

		virtual bool isThirdPartyNode(int index) const = 0;

		virtual void deinitOpaqueNode(OpaqueNode* n) { }
	};

	/** A Factory that initialises the nodes using the templated OpaqueNode::create function.
	
		This will be used inside the project DLL (or the compiled plugin when the project nodes
		are embedded into the code. 
	*/
	struct StaticLibraryHostFactory : public FactoryBase
	{
		struct Item
		{
			String networkData;
			String id;
			bool isModNode = false;
			std::function<void(scriptnode::OpaqueNode* n)> f;
			std::function<void(scriptnode::OpaqueNode* n)> pf;
			int numDataObjects[(int)ExternalData::DataType::numDataTypes];
		};

		Array<Item> items;

		String getId(int index) const override;
		int getNumNodes() const override;

		int getWrapperType(int index) const override;

		bool isInterpretedNetwork(int index) const { return items[index].networkData.isNotEmpty(); }

		bool initOpaqueNode(scriptnode::OpaqueNode* n, int index, bool polyphonicIfPossible) override;

		int getNumDataObjects(int index, int dataTypeAsInt) const override;

		int getHash(int) const override { return -1; }

		/** We don't bother about whether it's a third party node or not in a static compiled plugin. */
		bool isThirdPartyNode(int index) const override { return false; };

		template <typename T, typename PolyT> void registerPolyNode()
		{
			registerNode<T>();
			items.getReference(items.size() - 1).pf = [](scriptnode::OpaqueNode* n) { n->create<PolyT>(); };
		}

		template <typename T> void registerDataNode()
		{
			Item i;
			T obj;

			i.id = obj.getId();
			i.isModNode = obj.isModNode();
			i.networkData = obj.getNetworkData();

			items.add(i);
		}

		template <typename T> void registerNode()
		{
			Item i;
			i.id = T::ObjectType::MetadataClass::getStaticId().toString();
			i.isModNode = T::ObjectType::isModNode();
			i.f = [](scriptnode::OpaqueNode* n) { n->create<T>(); };

			i.numDataObjects[(int)ExternalData::DataType::Table] = T::ObjectType::NumTables;
			i.numDataObjects[(int)ExternalData::DataType::SliderPack] = T::ObjectType::NumSliderPacks;
			i.numDataObjects[(int)ExternalData::DataType::AudioFile] = T::ObjectType::NumAudioFiles;
			i.numDataObjects[(int)ExternalData::DataType::FilterCoefficients] = T::ObjectType::NumFilters;
			i.numDataObjects[(int)ExternalData::DataType::DisplayBuffer] = T::ObjectType::NumDisplayBuffers;

			items.add(i);
		}

		Error getError() const override;

		void clearError() const override;
	};

	/** A reference counted object around a dynamic library and functions
		for creating / querying node specs. */
	struct ProjectDll : public ReferenceCountedObject
	{
		// This is just used to check whether the dll is deprecated and needs to be recompiled...
		// (It will be bumped whenever a breaking change into the DLL API is introduced)...
		static constexpr int DllUpdateCounter = 3;

		using Ptr = ReferenceCountedObjectPtr<ProjectDll>;

		enum class ExportedFunction
		{
			GetHash,
			GetWrapperType,
			GetNumNodes,
			GetNodeId,
			InitOpaqueNode,
			DeInitOpaqueNode,
			GetNumDataObjects,
			GetError,
			ClearError,
			IsThirdPartyNode,
			GetDLLVersionCounter,
			numFunctions
		};

		static String getFuncName(ExportedFunction f);

		typedef int(*GetHash)(int);
		typedef int(*GetWrapperType)(int);
		typedef int(*GetNumNodes)();
		typedef size_t(*GetNodeId)(int, char*);
		typedef void(*InitOpaqueNode)(scriptnode::OpaqueNode*, int, bool);
		typedef void(*DeInitOpaqueNode)(scriptnode::OpaqueNode*);
		typedef int(*GetNumDataObjects)(int, int);
		typedef Error(*GetError)();
		typedef void(*ClearError)();
		typedef bool(*IsThirdPartyNode)(int);
		typedef int(*GetDllVersionCounter)();

		int getWrapperType(int index) const;

		int getNumNodes() const;

		int getNumDataObjects(int nodeIndex, int dataTypeAsInt) const;

		bool isThirdPartyNode(int nodeIndex) const;

		String getNodeId(int index) const;

		bool initOpaqueNode(OpaqueNode* n, int index, bool polyphonicIfPossible);

		void deInitOpaqueNode(OpaqueNode* n);

		void clearError() const;

		Error getError() const;

		ProjectDll(const File& f);

		~ProjectDll();

		operator bool() const
		{
			return r.wasOk();
		}

		String getInitError() const { return r.getErrorMessage(); }

		int getHash(int index) const;

        File getDllFile() const { return loadedFile; }
        
	private:

		void clearAllFunctions()
		{
			memset(functions, 0, sizeof(void*) * (int)ExportedFunction::numFunctions);
		}

		void* getFromDll(ExportedFunction f, const File& dllFile)
		{
			auto id = getFuncName(f);

			void* func = nullptr;

			if (r.failed())
				return func;

			func = dll->getFunction(id);

			if (func == nullptr)
			{
				r = Result::fail("Can't find function " + id + "() in " + dllFile.getFileName());
				clearAllFunctions();
			}
				
			return func;
		};

        File loadedFile;
		Result r;

		void* functions[(int)ExportedFunction::numFunctions];

		ScopedPointer<DynamicLibrary> dll;
	};

	/** The factory that creates the nodes on the host side.
	
		It will call into the provided dll and initialise the given opaque node using
		the DLL functions. 

		It uses a reference counted pointer to the given DLL object, so creating and
		using these is very lightweight.
	*/
	struct DynamicLibraryHostFactory : public FactoryBase
	{
		DynamicLibraryHostFactory(ProjectDll::Ptr dll_);

		~DynamicLibraryHostFactory();

		int getNumNodes() const override;
		String getId(int index) const override;
		bool initOpaqueNode(scriptnode::OpaqueNode* n, int index, bool polyphonicIfPossible) override;
		int getNumDataObjects(int index, int dataTypeAsInt) const override;
		int getWrapperType(int index) const override;

		bool isThirdPartyNode(int index) const override;

		void clearError() const override;

		Error getError() const override;
		
		void deinitOpaqueNode(scriptnode::OpaqueNode* n) override;

		int getHash(int index) const override;

	private:

		ProjectDll::Ptr projectDll;
	};

	struct InterpretedNetworkData
	{
		virtual ~InterpretedNetworkData() {};

		virtual String getId() const = 0;
		virtual bool isModNode() const = 0;
		virtual String getNetworkData() const = 0;
	};
}

} 
