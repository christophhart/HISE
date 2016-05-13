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


void CompileExporter::exportMainSynthChainAsPackage(ModulatorSynthChain *chainToExport)
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
		createPluginDataHeaderFile(solutionDirectory, uniqueId, version, publicKey);
		
		copyHISEImageFiles(chainToExport);

		const String directoryPath = File(solutionDirectory).getChildFile("temp/").getFullPathName();

		writePresetFile(chainToExport, directoryPath, uniqueId);
		writeExternalScriptFiles(chainToExport, directoryPath);
		writeUserPresetFiles(chainToExport, directoryPath);
		writeReferencedAudioFiles(chainToExport, directoryPath);
		writeReferencedImageFiles(chainToExport, directoryPath);

		String presetDataString("PresetData");

		String sourceDirectory = solutionDirectory + "/Source";

		CppBuilder::exportValueTreeAsCpp(directoryPath, sourceDirectory, presetDataString);

		createIntrojucerFile(chainToExport);

		compileSolution(chainToExport, buildOption);
	}
}


bool CompileExporter::checkSanity(ModulatorSynthChain *chainToExport)
{
	// Check if a frontend script is in the main synth chain

	MidiProcessorChain *mc = dynamic_cast<MidiProcessorChain*>(chainToExport->getChildProcessor(ModulatorSynth::MidiProcessor));

	bool frontWasFound = false;

	for (int i = 0; i < mc->getNumChildProcessors(); i++)
	{
		if (ScriptProcessor *sp = dynamic_cast<ScriptProcessor*>(mc->getChildProcessor(i)))
		{
			if (sp->isFront())
			{
				frontWasFound = true;
				break;
			}
		}
	}

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
        samplePool->loadFileIntoPool(iter.getFile().getFullPathName());
    }
    
	
	ValueTree sampleTree = samplePool->exportAsValueTree();
	PresetHandler::writeValueTreeAsFile(sampleTree, File(directoryPath).getChildFile("impulses").getFullPathName());
}

void CompileExporter::writeExternalScriptFiles(ModulatorSynthChain * chainToExport, const String &directoryPath)
{
	Processor::Iterator<ScriptProcessor> iter(chainToExport);

	ValueTree externalScriptFiles = FileChangeListener::collectAllScriptFiles(chainToExport);

	PresetHandler::writeValueTreeAsFile(externalScriptFiles, File(directoryPath).getChildFile("externalScripts").getFullPathName());
}

void CompileExporter::writeUserPresetFiles(ModulatorSynthChain * chainToExport, const String &directoryPath)
{
	File presetDirectory = GET_PROJECT_HANDLER(chainToExport).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);

	DirectoryIterator iter(presetDirectory, false, "*", File::findFiles);

	ValueTree userPresets("UserPresets");

	while (iter.next())
	{
		File f = iter.getFile();

		XmlDocument doc(f);

		ScopedPointer<XmlElement> xml = doc.getDocumentElement();

		if (xml != nullptr)
		{
			xml->setAttribute("FileName", f.getFileNameWithoutExtension());

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

void CompileExporter::writePresetFile(ModulatorSynthChain * chainToExport, const String directoryPath, const String &uniqueName)
{
	ValueTree preset = chainToExport->exportAsValueTree();

	PresetHandler::stripViewsFromPreset(preset);

	File presetFile(directoryPath + "/preset");

	ReferenceSearcher::writeValueTreeToFile(preset, presetFile);
}

CompileExporter::ErrorCodes CompileExporter::compileSolution(ModulatorSynthChain *chainToExport, BuildOption buildOption)
{
	BatchFileCreator::createBatchFile(chainToExport, buildOption);

	File batchFile = BatchFileCreator::getBatchFile(chainToExport);
    
#if JUCE_WINDOWS
    
    String command = "\"" + batchFile.getFullPathName() + "\"";
    
#else
    

    
    
    
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



CompileExporter::ErrorCodes CompileExporter::createPluginDataHeaderFile(const String &solutionDirectory, const String &uniqueName, const String &version, const String &publicKey)
{
	String pluginDataHeaderFile;

	if (publicKey.isNotEmpty())
	{
		//pluginDataHeaderFile << "#define PUBLIC_KEY \"" << publicKey << "\"\n";
		pluginDataHeaderFile << "\n";
	}
	pluginDataHeaderFile << "#include \"JuceHeader.h\"" << "\n";
	pluginDataHeaderFile << "#include \"PresetData.h\"\n";
	pluginDataHeaderFile << "\n";
	
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
	
	pluginDataHeaderFile << "AudioProcessor* JUCE_CALLTYPE createPluginFilter() { CREATE_PLUGIN; }\n";
	pluginDataHeaderFile << "\n";
	File pluginDataHeader = File(solutionDirectory).getChildFile("Source/Plugin.cpp");
	
	pluginDataHeader.create();
	pluginDataHeader.replaceWithText(pluginDataHeaderFile);

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

	File resourcesFileObject(solutionDirectory + "/Builds/VisualStudio2013/resources.rc");

	resourcesFileObject.deleteFile();

	resourcesFileObject.create();
	resourcesFileObject.appendText(resourcesFile);

	return ErrorCodes::OK;
}

#define REPLACE_WILDCARD(wildcard, settingId) (templateProject = templateProject.replace(wildcard, SettingWindows::getSettingValue((int)settingId, &GET_PROJECT_HANDLER(chainToExport))))


CompileExporter::ErrorCodes CompileExporter::createIntrojucerFile(ModulatorSynthChain *chainToExport)
{
	MemoryInputStream mis(projectTemplate_jucer, sizeof(projectTemplate_jucer), false);

	String templateProject = String(projectTemplate_jucer);

	REPLACE_WILDCARD("%NAME%", SettingWindows::ProjectSettingWindow::Attributes::Name);
	REPLACE_WILDCARD("%VERSION%", SettingWindows::ProjectSettingWindow::Attributes::Version);
	REPLACE_WILDCARD("%DESCRIPTION%", SettingWindows::ProjectSettingWindow::Attributes::Description);
	REPLACE_WILDCARD("%BUNDLE_ID%", SettingWindows::ProjectSettingWindow::Attributes::BundleIdentifier);
	REPLACE_WILDCARD("%PC%", SettingWindows::ProjectSettingWindow::Attributes::PluginCode);

	REPLACE_WILDCARD("%COMPANY%", SettingWindows::UserSettingWindow::Attributes::Company);
	REPLACE_WILDCARD("%MC%", SettingWindows::UserSettingWindow::Attributes::CompanyCode);
	REPLACE_WILDCARD("%COMPANY_WEBSITE%", SettingWindows::UserSettingWindow::Attributes::CompanyURL);

	REPLACE_WILDCARD("%JUCE_PATH%", SettingWindows::CompilerSettingWindow::Attributes::JucePath);
	REPLACE_WILDCARD("%HISE_PATH%", SettingWindows::CompilerSettingWindow::Attributes::HisePath);
	REPLACE_WILDCARD("%VSTSDK_FOLDER%", SettingWindows::CompilerSettingWindow::Attributes::VSTSDKPath);
	REPLACE_WILDCARD("%USE_IPP%", SettingWindows::CompilerSettingWindow::Attributes::IPPInclude);

    const bool useCopyProtection = GET_PROJECT_HANDLER(chainToExport).getPublicKey().isNotEmpty();
    
    templateProject = templateProject.replace("%USE_COPY_PROTECTION%", useCopyProtection ? "enabled" : "disabled");
    
#if JUCE_MAC
    
    REPLACE_WILDCARD("%IPP_COMPILER_FLAGS%", SettingWindows::CompilerSettingWindow::Attributes::IPPLinker);
    REPLACE_WILDCARD("%IPP_HEADER%", SettingWindows::CompilerSettingWindow::Attributes::IPPInclude);
    REPLACE_WILDCARD("%IPP_LIBRARY%", SettingWindows::CompilerSettingWindow::Attributes::IPPLibrary);
    
#endif
    
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

void CompileExporter::BatchFileCreator::createBatchFile(ModulatorSynthChain *chainToExport, BuildOption buildOption)
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
    
#if JUCE_WINDOWS
    
	const String msbuildPath = "C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\MsBuild.exe";

    const String introjucerPath = File(SettingWindows::getSettingValue((int)SettingWindows::CompilerSettingWindow::Attributes::IntrojucerPath)).getChildFile("The Introjucer.exe").getFullPathName();

    
	ADD_LINE("@echo off");
	ADD_LINE("set project=" << projectName);
	ADD_LINE("");
	ADD_LINE("cd \"" << buildPath << "\"");
	ADD_LINE("\"" << introjucerPath << "\" --resave AutogeneratedProject.jucer");
	ADD_LINE("");

	if (buildOption == VSTx86 || buildOption == VSTx64x86)
	{
		ADD_LINE("echo Compiling 32bit Plugin %project% ...");
		ADD_LINE("set VisualStudioVersion=12.0");
		ADD_LINE("set Platform=Win32");
		ADD_LINE(msbuildPath << " \"Builds\\VisualStudio2013\\%project%.sln\" /p:Configuration=\"Release\" /verbosity:minimal");
		ADD_LINE("");
		ADD_LINE("echo Compiling finished.Cleaning up...");
		ADD_LINE("del \"Compiled\\%project%.exp\"");
		ADD_LINE("del \"Compiled\\%project%.lib\"");
		ADD_LINE("");
	}
	
	if (buildOption == VSTx64 || buildOption == VSTx64x86)
	{
		ADD_LINE("echo Compiling 64bit Plugin %project% ...");
		ADD_LINE("set VisualStudioVersion=12.0");
		ADD_LINE("set Platform=X64");
		ADD_LINE(msbuildPath << " \"Builds\\VisualStudio2013\\%project%.sln\" /p:Configuration=\"Release 64bit\" /verbosity:minimal");
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
    ADD_LINE("echo Compiling VST / AU Plugin " << projectName << " ...");
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
