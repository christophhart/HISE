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

	/** Sets a callback that will be executed synchronously before the preset was loaded*/
	void setPreCallback(var presetPreCallback);

	/** Sets a callback that will be executed after the preset has been loaded. */
	void setPostCallback(var presetPostCallback);

	/** Enables a preprocessing of every user preset that is being loaded. */
	void setEnableUserPresetPreprocessing(bool processBeforeLoading, bool shouldUnpackComplexData);

	/** Checks if the given version string is a older version than the current project version number. */
	bool isOldVersion(const String& version);

	// ===============================================================================================

	var convertToJson(const ValueTree& d);

	ValueTree applyJSON(const ValueTree& original, DynamicObject::Ptr obj);

	ValueTree prePresetLoad(const ValueTree& dataToLoad, const File& fileToLoad) override;

	void presetChanged(const File& newPreset) override;

	void presetListUpdated() override
	{

	}

private:

	bool enablePreprocessing = false;
	bool unpackComplexData = false;
	WeakCallbackHolder preCallback;
	WeakCallbackHolder postCallback;
	File currentlyLoadedFile;
	struct Wrapper;
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


struct ScriptUnlocker : public juce::OnlineUnlockStatus,
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

		/** Sets a function that performs a product name check and expects to return true or false for a match. */
		void setProductCheckFunction(var f);

		/** This checks if there is a key file and applies it.  */
		var loadKeyFile();

		/** Writes the key data to the location. */
		var writeKeyFile(const String& keyData);

		/** Returns the user email that was used for the registration. */
		String getUserEmail() const;

		/** Returns the machine ID that is encoded into the license file. This does not look in the encrypted blob, but just parses the header string. */
		String getRegisteredMachineId();

		WeakCallbackHolder pcheck;

		struct Wrapper;

		JUCE_DECLARE_WEAK_REFERENCEABLE(RefObject);
	};

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
