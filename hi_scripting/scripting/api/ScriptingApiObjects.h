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


#ifndef SCRIPTINGAPIOBJECTS_H_INCLUDED
#define SCRIPTINGAPIOBJECTS_H_INCLUDED

namespace hise { using namespace juce;

class ApiHelpers
{
public:

	class ModuleHandler
	{
	public:

		ModuleHandler(Processor* parent_, JavascriptProcessor* sp);

		~ModuleHandler();
		
		bool removeModule(Processor* p);

		Processor* addModule(Chain* c, const String& type, const String& id, int index = -1);

		Modulator* addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator, const String& modName, bool connectAsStaticMod=false);

		JavascriptProcessor* getScriptProcessor() { return scriptProcessor.get(); };

	private:

		

		WeakReference<Processor> parent;
		WeakReference<JavascriptProcessor> scriptProcessor;

		Component::SafePointer<Component> mainEditor;
	};

	static Point<float> getPointFromVar(const var& data, Result* r = nullptr);

	static Rectangle<float> getRectangleFromVar(const var &data, Result *r = nullptr);

	static Rectangle<int> getIntRectangleFromVar(const var &data, Result* r = nullptr);

	static String getFileNameFromErrorMessage(const String &errorMessage);

	static StringArray getJustificationNames();

	static Justification getJustification(const String& justificationName, Result* r = nullptr);

#if USE_BACKEND

	static AttributedString createAttributedStringFromApi(const ValueTree &method, const String &className, bool multiLine, Colour textColour);
	static String createCodeToInsert(const ValueTree &method, const String &className);
	static void getColourAndCharForType(int type, char &c, Colour &colour);
	static String getValueType(const var &v);

	struct Api
	{
		Api();

		ValueTree apiTree;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api)
	};

#endif
};


class ScriptCreatedComponentWrapper;
class ScriptContentComponent;
class ScriptedControlAudioParameter;
class AudioProcessorWrapper;
class SlotFX;

/** This class wrapps all available objects that can be created by a script.
*	@ingroup scripting
*/
class ScriptingObjects
{
public:

	class MidiList : public ConstScriptingObject,
					 public AssignableObject
	{
	public:

		// ============================================================================================================

		MidiList(ProcessorWithScriptingContent *p);
		~MidiList() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MidiList"); }

		void assign(const int index, var newValue);
		int getCachedIndex(const var &indexExpression) const override;
		var getAssignedValue(int index) const override;

		// ================================================================================================ API METHODS

		/** Fills the MidiList with a number specified with valueToFill. */
		void fill(int valueToFill);;

		/** Clears the MidiList to -1. */
		void clear();

		/** Returns the value at the given number. */
		int getValue(int index) const;

		/** Returns the number of occurences of 'valueToCheck' */
		int getValueAmount(int valueToCheck);;

		/** Returns the first index that contains this value. */
		int getIndex(int value) const;

		/** Checks if the list contains any data. */
		bool isEmpty() const { return empty; }

		/** Returns the number of values that are not -1. */
		int getNumSetValues() const { return numValues; }

		/** Sets the number to something between -127 and 128. */
		void setValue(int index, int value);;

		/** Encodes all values into a base64 encoded string for storage. */
		String getBase64String() const;

		/** Restore the values from a String that was created with getBase64String(). */
		void restoreFromBase64String(String base64encodedValues);

		// ============================================================================================================

		struct Wrapper;

	private:

		int data[128];
		bool empty;
		int numValues;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiList);

		// ============================================================================================================
	};

	class ScriptSliderPackData : public ConstScriptingObject,
								 public DebugableObject
	{
	public:

		ScriptSliderPackData(ProcessorWithScriptingContent* pwsc);

		~ScriptSliderPackData() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("SliderPackData"); }

		String getDebugName() const override { return "SliderPackData"; };
		String getDebugValue() const override { return String(getNumSliders()); };

		void rightClickCallback(const MouseEvent& e, Component *c) override;

		SliderPackData* getSliderPackData() { return &data; }

		// ============================================================================================================

		/** Returns the step size. */
		var getStepSize() const;

		/** Sets the amount of sliders. */
		void setNumSliders(var numSliders);

		/** Returns the amount of sliders. */
		int getNumSliders() const;

		/** Sets the value at the given position. */
		void setValue(int sliderIndex, float value);

		/** Returns the value at the given position. */
		float getValue(int index) const;

		/** Sets the range. */
		void setRange(double minValue, double maxValue, double stepSize);

		// ============================================================================================================

	private:

		struct Wrapper;

		SliderPackData data;

	};

	class ScriptingMessageHolder : public ConstScriptingObject,
								   public DebugableObject
	{
	public:

		// ============================================================================================================

		ScriptingMessageHolder(ProcessorWithScriptingContent* content);
		~ScriptingMessageHolder() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Message"); }

		String getDebugName() const override { return "MessageHolder"; };
		String getDebugValue() const override { return e.getTypeAsString() + "[" + String(e.getNoteNumber()) + "," + String(e.getVelocity()) + "," + String(e.getChannel()) + "]"; };
		
		// ============================================================================================================

		/** Return the note number. This can be called only on midi event callbacks. */
		int getNoteNumber() const;

		/** returns the controller number or 'undefined', if the message is neither controller nor pitch wheel nor aftertouch.
		*
		*	You can also check for pitch wheel values and aftertouch messages.
		*	Pitchwheel has number 128, Aftertouch has number 129.
		*/
		var getControllerNumber() const;

		/** Returns the value of the controller. */
		var getControllerValue() const;

		/** Returns the MIDI Channel from 1 to 16. */
		int getChannel() const;

		/** Changes the MIDI channel from 1 to 16. */
		void setChannel(int newChannel);

		/** Changes the note number. */
		void setNoteNumber(int newNoteNumber);

		/** Changes the velocity (range 1 - 127). */
		void setVelocity(int newVelocity);

		/** Changes the ControllerNumber. */
		void setControllerNumber(int newControllerNumber);

		/** Changes the controller value (range 0 - 127). */
		void setControllerValue(int newControllerValue);

		/** Returns the Velocity. */
		int getVelocity() const;

		/** Ignores the event. */
		void ignoreEvent(bool shouldBeIgnored = true);;

		/** Returns the event id of the current message. */
		int getEventId() const;

		/** Transposes the note on. */
		void setTransposeAmount(int tranposeValue);

		/** Gets the tranpose value. */
		int getTransposeAmount() const;

		/** Sets the coarse detune amount in semitones. */
		void setCoarseDetune(int semiToneDetune);

		/** Returns the coarse detune amount in semitones. */
		int getCoarseDetune() const;

		/** Sets the fine detune amount in cents. */
		void setFineDetune(int cents);

		/** Returns the fine detune amount int cents. */
		int getFineDetune() const;

		/** Sets the volume of the note (-100 = silence). */
		void setGain(int gainInDecibels);

		/** Returns the volume of the note. */
		int getGain() const;

		/** Returns the current timestamp. */
		int getTimestamp() const;

		/** Sets the timestamp in samples. */
		void setTimestamp(int timestampSamples);

		/** Adds the given sample amount to the current timestamp. */
		void addToTimestamp(int deltaSamples);

		/** Creates a info string for debugging. */
		String dump() const;

		// ============================================================================================================

		void setMessage(const HiseEvent &newEvent) { e = HiseEvent(newEvent); }

		HiseEvent getMessageCopy() const { return e; }

	private:

		struct Wrapper;

		HiseEvent e;
	};

	
	/** A scripting objects that wraps an existing Modulator.
	*/
	class ScriptingModulator : public ConstScriptingObject,
							   public AssignableObject,
							   public DebugableObject
	{
	public:

		ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_);;
		~ScriptingModulator() {};

		// ============================================================================================================

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Modulator"); }
		bool objectDeleted() const override { return mod.get() == nullptr; }
		bool objectExists() const override { return mod != nullptr;	}

		String getDebugName() const override;
		String getDebugValue() const override;
		String getDebugDataType() const override { return getObjectName().toString(); }
		void doubleClickCallback(const MouseEvent &e, Component* componentToNotify) override;

		void rightClickCallback(const MouseEvent& e, Component* t) override;

		// ============================================================================================================

		int getCachedIndex(const var &indexExpression) const override;
		void assign(const int index, var newValue) override;
		var getAssignedValue(int /*index*/) const override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the modulator. */
		String getId() const;

		/** Sets the attribute of the Modulator. You can look up the specific parameter indexes in the manual. */
		void setAttribute(int index, float value);

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the Modulator. */
		void setBypassed(bool shouldBeBypassed);;

		/** Checks if the modulator is bypassed. */
		bool isBypassed() const;

		/** Changes the Intensity of the Modulator. Ranges: Gain Mode 0 ... 1, PitchMode -12 ... 12. */
		void setIntensity(float newIntensity);

		/** Returns the intensity of the Modulator. Ranges: Gain: 0...1, Pitch: -12...12. */
		float getIntensity() const;

		/** Returns the current peak value of the modulator. */
		float getCurrentLevel();

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Export the control values (without the script). */
		String exportScriptControls();

		/** Restores the control values for scripts (without recompiling). */
		void restoreScriptControls(String base64Controls);

		/** Adds a modulator to the given chain and returns a reference. */
		var addModulator(var chainIndex, var typeName, var modName);

		/** Adds a and connects a receiver modulator for the given global modulator. */
		var addGlobalModulator(var chainIndex, var globalMod, String modName);

		/** Adds and connects a receiving static time variant modulator for the given global modulator. */
		var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);

		/** Returns a reference as table processor to modify the table or undefined if no table modulator. */
		var asTableProcessor();

        /** Returns the Modulator chain with the given index. */
        var getModulatorChain(var chainIndex);
        
		

		// ============================================================================================================

		struct Wrapper;
		
		Modulator* getModulator() { return mod.get(); }

	private:

		ApiHelpers::ModuleHandler moduleHandler;

		WeakReference<Modulator> mod;
		Modulation *m;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingModulator);

		// ============================================================================================================
	};



	class ScriptingEffect : public ConstScriptingObject,
							public DebugableObject
	{
	public:


		class FilterModeObject : public ConstScriptingObject
		{
		public:

			FilterModeObject(const ProcessorWithScriptingContent* p);
			Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FilterModes"); }

		private:

		};


		// ============================================================================================================

		ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx);
		~ScriptingEffect() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Effect"); }
		bool objectDeleted() const override { return effect.get() == nullptr; }
		bool objectExists() const override { return effect != nullptr; }

		// ============================================================================================================ 

		String getDebugName() const override { return effect.get() != nullptr ? effect->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component* ) override {};

		void rightClickCallback(const MouseEvent& e, Component* t) override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the effect. */
		String getId() const;

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue);

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed);

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Export the control values (without the script). */
		String exportScriptControls();

		/** Restores the control values for scripts (without recompiling). */
		void restoreScriptControls(String base64Controls);

		/** Returns the current peak level for the given channel. */
		float getCurrentLevel(bool leftChannel);

		/** Adds a modulator to the given chain and returns a reference. */
		var addModulator(var chainIndex, var typeName, var modName);

		/** Returns the Modulator chain with the given index. */
		var getModulatorChain(var chainIndex);

		/** Adds a and connects a receiver modulator for the given global modulator. */
		var addGlobalModulator(var chainIndex, var globalMod, String modName);

		/** Adds and connects a receiving static time variant modulator for the given global modulator. */
		var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);

		// ============================================================================================================

		struct Wrapper;

		EffectProcessor* getEffect() { return dynamic_cast<EffectProcessor*>(effect.get()); }

	private:

		ApiHelpers::ModuleHandler moduleHandler;

		WeakReference<Processor> effect;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingEffect);

		// ============================================================================================================
	};


	class ScriptingSlotFX : public ConstScriptingObject,
							public DebugableObject
	{
	public:

		// ============================================================================================================

		ScriptingSlotFX(ProcessorWithScriptingContent *p, EffectProcessor *fx);
		~ScriptingSlotFX() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("SlotFX"); }
		bool objectDeleted() const override { return slotFX.get() == nullptr; }
		bool objectExists() const override { return slotFX != nullptr; }

		// ============================================================================================================ 

		String getDebugName() const override { return slotFX.get() != nullptr ? slotFX->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component*) override {};

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		
		/** Bypasses the effect. This uses the soft bypass feature of the SlotFX module. */
		void setBypassed(bool shouldBeBypassed);

		/** Clears the slot (loads a unity gain module). */
		void clear();

		/** Loads the effect with the given name and returns a reference to it. */
		ScriptingEffect* setEffect(String effectName);

		/** Swaps the effect with the other slot. */
		void swap(var otherSlot);

		// ============================================================================================================

		struct Wrapper;

		SlotFX* getSlotFX();

	private:

		WeakReference<Processor> slotFX;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingSlotFX);

		// ============================================================================================================
	};


	class ScriptRoutingMatrix : public ConstScriptingObject,
		public DebugableObject
	{
	public:

		ScriptRoutingMatrix(ProcessorWithScriptingContent *p, Processor *processor);
		~ScriptRoutingMatrix() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("RoutingMatrix"); }
		bool objectDeleted() const override { return rp.get() == nullptr; }
		bool objectExists() const override { return rp != nullptr; }

		// ============================================================================================================ 

		String getDebugName() const override { return rp.get() != nullptr ? rp->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component*) override {};

		// ============================================================================================================ 

		/** adds a connection to the given channels. */
		bool addConnection(int sourceIndex, int destinationIndex);

		/** Removes the connection from the given channels. */
		bool removeConnection(int sourceIndex, int destinationIndex);

		/** Removes all connections. */
		void clear();

		/** Gets the current peak value of the given channelIndex. */
		float getSourceGainValue(int channelIndex);

		// ============================================================================================================ 

		struct Wrapper;

	private:

		WeakReference<Processor> rp;


	};



	class ScriptingSynth : public ConstScriptingObject,
						   public DebugableObject
	{
	public:

		// ============================================================================================================ 

		ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_);
		~ScriptingSynth() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ChildSynth"); };
		bool objectDeleted() const override { return synth.get() == nullptr; };
		bool objectExists() const override { return synth != nullptr; };

		// ============================================================================================================ 

		String getDebugName() const override { return synth.get() != nullptr ? synth->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(synth.get() != nullptr ? dynamic_cast<ModulatorSynth*>(synth.get())->getNumActiveVoices() : 0) + String(" voices"); }
		void doubleClickCallback(const MouseEvent &, Component* ) override {};

		void rightClickCallback(const MouseEvent& e, Component* t) override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the synth. */
		String getId() const;

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue);;

        /** Returns the attribute with the given index. */
        float getAttribute(int index);

		/** Returns the number of attributes. */
		int getNumAttributes() const;
        
		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed);

		/** Returns the child synth with the given index. */
		ScriptingSynth* getChildSynthByIndex(int index);

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Returns the current peak level for the given channel. */
		float getCurrentLevel(bool leftChannel);

		/** Adds a modulator to the given chain and returns a reference. */
		var addModulator(var chainIndex, var typeName, var modName);

		/** Returns the modulator chain with the given index. */
		var getModulatorChain(var chainIndex);

		/** Adds a and connects a receiver modulator for the given global modulator. */
		var addGlobalModulator(var chainIndex, var globalMod, String modName);

		/** Adds and connects a receiving static time variant modulator for the given global modulator. */
		var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);

		/** Returns a reference as Sampler or undefined if no Sampler. */
		var asSampler();

		/** Returns a reference to the routing matrix object of the sound generator. */
		var getRoutingMatrix();

		// ============================================================================================================ 

		struct Wrapper;

	private:

		ApiHelpers::ModuleHandler moduleHandler;

		WeakReference<Processor> synth;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingSynth);

		// ============================================================================================================
	};


	class ScriptingMidiProcessor : public ConstScriptingObject,
								   public AssignableObject,
								   public DebugableObject
	{
	public:

		// ============================================================================================================ 

		ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_);;
		~ScriptingMidiProcessor() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MidiProcessor"); }
		bool objectDeleted() const override { return mp.get() == nullptr; }
		bool objectExists() const override { return mp != nullptr; }

		String getDebugName() const override { return mp.get() != nullptr ? mp->getId() : "Invalid"; };
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component* ) override {};

		void rightClickCallback(const MouseEvent& e, Component* t) override;

		int getCachedIndex(const var &indexExpression) const override;
		void assign(const int index, var newValue) override;
		var getAssignedValue(int /*index*/) const override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the MIDI Processor. */
		String getId() const;

		/** Sets the attribute of the MidiProcessor. If it is a script, then the index of the component is used. */
		void setAttribute(int index, float value);

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the MidiProcessor. */
		void setBypassed(bool shouldBeBypassed);;

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Export the control values (without the script). */
		String exportScriptControls();

		/** Restores the control values for scripts (without recompiling). */
		void restoreScriptControls(String base64Controls);

		// ============================================================================================================

		struct Wrapper;

	private:

		WeakReference<MidiProcessor> mp;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingMidiProcessor)

		// ============================================================================================================
	};

	class ScriptingAudioSampleProcessor : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, AudioSampleProcessor *sampleProcessor);
		~ScriptingAudioSampleProcessor() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("AudioSampleProcessor"); };
		bool objectDeleted() const override { return audioSampleProcessor.get() == nullptr; }
		bool objectExists() const override { return audioSampleProcessor != nullptr; }

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue);;

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed);

		/** loads the file. You can use the wildcard {PROJECT_FOLDER} to get the audio file folder for the current project. */
		void setFile(String fileName);

		/** Returns the length of the current sample selection in samples. */
		int getSampleLength() const;

		/** Sets the length of the current sample selection in samples. */
		void setSampleRange(int startSample, int endSample);

		// ============================================================================================================

		struct Wrapper; 

	private:

		WeakReference<Processor> audioSampleProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingAudioSampleProcessor);

		// ============================================================================================================
	};

	class ScriptingTableProcessor : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		ScriptingTableProcessor(ProcessorWithScriptingContent *p, LookupTableProcessor *tableProcessor);
		~ScriptingTableProcessor() {};

		Identifier getObjectName() const override {	RETURN_STATIC_IDENTIFIER("TableProcessor"); };
		bool objectDeleted() const override { return tableProcessor.get() == nullptr; }
		bool objectExists() const override { return tableProcessor != nullptr; }

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Sets the point with the given index to the values. */
		void setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve);

		/** Adds a new table point (x and y are normalized coordinates). */
		void addTablePoint(int tableIndex, float x, float y);

		/** Resets the table with the given index to a 0..1 line. */
		void reset(int tableIndex);

		// ============================================================================================================
		
		struct Wrapper;

	private:

		WeakReference<Processor> tableProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingTableProcessor);

		// ============================================================================================================
	};


	class TimerObject : public DynamicScriptingObject,
					    public ControlledObject
	{
	public:

		// ============================================================================================================

		TimerObject(ProcessorWithScriptingContent *p);
		~TimerObject();

		void mainControllerIsDeleted() { stopTimer(); };

		// ============================================================================================================

		Identifier getObjectName() const override { return "Timer"; }
		bool objectDeleted() const override { return false; }
		bool objectExists() const override { return false; }

		void timerCallback();
		void timerCallbackInternal(const var& callback, Result& r);

		// ============================================================================================================
		
		/** Starts the timer. */
		void startTimer(int intervalInMilliSeconds);

		/** Stops the timer. */
		void stopTimer();
		
		/** Sets the function that will be called periodically. */
		void setTimerCallback(var callbackFunction);

		struct Wrapper;

		// ============================================================================================================

	private:

		struct InternalTimer : public Timer
		{
		public:

			InternalTimer(TimerObject* parent_):
				parent(parent_)
			{
				
			}

			void timerCallback()
			{
				parent->timerCallback();
			}

		private:

			TimerObject* parent;
		};

		InternalTimer it;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimerObject)
        JUCE_DECLARE_WEAK_REFERENCEABLE(TimerObject);
	};


	class PathObject : public ConstScriptingObject,
					   public DebugableObject
	{
	public:

		// ============================================================================================================

		PathObject(ProcessorWithScriptingContent* p);
		~PathObject();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Path"); }

		
		String getDebugValue() const override { return String(p.getLength(), 2); }
		String getDebugName() const override { return "Path"; }
		
		void rightClickCallback(const MouseEvent &e, Component* componentToNotify) override;

		// ============================================================================================================ API Methods

		/** Loads a path from a data array. */
		void loadFromData(var data);

		/** Clears the Path. */
		void clear();

		/** Starts a new Path. It does not clear the path, so use 'clear()' if you want to start all over again. */
		void startNewSubPath(var x, var y);

		/** Closes the Path. */
		void closeSubPath();

		/** Adds a line to [x,y]. */
		void lineTo(var x, var y);

		/** Adds a quadratic bezier curve with the control point [cx,cy] and the end point [x,y]. */
		void quadraticTo(var cx, var cy, var x, var y);

		/** Adds an arc to the path. */
		void addArc(var area, var fromRadians, var toRadians);

		/** Returns the area ([x, y, width, height]) that the path is occupying with the scale factor applied. */
		var getBounds(var scaleFactor);

		// ============================================================================================================

		struct Wrapper;

		const Path& getPath() const { return p; }

	private:

		Path p;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PathObject);

		// ============================================================================================================
	};

	class GraphicsObject : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		GraphicsObject(ProcessorWithScriptingContent *p, ConstScriptingObject* parent);
		~GraphicsObject();

		// ============================================================================================================

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Graphics"); }
		
		// ============================================================================================================ API Methods

		/** Fills the whole area with the given colour. */
		void fillAll(var colour);

		/** Fills a rectangle with the given colour. */
		void fillRect(var area);

		/** Draws a rectangle. */
		void drawRect(var area, float borderSize);

		/** Fills a rounded rectangle. */
		void fillRoundedRectangle(var area, float cornerSize);

		/** Draws a rounded rectangle. */
		void drawRoundedRectangle(var area, float cornerSize, float borderSize);

		/** Draws a (non interpolated) horizontal line. */
		void drawHorizontalLine(int y, float x1, float x2);

		/** Sets a global transparency level. */
		void setOpacity(float alphaValue);

		/** Draws a line. */
		void drawLine(float x1, float x2, float y1, float y2, float lineThickness);

		/** Sets the current colour. */
		void setColour(var colour);

		/** Sets the current font. */
		void setFont(String fontName, float fontSize);

		/** Draws a centered and vertically stretched text. */
		void drawText(String text, var area);

		/** Draws a text with the given alignment (see the Label alignment property). */
		void drawAlignedText(String text, var area, String alignment);

		/** Sets the current gradient via an array [Colour1, x1, y1, Colour2, x2, y2] */
		void setGradientFill(var gradientData);

		/** Draws a ellipse in the given area. */
		void drawEllipse(var area, float lineThickness);
		
		/** Fills a ellipse in the given area. */
		void fillEllipse(var area);

		/** Draws a image into the area. */
		void drawImage(String imageName, var area, int xOffset, int yOffset);

		/** Draws a drop shadow around a rectangle. */
		void drawDropShadow(var area, var colour, int radius);

		/** Draws a triangle rotated by the angle in radians. */
		void drawTriangle(var area, float angle, float lineThickness);

		/** Fills a triangle rotated by the angle in radians. */
		void fillTriangle(var area, float angle);

		/** Adds a drop shadow based on the alpha values of the current image. */
		void addDropShadowFromAlpha(var colour, int radius);

		/** Fills a Path. */
		void fillPath(var path, var area);

		/** Draws the given path. */
		void drawPath(var path, var area, var thickNess);

		/** Rotates the canvas around center ([x, y]) by the given amount in radian. */
		void rotate(var angleInRadian, var center);
		
		// ============================================================================================================

		struct Wrapper;

		
		DrawActions::Handler& getDrawHandler() { return drawActionHandler; }

	private:

		Point<float> getPointFromVar(const var& data);
		Rectangle<float> getRectangleFromVar(const var &data);
		Rectangle<int> getIntRectangleFromVar(const var &data);

		Result rectangleResult;

		ConstScriptingObject* parent = nullptr;

		DrawActions::Handler drawActionHandler;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicsObject);

		// ============================================================================================================
	};

	class ExpansionObject : public ConstScriptingObject,
							public DebugableObject
	{
	public:

		ExpansionObject(ProcessorWithScriptingContent* p, Expansion* expansion);

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Expansion"); }

		virtual bool objectDeleted() const { return data.get() == nullptr; }
		virtual bool objectExists() const { return data.get() != nullptr; }

		String getDebugName() const override { return data != nullptr ? data->name.get() : "Deleted"; }
		String getDebugValue() const override { return data != nullptr ? data->name.get() : "Deleted"; }

		/** Returns a list of all samplemaps in this expansion pack. */
		var getSampleMapList();

		/** Returns a list of all audio files in this expansion pack. */
		var getAudioFileList();

		/** Returns a list of all image files in this expansion pack. */
		var getImageFilelist();

		/** Returns a reference string that can be used to load expansion pack data. */
		var getReferenceString(var relativeFilePath);

	private:

		struct Wrapper;

		WeakReference<Expansion> data;
	};

	class ExpansionHandlerObject : public ConstScriptingObject,
								   public ExpansionHandler::Listener
	{
	public:

		ExpansionHandlerObject(ProcessorWithScriptingContent* p);

		~ExpansionHandlerObject();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ExpansionHandler"); }

		void expansionPackLoaded(Expansion* currentExpansion) override;

		void expansionPackCreated(Expansion* exp) { expansionPackLoaded(exp); };

		/** Returns a list of expansion objects. */
		var getExpansionList();

		/** Returns a reference to the currently loaded expansion or undefined. */
		var getCurrentExpansion();

		/** Sets a function that will be executed when a new expansion will be loaded. */
		void setLoadingCallback(var function);

		/** Loads the expansion with the given name. Returns false, if expansion can't be loaded. */
		bool loadExpansion(const String expansionName);


	private:



		var loadingCallback;

		struct Wrapper;

		ExpansionHandler& handler;

	};

};



} // namespace hise
#endif  // SCRIPTINGAPIOBJECTS_H_INCLUDED
