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

#pragma once

/** Set this to zero in order to write the hxi data as value tree. */
#ifndef HISE_USE_XML_FOR_HXI
#define HISE_USE_XML_FOR_HXI 0
#endif

namespace hise { using namespace juce;

class ScriptUserPresetHandler : public ConstScriptingObject,
								public ControlledObject,
								public MainController::UserPresetHandler::Listener
{
public:

	ScriptUserPresetHandler(ProcessorWithScriptingContent* pwsc);

	~ScriptUserPresetHandler();

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("UserPresetHandler"); }

	int getNumChildElements() const override
	{
		return 2;
	}

	DebugInformationBase* getChildElement(int index) override
	{
		if (index == 0)
			return preCallback.createDebugObject("preCallback");
		if (index == 1)
			return postCallback.createDebugObject("postCallback");
        
        return nullptr;
	}

	// =================================================================================== API Methods

	/** Enables Engine.undo() to restore the previous user preset (default is disabled). */
	void setUseUndoForPresetLoading(bool shouldUseUndoManager);

	/** Sets a callback that will be executed synchronously before the preset was loaded*/
	void setPreCallback(var presetPreCallback);

	/** Sets a callback that will be executed after the preset has been loaded. */
	void setPostCallback(var presetPostCallback);

	/** Enables a preprocessing of every user preset that is being loaded. */
	void setEnableUserPresetPreprocessing(bool processBeforeLoading, bool shouldUnpackComplexData);

	/** Checks if the given version string is a older version than the current project version number. */
	bool isOldVersion(const String& version);

	/** Disables the default user preset data model and allows a manual data handling. */
	void setUseCustomUserPresetModel(var loadCallback, var saveCallback, bool usePersistentObject);

	/** Enables host / MIDI automation with the custom user preset model. */
	void setCustomAutomation(var automationData);

	/** Attaches a callback to automation changes. Use empty string to attach to all callbacks. */
	void attachAutomationCallback(String automationId, var updateCallback, bool isSynchronous);

	/** Clears all attached callbacks. */
	void clearAttachedCallbacks();

	/** Updates the given automation values and optionally sends out a message. */
	void updateAutomationValues(var data, bool sendMessage, bool useUndoManager);

	/** Creates an object containing the values for every automation ID. */
	var createObjectForAutomationValues();

	/** Creates an object containing all values of components with the `saveInPreset` flag. */
	var createObjectForSaveInPresetComponents();

	/** Restores all values of components with the `saveInPreset` flag. */
	void updateSaveInPresetComponents(var obj);

	/** Restores the values for all UI elements that are connected to a processor with the `processorID` / `parameterId` properties. */
	void updateConnectedComponentsFromModuleState();
	
	/** Runs a few tests that catches data persistency issues. */
	void runTest();

	// ===============================================================================================

	var convertToJson(const ValueTree& d);

	ValueTree applyJSON(const ValueTree& original, DynamicObject::Ptr obj);

	ValueTree prePresetLoad(const ValueTree& dataToLoad, const File& fileToLoad) override;

	void presetChanged(const File& newPreset) override;

	void presetListUpdated() override
	{

	}

	

	void loadCustomUserPreset(const var& dataObject) override
	{
		if (customLoadCallback)
		{
			var args = dataObject;
			auto ok = customLoadCallback.callSync(&args, 1, nullptr);

			if (!ok.wasOk())
				debugError(getMainController()->getMainSynthChain(), ok.getErrorMessage());
		}
	}

	var saveCustomUserPreset(const String& presetName) override
	{
		if (customSaveCallback)
		{
			var rv;
			var args = presetName;
			auto ok = customSaveCallback.callSync(&args, 1, &rv);

			if (!ok.wasOk())
				debugError(getMainController()->getMainSynthChain(), ok.getErrorMessage());

			return rv;
		}

		return {};
	}
	
	

private:

	struct AttachedCallback: public ReferenceCountedObject
	{
		AttachedCallback(ScriptUserPresetHandler* parent, MainController::UserPresetHandler::CustomAutomationData::Ptr cData, var f, bool isSynchronous);

		~AttachedCallback();

		String id;
		WeakCallbackHolder customUpdateCallback;
		WeakCallbackHolder customAsyncUpdateCallback;

		static void onCallbackSync(AttachedCallback& c, var* args);

		static void onCallbackAsync(AttachedCallback& c, int index, float newValue);

		MainController::UserPresetHandler::CustomAutomationData::Ptr cData;

		JUCE_DECLARE_WEAK_REFERENCEABLE(AttachedCallback);
	};

public:

private:

	bool enablePreprocessing = false;
	bool unpackComplexData = false;
	WeakCallbackHolder preCallback;
	WeakCallbackHolder postCallback;

	WeakCallbackHolder customLoadCallback;
	WeakCallbackHolder customSaveCallback;
	
	ReferenceCountedArray<AttachedCallback> attachedCallbacks;

	File currentlyLoadedFile;
	struct Wrapper;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptUserPresetHandler);
};


class ScriptExpansionHandler : public ConstScriptingObject,
							   public ControlledObject,
							   public ExpansionHandler::Listener
{
public:

	ScriptExpansionHandler(JavascriptProcessor* jp_);

	~ScriptExpansionHandler();

	Identifier getObjectName() const override { return "ExpansionHandler"; }

	// ============================================================== API Calls

	/** Set a encryption key that will be used to encrypt the content (deprecated). */
	void setEncryptionKey(String newKey);

	/** Set a credentials object that can be embedded into each expansion. */
	void setCredentials(var newCredentials);

	/** Sets whether the installExpansionFromPackage function should install full dynamics. */
	void setInstallFullDynamics(bool shouldInstallFullDynamics);

	/** Sets a error function that will be executed. */
	void setErrorFunction(var newErrorFunction);

	/** Calls the error function specified with setErrorFunction. */
	void setErrorMessage(String errorMessage);

	/** Returns a list of all available expansions. */
	var getExpansionList();

	/** Returns the expansion with the given name*/
	var getExpansion(var name);

	/** Returns the currently active expansion (if there is one). */
	var getCurrentExpansion();

	/** Set a function that will be called whenever a expansion is being loaded. */
	void setExpansionCallback(var expansionLoadedCallback);

	/** Set a function that will be called during installation of a new expansion. */
	void setInstallCallback(var installationCallback);

	/** Returns a list of all expansions that aren't loaded properly yet. */
	var getUninitialisedExpansions();

	/** Call this to refresh the expansion list. */
	bool refreshExpansions();

	/** Sets the current expansion as active expansion. */
	bool setCurrentExpansion(var expansionName);

	/** Encrypts the given hxi file. */
	bool encodeWithCredentials(var hxiFile);

	/** Decompresses the samples and installs the .hxi / .hxp file. */
	bool installExpansionFromPackage(var packageFile, var sampleDirectory);

	/** Checks if the expansion is already installed and returns a reference to the expansion if it exists. */
	var getExpansionForInstallPackage(var packageFile);

	/** Sets a list of allowed expansion types that can be loaded. */
	void setAllowedExpansionTypes(var typeList);

	// ============================================================== End of API calls

	bool isUsingCredentials() const
	{
		auto& h = getMainController()->getExpansionHandler();
		return h.getCredentials().isObject();
	}

	String getEncryptionKey() const
	{
		return getMainController()->getExpansionHandler().getEncryptionKey();
	}

	void expansionPackLoaded(Expansion* currentExpansion) override;
	void expansionPackCreated(Expansion* newExpansion) override;

	void logMessage(const String& message, bool isCritical) override;

private:

	struct InstallState: public Timer,
						 public ExpansionHandler::Listener
	{
		InstallState(ScriptExpansionHandler& parent_);

		~InstallState();

		void expansionPackCreated(Expansion* newExpansion) override;

		void expansionInstallStarted(const File& targetRoot, const File& packageFile, const File& sampleDirectory);

		void expansionInstalled(Expansion* newExpansion) override;

		void call();

		void timerCallback() override;

		var getObject();

		double getProgress();
		
		double getTotalProgress();

		ScriptExpansionHandler& parent;

		int status = 0;
		File sourceFile;
		File targetFolder;
		File sampleFolder;
		Expansion* createdExpansion = nullptr;

		SimpleReadWriteLock timerLock;
	};

	ProcessorWithScriptingContent* getScriptProcessor();

	void reportScriptError(const String& error)
	{
#if USE_BACKEND
		throw error;
#endif
	}

	struct Wrapper;

	WeakCallbackHolder errorFunction;
	WeakCallbackHolder expansionCallback;
	WeakCallbackHolder installCallback;
	WeakReference<JavascriptProcessor> jp;

	ScopedPointer<InstallState> currentInstaller;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptExpansionHandler);
};


class ScriptEncryptedExpansion : public Expansion
{
public:

	ScriptEncryptedExpansion(MainController* mc, const File& f);

	Result loadValueTree(ValueTree& v);

	virtual ExpansionType getExpansionType() const;

	Result encodeExpansion() override;

	Array<SubDirectories> getSubDirectoryIds() const override;
	Result initialise() override;
	juce::BlowFish* createBlowfish();

	static BlowFish* createBlowfish(MainController* mc);
	static bool encryptIntermediateFile(MainController* mc, const File& f, File expansionRoot=File());

	void extractUserPresetsIfEmpty(ValueTree encryptedTree, bool forceExtraction = false);

protected:

	void encodePoolAndUserPresets(ValueTree &hxiData, bool encodeAdditionalData);
	Result skipEncryptedExpansionWithoutKey();
	Result initialiseFromValueTree(const ValueTree& hxiData);
	PoolBase::DataProvider::Compressor* createCompressor(bool createEncrypted);
	void setCompressorForPool(SubDirectories fileType, bool createEncrypted);
	void addDataType(ValueTree& parent, SubDirectories fileType);
	void restorePool(ValueTree encryptedTree, SubDirectories fileType);
	void addUserPresets(ValueTree encryptedTree);

	Result returnFail(const String& errorMessage);
};

/** This expansion type can be used for a custom C++ shell that will load any instrument.

	Unlike the default expansion (or script encrypted expansion) this expansion will include
	the current patch and can be loaded into a C++ shell.
*/
class FullInstrumentExpansion : public ScriptEncryptedExpansion,
								public ExpansionHandler::Listener
{
public:


	struct DefaultHandler : public ControlledObject,
		public ExpansionHandler::Listener
	{
		DefaultHandler(MainController* mc, ValueTree t) :
			ControlledObject(mc),
			defaultPreset(t),
			defaultIsLoaded(true)
		{
			getMainController()->getExpansionHandler().addListener(this);
		}

		~DefaultHandler()
		{
			getMainController()->getExpansionHandler().removeListener(this);
		}

		void expansionPackLoaded(Expansion* e) override
		{
			if (e != nullptr)
			{
				defaultIsLoaded = false;
			}
			else
			{
				if (!defaultIsLoaded)
				{
					auto copy = defaultPreset.createCopy();

					getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), [copy, this](Processor* p)
					{
						defaultIsLoaded = true;
						p->getMainController()->loadPresetFromValueTree(copy);

						return SafeFunctionCall::OK;
					}, MainController::KillStateHandler::SampleLoadingThread);
				}
			}
		}

	private:

		ValueTree defaultPreset;
		bool defaultIsLoaded = true;
	};

	FullInstrumentExpansion(MainController* mc, const File& f) :
		ScriptEncryptedExpansion(mc, f)
	{

	}

	~FullInstrumentExpansion()
	{
		getMainController()->getExpansionHandler().removeListener(this);
	}

	static void setNewDefault(MainController* mc, ValueTree t);

	static bool isEnabled(const MainController* mc);

	static Expansion* getCurrentFullExpansion(const MainController* mc);

	void setIsProjectExporter()
	{
		isProjectExport = true;
	}

	ValueTree getValueTreeFromFile(Expansion::ExpansionType type);

	Result lazyLoad();

	void expansionPackLoaded(Expansion* e);

	Result initialise() override;

	Result encodeExpansion() override;

	ValueTree getEmbeddedNetwork(const String& id) override
	{
		return networks.getChildWithProperty("ID", id);
	}

	bool fullyLoaded = false;
	ValueTree presetToLoad;
	bool isProjectExport = false;
	String currentKey;

	ValueTree networks;
};


class ScriptExpansionReference : public ConstScriptingObject
{
public:

	// ============================================================================================================

	ScriptExpansionReference(ProcessorWithScriptingContent* p, Expansion* e);

	// ============================================================================================================

	Identifier getObjectName() const override { return "Expansion"; }
	bool objectDeleted() const override { return exp == nullptr; }
	bool objectExists() const override { return exp != nullptr; }

	BlowFish* createBlowfish();

	// ============================================================================================================

	/** Returns the root folder for this expansion. */
	var getRootFolder();

	/** Returns a list of all available sample maps in the expansion. */
	var getSampleMapList() const;

	/** Returns a list of all available images in the expansion. */
	var getImageList() const;

	/** Returns a list of all available audio files in the expansion. */
	var getAudioFileList() const;

	/** Returns a list of all available MIDI files in the expansion. */
	var getMidiFileList() const;

	/** Returns a list of all available data files in the expansion. */
	var getDataFileList() const;

	/** Returns a list of all available user presets in the expansion. */
	var getUserPresetList() const;

	/** Returns the folder where this expansion looks for samples. */
	var getSampleFolder();

	/** Changes the sample folder of that particular expansion. */
	bool setSampleFolder(var newSampleFolder);

	/** Attempts to parse a JSON file in the AdditionalSourceCode directory of the expansion. */
	var loadDataFile(var relativePath);

	/** Returns a valid wildcard reference ((`{EXP::Name}relativePath`) for the expansion. */
	String getWildcardReference(var relativePath);

	/** Writes the given data into the file in the AdditionalSourceCode directory of the expansion. */
	bool writeDataFile(var relativePath, var dataToWrite);

	/** Returns an object containing all properties of the expansion. */
	var getProperties() const;

	/** returns the expansion type. Use the constants of ExpansionHandler to resolve the integer number. */
	int getExpansionType() const;

	/** Reextracts (and overrides) the user presets from the given expansion. Only works with intermediate / encrypted expansions. */
	bool rebuildUserPresets();

	/** Sets whether the samples are allowed to be duplicated for this expansion. Set this to false if you operate on the same samples differently. */
	void setAllowDuplicateSamples(bool shouldAllowDuplicates);

	/** Unloads this expansion so it will not show up in the list of expansions until the next restart. */
	void unloadExpansion();

private:

	friend class ScriptExpansionHandler;

	struct Wrapper;

	WeakReference<Expansion> exp;
};



class ExpansionEncodingWindow : public DialogWindowWithBackgroundThread,
	public ControlledObject,
	public ExpansionHandler::Listener
{
public:

	static constexpr int AllExpansionId = 9000000;

	ExpansionEncodingWindow(MainController* mc, Expansion* eToEncode, bool isProjectExport);
	~ExpansionEncodingWindow();

	void logMessage(const String& message, bool /*isCritical*/)
	{
		showStatusMessage(message);
	}

	void run() override;
	void threadFinished();

	Result encodeResult;
	
	bool projectExport = false;
	WeakReference<Expansion> e;
};

struct UnlockerHandler
{
	virtual ~UnlockerHandler() {};
	virtual juce::OnlineUnlockStatus* getUnlockerObject() = 0;
};

struct ScriptUnlocker : public juce::OnlineUnlockStatus,
					    public UnlockerHandler,
					    public ControlledObject
{
	ScriptUnlocker(MainController* mc):
		ControlledObject(mc)
	{

	}

	struct RefObject : public ConstScriptingObject
	{
		RefObject(ProcessorWithScriptingContent* p);

		~RefObject();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Unlocker"); }

		WeakReference<ScriptUnlocker> unlocker;

		/** Checks if the registration went OK. */
		var isUnlocked() const;

		/** Checks if the unlocker's license system has an expiration date. */
		var canExpire() const;

		/** If the unlocker has an expiration date, it will check it against the RSA encoded time string from the server. */
		var checkExpirationData(const String& encodedTimeString);

		/** Sets a function that performs a product name check and expects to return true or false for a match. */
		void setProductCheckFunction(var f);

		/** This checks if there is a key file and applies it.  */
		var loadKeyFile();

        /** Checks whether the key file exists. */
        bool keyFileExists() const;
        
		/** Writes the key data to the location. */
		var writeKeyFile(const String& keyData);

		/** Checks if the possibleKeyData might contain a key file. */
		bool isValidKeyFile(var possibleKeyData);

		/** Returns the user email that was used for the registration. */
		String getUserEmail() const;

		/** Returns the machine ID that is encoded into the license file. This does not look in the encrypted blob, but just parses the header string. */
		String getRegisteredMachineId();

		WeakCallbackHolder pcheck;

		struct Wrapper;

		JUCE_DECLARE_WEAK_REFERENCEABLE(RefObject);
	};

	juce::OnlineUnlockStatus* getUnlockerObject() override { return this; }

	String getProductID() override;
	bool doesProductIDMatch(const String& returnedIDFromServer) override;
	RSAKey getPublicKey() override;
	void saveState(const String&) override;
	String getState() override;
	String getWebsiteName() override;
	URL getServerAuthenticationURL() override;
	String readReplyFromWebserver(const String& email, const String& password) override;

	var loadKeyFile();
	File getLicenseKeyFile();
	WeakReference<RefObject> currentObject;

	String registeredMachineId;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptUnlocker);
};

} // namespace hise
