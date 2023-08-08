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

#ifndef DSPINSTANCE_H_INCLUDED
#define DSPINSTANCE_H_INCLUDED

#include <atomic>

namespace hise { using namespace juce;

#define REGISTER_STATIC_DSP_LIBRARIES() void hise::DspFactory::Handler::registerStaticFactories(hise::DspFactory::Handler *instance)
#define REGISTER_STATIC_DSP_FACTORY(factoryName) hise::DspFactory::Handler::registerStaticFactory<factoryName>(instance);

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

	void getAllStaticLibraries(StringArray &libraries);;

	void getAllDynamicLibraries(StringArray &libraries);

	void setMainController(MainController* mc_);

private:

	static void registerStaticFactories(Handler *instance);
	
	ReferenceCountedArray<DspFactory> staticFactories;
	ReferenceCountedArray<DspFactory> loadedPlugins;

	MainController* mc = nullptr;

};

class DspFactory::LibraryLoader : public DynamicObject
{
public:

	LibraryLoader(Processor* p_);

	StringArray getListOfAllAvailableModules();

	var load(const String &name, const String &password);

	var list();

private:

	struct Wrapper
	{
		DYNAMIC_METHOD_WRAPPER_WITH_RETURN(LibraryLoader, load, ARG(0).toString(), ARG(1).toString());
		DYNAMIC_METHOD_WRAPPER_WITH_RETURN(LibraryLoader, list);
	};

	SharedResourcePointer<DspFactory::Handler> handler;

	Processor* p = nullptr;
	MainController* mc = nullptr;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryLoader)
};

/** This objects is a wrapper around the actual DSP module that is loaded from a plugin.
*
*	It contains the glue code for accessing it per Javascript and is reference counted to manage the lifetime of the external module.
*/
class DspInstance : public ConstScriptingObject,
					public AssignableObject
{
public:

	struct Listener
	{
		virtual ~Listener() {};

		virtual void parameterChanged(int parameterIndex) = 0;
		virtual void preBlockProcessed(const float** data, int numChannels, int numSamples) = 0;
		virtual void blockWasProcessed(const float** data, int numChannels, int numSamples) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	/** Creates a new instance from the given Factory with the supplied name. */
	DspInstance(const DspFactory *f, const String &moduleName_);
	~DspInstance();

		void initialise();
	void unload();

	String getDebugName() const override { return "DSP object"; }
	String getDebugValue() const override { return moduleName; }

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

	void setProcessor(Processor* p);

	void setId(const String& newName);

	void checkPriorityInversion();

	void addListener(Listener* l);

	void removeListener(Listener* l);

private:

	Array<WeakReference<Listener>> listeners;

	WeakReference<Processor> processor;

	DebugLogger* logger = nullptr;

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

	Identifier debugId;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspInstance)
};



} // namespace hise
#endif  // DSPINSTANCE_H_INCLUDED
