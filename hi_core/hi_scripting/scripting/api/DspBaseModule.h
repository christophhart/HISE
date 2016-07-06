/*
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
*/

#ifndef DSPBASEMODULE_H_INCLUDED
#define DSPBASEMODULE_H_INCLUDED

#include "ScriptMacroDefinitions.h"


class AssignableObject
{
public:

    virtual ~AssignableObject() {};
    
	virtual void assign(const int index, var newValue) = 0;

	virtual var getAssignedValue(int index) const = 0;

	virtual int getIndex(const var &indexExpression) const = 0;
};

#if HISE_DLL

/** A generic template factory class.
*
*	Create a instance with the base class as template and add subclasses via registerType<SubClass>().
*	The subclasses must have a static method
*
*		static Identifier getName() { RETURN_STATIC_IDENTIFIER(name) }
*
*	so that the factory can decide which subtype to create.
*/
template <typename BaseClass>
class Factory
{
public:

	/** Register a subclass to this factory. The subclass must have a static method 'Identifier getName()'. */
	template <typename DerivedClass> void registerType()
	{
		if (std::is_base_of<BaseClass, DerivedClass>::value)
		{
			ids.add(DerivedClass::getName());
			functions.add(&createFunc<DerivedClass>);
		}
	}

	/** Creates a subclass instance with the registered Identifier and returns a base class pointer to this. You need to take care of the ownership of course. */
	BaseClass* createFromId(const Identifier &id) const
	{
		const int index = ids.indexOf(id);

		if (index != -1) return functions[index]();
		else			 return nullptr;
	}

	const Array<Identifier> &getIdList() const { return ids; }

private:

	/** @internal */
	template <typename DerivedClass> static BaseClass* createFunc() { return new DerivedClass(); }

	/** @internal */
	typedef BaseClass* (*PCreateFunc)();

	Array<Identifier> ids;
	Array <PCreateFunc> functions;;
};

#endif

/** This interface class is the base class for all modules that can be loaded as plugin into the script FX processor. 
*
*	A DspBaseObject can have parameters and constants which can be conveniently accessed via Javascript like this:
*
*	    // Parameter:
*		module["Gain"] = 0.2;
*		var x = module["Gain"];
*
*		// Constant
*		var y = module.PI;
*
*	
*/
class DspBaseObject
{
public:

	// ================================================================================================================

	/** Overwrite this method and setup the processing for the given specifications. */
	virtual void prepareToPlay(double sampleRate, int blockSize) = 0;

	/** Overwrite this method and do your processing on the given sample data. */
	virtual void processBlock(float **data, int numChannels, int numSamples) = 0;

	// =================================================================================================================
    virtual ~DspBaseObject() {}
    
protected:

	friend class DspInstance;

	/** Return the number of parameters for this module. This must be a constant. */
	virtual int getNumParameters() const = 0;

	/** Return a Identifier for each parameter Index. 
	*
	*	This must be a constant Identifier and will be used to access properties in Javascript. */
	virtual const Identifier &getIdForParameter(int index) const = 0;

	/** This must return the value for each identifier. 
	*
	*	If the value is not a primitive type, make sure that the lifetime is managed by your module. */
	virtual float getParameter(int index) const = 0;

	/** This must set the parameter to your value. */
	virtual void setParameter(int index, float newValue) = 0;

	// =================================================================================================================

	/** Overwrite this method if your module has constants that*/
	virtual int getNumConstants() const { return 0; };

	/** Overwrite this method and return the value of the constant. */
	virtual var getConstant(int /*index*/) { return var::undefined(); }

	/** Overwrite this method and return the id of the constant. */
	virtual const Identifier& getIdForConstant(int /*index*/) { return Identifier::null; }

	// =================================================================================================================

};


#ifdef HISE_DLL

#if JUCE_WINDOWS
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static Factory<DspBaseObject> baseObjects;

DLL_EXPORT void destroyDspObject(DspBaseObject* handle) { delete handle; }

DLL_EXPORT void initialise();

DLL_EXPORT const void *getModuleList() { return &baseObjects.getIdList(); }

#endif

#endif  // DSPBASEMODULE_H_INCLUDED
