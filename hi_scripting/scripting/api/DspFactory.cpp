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

namespace hise { using namespace juce;


	DspFactory::LibraryLoader::LibraryLoader(Processor* p_):
		p(p_)
	{
		if (p != nullptr)
		{
			mc = p->getMainController();
			handler->setMainController(mc);
			ADD_DYNAMIC_METHOD(load);
			ADD_DYNAMIC_METHOD(list);
		}
	}

	var DspFactory::LibraryLoader::load(const String& name, const String& password)
	{
		DspFactory *f = dynamic_cast<DspFactory*>(handler->getFactory(name, password));

		f->currentProcessor = p;

		return var(f);
	}

	var DspFactory::LibraryLoader::list()
	{
		StringArray s1, s2;

		handler->getAllStaticLibraries(s1);
		handler->getAllDynamicLibraries(s2);

		String output = "Available static libraries: \n";
		output << s1.joinIntoString("\n");

		output << "\nAvailable dynamic libraries: " << "\n";
		output << s2.joinIntoString("\n");

		return var(output);
	}

juce::StringArray DspFactory::LibraryLoader::getListOfAllAvailableModules()
{
	StringArray allModules;
	StringArray libs;

	handler->getAllStaticLibraries(libs);

	handler->getAllDynamicLibraries(libs);

	for (auto lib : libs)
	{
		if (auto f = handler->getFactory(lib, ""))
		{
			auto moduleList = f->getModuleList();

			if (moduleList.isArray())
			{
				for (auto moduleId : *moduleList.getArray())
				{
					allModules.add(lib + "." + moduleId.toString());
				}
			}
		}
	}

	return allModules;
}

struct DspFactory::Wrapper
{
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, createModule, ARG(0).toString());
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, getModuleList);
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, getErrorCode);
};

DspFactory::DspFactory() :
DynamicObject()
{
	ADD_DYNAMIC_METHOD(createModule);
	ADD_DYNAMIC_METHOD(getModuleList);
	ADD_DYNAMIC_METHOD(getErrorCode);
	
}

struct DynamicDspFactory::Wrapper
{
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, createModule, ARG(0).toString());
	DYNAMIC_METHOD_WRAPPER(DynamicDspFactory, unloadToRecompile);
	DYNAMIC_METHOD_WRAPPER(DynamicDspFactory, reloadAfterRecompile);
};

DynamicDspFactory::DynamicDspFactory(const String &name_, const String& args_) :
name(name_),
args(args_)
{
	openDynamicLibrary();


	ADD_DYNAMIC_METHOD(createModule);
	ADD_DYNAMIC_METHOD(unloadToRecompile);
	ADD_DYNAMIC_METHOD(reloadAfterRecompile);
	
	setProperty("LoadingSuccessful", 0);
	setProperty("Uninitialised", 1);
	setProperty("MissingLibrary", 2);
	setProperty("NoValidLibrary", 3);
	setProperty("NoVersionMatch", 4);
	setProperty("KeyInvalid", 5);
}

void DynamicDspFactory::openDynamicLibrary()
{
#if JUCE_WINDOWS

	const File path = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("dll/");

#if USE_FRONTEND

	

	if (!path.isDirectory())
	{
		errorCode = (int)LoadingErrorCode::MissingLibrary;
		return;
	}

#endif

#if JUCE_32BIT
	const String libraryName = name + "_x86.dll";
#else
	const String libraryName = name + "_x64.dll";
#endif

#else
    

    const File path = NativeFileHandler::getAppDataDirectory(nullptr).getChildFile("lib/");
    
#if USE_FRONTEND
    
    if (!path.isDirectory())
    {
        errorCode = (int)LoadingErrorCode::MissingLibrary;
        return;
    }
    
#endif
    
	const String libraryName = name + ".dylib";
#endif

	const String fullLibraryPath = path.getChildFile(libraryName).getFullPathName();
	File f(fullLibraryPath);

	if (!f.existsAsFile())
	{
		errorCode = (int)LoadingErrorCode::MissingLibrary;
	}
	else
	{
		library = new DynamicLibrary();
		library->open(fullLibraryPath);

		errorCode = initialise(args);
	}
}

void GlobalScriptCompileBroadcaster::createDummyLoader()
{
	dummyLibraryLoader = new DspFactory::LibraryLoader(nullptr);
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

typedef LoadingErrorCode(*init_)(const char* name);

int DynamicDspFactory::initialise(const String &arguments)
{
	if (library != nullptr)
	{
		init_ d = (init_)library->getFunction("initialise");

		if (d != nullptr)
		{
			isUnloadedForCompilation = false;
			return (int)d(arguments.getCharPointer());
		}
		else
		{
			return (int)LoadingErrorCode::NoValidLibrary;
		}
	}
	else
	{
		return (int)LoadingErrorCode::MissingLibrary;
	}
}

var DynamicDspFactory::createModule(const String &moduleName) const
{
	if (isUnloadedForCompilation)
        throw String("Can't load modules for \"unloaded for recompile\" Libraries");

	ScopedPointer<DspInstance> instance = new DspInstance(this, moduleName);

	instance->setProcessor(currentProcessor);
	instance->setId(moduleName);

	try
	{
		instance->initialise();
	}
	catch (String errorMessage)
	{
		DBG(errorMessage);
                
		return var::undefined();
	}

	return var(instance.release());
}

void DynamicDspFactory::unloadToRecompile()
{
	if (isUnloadedForCompilation == false)
	{
		library->close();

		isUnloadedForCompilation = true;
	}
}

void DynamicDspFactory::reloadAfterRecompile()
{
	if (isUnloadedForCompilation == true)
	{
		jassert(library->getNativeHandle() == nullptr);

		isUnloadedForCompilation = false;

		openDynamicLibrary();
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

	return var::undefined();
}

var DynamicDspFactory::getErrorCode() const
{
	return var(errorCode);
}

void DynamicDspFactory::unload()
{
	library = nullptr;
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
	ScopedPointer<DspInstance> instance = new DspInstance(this, name);

	instance->setProcessor(currentProcessor);
	instance->setId(name);

	try
	{
		instance->initialise();
	}
	catch (String errorMessage)
	{
		DBG(errorMessage);
                
		return var::undefined();
	}

	return var(instance.release());
}


DspFactory::Handler::Handler()
{
	registerStaticFactories(this);
}

DspFactory::Handler::~Handler()
{
	loadedPlugins.clear();
}

DspInstance * DspFactory::Handler::createDspInstance(const String &factoryName, const String& factoryPassword, const String &moduleName)
{
	return new DspInstance(getFactory(factoryName, factoryPassword), moduleName);
}




DspFactory * DspFactory::Handler::getFactory(const String &name, const String& password)
{
	Identifier id(name);

	for (int i = 0; i < staticFactories.size(); i++)
	{
		if (staticFactories[i]->getId() == id)
		{
			return staticFactories[i].get();
		}
	}

	for (int i = 0; i < loadedPlugins.size(); i++)
	{
		if (loadedPlugins[i]->getId() == id)
		{
			return loadedPlugins[i].get();
		}
	}

	try
	{
		ScopedPointer<DynamicDspFactory> newLib = new DynamicDspFactory(name, password);
		loadedPlugins.add(newLib.release());
		return loadedPlugins.getLast().get();
	}
	catch (String& errorMessage)
	{
		throw errorMessage;
		return nullptr;
	}
	
	
}

void DspFactory::Handler::getAllStaticLibraries(StringArray& libraries)
{
	for (int i = 0; i < staticFactories.size(); i++)
	{
		libraries.add(staticFactories[i]->getId().toString());
	}
}

void DspFactory::Handler::getAllDynamicLibraries(StringArray& libraries)
{
	for (int i = 0; i < loadedPlugins.size(); i++)
	{
		libraries.add(loadedPlugins[i]->getId().toString());
	}
}


void DspFactory::Handler::setMainController(MainController* mc_)
{
	mc = mc_;
}

} // namespace hise
