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

#ifndef DSPINSTANCE_H_INCLUDED
#define DSPINSTANCE_H_INCLUDED

#include <atomic>

#define REGISTER_STATIC_DSP_LIBRARIES() void DspFactory::Handler::registerStaticFactories(DspFactory::Handler *instance)
#define REGISTER_STATIC_DSP_FACTORY(factoryName) DspFactory::Handler::registerStaticFactory<factoryName>(instance);

class DynamicDspFactory : public DspFactory
{
public:

	DynamicDspFactory(const String &name_, const String& args);

	void openDynamicLibrary();

	DspBaseObject *createDspBaseObject(const String &moduleName) const override;
	void destroyDspBaseObject(DspBaseObject *object) const override;

	int initialise(const String &args);
	var createModule(const String &moduleName) const override;

	void unloadToRecompile();

	void reloadAfterRecompile();

	Identifier getId() const override { static const Identifier id(name); return id; }
	var getModuleList() const override;
	var getErrorCode() const override;

	void unload() override;

	
	struct Wrapper;

private:

	
	bool isUnloadedForCompilation = false;

	int errorCode;
	const String name;
	const String args;
	ScopedPointer<DynamicLibrary> library;

	String projectPath;
	String buildScheme;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicDspFactory)
};



typedef DspBaseObject*(*createDspModule_)(const char *);

class TccDspFactory;

class DspFactory::Handler
{
public:

	Handler();
	~Handler();

	DspInstance *createDspInstance(const String &factoryName, const String& factoryPassword, const String &moduleName);

	template <class T> static void registerStaticFactory(Handler *instance)
	{
		StaticDspFactory* staticFactory = new T();
		staticFactory->registerModules();

		instance->staticFactories.add(staticFactory);
	}

	/** Returns a factory with the given name.
	*
	*	It looks for static factories first. If no static library is found, it searches for opened dynamic factories.
	*	If no dynamic factory is found, it will open the dynamic library at the standard path and returns this instance.
	*/
	DspFactory *getFactory(const String &name, const String& password);

	void getAllStaticLibraries(StringArray &libraries)
	{
		for (int i = 0; i < staticFactories.size(); i++)
		{
			libraries.add(staticFactories[i]->getId().toString());
		}
	};

	void getAllDynamicLibraries(StringArray &libraries)
	{
		for (int i = 0; i < loadedPlugins.size(); i++)
		{
			libraries.add(loadedPlugins[i]->getId().toString());
		}
	}

	void setMainController(MainController* mc_);

private:

	static void registerStaticFactories(Handler *instance);
	
	ReferenceCountedArray<DspFactory> staticFactories;
	ReferenceCountedArray<DspFactory> loadedPlugins;

	MainController* mc = nullptr;

#if JUCE_IOS
#else
	ReferenceCountedObjectPtr<TccDspFactory> tccFactory;
#endif
};

/** This objects is a wrapper around the actual DSP module that is loaded from a plugin.
*
*	It contains the glue code for accessing it per Javascript and is reference counted to manage the lifetime of the external module.
*/
class DspInstance : public ConstScriptingObject,
					public AssignableObject
{
public:

	/** Creates a new instance from the given Factory with the supplied name. */
	DspInstance(const DspFactory *f, const String &moduleName_);
	~DspInstance();

	void initialise();
	void unload();

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("DspModule") };

	/** Calls the setup method of the external module. */
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	/** Calls the processMethod of the external module. */
	void processBlock(const var &data);

	/** Sets the float parameter with the given index. */
	void setParameter(int index, float newValue);

	/** Returns the parameter with the given index. */
	var getParameter(int index) const;

    /** Returns the number of parameters. */
    var getNumParameters() const;
    
    /** Returns the number of constants. */
    var getNumConstants() const;
    
    /** Returns the name of the constant. */
    var getConstantId(int index) const;
    
    /** Returns the constant at the given index. */
    var getConstant(int index) const;
    
	/** Sets a String value. */
	void setStringParameter(int index, String value);

	/** Gets the string value. */
	String getStringParameter(int index);

	/** Enables / Disables the processing. */
	void setBypassed(bool shouldBeBypassed);

	/** Checks if the processing is enabled. */
	bool isBypassed() const;

	void assign(const int index, var newValue) override;
	var getAssignedValue(int index) const override;
	int getCachedIndex(const var &name) const override;

	/** Applies the module on the data. */
	void operator >>(const var &data);

	/** Applies the module on the data. */
	void operator <<(const var &data);

	/** Returns an informative String. */
	var getInfo() const;

	struct Wrapper;

private:

    const SpinLock& getLock() const { return lock; };
    
    SpinLock lock;
    
	void throwError(const String &errorMessage)
	{
		throw String(errorMessage);
	}

	const String moduleName;

	DspBaseObject *object;
	DspFactory::Ptr factory;

	AudioSampleBuffer bypassSwitchBuffer;

	std::atomic<bool> bypassed;

	bool switchBypassFlag = false;

	bool prepareToPlayWasCalled = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspInstance)
};




#endif  // DSPINSTANCE_H_INCLUDED
