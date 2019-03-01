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

namespace hise {
using namespace juce;


void MarkdownDataBase::buildDataBase()
{
	rootItem = {};
	rootItem.type = Item::Root;
	rootItem.url = "/";

	if (getDatabaseFile().existsAsFile())
	{
		zstd::ZDefaultCompressor compressor;

		ValueTree v;
		auto r = compressor.expand(getDatabaseFile(), v);

		if (r.wasOk())
		{
			loadFromValueTree(v);
			return;
		}
	}

	for (auto g : itemGenerators)
		rootItem.children.add(g->createRootItem(*this));
}


void MarkdownDataBase::DirectoryItemGenerator::addFileRecursive(Item& folder, File f)
{
	if (f.isDirectory())
	{
		folder.c = c;
		folder.type = Item::Folder;
		folder.description = "Folder";
		folder.fileName = f.getFileName();
		folder.keywords.add(folder.fileName);
		folder.tocString = folder.fileName;
		folder.url = "{FOLDER_TOC}" + f.getRelativePathFrom(dbRoot);

		Array<File> childFiles;

		f.findChildFiles(childFiles, File::findFilesAndDirectories, false);

		for (auto c : childFiles)
		{
			if (!c.isDirectory() && !c.hasFileExtension(".md"))
				continue;

			Item newItem;
			addFileRecursive(newItem, c);

			if (newItem.type != Item::Invalid)
				folder.children.add(newItem);
		}
	}
	else
	{
		// Skip README.md files (they will be displayed in the folder item.
		if (f.getFileName().toLowerCase() == "readme.md")
			return;

		MarkdownParser::createDatabaseEntriesForFile(dbRoot, folder, f, c);
	}
}


}