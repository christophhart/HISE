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

namespace hise { using namespace juce;

#define GET_SETTING(id) dataObject.getSetting(id).toString()
#define IS_SETTING_TRUE(id) (bool)dataObject.getSetting(id) == true


void loadOtherReferencedImages(ModulatorSynthChain* chainToExport)
{
	auto mc = chainToExport->getMainController();

	auto& handler = GET_PROJECT_HANDLER(chainToExport);

	const bool hasCustomSkin = handler.getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("keyboard").isDirectory();

	if (!hasCustomSkin)
		return;

	for (int i = 0; i < 12; i++)
	{
		auto img = ImagePool::loadImageFromReference(mc, "{PROJECT_FOLDER}keyboard/up_" + String(i) + ".png");
		jassert(img.isValid());
		auto img2 = ImagePool::loadImageFromReference(mc, "{PROJECT_FOLDER}keyboard/down_" + String(i) + ".png");
		jassert(img2.isValid());
	}

	const bool hasAboutPageImage = handler.getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("about.png").existsAsFile();

	if (hasAboutPageImage)
	{
		// make sure it's loaded
		ImagePool::loadImageFromReference(mc, "{PROJECT_FOLDER}about.png");
	}
}

ValueTree BaseExporter::exportReferencedImageFiles()
{
	// Export the interface


	loadOtherReferencedImages(chainToExport);

	ImagePool *imagePool = chainToExport->getMainController()->getSampleManager().getImagePool();

	

	ValueTree imageTree = imagePool->exportAsValueTree();

	return imageTree;

	
}

ValueTree BaseExporter::exportReferencedAudioFiles()
{
	// Search for impulse responses

	DirectoryIterator iter(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles), false);

	AudioSampleBufferPool *samplePool = chainToExport->getMainController()->getSampleManager().getAudioSampleBufferPool();

	while (iter.next())
	{
#if JUCE_WINDOWS

		// Skip OSX hidden files on windows...
		if (iter.getFile().getFileName().startsWith(".")) continue;

#endif

		samplePool->loadFileIntoPool(iter.getFile().getFullPathName());
	}

	return samplePool->exportAsValueTree();

	
}

ValueTree BaseExporter::exportUserPresetFiles()
{
	File presetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

	DirectoryIterator iter(presetDirectory, true, "*", File::findFiles);

	ValueTree userPresets("UserPresets");

	while (iter.next())
	{
		File f = iter.getFile();

        if(f.isHidden())
            continue;
        
#if JUCE_WINDOWS
		if (f.getFileName().startsWith("."))
			continue;
#endif

		XmlDocument doc(f);

		ScopedPointer<XmlElement> xml = doc.getDocumentElement();

		if (xml != nullptr)
		{
			File pd = f.getParentDirectory();

			const String category = pd == presetDirectory ? "" : pd.getFileName();

			xml->setAttribute("FileName", f.getFileNameWithoutExtension());
			xml->setAttribute("Category", category);

			ValueTree v = ValueTree::fromXml(*xml);

			userPresets.addChild(v, -1, nullptr);
		}
		else
		{
			// The presets must be valid XML files!
			jassertfalse;
		}
	}

	return userPresets;

	
}

ValueTree BaseExporter::exportEmbeddedFiles(bool includeSampleMaps)
{
	ValueTree externalScriptFiles = FileChangeListener::collectAllScriptFiles(chainToExport);
	ValueTree customFonts = chainToExport->getMainController()->exportCustomFontsAsValueTree();
	
	

	ValueTree externalFiles("ExternalFiles");
	externalFiles.addChild(externalScriptFiles, -1, nullptr);
	externalFiles.addChild(customFonts, -1, nullptr);

	if (includeSampleMaps)
	{
		ValueTree sampleMaps = collectAllSampleMapsInDirectory();
		externalFiles.addChild(sampleMaps, -1, nullptr);
	}

	return externalFiles;

	
}

ValueTree BaseExporter::exportPresetFile()
{
	ValueTree preset = chainToExport->exportAsValueTree();

	PresetHandler::stripViewsFromPreset(preset);

	return preset;

	
}


BaseExporter::BaseExporter(ModulatorSynthChain* chainToExport_) :
	chainToExport(chainToExport_),
	dataObject(dynamic_cast<GlobalSettingManager*>(chainToExport_->getMainController())->getSettingsObject())
{

}

ValueTree BaseExporter::collectAllSampleMapsInDirectory()
{
	ValueTree sampleMaps("SampleMaps");

	File sampleMapDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);

	Array<File> sampleMapFiles;

	sampleMapDirectory.findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

	sampleMapFiles.sort();

	for (int i = 0; i < sampleMapFiles.size(); i++)
	{
        if(sampleMapFiles[i].isHidden() || sampleMapFiles[i].getFileName().startsWith("."))
            continue;
        
		ScopedPointer<XmlElement> xml = XmlDocument::parse(sampleMapFiles[i]);

		if (xml != nullptr)
		{
			ValueTree sampleMap = ValueTree::fromXml(*xml);
			sampleMaps.addChild(sampleMap, -1, nullptr);
		}
	}

	

	return sampleMaps;
}

bool CompileExporter::globalCommandLineExport = false;
bool CompileExporter::useCIMode = false;

void CompileExporter::printErrorMessage(const String& title, const String &message)
{
	if (isExportingFromCommandLine())
	{
		std::cout << "ERROR: " << title << std::endl;
		std::cout << message;
	}
	else
	{
		PresetHandler::showMessageWindow(title, message, PresetHandler::IconType::Error);
	}
}

String CompileExporter::getCompileResult(ErrorCodes result)
{
	switch (result)
	{
	case CompileExporter::OK: return "OK";
    case CompileExporter::SanityCheckFailed: return "The sanity check failed. Aborting export...";
	case CompileExporter::PresetIsInvalid:  return "Preset file not found";
	case CompileExporter::ProjectXmlInvalid: return "Project XML invalid";
	case CompileExporter::HISEImageDirectoryNotFound: return "HISE image directory not found";
	case CompileExporter::IntrojucerNotFound: return "Projucer not found";
	case CompileExporter::UserAbort: return "User Abort";
	case CompileExporter::MissingArguments: return "Missing arguments";
	case CompileExporter::BuildOptionInvalid: return "Invalid build options";
	case CompileExporter::CompileError: return "Compilation error";
	case CompileExporter::VSTSDKMissing: return "VST SDK is missing";
	case CompileExporter::AAXSDKMissing: return "AAX SDK is missing";
	case CompileExporter::ASIOSDKMissing: return "ASIO SDK is missing";
	case CompileExporter::HISEPathNotSpecified: return "HISE path not set";
	case CompileExporter::numErrorCodes: return "OK";
		
	default:
		break;
	}

	return "OK";
}


void CompileExporter::writeValueTreeToTemporaryFile(const ValueTree& v, const String &tempFolder, const String& childFile, bool compress)
{
	PresetHandler::writeValueTreeAsFile(v, File(tempFolder).getChildFile(childFile).getFullPathName(), compress); ;
}


CompileExporter::CompileExporter(ModulatorSynthChain* chainToExport_) :
	BaseExporter(chainToExport_),
	
	hisePath(File()),
    useIpp(false)
{}

CompileExporter::ErrorCodes CompileExporter::exportMainSynthChainAsInstrument(BuildOption option/*=BuildOption::Cancelled*/)
{
	ErrorCodes result = exportInternal(TargetTypes::InstrumentPlugin, option);
    if(result != ErrorCodes::OK) printErrorMessage("Export Error", getCompileResult(result));
    return result;
}


CompileExporter::ErrorCodes CompileExporter::exportMainSynthChainAsFX(BuildOption option/*=BuildOption::Cancelled*/)
{
	ErrorCodes result = exportInternal(TargetTypes::EffectPlugin, option);
    if(result != ErrorCodes::OK) printErrorMessage("Export Error", getCompileResult(result));
    return result;
    
}

CompileExporter::ErrorCodes CompileExporter::exportMainSynthChainAsStandaloneApp(BuildOption option/*=BuildOption::Cancelled*/)
{
	ErrorCodes result = exportInternal(TargetTypes::StandaloneApplication, option);
    if(result != ErrorCodes::OK) printErrorMessage("Export Error", getCompileResult(result));
    return result;
    
}


CompileExporter::ErrorCodes CompileExporter::compileFromCommandLine(const String& commandLine, String& pluginFile)
{

	//String options = commandLine.fromFirstOccurrenceOf("export ", false, false);

	StringArray args = StringArray::fromTokens(commandLine, true);

	String exportType = args[0];
	args.remove(0);

	ErrorCodes result = ErrorCodes::OK;

	if (args.size() > 2)
	{
		CompileExporter::setExportingFromCommandLine();
		CompileExporter::setExportUsingCI(exportType == "export_ci");

		ScopedPointer<StandaloneProcessor> processor = new StandaloneProcessor();
		ScopedPointer<BackendRootWindow> editor = dynamic_cast<BackendRootWindow*>(processor->createEditor());
		ModulatorSynthChain* mainSynthChain = editor->getBackendProcessor()->getMainSynthChain();
		File currentProjectFolder = GET_PROJECT_HANDLER(mainSynthChain).getWorkDirectory();

		File presetFile;

		if (isUsingCIMode())
		{
			presetFile = currentProjectFolder.getChildFile(args[0].unquoted());
		}
		else
		{
			presetFile = File(args[0].unquoted());
		}

		if (!presetFile.existsAsFile())
		{
			return ErrorCodes::PresetIsInvalid;
		}

		File projectDirectory = presetFile.getParentDirectory().getParentDirectory();

		std::cout << "Loading the preset...";

		CompileExporter exporter(mainSynthChain);

		bool switchBack = false;

		if (currentProjectFolder != projectDirectory)
		{
			switchBack = true;
			GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(projectDirectory, editor);
		}

		if (presetFile.getFileExtension() == ".hip")
		{
			editor->getBackendProcessor()->loadPresetFromFile(presetFile, editor);
		}
		else if (presetFile.getFileExtension() == ".xml")
		{
			BackendCommandTarget::Actions::openFileFromXml(editor, presetFile);
		}

		std::cout << "DONE" << std::endl << std::endl;

		BuildOption b = exporter.getBuildOptionFromCommandLine(args);

		pluginFile = HelperClasses::getFileNameForCompiledPlugin(exporter.dataObject, mainSynthChain, b);

		if (BuildOptionHelpers::isEffect(b)) result = exporter.exportMainSynthChainAsFX(b);
		else if (BuildOptionHelpers::isInstrument(b)) result = exporter.exportMainSynthChainAsInstrument(b);
		else if (BuildOptionHelpers::isStandalone(b)) result = exporter.exportMainSynthChainAsStandaloneApp(b);
		else result = ErrorCodes::BuildOptionInvalid;

		if (!isUsingCIMode() && switchBack)
		{
			GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(currentProjectFolder, editor);
		}

		editor = nullptr;
		processor = nullptr;

		return result;
	}
	else
	{
		return ErrorCodes::MissingArguments;
	}
}

int CompileExporter::getBuildOptionPart(const String& argument)
{
	if (argument.length() < 2) return 0;

    auto pointer = argument.getCharPointer();
    
    juce_wchar c = *pointer;
    
    while(!CharacterFunctions::isLetter(c))
    {
        pointer++;
        c = *pointer;
    }

	switch (c)
	{
	case 'p':
	{
		const String pluginName = argument.fromFirstOccurrenceOf("-p:", false, true);

		if (pluginName == "VST") return 0x0010;
		else if (pluginName == "AU") return 0x0020;
		else if (pluginName == "VST_AU") return 0x0040;
		else if (pluginName == "AAX") return 0x0080;
        else if (pluginName == "ALL") return 0x10000;
		else return 0;
	}
    case 'i':
    {
        useIpp = true;
        return 0;
    }
	case 'a':
	{
		const String architectureName = argument.fromFirstOccurrenceOf("-a:", false, true);

		if (architectureName == "x86") return 0x0001;
		else if (architectureName == "x64") return 0x0002;
		else if (architectureName == "x86x64") return 0x0004;
		else return 0;
	}
	case 't':
	{
		const String typeName = argument.fromFirstOccurrenceOf("-t:", false, true);

		if (typeName == "standalone") return 0x0100;
		else if (typeName == "instrument") return 0x0200;
		else if (typeName == "effect") return 0x0400;
		else return 0;
	}
	case 'h':
	{
		hisePath = File(argument.fromFirstOccurrenceOf("-h:", false, true).removeCharacters("\""));
		return 0;
	}
	}

	return 0;
}

CompileExporter::BuildOption CompileExporter::getBuildOptionFromCommandLine(StringArray &args)
{
	Array<int> buildParts;

#if JUCE_WINDOWS
	buildParts.add(0x1000);
#elif JUCE_LINUX
    buildParts.add(0x0000);
#else

#if HISE_IOS
	buildParts.add(0x4000);
#else
	buildParts.add(0x2000);
#endif
#endif

	for (int i = 1; i < args.size(); i++)
		buildParts.add(getBuildOptionPart(args[i]));


#if JUCE_MAC
	buildParts.add(0x0004);
#endif


	int o = 0;

	for (int i = 0; i < buildParts.size(); i++)
	{
		o |= buildParts[i];
	}

	BuildOption op = (BuildOption)o;

	return op;
}

CompileExporter::ErrorCodes CompileExporter::exportInternal(TargetTypes type, BuildOption option)
{
	const auto& data = dynamic_cast<GlobalSettingManager*>(chainToExport->getMainController())->getSettingsObject();

	if (!useIpp) useIpp = data.getSetting(HiseSettings::Compiler::UseIPP);
    
	if(!hisePath.isDirectory()) 
		hisePath = data.getSetting(HiseSettings::Compiler::HisePath);

	if (!hisePath.isDirectory()) 
		return ErrorCodes::HISEPathNotSpecified;

	if (!checkSanity(option)) return ErrorCodes::SanityCheckFailed;

	ErrorCodes result = ErrorCodes::OK;

	String uniqueId, version, solutionDirectory, publicKey;
	if(option == BuildOption::Cancelled) option = showCompilePopup(type);

	if (option == BuildOption::Cancelled) return ErrorCodes::UserAbort;

	publicKey = GET_PROJECT_HANDLER(chainToExport).getPublicKey();
	uniqueId = data.getSetting(HiseSettings::Project::Name);
	version = data.getSetting(HiseSettings::Project::Version);

	solutionDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getFullPathName();

	if (option != Cancelled)
	{
		// Use plugin mode for iOS Standalone AUv3 apps...
		const bool createPlugin = type != TargetTypes::StandaloneApplication || BuildOptionHelpers::isIOS(option);

		if (createPlugin)
		{
			result = createPluginDataHeaderFile(solutionDirectory, publicKey, BuildOptionHelpers::isIOS(option));
			if (result != ErrorCodes::OK) return result;
		}
		else
		{
			result = createStandaloneAppHeaderFile(solutionDirectory, uniqueId, version, publicKey);
			if (result != ErrorCodes::OK) return result;
		}

		result = copyHISEImageFiles();

		if (result != ErrorCodes::OK) return result;

		const String directoryPath = File(solutionDirectory).getChildFile("temp/").getFullPathName();

		convertTccScriptsToCppClasses();
		writeValueTreeToTemporaryFile(exportPresetFile(), directoryPath, "preset");

#if DONT_EMBED_FILES_IN_FRONTEND
		const bool embedFiles = false;
#else
		// Don't embedd external files on iOS for quicker loading times...
		const bool embedFiles = !BuildOptionHelpers::isIOS(option);
#endif

		// Embed the user presets and extract them on first load
		writeValueTreeToTemporaryFile(UserPresetHelpers::collectAllUserPresets(chainToExport), directoryPath, "userPresets", true);

		// Always embed scripts and fonts, but don't embed samplemaps
		writeValueTreeToTemporaryFile(exportEmbeddedFiles(embedFiles && type != TargetTypes::EffectPlugin), directoryPath, "externalFiles", true);

		if (embedFiles)
		{
			if (IS_SETTING_TRUE(HiseSettings::Project::EmbedAudioFiles))
			{
				writeValueTreeToTemporaryFile(exportReferencedAudioFiles(), directoryPath, "impulses");
				writeValueTreeToTemporaryFile(exportReferencedImageFiles(), directoryPath, "images");
			}
			else
			{
				File appFolder = ProjectHandler::Frontend::getAppDataDirectory(chainToExport).getChildFile("AudioResources.dat");
				PresetHandler::writeValueTreeAsFile(exportReferencedAudioFiles(), appFolder.getFullPathName());

				File imageFolder = ProjectHandler::Frontend::getAppDataDirectory(chainToExport).getChildFile("ImageResources.dat");
				PresetHandler::writeValueTreeAsFile(exportReferencedImageFiles(), imageFolder.getFullPathName());
			}
		}

		String presetDataString("PresetData");
		String sourceDirectory = solutionDirectory + "/Source";

		CppBuilder::exportValueTreeAsCpp(directoryPath, sourceDirectory, presetDataString);

		if (createPlugin)
		{
			result = createPluginProjucerFile(type, option, chainToExport);
			if (result != ErrorCodes::OK) return result;
		}
		else
		{
			result = createStandaloneAppProjucerFile(option);
			if (result != ErrorCodes::OK) return result;
		}

		result = compileSolution(option, type);

		return result;
	}

	return ErrorCodes::UserAbort;
}

String checkSampleReferences(ModulatorSynthChain* chainToExport)
{
    Processor::Iterator<ModulatorSampler> iter(chainToExport);
    
    const File sampleFolder = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Samples);
    
    Array<File> sampleFiles;
    
    sampleFolder.findChildFiles(sampleFiles, File::findFiles, true);
    
    while(ModulatorSampler* sampler = iter.getNextProcessor())
    {
        auto map = sampler->getSampleMap();
        
        auto v = map->exportAsValueTree();
        
        const String faulty = map->checkReferences(v, sampleFolder, sampleFiles);
        
        if(faulty.isNotEmpty())
            return faulty;
    }
    
    return String();
}

bool CompileExporter::checkSanity(BuildOption option)
{
	// Check if a frontend script is in the main synth chain

    const bool frontWasFound = chainToExport->hasDefinedFrontInterface();

	if (!frontWasFound)
	{
		printErrorMessage("No Interface found.", "You have to add at least one script processor and call Synth.addToFront(true).");
		return false;
	}

	// Check the settings are correct

	ProjectHandler *handler = &GET_PROJECT_HANDLER(chainToExport);

	const String productName = GET_SETTING(HiseSettings::Project::Name);

    if(productName.isEmpty())
    {
        printErrorMessage("Empty Project Name", "You have to specify a project name via File -> Settings -> Project Settings");
        return false;
    }
    
    if (!productName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
    {
        printErrorMessage("Illegal Project name", "The Project name must not contain exotic characters");
        return false;
    }
    
	const String companyName = GET_SETTING(HiseSettings::User::Company);
    
    if(companyName.isEmpty())
    {
        printErrorMessage("Empty Company Name", "You have to specify a company name via File -> Settings -> User Settings");
        return false;
    }
    
    if (!companyName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
    {
        printErrorMessage("Illegal Project name", "The Company name must not contain exotic characters");
        return false;
    }
    
	const String pluginCode = GET_SETTING(HiseSettings::Project::PluginCode);

    const String codeWildcard = "[A-Z][a-z][a-z][a-z]";
    
    if(!RegexFunctions::matchesWildcard(codeWildcard, pluginCode))
    {
        printErrorMessage("Illegal Project code", "The Plugin Code must have this structure: 'Abcd'");
        return false;
    }

	const String companyCode = GET_SETTING(HiseSettings::User::CompanyCode);
    
    if(!RegexFunctions::matchesWildcard(codeWildcard, companyCode))
    {
        printErrorMessage("Illegal Company code", "The Company Code must have this structure: 'Abcd'");
        return false;
    }
    
	const File hiseDirectory = GET_SETTING(HiseSettings::Compiler::HisePath);

    if(!hiseDirectory.isDirectory() || !hiseDirectory.getChildFile("hi_core/").isDirectory())
    {
        printErrorMessage("HISE path is not valid", "You need to set the correct path to the HISE SDK at File -> Settings -> Compiler Settings");
        return false;
    };

	File splashScreenFile = handler->getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("SplashScreen.png");
	File splashScreeniPhoneFile = handler->getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("SplashScreeniPhone.png");

	if (splashScreenFile.existsAsFile() || splashScreeniPhoneFile.existsAsFile())
	{
		if (!splashScreenFile.existsAsFile())
		{
			printErrorMessage("Missing Splash screen file", "You have specified a splash screen file for iPhone but not for iPad. Add a file called `SplashScreen.png` to your image folder");
			return false;
		}
			

		if (!splashScreeniPhoneFile.existsAsFile())
		{
			printErrorMessage("Missing Splash screen file", "You have specified a splash screen file for iPad but not for iPhone. Add a file called `SplashScreeniPhone.png` to your image folder");
			return false;
		}
	}

    if(BuildOptionHelpers::isVST(option))
    {
        const File vstSDKDirectory = hiseDirectory.getChildFile("tools/SDK/VST3 SDK/public.sdk/");
        
        if(!vstSDKDirectory.isDirectory())
        {
            printErrorMessage("VST SDK not found", "You need to download the VST SDK and copy it to '%HISE_SDK%/tools/SDK/VST3 SDK/'");
            return false;
        }
    }
    
#if JUCE_WINDOWS
    if(BuildOptionHelpers::isStandalone(option))
    {
        const File asioSDK = hiseDirectory.getChildFile("tools/SDK/ASIOSDK2.3/common");
        
        if(!asioSDK.isDirectory())
        {
            printErrorMessage("ASIO SDK not found", "You need to download the ASIO SDK and copy it to '%HISE_SDK%/tools/SDK/ASIOSDK2.3/'");
            return false;
        }
    }
#endif

    if(BuildOptionHelpers::isAAX(option))
    {
        const File aaxSDK = hiseDirectory.getChildFile("tools/SDK/AAX/Libs");
        
        if(!aaxSDK.isDirectory())
        {
            printErrorMessage("AAX SDK not found", "You need to get the AAX SDK from Avid and copy it to '%HISE_SDK%/tools/SDK/AAX/'");
            return false;
        }
    }
    
    if(!isExportingFromCommandLine() && PresetHandler::showYesNoWindow("Check Sample references", "Do you want to validate all sample references"))
    {
        const String faultySample = checkSampleReferences(chainToExport);
        
        if(faultySample.isNotEmpty())
        {
            printErrorMessage("Wrong / missing sample reference", "The sample " + faultySample + " is missing or an absolute path");
            return false;
        }
    }
    
	return true;
}

CompileExporter::BuildOption CompileExporter::showCompilePopup(TargetTypes type)
{
	AlertWindowLookAndFeel pplaf;

    const String n = (type == CompileExporter::TargetTypes::StandaloneApplication) ? "Standalone App" : "Plugin";
    
	AlertWindow w("Compile Patch as " + n, String(), AlertWindow::AlertIconType::NoIcon);

	w.setLookAndFeel(&pplaf);
	w.setUsingNativeTitleBar(true);
	w.setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	w.setColour(AlertWindow::textColourId, Colours::white);
	w.addComboBox("buildOption", StringArray(), "Export Format");

	ComboBox* b = w.getComboBoxComponent("buildOption");

#if JUCE_WINDOWS

	switch (type)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin:
		b->addItem("VSTi 64bit", BuildOption::VSTiWindowsx64);
		b->addItem("VSTi 32bit", BuildOption::VSTiWindowsx86);
		b->addItem("VSTi 32bit/64bit", BuildOption::VSTiWindowsx64x86);
		b->addItem("AAX 64bit", BuildOption::AAXWindowsx64);
		b->addItem("AAX 32bit", BuildOption::AAXWindowsx86);
		b->addItem("AAX 32bit/64bit", BuildOption::AAXWindowsx86x64);
        b->addItem("All platforms", BuildOption::AllPluginFormatsInstrument);
		break;
	case CompileExporter::TargetTypes::EffectPlugin:
		b->addItem("VST 64bit", BuildOption::VSTWindowsx64);
		b->addItem("VST 32bit", BuildOption::VSTWindowsx86);
		b->addItem("VST 32bit/64bit", BuildOption::VSTWindowsx64x86);
		b->addItem("AAX 64bit", BuildOption::AAXWindowsx64);
		b->addItem("AAX 32bit", BuildOption::AAXWindowsx86);
		b->addItem("AAX 32bit/64bit", BuildOption::AAXWindowsx86x64);
        b->addItem("All Platforms", BuildOption::AllPluginFormatsFX);
		break;
	case CompileExporter::TargetTypes::StandaloneApplication:
		b->addItem("Standalone 64bit", BuildOption::StandaloneWindowsx64);
		b->addItem("Standalone 32bit", BuildOption::StandaloneWindowsx86);
		b->addItem("Standalone 32bit/64bit", BuildOption::StandaloneWindowsx64x86);
		break;
	case CompileExporter::TargetTypes::numTargetTypes:
		break;
	default:
		break;
	}
#elif JUCE_LINUX
	switch (type)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin:
		b->addItem("VSTi", BuildOption::VSTiLinux);
		break;
	case CompileExporter::TargetTypes::EffectPlugin:
		b->addItem("VST", BuildOption::VSTLinux);
		break;
	case CompileExporter::TargetTypes::StandaloneApplication:
		b->addItem("Standalone Linux", BuildOption::StandaloneLinux);
		break;
	case CompileExporter::TargetTypes::numTargetTypes:
		break;
	default:
		break;
	}
#else
	switch (type)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin:
		b->addItem("AUi", BuildOption::AUimacOS);
		b->addItem("AAX", BuildOption::AAXmacOS);
		b->addItem("VSTi", BuildOption::VSTimacOS);
		b->addItem("VSTi + AUi", BuildOption::VSTiAUimacOS);
        b->addItem("All platforms", BuildOption::AllPluginFormatsInstrument);
		break;
	case CompileExporter::TargetTypes::EffectPlugin:
		b->addItem("AU", BuildOption::AUmacOS);
		b->addItem("AAX", BuildOption::AAXmacOS);
		b->addItem("VST", BuildOption::VSTmacOS);
		b->addItem("VST + AU", BuildOption::VSTAUmacOS);
        b->addItem("All Platforms", BuildOption::AllPluginFormatsFX);
		break;
	case CompileExporter::TargetTypes::StandaloneApplication:
		b->addItem("Standalone macOS", BuildOption::StandalonemacOS);
		b->addItem("Standalone iOS (iPad / iPhone)", BuildOption::StandaloneiOS);
		b->addItem("Standalone iPad only", BuildOption::StandaloneiPad);
		b->addItem("Standalone iPhone only", BuildOption::StandaloneiPhone);
		break;
	case CompileExporter::TargetTypes::numTargetTypes:
		break;
	default:
		break;
	}

#endif

	w.addButton("OK", 1, KeyPress(KeyPress::returnKey));
	w.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	w.getComboBoxComponent("buildOption")->setLookAndFeel(&pplaf);

	if (w.runModalLoop())
	{
		int i = w.getComboBoxComponent("buildOption")->getSelectedId();

		return (BuildOption)i;
	}
	else
	{
		return Cancelled;
	}

}

StringArray CompileExporter::getTccSection(const StringArray &cLines, const String &sectionName)
{
	int startIndex = -1;
	int endIndex = -1;

	for (int i = 0; i < cLines.size(); i++)
	{
		if (cLines[i].startsWith("// [" + sectionName + "]")) startIndex = i;
		if (cLines[i].startsWith("// [/" + sectionName + "]")) endIndex = i;
	}

	if (startIndex != -1 && (endIndex - startIndex) > 0)
	{
		StringArray section;
		section.addArray(cLines, startIndex + 1, endIndex - startIndex-1);
		return section;
	}

	return StringArray();	
}

void CompileExporter::convertTccScriptsToCppClasses()
{
	File targetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode);

	Array<File> scriptFiles;

	Array<File> convertedCppFiles;

	GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Scripts).findChildFiles(scriptFiles, File::findFiles, true, "*.c");

	if (scriptFiles.size() == 0) 
		return;

	for (int i = 0; i < scriptFiles.size(); i++)
	{
		createCppFileFromTccScript(targetDirectory, scriptFiles[i], convertedCppFiles);
	}

	String output;

	static const String tab = "\t";
	static String sep = newLine;
	static const String namespaceName = "TccConvertedScriptObjects";
	sep << tab;

	output << "/** Autogenerated Factory class for converted TCC scripts. */" << newLine;
	output << newLine;
	output << "#include <JuceHeader.h>" << newLine;
	output << "#include \"AdditionalSourceCode.h\"" << newLine;
	output << newLine;
	output << "namespace " << namespaceName << newLine;
	output << "{" << newLine;

	for (int i = 0; i < convertedCppFiles.size(); i++)
	{
		output << "#include \"" << convertedCppFiles[i].getFileName() << "\"" << newLine;
	}

	output << "}" << newLine;
	output << newLine;
	
	output << "void ConvertedTccScriptFactory::registerModules()" << newLine;
	output << "{" << newLine;
	
	for (int i = 0; i < scriptFiles.size(); i++)
	{
		output << tab << "registerDspModule<" << namespaceName << "::" << scriptFiles[i].getFileNameWithoutExtension().replace("_Tcc", "") << ">();" << newLine;
	}

	output << "}" << newLine;
	
	output << newLine;

	File outputFile = targetDirectory.getChildFile("ConvertedTccScriptFactory.cpp");

	outputFile.deleteFile();

	FileOutputStream fos(outputFile);

	fos.writeText(output, false, false);
}

void CompileExporter::createCppFileFromTccScript(File& targetDirectory, const File &f, Array<File>& convertedList)
{
	String className = f.getFileNameWithoutExtension();

	File outputFile(targetDirectory.getChildFile("_Tcc" + f.getFileNameWithoutExtension() + ".cpp"));

	StringArray sa;

	f.readLines(sa);

	StringArray includes = getTccSection(sa, "INCLUDE SECTION");
	StringArray memberFunctions = getTccSection(sa, "CALLBACK SECTION");
	StringArray privateMembers = getTccSection(sa, "PRIVATE MEMBER SECTION");

	outputFile.deleteFile();

	FileOutputStream fos(outputFile);

	String output;

	static const String tab = "\t";
	static String sep = newLine;
	sep << tab;

	output << "/** Autogenerated CPP file from " << f.getFullPathName() << " */" << newLine;
	output << newLine;
	output << includes.joinIntoString(newLine) << newLine;
	output << newLine;
	output << "class " << className << ": public DspBaseObject, public TccLibraryFunctions" << newLine;
	output << "{" << newLine << "public:" << newLine;
	output << newLine;
	output << tab << "// ============================================================================================" << newLine;
	output << tab << newLine;
	output << tab << className << "() { initialise(); }; " << newLine;
	output << tab << "~" << className << "() { release(); }; " << newLine;
	output << tab << "static Identifier getName() { RETURN_STATIC_IDENTIFIER(\"" << className << "\"); }" << newLine;
	output << tab << newLine;
	output << tab << "// ============================================================================================" << newLine;
	output << tab << newLine;
	output << tab << memberFunctions.joinIntoString(sep);
	output << newLine;
	output << newLine;
	output << tab << "// ============================================================================================" << newLine;
	output << tab << newLine;
	output << "private:" << newLine;
	output << tab << privateMembers.joinIntoString(sep);
	output << newLine;
	output << "};" << newLine;

	output = output.replace("CPP_CONST", "const");

	output = output.replace("CPP_OVERRIDE", "override");

	convertedList.add(outputFile);

	fos.writeText(output, false, false);
}

CompileExporter::ErrorCodes CompileExporter::compileSolution(BuildOption buildOption, TargetTypes types)
{
	BatchFileCreator::createBatchFile(this, buildOption, types);

	File batchFile = BatchFileCreator::getBatchFile(this);
    
#if JUCE_WINDOWS
    
    String command = "\"" + batchFile.getFullPathName() + "\"";
    
#elif JUCE_LINUX

	PresetHandler::showMessageWindow("Batch file created.", "The batch file was created in the build directory.Click OK to open the location");
	String permissionCommand = "chmod +x \"" + batchFile.getFullPathName() + "\"";
    system(permissionCommand.getCharPointer());

	batchFile.getParentDirectory().revealToUser();

#else
    
    String permissionCommand = "chmod +x \"" + batchFile.getFullPathName() + "\"";
    system(permissionCommand.getCharPointer());
    
    String command = "open \"" + batchFile.getFullPathName() + "\"";

#endif
    

#if JUCE_MAC
	if (globalCommandLineExport)
	{
		Logger::writeToLog("Execute" + permissionCommand);
		Logger::writeToLog("Call " + command + " in order to compile the project");

		return ErrorCodes::OK;
	}
#endif

#if JUCE_LINUX
	return ErrorCodes::OK;
#else

	if (!isUsingCIMode())
	{
		int returnType = system(command.getCharPointer());

		if (returnType != 0)
		{
			return ErrorCodes::CompileError;
		}
	}

	

	batchFile.getParentDirectory().getChildFile("temp/").deleteRecursively();

	return ErrorCodes::OK;
#endif
}


class StringObfuscater
{
public:

	static String getStringConcatenationExpression(Random& rng, int start, int length)
	{
		jassert(length > 0);

		if (length == 1)
			return "s" + String(start);

        int randomPart = length > 3 ? rng.nextInt(length / 3) : 0;
        
		int breakPos = jlimit(1, length - 1, (length / 3) + randomPart );

		return "(" + getStringConcatenationExpression(rng, start, breakPos)
			+ " + " + getStringConcatenationExpression(rng, start + breakPos, length - breakPos) + ")";
	};

	static String generateObfuscatedStringCode(const String& args)
	{
		const String originalText(args);

		struct Section
		{
			String text;
			int position, index;

			void writeGenerator(MemoryOutputStream& out) const
			{
				String name("s" + String(index));

				out << "    String " << name << ";  " << name;

				for (int i = 0; i < text.length(); ++i)
					out << " << '" << String::charToString(text[i]) << "'";

				out << ";" << newLine;
			}
		};

		Array<Section> sections;
		String text = originalText;
		Random rng;

		while (text.isNotEmpty())
		{
			int pos = jmax(0, text.length() - (1 + rng.nextInt(6)));
			Section s = { text.substring(pos), pos, 0 };
			sections.insert(0, s);
			text = text.substring(0, pos);
		}

		for (int i = 0; i < sections.size(); ++i)
			sections.getReference(i).index = i;

		for (int i = 0; i < sections.size(); ++i)
			sections.swap(i, rng.nextInt(sections.size()));

		MemoryOutputStream out;

		out << "RSAKey hise::Unlocker::getPublicKey()" << newLine
			<< "{" << newLine;

		for (int i = 0; i < sections.size(); ++i)
			sections.getReference(i).writeGenerator(out);

		out << newLine
			<< "    String result = " << getStringConcatenationExpression(rng, 0, sections.size()) << ";" << newLine
			<< newLine
			<< "    jassert (result == " << originalText.quoted() << ");" << newLine
			<< "    return RSAKey(result);" << newLine
			<< "}" << newLine;

		return out.toString();
	};
};





CompileExporter::ErrorCodes CompileExporter::createPluginDataHeaderFile(const String &solutionDirectory, 
																		const String &publicKey,
                                                                        bool iOSAUv3)
{
	String pluginDataHeaderFile;

	HeaderHelpers::addBasicIncludeLines(pluginDataHeaderFile);

	HeaderHelpers::addAdditionalSourceCodeHeaderLines(this,pluginDataHeaderFile);
	HeaderHelpers::addStaticDspFactoryRegistration(pluginDataHeaderFile, this);
	HeaderHelpers::addCopyProtectionHeaderLines(publicKey, pluginDataHeaderFile);

	if (IS_SETTING_TRUE(HiseSettings::Project::EmbedAudioFiles))
	{
		pluginDataHeaderFile << "AudioProcessor* JUCE_CALLTYPE createPluginFilter() { CREATE_PLUGIN_WITH_AUDIO_FILES(nullptr, nullptr); }\n";
		pluginDataHeaderFile << "\n";
	}
	else
	{
		pluginDataHeaderFile << "AudioProcessor* JUCE_CALLTYPE createPluginFilter() { CREATE_PLUGIN(nullptr, nullptr); }\n";
		pluginDataHeaderFile << "\n";
	}

    if(iOSAUv3)
    {
        pluginDataHeaderFile << "AudioProcessor* hise::StandaloneProcessor::createProcessor() { CREATE_PLUGIN(deviceManager, callback); }\n\n";
        pluginDataHeaderFile << "START_JUCE_APPLICATION(hise::FrontendStandaloneApplication)\n\n";
    }
    else
    {
        pluginDataHeaderFile << "AudioProcessor* hise::StandaloneProcessor::createProcessor() { return nullptr; }\n";
    }
    
	HeaderHelpers::addProjectInfoLines(this, pluginDataHeaderFile);
	HeaderHelpers::addCustomToolbarRegistration(this, pluginDataHeaderFile);
	HeaderHelpers::writeHeaderFile(solutionDirectory, pluginDataHeaderFile);

	return ErrorCodes::OK;
}

CompileExporter::ErrorCodes CompileExporter::createStandaloneAppHeaderFile(const String& solutionDirectory, const String& uniqueId, const String& version, String publicKey)
{
	ignoreUnused(version, uniqueId);

	String pluginDataHeaderFile;

	HeaderHelpers::addBasicIncludeLines(pluginDataHeaderFile);

	HeaderHelpers::addAdditionalSourceCodeHeaderLines(this,pluginDataHeaderFile);
	HeaderHelpers::addStaticDspFactoryRegistration(pluginDataHeaderFile, this);
	HeaderHelpers::addCopyProtectionHeaderLines(publicKey, pluginDataHeaderFile);

	if (GET_SETTING(HiseSettings::Project::EmbedAudioFiles) == "No")
	{
		pluginDataHeaderFile << "AudioProcessor* hise::StandaloneProcessor::createProcessor() { CREATE_PLUGIN(deviceManager, callback); }\n";
		pluginDataHeaderFile << "\n";
		pluginDataHeaderFile << "START_JUCE_APPLICATION(hise::FrontendStandaloneApplication)\n";
	}
	else
	{
		pluginDataHeaderFile << "AudioProcessor* hise::StandaloneProcessor::createProcessor() { CREATE_PLUGIN_WITH_AUDIO_FILES(deviceManager, callback); }\n";
		pluginDataHeaderFile << "\n";
		pluginDataHeaderFile << "START_JUCE_APPLICATION(hise::FrontendStandaloneApplication)\n";
	}

	HeaderHelpers::addProjectInfoLines(this, pluginDataHeaderFile);
	HeaderHelpers::addCustomToolbarRegistration(this, pluginDataHeaderFile);
	HeaderHelpers::writeHeaderFile(solutionDirectory, pluginDataHeaderFile);

	return ErrorCodes::OK;
}

CompileExporter::ErrorCodes CompileExporter::createResourceFile(const String &solutionDirectory, const String & uniqueName, const String &version)
{
	// WRITE THE RESOURCES.RC file

	String resourcesFile;

	resourcesFile << "#undef  WIN32_LEAN_AND_MEAN" << "\n";
	resourcesFile << "#define WIN32_LEAN_AND_MEAN" << "\n";
	resourcesFile << "#include <windows.h>" << "\n";
	resourcesFile << "" << "\n";
	resourcesFile << "VS_VERSION_INFO VERSIONINFO" << "\n";
	resourcesFile << "FILEVERSION " << version.replaceCharacter('.', ',') << "\n";
	resourcesFile << "PRODUCTVERSION " << version.replaceCharacter('.', ',') << "\n";
	resourcesFile << "BEGIN" << "\n";
	resourcesFile << "  BLOCK \"StringFileInfo\"" << "\n";
	resourcesFile << "  BEGIN" << "\n";
	resourcesFile << "    BLOCK \"040904E4\"" << "\n";
	resourcesFile << "    BEGIN" << "\n";
	resourcesFile << "      VALUE \"CompanyName\", \"Hart Instruments\\0\"" << "\n";
	resourcesFile << "      VALUE \"FileDescription\", \"" << uniqueName << "\\0\"" << "\n";
	resourcesFile << "      VALUE \"FileVersion\", \"" << version << "\\0\"" << "\n";
	resourcesFile << "      VALUE \"ProductName\", \"" << uniqueName << "\\0\"" << "\n";
	resourcesFile << "      VALUE \"ProductVersion\", \"" << version << "\\0\"" << "\n";
	resourcesFile << "    END" << "\n";
	resourcesFile << "  END" << "\n";
	resourcesFile << "" << "\n";
	resourcesFile << "  BLOCK \"VarFileInfo\"" << "\n";
	resourcesFile << "  BEGIN" << "\n";
	resourcesFile << "    VALUE \"Translation\", 0x409, 65001" << "\n";
	resourcesFile << "  END" << "\n";
	resourcesFile << "END" << "\n";

    String year = HelperClasses::isUsingVisualStudio2015(dataObject) ? "2015" : "2017";

	File resourcesFileObject(solutionDirectory + "/Builds/VisualStudio" + year + "/resources.rc");

	resourcesFileObject.deleteFile();

	resourcesFileObject.create();
	resourcesFileObject.appendText(resourcesFile);

	return ErrorCodes::OK;
}


#define REPLACE_WILDCARD_WITH_STRING(wildcard, s) (templateProject = templateProject.replace(wildcard, s))
#define REPLACE_WILDCARD(wildcard, settingId) templateProject = templateProject.replace(wildcard, GET_SETTING(settingId));

struct FileHelpers
{
	
	static String createAlphaNumericUID()
	{
		String uid;
		const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		Random r;

		uid << chars[r.nextInt(52)]; // make sure the first character is always a letter

		for (int i = 5; --i >= 0;)
		{
			r.setSeedRandomly();
			uid << chars[r.nextInt(62)];
		}

		return uid;
	}
};



hise::CompileExporter::ErrorCodes CompileExporter::createPluginProjucerFile(TargetTypes type, BuildOption option, ModulatorSynthChain* chain)
{
	String templateProject = String(projectTemplate_jucer);

	REPLACE_WILDCARD("%NAME%", HiseSettings::Project::Name);
	REPLACE_WILDCARD("%VERSION%", HiseSettings::Project::Version);
	REPLACE_WILDCARD("%DESCRIPTION%", HiseSettings::Project::Description);
	REPLACE_WILDCARD("%BUNDLE_ID%", HiseSettings::Project::BundleIdentifier);
	REPLACE_WILDCARD("%PC%", HiseSettings::Project::PluginCode);

	ProjectTemplateHelpers::handleVisualStudioVersion(dataObject,templateProject);

	REPLACE_WILDCARD_WITH_STRING("%CHANNEL_CONFIG%", "");

	REPLACE_WILDCARD_WITH_STRING("%PLUGIN_CHANNEL_AMOUNT%", ProjectTemplateHelpers::getPluginChannelAmount(chain));


	if (type == TargetTypes::EffectPlugin)
	{
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "0");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN", "0");
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "enabled");
        REPLACE_WILDCARD_WITH_STRING("%AAX_CATEGORY%", "AAX_ePlugInCategory_Modulation");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "1");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN", "1");
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
        REPLACE_WILDCARD_WITH_STRING("%AAX_CATEGORY%", "AAX_ePlugInCategory_SWGenerators");
	}

	REPLACE_WILDCARD_WITH_STRING("%IS_STANDALONE_FRONTEND%", "disabled");

	ProjectTemplateHelpers::handleCompanyInfo(this, templateProject);

	if (BuildOptionHelpers::isIOS(option))
	{
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AU%", "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_VST%", "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AAX%", "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AUV3%", "1");

		REPLACE_WILDCARD_WITH_STRING("%VSTSDK_FOLDER", String());
		REPLACE_WILDCARD_WITH_STRING("%AAX_PATH%", String());
		REPLACE_WILDCARD_WITH_STRING("%AAX_RELEASE_LIB%", String());
		REPLACE_WILDCARD_WITH_STRING("%AAX_DEBUG_LIB%", String());
		REPLACE_WILDCARD_WITH_STRING("%AAX_IDENTIFIER%", String());
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FAMILY%", ProjectTemplateHelpers::getTargetFamilyString(option));

		const File sampleFolder = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::Samples);

		String iOSResourceFile;

		bool hasCustomiOSFolder = sampleFolder.getChildFile("iOS").isDirectory();

		if (hasCustomiOSFolder)
		{
			bool hasCustomiOSSampleFolder = sampleFolder.getChildFile("Samples").isDirectory();

			if (hasCustomiOSSampleFolder)
				iOSResourceFile = sampleFolder.getChildFile("iOS/Samples").getFullPathName();
			else
				iOSResourceFile = sampleFolder.getFullPathName();
		}
		else
			iOSResourceFile = sampleFolder.getFullPathName();


		REPLACE_WILDCARD_WITH_STRING("%IOS_SAMPLE_FOLDER%", iOSResourceFile);

		const File imageFolder = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::Images);
		const File audioFolder = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles);
		const File sampleMapFolder = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);

		REPLACE_WILDCARD_WITH_STRING("%IOS_IMAGE_FOLDER%", imageFolder.getFullPathName());
		REPLACE_WILDCARD_WITH_STRING("%IOS_AUDIO_FOLDER%", audioFolder.getFullPathName());
		REPLACE_WILDCARD_WITH_STRING("%IOS_SAMPLEMAP_FOLDER%", sampleMapFolder.getFullPathName());

		const String appGroupId = GET_SETTING(HiseSettings::Project::AppGroupID);
        
        REPLACE_WILDCARD_WITH_STRING("%USE_APP_GROUPS%", appGroupId.isEmpty() ? "0" : "1");
        REPLACE_WILDCARD_WITH_STRING("%APP_GROUP_ID%",  appGroupId);
        
		REPLACE_WILDCARD("%DEVELOPMENT_TEAM_ID%", HiseSettings::User::TeamDevelopmentID);
        
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AUV3%", "0");

		const bool buildAU = BuildOptionHelpers::isAU(option);
		const bool buildVST = BuildOptionHelpers::isVST(option);
		const bool buildAAX = BuildOptionHelpers::isAAX(option);

		REPLACE_WILDCARD_WITH_STRING("%BUILD_AU%", buildAU ? "1" : "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_VST%", buildVST ? "1" : "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AAX%", buildAAX ? "1" : "0");

		const File vstSDKPath = hisePath.getChildFile("tools/SDK/VST3 SDK");

		if (buildVST && !vstSDKPath.isDirectory())
		{
			return ErrorCodes::VSTSDKMissing;
		}

		if (buildVST)
			REPLACE_WILDCARD_WITH_STRING("%VSTSDK_FOLDER%", vstSDKPath.getFullPathName());
		else
			REPLACE_WILDCARD_WITH_STRING("%VSTSDK_FOLDER", String());

		if (buildAAX)
		{
			const File aaxPath = hisePath.getChildFile("tools/SDK/AAX");

			if (!aaxPath.isDirectory())
			{
				return ErrorCodes::AAXSDKMissing;
			}

			REPLACE_WILDCARD_WITH_STRING("%AAX_PATH%", aaxPath.getFullPathName());
			REPLACE_WILDCARD_WITH_STRING("%AAX_RELEASE_LIB%", aaxPath.getChildFile("Libs/Release/").getFullPathName());
			REPLACE_WILDCARD_WITH_STRING("%AAX_DEBUG_LIB%", aaxPath.getChildFile("Libs/Debug/").getFullPathName());

			String aaxIdentifier = "com.";
			aaxIdentifier << GET_SETTING(HiseSettings::User::Company).removeCharacters(" -_.,;");
			aaxIdentifier << "." << GET_SETTING(HiseSettings::Project::Name).removeCharacters(" -_.,;");

			REPLACE_WILDCARD_WITH_STRING("%AAX_IDENTIFIER%", aaxIdentifier);
		}
		else
		{
			REPLACE_WILDCARD_WITH_STRING("%AAX_PATH%", String());
			REPLACE_WILDCARD_WITH_STRING("%AAX_RELEASE_LIB%", String());
			REPLACE_WILDCARD_WITH_STRING("%AAX_DEBUG_LIB%", String());
			REPLACE_WILDCARD_WITH_STRING("%AAX_IDENTIFIER%", String());
		}
	}

	ProjectTemplateHelpers::handleCompilerInfo(this, templateProject);

	ProjectTemplateHelpers::handleAdditionalSourceCode(this, templateProject, option);
	ProjectTemplateHelpers::handleCopyProtectionInfo(this, templateProject, option);

	return HelperClasses::saveProjucerFile(templateProject, this);
}

hise::CompileExporter::CompileExporter::ErrorCodes CompileExporter::createStandaloneAppProjucerFile(BuildOption option)
{
	String templateProject = String(projectStandaloneTemplate_jucer);

	auto name = GET_SETTING(HiseSettings::Project::Name);

	REPLACE_WILDCARD("%NAME%", HiseSettings::Project::Name);
	REPLACE_WILDCARD("%VERSION%", HiseSettings::Project::Version);
	REPLACE_WILDCARD("%BUNDLE_ID%", HiseSettings::Project::BundleIdentifier);

	const File asioPath = hisePath.getChildFile("tools/SDK/ASIOSDK2.3/common");

#if JUCE_WINDOWS

	if (!asioPath.isDirectory()) return ErrorCodes::ASIOSDKMissing;

	REPLACE_WILDCARD_WITH_STRING("%USE_ASIO%", "enabled");
	REPLACE_WILDCARD_WITH_STRING("%ASIO_SDK_PATH%", asioPath.getFullPathName());

#else
	REPLACE_WILDCARD_WITH_STRING("%USE_ASIO%", "disabled");
	REPLACE_WILDCARD_WITH_STRING("%ASIO_SDK_PATH%", String());

#endif

	REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
	REPLACE_WILDCARD_WITH_STRING("%IS_STANDALONE_FRONTEND%", "enabled");

    

	ProjectTemplateHelpers::handleVisualStudioVersion(dataObject,templateProject);

	ProjectTemplateHelpers::handleCompanyInfo(this, templateProject);
	ProjectTemplateHelpers::handleCompilerInfo(this, templateProject);

	ProjectTemplateHelpers::handleAdditionalSourceCode(this, templateProject, option);
	ProjectTemplateHelpers::handleCopyProtectionInfo(this, templateProject, option);

	return HelperClasses::saveProjucerFile(templateProject, this);
}


void CompileExporter::ProjectTemplateHelpers::handleCompilerInfo(CompileExporter* exporter, String& templateProject)
{
	const File jucePath = exporter->hisePath.getChildFile("JUCE/modules");

	REPLACE_WILDCARD_WITH_STRING("%HISE_PATH%", exporter->hisePath.getFullPathName());
	REPLACE_WILDCARD_WITH_STRING("%JUCE_PATH%", jucePath.getFullPathName());
	
    REPLACE_WILDCARD_WITH_STRING("%USE_IPP%", exporter->useIpp ? "enabled" : "disabled");
    REPLACE_WILDCARD_WITH_STRING("%IPP_WIN_SETTING%", exporter->useIpp ? "Sequential" : String());
    
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_WIN%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsWindows).toString());
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_OSX%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsOSX).toString());
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_IOS%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsIOS).toString());

	REPLACE_WILDCARD_WITH_STRING("%COPY_PLUGIN%", isUsingCIMode() ? "0" : "1");

#if JUCE_MAC
	REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a");
	REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", "/opt/intel/ipp/include");
	REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", "/opt/intel/ipp/lib");
#else
	REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", String());
#endif
}

void CompileExporter::ProjectTemplateHelpers::handleCompanyInfo(CompileExporter* exporter, String& templateProject)
{
	REPLACE_WILDCARD_WITH_STRING("%COMPANY%", exporter->dataObject.getSetting(HiseSettings::User::Company).toString());
	REPLACE_WILDCARD_WITH_STRING("%MC%", exporter->dataObject.getSetting(HiseSettings::User::CompanyCode).toString());
	REPLACE_WILDCARD_WITH_STRING("%COMPANY_WEBSITE%", exporter->dataObject.getSetting(HiseSettings::User::CompanyURL).toString());
	REPLACE_WILDCARD_WITH_STRING("%COMPANY_COPYRIGHT%", exporter->dataObject.getSetting(HiseSettings::User::CompanyCopyright).toString());
    REPLACE_WILDCARD_WITH_STRING("%COPYRIGHT_NOTICE%", exporter->dataObject.getSetting(HiseSettings::User::CompanyCopyright).toString());
}

void CompileExporter::ProjectTemplateHelpers::handleVisualStudioVersion(const HiseSettings::Data& dataObject, String& templateProject)
{
	const bool isUsingVisualStudio2015 = HelperClasses::isUsingVisualStudio2015(dataObject);

	if (isUsingVisualStudio2015)
	{
		REPLACE_WILDCARD_WITH_STRING("%VS_VERSION%", "VS2015");
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FOLDER%", "VisualStudio2015");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%VS_VERSION%", "VS2017");
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FOLDER%", "VisualStudio2017");
	}
}



void CompileExporter::ProjectTemplateHelpers::handleAdditionalSourceCode(CompileExporter* exporter, String &templateProject, BuildOption option)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

	Array<File> additionalSourceFiles;

	File additionalSourceCodeDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode);

	File additionalMainHeaderFile = additionalSourceCodeDirectory.getChildFile("AdditionalSourceCode.h");

	if (!BuildOptionHelpers::isIOS(option) && additionalMainHeaderFile.existsAsFile())
	{
		additionalSourceFiles.add(additionalMainHeaderFile);
	}

#if JUCE_WINDOWS

	File resourceFile = additionalSourceCodeDirectory.getChildFile("resources.rc");

	if (resourceFile.existsAsFile())
	{
		File resourceTargetFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("Builds/VisualStudio2015/resources.rc");

		const String command = "copy &quot;" + resourceFile.getFullPathName() + "&quot; &quot;" + resourceTargetFile.getFullPathName() + "&quot;";

		REPLACE_WILDCARD_WITH_STRING("%PREBUILD_COMMAND%", command);
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%PREBUILD_COMMAND%", "");
	}

#endif

	File copyProtectionCppFile = additionalSourceCodeDirectory.getChildFile("CopyProtection.cpp");

	File copyProtectionTargetFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("Source/CopyProtection.cpp");

	if (copyProtectionCppFile.existsAsFile())
	{
		copyProtectionCppFile.copyFileTo(copyProtectionTargetFile);
	}
	else
	{
		copyProtectionTargetFile.create();
	}

	File turboActivateFile = additionalSourceCodeDirectory.getChildFile("TurboActivate.dat");

	if (turboActivateFile.existsAsFile())
		additionalSourceFiles.add(turboActivateFile);

	File iconFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("Icon.png");

	if (iconFile.existsAsFile())
	{
		additionalSourceFiles.add(iconFile);
	}
	else
	{
		templateProject = templateProject.replace("%ICON_FILE%", "");
	}

	File splashScreenFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("SplashScreen.png");

    File splashScreeniPhoneFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("SplashScreeniPhone.png");
    
	if (splashScreenFile.existsAsFile() || splashScreeniPhoneFile.existsAsFile())
	{
        if(splashScreenFile.existsAsFile())
        {
            additionalSourceFiles.add(splashScreenFile);
        }
		
        if(splashScreeniPhoneFile.existsAsFile())
        {
            additionalSourceFiles.add(splashScreeniPhoneFile);
        }
		
		REPLACE_WILDCARD_WITH_STRING("%USE_SPLASH_SCREEN%", "enabled");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%USE_SPLASH_SCREEN%", "disabled");
	}

#if JUCE_MAC
    
    
    const String additionalStaticLibs = exporter->GET_SETTING(HiseSettings::Project::OSXStaticLibs);
    templateProject = templateProject.replace("%OSX_STATIC_LIBS%", additionalStaticLibs);
#else

	auto additionalStaticLibFolder = exporter->GET_SETTING(HiseSettings::Project::WindowsStaticLibFolder);

	

	if (additionalStaticLibFolder.isNotEmpty())
	{
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_D64%", additionalStaticLibFolder + "/Debug_x64");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_R64%", additionalStaticLibFolder + "/Release_x64");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_D32%", additionalStaticLibFolder + "/Debug_x86");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_R32%", additionalStaticLibFolder + "/Release_x86");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_D64%", "");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_R64%", "");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_D32%", "");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_R32%", "");
	}
#endif
    
	if (additionalSourceFiles.size() != 0)
	{
		StringArray additionalFileDefinitions;

		for (int i = 0; i < additionalSourceFiles.size(); i++)
		{
			if (additionalSourceFiles[i].getFileName().startsWith("_Tcc"))
			{
				// Will be included in the Factory .cpp source file
				continue;
			}

			bool isSourceFile = additionalSourceFiles[i].hasFileExtension(".cpp");

            bool isSplashScreen = additionalSourceFiles[i].getFileName().contains("SplashScreen");

			bool isTurboActivate = (additionalSourceFiles[i].getFileName() == "TurboActivate.dat");

			const String relativePath = additionalSourceFiles[i].getRelativePathFrom(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries));

			String newAditionalSourceLine;
            
            String fileId = FileHelpers::createAlphaNumericUID();
            
            if(additionalSourceFiles[i].getFileName() == "Icon.png")
            {
                templateProject = templateProject.replace("%ICON_FILE%", "smallIcon=\"" + fileId + "\" bigIcon=\"" + fileId + "\"");
            }
            
			newAditionalSourceLine << "      <FILE id=\"" << fileId << "\" name=\"" << additionalSourceFiles[i].getFileName() << "\" compile=\"" << (isSourceFile ? "1" : "0") << "\" ";
			newAditionalSourceLine << "resource=\"" << (isSplashScreen || isTurboActivate ? "1" : "0") << "\"\r\n";
			newAditionalSourceLine << "            file=\"" << relativePath << "\"/>\r\n";

			additionalFileDefinitions.add(newAditionalSourceLine);
		}

        
        
        
        
		templateProject = templateProject.replace("%ADDITIONAL_FILES%", additionalFileDefinitions.joinIntoString(""));

		templateProject = templateProject.replace("%USE_CUSTOM_FRONTEND_TOOLBAR%", "disabled");
        
	}
    else
    {
        templateProject = templateProject.replace("%ADDITIONAL_FILES%", "");
    }

}

void CompileExporter::ProjectTemplateHelpers::handleCopyProtectionInfo(CompileExporter* exporter, String &templateProject, BuildOption option)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

    const bool useCopyProtection = !BuildOptionHelpers::isIOS(option) && GET_PROJECT_HANDLER(chainToExport).getPublicKey().isNotEmpty();

	templateProject = templateProject.replace("%USE_COPY_PROTECTION%", useCopyProtection ? "enabled" : "disabled");
}

String CompileExporter::ProjectTemplateHelpers::getTargetFamilyString(BuildOption option)
{
	const bool isIPad = BuildOptionHelpers::isIPad(option);
	const bool isIPhone = BuildOptionHelpers::isIPhone(option);

	if (isIPad && isIPhone)
		return "1,2";
	else if (isIPad)
		return "2";
	else if (isIPhone)
		return "1";
	else
	{
		jassertfalse;
		return "";
	}
}

juce::String CompileExporter::ProjectTemplateHelpers::getPluginChannelAmount(ModulatorSynthChain* chain)
{
	return "HISE_NUM_PLUGIN_CHANNELS=" + String(chain->getMatrix().getNumSourceChannels());
}

CompileExporter::ErrorCodes CompileExporter::copyHISEImageFiles()
{
	File imageDirectory = hisePath.getChildFile("hi_core/hi_images/");
	File targetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("Source/Images/");

	targetDirectory.createDirectory();

	if (!imageDirectory.isDirectory())
	{
		return ErrorCodes::HISEImageDirectoryNotFound;
	}

	if (!imageDirectory.copyDirectoryTo(targetDirectory)) return ErrorCodes::HISEImageDirectoryNotFound;

	return ErrorCodes::OK;
}

File CompileExporter::getProjucerProjectFile()
{
	return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("AutogeneratedProject.jucer");
}










#undef REPLACE_WILDCARD

//==============================================================================
int CppBuilder::addFile(const File& file,
	const String& classname,
	OutputStream& headerStream,
	OutputStream& cppStream)
{
	MemoryBlock mb;
	file.loadFileAsData(mb);

	const String name(file.getFileName()
		.replaceCharacter(' ', '_')
		.replaceCharacter('.', '_')
		.retainCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789"));

	std::cout << "Adding " << name << ": "
		<< (int)mb.getSize() << " bytes" << std::endl;

	headerStream << "    extern const char*  " << name << ";\r\n"
		"    const int           " << name << "Size = "
		<< (int)mb.getSize() << ";\r\n\r\n";

	static int tempNum = 0;

	cppStream << "static const unsigned char temp" << ++tempNum << "[] = {";

	size_t i = 0;
	const uint8* const data = (const uint8*)mb.getData();

	while (i < mb.getSize() - 1)
	{
		if ((i % 40) != 39)
			cppStream << (int)data[i] << ",";
		else
			cppStream << (int)data[i] << ",\r\n  ";

		++i;
	}

	cppStream << (int)data[i] << ",0,0};\r\n";

	cppStream << "const char* " << classname << "::" << name
		<< " = (const char*) temp" << tempNum << ";\r\n\r\n";

	return (int)mb.getSize();
}

bool CppBuilder::isHiddenFile(const File& f, const File& root)
{
	return f.getFileName().endsWithIgnoreCase(".scc")
		|| f.getFileName() == ".svn"
		|| f.getFileName().startsWithChar('.')
		|| (f.getSize() == 0 && !f.isDirectory())
		|| (f.getParentDirectory() != root && isHiddenFile(f.getParentDirectory(), root));
}


//==============================================================================
int CppBuilder::exportValueTreeAsCpp(const File &sourceDirectory, const File &destDirectory, String &className)
{
	className = className.trim();

	const File headerFile(destDirectory.getChildFile(className).withFileExtension(".h"));
	const File cppFile(destDirectory.getChildFile(className).withFileExtension(".cpp"));

	std::cout << "Creating " << headerFile.getFullPathName()
		<< " and " << cppFile.getFullPathName()
		<< " from files in " << sourceDirectory.getFullPathName()
		<< "..." << std::endl << std::endl;

	headerFile.deleteFile();
	cppFile.deleteFile();

	Array<File> files;
	sourceDirectory.findChildFiles(files, File::findFiles, false, "*");

	ScopedPointer<OutputStream> header(headerFile.createOutputStream());

	if (header == nullptr)
	{
		std::cout << "Couldn't open "
			<< headerFile.getFullPathName() << " for writing" << std::endl << std::endl;
		return 0;
	}

	ScopedPointer<OutputStream> cpp(cppFile.createOutputStream());

	if (cpp == nullptr)
	{
		std::cout << "Couldn't open "
			<< cppFile.getFullPathName() << " for writing" << std::endl << std::endl;
		return 0;
	}

	*header << "/* (Auto-generated binary data file). */\r\n\r\n"
		"#ifndef BINARY_" << className.toUpperCase() << "_H\r\n"
		"#define BINARY_" << className.toUpperCase() << "_H\r\n\r\n"
		"namespace " << className << "\r\n"
		"{\r\n";

	*cpp << "/* (Auto-generated binary data file). */\r\n\r\n"
		"#include \"" << className << ".h\"\r\n\r\n";

	int totalBytes = 0;

	for (int i = 0; i < files.size(); ++i)
	{
		const File file(files[i]);

		// (avoid source control files and hidden files..)
		if (!isHiddenFile(file, sourceDirectory))
		{
			if (file.getParentDirectory() != sourceDirectory)
			{
				*header << "  #ifdef " << file.getParentDirectory().getFileName().toUpperCase() << "\r\n";
				*cpp << "#ifdef " << file.getParentDirectory().getFileName().toUpperCase() << "\r\n";

				totalBytes += addFile(file, className, *header, *cpp);

				*header << "  #endif\r\n";
				*cpp << "#endif\r\n";
			}
			else
			{
				totalBytes += addFile(file, className, *header, *cpp);
			}
		}
	}

	*header << "}\r\n\r\n"
		"#endif\r\n";

	header = nullptr;
	cpp = nullptr;

	std::cout << std::endl << " Total size of binary data: " << totalBytes << " bytes" << std::endl;

	return 0;
}

#define ADD_LINE(x) (batchContent << x << NewLine::getDefault())

void CompileExporter::BatchFileCreator::createBatchFile(CompileExporter* exporter, BuildOption buildOption, TargetTypes types)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

	File batchFile = getBatchFile(exporter);

	if (!exporter->isExportingFromCommandLine() && batchFile.existsAsFile())
	{
		if (!PresetHandler::showYesNoWindow("Batch File already found", "Do you want to rewrite the batch file for the compile process?"))
		{
			return;
		}
	}

    batchFile.deleteFile();
    
	const String buildPath = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getFullPathName();

    String batchContent;
    
	const String projectName = exporter->GET_SETTING(HiseSettings::Project::Name);
    
	String projectType;

	switch (types)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin: projectType = "Instrument plugin"; break;
	case CompileExporter::TargetTypes::EffectPlugin: projectType = "FX plugin"; break;
	case CompileExporter::TargetTypes::StandaloneApplication: projectType = "Standalone application"; break;
    case CompileExporter::TargetTypes::numTargetTypes: break;
	}

#if JUCE_WINDOWS
    
	const String msbuildPath = HelperClasses::isUsingVisualStudio2015(exporter->dataObject) ? "\"C:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MsBuild.exe\"" :
																	      "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\MSBuild\\15.0\\Bin\\MsBuild.exe\"";

	const String projucerPath = exporter->hisePath.getChildFile("tools/Projucer/Projucer.exe").getFullPathName();
	
	const String vsArgs = "/p:Configuration=\"Release\" /verbosity:minimal";

	const String vsFolder = HelperClasses::isUsingVisualStudio2015(exporter->dataObject) ? "VisualStudio2015" : "VisualStudio2017";


	ADD_LINE("@echo off");
	ADD_LINE("set project=" << projectName);
	ADD_LINE("set build_path=" << buildPath);
	ADD_LINE("set msbuild=" << msbuildPath);
	ADD_LINE("set vs_args=" << vsArgs);
#if JUCE_64BIT
    ADD_LINE("set PreferredToolArchitecture=x64");
#endif

	if (HelperClasses::isUsingVisualStudio2015(exporter->dataObject))
	{
		ADD_LINE("set VisualStudioVersion=14.0");
	}
	else
	{
		ADD_LINE("set VisualStudioVersion=15.0");
	}

	ADD_LINE("");
	ADD_LINE("\"" << projucerPath << "\" --resave \"%build_path%\\AutogeneratedProject.jucer\"");
	ADD_LINE("");

	if (BuildOptionHelpers::is32Bit(buildOption))
	{
		ADD_LINE("echo Compiling 32bit " << projectType << " %project% ...");
		ADD_LINE("set Platform=Win32");
		ADD_LINE("%msbuild% \"%build_path%\\Builds\\" << vsFolder << "\\%project%.sln\" %vs_args%");

		ADD_LINE("");

		if (isUsingCIMode())
		{
			ADD_LINE("if %errorlevel% NEQ 0 (");
			ADD_LINE("  echo Compile error at " << projectType);
			ADD_LINE("  exit 1");
			ADD_LINE(")");
		}

		
		ADD_LINE("");
	}
	
	if (BuildOptionHelpers::is64Bit(buildOption))
	{
		ADD_LINE("echo Compiling 64bit " << projectType << " %project% ...");
		ADD_LINE("set Platform=X64");
		ADD_LINE("%msbuild% \"%build_path%\\Builds\\" << vsFolder << "\\%project%.sln\" %vs_args%");
		
		ADD_LINE("");

		if (isUsingCIMode())
		{
			ADD_LINE("if %errorlevel% NEQ 0 (");
			ADD_LINE("  echo Compile error at " << projectType);
			ADD_LINE("  exit 1");
			ADD_LINE(")");
		}

		ADD_LINE("");
	}

	ADD_LINE("");

	if (!CompileExporter::isExportingFromCommandLine())
		ADD_LINE("pause");
	
    batchFile.replaceWithText(batchContent);
    
	

#elif JUCE_LINUX

	const String projucerPath = exporter->hisePath.getChildFile("tools/projucer/Projucer").getFullPathName();

	ADD_LINE("\"" << projucerPath << "\" --resave AutogeneratedProject.jucer");
	ADD_LINE("cd Builds/LinuxMakefile/");
	ADD_LINE("echo Compiling " << projectType << " " << projectName << " ...");
    ADD_LINE("make CONFIG=Release AR=gcc-ar");
	ADD_LINE("echo Compiling finished. Cleaning up...");

	File tempFile = batchFile.getSiblingFile("tempBatch");
    
    tempFile.create();
    tempFile.replaceWithText(batchContent);
    
    String lineEndChange = "tr -d '\r' < \""+tempFile.getFullPathName()+"\" > \"" + batchFile.getFullPathName() + "\"";
    
    system(lineEndChange.getCharPointer());
    
    tempFile.deleteFile();


#else
    
	const String projucerPath = exporter->hisePath.getChildFile("tools/Projucer/Projucer.app/Contents/MacOS/Projucer").getFullPathName();

    ADD_LINE("cd \"`dirname \"$0\"`\"");
    ADD_LINE("\"" << projucerPath << "\" --resave AutogeneratedProject.jucer");
    ADD_LINE("");
    
    if(BuildOptionHelpers::isIOS(buildOption))
    {
        ADD_LINE("echo Sucessfully exported. Open XCode to compile and transfer to your iOS device.");
    }
    else
    {
        ADD_LINE("echo Compiling " << projectType << " " << projectName << " ...");

		String xcodeLine;
		xcodeLine << "xcodebuild -project \"Builds/MacOSX/" << projectName << ".xcodeproj\" -configuration \"Release\"";

		if (!isUsingCIMode())
		{
			xcodeLine << " | xcpretty";
		}

        ADD_LINE(xcodeLine);
        ADD_LINE("echo Compiling finished. Cleaning up...");
    }
    
    File tempFile = batchFile.getSiblingFile("tempBatch");
    
    tempFile.create();
    tempFile.replaceWithText(batchContent);
    
    String lineEndChange = "tr -d '\r' < \""+tempFile.getFullPathName()+"\" > \"" + batchFile.getFullPathName() + "\"";
    
    system(lineEndChange.getCharPointer());
    
    tempFile.deleteFile();
    
#endif
}

#undef ADD_LINE

File CompileExporter::BatchFileCreator::getBatchFile(CompileExporter* exporter)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

#if JUCE_WINDOWS
	return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("batchCompile.bat");
#elif JUCE_LINUX
	return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("batchCompileLinux.sh");
#else
    return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("batchCompileOSX");
#endif
}

juce::String CompileExporter::HelperClasses::getFileNameForCompiledPlugin(const HiseSettings::Data& dataObject, ModulatorSynthChain* chain, BuildOption option)
{
	File directory = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("Compiled");

	auto name = GET_SETTING(HiseSettings::Project::Name);

	String suffix;

	switch (option)
	{
	case CompileExporter::VSTWindowsx86:
		suffix = " x86.dll";
		break;
	case CompileExporter::VSTWindowsx64:
		suffix = " x64.dll";
		break;
	case CompileExporter::VSTWindowsx64x86:
		suffix = " x64.dll";
		break;
	case CompileExporter::VSTiWindowsx86:
		suffix = " x86.dll";
		break;
	case CompileExporter::VSTiWindowsx64:
		suffix = " x64.dll";
		break;
	case CompileExporter::VSTiWindowsx64x86:
		suffix = " x64.dll";
		break;
	case CompileExporter::AUmacOS:
		suffix = ".component";
		break;
	case CompileExporter::VSTmacOS:
		suffix = ".vst";
		break;
	case CompileExporter::VSTAUmacOS:
		suffix = ".component";
		break;
	case CompileExporter::AUimacOS:
		suffix = ".component";
		break;
	case CompileExporter::VSTimacOS:
		suffix = ".vst";
		break;
	case CompileExporter::VSTiAUimacOS:
		suffix = ".component";
		break;
	default:
		break;
	}

	if (suffix.isNotEmpty())
	{
		return directory.getChildFile(name + suffix).getFullPathName();
	}

	return String();
}

bool CompileExporter::HelperClasses::isUsingVisualStudio2015(const HiseSettings::Data& dataObject)
{
	// Always use VS2017 in CI mode
	if (isUsingCIMode())
		return false;

	const String v = GET_SETTING(HiseSettings::Compiler::VisualStudioVersion);

	return v.isEmpty() || (v == "Visual Studio 2015");
}

CompileExporter::ErrorCodes CompileExporter::HelperClasses::saveProjucerFile(String templateProject, CompileExporter* exporter)
{
	XmlDocument doc(templateProject);

	ScopedPointer<XmlElement> xml = doc.getDocumentElement();

	jassert(xml != nullptr);

	if (xml != nullptr)
	{
		File projectFile = exporter->getProjucerProjectFile();

		projectFile.create();
		projectFile.replaceWithText(xml->createDocument(""));
	}
	else
	{
		PresetHandler::showMessageWindow("XML Parsing Error", doc.getLastParseError(), PresetHandler::IconType::Error);

		return ErrorCodes::ProjectXmlInvalid;
	}

	return ErrorCodes::OK;
}

void CompileExporter::HeaderHelpers::addBasicIncludeLines(String& pluginDataHeaderFile)
{
	pluginDataHeaderFile << "\n";

	pluginDataHeaderFile << "#include \"JuceHeader.h\"" << "\n";
	pluginDataHeaderFile << "#include \"PresetData.h\"\n";
}

void CompileExporter::HeaderHelpers::addAdditionalSourceCodeHeaderLines(CompileExporter* exporter, String& pluginDataHeaderFile)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

	File addSourceFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode).getChildFile("AdditionalSourceCode.h");

	if (addSourceFile.existsAsFile())
	{
		pluginDataHeaderFile << "#include \"../../AdditionalSourceCode/AdditionalSourceCode.h\"\n";
	}

	pluginDataHeaderFile << "\n";
}

void CompileExporter::HeaderHelpers::addStaticDspFactoryRegistration(String& pluginDataHeaderFile, CompileExporter* exporter)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

	pluginDataHeaderFile << "REGISTER_STATIC_DSP_LIBRARIES()" << "\n";
	pluginDataHeaderFile << "{" << "\n";
	pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(hise::HiseCoreDspFactory);" << "\n";

	File tccConvertedFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode).getChildFile("ConvertedTccScriptFactory.cpp");

	if (tccConvertedFile.existsAsFile())
	{
		pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(ConvertedTccScriptFactory);" << "\n";
	}

	const String additionalDspClasses = exporter->GET_SETTING(HiseSettings::Project::AdditionalDspLibraries);

	if (additionalDspClasses.isNotEmpty())
	{
		StringArray sa = StringArray::fromTokens(additionalDspClasses, ",;", "");

		for (int i = 0; i < sa.size(); i++)
			pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(" + sa[i] + ");" << "\n";
	}
    
	pluginDataHeaderFile << "}" << "\n";
}

void CompileExporter::HeaderHelpers::addCopyProtectionHeaderLines(const String &publicKey, String& pluginDataHeaderFile)
{
	if (publicKey.isNotEmpty())
	{
		pluginDataHeaderFile << "#if USE_COPY_PROTECTION" << "\n";

		pluginDataHeaderFile << StringObfuscater::generateObfuscatedStringCode(publicKey);
		//pluginDataHeaderFile << "RSAKey Unlocker::getPublicKey() { return RSAKey(String(PUBLIC_KEY)); };" << "\n"; 
		pluginDataHeaderFile << "#endif" << "\n";
	}
	else
	{
		pluginDataHeaderFile << "#if USE_COPY_PROTECTION" << "\n";
		pluginDataHeaderFile << "RSAKey hise::Unlocker::getPublicKey() { return RSAKey(\"\"); };" << "\n";
		pluginDataHeaderFile << "#endif" << "\n";
	}
}

void CompileExporter::HeaderHelpers::addCustomToolbarRegistration(CompileExporter* /*exporter*/, String& /*pluginDataHeaderFile*/)
{}

void CompileExporter::HeaderHelpers::addProjectInfoLines(CompileExporter* exporter, String& pluginDataHeaderFile)
{
	const String companyName = exporter->GET_SETTING(HiseSettings::User::Company);
	const String companyWebsiteName = exporter->GET_SETTING(HiseSettings::User::CompanyURL);
	const String projectName = exporter->GET_SETTING(HiseSettings::Project::Name);
	const String versionString = exporter->GET_SETTING(HiseSettings::Project::Version);
	const String appGroupString = exporter->GET_SETTING(HiseSettings::Project::AppGroupID);

	pluginDataHeaderFile << "String hise::ProjectHandler::Frontend::getProjectName() { return \"" << projectName << "\"; };\n";
	pluginDataHeaderFile << "String hise::ProjectHandler::Frontend::getCompanyName() { return \"" << companyName << "\"; };\n";
	pluginDataHeaderFile << "String hise::ProjectHandler::Frontend::getCompanyWebsiteName() { return \"" << companyWebsiteName << "\"; };\n";
	pluginDataHeaderFile << "String hise::ProjectHandler::Frontend::getVersionString() { return \"" << versionString << "\"; };\n";
    
    pluginDataHeaderFile << "String hise::ProjectHandler::Frontend::getAppGroupId() { return \"" << appGroupString << "\"; };\n";
    
}

void CompileExporter::HeaderHelpers::writeHeaderFile(const String & solutionDirectory, const String& pluginDataHeaderFile)
{
	File pluginDataHeader = File(solutionDirectory).getChildFile("Source/Plugin.cpp");

	pluginDataHeader.create();
	pluginDataHeader.replaceWithText(pluginDataHeaderFile);
}

void CompileExporter::BuildOptionHelpers::runUnitTests()
{
	jassert(BuildOptionHelpers::is32Bit(VSTWindowsx86));
	jassert(!BuildOptionHelpers::is64Bit(VSTWindowsx86));
	jassert(BuildOptionHelpers::isVST(VSTWindowsx64x86));
	jassert(!BuildOptionHelpers::isStandalone(VSTWindowsx64x86));
	jassert(BuildOptionHelpers::isEffect(VSTWindowsx64x86));
	jassert(BuildOptionHelpers::is64Bit(VSTWindowsx64x86));
	jassert(!BuildOptionHelpers::isStandalone(AUmacOS));
	jassert(BuildOptionHelpers::isOSX(AUimacOS));
	jassert(BuildOptionHelpers::isInstrument(VSTimacOS));
	jassert(BuildOptionHelpers::isVST(VSTiAUimacOS));
	jassert(BuildOptionHelpers::isAAX(AAXWindowsx64));
	jassert(BuildOptionHelpers::isStandalone(StandaloneWindowsx64));
	jassert(BuildOptionHelpers::is32Bit(StandaloneWindowsx64x86));
	jassert(BuildOptionHelpers::is64Bit(StandaloneWindowsx64x86));
	jassert(BuildOptionHelpers::isStandalone(StandaloneWindowsx64x86));
}

} // namespace hise
