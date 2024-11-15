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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef BACKEND_PROCESSOR_H_INCLUDED
#define BACKEND_PROCESSOR_H_INCLUDED

namespace hise { using namespace juce;



class BackendProcessor;

struct AnalyserInfo: public ReferenceCountedObject
{
	AnalyserInfo(SimpleRingBuffer::PropertyObject* o1, SimpleRingBuffer::PropertyObject* o2)
	{
		ringBuffer[0] = new SimpleRingBuffer();
		ringBuffer[0]->setPropertyObject(o1);
		ringBuffer[1] = new SimpleRingBuffer();
		ringBuffer[1]->setPropertyObject(o2);
	}
	
	using Ptr = ReferenceCountedObjectPtr<AnalyserInfo>;

	SimpleRingBuffer::Ptr getAnalyserBuffer(bool getPost) const
	{
		return ringBuffer[(int)getPost];
	}
	
	std::pair<WeakReference<Processor>, int> currentlyAnalysedProcessor = { nullptr, 0 };
	int lastNoteNumber = -1;
	double duration = 0.0;
	SimpleRingBuffer::Ptr ringBuffer[2];
};

struct ExampleAssetManager: public ReferenceCountedObject,
							public ProjectHandler
						    
{
	ExampleAssetManager(MainController* mc):
	  ProjectHandler(mc),
	  mainProjectHandler(mc->getCurrentFileHandler())
	{
		
	}

	bool initialised = false;

	void initialise();

	FileHandlerBase& mainProjectHandler;

	File getSubDirectory(SubDirectories dir) const override;

	Array<SubDirectories> getSubDirectoryIds() const override;

	File getRootFolder() const override
	{
		return rootDirectory;
	}

	File rootDirectory;

	using Ptr = ReferenceCountedObjectPtr<ExampleAssetManager>;
};

/** This is the main audio processor for the backend application. 
*
*	It connects to a BackendProcessorEditor and has extensive development features.
*
*	It is a wrapper for all plugin types and provides 8 parameters for the macro controls.
*	It also acts as global MainController to allow every child object to get / set certain global information
*/
class BackendProcessor: public PluginParameterAudioProcessor,
					    public AudioProcessorDriver,
						public MainController,
						public ProjectHandler::Listener,
						public MarkdownDatabaseHolder,
						public ExpansionHandler::Listener,
						public SimpleRingBuffer::WriterBase
{
public:
	BackendProcessor(AudioDeviceManager *deviceManager_=nullptr, AudioProcessorPlayer *callback_=nullptr);

	~BackendProcessor();

	void projectChanged(const File& newRootDirectory) override;

	void refreshExpansionType();

	void handleEditorData(bool save);

	void prepareToPlay (double sampleRate, int samplesPerBlock);
	void releaseResources() 
	{
		
	};

	void getStateInformation	(MemoryBlock &destData) override;;

	void logMessage(const String& message, bool isCritical) override
	{
		if (isCritical)
		{
			debugError(getMainSynthChain(), message);
		}
		else
			debugToConsole(getMainSynthChain(), message);
	}

	void setStateInformation(const void *data,int sizeInBytes) override;

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	virtual void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);;

	void handleControllersForMacroKnobs(const MidiBuffer &midiMessages);

	AudioProcessorEditor* createEditor();
	bool hasEditor() const {return true;};

	bool acceptsMidi() const {return true;};
	bool producesMidi() const {return false;};
	
	double getTailLengthSeconds() const {return 0.0;};

	ModulatorSynthChain *getMainSynthChain() override {return synthChain; };

	const ModulatorSynthChain *getMainSynthChain() const override { return synthChain; }

	void registerItemGenerators() override;
	void registerContentProcessor(MarkdownContentProcessor* processor) override;
	File getCachedDocFolder() const override;
	File getDatabaseRootDirectory() const override;

	void toggleDynamicBufferSize() 
	{
		simulateDynamicBufferSize = !isUsingDynamicBufferSize();
	}

	void setDatabaseRootDirectory(File newDatabaseDirectory)
	{
		databaseRoot = newDatabaseDirectory;
	}

	BackendProcessor* getDocProcessor();

	BackendRootWindow* getDocWindow();

	Component* getRootComponent() override;

	bool databaseDirectoryInitialised() const override
	{
		auto path = getSettingsObject().getSetting(HiseSettings::Documentation::DocRepository).toString();

		auto d = File(path);
		
		return d.isDirectory() && d.getChildFile("hise-modules").isDirectory();
	}

	/// @brief returns the number of PluginParameter objects, that are added in the constructor
    int getNumParameters() override
	{
		return 8;
	}

	/// @brief returns the PluginParameter value of the indexed PluginParameter.
    float getParameter (int index) override
	{
		return synthChain->getMacroControlData(index)->getCurrentValue() / 127.0f;
	}

	/** @brief sets the PluginParameter value.
	
		This method uses the 0.0-1.0 range to ensure compatibility with hosts. 
		A more user friendly but slower function for GUI handling etc. is setParameterConverted()
	*/
    void setParameter (int index, float newValue) override
	{
		synthChain->setMacroControl(index, newValue * 127.0f, sendNotificationAsync);
	}


	/// @brief returns the name of the PluginParameter
    const String getParameterName (int index) override
	{
		return "Macro " + String(index + 1);
	}

	/// @brief returns a converted and labeled string that represents the current value
    const String getParameterText (int index) override
	{
		
		return String(synthChain->getMacroControlData(index)->getDisplayValue(), 1);
	}

	JavascriptProcessor* createInterface(int width, int height);;

	void setEditorData(var editorState);

	juce::OnlineUnlockStatus* getLicenseUnlocker() 
	{
		return &scriptUnlocker;
	}

	ScriptUnlocker scriptUnlocker;

#if HISE_INCLUDE_SNEX_FLOATING_TILES
	snex::ui::WorkbenchManager workbenches;
	virtual void* getWorkbenchManager() override { return reinterpret_cast<void*>(&workbenches); }
#endif

	BackendDllManager::Ptr dllManager;

	
    LambdaBroadcaster<Processor*> processorAddBroadcaster;
	
	LambdaBroadcaster<Identifier, Processor*> workspaceBroadcaster;

    ExternalClockSimulator externalClockSim;
	
	void pushToAnalyserBuffer(AnalyserInfo::Ptr info, bool post, const AudioSampleBuffer& buffer, int numSamples);

	int getCurrentAnalyserNoteNumber() const
	{
		return currentNoteNumber;
	}

	AnalyserInfo::Ptr getAnalyserInfoForProcessor(Processor* p);

	Result setAnalysedProcessor(AnalyserInfo::Ptr newInfo, bool add);

	bool isSnippetBrowser() const
	{
		return isSnippet;
	}

	void setIsSnippetBrowser()
	{
		isSnippet = true;
	}

	ExampleAssetManager::Ptr getAssetManager()
	{
		if(assetManager == nullptr)
			assetManager = new ExampleAssetManager(this);

		return assetManager;
	}

	ExampleAssetManager::Ptr assetManager;

	File customDocCacheFolder;

private:

	bool isSnippet = false;

	int currentNoteNumber = -1;

	mutable SimpleReadWriteLock postAnalyserLock;

	ReferenceCountedArray<AnalyserInfo> currentAnalysers;
	
	File databaseRoot;

	MemoryBlock tempLoadingData;

	friend class BackendProcessorEditor;
	friend class BackendCommandTarget;
	friend class CombinedDebugArea;

	ScopedPointer<ModulatorSynthChain> synthChain;
	
	var editorInformation;

	ScopedPointer<BackendProcessor> docProcessor;
	BackendRootWindow* docWindow;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackendProcessor)
};

struct ScopedAnalyser
{
	ScopedAnalyser(MainController* mc, Processor* p, AudioSampleBuffer& b, int numSamples_):
	  bp(static_cast<BackendProcessor*>(mc))
	{
		info = bp->getAnalyserInfoForProcessor(p);

		if(info == nullptr)
			return;

		buffer = &b;
		numSamples = numSamples_;

		bp->pushToAnalyserBuffer(info, false, *buffer, numSamples);
		milliSeconds = Time::getMillisecondCounterHiRes();
	}

	~ScopedAnalyser()
	{
		if(buffer != nullptr)
		{
			auto now = Time::getMillisecondCounterHiRes();

			info->duration = now - milliSeconds;
			bp->pushToAnalyserBuffer(info, true, *buffer, numSamples);
		}
	}

	double milliSeconds = 0.0;
	AnalyserInfo::Ptr info;
	AudioSampleBuffer* buffer = nullptr;
	int numSamples = -1;
	BackendProcessor* bp;
	
};


} // namespace hise

#endif

