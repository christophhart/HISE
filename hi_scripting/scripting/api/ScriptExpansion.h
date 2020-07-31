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

namespace hise { using namespace juce;


class ScriptExpansionHandler : public ConstScriptingObject,
							   public ControlledObject,
							   public ExpansionHandler::Listener
{
public:

	ScriptExpansionHandler(JavascriptProcessor* jp_);

	~ScriptExpansionHandler();

	Identifier getObjectName() const override { return "ExpansionHandler"; }

	// ============================================================== API Calls

	/** Set a encryption key that will be used to encrypt the content. */
	void setEncryptionKey(String newKey);

	/** Set a credentials object that can be embedded into each expansion. */
	void setCredentials(var newCredentials);

	/** Sets a error function that will be executed. */
	void setErrorFunction(var newErrorFunction);

	/** Calls the error function specified with setErrorFunction. */
	void setErrorMessage(String errorMessage);

	/** Returns a list of all available expansions. */
	var getExpansionList();

	/** Set a function that will be called whenever a expansion is being loaded. */
	void setExpansionCallback(var expansionLoadedCallback);

	/** Returns a list of all expansions that aren't loaded properly yet. */
	var getUninitialisedExpansions();

	/** Call this to refresh the expansion list. */
	void refreshExpansions();

	/** Sets the current expansion as active expansion. */
	bool setCurrentExpansion(String expansionName);

	/** Encrypts the given hxi file. */
	bool encodeWithCredentials(var hxiFile);



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

	struct Wrapper;

	var errorFunction;
	var loadedCallback;
	WeakReference<JavascriptProcessor> jp;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptExpansionHandler);
};


class ScriptEncryptedExpansion : public Expansion
{
public:

	ScriptEncryptedExpansion(MainController* mc, const File& f);

	virtual ExpansionType getExpansionType() const;

	void encodeExpansion() override;

	Array<SubDirectories> getSubDirectoryIds() const override;

	Result initialise() override;
	
	
	juce::BlowFish* createBlowfish();

	static BlowFish* createBlowfish(MainController* mc);

	static bool encryptIntermediateFile(MainController* mc, const File& f);

private:

	Result skipEncryptedExpansionWithoutKey();

	Result initialiseFromValueTree(const ValueTree& hxiData);

	PoolBase::DataProvider::Compressor* createCompressor(bool createEncrypted);

	void setCompressorForPool(SubDirectories fileType, bool createEncrypted);

	void addDataType(ValueTree& parent, SubDirectories fileType);

	void restorePool(ValueTree encryptedTree, SubDirectories fileType);

	void addUserPresets(ValueTree encryptedTree);

	void extractUserPresetsIfEmpty(ValueTree encryptedTree);
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

	var getMidiFileList() const;

	/** Attempts to parse a JSON file in the AdditionalSourceCode directory of the expansion. */
	var loadDataFile(var relativePath);

	/** Writes the given data into the file in the AdditionalSourceCode directory of the expansion. */
	bool writeDataFile(var relativePath, var dataToWrite);

	/** Returns an object containing all properties of the expansion. */
	var getProperties() const;

	/** Encodes the expansion with the credentials provided. */
private:

	friend class ScriptExpansionHandler;

	struct Wrapper;

	WeakReference<Expansion> exp;
};

} // namespace hise
