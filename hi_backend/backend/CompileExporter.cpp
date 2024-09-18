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



ValueTree BaseExporter::exportUserPresetFiles()
{
	File presetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

    ValueTree userPresets("UserPresets");
    
	for(auto e: RangedDirectoryIterator(presetDirectory, true, "*", File::findFiles))
    {
		File f = e.getFile();

        if(f.isHidden())
            continue;
        
#if JUCE_WINDOWS
		if (f.getFileName().startsWith("."))
			continue;
#endif

		XmlDocument doc(f);

		if (auto xml = doc.getDocumentElement())
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

juce::ValueTree BaseExporter::exportEmbeddedFiles()
{
	ValueTree externalScriptFiles = FileChangeListener::collectAllScriptFiles(chainToExport);
	ValueTree customFonts = chainToExport->getMainController()->exportCustomFontsAsValueTree();
	ValueTree markdownDocs = chainToExport->getMainController()->exportAllMarkdownDocsAsValueTree();
	ValueTree networkFiles = BackendDllManager::exportAllNetworks(chainToExport->getMainController(), false);

	ValueTree webViewResources = chainToExport->getMainController()->exportWebViewResources();

	ValueTree externalFiles("ExternalFiles");
	externalFiles.addChild(externalScriptFiles, -1, nullptr);
	externalFiles.addChild(customFonts, -1, nullptr);
	externalFiles.addChild(markdownDocs, -1, nullptr);
	externalFiles.addChild(networkFiles, -1, nullptr);
	externalFiles.addChild(webViewResources, -1, nullptr);

	ValueTree defaultPreset("DefaultPreset");
	ValueTree dc = chainToExport->getMainController()->getUserPresetHandler().defaultPresetManager->getDefaultPreset();

	if (dc.isValid())
	{
		defaultPreset.addChild(dc.createCopy(), -1, nullptr);
	}

	externalFiles.addChild(defaultPreset, -1, nullptr);

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
        
		if (auto xml = XmlDocument::parse(sampleMapFiles[i]))
		{
			ValueTree sampleMap = ValueTree::fromXml(*xml);
			sampleMaps.addChild(sampleMap, -1, nullptr);
		}
	}

	

	return sampleMaps;
}

bool CompileExporter::globalCommandLineExport = false;
bool CompileExporter::useCIMode = false;
int CompileExporter::forcedVSTVersion = 0;

void CompileExporter::printErrorMessage(const String& title, const String &message)
{
	if (shouldBeSilent())
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
	case CompileExporter::CorruptedPoolFiles:	return "Pooled binary resources are corrupt. Clean build folder and retry.";
	case CompileExporter::numErrorCodes: return "OK";
		
	default:
		break;
	}

	return "OK";
}


juce::File CompileExporter::getBuildFolder() const
{
	return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries);
}

void CompileExporter::writeValueTreeToTemporaryFile(const ValueTree& v, const String &tempFolder, const String& childFile, bool compress)
{
	auto file = File(tempFolder).getChildFile(childFile);

	PresetHandler::writeValueTreeAsFile(v, file.getFullPathName(), compress); ;
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


CompileExporter::ErrorCodes CompileExporter::exportMainSynthChainAsMidiFx(BuildOption option)
{
	ErrorCodes result = exportInternal(TargetTypes::MidiEffectPlugin, option);

	if (result != ErrorCodes::OK)
		printErrorMessage("Export Error", getCompileResult(result));

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
        
        dynamic_cast<GlobalSettingManager*>(mainSynthChain->getMainController())->getSettingsObject().addTemporaryDefinitions(getTemporaryDefinitions(commandLine));
        
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

        exporter.noLto = args.contains("-nolto");
        
		bool switchBack = false;

		if (currentProjectFolder != projectDirectory)
		{
			switchBack = true;
			GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(projectDirectory);
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

		exporter.setRawExportMode(exportType == "export_raw");

		if (BuildOptionHelpers::isEffect(b)) result = exporter.exportMainSynthChainAsFX(b);
		else if (BuildOptionHelpers::isInstrument(b)) result = exporter.exportMainSynthChainAsInstrument(b);
		else if (BuildOptionHelpers::isStandalone(b)) result = exporter.exportMainSynthChainAsStandaloneApp(b);
		else if (BuildOptionHelpers::isMidiEffect(b)) result = exporter.exportMainSynthChainAsMidiFx(b);
		else result = ErrorCodes::BuildOptionInvalid;

		if (!isUsingCIMode() && switchBack)
		{
			GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(currentProjectFolder);
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
		const String pluginName = argument.fromFirstOccurrenceOf("-p:", false, true).toUpperCase();

		if (pluginName == "VST23AU")
		{
			CompileExporter::forcedVSTVersion = 23; // you now, 2 + 3...
			return 0x0010;
		}

		if (pluginName == "VST2")
		{
			CompileExporter::forcedVSTVersion = 2;
			return 0x0010;
		}
			
		else if (pluginName == "VST3")
		{
			CompileExporter::forcedVSTVersion = 3;
			return 0x0010;
		}
		else
			CompileExporter::forcedVSTVersion = 0;

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
    case 'l':
    {
        legacyCpuSupport = true;
        return 0;
    }
	case 'a':
	{
		// Always return 64bit, 32bit is dead.
		return 0x0002;
#if 0
		const String architectureName = argument.fromFirstOccurrenceOf("-a:", false, true);



		if (architectureName == "x86") return 0x0001;
		else if (architectureName == "x64") return 0x0002;
		else if (architectureName == "x86x64") return 0x0004;
		else return 0;
#endif
	}
	case 't':
	{
		const String typeName = argument.fromFirstOccurrenceOf("-t:", false, true);

		if (typeName == "standalone") return 0x0100;
		else if (typeName == "instrument") return 0x0200;
		else if (typeName == "effect") return 0x0400;
		else if (typeName == "midi") return 0x0800;
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
	
	if (!legacyCpuSupport) legacyCpuSupport = data.getSetting(HiseSettings::Compiler::LegacyCPUSupport);
	    
	if (!hisePath.isDirectory())
	{
		if (isUsingCIMode())
		{
			auto appPath = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory();

			while (!appPath.isRoot() && !appPath.getChildFile("hi_core").isDirectory())
			{
				appPath = appPath.getParentDirectory();
			}

			if (!appPath.isRoot())
				hisePath = appPath;
		}
		else
		{
			hisePath = data.getSetting(HiseSettings::Compiler::HisePath);
		}
	}
	
	if (!hisePath.isDirectory()) 
		return ErrorCodes::HISEPathNotSpecified;

	if (!checkSanity(type, option)) return ErrorCodes::SanityCheckFailed;

	chainToExport->getMainController()->getUserPresetHandler().initDefaultPresetManager({});

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

		auto tempDirectory = File(solutionDirectory).getChildFile("temp/");

		const String directoryPath = tempDirectory.getFullPathName();

        JavascriptProcessor::ScopedPreprocessorMerger sm(chainToExport->getMainController());

		try
		{
			compressValueTree<PresetDictionaryProvider>(exportPresetFile(), directoryPath, "preset");
		}
		catch(Result& r)
		{
			std::cout << "Error at exporting preset: " + r.getErrorMessage();
			return ErrorCodes::CompileError;
		}

		

#if DONT_EMBED_FILES_IN_FRONTEND
		const bool embedFiles = false;
#else
		// Don't embedd external files on iOS for quicker loading times...
        const bool embedFiles = !BuildOptionHelpers::isIOS(option);
#endif

		auto embedUserPresets = data.getSetting(HiseSettings::Project::EmbedUserPresets);

		auto userPresetTree = embedUserPresets ? UserPresetHelpers::collectAllUserPresets(chainToExport):
												 ValueTree("UserPresets");

		// Embed the user presets and extract them on first load
		compressValueTree<UserPresetDictionaryProvider>(userPresetTree, directoryPath, "userPresets");

		try
		{
			// Always embed scripts and fonts, but don't embed samplemaps
			compressValueTree<JavascriptDictionaryProvider>(exportEmbeddedFiles(), directoryPath, "externalFiles");
		}
		catch(Result& r)
		{
			std::cout << "Error at embedding external files: " + r.getErrorMessage();
			return ErrorCodes::CorruptedPoolFiles;
		}
		
		auto& handler = GET_PROJECT_HANDLER(chainToExport);

		auto iof = handler.getTempFileForPool(FileHandlerBase::Images);
		auto sof = handler.getTempFileForPool(FileHandlerBase::AudioFiles);
		auto smof = handler.getTempFileForPool(FileHandlerBase::SampleMaps);
		auto mof = handler.getTempFileForPool(FileHandlerBase::MidiFiles);

		bool alreadyExported = iof.existsAsFile() || sof.existsAsFile() || smof.existsAsFile() || mof.existsAsFile();

		if (rawMode || (alreadyExported && data.getSetting(HiseSettings::Compiler::RebuildPoolFiles)))
		{
			iof.deleteFile();
			sof.deleteFile();
			smof.deleteFile();
			mof.deleteFile();

			std::cout << "Exporting the pooled resources...";

			handler.exportAllPoolsToTemporaryDirectory(chainToExport, nullptr);

			std::cout << "DONE\n" ;

			if (rawMode)
			{
				auto printExportedFiles = [](MainController* mc, const Array<PoolReference>& files, ProjectHandler::SubDirectories d)
				{
					String name = ProjectHandler::getIdentifier(d);
					NewLine nl;

					std::cout << "Exported " << name << " resources: " << nl;

					auto& handler = mc->getCurrentFileHandler();

					
					auto folder = handler.getSubDirectory(d);

					for (const auto& ref : files)
					{
						std::cout << ref.getFile().getRelativePathFrom(folder) << nl;
					}

					std::cout << "=============================================";
				};

				auto mc = chainToExport->getMainController();

				auto audioPool = mc->getCurrentAudioSampleBufferPool();
				auto imagePool = mc->getCurrentImagePool();
				auto sampleMapPool = mc->getCurrentSampleMapPool();
				auto midiPool = mc->getCurrentMidiFilePool();

				printExportedFiles(mc, audioPool->getListOfAllReferences(false), ProjectHandler::AudioFiles);

				printExportedFiles(mc, imagePool->getListOfAllReferences(false), ProjectHandler::Images);

				printExportedFiles(mc, sampleMapPool->getListOfAllReferences(false), ProjectHandler::SampleMaps);

				printExportedFiles(mc, midiPool->getListOfAllReferences(false), ProjectHandler::MidiFiles);
			}

			alreadyExported = true;
		}

		if (!alreadyExported)
		{
			handler.exportAllPoolsToTemporaryDirectory(chainToExport, nullptr);
		}

		File imageOutputFile, sampleOutputFile, samplemapFile, midiFile;

		if (embedFiles)
		{
			samplemapFile = tempDirectory.getChildFile("samplemaps");

			if (smof.existsAsFile())
				smof.copyFileTo(samplemapFile);
			else
				return ErrorCodes::CorruptedPoolFiles;

			midiFile = tempDirectory.getChildFile("midiFiles");
			
			if (mof.existsAsFile())
				mof.copyFileTo(midiFile);
			else
				return ErrorCodes::CorruptedPoolFiles;

			auto pname = GET_SETTING(HiseSettings::Project::Name);
			auto cname = GET_SETTING(HiseSettings::User::Company);
			auto projectFolder = ProjectHandler::getAppDataDirectory(chainToExport->getMainController()).getParentDirectory().getChildFile(cname).getChildFile(pname);

			if (IS_SETTING_TRUE(HiseSettings::Project::EmbedAudioFiles))
			{
				
				sampleOutputFile = tempDirectory.getChildFile("impulses");

				if (sof.existsAsFile())
					sof.copyFileTo(sampleOutputFile);
				else
					return ErrorCodes::CorruptedPoolFiles;
			}
			else if (PresetHandler::showYesNoWindow("Copy Audio files to app data directory?",
				"Do you want to copy the audio pool file to your project's app data directory?"))
			{
				sampleOutputFile = projectFolder.getChildFile(sof.getFileName());
				sof.copyFileTo(sampleOutputFile);
			}

			if (IS_SETTING_TRUE(HiseSettings::Project::EmbedImageFiles))
			{
				imageOutputFile = tempDirectory.getChildFile("images");

				if (iof.existsAsFile())
					iof.copyFileTo(imageOutputFile);
				else
					return ErrorCodes::CorruptedPoolFiles;
			}
			else if (PresetHandler::showYesNoWindow("Copy Image files to app data directory?",
				"Do you want to copy the image pool file to your project's app data directory?"))
			{
				imageOutputFile = projectFolder.getChildFile(iof.getFileName());
				iof.copyFileTo(imageOutputFile);
			}
		}
		else if (BuildOptionHelpers::isIOS(option))
		{
			{
				// Make sure the binary data exists to prevent compilation error

				samplemapFile = tempDirectory.getChildFile("samplemaps");
				imageOutputFile = tempDirectory.getChildFile("images");
				sampleOutputFile = tempDirectory.getChildFile("impulses");
				midiFile = tempDirectory.getChildFile("midiFiles");

				const String unused = "unused";

				samplemapFile.replaceWithText(unused);
				imageOutputFile.replaceWithText(unused);
				sampleOutputFile.replaceWithText(unused);
				midiFile.replaceWithText(unused);
			}

			auto resourceFolder = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("EmbeddedResources");

			if (!resourceFolder.isDirectory())
				resourceFolder.createDirectory();

			sampleOutputFile = resourceFolder.getChildFile("AudioResources.dat");
			imageOutputFile = resourceFolder.getChildFile("ImageResources.dat");
			samplemapFile = resourceFolder.getChildFile("SampleMapResources.dat");
			midiFile = resourceFolder.getChildFile("MidiFilesResources.dat");

			if (smof.existsAsFile())
				smof.copyFileTo(samplemapFile);

			if (iof.existsAsFile())
				iof.copyFileTo(imageOutputFile);

			if (sof.existsAsFile())
				sof.copyFileTo(sampleOutputFile);

			if (mof.existsAsFile())
				mof.copyFileTo(midiFile);
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
	{
		Processor::Iterator<ExternalDataHolder> iter(chainToExport);

		while(auto p = iter.getNextProcessor())
		{
			if(auto numAudioSlots = p->getNumDataObjects(ExternalData::DataType::AudioFile))
			{
				for(int i = 0; i < numAudioSlots; i++)
				{
					auto ref = p->getAudioFile(i)->toBase64String();

					if(ref.isNotEmpty())
					{
						PoolReference r(chainToExport->getMainController(), ref, ProjectHandler::AudioFiles);

						auto f = r.getFile();

						if(!f.existsAsFile())
							return ref;
					}
				}
			}

		}
	}
	{
		Processor::Iterator<ModulatorSampler> iter(chainToExport);
    
	    const File sampleFolder = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Samples);
	    
	    Array<File> sampleFiles;
	    
	    sampleFolder.findChildFiles(sampleFiles, File::findFiles, true);
	    
		Array<File> sampleMapFiles;

		auto sampleMapRoot = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);

		sampleMapRoot.findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

		Array<PooledSampleMap> maps;

		for (const auto& f : sampleMapFiles)
		{
			PoolReference ref(chainToExport->getMainController(), f.getFullPathName(), FileHandlerBase::SampleMaps);

			maps.add(chainToExport->getMainController()->getCurrentSampleMapPool()->loadFromReference(ref, PoolHelpers::LoadAndCacheStrong));
		}

		for (auto d : maps)
		{
			if (auto v = d.getData())
			{
				const String faulty = SampleMap::checkReferences(chainToExport->getMainController(), *v, sampleFolder, sampleFiles);

				if (faulty.isNotEmpty())
					return faulty;
			}
		}
	 
	    return String();
	}

    
}

bool CompileExporter::checkSanity(TargetTypes type, BuildOption option)
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

#if !JUCE_LINUX
    if(BuildOptionHelpers::isAAX(option))
    {
        const File aaxSDK = hiseDirectory.getChildFile("tools/SDK/AAX/Libs");
        
        if(!aaxSDK.isDirectory())
        {
            printErrorMessage("AAX SDK not found", "You need to get the AAX SDK from Avid and copy it to '%HISE_SDK%/tools/SDK/AAX/'");
            return false;
        }
    }
#endif
    
	for (auto fx : ProcessorHelpers::getListOfAllProcessors<HardcodedSwappableEffect>(chainToExport))
	{
		auto r = fx->sanityCheck();

		if (!r.wasOk())
		{
			printErrorMessage("Hardcoded FX Sanity check failed", r.getErrorMessage());
			return false;
		}
	}

	for (auto n : ProcessorHelpers::getListOfAllProcessors<scriptnode::DspNetwork::Holder>(chainToExport))
	{
		if (auto network = n->getActiveOrDebuggedNetwork())
		{
			auto r = network->checkBeforeCompilation();

			if (!r.wasOk())
			{
				printErrorMessage("DSP Network sanity check failed", r.getErrorMessage());
				return false;
			}
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

    GlobalHiseLookAndFeel::setDefaultColours(*b);
    
#if JUCE_WINDOWS

	switch (type)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin:
		b->addItem("VSTi 64bit", BuildOption::VSTiWindowsx64);
		b->addItem("AAX 64bit", BuildOption::AAXWindowsx64);
		b->addItem("All platforms", BuildOption::AllPluginFormatsInstrument);
		break;
	case CompileExporter::TargetTypes::EffectPlugin:
		b->addItem("VST 64bit", BuildOption::VSTWindowsx64);
		b->addItem("AAX 64bit", BuildOption::AAXWindowsx64);
		b->addItem("All Platforms", BuildOption::AllPluginFormatsFX);
		break;
	case CompileExporter::TargetTypes::MidiEffectPlugin:
		b->addItem("VST 64bit", BuildOption::MidiFXWIndowsx64);
		break;
	case CompileExporter::TargetTypes::StandaloneApplication:
		b->addItem("Standalone 64bit", BuildOption::StandaloneWindowsx64);
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
		b->addItem("Headless VSTi", BuildOption::HeadlessLinuxVSTi);
		break;
	case CompileExporter::TargetTypes::EffectPlugin:
		b->addItem("VST", BuildOption::VSTLinux);
		b->addItem("Headless VST", BuildOption::HeadlessLinuxVST);
		break;
	case CompileExporter::TargetTypes::StandaloneApplication:
		b->addItem("Standalone Linux", BuildOption::StandaloneLinux);
		break;
	case CompileExporter::TargetTypes::MidiEffectPlugin:
		b->addItem("Midi FX", BuildOption::MidiFXLinux);
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
	case CompileExporter::TargetTypes::MidiEffectPlugin:
		b->addItem("VST MidiFX", BuildOption::VSTMIDImacOS);
		b->addItem("AU MidiFX", BuildOption::AUMIDImacOS);
		b->addItem("VST/AU MidiFX", BuildOption::VSTAUMIDImacOS);
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
    w.getComboBoxComponent("buildOption")->setSelectedItemIndex(0, dontSendNotification);
    
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


CompileExporter::ErrorCodes CompileExporter::compileSolution(BuildOption buildOption, TargetTypes types)
{
	BatchFileCreator::createBatchFile(this, buildOption, types);

	File batchFile = BatchFileCreator::getBatchFile(this);
    
#if JUCE_WINDOWS
    
    String command = "\"" + batchFile.getFullPathName() + "\"";
    
#elif JUCE_LINUX
	if (!isUsingCIMode())
	{
		String permissionCommand = "chmod +x \"" + batchFile.getFullPathName() + "\"";
	  system(permissionCommand.getCharPointer());
		
		if (PresetHandler::showYesNoWindow("Batch file created.", "The batch file was created in the build directory. Do you want to open the location?"))
		{
			batchFile.getParentDirectory().revealToUser();	
		}		
	}
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

    HeaderHelpers::addBasicIncludeLines(this, pluginDataHeaderFile, iOSAUv3);

	HeaderHelpers::addAdditionalSourceCodeHeaderLines(this,pluginDataHeaderFile);
	HeaderHelpers::addStaticDspFactoryRegistration(pluginDataHeaderFile, this);
	HeaderHelpers::addCopyProtectionHeaderLines(publicKey, pluginDataHeaderFile);

	pluginDataHeaderFile << "AudioProcessor* JUCE_CALLTYPE createPluginFilter() { CREATE_PLUGIN(nullptr, nullptr); }\n";
	pluginDataHeaderFile << "\n";

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
	HeaderHelpers::addFullExpansionTypeSetter(this, pluginDataHeaderFile);
	HeaderHelpers::writeHeaderFile(solutionDirectory, pluginDataHeaderFile);

	return ErrorCodes::OK;
}

CompileExporter::ErrorCodes CompileExporter::createStandaloneAppHeaderFile(const String& solutionDirectory, const String& uniqueId, const String& version, String publicKey)
{
	ignoreUnused(version, uniqueId);

	String pluginDataHeaderFile;

	HeaderHelpers::addBasicIncludeLines(this, pluginDataHeaderFile);

	HeaderHelpers::addAdditionalSourceCodeHeaderLines(this,pluginDataHeaderFile);
	HeaderHelpers::addStaticDspFactoryRegistration(pluginDataHeaderFile, this);
	HeaderHelpers::addCopyProtectionHeaderLines(publicKey, pluginDataHeaderFile);

    pluginDataHeaderFile << "AudioProcessor* hise::StandaloneProcessor::createProcessor() { CREATE_PLUGIN(deviceManager, callback); }\n";
    pluginDataHeaderFile << "\n";
    pluginDataHeaderFile << "START_JUCE_APPLICATION(hise::FrontendStandaloneApplication)\n";

	HeaderHelpers::addProjectInfoLines(this, pluginDataHeaderFile);
	HeaderHelpers::addFullExpansionTypeSetter(this, pluginDataHeaderFile);
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

    String year = HelperClasses::isUsingVisualStudio2017(dataObject) ? "2017" : "2022";

	File resourcesFileObject(solutionDirectory + "/Builds/VisualStudio" + year + "/resources.rc");

	resourcesFileObject.deleteFile();

	resourcesFileObject.create();
	resourcesFileObject.appendText(resourcesFile);

	return ErrorCodes::OK;
}


#define REPLACE_WILDCARD_WITH_STRING(wildcard, s) (templateProject = templateProject.replace(wildcard, s))
#define REPLACE_WILDCARD(wildcard, settingId) templateProject = templateProject.replace(wildcard, GET_SETTING(settingId));
#define SET_JUCER_FLAG(wildcard, settingId) REPLACE_WILDCARD_WITH_STRING(wildcard, GET_SETTING(settingId) == "1" ? "enabled" : "disabled");

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

	auto x = GET_SETTING(HiseSettings::Project::SupportFullDynamicsHLAC);

	String fullDynamicsSupport = x == "1" ? "enabled" : "disabled";
	REPLACE_WILDCARD_WITH_STRING("%SUPPORT_FULL_DYNAMICS%", fullDynamicsSupport);

	String readOnlyFactoryPresets = GET_SETTING(HiseSettings::Project::ReadOnlyFactoryPresets) == "1" ? "enabled" : "disabled";
	REPLACE_WILDCARD_WITH_STRING("%READ_ONLY_FACTORY_PRESETS%", readOnlyFactoryPresets);

	String overwriteUserPresets = GET_SETTING(HiseSettings::Project::OverwriteOldUserPresets) == "1" ? "enabled" : "disabled";
	REPLACE_WILDCARD_WITH_STRING("%OVERWRITE_OLD_USER_PRESETS%", overwriteUserPresets);

    String vst3Category = GET_SETTING(HiseSettings::Project::VST3Category);
    
	if (type == TargetTypes::EffectPlugin)
	{
        if(vst3Category.isNotEmpty())
            vst3Category = "Fx," + vst3Category;
        else
            vst3Category = "Fx";
        
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "0");
        REPLACE_WILDCARD_WITH_STRING("%PLUGIN_PRODUCES_MIDI_OUT%", "0");
        
		auto midiInputEnabled = GET_SETTING(HiseSettings::Project::EnableMidiInputFX);
		auto soundGeneratorsEnabled = GET_SETTING(HiseSettings::Project::EnableSoundGeneratorsFX);

		REPLACE_WILDCARD_WITH_STRING("%ENABLE_MIDI_INPUT_FX%", midiInputEnabled == "1" ? "enabled" : "disabled");
		REPLACE_WILDCARD_WITH_STRING("%PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN%", soundGeneratorsEnabled == "1" ? "enabled" : "disabled");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN%", midiInputEnabled);
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "enabled");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISMIDIFX%", "0");

		String monoSupport = GET_SETTING(HiseSettings::Project::SupportMonoFX) == "1" ? "enabled" : "disabled";

		REPLACE_WILDCARD_WITH_STRING("%SUPPORT_MONO%", monoSupport);
        REPLACE_WILDCARD("%AAX_CATEGORY%", HiseSettings::Project::AAXCategoryFX);
		REPLACE_WILDCARD_WITH_STRING("%HISE_MIDIFX_PLUGIN%", "disabled");
	}
	else if (type == TargetTypes::MidiEffectPlugin)
	{
        if(vst3Category.isNotEmpty())
            vst3Category = "Fx," + vst3Category;
        else
            vst3Category = "Fx";
        
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN%", "1");
        REPLACE_WILDCARD_WITH_STRING("%PLUGIN_PRODUCES_MIDI_OUT%", "1");
        
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "0");
        
        
        
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISMIDIFX%", "1");
		REPLACE_WILDCARD_WITH_STRING("%PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN%", "disabled");
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
		REPLACE_WILDCARD_WITH_STRING("%HISE_MIDIFX_PLUGIN%", "enabled");
		REPLACE_WILDCARD_WITH_STRING("%SUPPORT_MONO%", "disabled");
	}
	else
	{
        if(vst3Category.isNotEmpty())
            vst3Category = "Instrument," + vst3Category;
        else
            vst3Category = "Instrument";
        
		REPLACE_WILDCARD_WITH_STRING("%SUPPORT_MONO%", "disabled");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISMIDIFX%", "0");
        
        auto midiOutputEnabled = GET_SETTING(HiseSettings::Project::EnableMidiOut);
        
        REPLACE_WILDCARD_WITH_STRING("%PLUGIN_PRODUCES_MIDI_OUT%", midiOutputEnabled);
        REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "1");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN", "1");
		REPLACE_WILDCARD_WITH_STRING("%ENABLE_MIDI_INPUT_FX%", "disabled");
		REPLACE_WILDCARD_WITH_STRING("%PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN%", "disabled");
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
        REPLACE_WILDCARD_WITH_STRING("%AAX_CATEGORY%", "AAX_ePlugInCategory_SWGenerators");
		REPLACE_WILDCARD_WITH_STRING("%HISE_MIDIFX_PLUGIN%", "disabled");
	}

    REPLACE_WILDCARD_WITH_STRING("%VST3_CATEGORY%", vst3Category);
    
	REPLACE_WILDCARD_WITH_STRING("%IS_STANDALONE_FRONTEND%", "disabled");

	ProjectTemplateHelpers::handleCompanyInfo(this, templateProject);

	if (BuildOptionHelpers::isIOS(option))
	{
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AU%", "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_VST%", "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_VST3%", "0");
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

		bool buildAU = BuildOptionHelpers::isAU(option);
		bool buildVST = BuildOptionHelpers::isVST(option);
		const bool headlessLinux = BuildOptionHelpers::isHeadlessLinuxPlugin(option);

		buildVST |= headlessLinux;

		auto vst3 = GET_SETTING(HiseSettings::Project::VST3Support) == "1";

		bool buildVST2 = buildVST && !vst3;
		bool buildVST3 = buildVST && vst3;

		if (forcedVSTVersion != 0)
		{
			// Only possible in command line export...
			jassert(isExportingFromCommandLine());
			jassert(isUsingCIMode());
			jassert(BuildOptionHelpers::isVST(option));
			buildVST2 = forcedVSTVersion == 2 || forcedVSTVersion == 23;
			buildVST3 = forcedVSTVersion == 3 || forcedVSTVersion == 23;

#if JUCE_MAC
			buildAU = forcedVSTVersion == 23;
#endif
		}

#if JUCE_LINUX
		const bool buildAAX = false;
#else
		const bool buildAAX = BuildOptionHelpers::isAAX(option);
#endif

		REPLACE_WILDCARD_WITH_STRING("%BUILD_AU%", buildAU ? "1" : "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_VST%", buildVST2 ? "1" : "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_VST3%", buildVST3 ? "1" : "0");
		REPLACE_WILDCARD_WITH_STRING("%BUILD_AAX%", buildAAX ? "1" : "0");

		const File vstSDKPath = hisePath.getChildFile("tools/SDK/VST3 SDK");

		if (buildVST && !vstSDKPath.isDirectory())
		{
			return ErrorCodes::VSTSDKMissing;
		}

		if (buildVST)
			REPLACE_WILDCARD_WITH_STRING("%VSTSDK_FOLDER%", hisePath.getChildFile("tools/SDK/VST3 SDK").getFullPathName());
		else
			REPLACE_WILDCARD_WITH_STRING("%VSTSDK_FOLDER", String());

		if (buildVST3)
			REPLACE_WILDCARD_WITH_STRING("%VSTSDK3_FOLDER%", hisePath.getChildFile("JUCE/modules/juce_audio_processors/format_types/VST3_SDK").getFullPathName());
		else
			REPLACE_WILDCARD_WITH_STRING("%VSTSDK3_FOLDER", String());

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
            
            // Only build 64bit Intel binaries for AAX
            // REPLACE_WILDCARD_WITH_STRING("%ARM_ARCH%", "x86_64");
            
            // Welcome to the future...
            REPLACE_WILDCARD_WITH_STRING("%ARM_ARCH%", "arm64,arm64e,x86_64");
		}
		else
		{
			REPLACE_WILDCARD_WITH_STRING("%AAX_PATH%", String());
			REPLACE_WILDCARD_WITH_STRING("%AAX_RELEASE_LIB%", String());
			REPLACE_WILDCARD_WITH_STRING("%AAX_DEBUG_LIB%", String());
			REPLACE_WILDCARD_WITH_STRING("%AAX_IDENTIFIER%", String());
            REPLACE_WILDCARD_WITH_STRING("%ARM_ARCH%", "arm64,arm64e,x86_64");
		}
	}

	// Add the linux GUI packages for non headless plugin builds to the projucer exporter...
	if (BuildOptionHelpers::isLinux(option))
	{
		if(BuildOptionHelpers::isHeadlessLinuxPlugin(option))
		{
			REPLACE_WILDCARD_WITH_STRING("%LINUX_GUI_LIBS%", "");
			REPLACE_WILDCARD_WITH_STRING("%JUCE_HEADLESS_PLUGIN_CLIENT%", "enabled");
		}
		else
		{
			REPLACE_WILDCARD_WITH_STRING("%LINUX_GUI_LIBS%", "x11 xinerama xext");
			REPLACE_WILDCARD_WITH_STRING("%JUCE_HEADLESS_PLUGIN_CLIENT%", "disabled");
		}
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%LINUX_GUI_LIBS%", "");
		REPLACE_WILDCARD_WITH_STRING("%JUCE_HEADLESS_PLUGIN_CLIENT%", "disabled");
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

#if JUCE_LINUX
    REPLACE_WILDCARD_WITH_STRING("%USE_JACK%", "enabled");
	REPLACE_WILDCARD_WITH_STRING("%LINUX_GUI_LIBS%", "x11 xinerama xext");
#else
    REPLACE_WILDCARD_WITH_STRING("%USE_JACK%", "disabled");
	REPLACE_WILDCARD_WITH_STRING("%LINUX_GUI_LIBS%", "");
#endif

	REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
	REPLACE_WILDCARD_WITH_STRING("%IS_STANDALONE_FRONTEND%", "enabled");

	String fullDynamicsSupport = GET_SETTING(HiseSettings::Project::SupportFullDynamicsHLAC) == "1" ? "enabled" : "disabled";
	REPLACE_WILDCARD_WITH_STRING("%SUPPORT_FULL_DYNAMICS%", fullDynamicsSupport);

	String readOnlyFactoryPresets = GET_SETTING(HiseSettings::Project::ReadOnlyFactoryPresets) == "1" ? "enabled" : "disabled";
	REPLACE_WILDCARD_WITH_STRING("%READ_ONLY_FACTORY_PRESETS%", readOnlyFactoryPresets);

	String overwriteUserPresets = GET_SETTING(HiseSettings::Project::OverwriteOldUserPresets) == "1" ? "enabled" : "disabled";
	REPLACE_WILDCARD_WITH_STRING("%OVERWRITE_OLD_USER_PRESETS%", overwriteUserPresets);

	ProjectTemplateHelpers::handleVisualStudioVersion(dataObject,templateProject);

	ProjectTemplateHelpers::handleCompanyInfo(this, templateProject);
	ProjectTemplateHelpers::handleCompilerInfo(this, templateProject);

	ProjectTemplateHelpers::handleAdditionalSourceCode(this, templateProject, option);
	ProjectTemplateHelpers::handleCopyProtectionInfo(this, templateProject, option);

	return HelperClasses::saveProjucerFile(templateProject, this);
}


void CompileExporter::ProjectTemplateHelpers::handleCompilerWarnings(String& templateProject)
{
	// We'll deactivate these warnings
	static Array<int> msvcWarnings = {
		4100, // unused parameter
		4127, // weird linker error in ASMJIT
		4244, // possible precision loss (we're lazy here)
		4661, // incomplete template definition
		4456, // scope masking
		4457, // scope masking
		4458, // scope masking
		4459  // scope masking
	};

	String warnings;

	for (auto v : msvcWarnings)
	{
		warnings << " /wd&quot;" << String(v) << "&quot;";
	}

	REPLACE_WILDCARD_WITH_STRING("%MSVC_WARNINGS%", warnings);
}

void CompileExporter::ProjectTemplateHelpers::handleCompilerInfo(CompileExporter* exporter, String& templateProject)
{
	handleCompilerWarnings(templateProject);

	const auto includeLoris = (bool)exporter->dataObject.getSetting(HiseSettings::Project::IncludeLorisInFrontend);

	if(includeLoris)
	{
		REPLACE_WILDCARD_WITH_STRING("%LORIS_MODULEPATH%", R"(<MODULEPATH id="hi_loris" path="%HISE_PATH%"/>)");
		REPLACE_WILDCARD_WITH_STRING("%LORIS_MODULEINFO%", R"(<MODULE id="hi_loris" showAllCode="1" useLocalCopy="0" useGlobalPath="0"/>)");
		REPLACE_WILDCARD_WITH_STRING("%HISE_INCLUDE_LORIS%", "1");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%LORIS_MODULEPATH%", "");
		REPLACE_WILDCARD_WITH_STRING("%LORIS_MODULEINFO%", "");
		REPLACE_WILDCARD_WITH_STRING("%HISE_INCLUDE_LORIS%", "0");
	}

	const File jucePath = exporter->hisePath.getChildFile("JUCE/modules");

	REPLACE_WILDCARD_WITH_STRING("%HISE_PATH%", exporter->hisePath.getFullPathName());
	REPLACE_WILDCARD_WITH_STRING("%JUCE_PATH%", jucePath.getFullPathName());
	
    REPLACE_WILDCARD_WITH_STRING("%LINK_TIME_OPTIMISATION%", exporter->noLto ? "0" : "1");
    
	auto includeFaust = BackendDllManager::shouldIncludeFaust(exporter->chainToExport->getMainController());


	REPLACE_WILDCARD_WITH_STRING("%HISE_INCLUDE_FAUST%", includeFaust ? "enabled" : "disabled");

    String headerPath;
    
	if (includeFaust)
	{
		headerPath = exporter->dataObject.getFaustPath().getChildFile("include").getFullPathName();
	}
	
    if(BackendDllManager::hasRNBOFiles(exporter->chainToExport->getMainController()))
    {
        auto folder = BackendDllManager::getRNBOSourceFolder(exporter->chainToExport->getMainController());
        
        headerPath << ";" << folder.getFullPathName();
        headerPath << ";" << folder.getChildFile("common").getFullPathName();
    }
    
    REPLACE_WILDCARD_WITH_STRING("%FAUST_HEADER_PATH%", headerPath);
    
    REPLACE_WILDCARD_WITH_STRING("%USE_IPP%", exporter->useIpp ? "1" : "0");

    REPLACE_WILDCARD_WITH_STRING("%IPP_1A%", exporter->useIpp ? "Static_Library" : String());
		REPLACE_WILDCARD_WITH_STRING("%UAC_LEVEL%", exporter->dataObject.getSetting(HiseSettings::Project::AdminPermissions) ? "/MANIFESTUAC:level='requireAdministrator'" : String());
    
    REPLACE_WILDCARD_WITH_STRING("%LEGACY_CPU_SUPPORT%", exporter->legacyCpuSupport ? "1" : "0");
    
    auto s = exporter->dataObject.getTemporaryDefinitionsAsString();
    
    REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_LINUX%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsLinux).toString() + s);
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_WIN%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsWindows).toString() + s);
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_OSX%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsOSX).toString() + s);
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_IOS%", exporter->dataObject.getSetting(HiseSettings::Project::ExtraDefinitionsIOS).toString());

#if JUCE_WINDOWS
    const auto useGlobalAppFolder = (bool)exporter->dataObject.getSetting(HiseSettings::Project::UseGlobalAppDataFolderWindows);
#elif JUCE_MAC
    const auto useGlobalAppFolder = (bool)exporter->dataObject.getSetting(HiseSettings::Project::UseGlobalAppDataFolderMacOS);
#else
    // Dave let me know if you need this LOL...
    const bool useGlobalAppFolder = false;
#endif
    
    REPLACE_WILDCARD_WITH_STRING("%USE_GLOBAL_APP_FOLDER%", useGlobalAppFolder ? "enabled" : "disabled");
    
	auto allow32BitMacOS = exporter->dataObject.getSetting(HiseSettings::Compiler::Support32BitMacOS);

	if (allow32BitMacOS)
	{
		REPLACE_WILDCARD_WITH_STRING("%MACOS_ARCHITECTURE%", "64BitUniversal");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%MACOS_ARCHITECTURE%", "64BitIntel");
	}

    auto copyPlugin = !isUsingCIMode();
    
#if JUCE_MAC
    auto macOSVersion = SystemStats::getOperatingSystemType();
    
    // deactivate copy step on Sonoma to avoid the cycle dependencies error...
    if(macOSVersion == SystemStats::MacOS_14)
    {
        PresetHandler::showMessageWindow("Copystep diabled", "macOS Sonoma will cause a compile error if the copy step is enabled, so you have to copy the plugin files into the plugin folders manually after compilation");
        copyPlugin = false;
    }
#endif
    
	REPLACE_WILDCARD_WITH_STRING("%COPY_PLUGIN%", copyPlugin ? "1" : "0");

#if JUCE_MAC
	REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", exporter->useIpp ? "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a" : String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", exporter->useIpp ? "/opt/intel/ipp/include" : String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", exporter->useIpp ? "/opt/intel/ipp/lib" : String());
#endif

#if JUCE_LINUX
	REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", exporter->useIpp ? "/opt/intel/ipp/lib/intel64/libippi.a  /opt/intel/ipp/lib/intel64/libipps.a /opt/intel/ipp/lib/intel64/libippvm.a /opt/intel/ipp/lib/intel64/libippcore.a" : String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", exporter->useIpp ? "/opt/intel/ipp/include" : String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", exporter->useIpp ? "/opt/intel/ipp/lib" : String());
#endif

#if !JUCE_MAC && !JUCE_LINUX
	REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", String());
	REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", String());
#endif

	const auto includePerfetto = (bool)exporter->dataObject.getSetting(HiseSettings::Project::CompileWithPerfetto);

	REPLACE_WILDCARD_WITH_STRING("%PERFETTO_INCLUDE_WIN%", includePerfetto ? "\nPERFETTO=1\nNOMINMAX=1\nWIN32_LEAN_AND_MEAN=1" : "");
	REPLACE_WILDCARD_WITH_STRING("%PERFETTO_COMPILER_FLAGS_WIN%", includePerfetto ? " /Zc:__cplusplus /permissive- /vmg" : "");
	REPLACE_WILDCARD_WITH_STRING("%PERFETTO_INCLUDE_MACOS%", includePerfetto ? "\nPERFETTO=1" : "");

	const auto dontStripSymbols = (bool)exporter->dataObject.getSetting(HiseSettings::Project::CompileWithDebugSymbols);

	REPLACE_WILDCARD_WITH_STRING("%STRIP_SYMBOLS_WIN%", dontStripSymbols ? " alwaysGenerateDebugSymbols=\"1\" debugInformationFormat=\"ProgramDatabase\"" : "");

    if(dontStripSymbols)
    {
        REPLACE_WILDCARD_WITH_STRING("%STRIP_SYMBOLS_MACOS%", "stripLocalSymbols=\"0\" customXcodeFlags=\"STRIP_INSTALLED_PRODUCT=NO,COPY_PHASE_STRIP=NO\"");
    }
    else
    {
        REPLACE_WILDCARD_WITH_STRING("%STRIP_SYMBOLS_MACOS%", "stripLocalSymbols=\"1\"");
    }
    
	auto& dataObject = exporter->dataObject;
	auto expansionType = GET_SETTING(HiseSettings::Project::ExpansionType);

	if (expansionType == "Custom" || expansionType == "Full")
	{
		REPLACE_WILDCARD_WITH_STRING("%USE_CUSTOM_EXPANSION_TYPE%", "1");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%USE_CUSTOM_EXPANSION_TYPE%", "0");
	}
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
	const bool isUsingVisualStudio2017 = HelperClasses::isUsingVisualStudio2017(dataObject);

	auto shouldUseVS2017 = !(bool)HISE_USE_VS2022;

	if (isUsingVisualStudio2017 != shouldUseVS2017)
	{
		auto buildVersion = shouldUseVS2017 ? "VS2017" : "VS2022";
		auto settingsVersion = isUsingVisualStudio2017 ? "VS2017" : "VS2022";

		String message;

		message << "The visual studio version you have build HISE with (" << buildVersion;
		message << ") is not the one you've selected in the compiler settings (" << settingsVersion;
		message << ")  \n> If you have installed both versions then the compilation should work, otherwise you need to change the VisualStudioVersion setting in the Development settings of HISE to the version that you have installed.  \nPress OK to resume the export process...";

		PresetHandler::showMessageWindow("VS Version mismatch detected", message, PresetHandler::IconType::Warning);
	}
	
	if (isUsingVisualStudio2017)
	{
		REPLACE_WILDCARD_WITH_STRING("%VS_VERSION%", "VS2017");
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FOLDER%", "VisualStudio2017");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%VS_VERSION%", "VS2022");
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FOLDER%", "VisualStudio2022");
	}
}

XmlElement* createXmlElementForFile(ModulatorSynthChain* chainToExport, String& templateProject, File f, bool allowCompilation)
{
	if (f.getFileName().startsWith("."))
		return nullptr;

	ScopedPointer<XmlElement> xml = new XmlElement(f.isDirectory() ? "GROUP" : "FILE");

	auto fileId = FileHelpers::createAlphaNumericUID();

	xml->setAttribute("id", fileId);
	xml->setAttribute("name", f.getFileName());

	if (f.isDirectory())
	{
		Array<File> children;

		f.findChildFiles(children, File::findFilesAndDirectories, false);

		for (auto c : children)
		{
			bool isCustomNodeIncludeFile = (c.getFileName() == "includes.cpp" && c.getParentDirectory().getFileName() == "CustomNodes") ||
                c.getFileName() == "RNBO.cpp";

			if (auto c_xml = createXmlElementForFile(chainToExport, templateProject, c, isCustomNodeIncludeFile))
			{
				xml->addChildElement(c_xml);
			}
		}
	}
	else
	{
		if (f.getFileName() == "Icon.png")
			templateProject = templateProject.replace("%ICON_FILE%", "smallIcon=\"" + fileId + "\" bigIcon=\"" + fileId + "\"");

		bool isSourceFile = (allowCompilation && f.hasFileExtension(".cpp")) || f.getFileName() == "factory.cpp";
		bool isSplashScreen = f.getFileName().contains("SplashScreen");

		xml->setAttribute("compile", isSourceFile ? 1 : 0);
		xml->setAttribute("resource", isSplashScreen ? 1 : 0);

		const String relativePath = f.getRelativePathFrom(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries));

		xml->setAttribute("file", relativePath);
	}
	
	return xml.release();
}


void CompileExporter::ProjectTemplateHelpers::handleAdditionalSourceCode(CompileExporter* exporter, String &templateProject, BuildOption /*option*/)
{
	ModulatorSynthChain* chainToExport = exporter->chainToExport;

	auto& dataObject = exporter->dataObject;

	SET_JUCER_FLAG("%USE_RAW_FRONTEND%", HiseSettings::Project::UseRawFrontend);

	Array<File> additionalSourceFiles;

	File additionalSourceCodeDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode);

	additionalSourceCodeDirectory.findChildFiles(additionalSourceFiles, File::findFiles, false, "*.h");
	additionalSourceCodeDirectory.findChildFiles(additionalSourceFiles, File::findFiles, false, "*.cpp");
	additionalSourceCodeDirectory.findChildFiles(additionalSourceFiles, File::findDirectories, false);

	for (int i = 0; i < additionalSourceFiles.size(); i++)
	{
		if (additionalSourceFiles[i].getFileName().startsWith("."))
			additionalSourceFiles.remove(i--);
	}

	File copyProtectionCppFile = additionalSourceCodeDirectory.getChildFile("CopyProtection.cpp");

	// This will be copied to the source directory
	additionalSourceFiles.removeAllInstancesOf(copyProtectionCppFile);

	File copyProtectionTargetFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("Source/CopyProtection.cpp");

	if (copyProtectionCppFile.existsAsFile())
		copyProtectionCppFile.copyFileTo(copyProtectionTargetFile);
	else
		copyProtectionTargetFile.create();

	


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


	String beatportLibPath = "";

	if(additionalSourceCodeDirectory.getChildFile("beatport").isDirectory())
	{
#if JUCE_WINDOWS
        beatportLibPath << additionalSourceCodeDirectory.getFullPathName() + "/beatport/lib";
        beatportLibPath << ";";
        beatportLibPath = beatportLibPath.replaceCharacter('/', '\\');
        
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_DEBUG_LIB%", "");
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_RELEASE_LIB%", "");
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_LIB_MACOS%", "");
#else
        
        auto df = additionalSourceCodeDirectory.getChildFile("beatport").getChildFile("lib").getChildFile("macos").getChildFile("Debug");
        auto rf = additionalSourceCodeDirectory.getChildFile("beatport").getChildFile("lib").getChildFile("macos").getChildFile("Release");
        
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_DEBUG_LIB%", df.getFullPathName());
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_RELEASE_LIB%", rf.getFullPathName());
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_LIB_MACOS%", "Access");
#endif
	}
    else
    {
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_DEBUG_LIB%", "");
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_RELEASE_LIB%", "");
        REPLACE_WILDCARD_WITH_STRING("%BEATPORT_LIB_MACOS%", "");
    }

#if JUCE_MAC
    const String additionalStaticLibs = exporter->GET_SETTING(HiseSettings::Project::OSXStaticLibs);
    templateProject = templateProject.replace("%OSX_STATIC_LIBS%", additionalStaticLibs);
#else

	auto additionalStaticLibFolder = exporter->GET_SETTING(HiseSettings::Project::WindowsStaticLibFolder);
	
	if (additionalStaticLibFolder.isNotEmpty())
	{
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_D64%", beatportLibPath + additionalStaticLibFolder + "/Debug_x64");
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_R64%", beatportLibPath + additionalStaticLibFolder + "/Release_x64");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_D64%", beatportLibPath);
		REPLACE_WILDCARD_WITH_STRING("%WIN_STATIC_LIB_FOLDER_R64%", beatportLibPath);
	}
#endif
    
	if (additionalSourceFiles.size() != 0)
	{
		StringArray additionalFileDefinitions;

		for (int i = 0; i < additionalSourceFiles.size(); i++)
		{
			auto fileEntry = createXmlElementForFile(chainToExport, templateProject, additionalSourceFiles[i], true);

			String newAditionalSourceLine = fileEntry->createDocument("", false, false);
            


#if 0
            String fileId = FileHelpers::createAlphaNumericUID();
            
            
            
			newAditionalSourceLine << "      <FILE id=\"" << fileId << "\" name=\"" << additionalSourceFiles[i].getFileName() << "\" compile=\"" << (isSourceFile ? "1" : "0") << "\" ";
			newAditionalSourceLine << "resource=\"" << (isSplashScreen ? "1" : "0") << "\"\r\n";
			newAditionalSourceLine << "            file=\"" << relativePath << "\"/>\r\n";
#endif

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
    auto& dataObject = dynamic_cast<BackendProcessor*>(chain->getMainController())->getSettingsObject();
    
    auto dobj = dataObject.getExtraDefinitionsAsObject();
    
    auto obj = dobj.getDynamicObject();
    
	// If we've defined this manually, we override the routing matrix value
	// in order to allow exporting multichannel plugins with a stereo output configuration
	if (obj->hasProperty("HISE_NUM_PLUGIN_CHANNELS"))
	{
		return "";
	}

    int numChannels = chain->getMatrix().getNumSourceChannels();
    
    if(IS_SETTING_TRUE(HiseSettings::Project::ForceStereoOutput))
        numChannels = 2;
    
	return "HISE_NUM_PLUGIN_CHANNELS=" + String(numChannels);
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

	auto header = headerFile.createOutputStream();

	if (header == nullptr)
	{
		std::cout << "Couldn't open "
			<< headerFile.getFullPathName() << " for writing" << std::endl << std::endl;
		return 0;
	}

	auto cpp = cppFile.createOutputStream();

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
    ignoreUnused(chainToExport);

	File batchFile = getBatchFile(exporter);

	if (!exporter->shouldBeSilent() && batchFile.existsAsFile())
	{
		if (!PresetHandler::showYesNoWindow("Batch File already found", "Do you want to rewrite the batch file for the compile process?"))
		{
			return;
		}
	}

    batchFile.deleteFile();
    
	const String buildPath = exporter->getBuildFolder().getFullPathName();

    String batchContent;
    
	const String projectName = exporter->GET_SETTING(HiseSettings::Project::Name);
    
	String projectType;

	switch (types)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin: projectType = "Instrument plugin"; break;
	case CompileExporter::TargetTypes::EffectPlugin: projectType = "FX plugin"; break;
	case CompileExporter::TargetTypes::StandaloneApplication: projectType = "Standalone application"; break;
    case CompileExporter::TargetTypes::MidiEffectPlugin: projectType = "MIDI FX plugin"; break;
    case CompileExporter::TargetTypes::numTargetTypes:
        default:                                        break;
	}

#if JUCE_WINDOWS
    
	const String msbuildPath = HelperClasses::isUsingVisualStudio2017(exporter->dataObject) ? 
		"\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\MSBuild\\15.0\\Bin\\MsBuild.exe\"" :
		"\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MsBuild.exe\"";

	const String projucerPath = exporter->hisePath.getChildFile("tools/Projucer/Projucer.exe").getFullPathName();
	
	const String vsArgs = "/p:Configuration=\"" + exporter->configurationName + "\" /verbosity:minimal";

	const String vsFolder = HelperClasses::isUsingVisualStudio2017(exporter->dataObject) ? "VisualStudio2017" : "VisualStudio2022";

	ADD_LINE("@echo off");

	ADD_LINE("set project=" << projectName);
	ADD_LINE("set build_path=" << buildPath);

	if (!exporter->rawMode)
	{
		
		ADD_LINE("set msbuild=" << msbuildPath);
		ADD_LINE("set vs_args=" << vsArgs);
		ADD_LINE("set PreferredToolArchitecture=x64");

		if (HelperClasses::isUsingVisualStudio2017(exporter->dataObject))
		{
			ADD_LINE("set VisualStudioVersion=15.0");
		}
		else
		{
			ADD_LINE("set VisualStudioVersion=17.0");
		}
	}
	
	ADD_LINE("");
	ADD_LINE("\"" << projucerPath << "\" --resave \"%build_path%\\AutogeneratedProject.jucer\"");
	ADD_LINE("");

	if (!exporter->rawMode && BuildOptionHelpers::is32Bit(buildOption))
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
	
	if (!exporter->rawMode && BuildOptionHelpers::is64Bit(buildOption))
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

	if (exporter->rawMode)
	{
		ADD_LINE("echo Project was exported succesfully. Open the VS2017 Solution file found in Binaries/Builds/VS2017");
	}

	if (!CompileExporter::isExportingFromCommandLine())
		ADD_LINE("pause");
	
    batchFile.replaceWithText(batchContent);
    
	

#elif JUCE_LINUX

	const String projucerPath = exporter->hisePath.getChildFile("tools/projucer/Projucer").getFullPathName();

	ADD_LINE("\"" << projucerPath << "\" --resave AutogeneratedProject.jucer");
	ADD_LINE("cd Builds/LinuxMakefile/");
	ADD_LINE("echo Compiling " << projectType << " " << projectName << " ...");
    ADD_LINE("make CONFIG=" << exporter->configurationName << " AR=gcc-ar -j`nproc --ignore=2`");
	ADD_LINE("echo Compiling finished. Cleaning up...");

	File tempFile = batchFile.getSiblingFile("tempBatch");
    
    tempFile.create();
    tempFile.replaceWithText(batchContent);
    
    String lineEndChange = "tr -d '\r' < \""+tempFile.getFullPathName()+"\" > \"" + batchFile.getFullPathName() + "\"";
    
    system(lineEndChange.getCharPointer());
    
    tempFile.deleteFile();


#else
    
	const String projucerPath = exporter->hisePath.getChildFile("tools/Projucer/Projucer.app/Contents/MacOS/Projucer").getFullPathName();

    ADD_LINE("chmod +x \"" << projucerPath << "\"");
    
    ADD_LINE("cd \"`dirname \"$0\"`\"");
    ADD_LINE("\"" << projucerPath << "\" --resave AutogeneratedProject.jucer");
    ADD_LINE("");
    
    if(exporter->rawMode || BuildOptionHelpers::isIOS(buildOption))
    {
		if (exporter->rawMode)
		{
			ADD_LINE("echo Sucessfully exported. Open the XCode project found under Binaries/Builds/MacOSX.");
		}
		else
		{
			ADD_LINE("echo Sucessfully exported. Open XCode to compile and transfer to your iOS device.");
		}

        
    }
    else
    {
        // Allow the errorcode to flow through xcpretty
        ADD_LINE("set -o pipefail");
        
        ADD_LINE("echo Compiling " << projectType << " " << projectName << " ...");

		int threads = SystemStats::getNumCpus() - 2;
		String xcodeLine;
        
		xcodeLine << "xcodebuild -project \"Builds/MacOSX/" << projectName << ".xcodeproj\" -configuration \"" << exporter->configurationName << "\" -jobs \"" << threads << "\"";
		xcodeLine << " | xcpretty";
		
        ADD_LINE(xcodeLine);
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
#if JUCE_WINDOWS
	return exporter->getBuildFolder().getChildFile("batchCompile.bat");
#elif JUCE_LINUX
	return exporter->getBuildFolder().getChildFile("batchCompileLinux.sh");
#else
    return exporter->getBuildFolder().getChildFile("batchCompileOSX");
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

bool CompileExporter::HelperClasses::isUsingVisualStudio2017(const HiseSettings::Data& dataObject)
{
	// Always use the version you build HISE with in CI mode
	if (isUsingCIMode())
	{
		return !HISE_USE_VS2022;
	}

	const String v = GET_SETTING(HiseSettings::Compiler::VisualStudioVersion);

	return v.isEmpty() || (v == "Visual Studio 2017");
}

CompileExporter::ErrorCodes CompileExporter::HelperClasses::saveProjucerFile(String templateProject, CompileExporter* exporter)
{
	XmlDocument doc(templateProject);

	if (auto xml = doc.getDocumentElement())
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

void CompileExporter::HeaderHelpers::addBasicIncludeLines(CompileExporter* exporter, String& p, bool isIOS)
{
	p << "\n";

	p << "#include \"JuceHeader.h\"" << "\n";
	p << "#include \"PresetData.h\"\n";

	p << "\nBEGIN_EMBEDDED_DATA()";
    
	auto& dataObject = exporter->dataObject;

    if(!isIOS)
    {
		if (IS_SETTING_TRUE(HiseSettings::Project::EmbedAudioFiles))
			p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::AudioFiles, PresetData::impulses, PresetData::impulsesSize);";
		else
			p << "\nDEFINE_EXTERNAL_DATA(hise::FileHandlerBase::AudioFiles)";

		if (IS_SETTING_TRUE(HiseSettings::Project::EmbedImageFiles))
			p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::Images, PresetData::images, PresetData::imagesSize);";
		else
			p << "\nDEFINE_EXTERNAL_DATA(hise::FileHandlerBase::Images);";

		p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::MidiFiles, PresetData::midiFiles, PresetData::midiFilesSize);";
        p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::SampleMaps, PresetData::samplemaps, PresetData::samplemapsSize);";
    }

	p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::Scripts, PresetData::externalFiles, PresetData::externalFilesSize);";
	p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::Presets, PresetData::preset, PresetData::presetSize);";
	p << "\nDEFINE_EMBEDDED_DATA(hise::FileHandlerBase::UserPresets, PresetData::userPresets, PresetData::userPresetsSize);";
	p << "\nEND_EMBEDDED_DATA()";
	p << "\n";
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
	pluginDataHeaderFile << "REGISTER_STATIC_DSP_LIBRARIES()" << "\n";
	pluginDataHeaderFile << "{" << "\n";
	pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(hise::HiseCoreDspFactory);" << "\n";

	const String additionalDspClasses = exporter->GET_SETTING(HiseSettings::Project::AdditionalDspLibraries);

	if (additionalDspClasses.isNotEmpty())
	{
		StringArray sa = StringArray::fromTokens(additionalDspClasses, ",;", "");

		for (int i = 0; i < sa.size(); i++)
			pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(" + sa[i] + ");" << "\n";
	}
    
	pluginDataHeaderFile << "}" << "\n";

	auto nodeIncludeFile = GET_PROJECT_HANDLER(exporter->chainToExport).getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile("nodes").getChildFile("includes.h");
	
	// We need to add this function body or the linker will complain (if the file exists, it'll be defined
	if(!nodeIncludeFile.existsAsFile())
		pluginDataHeaderFile << "scriptnode::dll::FactoryBase* scriptnode::DspNetwork::createStaticFactory() { return nullptr; }\n";
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

void CompileExporter::HeaderHelpers::addFullExpansionTypeSetter(CompileExporter* exporter, String& pluginDataHeaderFile)
{
	if (FullInstrumentExpansion::isEnabled(exporter->chainToExport->getMainController()))
		pluginDataHeaderFile << "\nSET_CUSTOM_EXPANSION_TYPE(FullInstrumentExpansion);\n";
}

void CompileExporter::HeaderHelpers::addProjectInfoLines(CompileExporter* exporter, String& pluginDataHeaderFile)
{
	const String companyName = exporter->GET_SETTING(HiseSettings::User::Company);
	const String companyWebsiteName = exporter->GET_SETTING(HiseSettings::User::CompanyURL);
	const String companyCopyright = exporter->GET_SETTING(HiseSettings::User::CompanyCopyright);
	const String projectName = exporter->GET_SETTING(HiseSettings::Project::Name);
	const String versionString = exporter->GET_SETTING(HiseSettings::Project::Version);
	const String appGroupString = exporter->GET_SETTING(HiseSettings::Project::AppGroupID);
	const String expType = exporter->GET_SETTING(HiseSettings::Project::ExpansionType);
	const String expKey = exporter->GET_SETTING(HiseSettings::Project::EncryptionKey);
	const String defaultPreset = exporter->GET_SETTING(HiseSettings::Project::DefaultUserPreset);
	const String hiseVersion = PresetHandler::getVersionString();

	String nl = "\n";
	
	pluginDataHeaderFile << "String hise::FrontendHandler::getProjectName() { return " << projectName.quoted() << "; };" << nl;
	pluginDataHeaderFile << "String hise::FrontendHandler::getCompanyName() { return " << companyName.quoted() << "; };" << nl;
	pluginDataHeaderFile << "String hise::FrontendHandler::getCompanyWebsiteName() { return " << companyWebsiteName.quoted() << "; };" << nl;
	pluginDataHeaderFile << "String hise::FrontendHandler::getCompanyCopyright() { return " << companyCopyright.quoted() << "; };" << nl;
	pluginDataHeaderFile << "String hise::FrontendHandler::getVersionString() { return " << versionString.quoted() << "; };" << nl;
    
    pluginDataHeaderFile << "String hise::FrontendHandler::getAppGroupId() { return " << appGroupString.quoted() << "; };" << nl;
    
	pluginDataHeaderFile << "String hise::FrontendHandler::getExpansionKey() { return " << expKey.quoted() << "; };" << nl;
	pluginDataHeaderFile << "String hise::FrontendHandler::getExpansionType() { return " << expType.quoted() << "; };" << nl;
	pluginDataHeaderFile << "String hise::FrontendHandler::getHiseVersion() { return " << hiseVersion.quoted() << "; };" << nl;

	pluginDataHeaderFile << "String hise::FrontendHandler::getDefaultUserPreset() const { return " << defaultPreset.quoted() << "; };" << nl;
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
