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

#ifndef DSPFACTORY_H_INCLUDED
#define DSPFACTORY_H_INCLUDED


class DspInstance;

/** The DspFactory class is the abstract base class for handling additional modules.
*
*   You won't have to deal with this class, but more with one of its derived classes: 
*   - the DynamicDspFactory for handling dynamic libraries (dylibs / DLLs)
*   - the StaticDspLibrary for handling static libraries.
*
*   A factory is an object that can create different modules within Javascript:
*
*   @code{.js}
*   Libraries.load
*
*/
class DspFactory : public DynamicObject
{
public:

	DspFactory();

	/** If you write a static DspLibrary, you need to overwrite this method and return an unique ID. */
	virtual Identifier getId() const = 0;

	virtual var createModule(const String &module) const = 0;

	/** This method is called by the DspInstance object to create a instance of a DSP module. */
	virtual DspBaseObject *createDspBaseObject(const String &module) const = 0;

	/** This method is called by the DspInstance object's destructor to delete the module. */
	virtual void destroyDspBaseObject(DspBaseObject *object) const = 0;

	/** This method must return an array with all module names that can be created. */
	virtual var getModuleList() const = 0;

	virtual var getErrorCode() const { return var(0); }

	virtual void unload() {};

	struct Wrapper;
	class Handler;
	class LibraryLoader;

	typedef ReferenceCountedObjectPtr<DspFactory> Ptr;
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

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("DspModule") };

	/** Calls the setup method of the external module. */
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	/** Calls the processMethod of the external module. */
	void processBlock(const var &data);

	void setParameter(int index, var newValue);
	var getParameter(int index) const;

	void assign(const int index, var newValue) override;
	var getAssignedValue(int index) const override;
	int getCachedIndex(const var &name) const override;

	/** Applies the module on the data. */
	void operator >>(const var &data);

	/** Applies the module on the data. */
	void operator <<(const var &data);

	var getInfo() const;

	struct Wrapper;

private:

	void throwError(const String &errorMessage)
	{
		throw String(errorMessage);
	}

	const String moduleName;

	DspBaseObject *object;
	DspFactory::Ptr factory;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspInstance)
};


/** This class is used to create modules.
*
*	Unline the DynamicDspFactory, it needs to be embedded in the main application.
*
*   If you want to embed your modules as static library into your compiled plugin, subclass this library
*   and register every module that can be created using your
*  
*   Take a look at the HiseCoreDspFactory class to see a living example.
*/
class StaticDspFactory : public DspFactory
{
public:

	StaticDspFactory() {};
	virtual ~StaticDspFactory() {};

	var createModule(const String &name) const override;

	DspBaseObject *createDspBaseObject(const String &moduleName) const override;
	void destroyDspBaseObject(DspBaseObject* handle) const override;

	/** Overwrite this method and register every module you want to create with this factory using registerDspModule<Type>(). */
	virtual void registerModules() = 0;

	var getModuleList() const override;

protected:

	/** Use this helper method to register every module. */
	template <class DspModule>void registerDspModule() { factory.registerType<DspModule>(); }

private:

	Factory<DspBaseObject> factory;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StaticDspFactory)
};


class DynamicDspFactory : public DspFactory
{
public:

	DynamicDspFactory(const String &name_, const String& args);

	DspBaseObject *createDspBaseObject(const String &moduleName) const override;
	void destroyDspBaseObject(DspBaseObject *object) const override;

	int initialise(const String &args);
	var createModule(const String &moduleName) const override;
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER(name); }
	var getModuleList() const override;
	var getErrorCode() const override;

	void unload() override;

private:

	int errorCode;
	const String name;
	ScopedPointer<DynamicLibrary> library;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicDspFactory)
};




#endif  // DSPFACTORY_H_INCLUDED
