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

#ifndef HI_SCRIPTING_API_H_INCLUDED
#define HI_SCRIPTING_API_H_INCLUDED

namespace hise { using namespace juce;


class ScriptBaseMidiProcessor;
class JavascriptMidiProcessor;

/** This class wraps all available functions for the scripting engine provided by a ScriptProcessor.
*	@ingroup scripting
*/
class ScriptingApi
{
public:

	/** All scripting methods related to the midi message that triggered the callback.
	*	@ingroup scriptingApi
	*
	*	Every method must be called on the message like this:
	*
	*		message.delayEvent(200);
	*/
	class Message: public ScriptingObject,
				   public ApiClass
	{
	public:

		// ============================================================================================================

		Message(ProcessorWithScriptingContent *p);;
		~Message();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Message"); }
		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Message"); }


		// ============================================================================================================ API Methods

		/** Return the note number. This can be called only on midi event callbacks. */
		int getNoteNumber() const;

		/** Delays the event by the sampleAmount. */
		void delayEvent(int samplesToDelay);

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

		/** Checks if the message is a program change message. */
		bool isProgramChange();

		/** Returns the program change number or -1 if it isn't a program change message. */
		int getProgramChangeNumber();

		/** Returns the Velocity. */
		int getVelocity() const;

		/** Ignores the event. */
		void ignoreEvent(bool shouldBeIgnored=true);;

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

		/** Returns the timestamp of the message. */
		int getTimestamp() const;

		/** Sets the start offset for the given message. */
		void setStartOffset(int newStartOffset);

		int getStartOffset() const;

		/** Stores a copy of the current event into the given holder object. */
		void store(var messageEventHolder) const;

		/** Creates a artificial copy of this event and returns the new event ID. */
		int makeArtificial();

		/** Checks if the event was created by a script earlier. */
		bool isArtificial() const;

		// ============================================================================================================

		void setHiseEvent(HiseEvent &m);
		void setHiseEvent(const HiseEvent& m);

		HiseEvent& getCurrentEventReference();

		struct Wrapper;

	private:

		friend class JavascriptMidiProcessor;
		friend class HardcodedScriptProcessor;

		HiseEvent* messageHolder;
		const HiseEvent* constMessageHolder;

		uint16 artificialNoteOnIds[128];

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Message);
	};

	/** All scripting methods related to the main engine can be accessed here.
	*	@ingroup scriptingApi
	*/
	class Engine: public ScriptingObject,
				  public ApiClass
	{
	public:

		// ============================================================================================================

		Engine(ProcessorWithScriptingContent *p);
		~Engine() {};

		Identifier getObjectName() const override  { RETURN_STATIC_IDENTIFIER("Engine"); };

		// ============================================================================================================ API Methods

		/** Loads a font file. This is deprecated, because it might result in different names on various OS. Use loadFontAs() instead. */
		void loadFont(const String &fileName);

		/** Loads the font from the given file in the image folder and registers it under the fontId. This is platform agnostic. */
		void loadFontAs(String fileName, String fontId);

		/** Sets the font that will be used as default font for various things. */
		void setGlobalFont(String fontName);

		/** Returns the current sample rate. */
		double getSampleRate() const;

		/** Converts milli seconds to samples */
		double getSamplesForMilliSeconds(double milliSeconds) const;;

		/** Converts samples to quarter beats using the current tempo. */
		double getQuarterBeatsForSamples(double samples);

		/** Converts milliseconds to quarter beats using the current tempo. */
		double getQuarterBeatsForMilliSeconds(double milliSeconds);

		/** Converts quarter beats to samples using the current tempo. */
		double getSamplesForQuarterBeats(double quarterBeats);

		/** Converts quarter beats to milliseconds using the current tempo. */
		double getMilliSecondsForQuarterBeats(double quarterBeats);

		/** Converts samples to quarter beats using the given tempo. */
		double getQuarterBeatsForSamplesWithTempo(double samples, double bpm);

		/** Converts milliseconds to quarter beats using the given tempo. */
		double getQuarterBeatsForMilliSecondsWithTempo(double milliSeconds, double bpm);

		/** Converts quarter beats to samples using the given tempo. */
		double getSamplesForQuarterBeatsWithTempo(double quarterBeats, double bpm);

		/** Converts quarter beats to milliseconds using the given tempo. */
		double getMilliSecondsForQuarterBeatsWithTempo(double quarterBeats, double bpm);

		/** Converts samples to milli seconds. */
		double getMilliSecondsForSamples(double samples) const { return samples / getSampleRate() * 1000.0; };

		/** Converts decibel (-100.0 ... 0.0) to gain factor (0.0 ... 1.0). */
		double getGainFactorForDecibels(double decibels) const { return Decibels::decibelsToGain<double>(decibels); };

		/** Converts gain factor (0.0 .. 1.0) to decibel (-100.0 ... 0). */
		double getDecibelsForGainFactor(double gainFactor) const { return Decibels::gainToDecibels<double>(gainFactor); };

		/** Converts midi note number 0 ... 127 to Frequency 20 ... 20.000. */
		double getFrequencyForMidiNoteNumber(int midiNumber) const { return MidiMessage::getMidiNoteInHertz(midiNumber); };

		/** Converts a semitone value to a pitch ratio (-12 ... 12) -> (0.5 ... 2.0) */
		double getPitchRatioFromSemitones(double semiTones) const { return pow(2.0, semiTones / 12.0); }

		/** Converts a pitch ratio to semitones (0.5 ... 2.0) -> (-12 ... 12) */
		double getSemitonesFromPitchRatio(double pitchRatio) const { return 1200.0 * log2(pitchRatio); }

		/** Returns the downsampling factor for the modulation signal (default is 8). */
		double getControlRateDownsamplingFactor() const;

		/** Iterates the given sub-directory of the Samples folder and returns a list with all references to audio files. */
		var getSampleFilesFromDirectory(const String& relativePathFromSampleFolder, bool recursive);

		/** Shows a message with a question and executes the function after the user has selected his choice. */
		void showYesNoWindow(String title, String markdownMessage, var callback);

		/** Creates a (or returns an existing ) script look and feel object. */
		var createGlobalScriptLookAndFeel();

		/** Returns the latency of the plugin as reported to the host. Default is 0. */
		int getLatencySamples() const;

		/** sets the latency of the plugin as reported to the host. Default is 0. */
		void setLatencySamples(int latency);

		/** Converts MIDI note number to Midi note name ("C3" for middle C). */
		String getMidiNoteName(int midiNumber) const { return MidiMessage::getMidiNoteName(midiNumber, true, true, 3); };

		/** Converts MIDI note name to MIDI number ("C3" for middle C). */
		int getMidiNoteFromName(String midiNoteName) const;

		/** Creates a Dsp node network. */
		var createDspNetwork(String id);

		/** Creates (and activates) the expansion handler. */
		var createExpansionHandler();

		/** Creates a reference to the DSP network of another script processor. */
		var getDspNetworkReference(String processorId, String id);

		/** Sends an allNotesOff message at the next buffer. */
		void allNotesOff();

		/** Adds an entire module to the user preset system. */
		void addModuleStateToUserPreset(var moduleId);

		/** Returns the uptime of the engine in seconds. */
		double getUptime() const;

		/** Sets a key of the global keyboard to the specified colour (using the form 0x00FF00 for eg. of the key to the specified colour. */
		void setKeyColour(int keyNumber, int colourAsHex);

		/** Extends the compilation timeout. Use this if you have a long task that would get cancelled otherwise. This is doing nothing in compiled plugins. */
		void extendTimeOut(int additionalMilliseconds);

		/** Changes the lowest visible key on the on screen keyboard. */
		void setLowestKeyToDisplay(int keyNumber);

		/** Shows a error message on the compiled plugin (or prints it on the console). Use isCritical if you want to disable the "Ignore" Button. */
		void showErrorMessage(String message, bool isCritical);

		/** Shows a message with an overlay on the compiled plugin with an "OK" button in order to notify the user about important events. */
		void showMessage(String message);

		/** Returns the millisecond value for the supplied tempo (HINT: Use "TempoSync" mode from Slider!) */
		double getMilliSecondsForTempo(int tempoIndex) const;;

        /** launches the given URL in the system's web browser. */
        void openWebsite(String url);

		/** Creates a list of all available expansions. */
		var getExpansionList();

		/** Sets the active expansion and updates the preset browser. */
		bool setCurrentExpansion(const String& expansionName);

		/** Loads the next user preset. */
		void loadNextUserPreset(bool stayInDirectory);

		/** Loads the previous user preset. */
		void loadPreviousUserPreset(bool stayInDirectory);

		/** Checks if the global MPE mode is enabled. */
		bool isMpeEnabled() const;

		/** Returns the currently loaded user preset (without extension). */
		String getCurrentUserPresetName();

		/** Asks for a preset name (if presetName is empty) and saves the current user preset. */
		void saveUserPreset(String presetName);

		/** Loads a user preset with the given relative path (use `/` for directory separation). */
		void loadUserPreset(const String& relativePathWithoutFileEnding);

		/** Sets the tags that appear in the user preset browser. */
		void setUserPresetTagList(var listOfTags);

		/** Returns a list of all available user presets as relative path. */
		var getUserPresetList() const;

		/** Sets whether the samples are allowed to be duplicated. Set this to false if you operate on the same samples differently. */
		void setAllowDuplicateSamples(bool shouldAllow);

		/** Calling this makes sure that all audio files are loaded into the pool and will be available in the compiled plugin. */
		void loadAudioFilesIntoPool();

		/** Loads an image into the pool. You can use a wildcard to load multiple images at once. */
		void loadImageIntoPool(const String& id);

		/** Removes all entries from the samplemap pool */
		void clearSampleMapPool();

		/** Removes all entries from the MIDi file pool. */
		void clearMidiFilePool();

		/** Rebuilds the entries for all cached pools (MIDI files and samplemaps). */
		void rebuildCachedPools();

		/** Returns the Bpm of the host. */
		double getHostBpm() const;

		/** Overwrites the host BPM. Use -1 for sync to host. */
		void setHostBpm(double newTempo);

		/** Returns the current memory usage in MB. */
		double getMemoryUsage() const;

		/** Returns the current CPU usage in percent (0 ... 100) */
		double getCpuUsage() const;

		/** Returns the amount of currently active voices. */
		int getNumVoices() const;

		/** Returns the name for the given macro index. */
		String getMacroName(int index);

		/** Enables the macro system to be used by the end user. */
		void setFrontendMacros(var nameList);

		/** Returns the current operating system ("OSX" or ("WIN"). */
		String getOS();

		/** Returns the mobile device that this software is running on. */
		String getDeviceType();

		/** Returns the full screen resolution for the current device. */
		var getDeviceResolution();

		/** Returns true if running as VST / AU / AAX plugin. */
		bool isPlugin() const;

		/** Returns the preload progress from 0.0 to 1.0. Use this to display some kind of loading icon. */
		double getPreloadProgress();

		/** Returns the current Zoom Level. */
		var getZoomLevel() const;

		/** Returns an object that contains all filter modes. */
		var getFilterModeList() const;

        /** Returns the product version (not the HISE version!). */
        String getVersion();

        /** Returns the product name (not the HISE name!). */
        String getName();

		/** Returns the current peak volume (0...1) for the given channel. */
		double getMasterPeakLevel(int channel);

		/** Returns a object that contains the properties for the settings dialog. */
		var getSettingsWindowObject();

		/** Allows access to the data of the host (playing status, timeline, etc...). */
		DynamicObject *getPlayHead();

		/** Checks if the given CC number is used for parameter automation and returns the index of the control. */
		int isControllerUsedByAutomation(int controllerNumber);

		/** Creates a MIDI List object. */
        ScriptingObjects::MidiList *createMidiList();

		/** Creates a SliderPack Data object. */
		ScriptingObjects::ScriptSliderPackData* createSliderPackData();

		/** Creates a SliderPack Data object and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptSliderPackData* createAndRegisterSliderPackData(int index);

		/** Creates a Table object and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptTableData* createAndRegisterTableData(int index);

		/** Creates a audio file holder and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptAudioFile* createAndRegisterAudioFile(int index);

		/** Creates a new timer object. */
		ScriptingObjects::TimerObject* createTimerObject();

		/** Creates a storage object for Message events. */
		ScriptingObjects::ScriptingMessageHolder* createMessageHolder();

		/** Exports an object as JSON. */
		void dumpAsJSON(var object, String fileName);

		/** Imports a JSON file as object. */
		var loadFromJSON(String fileName);

		/** Displays the progress (0.0 to 1.0) in the progress bar of the editor. */
		void setCompileProgress(var progress);

		/** Matches the string against the regex token. */
		bool matchesRegex(String stringToMatch, String regex);

        /** Returns an array with all matches. */
        var getRegexMatches(String stringToMatch, String regex);

        /** Returns a string of the value with the supplied number of digits. */
        String doubleToString(double value, int digits);

		/** Reverts the last controller change. */
		void undo();

		/** Redo the last controller change. */
		void redo();

		// ============================================================================================================

		struct Wrapper;

		ScriptBaseMidiProcessor* parentMidiProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine);
	};

	/** All scripting functions for sampler specific functionality. */
	class Sampler : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler);
		~Sampler() {};

		Identifier getObjectName() const override { return "Sampler"; }
		bool objectDeleted() const override { return sampler.get() == nullptr; }
		bool objectExists() const override { return sampler.get() != nullptr; }

		// ============================================================================================================ API Methods

		/** Enables / Disables the automatic round robin group start logic (works only on samplers). */
		void enableRoundRobin(bool shouldUseRoundRobin);

		/** Enables the group with the given index (one-based). Works only with samplers and `enableRoundRobin(false)`. */
		void setActiveGroup(int activeGroupIndex);

		/** Returns the amount of actual RR groups for the notenumber and velocity*/
		int getRRGroupsForMessage(int noteNumber, int velocity);

		/** Recalculates the RR Map. Call this at compile time if you want to use 'getRRGroupForMessage()'. */
		void refreshRRMap();

		/** Selects samples using the regex string as wildcard and the selectMode ("SELECT", "ADD", "SUBTRACT")*/
		void selectSounds(String regex);

		/** Returns the amount of selected samples. */
		int getNumSelectedSounds();

		/** Sets the property of the sampler sound for the selection. */
		void setSoundPropertyForSelection(int propertyIndex, var newValue);

		/** Sets the property for all samples of the sampler. */
		void setSoundPropertyForAllSamples(int propertyIndex, var newValue);

		/** Returns the property of the sound with the specified index. */
		var getSoundProperty(int propertyIndex, int soundIndex);

		/** Sets the property for the index within the selection. */
		void setSoundProperty(int soundIndex, int propertyIndex, var newValue);

		/** Purges all samples of the given mic (Multimic samples only). */
		void purgeMicPosition(String micName, bool shouldBePurged);

		/** Returns the name of the channel with the given index (Multimic samples only. */
		String getMicPositionName(int channelIndex);

		/** Returns an array with all samples that match this regex. */
		var createSelection(String regex);

		/** Returns an array with all samples from the index data (can be either int or array of int, -1 selects all.). */
		var createSelectionFromIndexes(var indexData);

		/** Returns a list of the sounds selected by the selectSounds() method. */
		var createListFromScriptSelection();

		/** Returns a list of the sounds selected in the samplemap. */
		var createListFromGUISelection();

        /** Loads the content of the given sample into an array of VariantBuffers that can be used
            for analysis.
        */
        var loadSampleForAnalysis(int indexInSelection);

		/** Returns the number of mic positions. */
		int getNumMicPositions() const;

		/** Checks if the mic position is purged. */
		bool isMicPositionPurged(int micIndex);

		/** Checks whether the note number is mapped to any samples. */
		bool isNoteNumberMapped(int noteNumber);

		/** Refreshes the interface. Call this after you changed the properties. */
		void refreshInterface();

		/** Loads a new samplemap into this sampler. */
		void loadSampleMap(const String &fileName);

		/** Loads a few samples in the current samplemap and returns a list of references to these samples. */
		var importSamples(var fileNameList, bool skipExistingSamples);

		/** Returns an array with all available sample maps. */
		var getSampleMapList() const;

        /** Returns the currently loaded sample map. */
        String getCurrentSampleMapId() const;

		/** Returns the number of attributes. */
		int getNumAttributes() const;

        /** Gets the attribute with the given index (use the constants for clearer code). */
        var getAttribute(int index) const;
        
        /** Returns the ID of the attribute with the given index. */
		String getAttributeId(int index);

        /** Sets a attribute to the given value. */
        void setAttribute(int index, var newValue);

		/** Disables dynamic resizing when a sample map is loaded. */
		void setUseStaticMatrix(bool shouldUseStaticMatrix);

		/** Enables a presorting of the sounds into RR groups. This might improve the performance at voice start if you have a lot of samples (> 20.000) in many RR groups. */
		void setSortByRRGroup(bool shouldSort);

		/** Saves (and loads) the current samplemap to the given path (which should be the same string as the ID). */
		bool saveCurrentSampleMap(String relativePathWithoutXml);

		/** Clears the current samplemap. */
		bool clearSampleMap();

		// ============================================================================================================

		struct Wrapper;

	private:

		WeakReference<Processor> sampler;
		SelectedItemSet<ModulatorSamplerSound::Ptr> soundSelection;

		Array<Identifier> sampleIds;
	};


	/** Provides access to the synth where the script processor resides.
	*	@ingroup scriptingApi
	*
	*	There are special methods for SynthGroups which only work with SynthGroups
	*/
	class Synth: public ScriptingObject,
				 public ApiClass
	{
	public:

		// ============================================================================================================

		Synth(ProcessorWithScriptingContent *p, ModulatorSynth *ownerSynth);
		~Synth() { artificialNoteOns.clear(); }

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Synth"); };

		typedef ScriptingObjects::ScriptingModulator ScriptModulator;
		typedef ScriptingObjects::ScriptingEffect ScriptEffect;
		typedef ScriptingObjects::ScriptingMidiProcessor ScriptMidiProcessor;
		typedef ScriptingObjects::ScriptingSynth ScriptSynth;
		typedef ScriptingObjects::ScriptingAudioSampleProcessor ScriptAudioSampleProcessor;
		typedef ScriptingObjects::ScriptingTableProcessor ScriptTableProcessor;
		typedef ScriptingObjects::ScriptingSlotFX ScriptSlotFX;
		typedef ScriptingObjects::ScriptedMidiPlayer ScriptMidiPlayer;
		typedef ScriptingObjects::ScriptRoutingMatrix ScriptRoutingMatrix;

		// ============================================================================================================ API Methods

		/** Adds the interface to the Container's body (or the frontend interface if compiled) */
		void addToFront(bool addToFront);

		/** Defers all callbacks to the message thread (midi callbacks become read-only). */
		void deferCallbacks(bool makeAsynchronous);

		/** Sends a note off message. The envelopes will tail off. */
		void noteOff(int noteNumber);

		/** Sends a note off message for the supplied event ID. This is more stable than the deprecated noteOff() method. */
		void noteOffByEventId(int eventId);

		/** Sends a note off message for the supplied event ID with the given delay in samples. */
		void noteOffDelayedByEventId(int eventId, int timestamp);

		/** Plays a note and returns the event id. Be careful or you get stuck notes! */
		int playNote(int noteNumber, int velocity);

		/** Plays a note and returns the event id with the given channel and start offset. */
		int playNoteWithStartOffset(int channel, int number, int velocity, int offset);

		/** Fades all voices with the given event id to the target volume (in decibels). */
		void addVolumeFade(int eventId, int fadeTimeMilliseconds, int targetVolume);

		/** Adds a pitch fade to the given event ID. */
		void addPitchFade(int eventId, int fadeTimeMilliseconds, int targetCoarsePitch, int targetFinePitch);

		/** Adds the event from the given holder and returns a event id for note ons. */
		int addMessageFromHolder(var messageHolder);

		/** Starts the timer of the synth. */
		void startTimer(double seconds);

		/** Sets an attribute of the parent synth. */
		void setAttribute(int attributeIndex, float newAttribute);

		/** Applies a gain factor to a specified voice. */
		void setVoiceGainValue(int voiceIndex, float gainValue);

		/** Applies a pitch factor (0.5 ... 2.0) to a specified voice. */
		void setVoicePitchValue(int voiceIndex, double pitchValue);

		/** Returns the attribute of the parent synth. */
		float getAttribute(int attributeIndex) const;

		/** Adds a note on to the buffer. */
		int addNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples);

		/** Adds a note off to the buffer. */
		void addNoteOff(int channel, int noteNumber, int timeStampSamples);

		/** Adds a controller to the buffer. */
		void addController(int channel, int number, int value, int timeStampSamples);

		/** Sets the internal clock speed. */
		void setClockSpeed(int clockSpeed);

		/** If set to true, this will kill retriggered notes (default). */
		void setShouldKillRetriggeredNote(bool killNote);

		/** Returns an array of all modulators that match the given regex. */
		var getAllModulators(String regex);

		/** Stops the timer of the synth. You can call this also in the timer callback. */
		void stopTimer();

		/** Checks if the timer for this script is running. */
		bool isTimerRunning() const;

		/** Returns the current timer interval in seconds. */
		double getTimerInterval() const;

		/** Sets one of the eight macro controllers to the newValue.
		*
		*	@param macroIndex the index of the macro from 1 - 8
		*	@param newValue The range for the newValue is 0.0 - 127.0.
		*/
		void setMacroControl(int macroIndex, float newValue);


		/** Sends a controller event to the synth. */
		void sendController(int controllerNumber, int controllerValue);

		/** The same as sendController (for backwards compatibility) */
		void sendControllerToChildSynths(int controllerNumber, int controllerValue);

		/** Returns the number of child synths. Works with SynthGroups and SynthChains. */
		int getNumChildSynths() const;

		/** Sets a ModulatorAttribute.
		*
		*	@param chainId the chain where the Modulator is. GainModulation = 1, PitchModulation = 0
		*	@param modulatorIndex the index of the Modulator starting with 0.
		*	@param attributeIndex the index of the Modulator starting with 0. Intensity is '-12', Bypassed is '-13'
		*	@param newValue the value. The range for Gain is 0.0 - 1.0, the Range for Pitch is -12.0 ... 12.0
		*
		*/
		void setModulatorAttribute(int chainId, int modulatorIndex, int attributeIndex, float newValue);

		/** Returns the number of pressed keys (!= the number of playing voices!). */
		int getNumPressedKeys() const {return numPressedKeys.get(); };

		/** Checks if any key is pressed. */
		bool isLegatoInterval() const { return numPressedKeys.get() != 1; };

		/** Checks if the given key is pressed. */
		bool isKeyDown(int noteNumber) { return keyDown[noteNumber]; };

		/** Adds a Modulator to the synth's chain. If it already exists, it returns the index. */
		ScriptModulator* addModulator(int chainId, const String &type, const String &id);

		/** Removes the modulator. */
		bool removeModulator(var mod);

		/** Adds a effect (index = -1 to append it at the end). */
		ScriptEffect* addEffect(const String &type, const String &id, int index);

		/** Removes the given effect. */
		bool removeEffect(var effect);

		/** Returns the Modulator with the supplied name. Can be only called in onInit. It looks also in all child processors. */
		ScriptModulator *getModulator(const String &name);

		/** Returns the Effect with the supplied name. Can only be called in onInit(). It looks also in all child processors. */
		ScriptEffect *getEffect(const String &name);
	
        /** Returns an array of all effects that match the given regex. */
        var getAllEffects(String regex);

		/** Returns the MidiProcessor with the supplied name. Can not be the own name! */
		ScriptMidiProcessor * getMidiProcessor(const String &name);

		/** Returns the child synth with the supplied name. */
		ScriptSynth * getChildSynth(const String &name);

		/** Returns the child synth with the given index. */
		ScriptSynth* getChildSynthByIndex(int index);

		/** Searches the child processors and returns a list with every ID of the given type. */
		var getIdList(const String &type);

		/** Returns the child synth with the supplied name. */
		ScriptAudioSampleProcessor * getAudioSampleProcessor(const String &name);

		/** Returns the table processor with the given name. */
		ScriptTableProcessor *getTableProcessor(const String &name);

		/** Returns the first sampler with the name name. */
		Sampler *getSampler(const String &name);

		/** Returns the first slot with the given name. */
		ScriptSlotFX* getSlotFX(const String& name);

		/** Creates a reference to the given MIDI player. */
		ScriptMidiPlayer* getMidiPlayer(const String& playerId);

		/** Creates a reference to the routing matrix of the given processor. */
		ScriptRoutingMatrix* getRoutingMatrix(const String& processorId);

		/** Returns the index of the Modulator in the chain with the supplied chainId */
		int getModulatorIndex(int chainId, const String &id) const;

		/** Returns true if the sustain pedal is pressed. */
		bool isSustainPedalDown() const { return sustainState; }

		// ============================================================================================================

		void clearNoteCounter()
		{
			keyDown.clear();
			numPressedKeys.set(0);
		}

		void handleNoteCounter(const HiseEvent& e, bool inc) noexcept
		{
			if (e.isArtificial())
				return;

			if (inc)
			{
				++numPressedKeys;
				keyDown.setBit(e.getNoteNumber(), true);
			}
			else
			{
				--numPressedKeys; 
				if (numPressedKeys.get() < 0) 
					numPressedKeys.set(0);

				keyDown.setBit(e.getNoteNumber(), false);
			}
		}

		void setSustainPedal(bool shouldBeDown) { sustainState = shouldBeDown; };

		struct Wrapper;

	private:

		int internalAddNoteOn(int channel, int noteNumber, int velocity, int timestamp, int startOffset);

		friend class ModuleHandler;

		OwnedArray<Message> artificialNoteOns;
		ModulatorSynth * const owner;
		Atomic<int> numPressedKeys;
		BigInteger keyDown;

		ApiHelpers::ModuleHandler moduleHandler;

		SelectedItemSet<WeakReference<ModulatorSamplerSound>> soundSelection;

		ScriptBaseMidiProcessor* parentMidiProcessor = nullptr;
		JavascriptMidiProcessor* jp = nullptr;

		bool sustainState;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Synth);
	};

	/** A set of handy function to debug the script.
	*	@ingroup scriptingApi
	*
	*
	*/
	class Console: public ApiClass,
				   public ScriptingObject
	{
	public:

		// ============================================================================================================

		Console(ProcessorWithScriptingContent *p);;

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Console"); }
		static Identifier getClassName()   { RETURN_STATIC_IDENTIFIER("Console"); };

		bool allowIllegalCallsOnAudioThread(int /*functionIndex*/) const override { return true; }

		// ============================================================================================================ API Methods

		/** Prints a message to the console. */
		void print(var debug);

		/** Starts the benchmark. You can give it a name that will be displayed with the result if desired. */
		void start() { startTime = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()); };

		/** Stops the benchmark and prints the result. */
		void stop();

		/** Clears the console. */
		void clear();

		/** Throws an error message if the condition is not true. */
		void assertTrue(var condition);

		/** Throws an error message if the values are not equal. */
		void assertEqual(var v1, var v2);

		/** Throws an error message if the value is undefined. */
		void assertIsDefined(var value);

		/** Throws an error message if the value is not an object or array. */
		void assertIsObjectOrArray(var value);

		/** Throws an error message if the value is not a legal number (eg. string or array or infinity or NaN). */
		void assertLegalNumber(var value);

		struct Wrapper;

	private:

		double startTime;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Console)
	};

	class Content;

	/** A list with all available modules. */
	class ModuleIds : public ApiClass
	{
	public:

		ModuleIds(ModulatorSynth* s);

		/** Returns the name. */
		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ModuleIds"); }

	private:

		static Array<Identifier> getTypeList(ModulatorSynth* s);

		ModulatorSynth* ownerSynth;
	};

	class Server : public ApiClass,
				   public ScriptingObject
	{
	public:

		enum StatusCodes
		{
			StatusOK = 200,
			StatusNotFound = 404,
			StatusServerError = 500,
			StatusAuthenticationFail = 403,
			numStatusCodes
		};

		Server(JavascriptProcessor* jp);

		~Server()
		{
			internalThread.stopThread(3000);
		}
		
		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Server"); }

		/** Sets the base URL for all server queries. */
		void setBaseURL(String url);

		/** Calls a sub URL and executes the callback when finished. */
		void callWithGET(String subURL, var parameters, var callback);

		/** Calls a sub URL with POST arguments and executes the callback when finished. */
		void callWithPOST(String subURL, var parameters, var callback);
		
		/** Adds the given String to the HTTP POST header. */
		void setHttpHeader(String additionalHeader);

		/** Downloads a file to the given target. */
		void downloadFile(String subURL, var parameters, var targetFile, var callback);

		/** Stops the pending download (and deletes the file). */
		bool stopDownload(String subURL, var parameters);

	private:

		struct PendingDownload: public ReferenceCountedObject,
								public URL::DownloadTask::Listener
		{
			void finished(URL::DownloadTask* , bool success) override
			{
				data->setProperty("success", success);
				data->setProperty("finished", true);
				call();

				isRunning = false;
				isFinished = true;
			}

			void progress(URL::DownloadTask* , int64 bytesDownloaded, int64 totalLength) override
			{
				data->setProperty("numTotal", totalLength);
				data->setProperty("numDownloaded", bytesDownloaded);
				call();
			}

			void abort()
			{
				data->setProperty("success", false);
				data->setProperty("finished", true);
				call();

				isRunning = false;
				isFinished = true;
				download = nullptr;

				targetFile.deleteFile();
			}

			void call();

			void start()
			{
				isRunning = true;
				download = downloadURL.downloadToFile(targetFile, {}, this);
				
				data = new DynamicObject();
				
				data->setProperty("numTotal", 0);
				data->setProperty("numDownloaded", 0);
				data->setProperty("finished", false);
				data->setProperty("success", false);
			}

			DynamicObject::Ptr data;
			using Ptr = ReferenceCountedObjectPtr<PendingDownload>;
			URL downloadURL;
			File targetFile;
			var callback;
			ScopedPointer<URL::DownloadTask> download;
			std::atomic<bool> isRunning = { false };
			std::atomic<bool> isFinished = { false };
			std::atomic<bool> shouldAbort = { false };
			JavascriptProcessor* jp = nullptr;
		};

		struct PendingCallback: public ReferenceCountedObject
		{
			using Ptr = ReferenceCountedObjectPtr<PendingCallback>;

			URL url;
			String extraHeader;
			bool isPost;
			var function;
			int status = 0;
		};

		struct WebThread : public Thread
		{
			WebThread(Server& p) :
				Thread("Server Thread"),
				parent(p)
			{};

			Server& parent;

			void run() override;

			CriticalSection queueLock;

			ReferenceCountedArray<PendingCallback> pendingCallbacks;
			ReferenceCountedArray<PendingDownload> pendingDownloads;

		} internalThread;

		JavascriptProcessor* jp;

		URL getWithParameters(String subURL, var parameters);

		URL baseURL;
		String extraHeader;

		struct Wrapper;
	};

	class FileSystem : public ApiClass,
					  public ScriptingObject,
					   public ControlledObject
	{
	public:

		enum SpecialLocations
		{
			AudioFiles,
			Expansions,
			Images,
			Samples,
			UserPresets,
			AppData,
			UserHome,
			Documents,
			Desktop,
			Downloads,
			numSpecialLocations
		};

		FileSystem(ProcessorWithScriptingContent* pwsc);
		~FileSystem();

		Identifier getObjectName() const override
		{
			return "FileSystem";
		}

		// ========================================================= API Calls

		/** Returns the current sample folder as File object. */
		var getFolder(var locationType);

		/** Returns a list of all child files of a directory that match the wildcard. */
		var findFiles(var directory, String wildcard, bool recursive);

		/** Opens a file browser to choose a file. */
		void browse(var startFolder, bool forSaving, String wildcard, var callback);

		/** Returns a unique machine ID that can be used to identify the computer. */
		String getSystemId();

		// ========================================================= End of API calls

		

		ProcessorWithScriptingContent* p;

	private:

		File getFile(SpecialLocations l);

		struct Wrapper;


	};

	class Colours: public ApiClass
	{
	public:

		// ============================================================================================================

		Colours();
		~Colours() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Colours"); }

		// ============================================================================================================ API Methods

		/** Returns a colour value with the specified alpha value. */
		int withAlpha(int colour, float alpha);

		// ============================================================================================================

		struct Wrapper;

	private:

		// ============================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Colours);
	};


	class ModulatorApi : public ApiClass
	{
	public:

		ModulatorApi(Modulator* mod_);

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Modulator") }

		/** Sets the intensity of the modulator (raw value) */
		void setIntensity(var newValue)
		{
			m->setIntensity((float)newValue);
			BACKEND_ONLY(mod->sendChangeMessage());
		}

		/** Bypasses the modulator. */
		void setBypassed(var newValue)
		{
			mod->setBypassed((bool)newValue);
			BACKEND_ONLY(mod->sendChangeMessage());
		}

	private:

		struct Wrapper
		{
			API_VOID_METHOD_WRAPPER_1(ModulatorApi, setIntensity);
			API_VOID_METHOD_WRAPPER_1(ModulatorApi, setBypassed);
		};

		Modulator* mod;
		Modulation* m;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorApi)
	};
};

} // namespace hise
#endif
