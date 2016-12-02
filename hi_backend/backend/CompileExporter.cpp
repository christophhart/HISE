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


void CompileExporter::exportMainSynthChainAsInstrument(ModulatorSynthChain *chainToExport)
{
	exportInternal(chainToExport, TargetTypes::InstrumentPlugin);
}


void CompileExporter::exportMainSynthChainAsFX(ModulatorSynthChain* chainToExport)
{
	exportInternal(chainToExport, TargetTypes::EffectPlugin);
}

void CompileExporter::exportMainSynthChainAsStandaloneApp(ModulatorSynthChain * chainToExport)
{
	exportInternal(chainToExport, TargetTypes::StandaloneApplication);
}

void CompileExporter::exportInternal(ModulatorSynthChain* chainToExport, TargetTypes type)
{
	if (!checkSanity(chainToExport)) return;

	String uniqueId, version, solutionDirectory, publicKey;
	BuildOption buildOption = showCompilePopup(publicKey, uniqueId, version, solutionDirectory);

	publicKey = GET_PROJECT_HANDLER(chainToExport).getPublicKey();
	uniqueId = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, &GET_PROJECT_HANDLER(chainToExport));
	version = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, &GET_PROJECT_HANDLER(chainToExport));

	solutionDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getFullPathName();

	if (buildOption != Cancelled)
	{
		if (type != TargetTypes::StandaloneApplication)
		{
			createPluginDataHeaderFile(chainToExport, solutionDirectory, uniqueId, version, publicKey);
		}
		else
		{
			createStandaloneAppHeaderFile(chainToExport, solutionDirectory, uniqueId, version, publicKey);
		}

		copyHISEImageFiles(chainToExport);

		const String directoryPath = File(solutionDirectory).getChildFile("temp/").getFullPathName();

		convertTccScriptsToCppClasses(chainToExport);
		writePresetFile(chainToExport, directoryPath, uniqueId);
		writeEmbeddedFiles(chainToExport, directoryPath);
		writeUserPresetFiles(chainToExport, directoryPath);

		writeReferencedAudioFiles(chainToExport, directoryPath);
		writeReferencedImageFiles(chainToExport, directoryPath);

		String presetDataString("PresetData");

		String sourceDirectory = solutionDirectory + "/Source";

		CppBuilder::exportValueTreeAsCpp(directoryPath, sourceDirectory, presetDataString);

		if (type == TargetTypes::StandaloneApplication)
		{
			createStandaloneAppIntrojucerFile(chainToExport);
		}
		else
		{
			createPluginIntrojucerFile(chainToExport, type);
		}

		compileSolution(chainToExport, buildOption, type);
	}
}

bool CompileExporter::checkSanity(ModulatorSynthChain *chainToExport)
{
	// Check if a frontend script is in the main synth chain

    const bool frontWasFound = chainToExport->hasDefinedFrontInterface();

	if (!frontWasFound)
	{
		PresetHandler::showMessageWindow("No Interface found.", "You have to add at least one script processor and call Synth.addToFront(true).", PresetHandler::IconType::Error);
		return false;
	}

	// Check the settings are correct

	ProjectHandler *handler = &GET_PROJECT_HANDLER(chainToExport);

	const String productName = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, handler);

	if (!productName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
	{
		PresetHandler::showMessageWindow("Illegal Project name", "The Project name must not contain exotic characters", PresetHandler::IconType::Error);
		return false;
	}

	return true;
}

CompileExporter::BuildOption CompileExporter::showCompilePopup(String &/*publicKey*/, String &/*uniqueId*/, String &/*version*/, String &/*solutionDirectory*/)
{
	AlertWindowLookAndFeel pplaf;

	AlertWindow w("Compile Patch as plugin", String::empty, AlertWindow::AlertIconType::NoIcon);

	w.setLookAndFeel(&pplaf);

	w.setUsingNativeTitleBar(true);

	w.setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	w.setColour(AlertWindow::textColourId, Colours::white);

	StringArray items;

#if JUCE_WINDOWS
    
	items.add("32bit VST");
	items.add("64bit VST");
	items.add("32bit & 64bit VSTs");

#else

    items.add("VST / AU");
    
#endif
    
	w.addComboBox("buildOption", items, "Plugin Format");


	w.addButton("OK", 1, KeyPress(KeyPress::returnKey));
	w.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	w.getComboBoxComponent("buildOption")->setLookAndFeel(&pplaf);

	if (w.runModalLoop())
	{
		int i = w.getComboBoxComponent("buildOption")->getSelectedItemIndex() + 1;

		return (BuildOption)i;
	}
	else
	{
		return Cancelled;
	}

}

void CompileExporter::writeReferencedImageFiles(ModulatorSynthChain * chainToExport, const String directoryPath)
{
	// Export the interface

	ImagePool *imagePool = chainToExport->getMainController()->getSampleManager().getImagePool();
	ValueTree imageTree = imagePool->exportAsValueTree();
	PresetHandler::writeValueTreeAsFile(imageTree, directoryPath + "/images");
}

void CompileExporter::writeReferencedAudioFiles(ModulatorSynthChain * chainToExport, const String directoryPath)
{
	// Search for impulse responses

    DirectoryIterator iter(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AudioFiles), false);
    
    AudioSampleBufferPool *samplePool = chainToExport->getMainController()->getSampleManager().getAudioSampleBufferPool();
    
    while(iter.next())
    {
#if JUCE_WINDOWS

		// Skip OSX hidden files on windows...
		if (iter.getFile().getFileName().startsWith(".")) continue;

#endif
        samplePool->loadFileIntoPool(iter.getFile().getFullPathName());
    }
    
	ValueTree sampleTree = samplePool->exportAsValueTree();

	if (SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::EmbedAudioFiles, &GET_PROJECT_HANDLER(chainToExport)) == "No")
	{
		PresetHandler::writeValueTreeAsFile(sampleTree, ProjectHandler::Frontend::getAppDataDirectory(&GET_PROJECT_HANDLER(chainToExport)).getChildFile("AudioResources.dat").getFullPathName());
	}
	else
	{
		PresetHandler::writeValueTreeAsFile(sampleTree, File(directoryPath).getChildFile("impulses").getFullPathName(), true);
	}
}

void CompileExporter::writeUserPresetFiles(ModulatorSynthChain * chainToExport, const String &directoryPath)
{
	File presetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

	DirectoryIterator iter(presetDirectory, true, "*", File::findFiles);

	ValueTree userPresets("UserPresets");

	while (iter.next())
	{
		File f = iter.getFile();

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
    
	PresetHandler::writeValueTreeAsFile(userPresets, File(directoryPath).getChildFile("userPresets").getFullPathName());
}

void CompileExporter::writeEmbeddedFiles(ModulatorSynthChain * chainToExport, const String &directoryPath)
{
	ValueTree externalScriptFiles = FileChangeListener::collectAllScriptFiles(chainToExport);
	ValueTree customFonts = chainToExport->getMainController()->exportCustomFontsAsValueTree();
	ValueTree sampleMaps = collectAllSampleMapsInDirectory(chainToExport);


	ValueTree externalFiles("ExternalFiles");
	externalFiles.addChild(externalScriptFiles, -1, nullptr);
	externalFiles.addChild(customFonts, -1, nullptr);
	externalFiles.addChild(sampleMaps, -1, nullptr);

	PresetHandler::writeValueTreeAsFile(externalFiles, File(directoryPath).getChildFile("externalFiles").getFullPathName(), true);
}

void CompileExporter::writePresetFile(ModulatorSynthChain * chainToExport, const String directoryPath, const String &/*uniqueName*/)
{
	ValueTree preset = chainToExport->exportAsValueTree();

	PresetHandler::stripViewsFromPreset(preset);

	File presetFile(directoryPath + "/preset");

	ReferenceSearcher::writeValueTreeToFile(preset, presetFile);
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

void CompileExporter::convertTccScriptsToCppClasses(ModulatorSynthChain* chainToExport)
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

CompileExporter::ErrorCodes CompileExporter::compileSolution(ModulatorSynthChain *chainToExport, BuildOption buildOption, TargetTypes type)
{
	BatchFileCreator::createBatchFile(chainToExport, buildOption, type);

	File batchFile = BatchFileCreator::getBatchFile(chainToExport);
    
#if JUCE_WINDOWS
    
    String command = "\"" + batchFile.getFullPathName() + "\"";
    
#else
    
    String permissionCommand = "chmod +x \"" + batchFile.getFullPathName() + "\"";
    system(permissionCommand.getCharPointer());
    
    String command = "open \"" + batchFile.getFullPathName() + "\"";

#endif
    
	int returnType = system(command.getCharPointer());

	if (returnType != 0)
	{
		PresetHandler::showMessageWindow("Compile Export Error", "The Project is invalid or the Introjucer could not be found.", PresetHandler::IconType::Error);

		return ErrorCodes::ProjectXmlInvalid;
	}

	batchFile.getParentDirectory().getChildFile("temp/").deleteRecursively();
    
    return ErrorCodes::OK;
}


class StringObfuscater
{
public:

	static String getStringConcatenationExpression(Random& rng, int start, int length)
	{
		jassert(length > 0);

		if (length == 1)
			return "s" + String(start);

		int breakPos = jlimit(1, length - 1, (length / 3) + rng.nextInt(length / 3));

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

		out << "RSAKey Unlocker::getPublicKey()" << newLine
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





CompileExporter::ErrorCodes CompileExporter::createPluginDataHeaderFile(ModulatorSynthChain* chainToExport,
																		const String &solutionDirectory, 
																		const String &/*uniqueName*/, 
																		const String &/*version*/, 
																		const String &publicKey)
{
	String pluginDataHeaderFile;

	HeaderHelpers::addBasicIncludeLines(pluginDataHeaderFile);

	HeaderHelpers::addAdditionalSourceCodeHeaderLines(chainToExport, pluginDataHeaderFile);
	HeaderHelpers::addStaticDspFactoryRegistration(pluginDataHeaderFile, chainToExport);
	HeaderHelpers::addCopyProtectionHeaderLines(publicKey, pluginDataHeaderFile);

	if (SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::EmbedAudioFiles, &GET_PROJECT_HANDLER(chainToExport)) == "No")
	{
		pluginDataHeaderFile << "AudioProcessor* JUCE_CALLTYPE createPluginFilter() { CREATE_PLUGIN(nullptr, nullptr); }\n";
		pluginDataHeaderFile << "\n";
	}
	else
	{
		pluginDataHeaderFile << "AudioProcessor* JUCE_CALLTYPE createPluginFilter() { CREATE_PLUGIN_WITH_AUDIO_FILES(nullptr, nullptr); }\n";
		pluginDataHeaderFile << "\n";
	}

	pluginDataHeaderFile << "AudioProcessor* StandaloneProcessor::createProcessor() { return nullptr; }\n";

	HeaderHelpers::addProjectInfoLines(chainToExport, pluginDataHeaderFile);
	HeaderHelpers::addCustomToolbarRegistration(chainToExport, pluginDataHeaderFile);
	HeaderHelpers::writeHeaderFile(solutionDirectory, pluginDataHeaderFile);

	return ErrorCodes::OK;
}

void CompileExporter::createStandaloneAppHeaderFile(ModulatorSynthChain* chainToExport, const String& solutionDirectory, const String& /*uniqueId*/, const String& /*version*/, String publicKey)
{
	String pluginDataHeaderFile;

	HeaderHelpers::addBasicIncludeLines(pluginDataHeaderFile);

	HeaderHelpers::addAdditionalSourceCodeHeaderLines(chainToExport, pluginDataHeaderFile);
	HeaderHelpers::addStaticDspFactoryRegistration(pluginDataHeaderFile, chainToExport);
	HeaderHelpers::addCopyProtectionHeaderLines(publicKey, pluginDataHeaderFile);

	if (SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::EmbedAudioFiles, &GET_PROJECT_HANDLER(chainToExport)) == "No")
	{
		pluginDataHeaderFile << "AudioProcessor* StandaloneProcessor::createProcessor() { CREATE_PLUGIN(deviceManager, callback); }\n";
		pluginDataHeaderFile << "\n";
		pluginDataHeaderFile << "START_JUCE_APPLICATION(FrontendStandaloneApplication)\n";
	}
	else
	{
		pluginDataHeaderFile << "AudioProcessor* StandaloneProcessor::createProcessor() { CREATE_PLUGIN_WITH_AUDIO_FILES(deviceManager, callback); }\n";
		pluginDataHeaderFile << "\n";
		pluginDataHeaderFile << "START_JUCE_APPLICATION(FrontendStandaloneApplication)\n";
	}

	HeaderHelpers::addProjectInfoLines(chainToExport, pluginDataHeaderFile);
	HeaderHelpers::addCustomToolbarRegistration(chainToExport, pluginDataHeaderFile);
	HeaderHelpers::writeHeaderFile(solutionDirectory, pluginDataHeaderFile);
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

	File resourcesFileObject(solutionDirectory + "/Builds/VisualStudio2013/resources.rc");

	resourcesFileObject.deleteFile();

	resourcesFileObject.create();
	resourcesFileObject.appendText(resourcesFile);

	return ErrorCodes::OK;
}

#define REPLACE_WILDCARD(wildcard, settingId) (templateProject = templateProject.replace(wildcard, SettingWindows::getSettingValue((int)settingId, &GET_PROJECT_HANDLER(chainToExport))))
#define REPLACE_WILDCARD_WITH_STRING(wildcard, s) (templateProject = templateProject.replace(wildcard, s))

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



CompileExporter::ErrorCodes CompileExporter::createPluginIntrojucerFile(ModulatorSynthChain *chainToExport, CompileExporter::TargetTypes type)
{
	String templateProject = String(projectTemplate_jucer);

	REPLACE_WILDCARD("%NAME%", SettingWindows::ProjectSettingWindow::Attributes::Name);
	REPLACE_WILDCARD("%VERSION%", SettingWindows::ProjectSettingWindow::Attributes::Version);
	REPLACE_WILDCARD("%DESCRIPTION%", SettingWindows::ProjectSettingWindow::Attributes::Description);
	REPLACE_WILDCARD("%BUNDLE_ID%", SettingWindows::ProjectSettingWindow::Attributes::BundleIdentifier);
	REPLACE_WILDCARD("%PC%", SettingWindows::ProjectSettingWindow::Attributes::PluginCode);

	ProjectTemplateHelpers::handleVisualStudioVersion(chainToExport, templateProject);

	if (type == TargetTypes::EffectPlugin)
	{
		REPLACE_WILDCARD_WITH_STRING("%CHANNEL_CONFIG%", "{2, 2}");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "0");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN", "0");
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "enabled");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%CHANNEL_CONFIG%", "{0, 2}");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINISSYNTH%", "1");
		REPLACE_WILDCARD_WITH_STRING("%PLUGINWANTSMIDIIN", "1");
		REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
	}

	REPLACE_WILDCARD_WITH_STRING("%IS_STANDALONE_FRONTEND%", "disabled");

	ProjectTemplateHelpers::handleCompanyInfo(chainToExport, templateProject);

	REPLACE_WILDCARD("%VSTSDK_FOLDER%", SettingWindows::CompilerSettingWindow::Attributes::VSTSDKPath);

	ProjectTemplateHelpers::handleCompilerInfo(chainToExport, templateProject);

	ProjectTemplateHelpers::handleAdditionalSourceCode(chainToExport, templateProject);
	ProjectTemplateHelpers::handleCopyProtectionInfo(chainToExport, templateProject);

	return HelperClasses::saveIntrojucerFile(templateProject, chainToExport);
}

CompileExporter::ErrorCodes CompileExporter::createStandaloneAppIntrojucerFile(ModulatorSynthChain* chainToExport)
{
	String templateProject = String(projectStandaloneTemplate_jucer);

	REPLACE_WILDCARD("%NAME%", SettingWindows::ProjectSettingWindow::Attributes::Name);
	REPLACE_WILDCARD("%VERSION%", SettingWindows::ProjectSettingWindow::Attributes::Version);
	REPLACE_WILDCARD("%BUNDLE_ID%", SettingWindows::ProjectSettingWindow::Attributes::BundleIdentifier);


	const bool useAsio = SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::UseASIO) == "Yes";

	REPLACE_WILDCARD("%ASIO_SDK_PATH%", SettingWindows::CompilerSettingWindow::Attributes::ASIOSDKPath);
	REPLACE_WILDCARD_WITH_STRING("%USE_ASIO%", useAsio ? "enabled" : "disabled");
	REPLACE_WILDCARD_WITH_STRING("%FRONTEND_IS_PLUGIN%", "disabled");
	REPLACE_WILDCARD_WITH_STRING("%IS_STANDALONE_FRONTEND%", "enabled");

	ProjectTemplateHelpers::handleVisualStudioVersion(chainToExport, templateProject);

	ProjectTemplateHelpers::handleCompanyInfo(chainToExport, templateProject);
	ProjectTemplateHelpers::handleCompilerInfo(chainToExport, templateProject);

	ProjectTemplateHelpers::handleAdditionalSourceCode(chainToExport, templateProject);
	ProjectTemplateHelpers::handleCopyProtectionInfo(chainToExport, templateProject);

	return HelperClasses::saveIntrojucerFile(templateProject, chainToExport);
}



void CompileExporter::ProjectTemplateHelpers::handleCompilerInfo(ModulatorSynthChain* chainToExport, String& templateProject)
{
	REPLACE_WILDCARD("%JUCE_PATH%", SettingWindows::CompilerSettingWindow::Attributes::JucePath);
	REPLACE_WILDCARD("%HISE_PATH%", SettingWindows::CompilerSettingWindow::Attributes::HisePath);
	REPLACE_WILDCARD("%USE_IPP%", SettingWindows::CompilerSettingWindow::Attributes::IPPInclude);

#if JUCE_MAC
	REPLACE_WILDCARD("%IPP_COMPILER_FLAGS%", SettingWindows::CompilerSettingWindow::Attributes::IPPLinker);
	REPLACE_WILDCARD("%IPP_HEADER%", SettingWindows::CompilerSettingWindow::Attributes::IPPInclude);
	REPLACE_WILDCARD("%IPP_LIBRARY%", SettingWindows::CompilerSettingWindow::Attributes::IPPLibrary);
#endif
}

void CompileExporter::ProjectTemplateHelpers::handleCompanyInfo(ModulatorSynthChain* chainToExport, String& templateProject)
{
	REPLACE_WILDCARD("%COMPANY%", SettingWindows::UserSettingWindow::Attributes::Company);
	REPLACE_WILDCARD("%MC%", SettingWindows::UserSettingWindow::Attributes::CompanyCode);
	REPLACE_WILDCARD("%COMPANY_WEBSITE%", SettingWindows::UserSettingWindow::Attributes::CompanyURL);
}

void CompileExporter::ProjectTemplateHelpers::handleVisualStudioVersion(ModulatorSynthChain * chainToExport, String& templateProject)
{
	const bool isUsingVisualStudio2015 = HelperClasses::isUsingVisualStudio2015(chainToExport);

	if (isUsingVisualStudio2015)
	{
		REPLACE_WILDCARD_WITH_STRING("%VS_VERSION%", "VS2015");
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FOLDER%", "VisualStudio2015");
	}
	else
	{
		REPLACE_WILDCARD_WITH_STRING("%VS_VERSION%", "VS2013");
		REPLACE_WILDCARD_WITH_STRING("%TARGET_FOLDER%", "VisualStudio2013");
	}
}



void CompileExporter::ProjectTemplateHelpers::handleAdditionalSourceCode(ModulatorSynthChain * chainToExport, String &templateProject)
{
	Array<File> additionalSourceFiles;

	File additionalSourceCodeDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode);

	File additionalMainHeaderFile = additionalSourceCodeDirectory.getChildFile("AdditionalSourceCode.h");

	if (additionalMainHeaderFile.existsAsFile())
	{
		additionalSourceFiles.add(additionalMainHeaderFile);
	}

    File iconFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Images).getChildFile("Icon.png");
    
    if(iconFile.existsAsFile())
    {
        additionalSourceFiles.add(iconFile);
    }
	
	//additionalSourceCodeDirectory.findChildFiles(additionalSourceFiles, File::findFiles, true);

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

			const String relativePath = additionalSourceFiles[i].getRelativePathFrom(GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries));

			String newAditionalSourceLine;
            
            
            String fileId = FileHelpers::createAlphaNumericUID();
            
            if(additionalSourceFiles[i].getFileName() == "Icon.png")
            {
                templateProject = templateProject.replace("%ICON_FILE%", "smallIcon=\"" + fileId + "\" bigIcon=\"" + fileId + "\"");
            }
            
			newAditionalSourceLine << "      <FILE id=\"" << fileId << "\" name=\"" << additionalSourceFiles[i].getFileName() << "\" compile=\"" << (isSourceFile ? "1" : "0") << "\" resource=\"0\"\r\n";
			newAditionalSourceLine << "            file=\"" << relativePath << "\"/>\r\n";

			additionalFileDefinitions.add(newAditionalSourceLine);
            
            
		}

        templateProject = templateProject.replace("%ICON_FILE%", "");
        
		templateProject = templateProject.replace("%ADDITIONAL_FILES%", additionalFileDefinitions.joinIntoString(""));

		if (additionalMainHeaderFile.existsAsFile())
		{
			const String customToolbarName = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::CustomToolbarClassName, &GET_PROJECT_HANDLER(chainToExport));

			templateProject = templateProject.replace("%USE_CUSTOM_FRONTEND_TOOLBAR%", customToolbarName.isNotEmpty() ? "enabled" : "disabled");
		}
	}

}

void CompileExporter::ProjectTemplateHelpers::handleCopyProtectionInfo(ModulatorSynthChain * chainToExport, String &templateProject)
{
	const bool useCopyProtection = GET_PROJECT_HANDLER(chainToExport).getPublicKey().isNotEmpty();

	templateProject = templateProject.replace("%USE_COPY_PROTECTION%", useCopyProtection ? "enabled" : "disabled");
}

CompileExporter::ErrorCodes CompileExporter::copyHISEImageFiles(ModulatorSynthChain *chainToExport)
{
	String hisePath = SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::HisePath);

	File imageDirectory = File(hisePath).getChildFile("hi_core/hi_images/");

	File targetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("Source/Images/");

	targetDirectory.createDirectory();

	if (!imageDirectory.isDirectory())
	{
		return ErrorCodes::HISEImageDirectoryNotFound;
	}

	if (!imageDirectory.copyDirectoryTo(targetDirectory)) return ErrorCodes::HISEImageDirectoryNotFound;

	return ErrorCodes::OK;
}

File CompileExporter::getIntrojucerProjectFile(ModulatorSynthChain *chainToExport)
{
	return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("AutogeneratedProject.jucer");
}



ValueTree CompileExporter::collectAllSampleMapsInDirectory(ModulatorSynthChain * chainToExport)
{
	ValueTree sampleMaps("SampleMaps");

	File sampleMapDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);

	Array<File> sampleMapFiles;

	sampleMapDirectory.findChildFiles(sampleMapFiles, File::findFiles, false, "*.xml");

	for (int i = 0; i < sampleMapFiles.size(); i++)
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(sampleMapFiles[i]);

		if (xml != nullptr)
		{
			ValueTree sampleMap = ValueTree::fromXml(*xml);
			sampleMaps.addChild(sampleMap, -1, nullptr);
		}
	}

	return sampleMaps;
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

void CompileExporter::BatchFileCreator::createBatchFile(ModulatorSynthChain *chainToExport, BuildOption buildOption, TargetTypes types)
{
	File batchFile = getBatchFile(chainToExport);

	if (batchFile.existsAsFile())
	{
		if (!PresetHandler::showYesNoWindow("Batch File already found", "Do you want to rewrite the batch file for the compile process?"))
		{
			return;
		}
	}

    batchFile.deleteFile();
    
	

	const String buildPath = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getFullPathName();

    String batchContent;
    
    const String projectName = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, &GET_PROJECT_HANDLER(chainToExport));
    
	String projectType;

	switch (types)
	{
	case CompileExporter::TargetTypes::InstrumentPlugin: projectType = "Instrument plugin"; break;
	case CompileExporter::TargetTypes::EffectPlugin: projectType = "FX plugin"; break;
	case CompileExporter::TargetTypes::StandaloneApplication: projectType = "Standalone application"; break;
    case CompileExporter::TargetTypes::numTargetTypes: break;
	}

#if JUCE_WINDOWS
    
	const bool isUsingVisualStudio2015 = SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::VisualStudioVersion, &GET_PROJECT_HANDLER(chainToExport)) == "Visual Studio 2015";



	const String msbuildPath = isUsingVisualStudio2015 ? "\"C:\\Program Files (x86)\\MSBuild\\14.0\\Bin\\MsBuild.exe\"" :
														 "\"C:\\Program Files (x86)\\MSBuild\\12.0\\Bin\\MsBuild.exe\"";

    const String introjucerPath = File(SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::IntrojucerPath)).getChildFile("The Introjucer.exe").getFullPathName();

	ADD_LINE("@echo off");
	ADD_LINE("set project=" << projectName);
	ADD_LINE("");
	ADD_LINE("cd \"" << buildPath << "\"");
	ADD_LINE("\"" << introjucerPath << "\" --resave AutogeneratedProject.jucer");
	ADD_LINE("");

	

	if (buildOption == VSTx86 || buildOption == VSTx64x86)
	{
		ADD_LINE("echo Compiling 32bit " << projectType << " %project% ...");

		if (isUsingVisualStudio2015)
		{
			ADD_LINE("set VisualStudioVersion=14.0");
		}
		else
		{
			ADD_LINE("set VisualStudioVersion=12.0");
		}

		ADD_LINE("set Platform=Win32");

		if (isUsingVisualStudio2015)
		{
			ADD_LINE(msbuildPath << " \"Builds\\VisualStudio2015\\%project%.sln\" /p:Configuration=\"Release\" /verbosity:minimal");
		}
		else
		{
			ADD_LINE(msbuildPath << " \"Builds\\VisualStudio2013\\%project%.sln\" /p:Configuration=\"Release\" /verbosity:minimal");
		}

		ADD_LINE("");
		ADD_LINE("echo Compiling finished.Cleaning up...");
		ADD_LINE("del \"Compiled\\%project%.exp\"");
		ADD_LINE("del \"Compiled\\%project%.lib\"");
		ADD_LINE("");
	}
	
	if (buildOption == VSTx64 || buildOption == VSTx64x86)
	{
		ADD_LINE("echo Compiling 64bit " << projectType << " %project% ...");
		
		if (isUsingVisualStudio2015)
		{
			ADD_LINE("set VisualStudioVersion=14.0");
		}
		else
		{
			ADD_LINE("set VisualStudioVersion=12.0");
		}

		ADD_LINE("set Platform=X64");
		
		if (isUsingVisualStudio2015)
		{
			ADD_LINE(msbuildPath << " \"Builds\\VisualStudio2015\\%project%.sln\" /p:Configuration=\"Release\" /verbosity:minimal");
		}
		else
		{
			ADD_LINE(msbuildPath << " \"Builds\\VisualStudio2013\\%project%.sln\" /p:Configuration=\"Release\" /verbosity:minimal");
		}

		ADD_LINE("");
		ADD_LINE("echo Compiling finished.Cleaning up...");
		ADD_LINE("del \"Compiled\\%project% x64.exp\"");
		ADD_LINE("del \"Compiled\\%project% x64.lib\"");
	}

	ADD_LINE("");
	ADD_LINE("pause");
    
    batchFile.replaceWithText(batchContent);
    
#else
    
    const String introjucerPath = File(SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::IntrojucerPath)).getChildFile("The Introjucer.app/Contents/MacOS/Introjucer").getFullPathName();

    ADD_LINE("cd \"`dirname \"$0\"`\"");
    ADD_LINE("\"" << introjucerPath << "\" --resave AutogeneratedProject.jucer");
    ADD_LINE("");
	ADD_LINE("echo Compiling " << projectType << " " << projectName << " ...");
    ADD_LINE("xcodebuild -project \"Builds/MacOSX/" << projectName << ".xcodeproj\" -configuration \"Release\"");
    ADD_LINE("echo Compiling finished. Cleaning up...");
    
    File tempFile = batchFile.getSiblingFile("tempBatch");
    
    tempFile.create();
    tempFile.replaceWithText(batchContent);
    
    String lineEndChange = "tr -d '\r' < \""+tempFile.getFullPathName()+"\" > \"" + batchFile.getFullPathName() + "\"";
    
    system(lineEndChange.getCharPointer());
    
    tempFile.deleteFile();
    
#endif
}

#undef ADD_LINE

File CompileExporter::BatchFileCreator::getBatchFile(ModulatorSynthChain *chainToExport)
{
#if JUCE_WINDOWS
	return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("batchCompile.bat");
#else
    return GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::Binaries).getChildFile("batchCompileOSX");
#endif
}

bool CompileExporter::HelperClasses::isUsingVisualStudio2015(ModulatorSynthChain* chain)
{
	return SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::VisualStudioVersion, &GET_PROJECT_HANDLER(chain)) == "Visual Studio 2015";
}

CompileExporter::ErrorCodes CompileExporter::HelperClasses::saveIntrojucerFile(String templateProject, ModulatorSynthChain * chainToExport)
{
	XmlDocument doc(templateProject);

	ScopedPointer<XmlElement> xml = doc.getDocumentElement();

	jassert(xml != nullptr);

	if (xml != nullptr)
	{
		File projectFile = getIntrojucerProjectFile(chainToExport);

		projectFile.create();

		projectFile.replaceWithText(xml->createDocument(""));
	}
	else
	{
		PresetHandler::showMessageWindow("Invalid XML", doc.getLastParseError(), PresetHandler::IconType::Error);
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

void CompileExporter::HeaderHelpers::addAdditionalSourceCodeHeaderLines(ModulatorSynthChain* chainToExport, String& pluginDataHeaderFile)
{
	File addSourceFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode).getChildFile("AdditionalSourceCode.h");

	if (addSourceFile.existsAsFile())
	{
		pluginDataHeaderFile << "#include \"../../AdditionalSourceCode/AdditionalSourceCode.h\"\n";
	}

	pluginDataHeaderFile << "\n";
}

void CompileExporter::HeaderHelpers::addStaticDspFactoryRegistration(String& pluginDataHeaderFile, ModulatorSynthChain* chainToExport)
{
	pluginDataHeaderFile << "REGISTER_STATIC_DSP_LIBRARIES()" << "\n";
	pluginDataHeaderFile << "{" << "\n";
	pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(HiseCoreDspFactory);" << "\n";

	File tccConvertedFile = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::AdditionalSourceCode).getChildFile("ConvertedTccScriptFactory.cpp");

	if (tccConvertedFile.existsAsFile())
	{
		pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(ConvertedTccScriptFactory);" << "\n";
	}

#if JUCE_MAC
	const String additionalDspClasses = SettingWindows::getSettingValue(
		(int)SettingWindows::ProjectSettingWindow::Attributes::AdditionalDspLibraries,
		&GET_PROJECT_HANDLER(chainToExport));

	if (additionalDspClasses.isNotEmpty())
	{
		StringArray sa = StringArray::fromTokens(additionalDspClasses, ",;", "");

		for (int i = 0; i < sa.size(); i++)
			pluginDataHeaderFile << "\tREGISTER_STATIC_DSP_FACTORY(" + sa[i] + ");" << "\n";
	}
#endif
    
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
		pluginDataHeaderFile << "RSAKey Unlocker::getPublicKey() { return RSAKey(\"\"); };" << "\n";
		pluginDataHeaderFile << "#endif" << "\n";
	}
}

void CompileExporter::HeaderHelpers::addCustomToolbarRegistration(ModulatorSynthChain* chainToExport, String& pluginDataHeaderFile)
{
	String customToolbarName = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::CustomToolbarClassName, &GET_PROJECT_HANDLER(chainToExport));

	if (customToolbarName.isEmpty()) customToolbarName = "DefaultFrontendBar";

	pluginDataHeaderFile << "\nCREATE_FRONTEND_BAR(" + customToolbarName + ")\n\n";
}

void CompileExporter::HeaderHelpers::addProjectInfoLines(ModulatorSynthChain* chainToExport, String& pluginDataHeaderFile)
{
	const String companyName = SettingWindows::getSettingValue((int)SettingWindows::UserSettingWindow::Attributes::Company, &GET_PROJECT_HANDLER(chainToExport));
	const String companyWebsiteName = SettingWindows::getSettingValue((int)SettingWindows::UserSettingWindow::Attributes::CompanyURL, &GET_PROJECT_HANDLER(chainToExport));
	const String projectName = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, &GET_PROJECT_HANDLER(chainToExport));
	const String versionString = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, &GET_PROJECT_HANDLER(chainToExport));

	pluginDataHeaderFile << "String ProjectHandler::Frontend::getProjectName() { return \"" << projectName << "\"; };\n";
	pluginDataHeaderFile << "String ProjectHandler::Frontend::getCompanyName() { return \"" << companyName << "\"; };\n";
	pluginDataHeaderFile << "String ProjectHandler::Frontend::getCompanyWebsiteName() { return \"" << companyWebsiteName << "\"; };\n";
	pluginDataHeaderFile << "String ProjectHandler::Frontend::getVersionString() { return \"" << versionString << "\"; };\n";
}

void CompileExporter::HeaderHelpers::writeHeaderFile(const String & solutionDirectory, const String& pluginDataHeaderFile)
{
	File pluginDataHeader = File(solutionDirectory).getChildFile("Source/Plugin.cpp");

	pluginDataHeader.create();
	pluginDataHeader.replaceWithText(pluginDataHeaderFile);
}
