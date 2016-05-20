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

#ifndef MAINCONTROLLER_H_INCLUDED
#define MAINCONTROLLER_H_INCLUDED

// ====================================================================================================
// Extern class definitions

class PluginParameterModulator;
class PluginParameterAudioProcessor;
class ControlledObject;
class Processor;
class Console;
class ModulatorSamplerSound;
class ModulatorSamplerSoundPool;
class AudioSampleBufferPool;
class Plotter;
class ScriptWatchTable;
class ScriptComponentEditPanel;
class ScriptProcessor;
class Modulator;
class CustomKeyboardState;
class ModulatorSynthChain;
class FactoryType;

// ====================================================================================================
// Macro definitions

#if USE_BACKEND

#define debugMod(text) { if(consoleEnabled) debugProcessor(text); };
#else
#define debugMod(text) {;};
#endif

#define USE_MIDI_CONTROLLERS_FOR_MACROS 0


class MainController;



/** This handles the MIDI automation for the frontend plugin.
*
*	For faster performance, one CC value can only control one parameter.
*
*/
class MidiControllerAutomationHandler: public RestorableObject
{
public:

	MidiControllerAutomationHandler(MainController *mc_) :
		anyUsed(false),
		mc(mc_)
	{
		tempBuffer.ensureSize(2048);

		clear();
	}

	void addMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex, NormalisableRange<double> parameterRange, int macroIndex)
	{
		ScopedLock sl(lock);

		unlearnedData.processor = interfaceProcessor;
		unlearnedData.attribute = attributeIndex;
		unlearnedData.parameterRange = parameterRange;
		unlearnedData.macroIndex = macroIndex;
		unlearnedData.used = true;
	}

	bool isLearningActive() const
	{
		ScopedLock sl(lock);

		return unlearnedData.used;
	}

	ValueTree exportAsValueTree() const override;

	void restoreFromValueTree(const ValueTree &v) override;

	bool isLearningActive(Processor *interfaceProcessor, int attributeIndex) const
	{
		return unlearnedData.processor == interfaceProcessor && unlearnedData.attribute == attributeIndex;
	}

	void deactivateMidiLearning()
	{
		ScopedLock sl(lock);

		unlearnedData = AutomationData();
	}
	
	void setUnlearndedMidiControlNumber(int ccNumber)
	{
		ScopedLock sl(lock);

		jassert(isLearningActive());

		automationData[ccNumber] = unlearnedData;

		unlearnedData = AutomationData();

		anyUsed = true;
	}

	int getMidiControllerNumber(Processor *interfaceProcessor, int attributeIndex) const
	{
		for (int i = 0; i < 128; i++)
		{
			const AutomationData *a = automationData + i;

			if (a->processor == interfaceProcessor && a->attribute == attributeIndex)
			{
				return i;
			}
		}

		return -1;
	}

	void refreshAnyUsedState()
	{
		ScopedLock sl(lock);

		anyUsed = false;

		for (int i = 0; i < 128; i++)
		{
			AutomationData *a = automationData + i;

			if (a->used)
			{
				anyUsed = true;
				break;
			}
		}
	}

	void clear()
	{
		for (int i = 0; i < 128; i++)
		{
			automationData[i] = AutomationData();
		};
	}

	void removeMidiControlledParameter(Processor *interfaceProcessor, int attributeIndex)
	{
		ScopedLock sl(lock);

		for (int i = 0; i < 128; i++)
		{
			AutomationData *a = automationData + i;

			if (a->processor == interfaceProcessor && a->attribute == attributeIndex)
			{
				*a = AutomationData();
				break;
			}
		}

		refreshAnyUsedState();
	}

	void handleParameterData(MidiBuffer &b);

	struct AutomationData
	{
		AutomationData():
			processor(nullptr),
			attribute(-1),
			parameterRange(NormalisableRange<double>()),
			macroIndex(-1),
			used(false)
		{}

		WeakReference<Processor> processor;
		int attribute;
		NormalisableRange<double> parameterRange;
		int macroIndex;
		bool used;
	};


private:

	CriticalSection lock;

	MainController *mc;

	bool anyUsed;

	MidiBuffer tempBuffer;

	AutomationData automationData[128];
	
	AutomationData unlearnedData;

};


#define GET_PROJECT_HANDLER(x)(x->getMainController()->getSampleManager().getProjectHandler())

/** A class for handling application wide tasks.
*	@ingroup core
*
*	Every Processor must be connected to a MainController instance and has access to its public methods.
*
*/
class MainController: public GlobalScriptCompileBroadcaster,
					  public ThreadWithQuasiModalProgressWindow::Holder
{
public:

	/** Contains all methods related to sample management. */
	class SampleManager
	{
	public:

		SampleManager(MainController *mc);

		void setLoadedSampleMaps(ValueTree &v) { sampleMaps = v; };

		/** returns a pointer to the thread pool that streams the samples from disk. */
		SampleThreadPool *getGlobalSampleThreadPool() { return samplerLoaderThreadPool; }

		/** returns a pointer to the global sample pool */
		ModulatorSamplerSoundPool *getModulatorSamplerSoundPool() const { return globalSamplerSoundPool; }

		/** Copies the samples to an internal clipboard for copy & paste functionality. */
		void copySamplesToClipboard(const Array<WeakReference<ModulatorSamplerSound>> &soundsToCopy);

		const ValueTree &getSamplesFromClipboard() const;

		bool clipBoardContainsData() const { return sampleClipboard.getNumChildren() != 0;}

		/** returns the fixed streaming buffer size. */
		int getStreamingBufferSize() const { return 4096; };

		/** This saves all referenced samples to a subfolder in the global sample folder with the name of the main synth chain. */
		void saveAllSamplesToGlobalFolder(const String &folderName);

		/** Returns the ValueTree that represents the samplemap with the specified file name.
		*
		*	This is used when a sample map is loaded - it checks if the name already exists in the loaded monolithic data
		*	and loads the sounds from there if there is a match. 
		*/
		const ValueTree getLoadedSampleMap(const String &fileName) const
		{
			for(int i = 0; i < sampleMaps.getNumChildren(); i++)
			{
				String childFileName = sampleMaps.getChild(i).getProperty("SampleMapIdentifier", String::empty);
				if(childFileName == fileName) return sampleMaps.getChild(i);
			}

			return ValueTree::invalid;
		}

		bool shouldUseRelativePathToProjectFolder() const { return useRelativePathsToProjectFolder; }

		void setShouldUseRelativePathToProjectFolder(bool shouldUse) { useRelativePathsToProjectFolder = shouldUse; }

		/** Returns the impulse response pool. */
		const AudioSampleBufferPool *getAudioSampleBufferPool() const {	return globalAudioSampleBufferPool; };

		/** Returns the impulse response pool. */
		AudioSampleBufferPool *getAudioSampleBufferPool() {	return globalAudioSampleBufferPool; };

		ImagePool *getImagePool() {return globalImagePool;};

		const ImagePool *getImagePool() const {return globalImagePool;};

		ProjectHandler &getProjectHandler() { return projectHandler; }

		const ProjectHandler &getProjectHandler() const { return projectHandler; }

	private:

		ProjectHandler projectHandler;

		ValueTree sampleClipboard;
		ValueTree sampleMaps;
		ScopedPointer<AudioSampleBufferPool> globalAudioSampleBufferPool;
		ScopedPointer<ImagePool> globalImagePool;
		ScopedPointer<ModulatorSamplerSoundPool> globalSamplerSoundPool;
		ScopedPointer<SampleThreadPool> samplerLoaderThreadPool;

		bool useRelativePathsToProjectFolder;

	};

	/** Contains methods for handling macros. */
	class MacroManager
	{
	public:

		MacroManager(MainController *mc_);

		ModulatorSynthChain *getMacroChain() { return macroChain; };

		const ModulatorSynthChain *getMacroChain() const {return macroChain; };

		void setMacroChain(ModulatorSynthChain *chain) { macroChain = chain; }

		bool midiMacroControlActive() const
		{
			for(int i = 0; i < 8; i++)
			{
				if(macroControllerNumbers[i] != -1) return true;
			}

			return false;
		}

		void setMidiControllerForMacro(int midiControllerNumber);

		void setMidiControllerForMacro(int macroIndex, int midiControllerNumber)
		{
			if( macroIndex < 8)
			{
				macroControllerNumbers[macroIndex] = midiControllerNumber;
			}
		};

		void setMacroControlMidiLearnMode(ModulatorSynthChain *chain, int index)
		{
			macroChain = chain;
			macroIndexForCurrentMidiLearnMode = index;
		}

		int getMacroControlForMidiController(int midiController)
		{
			for(int i = 0; i < 8; i++)
			{
				if(macroControllerNumbers[i] == midiController) return i;
			}

			return -1;
		}

		int getMidiControllerForMacro(int macroIndex)
		{
			if(macroIndex < 8)
			{
				return macroControllerNumbers[macroIndex];
			}
			else
			{
				return -1;
			}
		}

		bool midiControlActiveForMacro(int macroIndex) const
		{
			if(macroIndex < 8)
			{
				return macroControllerNumbers[macroIndex] != -1;
			}
			else
			{
				jassertfalse;
				return false;
			}
		};

		void removeMidiController(int macroIndex)
		{
			if(macroIndex < 8)
			{
				macroControllerNumbers[macroIndex] = -1;
			}
		}

		bool macroControlMidiLearnModeActive() { return macroIndexForCurrentMidiLearnMode != -1; }

		void setMacroControlLearnMode(ModulatorSynthChain *chain, int index)
		{
			macroChain = chain;
			macroIndexForCurrentLearnMode = index;
		}

		int getMacroControlLearnMode() const
		{
			return macroIndexForCurrentLearnMode;
		};

		void removeMacroControlsFor(Processor *p);

		void removeMacroControlsFor(Processor *p, Identifier name);
	

		MidiControllerAutomationHandler *getMidiControlAutomationHandler()
		{
			return &midiControllerHandler;
		};

		const MidiControllerAutomationHandler *getMidiControlAutomationHandler() const
		{
			return &midiControllerHandler;
		};

	private:

		MainController *mc;

		int macroControllerNumbers[8];

		ModulatorSynthChain *macroChain;
		int macroIndexForCurrentLearnMode;

		int macroIndexForCurrentMidiLearnMode;

		MidiControllerAutomationHandler midiControllerHandler;
	};

	MainController();

	virtual ~MainController() {	masterReference.clear(); };

	SampleManager &getSampleManager() {return *sampleManager;};

	MacroManager &getMacroManager() {return macroManager;};

	const MacroManager &getMacroManager() const {return macroManager;};

#if USE_BACKEND
	/** Writes to the console. */
	void writeToConsole(const String &message, int warningLevel, const Processor *p=nullptr, Colour c=Colours::transparentBlack);
#endif

	/** increases the voice counter. */
	void increaseVoiceCounter();

	/** Decreases the voice counter. */
	void decreaseVoiceCounter();

	/** Resets the voice counter. */
	void resetVoiceCounter();

	void loadPreset(const File &f, Component *mainEditor=nullptr);

	void loadPreset(ValueTree &v, Component *mainEditor=nullptr);

    void clearPreset();
    
	/** Compiles all scripts in the main synth chain */
	void compileAllScripts();

	/** Call this if you want all voices to stop. */
	void allNotesOff() { allNotesOffFlag = true; };

	/** Create a new processor and returns it. You have to supply a Chain that the Processor will be added to.
	*
	*	The function is static (it will get the MainController() instance from the FactoryType).
	*
	*	@param FactoryType this is used to create the processor and connect it to the MainController.
	*	@param typeName the identifier string of the processor that should be created.
	*	@param id the name of the processor to be created.
	*
	*	@returns a new processor. You have to manage the ownership yourself.
	*/
	static Processor *createProcessor(FactoryType *FactoryTypeToUse, 
 							   const Identifier &typeName, 
							   const String &id);



	/** Adds a PluginParameterModulator to the parameter list of the plugin. */
	int addPluginParameter(PluginParameterModulator *p);
	
	/** Removes a PluginParameterModulator from the parameter list of the plugin. */
	void removePluginParameter(PluginParameterModulator *p);
	
	/** same as AudioProcessor::beginParameterGesture(). */
	void beginParameterChangeGesture(int index);
	
	/** same as AudioProcessor::beginParameterGesture(). */
	void endParameterChangeGesture(int index);
	
	/** sets the plugin parameter to the new Value. */
	void setPluginParameter(int index, float newValue);
	
	/** Returns the uptime in seconds. */
	double getUptime() const noexcept
	{ 
		return uptime; 
	}

	/** returns the tempo as bpm. */
	double getBpm() const noexcept { return bpm > 0.0 ? bpm : 120.0; };

	/** skins the given component (applies the global look and feel to it). */
    void skin(Component &c);
	

	

	/** adds a TempoListener to the main controller that will receive a callback whenever the host changes the tempo. */
	void addTempoListener(TempoListener *t)
	{
		ScopedLock sl(lock);
		tempoListeners.addIfNotAlreadyThere(t);
	}

	/** removes a TempoListener. */
	void removeTempoListener(TempoListener *t)
	{
		ScopedLock sl(lock);
		tempoListeners.removeAllInstancesOf(t);
	};

	ApplicationCommandManager *getCommandManager()
	{
		return mainCommandManager;
	};

    const CriticalSection &getLock() const;
    
	void setKeyboardCoulour(int keyNumber, Colour colour);

	CustomKeyboardState &getKeyboardState();

	void setLowestKeyToDisplay(int lowestKeyToDisplay);

	void addPlottedModulator(Modulator *m);
	void removePlottedModulator(Modulator *m);
	
#if USE_BACKEND

	void setScriptWatchTable(ScriptWatchTable *table);
	void setScriptComponentEditPanel(ScriptComponentEditPanel *panel);

	void setWatchedScriptProcessor(ScriptProcessor *p, Component *editor);

	/** Sets the ScriptComponent object to be edited in the main ScriptComponentEditPanel.
	*
	*	The pointer is a DynamicObject because of forward declaration issues, but make sure, you only call this method with ScriptComponent objects
	*	(otherwise it will have no effect)
	*/
	void setEditedScriptComponent(DynamicObject *c, Component *listener);
	
#endif

	void setPlotter(Plotter *p);

	void setCurrentViewChanged();
	
	/** saves a variable into the global index. */
	void setGlobalVariable(int index, var newVariable);

	/** returns the variable saved at the global index. */
	var getGlobalVariable(int index) const;

	DynamicObject *getGlobalVariableObject() { return globalVariableObject.get(); };

	DynamicObject *getHostInfoObject() { return hostInfo.get(); }

	DynamicObject *getToolbarPropertiesObject() { return toolbarProperties.get(); };

	/** this must be overwritten by the derived class and return the master synth chain. */
	virtual ModulatorSynthChain *getMainSynthChain() = 0;

	virtual const ModulatorSynthChain *getMainSynthChain() const = 0;

	/** Returns the time that the plugin spends in its processBlock method. */
	int getCpuUsage() const {return usagePercent;};

	/** Returns the amount of playing voices. */
	int getNumActiveVoices() const 
	{ 
		ScopedLock sl(lock);

		return voiceAmount; 
	};

	void replaceReferencesToGlobalFolder();

	void showConsole(bool consoleShouldBeShown);

	/**	sets the console component to display all incoming messages. 
	*
	*	If you set 'isPopupConsole' to true, it will not overwrite the pointer to the main console.
	*/
	void setConsole(Console *c, bool isPopupConsole=false)
	{
#if USE_BACKEND
		if (isPopupConsole)
		{
			popupConsole = c;
			usePopupConsole = c != nullptr;
		}
		else
		{
			console = c;
			usePopupConsole = false;
		}

#else 
		ignoreUnused(c, isPopupConsole);

#endif
		
	};


    void clearConsole();



	void setLastActiveEditor(CodeEditorComponent *editor, CodeDocument::Position position)
	{
		lastActiveEditor = editor;
		lastPosition = position;
	}

	void insertStringAtLastActiveEditor(const String &string, bool selectArguments)
	{
		if (lastActiveEditor.getComponent() != nullptr)
		{
			lastActiveEditor->getDocument().deleteSection(lastActiveEditor->getSelectionStart(), lastActiveEditor->getSelectionEnd());
			lastActiveEditor->moveCaretTo(lastPosition, false);

			lastActiveEditor->insertTextAtCaret(string);

			
			
			if (selectArguments)
			{
				lastActiveEditor->moveCaretLeft(false, false);

				while (!lastActiveEditor->getTextInRange(lastActiveEditor->getHighlightedRegion()).contains("("))
				{
					lastActiveEditor->moveCaretLeft(false, true);
				}

				lastActiveEditor->moveCaretRight(false, true);
			}

			lastActiveEditor->grabKeyboardFocus();
		}
	}

    
    bool checkAndResetMidiInputFlag()
    {
        const bool returnValue = midiInputFlag;
        midiInputFlag = false;
        
        return returnValue;
    }

    float getGlobalCodeFontSize() const {return globalCodeFontSize; };
    
    void setGlobalCodeFontSize(float newFontSize)
    {
        globalCodeFontSize = newFontSize;
        codeFontChangeNotificator.sendSynchronousChangeMessage();
    };
    
    SafeChangeBroadcaster &getFontSizeChangeBroadcaster() { return codeFontChangeNotificator; };
    
    /** This sets the global pitch factor. */
    void setGlobalPitchFactor(double pitchFactorInSemiTones)
    {
        globalPitchFactor = pow(2, pitchFactorInSemiTones / 12.0);
    }
    
    /** This returns the global pitch factor. 
    *
    *   Use this in your startVoice method and multiplicate it with your angleDelta.
    */
    double getGlobalPitchFactor() const
    {
        return globalPitchFactor;
    }
    
    /** This returns the global pitch factor as semitones. 
    *
    *   This can be used for displaying / saving purposes.
    */
    double getGlobalPitchFactorSemiTones() const
    {
        return log2(globalPitchFactor) * 12.0;
    }
    
protected:

	/** This is the main processing loop that is shared among all subclasses. */
	void processBlockCommon(AudioSampleBuffer &b, MidiBuffer &mb);

	/** Sets the sample rate for the cpu meter. */
	void prepareToPlay(double sampleRate_, int samplesPerBlock);


	/** sets the new BPM and sends a message to all registered tempo listeners if the tempo changed. */
	void setBpm(double bpm_);

	/** @brief Add this at the beginning of your processBlock() method to enable CPU measurement */
	void startCpuBenchmark(int bufferSize);

	/** @brief Add this at the end of your processBlock() method to enable CPU measurement */
	void stopCpuBenchmark();

	/** Checks if a connected object called allNotesOff() and replaces the content of the supplied MidiBuffer with a allNoteOff event. */
	void checkAllNotesOff(MidiBuffer &midiMessages)
	{
		if(allNotesOffFlag)
		{
			midiMessages.clear();
			midiMessages.addEvent(MidiMessage::allNotesOff(1), 0);

			allNotesOffFlag = false;

			uptime = 0.0;

		}
	};

	double uptime;

	void setScrollY(int newY) {	scrollY = newY;	};
	int getScrollY() const {return scrollY;};

	void setComponentShown(int componentIndex, bool isShown)
	{
		shownComponents.setBit(componentIndex, isShown);
	};

	bool isComponentShown(int componentIndex) const 
	{
		return shownComponents[componentIndex];
	}
	
	

    void setMidiInputFlag() {midiInputFlag = true; };
    

private:

	void storePlayheadIntoDynamicObject(AudioPlayHead::CurrentPositionInfo &lastPosInfo);

	CustomKeyboardState keyboardState;

	AudioSampleBuffer multiChannelBuffer;

	friend class WeakReference<MainController>;
	WeakReference<MainController>::Master masterReference;

	Array<var> globalVariableArray;

	DynamicObject::Ptr globalVariableObject;
	DynamicObject::Ptr hostInfo;
	DynamicObject::Ptr toolbarProperties;

	CriticalSection lock;

	ScopedPointer<SampleManager> sampleManager;
	MacroManager macroManager;

	Component::SafePointer<Plotter> plotter;

	int bufferSize;

	AudioPlayHead::CurrentPositionInfo lastPosInfo;
	
	ScopedPointer<ApplicationCommandManager> mainCommandManager;

	ScopedPointer<KnobLookAndFeel> mainLookAndFeel;

	Component::SafePointer<CodeEditorComponent> lastActiveEditor;
	CodeDocument::Position lastPosition;

    SafeChangeBroadcaster codeFontChangeNotificator;
        
	WeakReference<Console> console;

	WeakReference<Console> popupConsole;
	bool usePopupConsole;

#if USE_BACKEND
    
	Component::SafePointer<ScriptWatchTable> scriptWatchTable;
	Component::SafePointer<ScriptComponentEditPanel> scriptComponentEditPanel;

#else

	const ScriptComponentEditPanel *scriptComponentEditPanel;
	const ScriptWatchTable *scriptWatchTable;

#endif

	Array<WeakReference<TempoListener>> tempoListeners;

	int usagePercent;

    float globalCodeFontSize;

    double globalPitchFactor;
    
	double bpm;
	int voiceAmount;
	bool allNotesOffFlag;
    
    bool midiInputFlag;
	
	double sampleRate;
	double temp_usage;
	int scrollY;
	BigInteger shownComponents;
};

/** A base class for all objects that need access to a MainController.
*	@ingroup core
*
*	If you want to have access to the main controller object, derive the class from this object and pass a pointer to the MainController
*	instance in the constructor.
*/
class ControlledObject
{
public:

	/** Creates a new ControlledObject. The MainController must be supplied. */
	ControlledObject(MainController *m);

	virtual ~ControlledObject();

	/** Provides read-only access to the main controller. */
	const MainController *getMainController() const
	{
		jassert(controller != nullptr);
		return controller;
	};

	/** Provides write access to the main controller. Use this if you want to make changes. */
	MainController *getMainController()
	{
		jassert(controller != nullptr);
		return controller;
	}

private:

	

	friend class WeakReference<ControlledObject>;
	WeakReference<ControlledObject>::Master masterReference;

	MainController* const controller;

	friend class MainController;
	friend class ProcessorFactory;
};


#endif