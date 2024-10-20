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

		/** Checks if the message is a MONOPHONIC aftertouch message. */
		bool isMonophonicAfterTouch() const;;

		/** Returns the aftertouch value of the monophonic aftertouch message. */
		int getMonophonicAftertouchPressure() const;;

		/** Sets the pressure value of the monophonic aftertouch message */
		void setMonophonicAfterTouchPressure(int pressure);;

		/** Checks if the message is a POLYPHONIC aftertouch message (Use isChannelPressure() for monophonic aftertouch). */
		bool isPolyAftertouch() const;;

		/** Returns the polyphonic aftertouch note number. */
		int getPolyAfterTouchNoteNumber() const;

		/** Checks if the message is a POLYPHONIC aftertouch message (Use isChannelPressure() for monophonic aftertouch). */
		int getPolyAfterTouchPressureValue() const;;

		/** Copied from MidiMessage. */
		void setPolyAfterTouchNoteNumberAndPressureValue(int noteNumber, int aftertouchAmount);;

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

		/** Creates a artificial copy of this event and returns the new event ID. If the event is already artificial it will return the event ID. */
		int makeArtificial();

		/** Creates a artificial copy of this event and returns the new event ID. If the event is artificial it will make a new one with a new ID. */
		int makeArtificialOrLocal();

		/** Checks if the event was created by a script earlier. */
		bool isArtificial() const;

		/** Sets a callback that will be performed when an all notes off message is received. */
		void setAllNotesOffCallback(var onAllNotesOffCallback);

		/** This will forward the message to the MIDI out of the plugin. */
		void sendToMidiOut();

		// ============================================================================================================

		void setHiseEvent(HiseEvent &m);
		void setHiseEvent(const HiseEvent& m);

		HiseEvent& getCurrentEventReference();

		struct Wrapper;

		void pushArtificialNoteOn(const HiseEvent& e)
		{
			jassert(e.isArtificial());
			artificialNoteOnIds[e.getNoteNumber()] = e.getEventId();
		}

		void onAllNotesOff();

	private:

		int makeArtificialInternal(bool makeLocal);
		
		WeakCallbackHolder allNotesOffCallback;

		friend class Synth;

		friend class JavascriptMidiProcessor;
		friend class HardcodedScriptProcessor;

		HiseEvent* messageHolder;
		const HiseEvent* constMessageHolder;

		uint16 artificialNoteOnIds[128];

		HiseEvent artificialNoteOnThatWasKilled;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Message);
		JUCE_DECLARE_WEAK_REFERENCEABLE(Message);
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
		~Engine();

		Identifier getObjectName() const override  { RETURN_STATIC_IDENTIFIER("Engine"); };

		// ============================================================================================================ API Methods

		/** Loads a font file. This is deprecated, because it might result in different names on various OS. Use loadFontAs() instead. */
		void loadFont(const String &fileName);

		/** Loads the font from the given file in the image folder and registers it under the fontId. This is platform agnostic. */
		void loadFontAs(String fileName, String fontId);

		/** Sets the font that will be used as default font for various things. */
		void setGlobalFont(String fontName);

		/** Sets the minimum sample rate for the global processing (and adds oversampling if the current samplerate is lower). */
		bool setMinimumSampleRate(double minimumSampleRate);

		/** Sets the maximum buffer size that is processed at once. If the buffer size from the audio driver / host is bigger than this number, it will split up the incoming buffer and call process multiple times. */
		void setMaximumBlockSize(int numSamplesPerBlock);

		/** Returns the current sample rate. */
		double getSampleRate() const;

		/** Returns the current maximum processing block size. */
		int getBufferSize() const;

		/** Converts milli seconds to samples */
		double getSamplesForMilliSeconds(double milliSeconds) const;;

		/** Returns the tempo name for the given index */
		String getTempoName(int tempoIndex);

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

		/** Returns the platform specific extra definitions from the Project settings as JSON object. */
		var getExtraDefinitionsInBackend();
		
		/** Shows a message with a question and executes the function after the user has selected his choice. */
		void showYesNoWindow(String title, String markdownMessage, var callback);

		/** Decodes an Base64 encrypted valuetree (eg. HiseSnippets). */
		String decodeBase64ValueTree(const String& b64Data);

		/** Creates a (or returns an existing ) script look and feel object. */
		var createGlobalScriptLookAndFeel();

		/** Performs an action that can be undone via Engine.undo(). */
		bool performUndoAction(var thisObject, var undoAction);

		/** Returns the amount of output channels. */
		int getNumPluginChannels() const;

        /** Creates an FFT object. */
		var createFFT();

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

		/** Creates a MIDI Automation handler. */
		var createMidiAutomationHandler();

		/** Creates an user preset handler. */
		var createUserPresetHandler();

		/** Creates a broadcaster that can send messages to attached listeners. */
		var createBroadcaster(var defaultValues);

		/** Creates a reference to the DSP network of another script processor. */
		var getDspNetworkReference(String processorId, String id);

		/** Returns a reference to the global routing manager. */
		var getGlobalRoutingManager();

        /** Returns a reference to the global Loris manager. */
        var getLorisManager();
        
		/** Returns a reference to a complex data type from the given module. */
		var getComplexDataReference(String dataType, String moduleId, int index);

		/** Creates a background task that can execute heavyweight functions. */
		var createBackgroundTask(String name);

        /** Creates a fix object factory using the data layout. */
        var createFixObjectFactory(var layoutDescription);

		/** Creates a thread safe storage container. */
		var createThreadSafeStorage();

		/** Creates a reference to the script license manager. */
		var createLicenseUnlocker();

		/** Creates a beatport manager object. */
		var createBeatportManager();

		/** Renders a MIDI event list as audio data on a background thread and calls a function when it's ready. */
		void renderAudio(var eventList, var finishCallback);

		/** Previews a audio buffer with a callback indicating the state. */
		void playBuffer(var bufferData, var callback, double fileSampleRate);

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

		/** Sets the global pitch factor (in semitones). */
		void setGlobalPitchFactor(double pitchFactorInSemitones);

		/** Returns the global pitch factor (in semitones). */
		double getGlobalPitchFactor() const;

		/** Changes the lowest visible key on the on screen keyboard. */
		void setLowestKeyToDisplay(int keyNumber);

		/** Shows a error message on the compiled plugin (or prints it on the console). Use isCritical if you want to disable the "Ignore" Button. */
		void showErrorMessage(String message, bool isCritical);

		/** Shows a message with an overlay on the compiled plugin with an "OK" button in order to notify the user about important events. */
		void showMessage(String message);

		/** Shows a message box with an OK button and a icon defined by the type variable. */
		void showMessageBox(String title, String markdownMessage, int type);

		/** Returns the millisecond value for the supplied tempo (HINT: Use "TempoSync" mode from Slider!) */
		double getMilliSecondsForTempo(int tempoIndex) const;;

		/** launches the given URL in the system's web browser. */
		void openWebsite(String url);

		/** Copies the given text to the clipboard. */
		void copyToClipboard(String textToCopy);

		/** Returns the clipboard content. */
		String getClipboardContent();

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
		void saveUserPreset(var presetName);

		/** Sorts an array with a given comparison function. */
		bool sortWithFunction(var arrayToSort, var sortFunction);

		/** Loads a user preset with the given relative path  (use `/` for directory separation) or the given ScriptFile object. */
		void loadUserPreset(var relativePathOrFileObject);

		/** Sets the tags that appear in the user preset browser. */
		void setUserPresetTagList(var listOfTags);

		/** Returns a list of all available user presets as relative path. */
		var getUserPresetList() const;

		/** Checks if the user preset is read only. */
		bool isUserPresetReadOnly(var optionalFile);

		/** Sets whether the samples are allowed to be duplicated. Set this to false if you operate on the same samples differently. */
		void setAllowDuplicateSamples(bool shouldAllow);

		/** Calling this makes sure that all audio files are loaded into the pool and will be available in the compiled plugin. Returns a list of all references. */
		var loadAudioFilesIntoPool();

		/** Loads a file and returns its content as array of Buffers. */
		var loadAudioFileIntoBufferArray(String audioFileReference);

		/** Returns the list of wavetables of the current expansion (or factory content). */
		var getWavetableList();

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

		/** Returns the current operating system ("OSX", "LINUX", or ("WIN"). */
		String getOS();
		
		/** Returns info about the current hardware and OS configuration. */
		var getSystemStats();
				
		/** Returns the mobile device that this software is running on. */
		String getDeviceType();

		/** Returns the full screen resolution for the current device. */
		var getDeviceResolution();

		/** Returns true if running as VST / AU / AAX plugin. */
		bool isPlugin() const;

		/** Returns true if the project is running inside HISE. You can use this during development to simulate different environments. */
		bool isHISE();

		/** Forces a full (asynchronous) reload of all samples (eg. after the sample directory has changed). */
		void reloadAllSamples();

		/** Returns the preload progress from 0.0 to 1.0. Use this to display some kind of loading icon. */
		double getPreloadProgress();

		/** Returns the current preload message if there is one. */
		String getPreloadMessage();

		/** Returns the current Zoom Level. */
		var getZoomLevel() const;

		/** Sets the new zoom level (1.0 = 100%) */
		void setZoomLevel(double newLevel);

		/** Sets the Streaming Mode (0 -> Fast-SSD, 1 -> Slow-HDD) */
		void setDiskMode(int mode);

		/** Returns an object that contains all filter modes. */
		var getFilterModeList() const;

		/** Returns the product version (not the HISE version!). */
    String getVersion();

    /** Returns the product name (not the HISE name!). */
    String getName();

		/** Returns project and company info from the Project's preferences. */
		var getProjectInfo();

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

		/** Creates a unordered stack that can hold up to 128 float numbers. */
		ScriptingObjects::ScriptUnorderedStack* createUnorderedStack();

		/** Creates a SliderPack Data object and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptSliderPackData* createAndRegisterSliderPackData(int index);

		/** Creates a Table object and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptTableData* createAndRegisterTableData(int index);

		/** Creates a audio file holder and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptAudioFile* createAndRegisterAudioFile(int index);

		/** Creates a ring buffer and registers it so you can access it from other modules. */
		ScriptingObjects::ScriptRingBuffer* createAndRegisterRingBuffer(int index);

		/** Creates a new timer object. */
		ScriptingObjects::TimerObject* createTimerObject();

		/** Creates a storage object for Message events. */
		ScriptingObjects::ScriptingMessageHolder* createMessageHolder();

		/** Creates a neural network with the given ID. */
		ScriptingObjects::ScriptNeuralNetwork* createNeuralNetwork(String id);

		/** Creates an object that can listen to transport events. */
		var createTransportHandler();

		/** Creates a modulation matrix object that handles dynamic modulation using the given Global Modulator Container as source. */
		var createModulationMatrix(String containerId);

		/** Creates a macro handler that lets you programmatically change the macro connections. */
		var createMacroHandler();

		/** Exports an object as JSON. */
		void dumpAsJSON(var object, String fileName);

		/** Compresses a JSON object as Base64 string using zstd. */
		String compressJSON(var object);

		/** Expands a compressed JSON object. */
		var uncompressJSON(const String& b64);

		/** Imports a JSON file as object. */
		var loadFromJSON(String fileName);

		/** Displays the progress (0.0 to 1.0) in the progress bar of the editor. */
		void setCompileProgress(var progress);

		/** Matches the string against the regex token. */
		bool matchesRegex(String stringToMatch, String regex);

		/** Creates an error handler that reacts on initialisation errors. */
		var createErrorHandler();

		/** Returns an array with all matches. */
		var getRegexMatches(String stringToMatch, String regex);

		/** Returns a string of the value with the supplied number of digits. */
		String doubleToString(double value, int digits);
		
		/** Returns the width of the string for the given font properties. */
		float getStringWidth(String text, String fontName, float fontSize, float fontSpacing);

		String intToHexString(int value);

		/** Signals that the application should terminate. */
		void quit();

		/** Reverts the last controller change. */
		void undo();

		/** Redo the last controller change. */
		void redo();

        /** Clears the undo history. */
        void clearUndoHistory();
        
		/** Returns a fully described string of this date and time in ISO-8601 format (using the local timezone) with or without divider characters. */
		String getSystemTime(bool includeDividerCharacters);
		
		// ============================================================================================================


		/** This warning will show up in the console so people can migrate in the next years... */
		void logSettingWarning(const String& methodName) const;

		struct Wrapper;

		double unused = 0.0;

		ScriptBaseMidiProcessor* parentMidiProcessor;

		ScopedPointer<Thread> currentExportThread;

		struct PreviewHandler;

		ScopedPointer<PreviewHandler> previewHandler;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine);
	};

	/** This class takes over a few of the Engine methods in order to break down this gigantomanic object. */
	class Date : public ApiClass,
				   public ScriptingObject
	{
	public:

		Date(ProcessorWithScriptingContent* s);
		~Date() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Date"); }

		// ================================================================================================== API Calls

		/** Returns a fully described string of this date and time in milliseconds or ISO-8601 format (using the local timezone) with or without divider characters. */
		String getSystemTimeISO8601(bool includeDividerCharacters);
		
		/** Returns the system time in milliseconds. */
		int64 getSystemTimeMs();
		
		/** Returns a time in milliseconds to a date string. */
		String millisecondsToISO8601(int64 miliseconds, bool includeDividerCharacters);
		
		/** Returns a date string to time in milliseconds. */
		int64 ISO8601ToMilliseconds(String iso8601);
		

		struct Wrapper;
	};


	/** This class takes over a few of the Engine methods in order to break down this gigantomanic object. */
	class Settings : public ApiClass,
					 public ScriptingObject
	{
	public:

		Settings(ProcessorWithScriptingContent* s);;

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Settings"); }

		// ================================================================================================== API Calls

		/** Returns the UI Zoom factor. */
		double getZoomLevel() const;

		/** Changes the UI zoom (1.0 = 100%). */
		void setZoomLevel(double newLevel);

		/** Gets the Streaming Mode (0 -> Fast-SSD, 1 -> Slow-HDD) */
		int getDiskMode();

		/** Sets the Streaming Mode (0 -> Fast-SSD, 1 -> Slow-HDD) */
		void setDiskMode(int mode);

		/** Returns available audio device types. */
		var getAvailableDeviceTypes();
		
		/** Returns the current audio device type. */
		String getCurrentAudioDeviceType();
		
		/** Sets the current audio device type*/
		void setAudioDeviceType(String deviceName);

		/** Returns names of available audio devices. */
		var getAvailableDeviceNames();

		/** Gets the current audio device name*/
		String getCurrentAudioDevice();
				
		/** Sets the current audio device */
		void setAudioDevice(String name);
		
		/** Returns array of available output channel pairs. */
		var getAvailableOutputChannels();

		/** Returns current output channel pair. */
		int getCurrentOutputChannel();
		
		/** Sets the output channel pair */
		void setOutputChannel(int index);
		
		/** Returns available buffer sizes for the selected audio device. */
		var getAvailableBufferSizes();
		
		/** Returns the current buffer block size. */
		int getCurrentBufferSize();
		
		/** Sets the buffer block size for the selected audio device. */
		void setBufferSize(int newBlockSize);
		
		/** Returns array of available sample rate. */
		var getAvailableSampleRates();

		/** Returns the current output sample rate (-1 if no audio device selected)*/
		double getCurrentSampleRate();
				
		/** Sets the output sample rate */
		void setSampleRate(double sampleRate);
		
		/** Returns current voice amount multiplier setting. */
		int getCurrentVoiceMultiplier();

		/** Sets the voice limit multiplier (1, 2, 4, or 8). */
		void setVoiceMultiplier(int newVoiceAmount);

		/** Clears all MIDI CC assignments. */
		void clearMidiLearn();

		/** Returns array of MIDI input device names. */
		var getMidiInputDevices();
		
		/** Enables or disables named MIDI input device. */
		void toggleMidiInput(const String &midiInputName, bool enableInput);

		/** Returns enabled state of midi input device. */
		bool isMidiInputEnabled(const String &midiInputName);
		
		/** Enables or disables MIDI channel (0 = All channels). */
		void toggleMidiChannel(int index, bool value);
		
		/** Returns enabled state of midi channel (0 = All channels). */
		bool isMidiChannelEnabled(int index);

		/** Returns an array of the form [width, height]. */
		var getUserDesktopSize();

		/** Returns whether OpenGL is enabled or not. The return value might be out of sync with the actual state (after you changed this setting until the next reload). */
		bool isOpenGLEnabled() const;

		/** Enable OpenGL. This setting will be applied the next time the interface is rebuild. */
		void setEnableOpenGL(bool shouldBeEnabled);
		
		/** Enables or disables debug logging */
		void setEnableDebugMode(bool shouldBeEnabled);

		/** Changes the sample folder. */
		void setSampleFolder(var sampleFolder);

		/** Starts the perfetto profile recording. */
		void startPerfettoTracing();

		/** Stops the perfetto profile recording and dumps the data to the given file. */
		void stopPerfettoTracing(var traceFileToUse);

		/** Calls abort to terminate the program. You can use this to check your crash reporting workflow. */
		void crashAndBurn();

		// ============================================================================================================

	private:

		

		GlobalSettingManager* gm;
		AudioProcessorDriver* driver;
		MainController* mc;

		struct Wrapper;
	};

	/** All scripting functions for sampler specific functionality. */
	class Sampler : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler);
		~Sampler() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Sampler"); }

		Identifier getObjectName() const override { return getClassName(); }
		bool objectDeleted() const override { return sampler.get() == nullptr; }
		bool objectExists() const override { return sampler.get() != nullptr; }

		// ============================================================================================================ API Methods

		/** Enables / Disables the automatic round robin group start logic (works only on samplers). */
		void enableRoundRobin(bool shouldUseRoundRobin);

		/** Enables the group with the given index (one-based). Works only with samplers and `enableRoundRobin(false)`. */
		void setActiveGroup(int activeGroupIndex);

		/** Enables the group with the given index (one-based) for the given event ID. Works only with samplers and `enableRoundRobin(false)`. */
		void setActiveGroupForEventId(int eventId, int activeGroupIndex);

		/** Enables the group with the given index (one-based). Allows multiple groups to be active. */
		void setMultiGroupIndex(var groupIndex, bool enabled);

		/** Enables the group with the given index (one-based) for the given event id. Allows multiple groups to be active. */
		void setMultiGroupIndexForEventId(int eventId, var groupIndex, bool enabled);

		/** Sets the volume of a particular group (use -1 for active group). Only works with disabled crossfade tables. */
		void setRRGroupVolume(int groupIndex, int gainInDecibels);

		/** Returns the currently (single) active RR group. */
		int getActiveRRGroup();

		/** Returns the RR group that is associated with the event ID. */
		int getActiveRRGroupForEventId(int eventId);

		/** Returns the number of currently active groups. */
		int getNumActiveGroups() const;

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

		/** Purges the array of sampler sounds (and unpurges the rest). */
		void purgeSampleSelection(var selection);

		/** Returns the name of the channel with the given index (Multimic samples only. */
		String getMicPositionName(int channelIndex);

		/** Returns an array with all samples that match this regex. */
		var createSelection(String regex);

		/** Returns an array with all samples from the index data (can be either int or array of int, -1 selects all.). */
		var createSelectionFromIndexes(var indexData);

		/** Returns an array with all samples that match the filter function. */
		var createSelectionWithFilter(var filterFunction);

		/** Returns a list of the sounds selected by the selectSounds() method. */
		var createListFromScriptSelection();

		/** Returns a list of the sounds selected in the samplemap. */
		var createListFromGUISelection();

		/** Sets the currently selected samples on the interface to the given list. */
		void setGUISelection(var sampleList, bool addToSelection);

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

		/** Loads a samplemap from a list of JSON objects. */
		void loadSampleMapFromJSON(var jsonSampleMap);

		/** Loads a base64 compressed string with the samplemap. */
		void loadSampleMapFromBase64(const String& b64);

		/** Returns a base64 compressed string containing the entire samplemap. */
		String getSampleMapAsBase64();

		/** Creates a JSON object from the sample file that can be used with loadSampleMapFromJSON. */
		var parseSampleFile(var sampleFile);

		/** Sets the timestretch ratio for the sampler depending on its timestretch mode. */
		void setTimestretchRatio(double newRatio);

		/** Returns the current timestretching options as JSON object. */
		var getTimestretchOptions();

		/** Sets the timestretching options from a JSON object. */
		void setTimestretchOptions(var newOptions);

		/** Returns the current release start options as JSON object. */
		var getReleaseStartOptions();

		/** Sets the options for the release start behaviour. */
		void setReleaseStartOptions(var newOptions);

		/** Converts the user preset data of a audio waveform to a base 64 samplemap. */
		String getAudioWaveformContentAsBase64(var presetObj);

		/** Loads an SFZ file into the sampler. */
		var loadSfzFile(var sfzFile);

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

		/** Returns the index of the attribute with the given ID. */
		int getAttributeIndex(String id);

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

		ValueTree convertJSONListToValueTree(var jsonSampleList);

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

		Synth(ProcessorWithScriptingContent *p, Message* messageObject, ModulatorSynth *ownerSynth);
		~Synth() {}

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Synth"); };

		typedef ScriptingObjects::ScriptingModulator ScriptModulator;
		typedef ScriptingObjects::ScriptingEffect ScriptEffect;
		typedef ScriptingObjects::ScriptingMidiProcessor ScriptMidiProcessor;
		typedef ScriptingObjects::ScriptingSynth ScriptSynth;
		typedef ScriptingObjects::ScriptingAudioSampleProcessor ScriptAudioSampleProcessor;
		typedef ScriptingObjects::ScriptingTableProcessor ScriptTableProcessor;
		typedef ScriptingObjects::ScriptSliderPackProcessor ScriptSliderPackProcessor;
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

        /** Injects a note on to the incoming MIDI buffer (just as if the virtual keyboard was pressed. */
        void playNoteFromUI(int channel, int noteNumber, int velocity);
        
        /** Injects a note off to the incoming MIDI buffer (similar to playNoteFromUI). */
        void noteOffFromUI(int channel, int noteNumber);
        
		/** Plays a note and returns the event id. Be careful or you get stuck notes! */
		int playNote(int noteNumber, int velocity);

		/** Plays a note and returns the event id with the given channel and start offset. */
		int playNoteWithStartOffset(int channel, int number, int velocity, int offset);

		/** Attaches an artificial note to be stopped when the original note is stopped. */
		bool attachNote(int originalNoteId, int artificialNoteId);

		/** Adds a few additional safe checks to prevent stuck notes from note offs being processed before their note-on message. */
		void setFixNoteOnAfterNoteOff(bool shouldBeFixed);

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

		/** Creates a Builder object that can be used to create the module tree. */
		var createBuilder();

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

		/** Checks if the artificial event is active */
		bool isArtificialEventActive(int eventId);

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

		/** Returns the sliderpack processor with the given name. */
		ScriptSliderPackProcessor* getSliderPackProcessor(const String& name);

		/** Returns a reference to a processor that holds a display buffer. */
		ScriptingObjects::ScriptDisplayBufferSource* getDisplayBufferSource(const String& name);

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

		/** Use a uniform voice index for the given container. */
		void setUseUniformVoiceHandler(String containerId, bool shouldUseUniformVoiceHandling);

		// ============================================================================================================

		void handleNoteCounter(const HiseEvent& e) noexcept
		{
			if (e.isArtificial())
				return;

			if (e.isNoteOn())
			{
				++numPressedKeys;
				keyDown.setBit(e.getNoteNumber(), true);
			}
			else if (e.isNoteOff())
			{
				--numPressedKeys; 
				if (numPressedKeys.get() < 0) 
					numPressedKeys.set(0);

				keyDown.setBit(e.getNoteNumber(), false);
			}
			else if (e.isAllNotesOff())
			{
				numPressedKeys = 0;
				keyDown.clear();
			}
		}

		void setSustainPedal(bool shouldBeDown) { sustainState = shouldBeDown; };

		struct Wrapper;

	private:

		int internalAddNoteOn(int channel, int noteNumber, int velocity, int timestamp, int startOffset);

		friend class ModuleHandler;

		WeakReference<Message> messageObject;

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
		void startBenchmark() { startTime = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()); };

		/** Stops the benchmark and prints the result. */
		void stopBenchmark();

		/** Causes the execution to stop(). */
		void stop(bool condition);

		/** Sends a blink message to the current editor. */
		void blink();

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

		/** Throws an error message if the value is a string. */
		void assertNoString(var value);

		/** Throws an error message if the value is not a legal number (eg. string or array or infinity or NaN). */
		void assertLegalNumber(var value);

		/** Throws an assertion in the attached debugger. */
		void breakInDebugger();

		struct Wrapper;

		void setDebugLocation(const Identifier& id_, int lineNumber_)
		{
			id = id_;
			lineNumber = lineNumber_;
		}

private:

		Identifier id;
		int lineNumber;

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

	class TransportHandler : public ConstScriptingObject,
							 public TempoListener,
							 public ControlledObject,
							 public PooledUIUpdater::Listener
	{
	public:

		

		TransportHandler(ProcessorWithScriptingContent* sp);;
		~TransportHandler();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("TransportHandler"); }
		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("TransportHandler"); };

		struct Callback: public PooledUIUpdater::Broadcaster
		{
			Callback(TransportHandler* p, const String& name, const var& f, bool sync, int numArgs);

			void call(var arg1, var arg2 = {}, var arg3 = {}, bool forceSynchronous = false);


			void callAsync();

			bool matches(const var& f) const;

		private:

			void callSync();

			const int numArgs;
			var args[3];

			JavascriptProcessor* jp;
			WeakReference<TransportHandler> th;
			const bool synchronous = false;
			WeakCallbackHolder callback;
		};

		// ======================================================================================

		/** Registers a callback to tempo changes. */
		void setOnTempoChange(var sync, var f);

		/** Registers a callback to transport state changes (playing / stopping). */
		void setOnTransportChange(var sync, var f);

		/** Registers a callback to time signature changes. */
		void setOnSignatureChange(var sync, var f);

		/** Registers a callback to changes in the musical position (bars / beats). */
		void setOnBeatChange(var sync, var f);

		/** Registers a callback to changes in the grid. */
		void setOnGridChange(var sync, var f);

		/** Registers a callback that will be executed asynchronously when the plugin's bypass state changes. */
		void setOnBypass(var f);

		/** Enables a high precision grid timer. */
		void setEnableGrid(bool shouldBeEnabled, int tempoFactor);

        /** Sets the internal clock to stop when the external clock was stopped. */
        void stopInternalClockOnExternalStop(bool shouldStop);
        
		/** Starts the internal master clock. */
		void startInternalClock(int timestamp);

		/** Stops the internal master clock. */
		void stopInternalClock(int timestamp);

		/** Sets the sync mode for the global clock. */
		void setSyncMode(int syncMode);

		/** sends a message on the next grid callback to resync the external clock. */
		void sendGridSyncOnNextCallback();

		/** If enabled, this will link the internal / external BPM to the sync mode. */
		void setLinkBpmToSyncMode(bool shouldPrefer);

		/** This will return true if the DAW is currently bouncing the audio to a file. You can use this in the transport change callback to modify your processing chain. */
		bool isNonRealtime() const;

	private:

		static void onBypassUpdate(TransportHandler& handler, bool state);

		void clearIf(ScopedPointer<Callback>& cb, const var& f)
		{
			if (cb != nullptr && cb->matches(f))
				cb = nullptr;
		}

		double bpm = 120.0;
		bool play = false;
		int nom = 4;
		int denom = 4;
		int beat = 0;
		bool newBar = true;
		int gridIndex = 0;
		int gridTimestamp = 0;
		bool firstGridInPlayback = false;

		struct Wrapper;

		ScopedPointer<Callback> tempoChangeCallback;
		ScopedPointer<Callback> transportChangeCallback;
		ScopedPointer<Callback> timeSignatureCallback;
		ScopedPointer<Callback> beatCallback;
		ScopedPointer<Callback> gridCallback;

		ScopedPointer<Callback> bypassCallback;

		ScopedPointer<Callback> tempoChangeCallbackAsync;
		ScopedPointer<Callback> transportChangeCallbackAsync;
		ScopedPointer<Callback> timeSignatureCallbackAsync;
		ScopedPointer<Callback> beatCallbackAsync;
		ScopedPointer<Callback> gridCallbackAsync;
		

		void tempoChanged(double newTempo) override;

		void onTransportChange(bool isPlaying, double ppqPosition) override;

		void onBeatChange(int newBeat, bool isNewBar) override;

		void onSignatureChange(int newNominator, int numDenominator) override;

		void onGridChange(int gridIndex_, uint16 timestamp, bool firstGridInPlayback_) override;

		void handlePooledMessage(PooledUIUpdater::Broadcaster* b) override
		{
			if (auto asC = dynamic_cast<Callback*>(b))
				asC->callAsync();
		}

		JUCE_DECLARE_WEAK_REFERENCEABLE(TransportHandler);
	};

	class Server : public ApiClass,
				   public ScriptingObject,
				   public GlobalServer::Listener
	{
	public:

		using WeakPtr = WeakReference<Server>;

		enum StatusCodes
		{
			StatusNoConnection = 0,
			StatusOK = 200,
			StatusNotFound = 404,
			StatusServerError = 500,
			StatusAuthenticationFail = 403,
			numStatusCodes
		};

		Server(JavascriptProcessor* jp);

		~Server()
		{
			globalServer.removeListener(this);
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

        /** Resends the last call to the Server (eg. in case that there was no internet connection). */
        bool resendLastCall();
        
		/** Downloads a file to the given target and returns a Download object. */
		var downloadFile(String subURL, var parameters, var targetFile, var callback);

        /** Sets a string that is parsed as timeout message when the server doesn't respond. Default is "{}" (empty JSON object). */
        void setTimeoutMessageString(String timeoutMessage);
        
        /** Sets whether to append a trailing slash to each POST call (default is true). */
        void setEnforceTrailingSlash(bool shouldAddSlash);
        
		/** Returns a list of all pending Downloads. */
		var getPendingDownloads();

		/** Returns a list of all pending Calls. */
		var getPendingCalls();

		/** Sets the maximal number of parallel downloads. */
		void setNumAllowedDownloads(int maxNumberOfParallelDownloads);

		/** Returns true if the system is connected to the internet. */
		bool isOnline();
		
		/** Removes all finished downloads from the list. */
		void cleanFinishedDownloads();

		/** This function will be called whenever there is server activity. */
		void setServerCallback(var callback);

		/** Checks if given email address is valid - not fool proof. */
    bool isEmailAddress(String email);
		
		void queueChanged(int numItems) override
		{
			if (serverCallback)
			{
				if(numItems < 2)
					serverCallback.call1(numItems == 1);
			}
		}

		void downloadQueueChanged(int) override
		{

		}

		juce::URL getWithParameters(String subURL, var parameters)
		{
			return globalServer.getWithParameters(subURL, parameters);
		}

	private:

		GlobalServer& globalServer;

		WeakCallbackHolder serverCallback;

		JavascriptProcessor* jp;

		struct Wrapper;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Server);
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
			Samples,
			UserPresets,
			AppData,
			UserHome,
			Documents,
			Desktop,
			Downloads,
			Applications,
			Temp,
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

		/** Returns a file object from an absolute path (eg. C:/Windows/MyProgram.exe). */
		var fromAbsolutePath(String path);

		/** Returns a file object for the given location type and the reference string which can either contain a wildcard like `{PROJECT_FOLDER}` or a full file path. */
		var fromReferenceString(String referenceStringOrFullPath, var locationType);

		/** Returns a list of all child files of a directory that match the wildcard. */
		var findFiles(var directory, String wildcard, bool recursive);

        /** Returns a list of all root drives of the current computer. */
        var findFileSystemRoots();
        
		/** Opens a file browser to choose a file. */
		void browse(var startFolder, bool forSaving, String wildcard, var callback);

		/** Opens a file browser to choose a directory. */
		void browseForDirectory(var startFolder, var callback);

		/** Returns a unique machine ID that can be used to identify the computer. */
		String getSystemId();
		
		/**  Convert a file size in bytes to a neat string description. */
		String descriptionOfSizeInBytes(int64 bytes);

		/** Returns the number of free bytes on the volume of a given folder. */
		int64 getBytesFreeOnVolume(var folder);

        /** Encrypts the given string using a RSA private key. */
        String encryptWithRSA(const String& dataToEncrypt, const String& privateKey);
        
        /** Decrypts the given string using a RSA public key. */
        String decryptWithRSA(const String& dataToDecrypt, const String& publicKey);

		/** Loads a bunch of dummy assets (audio files, MIDI files, filmstrips) for use in snippets & examples. */
		void loadExampleAssets();

		// ========================================================= End of API calls

		ProcessorWithScriptingContent* p;

	private:

		void browseInternally(File startFolder, bool forSaving, bool isDirectory, String wildcard, var callback);

		File getFile(SpecialLocations l);

		FileHandlerBase::SubDirectories getSubdirectory(var locationType);

		struct Wrapper;


	};

    class Threads: public ApiClass,
				   public ScriptingObject
    {
    public:

        Threads(ProcessorWithScriptingContent* p);

        Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Threads"); }

		// API METHODS ===============================================================================

		/** Returns the thread ID of the thread that is calling this method. */
        int getCurrentThread() const;

		/** Returns true if the audio callback is running or false if it's suspended during a load operation. */
        bool isAudioRunning() const;

		/** Returns true if the audio exporter is currently rendering the audio on a background thread. */
		bool isCurrentlyExporting() const;

		/** Returns true if the given thread is currently locked by the current thread. */
        bool isLockedByCurrentThread(int thread) const;

		/** Returns the thread ID of the thread the locks the given thread ID. */
        int getLockerThread(int threadThatIsLocked) const;

		/** Returns true if the given thread is currently locked. */
        bool isLocked(int thread) const;

		/** Returns the name of the given string (for debugging purposes only!). */
		String toString(int thread) const;

		/** Returns the name of the current thread (for debugging purposes only!). */
        String getCurrentThreadName() const
        {
			return toString(getCurrentThread());
        }

        /** Kills all voices, suspends the audio processing and calls the given function on the loading thread. Returns true if the function was executed synchronously. */
        bool killVoicesAndCall(const var& functionToExecute);

    private:

		using TargetThreadId = MainController::KillStateHandler::TargetThread;
		using LockId = LockHelpers::Type;

        static TargetThreadId getAsThreadId(int x);
        static LockId getAsLockId(int x);

        MainController::KillStateHandler& getKillStateHandler();
        const MainController::KillStateHandler& getKillStateHandler() const;

        struct Wrapper;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Threads);
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
		int withAlpha(var colour, float alpha);

		/** Returns a colour with the specified hue. */
		int withHue(var colour, float hue);

		/** Returns a colour with the specified saturation. */
		int withSaturation(var colour, float saturation);

		/** Returns a colour with the specified brightness. */
		int withBrightness(var colour, float brightness);

		/** Returns a colour with a multiplied alpha value. */
		int withMultipliedAlpha(var colour, float factor);

		/** Returns a colour with a multiplied saturation value. */
		int withMultipliedSaturation(var colour, float factor);

		/** Returns a colour with a multiplied brightness value. */
		int withMultipliedBrightness(var colour, float factor);

		/** Converts a colour to a [r, g, b, a] array that can be passed to GLSL as vec4. */
		var toVec4(var colour);

		/** Converts a colour from a [r, g, b, a] float array to a uint32 value. */
		int fromVec4(var vec4);

		/** Linear interpolation between two colours. */
		int mix(var colour1, var colour2, float alpha);

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
			BACKEND_ONLY(mod->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Intensity, dispatch::sendNotificationAsync););
		}

		/** Bypasses the modulator. */
		void setBypassed(var newValue)
		{
			mod->setBypassed((bool)newValue);
			BACKEND_ONLY(mod->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Bypassed, dispatch::sendNotificationAsync););
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
