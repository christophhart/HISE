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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

struct VirtualFolder : public soul::patch::VirtualFile,
	public ControlledObject
{
	VirtualFolder(MainController* mc, File parentDirectory, FileHandlerBase::SubDirectories fileType) :
		ControlledObject(mc),
		file(parentDirectory),
		type(fileType)
	{}

	soul::patch::String::Ptr getName() override { return soul::patch::makeString(file.getFullPathName()); }

	soul::patch::String::Ptr getAbsolutePath() override { return soul::patch::makeString(file.getFullPathName()); }

	soul::patch::VirtualFile::Ptr getParent() override
	{
		VirtualFile::Ptr p(new VirtualFolder(getMainController(), file.getParentDirectory(), type));
		return p;
	}

	soul::patch::VirtualFile::Ptr getChildFile(const char* subPath);

	/** Returns the file size, or -1 if unknown. */
	virtual int64_t getSize() override { return -1; }

	int64_t getLastModificationTime() override { return 0; }

	virtual int64_t read(uint64_t startPositionInFile, void* targetBuffer, uint64_t bytesToRead) 
	{
		ignoreUnused(startPositionInFile, targetBuffer, bytesToRead);
		return 0; 
	}

	void addRef() noexcept override { ++refCount; }
	void release() noexcept override { if (--refCount == 0) delete this; }
	std::atomic<int> refCount{ 0 };

private:

	File file;
	FileHandlerBase::SubDirectories type;
};

struct VirtualHiseFile : public soul::patch::VirtualFile,
						 public ControlledObject
{
	VirtualHiseFile(MainController* mc, PoolReference f);

	soul::patch::String::Ptr getName() override;

	/** Returns an absolute path for this file, if such a thing is appropriate. */
	soul::patch::String::Ptr getAbsolutePath() override;

	/** Returns the parent folder of this file, or nullptr if that isn't possible. */
	soul::patch::VirtualFile::Ptr getParent();

	soul::patch::VirtualFile::Ptr getChildFile(const char* subPath);

	/** Returns the file size, or -1 if unknown. */
	virtual int64_t getSize() override;

	int64_t getLastModificationTime() override;

	virtual int64_t read(uint64_t startPositionInFile, void* targetBuffer, uint64_t bytesToRead);

	void addRef() noexcept override { ++refCount; }
	void release() noexcept override { if (--refCount == 0) delete this; }
	std::atomic<int> refCount{ 0 };

	bool isValid() const { return stream != nullptr; }

private:

	
	bool isEmbeddedResource = false;
	ScopedPointer<InputStream> stream;
	PoolReference ref;
};



//==============================================================================
struct SoulLibraryHolder
{
	SoulLibraryHolder() = default;

	void ensureLibraryLoaded(const String& patchLoaderLibraryPath)
	{
		if (library == nullptr)
		{
			library = std::make_unique<soul::patch::SOULPatchLibrary>(patchLoaderLibraryPath.toRawUTF8());

			if (library->loadedSuccessfully())
				loadedPath = patchLoaderLibraryPath;
			else
				library.reset();
		}
		else
		{
			// This class isn't sophisticated enough to be able to load multiple
			// DLLs from different locations at the same time
			jassert(loadedPath == patchLoaderLibraryPath);
		}
	}

	std::unique_ptr<soul::patch::SOULPatchLibrary> library;
	juce::String loadedPath;

	soul::patch::PatchInstance::Ptr createInstance(soul::patch::VirtualFile::Ptr newPatch)
	{
		return library->createPatchFromFileBundle(newPatch);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoulLibraryHolder)
};

}
