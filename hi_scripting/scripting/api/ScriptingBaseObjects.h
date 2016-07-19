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

class ProcessorWithScriptingContent;

class JavascriptMidiProcessor;


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

protected:

	ScriptingObject(ProcessorWithScriptingContent *p);

	ProcessorWithScriptingContent *getScriptProcessor(); 

	const ProcessorWithScriptingContent *getScriptProcessor() const; 

	Processor* getProcessor() { return thisAsProcessor; };
	const Processor* getProcessor() const { return thisAsProcessor; }

	/** \internal Prints a error in the script to the console. */
	void reportScriptError(const String &errorMessage) const;

	/** \internal Prints a specially formatted message if the function is not allowed to be called in this callback. */
	void reportIllegalCall(const String &callName, const String &allowedCallback) const;
	
private:

	ProcessorWithScriptingContent* processor;	
	Processor* thisAsProcessor;
};

class EffectProcessor;


class ConstObjectWithApiCalls : public ScriptingObject,
								public ApiClass
{
public:

	ConstObjectWithApiCalls(ProcessorWithScriptingContent* p, int numConstants):
		ScriptingObject(p),
		ApiClass(numConstants)
	{

	}

	virtual Identifier getObjectName() const = 0;

	Identifier getName() const override { return getObjectName(); }

};

/** The base class for all objects that can be created by a script. 
*	@ingroup scripting
*
*	This class should be used whenever a complex data object should be created and used from within the script.
*	You need to overwrite the objectDeleted() and objectExists() methods.
*	From then on, you can use the checkValidObject() function within methods for sanity checking.
*/
class CreatableScriptObject: public ScriptingObject,
							 public DynamicObject
{
public:

	CreatableScriptObject(ProcessorWithScriptingContent *p):
		ScriptingObject(p)
	{
		setMethod("exists", Wrappers::checkExists);
		
	};

	virtual ~CreatableScriptObject() {};

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
			return false;
		}

		if(objectDeleted())
		{
			reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");	
			return false;
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



class ScriptCreatedComponentWrapper;
class ScriptContentComponent;
class ScriptedControlAudioParameter;
class AudioProcessorWrapper;

/** This class wrapps all available objects that can be created by a script.
*	@ingroup scripting
*/
class ScriptingObjects
{
public:

	class MidiList : public ConstObjectWithApiCalls,
					 public AssignableObject
	{
	public:

		MidiList(ProcessorWithScriptingContent *p) :
			ConstObjectWithApiCalls(p, 0)
		{
			ADD_API_METHOD_1(fill);
			ADD_API_METHOD_0(clear);
			ADD_API_METHOD_1(getValue);
			ADD_API_METHOD_1(getValueAmount);
			ADD_API_METHOD_1(getIndex);
			ADD_API_METHOD_0(isEmpty);
			ADD_API_METHOD_0(getNumSetValues);
			ADD_API_METHOD_2(setValue);
			ADD_API_METHOD_1(restoreFromBase64String);
			ADD_API_METHOD_0(getBase64String);

			clear();
		};

		Identifier getObjectName() const override { return "MidiList"; }

		void assign(const int index, var newValue)
		{
			setValue(index, (int)newValue);
		}

		int getCachedIndex(const var &indexExpression) const override
		{
			return (int)indexExpression;
		}

		var getAssignedValue(int index) const override
		{
			return getValue(index);
		}

		~MidiList() {};

		/** Fills the MidiList with a number specified with valueToFill. */
		void fill(int valueToFill)
		{
			for (int i = 0; i < 128; i++) data[i] = (int16)valueToFill;
			empty = false;
			numValues = 128;
		};

		/** Clears the MidiList to -1. */
		void clear()
		{
			fill(-1);
			empty = true;
			numValues = 0;
		}

		/** Returns the value at the given number. */
		int getValue(int index) const { if (index < 127 && index >= 0) return (int)data[index]; else return -1; }

		/** Returns the number of occurences of 'valueToCheck' */
		int getValueAmount(int valueToCheck)
		{
			if (empty) return 0;

			int amount = 0;

			for (int i = 0; i < 128; i++)
			{
				if (data[i] == (int16)valueToCheck) amount++;
			}

			return amount;
		};



		/** Returns the first index that contains this value. */
		int getIndex(int value) const
		{
			if (empty) return -1;
			for (int i = 0; i < 128; i++)
			{
				if (data[i] == (int16)value)
				{
					return i;
				}
			}

			return -1;
		}

		/** Checks if the list contains any data. */
		bool isEmpty() const { return empty; }

		/** Returns the number of values that are not -1. */
		int getNumSetValues() const { return numValues; }

		/** Sets the number to something between -127 and 128. */
		void setValue(int index, int value)
		{
			if (index >= 0 && index < 128)
			{
				data[index] = (int16)value;

				if (value == -1)
				{
					numValues--;
					if (numValues == 0) empty = true;
				}
				else
				{
					numValues++;
					empty = false;
				}
			}
		};

		/** Encodes all values into a base64 encoded string for storage. */
		String getBase64String() const
		{
			MemoryOutputStream stream;

			Base64::convertToBase64(stream, data, sizeof(int16) * 128);

			return stream.toString();
		}

		/** Restore the values from a String that was created with getBase64String(). */
		void restoreFromBase64String(String base64encodedValues)
		{
			MemoryOutputStream stream(data, sizeof(int16) * 128);

			Base64::convertFromBase64(stream, base64encodedValues);
		}

		struct Wrapper
		{
			API_VOID_METHOD_WRAPPER_1(MidiList, fill);
			API_VOID_METHOD_WRAPPER_0(MidiList, clear);
			API_METHOD_WRAPPER_1(MidiList, getValue);
			API_METHOD_WRAPPER_1(MidiList, getValueAmount);
			API_METHOD_WRAPPER_1(MidiList, getIndex);
			API_METHOD_WRAPPER_0(MidiList, isEmpty);
			API_METHOD_WRAPPER_0(MidiList, getNumSetValues);
			API_VOID_METHOD_WRAPPER_2(MidiList, setValue);
			API_VOID_METHOD_WRAPPER_1(MidiList, restoreFromBase64String);
			API_METHOD_WRAPPER_0(MidiList, getBase64String);
		};

	private:

		int16 data[128];
		bool empty;
		int numValues;
	};

	/** A scripting objects that wraps an existing Modulator.
	*/
	class ScriptingModulator : public CreatableScriptObject,
		public AssignableObject,
		public DebugableObject
	{
	public:


		ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m) :
			CreatableScriptObject(p),
			mod(m)
		{
			if (m != nullptr)
			{
				setName(m->getId());

				for (int i = 0; i < mod->getNumParameters(); i++)
				{
					setProperty(mod->getIdentifierForParameterIndex(i), var(i));
				}
			}
			else
			{
				setName("Invalid Modulator");
			}

			setMethod("setAttribute", Wrapper::setAttribute);
			setMethod("setBypassed", Wrapper::setBypassed);
			setMethod("setIntensity", Wrapper::setIntensity);

		};

		Identifier getObjectName() const override
		{
			return "Modulator";
		}

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Sets the attribute of the Modulator. You can look up the specific parameter indexes in the manual. */
		void setAttribute(int index, float value)
		{
			if (checkValidObject())
			{
				mod->setAttribute(index, value, sendNotification);
			}
		}

		String getDebugName() const override
		{
			if (objectExists() && !objectDeleted())
			{
				return mod->getType().toString();
			}

			return String("Deleted");
		}

		String getDebugValue() const override
		{
			if (objectExists() && !objectDeleted())
			{
				return String(mod->getOutputValue(), 2);
			}
			return "0.0";
		}

		void doubleClickCallback(Component *componentToNotify) override;

		/** Bypasses the Modulator. */
		void setBypassed(bool shouldBeBypassed)
		{
			if (checkValidObject())
			{
				mod->setBypassed(shouldBeBypassed);
				mod->sendChangeMessage();
			}
		};

		/** Changes the Intensity of the Modulator. Ranges: Gain Mode 0 ... 1, PitchMode -12 ... 12. */
		void setIntensity(float newIntensity);

		int getCachedIndex(const var &indexExpression) const override
		{
			if (checkValidObject())
			{
				Identifier id(indexExpression.toString());

				for (int i = 0; i < mod->getNumParameters(); i++)
				{
					if (id == mod->getIdentifierForParameterIndex(i)) return i;
				}
				return -1;
			}
			else
			{
				throw String("Modulator does not exist");
			}

		}

		void assign(const int index, var newValue) override
		{
			setAttribute(index, (float)newValue);
		}

		var getAssignedValue(int /*index*/) const override
		{
			return 1.0; // Todo...
		}

		struct Wrapper
		{
			static var setAttribute(const var::NativeFunctionArgs& args);
			static var setBypassed(const var::NativeFunctionArgs& args);
			static var setIntensity(const var::NativeFunctionArgs& args);
			static var exists(const var::NativeFunctionArgs& args);
		};

		bool objectDeleted() const override
		{
			return mod.get() == nullptr;
		}

		bool objectExists() const override
		{
			return mod != nullptr;
		}

	private:

		WeakReference<Modulator> mod;
	};


	class TimerObject : public Timer,
		public CreatableScriptObject
	{
	public:

		TimerObject(ProcessorWithScriptingContent *p) :
			CreatableScriptObject(p)
		{
			ADD_DYNAMIC_METHOD(startTimer);
			ADD_DYNAMIC_METHOD(stopTimer);
		};

		Identifier getObjectName() const override
		{
			return "Timer";
		}

		bool objectDeleted() const override { return false; }
		bool objectExists() const override { return false; }

		void timerCallback() override;;


		struct Wrapper
		{
			DYNAMIC_METHOD_WRAPPER(TimerObject, startTimer, (int)ARG(0));
			DYNAMIC_METHOD_WRAPPER(TimerObject, stopTimer);
		};
	};

	class ScriptingMidiProcessor : public CreatableScriptObject,
		public AssignableObject
	{
	public:

		ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_) :
			CreatableScriptObject(p),
			mp(mp_)
		{
			if (mp != nullptr)
			{
				setName(mp->getId());

				for (int i = 0; i < mp->getNumParameters(); i++)
				{
					setProperty(mp->getIdentifierForParameterIndex(i), var(i));
				}
			}
			else
			{
				setName("Invalid MidiProcessor");
			}

			setMethod("setAttribute", Wrapper::setAttribute);
			setMethod("setBypassed", Wrapper::setBypassed);
		};

		Identifier getObjectName() const override
		{
			return "MidiProcessor";
		}

		int getCachedIndex(const var &indexExpression) const override
		{
			if (checkValidObject())
			{
				Identifier id(indexExpression.toString());

				for (int i = 0; i < mp->getNumParameters(); i++)
				{
					if (id == mp->getIdentifierForParameterIndex(i)) return i;
				}
			}

			return -1;
		}

		void assign(const int index, var newValue) override
		{
			setAttribute(index, (float)newValue);
		}

		var getAssignedValue(int /*index*/) const override
		{
			return 1.0; // Todo...
		}

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Sets the attribute of the MidiProcessor. If it is a script, then the index of the component is used. */
		void setAttribute(int index, float value)
		{
			if (checkValidObject())
			{
				mp->setAttribute(index, value, sendNotification);
			}
		}

		/** Bypasses the MidiProcessor. */
		void setBypassed(bool shouldBeBypassed)
		{
			if (checkValidObject())
			{
				mp->setBypassed(shouldBeBypassed);
				mp->sendChangeMessage();
			}
		};

		struct Wrapper
		{
			static var setAttribute(const var::NativeFunctionArgs& args);
			static var setBypassed(const var::NativeFunctionArgs& args);
		};

		bool objectDeleted() const override
		{
			return mp.get() == nullptr;
		}

		bool objectExists() const override
		{
			return mp != nullptr;
		}

	private:

		WeakReference<MidiProcessor> mp;
	};

	class ScriptingAudioSampleProcessor : public CreatableScriptObject
	{
	public:

		ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, AudioSampleProcessor *sampleProcessor);

		Identifier getObjectName() const override
		{
			return "AudioSampleProcessor";
		};

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue)
		{
			if (checkValidObject())
			{
				audioSampleProcessor->setAttribute(parameterIndex, newValue, sendNotification);
			}
		};

		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed)
		{
			if (checkValidObject())
			{
				audioSampleProcessor->setBypassed(shouldBeBypassed);
				audioSampleProcessor->sendChangeMessage();
			}
		}

		/** loads the file. You can use the wildcard {PROJECT_FOLDER} to get the audio file folder for the current project. */
		void setFile(String fileName);

		/** Returns the length of the current sample selection in samples. */
		int getSampleLength() const;

		/** Sets the length of the current sample selection in samples. */
		void setSampleRange(int startSample, int endSample);

	protected:

		bool objectDeleted() const override
		{
			return audioSampleProcessor.get() == nullptr;
		}

		bool objectExists() const override
		{
			return audioSampleProcessor != nullptr;
		}

	private:

		struct Wrapper
		{
			static var setAttribute(const var::NativeFunctionArgs& args);
			static var setBypassed(const var::NativeFunctionArgs& args);
			static var getSampleLength(const var::NativeFunctionArgs& args);
			static var setFile(const var::NativeFunctionArgs& args);
			static var setSampleRange(const var::NativeFunctionArgs& args);
		};

		WeakReference<Processor> audioSampleProcessor;
	};

	class ScriptingTableProcessor : public CreatableScriptObject
	{
	public:

		ScriptingTableProcessor(ProcessorWithScriptingContent *p, LookupTableProcessor *tableProcessor);

		Identifier getObjectName() const override
		{
			return "TableProcessor";
		};

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Sets the point with the given index to the values. */
		void setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve);

		/** Adds a new table point (x and y are normalized coordinates). */
		void addTablePoint(int tableIndex, float x, float y);

		/** Resets the table with the given index to a 0..1 line. */
		void reset(int tableIndex);

	protected:

		bool objectDeleted() const override
		{
			return tableProcessor.get() == nullptr;
		}

		bool objectExists() const override
		{
			return tableProcessor != nullptr;
		}

	private:

		struct Wrapper
		{
			static var setTablePoint(const var::NativeFunctionArgs& args);
			static var addTablePoint(const var::NativeFunctionArgs& args);
			static var reset(const var::NativeFunctionArgs& args);
		};

		WeakReference<Processor> tableProcessor;
	};


	class ScriptingEffect : public CreatableScriptObject
	{
	public:
		ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx);

		Identifier getObjectName() const override
		{
			return "Effect";
		}

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue)
		{
			if (checkValidObject())
			{
				effect->setAttribute(parameterIndex, newValue, sendNotification);
			}
		};

		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed)
		{
			if (checkValidObject())
			{
				effect->setBypassed(shouldBeBypassed);
				effect->sendChangeMessage();
			}
		}

		bool objectDeleted() const override
		{
			return effect.get() == nullptr;
		}

		bool objectExists() const override
		{
			return effect != nullptr;
		}

	private:

		struct Wrapper
		{
			static var setAttribute(const var::NativeFunctionArgs& args);
			static var setBypassed(const var::NativeFunctionArgs& args);
		};

		WeakReference<Processor> effect;


	};



	class ScriptingSynth : public CreatableScriptObject
	{
	public:
		ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_);

		Identifier getObjectName() const override
		{
			return "ChildSynth";
		}

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue)
		{
			if (checkValidObject())
			{
				synth->setAttribute(parameterIndex, newValue, sendNotification);
			}
		};

		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed)
		{
			if (checkValidObject())
			{
				synth->setBypassed(shouldBeBypassed);
				synth->sendChangeMessage();
			}
		}



		bool objectDeleted() const override
		{
			return synth.get() == nullptr;
		}

		bool objectExists() const override
		{
			return synth != nullptr;
		}

	private:

		struct Wrapper
		{
			static var setAttribute(const var::NativeFunctionArgs& args);
			static var setBypassed(const var::NativeFunctionArgs& args);
		};

		WeakReference<Processor> synth;


	};


};



#endif  // SCRIPTINGBASEOBJECTS_H_INCLUDED
