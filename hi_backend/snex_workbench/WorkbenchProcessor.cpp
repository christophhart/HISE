/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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



namespace hise {
using namespace juce;



struct IncludeSorter
{
	static int compareElements(const File& f1, const File& f2)
	{
		using namespace scriptnode;
		using namespace snex::cppgen;

		auto xml1 = XmlDocument::parse(f1);
		auto xml2 = XmlDocument::parse(f2);

		if (xml1 != nullptr && xml2 != nullptr)
		{
			ValueTree v1 = ValueTree::fromXml(*xml1);
			ValueTree v2 = ValueTree::fromXml(*xml2);

			auto id1 = "project." + v1.getProperty(PropertyIds::ID).toString();
			auto id2 = "project." + v2.getProperty(PropertyIds::ID).toString();

			auto f1 = [id1](ValueTree& v)
			{
				if (v.getProperty(PropertyIds::FactoryPath).toString() == id1)
					return true;

				return false;
			};

			auto f2 = [id2](ValueTree& v)
			{
				if (v.getProperty(PropertyIds::FactoryPath).toString() == id2)
					return true;

				return false;
			};

			auto firstIsReferencedInSecond = valuetree::Helpers::forEach(v1, f2, valuetree::Helpers::IterationType::ChildrenFirst);

			auto secondIsReferencedInFirst = valuetree::Helpers::forEach(v1, f2, valuetree::Helpers::IterationType::ChildrenFirst);

			String e;
			e << "Cyclic reference: ";
			e << id1 << " && " << id2;

			if (firstIsReferencedInSecond)
			{
				if (secondIsReferencedInFirst)
					throw String(e);

				return -1;
			}

			if (secondIsReferencedInFirst)
			{
				if (firstIsReferencedInSecond)
					throw String(e);

				return 1;
			}
		}

		return 0;
	};
};

DspNetworkCompileExporter::DspNetworkCompileExporter(Component* e, BackendProcessor* bp) :
	DialogWindowWithBackgroundThread("Compile DSP networks"),
	ControlledObject(bp),
	CompileExporter(bp->getMainSynthChain()),
	editor(e)
{
	addComboBox("build", { "Debug", "CI", "Release" }, "Build Configuration");

#if !JUCE_DEBUG
    getComboBoxComponent("build")->setText("Release", dontSendNotification);
#endif
    
	if (getNetwork() == nullptr)
	{
		if (PresetHandler::showYesNoWindow("No DSP Network detected", "You need an active DspNetwork for the compilation process.  \n> Press OK to create a Script FX with an empty embedded Network"))
		{
			raw::Builder builder(bp);
			MainController::ScopedBadBabysitter sb(bp);

			auto jmp = builder.create<JavascriptMasterEffect>(bp->getMainSynthChain(), raw::IDs::Chains::FX);
			jmp->getOrCreate("internal_dsp");
		}
	}

	if (auto n = getNetwork())
		n->createAllNodesOnce();

	auto customProperties = bp->dllManager->getSubFolder(getMainController(), BackendDllManager::FolderSubType::ThirdParty).getChildFile("node_properties.json");

	if (customProperties.existsAsFile())
	{
		auto obj = JSON::parse(customProperties);
		
		if (auto o = obj.getDynamicObject())
		{
			for (auto& nv : o->getProperties())
			{
				if (auto ar = nv.value.getArray())
				{
					for (const auto& prop : *ar)
					{
						cppgen::CustomNodeProperties::addNodeIdManually(nv.name, prop.toString());
					}
				}
			}
		}
	}

	addBasicComponents(true);

	String s;
	s << "Nodes to compile:\n";

	for (auto f : bp->dllManager->getThirdPartyFiles(bp, false))
		s << " - " << f.getFileNameWithoutExtension() << " [external C++]\n";

	for (auto f : bp->dllManager->getNetworkFiles(bp, false))
		s << " - " << f.getFileNameWithoutExtension() << "\n";

	s = s.upToLastOccurrenceOf(", ", false, false);

	addTextBlock(s);

	showStatusMessage("Press OK to compile the nodes into a DLL");
}

void DspNetworkCompileExporter::writeDebugFileAndShowSolution()
{
    auto& settings = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();
    auto hisePath = settings.getSetting(HiseSettings::Compiler::HisePath).toString();
    auto solutionFolder = BackendDllManager::getSubFolder(getMainController(), BackendDllManager::FolderSubType::Binaries).getChildFile("Builds");
    auto projectName = settings.getSetting(HiseSettings::Project::Name).toString();
    auto debugExecutable = File(hisePath).getChildFile("projects/standalone/Builds/");
    
	

	
	
	auto currentExecutable = File::getSpecialLocation(File::currentExecutableFile);


#if JUCE_WINDOWS
    auto isUsingVs2017 = HelperClasses::isUsingVisualStudio2017(settings);
    auto vsString = isUsingVs2017 ? "VisualStudio2017" : "VisualStudio2022";
    auto vsVersion = isUsingVs2017 ? "15.0" : "17.0";
	auto folder = currentExecutable.getFullPathName().contains(" with Faust") ? "Debug with Faust" : "Minimal Build";

	debugExecutable = debugExecutable.getChildFile(vsString).getChildFile("x64").getChildFile(folder).getChildFile("App").getChildFile("HISE Debug.exe");

	// If this hits, then you have a mismatch between VS2022 and VS2017...
	jassertEqual(debugExecutable, currentExecutable);
	
    solutionFolder = solutionFolder.getChildFile(vsString);
    auto solutionFile = solutionFolder.getChildFile(projectName).withFileExtension("sln");
    
	ScopedPointer<XmlElement> xml = new XmlElement("Project");
	xml->setAttribute("ToolsVersion", vsVersion);
	xml->setAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	auto pg = new XmlElement("PropertyGroup");
	pg->setAttribute("Condition", "'$(Configuration)|$(Platform)'=='Debug|x64'");
	xml->addChildElement(pg);

	auto ldc = new XmlElement("LocalDebuggerCommand");

	jassert(debugExecutable.existsAsFile());

	ldc->addTextElement(debugExecutable.getFullPathName()) ;
	
	pg->addChildElement(ldc);
	auto df = new XmlElement("DebuggerFlavor");
	df->addTextElement("WindowsLocalDebugger");
	pg->addChildElement(df);

	auto userFile = solutionFile.getSiblingFile(projectName + "_DynamicLibrary.vcxproj.user");

	auto fileContent = xml->createDocument("");

	userFile.replaceWithText(fileContent);
    
	auto hasThirdPartyFiles = includedThirdPartyFiles.isEmpty();

    if (hasThirdPartyFiles && PresetHandler::showYesNoWindow("Quit HISE", "Do you want to quit HISE and show VS solution for debugging the DLL?  \n> Double click on the solution file, then run the VS debugger and it will open HISE with the ability to set VS breakpoints in your C++ nodes"))
    {
        solutionFile.revealToUser();
        JUCEApplication::quit();
    }
    
#elif JUCE_MAC
    debugExecutable = debugExecutable.getChildFile("MacOSX/build/Debug/HISE Debug.app");
    
    jassert(debugExecutable.isDirectory());
    solutionFolder = solutionFolder.getChildFile("MacOSX");
    auto solutionFile = solutionFolder.getChildFile(projectName).withFileExtension("xcodeproj");
    
    if (PresetHandler::showYesNoWindow("Show XCode Project", "Do you want to show the Xcode Project file?  \n> Double click on the file to open XCode, then choose `Debug->Attach to Process->HISE Debug` in order to run your C++ node in the Xcode Debugger"))
    {
        solutionFile.revealToUser();
    }
#endif
    
	
}

hise::DspNetworkCompileExporter::CppFileLocationType DspNetworkCompileExporter::getLocationType(const File& f) const
{
	if (f.getParentDirectory().getFileNameWithoutExtension() == "src")
		return ThirdPartySourceFile;

	if (f.getFileNameWithoutExtension() == "embedded_audiodata")
		return EmbeddedDataFile;

	for (auto& incF : includedFiles)
	{
		if (incF.getFileNameWithoutExtension() == f.getFileNameWithoutExtension())
			return CompiledNetworkFile;
	}

	for (auto& itf : includedThirdPartyFiles)
	{
		if (itf.getFileNameWithoutExtension() == f.getFileNameWithoutExtension())
			return ThirdPartyFile;
	}

	return UnknownFileType;
}

scriptnode::DspNetwork* DspNetworkCompileExporter::getNetwork()
{
	Processor::Iterator<JavascriptProcessor> iter(getMainController()->getMainSynthChain());

	while (auto jsp = iter.getNextProcessor())
	{
		if (auto n = jsp->getActiveOrDebuggedNetwork())
			return n;
	}
    
    return nullptr;
}



void DspNetworkCompileExporter::run()
{
	auto n = getNetwork();

	if (n == nullptr)
	{
		ok = (ErrorCodes)(int)DspNetworkErrorCodes::NoNetwork;
		errorMessage << "You need at least one active network for the export process.  \n";
		errorMessage << "> This is used to create all nodes once to setup the codegen properties";
		return;
	}

	if (!cppgen::CustomNodeProperties::isInitialised())
	{
		ok = ErrorCodes::CompileError;
		errorMessage << "the node properties are not initialised. Load a DspNetwork at least once";
		return;
	}

	getSourceDirectory(false).deleteRecursively();
	getSourceDirectory(false).createDirectory();
	getSourceDirectory(true).deleteRecursively();
	getSourceDirectory(true).createDirectory();

	

	showStatusMessage("Unload DLL");

	
	getDllManager()->unloadDll();

	showStatusMessage("Create files");

	auto buildFolder = getFolder(BackendDllManager::FolderSubType::Binaries);

	auto networkRoot = getFolder(BackendDllManager::FolderSubType::Networks);
	auto unsortedList = BackendDllManager::getNetworkFiles(getMainController(), false);

	auto unsortedListU = BackendDllManager::getNetworkFiles(getMainController(), true);

	

	for (auto s : unsortedList)
		unsortedListU.removeAllInstancesOf(s);

	showStatusMessage("Sorting include dependencies");

	Array<File> list, ulist;

	for (auto& nf : unsortedList)
	{
		if (nf.getFileName().startsWith("autosave"))
			continue;

		for (auto& sf : getIncludedNetworkFiles(nf))
		{
			if (!BackendDllManager::allowCompilation(sf))
			{
				ok = ErrorCodes::CompileError;
				errorMessage << "Error at compiling `" << nf.getFileNameWithoutExtension() << "`:\n> `" << sf.getFileNameWithoutExtension() << "` can't be included because it's not flagged for compilation\n";
				
				errorMessage << "Enable the `AllowCompilation` flag for " << sf.getFileNameWithoutExtension() << " or remove the node from " << nf.getFileNameWithoutExtension();

				return;
			}
				

			list.addIfNotAlreadyThere(sf);
		}
	}

	for (auto& nf : unsortedListU)
	{
		if (nf.getFileName().startsWith("autosave"))
			continue;

		for (auto& sf : getIncludedNetworkFiles(nf))
		{
			ulist.addIfNotAlreadyThere(sf);
		}
	}

	jassert(list.size() == unsortedList.size());

	auto sourceDir = getFolder(BackendDllManager::FolderSubType::ProjucerSourceFolder);

	

	using namespace snex::cppgen;

	ValueTreeBuilder::SampleList externalSamples;

	// set with all files to generate for all networks
	std::set<String> faustClassIds;

	for (auto e : list)
	{
		if (auto xml = XmlDocument::parse(e))
		{
			auto p = ValueTree::fromXml(*xml);
			auto v = p.getChild(0);

			auto id = v[scriptnode::PropertyIds::ID].toString();

			auto cr = ValueTreeBuilder::cleanValueTreeIds(v);

            if(!cr.wasOk())
            {
                errorMessage = "";
                errorMessage << id << ": " << cr.getErrorMessage();
                ok = ErrorCodes::ProjectXmlInvalid;
                return;
            }
            
			if (id.compareIgnoreCase(e.getFileNameWithoutExtension()) != 0)
			{
				errorMessage << "Error at exporting `" << e.getFileName() << "`: Name mismatch between DSP network file and Root container.  \n>";
				errorMessage << "You need to either rename the file to `" << id;
				errorMessage << ".xml` or edit the XML data and set the root node's ID to `" << e.getFileNameWithoutExtension() << "`.";
				ok = ErrorCodes::ProjectXmlInvalid;
				return;
			}

			if(cppgen::StringHelpers::makeValidCppName(id).compareIgnoreCase(id) != 0)
			{
				errorMessage << "Illegal ID: `" << id << "`  \n> The network ID must be a valid C++ identifier";
				ok = ErrorCodes::ProjectXmlInvalid;
				return;
			}
				
            showStatusMessage("Creating C++ file for Network " + id);
            
			ValueTreeBuilder b(v, ValueTreeBuilder::Format::CppDynamicLibrary);

			b.setCodeProvider(new BackendDllManager::FileCodeProvider(getMainController()));
			b.addAudioFileProvider(new PooledAudioFileDataProvider(getMainController()));

			auto f = sourceDir.getChildFile(id).withFileExtension(".h");

			auto r = b.createCppCode();

			faustClassIds.insert(r.faustClassIds->begin(), r.faustClassIds->end());

			externalSamples.addArray(b.getExternalSampleList());
			

			if (r.r.wasOk())
				f.replaceWithText(r.code);
			else
            {
                ok = ErrorCodes::ProjectXmlInvalid;

				errorMessage = "";
				errorMessage << f.getFileNameWithoutExtension() << ": " << r.r.getErrorMessage();

                
                return;
            };

			includedFiles.add(f);
		}
	}

#if HISE_INCLUDE_FAUST_JIT
	DBG("sourceDir: " + sourceDir.getFullPathName());



	auto codeDestDir = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile("src_");
	auto codeDestDirPath = codeDestDir.getFullPathName().toStdString();

	auto realCodeDestDir = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile("src");
	
	if (!codeDestDir.isDirectory())
		codeDestDir.createDirectory();

	if(!realCodeDestDir.isDirectory())
		realCodeDestDir.createDirectory();

	DBG("codeDestDirPath: " + codeDestDirPath);

	auto boilerplateDestDirPath = codeDestDir.getParentDirectory().getFullPathName().toStdString();
	DBG("boilerplateDestDirPath: " + boilerplateDestDirPath);
	// we either need to hard code this path and keep it consistent with faust_jit_node or hi_backend will have to depend on hi_faust_jit
	auto codeLibDir = getFolder(BackendDllManager::FolderSubType::CodeLibrary).getChildFile("faust");
	auto codeLibDirPath = codeLibDir.getFullPathName().toStdString();
	DBG("codeLibDirPath: " + codeLibDirPath);

	// create all necessary files before thirdPartyFiles
	for (const auto& classId : faustClassIds)
	{
		auto _classId = classId.toStdString();
		DBG("Found Faust classId: " + classId);
		auto faustSourcePath = codeLibDir.getChildFile(classId + ".dsp").getFullPathName().toStdString();

		auto boilerplate_path = scriptnode::faust::faust_jit_helpers::genStaticInstanceBoilerplate(boilerplateDestDirPath, _classId);
		if (boilerplate_path.size() > 0)
			DBG("Wrote boilerplate file to " + boilerplate_path);
		else
			DBG("Writing generated boilerplate failed.");

		std::vector<std::string> faustLibraryPaths = {codeLibDirPath};
		// lookup FaustPath from settings
		auto& settings = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();
        
        auto faustPath = settings.getFaustPath();
        
		if (faustPath.isDirectory()) {
			auto globalFaustLibraryPath = faustPath.getChildFile("share").getChildFile("faust");
            
			if (globalFaustLibraryPath.isDirectory()) {
				faustLibraryPaths.push_back(globalFaustLibraryPath.getFullPathName().toStdString());
			}
		}


		auto code_path = scriptnode::faust::faust_jit_helpers::genStaticInstanceCode(_classId, faustSourcePath, faustLibraryPaths, codeDestDirPath);
		
		auto ok = codeDestDir.getChildFile(code_path).copyFileTo(realCodeDestDir.getChildFile(code_path));

		if (code_path.size() > 0)
			DBG("Wrote code file to " + code_path);
		else
			DBG("Writing generated code failed.");
	}

#endif // HISE_INCLUDE_FAUST_JIT

	auto thirdPartyFiles = BackendDllManager::getThirdPartyFiles(getMainController(), false);

	if (!thirdPartyFiles.isEmpty())
	{
		showStatusMessage("Copying third party files");

		for (auto tpf : thirdPartyFiles)
		{
			includedThirdPartyFiles.insert(0, tpf);
		}
	}

	if (!externalSamples.isEmpty())
	{
        showStatusMessage("Writing embedded audio data file");
        
		auto eadFile = getSourceDirectory(true).getChildFile("embedded_audiodata.h");
		eadFile.deleteFile();

		FileOutputStream fos(eadFile);

		fos << "// Embedded audiodata" << "\n";

        fos << "#pragma once\n\n";
        
		fos << "namespace audiodata {\n";

		for (const auto& es : externalSamples)
		{
			auto& bf = es.data->buffer;

			for (int i = 0; i < bf.getNumChannels(); i++)
			{
				fos << "static const uint32 " << es.className << i << "[] = { \n";
				cppgen::IntegerArray<uint32, float>::writeToStream(fos, reinterpret_cast<uint32*>(bf.getWritePointer(i)), bf.getNumSamples());
				fos << "};\n";
			}
		}

		fos << "}\n";
		fos.flush();

		eadFile.copyFileTo(getSourceDirectory(false).getChildFile("embedded_audiodata.h"));
	}

	for (auto u : unsortedListU)
	{
		if (auto xml = XmlDocument::parse(u))
		{
			auto v = ValueTree::fromXml(*xml);

			zstd::ZDefaultCompressor comp;
			MemoryBlock mb;

			comp.compress(v, mb);

			auto id = v[scriptnode::PropertyIds::ID].toString() + "_networkdata";
			auto f = sourceDir.getChildFile(id).withFileExtension(".h");

			Base c(snex::cppgen::Base::OutputType::AddTabs);

			Namespace n(c, "project", false);

			DefinitionBase b(c, "scriptnode::dll::InterpretedNetworkData");

			Array<DefinitionBase*> baseClasses;
			baseClasses.add(&b);

			Struct s(c, id, baseClasses, {});

			c << "String getId() const override";
			{
				StatementBlock sb(c);
				String def;
				def << "return \"" << v[scriptnode::PropertyIds::ID].toString() << "\";";
				c << def;
			}

			c << "bool isModNode() const override";
			{
				StatementBlock sb(c);
				String def;
				def << "return ";
				auto hasMod = cppgen::ValueTreeIterator::hasChildNodeWithProperty(v, PropertyIds::IsPublicMod);
				def << (hasMod ? "true" : "false") << ";";
				c << def;
			}

			c << "String getNetworkData() const override";
			{
				StatementBlock sb(c);
				String def;
				def << "return \"" << mb.toBase64Encoding() << "\";";
				c << def;
			}

			s.flushIfNot();
			n.flushIfNot();
			
			f.replaceWithText(c.toString());

			includedFiles.add(f);
		}
	}
	
	hisePath = File(GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Compiler::HisePath));

	createProjucerFile();

	for (auto l : getSourceDirectory(true).findChildFiles(File::findFiles, true, "*.h"))
	{
		auto target = getSourceDirectory(false).getChildFile(l.getFileName());

		if (isInterpretedDataFile(target))
			l.moveFileTo(target);
		else
			l.copyFileTo(target);
	}

	createIncludeFile(getSourceDirectory(true));
	createIncludeFile(getSourceDirectory(false));


	try
	{
		createMainCppFile(false);
	}
	catch(Result& r)
	{
		ok = ErrorCodes::CompileError;
		errorMessage << r.getErrorMessage();
		return;		
	}
	

	for (int i = 0; i < includedFiles.size(); i++)
	{
		if (isInterpretedDataFile(includedFiles[i]))
			includedFiles.remove(i--);
	}

	createMainCppFile(true);

    silentMode = true;
    
#if JUCE_WINDOWS
	BuildOption o = CompileExporter::VSTiWindowsx64;
#elif JUCE_MAC
	BuildOption o = CompileExporter::VSTmacOS;
#else
	BuildOption o = CompileExporter::VSTLinux;
#endif

	showStatusMessage("Compiling dll plugin");

	configurationName = getComboBoxComponent("build")->getText();
	
#if JUCE_LINUX
	ok = ErrorCodes::OK;
#else
	ok = compileSolution(o, CompileExporter::TargetTypes::numTargetTypes);
#endif
}

void DspNetworkCompileExporter::threadFinished()
{
#if JUCE_LINUX
	ok = compileSolution(CompileExporter::VSTLinux, CompileExporter::TargetTypes::numTargetTypes);
#endif

	if (ok == ErrorCodes::OK)
	{
		if (isUsingCIMode()) {
			return;
		}

#if JUCE_DEBUG
		writeDebugFileAndShowSolution();
#endif

		globalCommandLineExport = false;

		
#if JUCE_LINUX
		PresetHandler::showMessageWindow("Project creation OK", "Please run the makefile, then restart HISE when the compilation is finished");
#else
		PresetHandler::showMessageWindow("Compilation OK", "Please restart HISE in order to load the new binary");
#endif
		
	}
	else 
		PresetHandler::showMessageWindow("Compilation Error", errorMessage, PresetHandler::IconType::Error);
}

juce::File DspNetworkCompileExporter::getBuildFolder() const
{
	return getFolder(BackendDllManager::FolderSubType::Binaries);
}



juce::Array<juce::File> DspNetworkCompileExporter::getIncludedNetworkFiles(const File& networkFile)
{
	using namespace scriptnode;
	using namespace snex::cppgen;

	Array<File> list;

	if (auto xml = XmlDocument::parse(networkFile))
	{
		ValueTree v = ValueTree::fromXml(*xml);

		auto f2 = [&list, networkFile](ValueTree& v)
		{
			auto p = v.getProperty(PropertyIds::FactoryPath).toString();

			if (p.startsWith("project."))
			{
				auto pId = p.fromFirstOccurrenceOf("project.", false, false);
				list.add(networkFile.getSiblingFile(pId).withFileExtension("xml"));
			}

			return false;
		};
	}

	list.add(networkFile);

	return list;
}

hise::BackendDllManager* DspNetworkCompileExporter::getDllManager()
{
	return dynamic_cast<BackendProcessor*>(getMainController())->dllManager.get();
}

bool DspNetworkCompileExporter::isInterpretedDataFile(const File& f)
{
	return f.getFileNameWithoutExtension().endsWith("_networkdata");
}

void DspNetworkCompileExporter::createIncludeFile(const File& sourceDir)
{
	File includeFile = sourceDir.getChildFile("includes.h");

	cppgen::Base i(cppgen::Base::OutputType::AddTabs);

	i.setHeader([]() { return "/* Autogenerated include file. */"; });

	i << "#if (defined (_WIN32) || defined (_WIN64))";
	i << "#pragma warning( push )";
	i << "#pragma warning( disable : 4189 4373)";  // unused variables, wrong override (from faust classes)
	i << "#else";
    i << "#pragma clang diagnostic push";
    i << "#pragma clang diagnostic ignored \"-Wunused-variable\"";
	i << "#endif";

    i.addEmptyLine();
    

    auto fileList = sourceDir.findChildFiles(File::findFiles, false, "*.h");

	auto thirdPartyFiles = getFolder(BackendDllManager::FolderSubType::ThirdParty).findChildFiles(File::findFiles, false, "*.h");

	fileList.addArray(thirdPartyFiles);

	for (auto& f : fileList)
	{
		if (getLocationType(f) == EmbeddedDataFile)
		{
			i.addComment("Include embedded audio data", cppgen::Base::CommentType::FillTo80Light);
			cppgen::Include m(i, sourceDir, f);
			break;
		}
	}

	bool somethingFound = false;

	for (auto& f : fileList)
	{
		if (getLocationType(f) == ThirdPartyFile)
		{
			if (!somethingFound)
			{
				i.addComment("Include third party header files", cppgen::Base::CommentType::FillTo80Light);
				somethingFound = true;
			}

			cppgen::Base dummyInclude(cppgen::Base::OutputType::NoProcessing);
			{
				dummyInclude.addComment("This just references the real file", cppgen::Base::CommentType::RawWithNewLine);
				cppgen::Include m(dummyInclude, sourceDir, f);
			}
			
			auto fInDir = sourceDir.getChildFile(f.getFileName());
			fInDir.replaceWithText(dummyInclude.toString());
			cppgen::Include m2(i, sourceDir, fInDir);
		}
	}

	if (somethingFound)
		i.addEmptyLine();

	somethingFound = false;

	for (auto& f : fileList)
	{
		if (getLocationType(f) == CompiledNetworkFile)
		{
			if (!somethingFound)
			{
				i.addComment("Include compiled network files", cppgen::Base::CommentType::FillTo80Light);
				somethingFound = true;
			}

			cppgen::Include m(i, sourceDir, f);
		}
	}

    i.addEmptyLine();
    
	i << "#if (defined (_WIN32) || defined (_WIN64))";
	i << "#pragma warning( pop )";
	i << "#else";
    i << "#pragma clang diagnostic pop";
	i << "#endif";
    
	includeFile.replaceWithText(i.toString());
}

void DspNetworkCompileExporter::createProjucerFile()
{
	String templateProject = String(projectDllTemplate_jucer);

	ProjectTemplateHelpers::handleCompilerWarnings(templateProject);
	
	auto& dataObject = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();

	ProjectTemplateHelpers::handleVisualStudioVersion(dataObject, templateProject);

	const File jucePath = hisePath.getChildFile("JUCE/modules");

	auto projectName = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();

    auto dllprefix = cppgen::StringHelpers::makeValidCppName(projectName);
    
	auto dllFolder = getFolder(BackendDllManager::FolderSubType::DllLocation);
	auto dbgFile = dllFolder.getChildFile(dllprefix + "_debug").withFileExtension(".dll");
	auto rlsFile = dllFolder.getChildFile(dllprefix).withFileExtension(".dll");
	auto ciFile = dllFolder.getChildFile(dllprefix + "_ci").withFileExtension(".dll");

	auto dbgName = dbgFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");
	auto rlsName = rlsFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");
	auto ciName =  ciFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");

    
    
#if JUCE_MAC
    REPLACE_WILDCARD_WITH_STRING("%USE_IPP_MAC%", useIpp ? "USE_IPP=1" : String());
    REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", useIpp ? "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a" : String());
    REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", useIpp ? "/opt/intel/ipp/include" : String());
    REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", useIpp ? "/opt/intel/ipp/lib" : String());
#endif

#if JUCE_LINUX
    REPLACE_WILDCARD_WITH_STRING("%USE_IPP_LINUX%", useIpp ? "USE_IPP=1" : "USE_IPP=0");
    REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", useIpp ? "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a" : String());
#endif

	REPLACE_WILDCARD_WITH_STRING("%DEBUG_DLL_NAME%", dbgName);
	REPLACE_WILDCARD_WITH_STRING("%RELEASE_DLL_NAME%", rlsName);
	REPLACE_WILDCARD_WITH_STRING("%CI_DLL_NAME%", ciName);
	REPLACE_WILDCARD_WITH_STRING("%NAME%", projectName);
	REPLACE_WILDCARD_WITH_STRING("%HISE_PATH%", hisePath.getFullPathName());
	REPLACE_WILDCARD_WITH_STRING("%JUCE_PATH%", jucePath.getFullPathName());

	String s = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::ExtraDefinitionsNetworkDll).toString();

	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_LINUX%", s);
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_WIN%", s);
	REPLACE_WILDCARD_WITH_STRING("%EXTRA_DEFINES_OSX%", s);

	auto includeFaust = BackendDllManager::shouldIncludeFaust(getMainController());
	REPLACE_WILDCARD_WITH_STRING("%HISE_INCLUDE_FAUST%", includeFaust ? "enabled" : "disabled");

    
    String headerPath;
    
    if (includeFaust)
    {
        auto faustPath = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject().getFaustPath();
        headerPath = faustPath.getChildFile("include").getFullPathName();
    }
    
    if(BackendDllManager::hasRNBOFiles(getMainController()))
    {
        auto folder = BackendDllManager::getRNBOSourceFolder(getMainController());
        
        headerPath << ";" << folder.getFullPathName();
        headerPath << ";" << folder.getChildFile("common").getFullPathName();
    }
    
    REPLACE_WILDCARD_WITH_STRING("%FAUST_HEADER_PATH%", headerPath);
    
    
    

	auto targetFile = getFolder(BackendDllManager::FolderSubType::Binaries).getChildFile("AutogeneratedProject.jucer");

	targetFile.replaceWithText(templateProject);
}

juce::File DspNetworkCompileExporter::getSourceDirectory(bool isDllMainFile) const
{
	File sourceDirectory;

	if (isDllMainFile)
		return getFolder(BackendDllManager::FolderSubType::ProjucerSourceFolder);
	else
		return GET_PROJECT_HANDLER(getMainController()->getMainSynthChain()).getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile("nodes");
}

void DspNetworkCompileExporter::createMainCppFile(bool isDllMainFile)
{
	File f;
	File sourceDirectory = getSourceDirectory(isDllMainFile);

	if (isDllMainFile)
		f = sourceDirectory.getChildFile("Main.cpp");
	else
		f = sourceDirectory.getChildFile("factory.cpp");

	using namespace cppgen;

	Base b(Base::OutputType::AddTabs);

	b.setHeader([]() { return "/** Autogenerated Main.cpp. */"; });

	b.addEmptyLine();

    b.addComment("Include only the DSP files ", snex::cppgen::Base::CommentType::FillTo80);
	Include(b, "AppConfig.h");
	Include(b, "hi_dsp_library/hi_dsp_library.h");
    Include(b, "hi_faust/hi_faust.h");
    
	Include(b, sourceDirectory, sourceDirectory.getChildFile("includes.h"));

    b.addComment("Now we can add the rest of the codebase", snex::cppgen::Base::CommentType::FillTo80);
    Include(b, "JuceHeader.h");
    
    b.addEmptyLine();

	b << "#if !JUCE_WINDOWS";
    b << "#pragma clang diagnostic push";
    b << "#pragma clang diagnostic ignored \"-Wreturn-type-c-linkage\"";
	b << "#endif";
    
	b.addEmptyLine();

	{
		b.addComment("Project Factory", snex::cppgen::Base::CommentType::FillTo80);

		Namespace n(b, "project", false);

		DefinitionBase bc(b, "scriptnode::dll::StaticLibraryHostFactory");

		Struct s(b, "Factory", { &bc }, {});

		b << "Factory()";

		{
			cppgen::StatementBlock bk(b);

			b << "TempoSyncer::initTempoData();";

			b.addComment("Node registrations", snex::cppgen::Base::CommentType::FillTo80Light);

			for (int i = 0; i < includedThirdPartyFiles.size(); i++)
			{
				String def;

				String nid;

				auto tid = includedThirdPartyFiles[i].getFileNameWithoutExtension();

				nid << "project::" << tid;

				auto illegalPoly = true;

				if(CustomNodeProperties::nodeHasProperty(tid, PropertyIds::AllowPolyphonic))
					illegalPoly = false;
				else
				{
					for(auto nf: includedFiles)
					{
						auto networkFile = getFolder(BackendDllManager::FolderSubType::Networks).getChildFile(nf.getFileNameWithoutExtension()).withFileExtension("xml");

						if(auto xml = XmlDocument::parse(networkFile))
						{
							auto d = xml->createDocument("");
							auto vt = ValueTree::fromXml(*xml);

							auto path = includedThirdPartyFiles[i].getFileNameWithoutExtension();
							auto fp = "project." + path;

							auto found = valuetree::Helpers::forEach(vt, [path, fp](const ValueTree& c)
							{
								if(c[PropertyIds::FactoryPath].toString() == fp)
									return true;

								if(c.getType() == scriptnode::PropertyIds::Property)
								{
									if(c[PropertyIds::ID].toString() == "ClassId")
									{
										return c[scriptnode::PropertyIds::Value].toString() == path;
									}
								}
                                
                                return false;
							});

							if(found)
							{
								auto networkIsPolyphonic = (bool)vt[scriptnode::PropertyIds::AllowPolyphonic];

								auto thisIllegal = !networkIsPolyphonic;

								auto isCppNode = CustomNodeProperties::nodeHasProperty(tid, PropertyIds::IsPolyphonic);



								if(networkIsPolyphonic && isCppNode)
								{
									// Otherwise this branch wouldn't get executed...
									jassert(!CustomNodeProperties::nodeHasProperty(tid, PropertyIds::AllowPolyphonic));
									throw Result::fail("The C++ node `" + nid + "` requires the `AllowPolyphonic` flag in node_properties.json because it is used in the polyphonic network `" + networkFile.getFileName() + "`");
								}

								// allow it being used in several places and set the flag to false
								// as soon as one of them is allowing polyphonic compilation
								illegalPoly &= thisIllegal;
							}
						}
					}
				}

				if(illegalPoly)
				{
					def << "registerPolyNode<" << nid << "<1>, wrap::illegal_poly<" << nid << "<1>>>();";
				}
				else
				{
					def << "registerPolyNode<" << nid << "<1>, " << nid << "<NUM_POLYPHONIC_VOICES>>();";
				}

				
				b << def;
			}

			for (int i = 0; i < includedFiles.size(); i++)
			{
				auto networkFile = getFolder(BackendDllManager::FolderSubType::Networks).getChildFile(includedFiles[i].getFileNameWithoutExtension()).withFileExtension("xml");

				auto isPolyNode = includedFiles[i].loadFileAsString().contains("polyphonic template declaration");
				auto illegalPoly = isPolyNode && !BackendDllManager::allowPolyphonic(networkFile);
				
				String classId = "project::" + includedFiles[i].getFileNameWithoutExtension();

				String def;

				String methodPrefix = "register";

				if (isInterpretedDataFile(includedFiles[i]))
					methodPrefix << "Data";

				if (!isPolyNode)
					def << methodPrefix << "Node<" << classId << ">();";
				else
				{
					def << methodPrefix << "PolyNode<" << classId << "<1>, ";
					
					if (illegalPoly)
						def << "wrap::illegal_poly<" << classId << "<1>>>();";
					else
						def << classId << "<NUM_POLYPHONIC_VOICES>>();";
				}
					

				b << def;
			}
		}
	}

	if (isDllMainFile)
	{
		b << "project::Factory f;";

		b.addEmptyLine();

		b.addComment("Exported DLL functions", snex::cppgen::Base::CommentType::FillTo80);

		{
			b << "DLL_EXPORT int getNumNodes()";
			StatementBlock bk(b);
			b << "return f.getNumNodes();";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT size_t getNodeId(int index, char* t)";
			StatementBlock bk(b);
			b << "return HelperFunctions::writeString(t, f.getId(index).getCharPointer());";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT bool isThirdPartyNode(int index)";
			StatementBlock bk(b);
			String def;
			def << "return index < " << String(includedThirdPartyFiles.size()) << ";";
			b << def;
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT int getNumDataObjects(int nodeIndex, int dataTypeAsInt)";
			StatementBlock bk(b);
			b << "return f.getNumDataObjects(nodeIndex, dataTypeAsInt);";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT void deInitOpaqueNode(scriptnode::OpaqueNode* n)";
			StatementBlock bk(b);
			b << "n->callDestructor();";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT void initOpaqueNode(scriptnode::OpaqueNode* n, int index, bool polyIfPossible)";
			StatementBlock bk(b);
			b << "f.initOpaqueNode(n, index, polyIfPossible);";
		}

		{
			b << "DLL_EXPORT int getHash(int index)";
			StatementBlock bk(b);

			if (includedFiles.size() == 0)
			{
				b << "return 0;";
			}
			else
			{
				String def1;
				def1 << "static const int thirdPartyOffset = " << String(includedThirdPartyFiles.size()) << ";";
				b << def1;

				String def;

				def << "static const int hashIndexes[" << includedFiles.size() << "] =";
				b << def;

				{
					StatementBlock l(b, true);

					for (int i = 0; i < includedFiles.size(); i++)
					{
						if (isInterpretedDataFile(includedFiles[i]))
							break;

						String l;

						auto hash = getDllManager()->getHashForNetworkFile(getMainController(), includedFiles[i].getFileNameWithoutExtension());

						if (hash == 0)
						{
							jassertfalse;
						}

						l << hash;

						if ((i != includedFiles.size() - 1) && !isInterpretedDataFile(includedFiles[i+1]))
							l << ", ";

						b << l;
					}
				}

				b << "return (index >= thirdPartyOffset) ? hashIndexes[index - thirdPartyOffset] : 0;";
			}
		}
		{
			b << "DLL_EXPORT int getWrapperType(int index)";
			StatementBlock bk(b);
			b << "return f.getWrapperType(index);";
		}

		{
			b << "DLL_EXPORT ErrorC getError()";
			StatementBlock bk(b);
			b << "return f.getError();";
		}

		{
			b << "DLL_EXPORT void clearError()";
			StatementBlock bk(b);
			b << "f.clearError();";
		}

		{
			b << "DLL_EXPORT int getDllVersionCounter()";
			StatementBlock bk(b);
			b << "return scriptnode::dll::ProjectDll::DllUpdateCounter;";
		}
	}
	else
	{
		b << "scriptnode::dll::FactoryBase* scriptnode::DspNetwork::createStaticFactory()";
		{
			StatementBlock sb(b);
			b << "return new project::Factory();";
		}
	}

    b.addEmptyLine();
	b << "#if !JUCE_WINDOWS";
    b << "#pragma clang diagnostic pop";
	b << "#endif";
    b.addEmptyLine();
    
	f.replaceWithText(b.toString());
    
    auto rnboSibling = f.getSiblingFile("RNBO.cpp");
    
    if(BackendDllManager::hasRNBOFiles(getMainController()))
    {
        Base r(Base::OutputType::AddTabs);
        
        r.setHeader([]() { return "/** Autogenerated RNBO.cpp. file */"; });

        b.addEmptyLine();

        auto rroot = BackendDllManager::getRNBOSourceFolder(getMainController());
        
        r << "#define RNBO_NO_PATCHERFACTORY 1";
        r << "#define RNBO_USE_FLOAT32 1";
        
        Include(r, sourceDirectory, rroot.getChildFile("RNBO.cpp"));
        
        rnboSibling.replaceWithText(r.toString());
    }
    else
    {
        rnboSibling.replaceWithText("");
    }
}

}
