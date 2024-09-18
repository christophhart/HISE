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

#pragma once


namespace hise
{
using namespace juce;

struct BackendDllManager : public ReferenceCountedObject,
	public ControlledObject
{
	struct FileCodeProvider : public snex::cppgen::ValueTreeBuilder::CodeProvider,
		public ControlledObject
	{
		FileCodeProvider(const MainController* mc) :
			ControlledObject(const_cast<MainController*>(mc))
		{};

		String getCode(const snex::NamespacedIdentifier& path, const Identifier& classId) const override
		{
			auto codeFolder = getSubFolder(getMainController(), FolderSubType::CodeLibrary);
			auto pathFolder = codeFolder.getChildFile(path.getIdentifier().toString());
			auto f = pathFolder.getChildFile(classId.toString()).withFileExtension("h");
			jassert(f.existsAsFile());
			return f.loadFileAsString();
		}
	};

	enum class DllType
	{
		Debug,
		CI,
		Release,
		Current,
		numDllTypes
	};

	enum class FolderSubType
	{
		Root,
		Networks,
		Tests,
		CustomNodes,
		CodeLibrary,
		AdditionalCode,
		Binaries,
		DllLocation,
		ProjucerSourceFolder,
		Layouts,
		ThirdParty,
		numFolderSubTypes
	};

	BackendDllManager(MainController* mc) :
		ControlledObject(mc)
	{}

	using Ptr = ReferenceCountedObjectPtr<BackendDllManager>;

	static Array<File> getNetworkFiles(MainController* mc, bool includeNoCompilers = true);

	static Array<File> getThirdPartyFiles(MainController* mc, bool getSrcDirectory);

	int getDllHash(int index);

	static int getHashForNetworkFile(MainController* mc, const String& id);

	bool unloadDll();
	bool loadDll(bool forceUnload);

	var getStatistics();

	/** This function will check whether the project that is about to be compiled requires the faust modules to be enabled. */
	static bool shouldIncludeFaust(MainController* mc);

	static bool allowCompilation(const File& networkFile);
	static bool allowPolyphonic(const File& networkFile);

	static ValueTree exportAllNetworks(MainController* mc, bool includeCompilers);

	static File createIfNotDirectory(const File& f);
	static File getSubFolder(const MainController* mc, FolderSubType t);

    static bool hasRNBOFiles(const MainController* mc)
    {
        return getRNBOSourceFolder(mc).isDirectory();   
    }
    
    static File getRNBOSourceFolder(const MainController* mc)
    {
        return getSubFolder(mc, FolderSubType::ThirdParty).getChildFile("src").getChildFile("rnbo");
    }
    
    static void addNodePropertyToJSONFile(const MainController* mc, const String& classId, const Identifier& property)
    {
        auto thirdPartyFolder = getSubFolder(mc, FolderSubType::ThirdParty);
        
        File npFile = thirdPartyFolder.getChildFile("node_properties").withFileExtension("json");

        var np;

        if(npFile.existsAsFile())
            np = JSON::parse(npFile);

        if (np.getDynamicObject() == nullptr)
            np = var(new DynamicObject());

        auto obj = np.getDynamicObject();

        Array<var> propList;
        propList.add(PropertyIds::IsPolyphonic.toString());

        obj->setProperty(Identifier(classId), var(propList));

        npFile.replaceWithText(JSON::toString(np));
    }
    
	static bool matchesDllType(DllType t, const File& f)
	{
		if (t == DllType::Current)
		{
#if JUCE_DEBUG
			t = DllType::Debug;
#elif HISE_CI
			t = DllType::CI;
#else
			t = DllType::Release;
#endif
		}
			
		auto fn = f.getFileNameWithoutExtension();

		if (t == DllType::CI)
			return fn.contains("_ci");
		if (t == DllType::Debug)
			return fn.contains("_debug");
		if (t == DllType::Release)
			return !fn.contains("_debug") && !fn.contains("_ci");

		return false;
	}

	File getBestProjectDll(DllType t) const
	{
		auto dllFolder = getSubFolder(getMainController(), FolderSubType::DllLocation);

#if JUCE_WINDOWS
		String extension = "*.dll";
#elif JUCE_LINUX
		String extension = "*.so";
#else
		String extension = "*.dylib";
#endif

		auto files = dllFolder.findChildFiles(File::findFiles, false, extension);

		for (int i = 0; i < files.size(); i++)
		{
			auto fileName = files[i].getFileName();

			if (!matchesDllType(t, files[i]))
				files.remove(i--);
		}

		if (files.isEmpty())
			return File();

		struct FileDateComparator
		{
			int compareElements(const File& first, const File& second)
			{
				auto t1 = first.getCreationTime();
				auto t2 = second.getCreationTime();

				if (t1 < t2) return 1;
				if (t2 < t1) return -1;
				return 0;
			}
		};

		FileDateComparator c;
		files.sort(c);

		return files.getFirst();
	}

	scriptnode::dll::ProjectDll::Ptr projectDll;
};
}
