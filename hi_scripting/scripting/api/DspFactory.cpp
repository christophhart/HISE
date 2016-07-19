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
*   Commercial licences for using HISE in an closed source project are
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



DspInstance::DspInstance(const DspFactory *f, const String &moduleName_) :
ConstScriptingObject(nullptr, NUM_API_FUNCTION_SLOTS),
moduleName(moduleName_),
factory(const_cast<DspFactory*>(f))
{
	if (factory != nullptr)
	{
		object = factory->createDspBaseObject(moduleName);

		if (object != nullptr)
		{
			ADD_API_METHOD_1(processBlock);
			ADD_API_METHOD_2(prepareToPlay);
			ADD_API_METHOD_2(setParameter);
			ADD_API_METHOD_1(getParameter);
			ADD_API_METHOD_0(getInfo);

			for (int i = 0; i < object->getNumConstants(); i++)
			{
				addConstant(object->getIdForConstant(i).toString(), object->getConstant(i));
			}
		}
	};
}


void DspInstance::processBlock(const var &data)
{
	if (object != nullptr)
	{
		if (data.isArray())
		{
			Array<var> *a = data.getArray();
			float *sampleData[4]; // this is an arbitrary amount, but it should be OK...
			int numSamples = -1;

			if (a == nullptr)
				throwError("processBlock must be called on array of buffers");

			for (int i = 0; i < jmin<int>(4, a->size()); i++)
			{
				VariantBuffer *b = a->getUnchecked(i).getBuffer();

				if (b != nullptr)
				{
					if (numSamples != -1 && b->size != numSamples)
						throwError("Buffer size mismatch");

					numSamples = b->size;
				}
				else throwError("processBlock must be called on array of buffers");

				sampleData[i] = b->buffer.getWritePointer(0);
			}

			object->processBlock(sampleData, a->size(), numSamples);
		}
		else if (data.isBuffer())
		{
			VariantBuffer *b = data.getBuffer();

			if (b != nullptr)
			{
				float *sampleData[1] = { b->buffer.getWritePointer(0) };
				const int numSamples = b->size;

				object->processBlock(sampleData, 1, numSamples);
			}
		}
		else throwError("Data Buffer is not valid");
	}
}

void DspInstance::setParameter(int index, var newValue)
{
	if (object != nullptr)
	{
		object->setParameter(index, newValue);
	}
}

var DspInstance::getParameter(int index) const
{
	if (object != nullptr)
	{
		return object->getParameter(index);
	}

	return var::undefined();
}

void DspInstance::assign(const int index, var newValue)
{
	setParameter(index, newValue);
}

var DspInstance::getAssignedValue(int index) const
{
	return getParameter(index);
}

int DspInstance::getCachedIndex(const var &name) const
{
	if (object != nullptr)
	{
		for (int i = 0; i < object->getNumParameters(); i++)
		{
			if (name.toString() == object->getIdForParameter(i).toString())
			{
				return i;
			}
		}
	}

	return -1;
}

void DspInstance::operator<<(var &/*data*/) const
{
	// DSP_TODO
}

var DspInstance::getInfo() const
{
	if (object != nullptr)
	{
		String info;

		info << "Name: " + moduleName << "\n";

		info << "Parameters: " << String(object->getNumParameters()) << "\n";

		for (int i = 0; i < object->getNumParameters(); i++)
		{
			const String line = String("Parameter #1: ") + object->getIdForParameter(i).toString() + String(", current value: ") + String(object->getParameter(i)) + String("\n");

			info << line;
		}

		info << "\n";

		info << "Constants: " << String(object->getNumConstants()) << "\n";

		for (int i = 0; i < object->getNumConstants(); i++)
		{
			info << "Constant #1: " << object->getIdForConstant(i) << +" = " << object->getConstant(i).toString() << "\n";
		}

		return var(info);
	}

	return var("No module loaded");
}

void DspInstance::operator>>(var &data)
{
	processBlock(data);
}

DspInstance::~DspInstance()
{
	if (factory != nullptr)
	{
		factory->destroyDspBaseObject(object);
		factory = nullptr;
	}
}

void DspInstance::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (object != nullptr)
	{
		object->prepareToPlay(sampleRate, samplesPerBlock);
	}
}

typedef DspBaseObject*(*createDspModule_)(const char *);

DynamicDspFactory::DynamicDspFactory(const String &name_) :
name(name_)
{
#if JUCE_WINDOWS

	const File path = File("D:\\Development\\HISE modules\\extras\\script_module_template\\Builds\\VisualStudio2013\\Debug");

	const String libraryName = name + ".dll";

	const String fullLibraryPath = path.getChildFile(libraryName).getFullPathName();


#else

	const String fullLibraryPath = name + ".dylib";

#endif

	library = new DynamicLibrary();
	library->open(fullLibraryPath);

	initialise();

	ADD_DYNAMIC_METHOD(createModule);
}

DspBaseObject * DynamicDspFactory::createDspBaseObject(const String &moduleName) const
{
	if (library != nullptr)
	{
		createDspModule_ c = (createDspModule_)library->getFunction("createDspObject");

		if (c != nullptr)
		{
			return c(moduleName.getCharPointer());
		}
	}

	return nullptr;
}

typedef void(*destroyDspModule_)(DspBaseObject*);

void DynamicDspFactory::destroyDspBaseObject(DspBaseObject *object) const
{
	if (library != nullptr)
	{
		destroyDspModule_ d = (destroyDspModule_)library->getFunction("destroyDspObject");

		if (d != nullptr && object != nullptr)
		{
			d(object);
		}
	}
}

typedef void(*init_)();

void DynamicDspFactory::initialise()
{
	if (library != nullptr)
	{
		init_ d = (init_)library->getFunction("initialise");

		if (d != nullptr)
		{
			d();
		}
		else
		{
			throw String("initialise() not implemented in Dynamic Library " + name);
		}
	}

}

var DynamicDspFactory::createModule(const String &moduleName) const
{
	DspInstance *i = new DspInstance(this, moduleName);

	return var(i);
}

typedef const Array<Identifier> &(*getModuleList_)();

var DynamicDspFactory::getModuleList() const
{
	if (library != nullptr)
	{
		getModuleList_ d = (getModuleList_)library->getFunction("getModuleList");

		if (d != nullptr)
		{
			const Array<Identifier> *ids = &d();

			Array<var> a;

			for (int i = 0; i < ids->size(); i++)
			{
				a.add(ids->getUnchecked(i).toString());
			}

			return var(a);
		}
		else
		{
			throw String("getModuleList not implemented in Dynamic Library " + name);
		}
	}

	return var::undefined();
}

DspBaseObject * StaticDspFactory::createDspBaseObject(const String &moduleName) const
{
	return factory.createFromId(moduleName);
}

void StaticDspFactory::destroyDspBaseObject(DspBaseObject* handle) const
{
	delete handle;
}

var StaticDspFactory::getModuleList() const
{
	Array<var> moduleList;

	for (int i = 0; i < factory.getIdList().size(); i++)
	{
		moduleList.add(factory.getIdList().getUnchecked(i).toString());
	}

	return var(moduleList);
}

var StaticDspFactory::createModule(const String &name) const
{
	DspInstance *instance = new DspInstance(this, name);

	return var(instance);
}


DspFactory::Handler::Handler()
{
	registerStaticFactories(this);
}

DspFactory::Handler::~Handler()
{
	loadedPlugins.clear();
}

DspInstance * DspFactory::Handler::createDspInstance(const String &factoryName, const String &moduleName)
{
	return new DspInstance(getFactory(factoryName), moduleName);
}


template <class T>
void DspFactory::Handler::registerStaticFactory(Handler *instance)
{
	StaticDspFactory* staticFactory = new T();

	staticFactory->registerModules();

	instance->staticFactories.add(staticFactory);
}

DspFactory * DspFactory::Handler::getFactory(const String &name)
{
	Identifier id(name);

	for (int i = 0; i < staticFactories.size(); i++)
	{
		if (staticFactories[i]->getId() == id)
		{
			return staticFactories[i];
		}
	}

	for (int i = 0; i < loadedPlugins.size(); i++)
	{
		if (loadedPlugins[i]->getId() == id)
		{
			return loadedPlugins[i];
		}
	}

	DynamicDspFactory *newLib = new DynamicDspFactory(name);

	loadedPlugins.add(newLib);

	return newLib;
}


void DspFactory::Handler::registerStaticFactories(Handler *instance)
{
	registerStaticFactory<HiseCoreDspFactory>(instance);
}