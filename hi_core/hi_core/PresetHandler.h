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

#ifndef PRESET_HANDLER_H_INCLUDED
#define PRESET_HANDLER_H_INCLUDED

#define PRESET_MENU_ITEM_DELTA 80
#define CLIPBOARD_ITEM_MENU_INDEX 999

class MainController;
class Chain;
class Processor;
class FactoryType;

#if USE_BACKEND

#define PRODUCT_ID ""
#define PUBLIC_KEY ""

#endif

#if USE_COPY_PROTECTION

class Unlocker: public OnlineUnlockStatus
{
public:
    Unlocker():
        state(String())
    {}

	String getProductID() override;;

	bool doesProductIDMatch(const String& returnedIDFromServer) override { return returnedIDFromServer == getProductID(); };

    juce::RSAKey getPublicKey() override;

    String getState() override
    {
        return state;
    };

	String getWebsiteName() override;

	String readReplyFromWebserver(const String &/*email*/, const String &/*password*/) override { return ""; };

	URL getServerAuthenticationURL() override;
   
    void saveState(const String &s) override
    {
        state = s;
    }
private:
    String state;
};



#endif

class AboutPage : public Component,
				  public ButtonListener
{
public:

    AboutPage();

	void refreshText();

	void buttonClicked(Button *b) override;

	void mouseDown(const MouseEvent &) override
	{

		Desktop::getInstance().getAnimator().fadeOut(this, 400);

		setVisible(false);
	}


	void resized() override
	{
		refreshText();

#if USE_BACKEND
		//checkUpdateButton->setBounds(16, getHeight() - 32, 100, 24);
#endif
	}

	void showAboutPage()
	{
		Desktop::getInstance().getAnimator().fadeIn(this, 400);
	}

	void paint(Graphics &g) override;

	void setUserEmail(const String &userEmail_)
	{
		userEmail = userEmail_;

		refreshText();
	}

private:

	AttributedString infoData;

	String userEmail;

	ScopedPointer<TextButton> checkUpdateButton;

	AlertWindowLookAndFeel alaf;
    
    Image aboutHeader;

};

class ModulatorSynthChain;


/** The class that wraps all file resolving issues.
*
*	It assumes a working directory and supplies correct paths for all OSes relative to the project root folder.
*
*/
class ProjectHandler
{
public:

	ProjectHandler()
	{
#if USE_BACKEND
		restoreWorkingProjects();
#endif
	}

	enum class SubDirectories
	{
		Scripts = 0,
		Binaries,
		Presets,
		UserPresets,
		XMLPresetBackups,
		Samples,
		Images,
		AudioFiles,
		SampleMaps,
		AdditionalSourceCode,
		numSubDirectories
	};

	void createNewProject(const File &workingDirectory);

	void setWorkingProject(const File &workingDirectory);

	static const StringArray &getRecentWorkDirectories() { return recentWorkDirectories; }

	/** Returns the subdirectory. */
	File getSubDirectory(SubDirectories dir) const;

	/** Returns the current work directory. */
	File getWorkDirectory() const;

	/** Checks if a directory is redirected. */
    bool isRedirected(SubDirectories dir) const;
    
	/** Checks if the ProjectHandler is active (if a directory is set). */
	bool isActive() const;
	
	/** creates a absolute path from the pathToFile and the specified sub directory. */
	String getFilePath(const String &pathToFile, SubDirectories subDir) const;
	
	/** Creates a reference string that can be used to obtain the file in the project directory. 
	*
	*	If the file is not in 
	*/
	const String getFileReference(const String &absoluteFileName, SubDirectories dir) const;

	/** Creates a platform dependant file in the subdirectory that redirects to another location.
	*
	*	This is mainly used for storing audio samples at another location to keep the project folder size small.
	*/
	void createLinkFile(SubDirectories dir, const File &relocation);

	/** */
	void setProjectSettings(Component *mainEditor=nullptr);

	static String getIdentifier(SubDirectories dir);

	static SubDirectories getSubDirectoryForIdentifier(Identifier id);

	/** Fills the given array with the contents of the specified directory. If 'sortByTime' is true, the most recent files will be the first items in the list. */
	void getFileList(Array<File> &filesInDirectory, SubDirectories dir, const String &wildcard, bool sortByTime = false, bool searchInSubfolders=false);

	void createRSAKey() const;

	String getPublicKey() const;

	String getPrivateKey() const;

    /** checks if this is a absolute path (including absolute win paths on OSX and absolute OSX paths on windows); */
    static bool isAbsolutePathCrossPlatform(const String &pathName)
    {
        const bool isAbsoluteWindowsPath = pathName.containsChar(':');
        const bool isAbsoluteOSXPath = pathName.startsWithChar('/');
        
        return isAbsoluteWindowsPath || isAbsoluteOSXPath || File::isAbsolutePath(pathName);
    }
    
	class Frontend
	{
	public:

		static File getSampleLocationForCompiledPlugin();

		/** This returns the app data directory, which must be created by the installer of your product.
		*
		*	On OSX this will be ("Users/Library/Application Support/Company/Product/") and on Windows ("Users/AppData/Local/Company/Product").
		*
		*	This directory will be used for:
		*	- sample location folder (using a LinkOS file)
		*	- user presets (in the UserPresets subfolder)
		*	- licence key file
		*/
		static File getAppDataDirectory(ProjectHandler *handler=nullptr);

		static File getLicenceKey();

		static String getLicenceKeyExtension();

		static String getSanitiziedFileNameForPoolReference(const String &absoluteFileName);
		
		static void setSampleLocation(const File &newLocation);
		static File getUserPresetDirectory();
		
		static String getProjectName();
		static String getCompanyName();
		static String getCompanyWebsiteName();
		static String getVersionString();
	};

private:


	struct FolderReference
	{
		FolderReference(SubDirectories d, bool r, File f) :
			directoryType(d),
			isReference(r),
			file(f)
		{};

		FolderReference() :
			directoryType(SubDirectories::numSubDirectories),
			isReference(false),
			file(File())
		{};

		FolderReference(const FolderReference& other) noexcept:
		    directoryType(other.directoryType),
			isReference(other.isReference),
			file(other.file)
		{
		}

		FolderReference& operator= (FolderReference other) noexcept
		{
			directoryType = other.directoryType;
			file = other.file;
			isReference = other.isReference;
			return *this;
		}

		SubDirectories directoryType;
		bool isReference;
		File file;
	};

	void restoreWorkingProjects();

	

	bool isValidProjectFolder(const File &file) const;

	Array<FolderReference> subDirectories;
	
    File getLinkFile(const File &subDirectory);
	
	void checkSubDirectories();

	File checkSubDirectory(SubDirectories dir);
	void checkSettingsFile();
	File currentWorkDirectory;

	static StringArray recentWorkDirectories;

	Component::SafePointer<Component> window;
};


class UserPresetData
{
public:
	// ================================================================================================================

	/** A Listener will be notified if a preset was loaded. */
	class Listener
	{
	public:

		virtual ~Listener() {};
		virtual void presetLoaded(int categoryIndex, int presetIndex, const String &presetName) = 0;
	};

	// ================================================================================================================

	UserPresetData(MainController* mc_);;
	~UserPresetData();

	// ================================================================================================================

	/** Call this when the amount of presets has changed. */
	void refreshPresetFileList();

	/** Loads a preset with the given category and preset index.
	*
	*	If the indexes are invalid, nothing will happen.
	*/
	void loadPreset(int categoryToLoad, int presetToLoad) const;

	/** Loads the next preset and adjusts the current index. */
	void loadNextPreset() const;

	/** Loads the previous preset and adjusts the current index. */
	void loadPreviousPreset() const;

	/** Fills a StringArray with all presets. You can use the indexes to call loadPreset(). */
	void fillPresetList(StringArray& listToFill, int categoryIndex) const;;

	/** Fills a StringArray with all categories (User Presets being the last category. */
	void fillCategoryList(StringArray& listToFill) const;

	/** Provides information about the current preset. */
	void getCurrentPresetIndexes(int &category, int &preset, String &name) const;

	/** Add a listener that will receive a notification when a preset was loaded. */
	void addListener(Listener *newListener) const;

	/** Removes a listener. */
	void removeListener(Listener *listenerToRemove) const;

private:

	// ================================================================================================================

	struct Entry
	{
		Entry(String name_, int id_, const ValueTree &v_) : name(name_), id(id_), v(v_) {};
		Entry() : name(""), id(-1) {};

		String name;
		int id;
		ValueTree v;
	};

	struct PresetCategory
	{
		PresetCategory(String name_) : name(name_) {};
		PresetCategory() : name("") {};

		String name;
		Array<Entry> presets;
	};

	// ================================================================================================================

	void addFactoryPreset(const String &name, const String &category, int id, ValueTree &v);
	void addUserPreset(const String &name, int id, const ValueTree &v);
	const PresetCategory* getPresetCategory(int index) const;

	MainController* mc;
	mutable Array<Listener*> listeners;

	OwnedArray<PresetCategory> factoryPresetCategories;
	ScopedPointer<PresetCategory> userPresets;

	mutable String currentName = "Default";
	mutable int currentCategoryIndex = 0;
	mutable	int currentPresetIndex = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UserPresetData);

	// ================================================================================================================
};


class UserPresetHandler
{
public:
    
    static void saveUserPreset(ModulatorSynthChain *chain, const String& targetFile=String());
    
    static void loadUserPreset(ModulatorSynthChain *chain, const File &fileToLoad);

	static void loadUserPreset(ModulatorSynthChain* chain, const ValueTree &v);
    
    static File getUserPresetFile(ModulatorSynthChain *chain, const String &fileNameWithoutExtension);
};

/** A helper class which provides loading and saving Processors to files and clipboard. 
*	@ingroup utility
*
*/
class PresetHandler
{
public:

	enum class IconType
	{
		Info = 0,
		Warning,
		Question,
		Error,
		numIconTypes
	};

	/** Saves the Processor into a subfolder of the directory provided with getPresetFolder(). */
	static void saveProcessorAsPreset(Processor *p, const String &directory=String());
	
	static void copyProcessorToClipboard(Processor *p);

	/** Opens a modal window that allow renaming of a Processor. */
	static String getCustomName(const String &typeName);

	/** Opens a Yes/No box (HI Style) */
	static bool showYesNoWindow(const String &title, const String &message, IconType icon=IconType::Question);

	/** Opens a message box (HI Style) */
	static void showMessageWindow(const String &title, const String &message, IconType icon=IconType::Info);


	/** Checks if an child processor has a already taken name. If silentMode is false, it will display a message box at the end. */
	static void checkProcessorIdsForDuplicates(Processor *rootProcessor, bool silentMode=true);

	/** Returns a popupmenu with all suiting Processors for the supplied FactoryType. */
	static PopupMenu getAllSavedPresets(int minIndex, Processor *parentChain);

	/** Checks if the file exists and returns it or opens a dialog to point to the missing file. */
	static File checkFile(const String &pathName);

	/** Checks if the directory exists and returns it or opens a dialog to point to the directory. */
	static File checkDirectory(const String &directoryName);

	/** Removes all view info from a ValueTree. */
	static void stripViewsFromPreset(ValueTree &preset)
	{
		preset.removeProperty("views", nullptr);
		preset.removeProperty("currentView", nullptr);

		preset.removeProperty("EditorState", nullptr);

		for(int i = 0; i < preset.getNumChildren(); i++)
		{
            ValueTree child = preset.getChild(i);
            
			stripViewsFromPreset(child);
		}
	}
    
    static File loadFile(const String &extension)
    {
		jassert(extension.isEmpty() || extension.startsWith("*"));

        FileChooser fc("Load File", File(), extension, true);
        
        if(fc.browseForFileToOpen())
        {
            
            return fc.getResult();
        }
        return File();
    }
    
	
	
    static void saveFile(const String &dataToSave, const String &extension)
    {
		jassert(extension.isEmpty() || extension.startsWith("*"));

        FileChooser fc("Save File", File(), extension);
        
        if(fc.browseForFileToSave(true))
        {
            fc.getResult().deleteFile();
            fc.getResult().create();
            fc.getResult().appendText(dataToSave);
        }
        
    }

    static void setChanged(Processor *p);
    
	/** Checks if the. */
	static String getProcessorNameFromClipboard(const FactoryType *t);

	/** Creates a processor from the Popupmenu. 
	*
	*	It will be connected to the MainController after creation.
	*
	*	@param menuIndexDelta - the menu index of the selected popupitem from the PopupMenu received with getAllSavedPresets.
	*							If the menu was added to another menu as submenu, you have to subtract the last item index before the submenu.
	*	@param m		      - the main controller. This must not be a nullptr!
	*
	*	@returns			  - a connected and restored Processor which can be added to a chain.
	*/
	static Processor *createProcessorFromPreset(int menuIndexDelta, Processor *parent);

	static File getPresetFileFromMenu(int menuIndexDelta, Processor *parent);

	/** Creates a processor from xml data in the clipboard.
	*
	*	The XML data must be parsed before this function, but it checks if a Processor can be created from the data.
	*/
	static Processor *createProcessorFromClipBoard(Processor *parent);

	static void setUniqueIdsForProcessor(Processor * root);

	static ValueTree changeFileStructureToNewFormat(const ValueTree &v)
	{
		ValueTree newTree("Processor");

		newTree.copyPropertiesFrom(v, nullptr);
		newTree.removeProperty("MacroControls", nullptr);
		newTree.removeProperty("EditorState", nullptr);

		newTree.setProperty("Type", v.getType().toString(), nullptr);

		ScopedPointer<XmlElement> editorValueSet = XmlDocument::parse(v.getProperty("EditorState", var::undefined()));

		if(newTree.hasProperty("Content"))
		{
			MemoryBlock b = *v.getProperty("Content", MemoryBlock()).getBinaryData();

			ValueTree restoredContentValues = ValueTree::readFromData(b.getData(), b.getSize());

			newTree.removeProperty("Content", nullptr);

			newTree.addChild(restoredContentValues, -1, nullptr);
		};

		if(editorValueSet != nullptr)
		{
			ValueTree editorStateValueTree = ValueTree::fromXml(*editorValueSet);
			newTree.addChild(editorStateValueTree, -1, nullptr);
		}

		ScopedPointer<XmlElement> macroControlData = XmlDocument::parse(v.getProperty("MacroControls", String()));

		if(macroControlData != nullptr)
		{
			ValueTree macros = ValueTree::fromXml(*macroControlData);
			newTree.addChild(macros, -1, nullptr);
		}

		ValueTree childProcessors("ChildProcessors");

		for(int i = 0; i < v.getNumChildren(); i++)
		{
			ValueTree newChild = changeFileStructureToNewFormat(v.getChild(i));

			childProcessors.addChild(newChild, -1, nullptr);
		}

		newTree.addChild(childProcessors, -1, nullptr);

		return newTree;
	}

#if USE_COPY_PROTECTION
	static bool loadKeyFile(Unlocker &ul)
	{
		File keyFile = ProjectHandler::Frontend::getLicenceKey();

		if (keyFile.existsAsFile())
		{
			String keyData = keyFile.loadFileAsString();

			if (ul.applyKeyFile(keyData))
			{
				return true;
			}
		}
		
        return false;
	};
#endif

	/** checks if one of the needed directories exists. */
	static bool checkDirectory(bool checkPresetDirectory=true)
	{
		const String keyName = checkPresetDirectory ? "Library Install Path" : "GlobalSampleFolder";

#if JUCE_WINDOWS

		if (WindowsRegistry::valueExists("HKEY_CURRENT_USER\\Software\\Hart Instruments\\" + keyName) && WindowsRegistry::getValue("HKEY_CURRENT_USER\\Software\\Hart Instruments\\" + keyName).isNotEmpty())
		{
			return true;
		}
		
		if(AlertWindow::showNativeDialogBox(keyName + " not found!", "Press OK to choose the " + keyName, false))
		{
			FileChooser fc("Select " + keyName);
			if (fc.browseForDirectory())
			{
				File directory = fc.getResult();

				if (checkPresetDirectory && directory.getFileNameWithoutExtension() != "SynthPresets") return false;
				
				WindowsRegistry::setValue("HKEY_CURRENT_USER\\Software\\Hart Instruments\\" + keyName, directory.getFullPathName());
				return true;

			}
			else return false;

		}

		return false;
			
#else
        
        if(checkPresetDirectory) return true; // The Preset directory must exist in OSX in "Library/Application Support/Hart Instruments"
		
        String path = getSettingsValue(keyName);
        
        if(path.isEmpty())
        {
            if(AlertWindow::showNativeDialogBox(keyName + " not found!", "Press OK to choose the " + keyName, false))
            {
                FileChooser fc("Select " + keyName);
                if (fc.browseForDirectory())
                {
                    File directory = fc.getResult();
                    
                    setSettingsValue(keyName, directory.getFullPathName());
                    return true;
                    
                }
                else return false;
                
            }
        }
        else
        {
            return true;
        }
        
        return true;
#endif
	};

	/** Returns the Preset Folder.
    *
    *   On Windows it returns the folder set in the registry. On Mac OSX it returns the folder "SynthPresets" in the User's Document
    *   Directory.
    */
	static File getPresetFolder() 
	{ 

        jassertfalse;

#if JUCE_WINDOWS
        
		File returnPath = File(File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getFullPathName() + "/Hart Instruments/SynthPresets/");
		
		if (!returnPath.exists())
		{
			showMessageWindow("The Preset Folder was not found", "The preset folder 'Hart Instruments/SynthPresets' was not found. It must be in the user documentation folder.", IconType::Error);
		}

		return returnPath;

#elif JUCE_MAC
        
#if HISE_IOS
      
        return File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory);
        
#else
        
        File returnPath = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() + "/Application Support/Hart Instruments/SynthPresets/");

		if (!returnPath.exists())
		{
			showMessageWindow("The Preset Folder was not found", "The preset folder 'Hart Instruments/SynthPresets' was not found. It must be in the user's 'Library/Application Support' folder.", IconType::Error);
		}

		return returnPath;
#endif

#else

		return File();

#endif
	};

	
	/** Opens a file dialog and saves the new path into the library's setting file. */
	static File getSampleFolder(const String &libraryName)
	{
		const bool search = NativeMessageBox::showOkCancelBox(AlertWindow::WarningIcon, "Sample Folder can't be found", "The sample folder for " + libraryName + "can't be found. Press OK to search or Cancel to abort loading");

		if(search)
		{
			FileChooser fc("Searching Sample Folder");

			if(fc.browseForDirectory())
			{
				File sampleFolder = fc.getResult();

				

				return sampleFolder;
			}
		}
		

		return File();
		
		
	}
    
    static AudioFormatReader *getReaderForFile(const File &file)
    {
        AudioFormatManager afm;
        afm.registerBasicFormats();
        return afm.createReaderFor(file);
    }
    
    static AudioFormatReader *getReaderForInputStream(InputStream *stream)
    {
        AudioFormatManager afm;
        afm.registerBasicFormats();
        return afm.createReaderFor(stream);
    }

	
	/** This looks in the application settings directory for a file called libraryName.library and creates it if it doesn't exist. */
	static File getSampleDataSettingsFile(const String &libraryName)
	{
		File settingsFolder = File(getDataFolder());

		if(!settingsFolder.exists()) settingsFolder.createDirectory();

		File settings = settingsFolder.getFullPathName() + "/" + libraryName + ".library";

		if(!settings.exists())
		{
			settings.create();
		}

		return settings;
	}

	
    static String getSettingsValue(const String &settingId)
    {
        ScopedPointer<XmlElement> xml = XmlDocument::parse(File(getDataFolder()).getChildFile("settings.xml"));
        
        if(xml != nullptr)
        {
            XmlElement *setting = xml->getChildByName(settingId);
            
            if(setting != nullptr) return setting->getStringAttribute("value");
            else return String();
        }
    
        jassertfalse;
        return String();
    }
    
    static void setSettingsValue(const String &settingId, const String &value)
    {
        File f = File(getDataFolder()).getChildFile("settings.xml");
        
        ScopedPointer<XmlElement> xml;
        
        if(!f.existsAsFile())
        {
            xml = new XmlElement("Settings");
            f.create();
        }
        else
        {
            xml = XmlDocument::parse(f);
        }
        
        jassert(xml != nullptr);
        
        if(xml != nullptr)
        {
            if(xml->getChildByName(settingId) != nullptr)
            {
                xml->getChildByName(settingId)->setAttribute("value", value);
            
            }
            else
            {
            
                XmlElement *setting = new XmlElement(settingId);
            
                setting->setAttribute("value", value);
            
                xml->addChildElement(setting);
            }
            String newContent = xml->createDocument("");
            
            f.replaceWithText(newContent);
            
            return;
        }
        
        jassertfalse;
        return;
    }
    
	static String getGlobalSampleFolder();

    static String getDataFolder();

	static ValueTree loadValueTreeFromData(const void* data, size_t size, bool wasCompressed)
	{
		if (wasCompressed)
		{
			return ValueTree::readFromGZIPData(data, size);
		}
		else
		{
			return ValueTree::readFromData(data, size);
		}
	}

	static void writeValueTreeAsFile(const ValueTree &v, const String &fileName, bool compressData=false)
	{
		File file(fileName);
		file.deleteFile();
		file.create();

		if (compressData)
		{
			FileOutputStream fos(file);

			GZIPCompressorOutputStream gzos(&fos, 9, false);

			MemoryOutputStream mos;

			v.writeToStream(mos);

			gzos.write(mos.getData(), mos.getDataSize());
			gzos.flush();
		}
		else
		{
			FileOutputStream fos(file);

			v.writeToStream(fos);
		}

		
	}

	static var writeValueTreeToMemoryBlock(const ValueTree &v)
	{
		MemoryBlock mb;

		MemoryOutputStream mos(mb, false);

		v.writeToStream(mos);

		return var(mb.getData(), mb.getSize());
	}

	static void writeSampleMapsToValueTree(ValueTree &sampleMapTree, ValueTree &preset);

	static void buildProcessorDataBase(Processor *root);

	static XmlElement *buildFactory(FactoryType *t, const String &factoryName);

	// creates a processor from the file
	static Processor *loadProcessorFromFile(File fileName, Processor *parent);

	
private:

	//static void handlePreset(int menuIndexDelta, Processor *p, bool createNewProcessor)

	// Returns the subdirectory for each processor type
	static File getDirectory(Processor *p);




};

/** A handler class for all stuff related to HISE Player.
*
*	This is a seperate class in order to keep the codebase clean.
*/
class PresetPlayerHandler
{
public:

	enum FolderType
	{
		GlobalSampleDirectory = 0,
		PackageDirectory,
		StreamedSampleFolder,
		ImageResources,
		AudioFiles
	};

	/** This returns the folder for the package structure. */
	static String getSpecialFolder(FolderType type, const String &packageName = String(), bool ignoreNonExistingDirectory=false);

	static void checkAndCreatePackage(const String &packageName);

	static void addInstrumentToPackageXml(const String &instrumentFileName, const String &packageName);

};

#endif