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




namespace hise
{
using namespace juce;

juce::Array<juce::File> BackendDllManager::getNetworkFiles(MainController* mc, bool includeNoCompilers)
{
	if (!mc->getCurrentFileHandler().getRootFolder().isDirectory())
		return {};

	auto networkDirectory = getSubFolder(mc, FolderSubType::Networks);

	auto files = networkDirectory.findChildFiles(File::findFiles, false, "*.xml");

	for (int i = 0; i < files.size(); i++)
	{
		if (files[i].getFileName().contains("autosave_"))
		{
			files.remove(i--);
			continue;
		}

		if (!includeNoCompilers)
		{
			if (!allowCompilation(files[i]))
				files.remove(i--);
		}
	}

	return files;
}

Array<juce::File> BackendDllManager::getThirdPartyFiles(MainController* mc, bool getSrcDirectory)
{
	auto thirdPartyFolder = getSubFolder(mc, FolderSubType::ThirdParty);

	Array<File> fileList;

	if (getSrcDirectory)
	{
		auto srcFolder = thirdPartyFolder.getChildFile("src");

		if (srcFolder.isDirectory())
			fileList.add(srcFolder);
	}
	else
	{
		auto thirdPartyFiles = thirdPartyFolder.findChildFiles(File::findFilesAndDirectories, false);

		for (auto f : thirdPartyFiles)
		{
			if (f.isHidden())
				continue;

			if (f.getFileExtension() == ".h")
				fileList.add(f);
		}
	}
		
	return fileList;
}

int BackendDllManager::getDllHash(int index)
{
	loadDll(false);

	if (projectDll != nullptr)
	{
		return projectDll->getHash(index);
	}

	return 0;
}

int BackendDllManager::getHashForNetworkFile(MainController* mc, const String& id)
{
	auto fileList = getNetworkFiles(mc, false);

	for (auto f : fileList)
	{
		if (f.getFileNameWithoutExtension() == id)
		{
			if (auto xml = XmlDocument::parse(f))
			{
				auto v = ValueTree::fromXml(*xml);

				zstd::ZDefaultCompressor comp;

				MemoryBlock mb;
				comp.compress(v, mb);

				return mb.toBase64Encoding().hashCode();
			}
		}
	}

	jassertfalse;
	return 0;
}

bool BackendDllManager::unloadDll()
{
	if (auto fh = ProcessorHelpers::getFirstProcessorWithType<scriptnode::DspNetwork::Holder>(getMainController()->getMainSynthChain()))
	{
		fh->setProjectDll(nullptr);
	}

	if (projectDll != nullptr)
	{
		projectDll = nullptr;
		return true;
	}

	return false;
}

bool BackendDllManager::loadDll(bool forceUnload)
{
	if (!getMainController()->getCurrentFileHandler().getRootFolder().isDirectory())
		return false;

	if (forceUnload)
		unloadDll();

	if (projectDll == nullptr)
	{
		auto dllFile = getBestProjectDll(DllType::Current);

		bool ok = false;

		if (dllFile.existsAsFile())
		{
			projectDll = new scriptnode::dll::ProjectDll(dllFile);

			return *projectDll;
		}

		return ok;
	}

	return false;
}

juce::var BackendDllManager::getStatistics()
{
	DynamicObject::Ptr obj = new DynamicObject();

#if JUCE_DEBUG
	auto f = getBestProjectDll(DllType::Debug);
#else
	auto f = getBestProjectDll(DllType::Release);
#endif

	obj->setProperty("File", f.getFileName());
	obj->setProperty("Loaded", projectDll != nullptr);

	if (projectDll != nullptr)
	{
		obj->setProperty("Valid", (bool)(*(projectDll.get())));
		obj->setProperty("InitError", projectDll->getInitError());
		
		Array<var> dllNodes;

		for (int i = 0; i < projectDll->getNumNodes(); i++)
		{
			dllNodes.add(projectDll->getNodeId(i));
		}

		obj->setProperty("Nodes", dllNodes);
	}

	return var(obj.get());
}

bool BackendDllManager::shouldIncludeFaust(MainController* mc)
{
#if !HISE_INCLUDE_FAUST
	return false;
#else
	auto hasFaustFiles = getSubFolder(mc, FolderSubType::CodeLibrary).getChildFile("faust").getNumberOfChildFiles(File::findFiles) != 0;
	
	return hasFaustFiles;
#endif
}

bool BackendDllManager::allowCompilation(const File& networkFile)
{
	if (auto xml = XmlDocument::parse(networkFile))
	{
		return xml->getBoolAttribute(PropertyIds::AllowCompilation);
	}

	return false;
}

bool BackendDllManager::allowPolyphonic(const File& networkFile)
{
	if (auto xml = XmlDocument::parse(networkFile))
	{
		return xml->getBoolAttribute(PropertyIds::AllowPolyphonic);
	}

	return false;
}

juce::ValueTree BackendDllManager::exportAllNetworks(MainController* mc, bool includeCompilers)
{
	ValueTree networks("Networks");
	auto allNetworks = getNetworkFiles(mc);

	auto compilers = getNetworkFiles(mc, false);

	for (auto n : allNetworks)
	{
		if (includeCompilers || !compilers.contains(n))
		{
			if (auto networkXml = XmlDocument::parse(n))
				networks.addChild(ValueTree::fromXml(*networkXml), -1, nullptr);
		}
	}

	return networks;
}

juce::File BackendDllManager::createIfNotDirectory(const File& f)
{
	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

juce::File BackendDllManager::getSubFolder(const MainController* mc, FolderSubType t)
{
	auto f = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::DspNetworks);

	switch (t)
	{
	case FolderSubType::Root:					return createIfNotDirectory(f);
	case FolderSubType::Networks:				return createIfNotDirectory(f.getChildFile("Networks"));
	case FolderSubType::Tests:					return createIfNotDirectory(f.getChildFile("Tests"));
	case FolderSubType::CustomNodes:			return createIfNotDirectory(f.getChildFile("CustomNodes"));
	case FolderSubType::AdditionalCode:			return createIfNotDirectory(f.getChildFile("AdditionalCode"));
	case FolderSubType::CodeLibrary:			return createIfNotDirectory(f.getChildFile("CodeLibrary"));
	case FolderSubType::FaustCode:				return createIfNotDirectory(f.getChildFile("CodeLibrary").getChildFile("faust"));
	case FolderSubType::ThirdParty:				return createIfNotDirectory(f.getChildFile("ThirdParty"));
	case FolderSubType::DllLocation:
#if JUCE_WINDOWS
		return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("dll").getChildFile("Dynamic Library"));
#else
		return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("dll"));
#endif
	case FolderSubType::Binaries:				return createIfNotDirectory(f.getChildFile("Binaries"));
	case FolderSubType::Layouts:				return createIfNotDirectory(f.getChildFile("Layouts"));
	case FolderSubType::ProjucerSourceFolder:	return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("Source"));
	default: jassertfalse; return {};
	}
}

}
