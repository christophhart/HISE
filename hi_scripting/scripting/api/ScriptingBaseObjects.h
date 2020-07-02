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

#ifndef SCRIPTINGBASEOBJECTS_H_INCLUDED
#define SCRIPTINGBASEOBJECTS_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSynthGroup;
class ProcessorWithScriptingContent;
class JavascriptMidiProcessor;

class ScriptParameterHandler
{
public:

	virtual ~ScriptParameterHandler() {}

	virtual Identifier getParameterId(int index) const = 0;
	virtual int getNumParameters() const = 0;
	virtual void setParameter(int index, float newValue) = 0;
	virtual float getParameter(int index) const = 0;
};

/** The base class for all scripting API classes. 
*	@ingroup scripting
*
*	It contains some basic methods for error handling.
*/
class ScriptingObject
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


	ProcessorWithScriptingContent *getScriptProcessor();
	const ProcessorWithScriptingContent *getScriptProcessor() const;

	Processor* getProcessor() { return thisAsProcessor; };
	const Processor* getProcessor() const { return thisAsProcessor; }

protected:

	ScriptingObject(ProcessorWithScriptingContent *p);


	/** \internal Prints a error in the script to the console. */
	void reportScriptError(const String &errorMessage) const;

	/** \internal Prints a specially formatted message if the function is not allowed to be called in this callback. */
	void reportIllegalCall(const String &callName, const String &allowedCallback) const;
	
    void logErrorAndContinue(const String& errorMessage) const;
    
private:

	ProcessorWithScriptingContent* processor;	
	Processor* thisAsProcessor;
};

class EffectProcessor;


class ConstScriptingObject : public ScriptingObject,
							 public ApiClass
{
public:

	ConstScriptingObject(ProcessorWithScriptingContent* p, int numConstants):
		ScriptingObject(p),
		ApiClass(numConstants)
	{

	}

	virtual Identifier getObjectName() const = 0;
	Identifier getInstanceName() const override { return name.isValid() ? name : getObjectName(); }

	/** \internal Overwrite this method and check if the object got deleted. Best thing is to use a WeakReference and check if it's nullptr. */
	virtual bool objectDeleted() const { return false; }

	/** \internal Overwrite this method and check if the object exists. Best thing is to initialize the pointer to nullptr and check that. */
	virtual bool objectExists() const { return false; }

	/** \internal This method combines the calls to objectDeleted() and objectExists() and creates a nice error message. */
	bool checkValidObject() const
	{
		if (!objectExists())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " does not exist.");
			RETURN_IF_NO_THROW(false)
		}

		if (objectDeleted())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");
			RETURN_IF_NO_THROW(false)
		}

		return true;
	}

	void setName(const Identifier &name_) noexcept{ name = name_; };

private:

	Identifier name;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ConstScriptingObject);
};

/** The base class for all objects that can be created by a script. 
*	@ingroup scripting
*
*	This class should be used whenever a complex data object should be created and used from within the script.
*	You need to overwrite the objectDeleted() and objectExists() methods.
*	From then on, you can use the checkValidObject() function within methods for sanity checking.
*/
class DynamicScriptingObject: public ScriptingObject,
							 public DynamicObject
{
public:

	DynamicScriptingObject(ProcessorWithScriptingContent *p):
		ScriptingObject(p)
	{
		setMethod("exists", Wrappers::checkExists);
		
	};

	virtual ~DynamicScriptingObject() {};

	/** \internal Overwrite this method and return the class name of the object which will be used in the script context. */
	virtual Identifier getObjectName() const = 0;

	String getInstanceName() const { return name; }

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
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " does not exist.");
			RETURN_IF_NO_THROW(false)
		}

		if(objectDeleted())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");	
			RETURN_IF_NO_THROW(false)
		}

		return true;
	}

	void setName(const String &name_) noexcept { name = name_; };

private:

	String name;

	struct Wrappers
	{
		static var checkExists(const var::NativeFunctionArgs& args);
	};

};



/** @internal A interface class for objects that can be used with the [] operator in Javascript.
	
*
*	It uses a cached look up index on compilation to accelerate the look up.
*/
class AssignableObject
{
public:

	virtual ~AssignableObject() {};

	/** Assign the value to the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	virtual void assign(const int index, var newValue) = 0;

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	virtual var getAssignedValue(int index) const = 0;

	/** Overwrite this and return an index that can be used to look up the value when the script is executed. */
	virtual int getCachedIndex(const var &indexExpression) const = 0;
};



struct ValueTreeConverters
{
	static String convertDynamicObjectToBase64(const var& object, const Identifier& id, bool compress);;

	static ValueTree convertDynamicObjectToValueTree(const var& object, const Identifier& id);

	static String convertValueTreeToBase64(const ValueTree& v, bool compress);

	static var convertBase64ToDynamicObject(const String& base64String, bool isCompressed);

	static ValueTree convertBase64ToValueTree(const String& base64String, bool isCompressed);

	static var convertValueTreeToDynamicObject(const ValueTree& v);

	static var convertFlatValueTreeToVarArray(const ValueTree& v);

	static ValueTree convertVarArrayToFlatValueTree(const var& ar, const Identifier& rootId, const Identifier& childId);

	static void copyDynamicObjectPropertiesToValueTree(ValueTree& v, const var& obj, bool skipArray=false);

	static void copyValueTreePropertiesToDynamicObject(const ValueTree& v, var& obj);

	static var convertContentPropertiesToDynamicObject(const ValueTree& v);

	static ValueTree convertDynamicObjectToContentProperties(const var& d);

	static var convertScriptNodeToDynamicObject(ValueTree v);

	static ValueTree convertDynamicObjectToScriptNodeTree(var obj);

private:

	static void v2d_internal(var& object, const ValueTree& v);

	static void d2v_internal(ValueTree& v, const Identifier& id, const var& object);;


};



} // namespace hise
#endif  // SCRIPTINGBASEOBJECTS_H_INCLUDED
