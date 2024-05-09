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

#ifndef GLOBALSCRIPTCOMPILEBROADCASTER_H_INCLUDED
#define GLOBALSCRIPTCOMPILEBROADCASTER_H_INCLUDED

namespace hise { using namespace juce;

class JavascriptMidiProcessor;
class JavascriptProcessor;

class GlobalScriptCompileBroadcaster;



/** A GlobalScriptCompileListener gets informed whenever a script was compiled.
*
*	add it with getMainController()->addGlolablScriptCompileListener(this),
*	overwrite the callback and do whatever you need to do.
*
*/
class GlobalScriptCompileListener
{
public:

	virtual ~GlobalScriptCompileListener() { masterReference.clear(); };

	/** Whenever a script was compiled, a message is sent out to every listener. */
	virtual void scriptWasCompiled(JavascriptProcessor *processor) = 0;

private:

	friend class WeakReference < GlobalScriptCompileListener > ;
	WeakReference<GlobalScriptCompileListener>::Master masterReference;
};

class ExternalScriptFile : public ReferenceCountedObject
{
public:

	enum class ResourceType
	{
		EmbeddedInSnippet,
		FileBased,
		numResourceTypes
	};

	struct RuntimeError
	{
		using Broadcaster = LambdaBroadcaster<Array<RuntimeError>*>;

		enum class ErrorLevel
		{
			Error = 0,
			Warning = 1,
			Invalid,
			numErrorLevels
		};

		RuntimeError(const String& e);

		String toString() const;

		RuntimeError();

		ErrorLevel errorLevel = ErrorLevel::Invalid;

		bool matches(const String& fileNameWithoutExtension) const;

	private:

		String file;

		int lineNumber = -1;
		
		String errorMessage;
	};



	ExternalScriptFile(const File&file);

	ExternalScriptFile(const File& rootDirectory, const ValueTree& v):
	  resourceType(ResourceType::EmbeddedInSnippet),
	  file(rootDirectory.getChildFile(v["filename"].toString())),
	  currentResult(Result::ok())
	{

#if USE_BACKEND
		content.replaceAllContent(v["content"]);
		content.setSavePoint();
		content.clearUndoHistory();
#endif
	}

	~ExternalScriptFile();

	void setResult(Result r);

	CodeDocument& getFileDocument();

	Result getResult() const;

	void reloadIfChanged()
	{
#if USE_BACKEND

		if(resourceType == ResourceType::EmbeddedInSnippet)
			return;

		auto thisLast = getFile().getLastModificationTime();

		if(thisLast > lastEditTime)
		{
			getFileDocument().replaceAllContent(getFile().loadFileAsString());
            getFileDocument().setSavePoint();
			lastEditTime = thisLast;
		}
#endif
	}

	void saveFile()
	{
		getFile().replaceWithText(getFileDocument().getAllContent());
		lastEditTime = getFile().getLastModificationTime();
        getFileDocument().setSavePoint();
	}

	File getFile() const;

	ValueTree toEmbeddableValueTree(const File& rootDirectory) const
	{
		auto fn = getFile();
		auto name =  fn.getRelativePathFrom(rootDirectory);
		
		ValueTree c("file");
		c.setProperty("filename", name.replaceCharacter('\\', '/'), nullptr);
		c.setProperty("content", content.getAllContent(), nullptr);
		return c;
	}

	typedef ReferenceCountedObjectPtr<ExternalScriptFile> Ptr;

	void setRuntimeErrors(const Result& r);

	RuntimeError::Broadcaster& getRuntimeErrorBroadcaster();

	ResourceType getResourceType() const
	{
		return resourceType;
	}

	bool extractEmbedded()
	{
		if(resourceType == ResourceType::EmbeddedInSnippet)
		{
			if(!file.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite local file", "The file " + getFile().getFileName() + " from the snippet already exists. Do you want to overwrite your local file?"))
			{
				file.getParentDirectory().createDirectory();
				file.replaceWithText(content.getAllContent());
				resourceType = ResourceType::FileBased;
				return true;
			}
		}

		return false;
	}

private:

	Time lastEditTime;

	ResourceType resourceType;

	RuntimeError::Broadcaster runtimeErrorBroadcaster;

	Array<RuntimeError> runtimeErrors;

	Result currentResult;

	File file;

	CodeDocument content;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalScriptFile)
	JUCE_DECLARE_WEAK_REFERENCEABLE(ExternalScriptFile);
};

class ScriptComponentEditBroadcaster;




/** This class sends a message to all registered listeners. */
class GlobalScriptCompileBroadcaster
{
public:

	GlobalScriptCompileBroadcaster();

	virtual ~GlobalScriptCompileBroadcaster();;

	/** This sends a synchronous message to all registered listeners.
	*
	*	Listeners which are manually inserted at the beginning (eg. ScriptingContentComponents) are notified first.
	*/
	void sendScriptCompileMessage(JavascriptProcessor *processorThatWasCompiled);

	/** Adds a ScriptListener. You can influence the order of the callback by inserting Listeners at the beginning of the list. */
	void addScriptListener(GlobalScriptCompileListener *listener, bool insertAtBeginning = false);;

	void removeScriptListener(GlobalScriptCompileListener *listener);;

	void setShouldUseBackgroundThreadForCompiling(bool shouldBeEnabled) noexcept;
	bool isUsingBackgroundThreadForCompiling() const noexcept;

	double getCompileTimeOut() const noexcept;

	void setEnableCompileAllScriptsOnPresetLoad(bool shouldBeEnabled) noexcept;;
	bool isCompilingAllScriptsOnPresetLoad() const noexcept;;

	bool isCallStackEnabled() const noexcept;;
	void updateCallstackSettingForExistingScriptProcessors();


	void fillExternalFileList(Array<File> &files, StringArray &processors);

	void setExternalScriptData(const ValueTree &collectedExternalScripts);

	String getExternalScriptFromCollection(const String &fileName);

	int getNumExternalScriptFiles() const;

	ExternalScriptFile::Ptr getExternalScriptFile(const File& fileToInclude, bool createIfNotFound);

	ExternalScriptFile::Ptr getExternalScriptFile(int index) const;

	void clearIncludedFiles();

	void restoreIncludedScriptFilesFromSnippet(const ValueTree& snippetTree);

	ValueTree collectIncludedScriptFilesForSnippet(const Identifier& id, const File& root) const;

	ScriptComponentEditBroadcaster* getScriptComponentEditBroadcaster();

	const ScriptComponentEditBroadcaster* getScriptComponentEditBroadcaster() const;

	ReferenceCountedObject* getCurrentScriptLookAndFeel();

	virtual void setCurrentScriptLookAndFeel(ReferenceCountedObject* newLaf);

	ReferenceCountedObject* getGlobalRoutingManager();

	void setGlobalRoutingManager(ReferenceCountedObject* newManager);;

	ValueTree exportWebViewResources();

	void restoreWebResources(const ValueTree& v);

    void clearWebResources();

	WebViewData::Ptr getOrCreateWebView(const Identifier& id);

	Array<Identifier> getAllWebViewIds() const;

	void setWebViewRoot(File newRoot);

private:
	
	File webViewRoot;

	Array<std::tuple<Identifier, WebViewData::Ptr>> webviews;

	ReferenceCountedObjectPtr<ReferenceCountedObject> currentScriptLaf;

	ScopedPointer<ScriptComponentEditBroadcaster> globalEditBroadcaster;

	ReferenceCountedObjectPtr<ReferenceCountedObject> routingManager;

    void createDummyLoader();
    
	bool useBackgroundCompiling;
	bool enableGlobalRecompile;

	ValueTree externalScripts;

    DynamicObject::Ptr dummyLibraryLoader; // prevents the SharedResourcePointer from deleting the handler
    
	ReferenceCountedArray<ExternalScriptFile> includedFiles;

	Array<WeakReference<GlobalScriptCompileListener>> listenerListStart;
	Array<WeakReference<GlobalScriptCompileListener>> listenerListEnd;
};

} // namespace hise

#endif  // GLOBALSCRIPTCOMPILEBROADCASTER_H_INCLUDED
