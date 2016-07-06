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
DynamicObject(),
moduleName(moduleName_),
factory(const_cast<DspFactory*>(f))
{
	if (factory != nullptr)
	{
		object = factory->createDspBaseObject(moduleName);

		if (object != nullptr)
		{
			ADD_DYNAMIC_METHOD(processBlock);
			ADD_DYNAMIC_METHOD(prepareToPlay);
			ADD_DYNAMIC_METHOD(setParameter);
			ADD_DYNAMIC_METHOD(getParameter);
			ADD_DYNAMIC_METHOD(getInfo);

			for (int i = 0; i < object->getNumConstants(); i++)
			{
				setProperty(object->getIdForConstant(i), object->getConstant(i));
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
				const int numChannels = 1;
				const int numSamples = b->size;

				object->processBlock(sampleData, 1, numSamples);
			}
		}
		else throwError("Data Buffer is not valid");
	}
}

DspInstance::~DspInstance()
{
	if (factory != nullptr)
	{
		factory->destroyDspBaseObject(object);
		factory = nullptr;
	}
}

typedef DspBaseObject*(*createDspModule_)(const char *);

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

	return var::undefined;
}

DspBaseObject * StaticDspFactory::createDspBaseObject(const String &moduleName) const
{
	return factory.createFromId(moduleName);
}

void StaticDspFactory::destroyDspBaseObject(DspBaseObject* handle) const
{
	delete handle;
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