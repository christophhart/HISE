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

#ifndef SCRIPTINGBASEOBJECTS_H_INCLUDED
#define SCRIPTINGBASEOBJECTS_H_INCLUDED


class ModulatorSynthGroup;

class ScriptBaseProcessor;

class ScriptProcessor;


/** The base class for all scripting API classes. 
*	@ingroup scripting
*
*	It contains some basic methods for error handling.
*/
class ScriptingObject: public DynamicObject
{
public:

	virtual ~ScriptingObject() {};

	/** \internal sanity check if the function call contains the correct amount of arguments. */
	bool checkArguments(const String &callName, int numArguments, int expectedArgumentAmount);
	
	/** \internal sanity check if all arguments are valid. 
	*
	*	It returns -1 if everything is ok or the index of the faulty argument. 
	*/
	int checkValidArguments(const var::NativeFunctionArgs &args);

	/** \internal Checks if the callback is made on the audio thread (check this with every API call that changes the MidiMessage. */
	bool checkIfSynchronous(const Identifier &callbackName) const;

protected:

	ScriptingObject(ScriptBaseProcessor *p);

	ScriptBaseProcessor *getScriptProcessor(); 

	const ScriptBaseProcessor *getScriptProcessor() const; 

	/** \internal Prints a error in the script to the console. */
	void reportScriptError(const String &errorMessage) const;

	/** \internal Prints a specially formatted message if the function is not allowed to be called in this callback. */
	void reportIllegalCall(const String &callName, const String &allowedCallback) const;
	
private:

	WeakReference<Processor> processor;	
};

class EffectProcessor;

/** The base class for all objects that can be created by a script. 
*	@ingroup scripting
*
*	This class should be used whenever a complex data object should be created and used from within the script.
*	You need to overwrite the objectDeleted() and objectExists() methods.
*	From then on, you can use the checkValidObject() function within methods for sanity checking.
*/
class CreatableScriptObject: public ScriptingObject
{
public:

	CreatableScriptObject(ScriptBaseProcessor *p):
		ScriptingObject(p)
	{
		setMethod("exists", Wrappers::checkExists);
		
	};

	virtual ~CreatableScriptObject() {};

	/** \internal Overwrite this method and return the class name of the object which will be used in the script context. */
	virtual Identifier getObjectName() const = 0;

	/** \internal returns a const reference to the internal data object. */
	const NamedValueSet &getProperties() const { return objectProperties; };

protected:

	

	/** \internal Overwrite this method and check if the object got deleted. Best thing is to use a WeakReference and check if it's nullptr. */
	virtual bool objectDeleted() const = 0;

	/** \internal Overwrite this method and check if the object exists. Best thing is to initialize the pointer to nullptr and check that. */
	virtual bool objectExists() const = 0;

	/** \internal This method combines the calls to objectDeleted() and objectExists() and creates a nice error message. */
	bool checkValidObject() const
	{
		if(!objectExists())
		{
			reportScriptError(getObjectName().toString() + " " + objectProperties["Name"].toString() + " does not exist.");
			return false;
		}

		if(objectDeleted())
		{
			reportScriptError(getObjectName().toString() + " " + objectProperties["Name"].toString() + " was deleted");	
			return false;
		}

		return true;
	}

	NamedValueSet objectProperties;

	struct Wrappers
	{
		static var checkExists(const var::NativeFunctionArgs& args);
	};

};




#endif  // SCRIPTINGBASEOBJECTS_H_INCLUDED
