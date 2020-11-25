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

namespace scriptnode
{
using namespace juce;
using namespace hise;

VirtualHiseFile::VirtualHiseFile(MainController* mc, PoolReference f) :
	ControlledObject(mc),
	ref(f)
{
	auto type = ref.getFileType();

	FileHandlerBase* handler = mc->getActiveFileHandler();

	if (auto exp = mc->getExpansionHandler().getExpansionForWildcardReference(ref.getReferenceString()))
	{
		handler = exp;
	}

	auto pool = handler->pool->getPoolBase(type);

	if (ref.getMode() == PoolReference::EmbeddedResource)
	{
		stream = pool->getDataProvider()->createInputStream(ref.getReferenceString());
		isEmbeddedResource = true;
	}
	else if (ref.isValid())
	{
		auto file = ref.getFile();

		if(file.existsAsFile())
			stream = new FileInputStream(file);
	}
}

soul::patch::String::Ptr scriptnode::VirtualHiseFile::getName()
{
	if (!isEmbeddedResource)
		return soul::patch::makeString(ref.getFile().getFileName());

	auto s = ref.getReferenceString().fromFirstOccurrenceOf("}", false, false);
	return soul::patch::makeString(s);
}

soul::patch::String::Ptr scriptnode::VirtualHiseFile::getAbsolutePath()
{
	if (isEmbeddedResource)
		return soul::patch::makeString(String());
	else
		return soul::patch::makeString(ref.getFile().getFullPathName());
}

soul::patch::VirtualFile::Ptr scriptnode::VirtualHiseFile::getParent()
{
	if (auto fis = dynamic_cast<FileInputStream*>(stream.get()))
	{
		soul::patch::VirtualFile::Ptr p(new VirtualFolder(getMainController(), fis->getFile().getParentDirectory(), ref.getFileType()));
		return p;
	}

	return nullptr;
}

soul::patch::VirtualFile::Ptr scriptnode::VirtualHiseFile::getChildFile(const char* subPath)
{
	return nullptr;
}

int64_t scriptnode::VirtualHiseFile::getSize()
{
	if (stream != nullptr)
		return stream->getTotalLength();

	return -1;
}

int64_t scriptnode::VirtualHiseFile::getLastModificationTime()
{
	return 0;
}

int64_t scriptnode::VirtualHiseFile::read(uint64_t startPositionInFile, void* targetBuffer, uint64_t bytesToRead)
{
	if (stream != nullptr)
	{
		if (!stream->setPosition(startPositionInFile))
			return -1;

		return stream->read(targetBuffer, bytesToRead);
	}

	return 0;
}


soul::patch::VirtualFile::Ptr VirtualFolder::getChildFile(const char* subPath)
{
	auto c = file.getChildFile(subPath);

	if (c.isDirectory())
	{
		VirtualFile::Ptr p(new VirtualFolder(getMainController(), c, type));
		return p;
	}
	else
	{
		PoolReference ref(getMainController(), c.getFullPathName(), type);
		VirtualFile::Ptr p(new VirtualHiseFile(getMainController(), ref));
		return p;
	}
}

}

