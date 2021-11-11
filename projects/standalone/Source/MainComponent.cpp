/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#define S(x) String(x, 2)


#include "MainComponent.h"

// Use this to quickly scale the window
#define SCALE_2 0



/** 
stack.setDataType({
	"Data": 2,
	"Number" : 0.0,
	"MyValue" : [1, 2],
	"MySubObject" :
	{
		"value": 2.0
	}
	});
*/

struct FixMemoryLayout
{
	enum class DataType
	{
		Integer,
		Boolean,
		Float,
		numTypes
	};

	struct Helpers
	{
		static void writeElement(DataType type, uint8* dataWithOffset, const var& newValue)
		{
			switch (type)
			{
			case DataType::Integer: *reinterpret_cast<int*>(dataWithOffset) = (int)newValue; break;
			case DataType::Float:	*reinterpret_cast<float*>(dataWithOffset) = (float)newValue; break;
			case DataType::Boolean: *reinterpret_cast<int*>(dataWithOffset) = (int)(bool)newValue; break;
			}
		}

		static var getElement(DataType type, const uint8* dataWithOffset)
		{
			switch (type)
			{
			case DataType::Integer: return var(*reinterpret_cast<const int*>(dataWithOffset));
			case DataType::Float:	return var(*reinterpret_cast<const float*>(dataWithOffset));
			case DataType::Boolean: return var(*reinterpret_cast<const int*>(dataWithOffset) != 0);
			default:				jassertfalse; return var();
			}
		}

		static DataType getTypeFromVar(const var& value, Result* r)
		{
			if (value.isArray())
				return getTypeFromVar(value[0], r);

			if (value.isInt() || value.isInt64())
				return DataType::Integer;

			if (value.isDouble())
				return DataType::Float;

			if (value.isBool())
				return DataType::Boolean;

			if (r != nullptr)
				*r = Result::fail("illegal data type: \"" + value.toString() + "\"");

			return DataType::numTypes;
		}

		static int getElementSizeFromVar(const var& value, Result* r)
		{
			if (value.isArray())
				return value.size();

			if (value.isObject() || value.isString())
			{
				if (r != nullptr)
					*r = Result::fail("illegal type");
			}

			return 1;
		}

		static uint32 getTypeSize(DataType type)
		{
			switch (type)
			{
			case DataType::Integer: return sizeof(int);
			case DataType::Boolean: return sizeof(int);
			case DataType::Float:   return sizeof(float);
			}
		}
	};

	struct Reference
	{
		uint8* data = nullptr;
		DataType type = DataType::numTypes;
		int elementSize = 0;

		Reference& operator=(var newValue)
		{
			if (elementSize == 1)
				Helpers::writeElement(type, data, newValue);

			return *this;
		}

		Reference operator[](const Identifier& id)
		{
			
		}

		Reference operator[](int index) const
		{
			Reference r;

			if (isPositiveAndBelow(index, elementSize))
			{
				r.type = type;
				r.data = data + Helpers::getTypeSize(type) * index;
				r.elementSize = 1;
			}

			return r;
		}

		bool isValid() const
		{
			return data != nullptr;
		}

		explicit operator int() const
		{
			return isValid() ? (int)Helpers::getElement(type, data) : 0;
		}

		explicit operator float() const
		{
			return isValid() ? (float)Helpers::getElement(type, data) : 0.0f;
		}

		explicit operator bool() const
		{
			return isValid() ? (bool)Helpers::getElement(type, data) : false;
		}
	};

protected:

	FixMemoryLayout() :
		initResult(Result::ok())
	{
	};

	struct MemoryLayoutItem: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<MemoryLayoutItem>;
		using List = ReferenceCountedArray<MemoryLayoutItem>;

		MemoryLayoutItem(uint32 offset_, const Identifier& id_, var defaultValue_, Result* r) :
			id(id_),
			type(Helpers::getTypeFromVar(defaultValue_, r)),
			elementSize(Helpers::getElementSizeFromVar(defaultValue_, r)),
			offset(offset_),
			defaultValue(defaultValue_)
		{

		}

		void resetToDefaultValue(uint8* dataStart)
		{
			write(dataStart, defaultValue, nullptr);
		}

		int getByteSize() const { return Helpers::getTypeSize(type) * elementSize; }

		void writeArrayElement(uint8* dataStart, int index, const var& newValue, Result* r)
		{
			if (isPositiveAndBelow(index, elementSize - 1))
			{
				Helpers::writeElement(type, dataStart + Helpers::getTypeSize(type) * index, newValue);
			}
			else
			{
				if (r != nullptr)
					*r = Result::fail("out of bounds");
			}
		}

		var getData(uint8* dataStart, Result* r) const
		{
			if (elementSize == 1)
			{
				return Helpers::getElement(type, dataStart + offset);
			}

			if (r != nullptr)
				*r = Result::fail("Can't get reference to fix array");

			return var();
		}

		void write(uint8* dataStart, const var& newValue, Result* r)
		{
			if (elementSize == 1)
			{
				if (newValue.isArray())
				{
					if (r != nullptr)
						*r = Result::fail("Can't write array to single element");

					return;
				}

				Helpers::writeElement(type, dataStart + offset, newValue);
			}
			else
			{
				if (auto ar = newValue.getArray())
				{
					auto numElementsToRead = ar->size();

					if (elementSize != numElementsToRead)
					{
						if (r != nullptr)
							*r = Result::fail("array size mismatch. Expected " + String(elementSize));

						return;
					}

					auto ts = Helpers::getTypeSize(type);

					for (int i = 0; i < numElementsToRead; i++)
					{
						auto d = dataStart + i * ts;
						Helpers::writeElement(type, d, ar->getUnchecked(i));
					}
				}
				else
				{
					if (r != nullptr)
						*r = Result::fail("This data type requires an array.");
				}
			}
		}

		Identifier id;
		DataType type;
		uint32 offset;
		int elementSize;
		var defaultValue;
	};

	MemoryLayoutItem::List layout;

	static MemoryLayoutItem::List createLayout(var layoutDescription, Result* r = nullptr)
	{
		MemoryLayoutItem::List items;

		if (auto obj = layoutDescription.getDynamicObject())
		{
			uint32 offset = 0;

			for (const auto& prop : obj->getProperties())
			{
				auto id = prop.name;
				auto v = prop.value;

				auto newItem = new MemoryLayoutItem(offset, prop.name, prop.value, r);
				items.add(newItem);
				offset += newItem->getByteSize();
			}
		}

		if (items.isEmpty())
			*r = Result::fail("No data");

		return items;
	}


	Result initResult;
};

struct FixObjectReference : public FixMemoryLayout
{
	FixObjectReference()
	{
		initResult = Result::fail("uninitialised");
	}

	FixObjectReference& operator=(const FixObjectReference& other)
	{
		if (elementSize == 0)
		{
			data = other.data;
			elementSize = other.elementSize;
			layout = other.layout;
			initResult = other.initResult;
		}
		else
		{
			jassert(other.elementSize == elementSize);
			jassert(layout.size() == other.layout.size());
			memcpy(data, other.data, elementSize);
		}

		return *this;
	}

	bool operator==(const FixObjectReference& other) const
	{
		if (data == other.data)
			return true;

		if (layout[0] == other.layout[0])
		{
			bool same = true;

			for (int i = 0; i < elementSize; i++)
				same &= (other.data[i] == data[i]);

			return true;
		}

		return false;
	}

	FixObjectReference(const FixObjectReference& other)
	{
		*this = other;
	}

	Reference operator[](const Identifier& id) const
	{
		Reference r;

		for (auto l : layout)
		{
			if (l->id == id)
			{
				r.data = data + l->offset;
				r.elementSize = l->elementSize;
				r.type = l->type;
				break;
			}
		}

		return r;
	}

	void init(const MemoryLayoutItem::List& layoutToUse, uint8* preallocatedData, bool resetToDefault)
	{
		data = preallocatedData;
		layout = layoutToUse;
		initResult = Result::ok();

		elementSize = 0;

		for (auto l : layoutToUse)
		{
			if(data != nullptr && resetToDefault)
				l->resetToDefaultValue(data + elementSize);

			elementSize += l->getByteSize();
		}
	}

	size_t elementSize = 0;
	uint8* data = nullptr;
};

struct FixLayoutDynamicObject: public FixMemoryLayout,
							   public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<FixLayoutDynamicObject>;

	FixLayoutDynamicObject(var dataDescription):
		FixMemoryLayout()
	{
		layout = createLayout(dataDescription, &initResult);

		if(initResult.failed())
			layout.clear();

		for (auto l : layout)
			numAllocated += l->getByteSize();

		if (numAllocated != 0)
		{
			data.allocate(numAllocated, true);
			reset();
		}
	}

	

	void reset()
	{
		if (numAllocated > 0)
		{
			auto ptr = data.get();

			for (auto l : layout)
			{
				l->resetToDefaultValue(ptr);
				ptr += l->getByteSize();
			}
		}
	}

	Reference operator[](const Identifier& id) const
	{
		Reference r;

		for (auto& l : layout)
		{
			if (id == l->id)
			{
				r.data = data.get() + l->offset;
				r.type = l->type;
				r.elementSize = l->elementSize;
			}
		}

		return r;
	}

	operator FixObjectReference() const
	{
		FixObjectReference r;
		r.init(layout, data.get(), false);
		return r;
	}

	bool isValid() const
	{
		return initResult.wasOk();
	}

private:


	HeapBlock<uint8> data;
	size_t numAllocated = 0;
};



struct FixLayoutUnorderedStack : public FixMemoryLayout,
								 public hise::UnorderedStack<FixObjectReference, 128>
{
	FixLayoutUnorderedStack(const var& description)
	{
		layout = createLayout(description, &initResult);

		if (initResult.wasOk())
		{
			auto ptr = begin();
			
			for (auto l : layout)
				elementSize += l->getByteSize();

			numAllocated = elementSize * 128;

			if (numAllocated > 0)
			{
				allocatedData.allocate(numAllocated, true);
				auto dataPtr = allocatedData.get();

				for (int i = 0; i < 128; i++)
				{
					ptr[i].init(layout, dataPtr, true);
					dataPtr += elementSize;
				}
			}
		}
	}

private:

	int numAllocated = 0;
	int elementSize = 0;
	HeapBlock<uint8> allocatedData;
};

struct FixLayoutDynamicObjectArray : public FixMemoryLayout,
									 public ReferenceCountedObject
{
	FixLayoutDynamicObjectArray(const var& description, int numElements):
		FixMemoryLayout()
	{
		layout = createLayout(description, &initResult);

		if (!initResult.wasOk())
			layout.clear();

		for (auto l : layout)
			elementSize += l->getByteSize();

		numAllocated = elementSize * numElements;

		if (numAllocated > 0)
		{
			data.allocate(numAllocated, true);

			for (int i = 0; i < numElements; i++)
			{
				auto ptr = data.get() + i * elementSize;

				uint32 offset = 0;

				FixObjectReference obj;
				obj.init(layout, ptr, true);
				items.add(obj);
			}
		}
	}

	FixObjectReference operator[](int index) const
	{
		if (isPositiveAndBelow(index, items.size()))
			return items.getReference(0);

		return {};
	}

	size_t elementSize = 0;
	size_t numElements = 0;
	size_t numAllocated = 0;
	Array<FixObjectReference> items;
	HeapBlock<uint8> data;
};

struct FixObjectFactory: public FixMemoryLayout
{
	FixObjectFactory(const var& d)
	{
		description = d;
		layout = createLayout(d, &initResult);
	}

	var createSingleObject()
	{
		if(initResult.wasOk())
			return var(new FixLayoutDynamicObject(description));

		return {};
	}

	var description;
};

//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine)
{
	DynamicObject::Ptr description = new DynamicObject();

	Array<var> data;
	data.add(1);
	data.add(2);
	data.add(3);
	data.add(21);

	description->setProperty("myValue", 12);
	description->setProperty("isOk", false);
	description->setProperty("data", var(data));

	auto dvar = var(description.get());

	FixLayoutDynamicObject fixData(dvar);

	//FixLayoutDynamicObjectArray myFixArray(dvar, 128);

	FixLayoutUnorderedStack myStack(dvar);

	

	myStack.insert(fixData);

	fixData["myValue"] = 18;

	myStack.insert(fixData);
	
	for (const auto& s : myStack)
	{
		int v = (int)s["myValue"];
		int x = 512;
	}
	
	FixObjectFactory factory(dvar);

	var x = factory.createSingleObject();

	standaloneProcessor = new hise::StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	setSize(editor->getWidth(), editor->getHeight());

	handleCommandLineArguments(commandLine);

}

void MainContentComponent::handleCommandLineArguments(const String& args)
{
	if (args.isNotEmpty())
	{
		String presetFilename = args.trimCharactersAtEnd("\"").trimCharactersAtStart("\"");

		if (File::isAbsolutePath(presetFilename))
		{
			File presetFile(presetFilename);

			File projectDirectory = File(presetFile).getParentDirectory().getParentDirectory();
			auto bpe = dynamic_cast<hise::BackendRootWindow*>(editor.get());
			auto mainSynthChain = bpe->getBackendProcessor()->getMainSynthChain();
			const File currentProjectFolder = GET_PROJECT_HANDLER(mainSynthChain).getWorkDirectory();

			if ((currentProjectFolder != projectDirectory) &&
				hise::PresetHandler::showYesNoWindow("Switch Project", "The file you are about to load is in a different project. Do you want to switch projects?", hise::PresetHandler::IconType::Question))
			{
				GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(projectDirectory);
			}

			if (presetFile.getFileExtension() == ".hip")
			{
				mainSynthChain->getMainController()->loadPresetFromFile(presetFile, editor);
			}
			else if (presetFile.getFileExtension() == ".xml")
			{
				hise::BackendCommandTarget::Actions::openFileFromXml(bpe, presetFile);
			}
		}
	}
}

MainContentComponent::~MainContentComponent()
{
	
	root = nullptr;
	editor = nullptr;

	standaloneProcessor = nullptr;
}

void MainContentComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
}

void MainContentComponent::resized()
{

#if SCALE_2
	editor->setSize(getWidth()*2, getHeight()*2);
#else
    editor->setSize(getWidth(), getHeight());
#endif

}

void MainContentComponent::requestQuit()
{
	standaloneProcessor->requestQuit();
}
