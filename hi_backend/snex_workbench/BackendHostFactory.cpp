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


namespace scriptnode
{
using namespace juce;
using namespace hise;




namespace dll
{

BackendHostFactory::BackendHostFactory(DspNetwork* n, ProjectDll::Ptr dll) :
	NodeFactory(n),
	dllFactory(dll)
{
	auto networks = BackendDllManager::getNetworkFiles(n->getScriptProcessor()->getMainController_());
	auto numNodes = networks.size();

	for (int i = 0; i < numNodes; i++)
	{
		auto f = networks[i];
		NodeFactory::Item item;
		item.id = f.getFileNameWithoutExtension();
		item.cb = [this, i, f](DspNetwork* p, ValueTree v)
		{
			auto nodeId = f.getFileNameWithoutExtension();
			auto networkFile = f;

			if (networkFile.existsAsFile())
			{
				if (auto xml = XmlDocument::parse(networkFile.loadFileAsString()))
				{
					auto nv = ValueTree::fromXml(*xml);

					auto useMod = cppgen::ValueTreeIterator::hasChildNodeWithProperty(nv, PropertyIds::IsPublicMod);

					if (useMod)
						return HostHelpers::initNodeWithNetwork<InterpretedModNode>(p, v, nv, useMod);
					else
						return HostHelpers::initNodeWithNetwork<InterpretedNode>(p, v, nv, useMod);
				}
			}
            
            jassertfalse;
            NodeBase* n = nullptr;
            return n;
		};

		monoNodes.add(item);
	}
}


}
}

namespace hise
{
using namespace juce;

juce::Array<juce::File> BackendDllManager::getNetworkFiles(MainController* mc, bool includeNoCompilers)
{
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
	case FolderSubType::DllLocation:
#if JUCE_WINDOWS
		return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("dll").getChildFile("Dynamic Library"));
#else
		return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("dll"));
#endif
	case FolderSubType::Binaries:				return createIfNotDirectory(f.getChildFile("Binaries"));
	case FolderSubType::Layouts:				return createIfNotDirectory(f.getChildFile("Layouts"));
	case FolderSubType::ProjucerSourceFolder:	return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("Source"));
    default: return {};
	}

	jassertfalse;
	return {};
}

}
