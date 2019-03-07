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


void DatabaseCrawler::Provider::Data::createFromFile(File root)
{
	if (v.isValid())
		return;

	File contentFile = root.getChildFile("Images.dat");

	zstd::ZDefaultCompressor comp;

	comp.expand(contentFile, v);
}

DatabaseCrawler::Provider::Provider(File root_, MarkdownParser* parent) :
	ImageProvider(parent),
	root(root_)
{
	data->createFromFile(root);
}

juce::Image DatabaseCrawler::Provider::findImageRecursive(ValueTree& t, const String& url)
{
	auto thisURL = t.getProperty("URL").toString();

	if (thisURL == url)
	{
		PNGImageFormat format;

		if (auto mb = t.getProperty("Data").getBinaryData())
			return format.loadFrom(mb->getData(), mb->getSize());
		else
			return {};
	}

	for (auto c : t)
	{
		auto s = findImageRecursive(c, url);

		if (s.isValid())
			return s;
	}

	return {};
}

juce::Image DatabaseCrawler::Provider::getImage(const String& url, float width)
{
	return resizeImageToFit(findImageRecursive(data->v, url), width);
}

void DatabaseCrawler::Resolver::Data::createFromFile(File root)
{
	if (v.isValid())
		return;

	File contentFile = root.getChildFile("Content.dat");

	zstd::ZDefaultCompressor comp;

	comp.expand(contentFile, v);
}

DatabaseCrawler::Resolver::Resolver(File root)
{
	data->createFromFile(root);
}

juce::String DatabaseCrawler::Resolver::findContentRecursive(ValueTree& t, const String& url)
{
	if (t.getProperty("URL").toString() == url)
		return t.getProperty("Content").toString();

	for (auto c : t)
	{
		auto s = findContentRecursive(c, url);

		if (s.isNotEmpty())
			return s;
	}

	return {};
}

juce::String DatabaseCrawler::Resolver::getContent(const String& url)
{
	return findContentRecursive(data->v, url);
}

DatabaseCrawler::DatabaseCrawler(MarkdownDataBase& database) :
	db(database)
{
	setLogger(new Logger());

	linkResolvers.add(new MarkdownParser::FolderTocCreator(database.getRoot()));
	linkResolvers.add(new MarkdownParser::FileLinkResolver(database.getRoot()));
}

void DatabaseCrawler::addContentToValueTree(ValueTree& v)
{
	auto url = v.getProperty("URL").toString();

	if ((int)v.getProperty("Type") == MarkdownDataBase::Item::Type::Headline)
		return;

	for (auto r : linkResolvers)
	{
		String content = r->getContent(url);

		if (content.isNotEmpty())
		{
			v.setProperty("Content", content, nullptr);
			numResolved++;
			break;
		}
	}

	if (!v.hasProperty("Content"))
	{
		logMessage("Can't resolve URL " + url);
		numUnresolved++;
	}

	for (auto& c : v)
		addContentToValueTree(c);
}

void DatabaseCrawler::createContentTree()
{
	if (contentTree.isValid())
		return;

	contentTree = db.rootItem.createValueTree();
	addContentToValueTree(contentTree);

	logMessage("Resolved URLs: " + String(numResolved));
	logMessage("unresolved URLs: " + String(numUnresolved));
}

void DatabaseCrawler::addImagesFromContent(float maxWidth /*= 1000.0f*/)
{
	jassert(contentTree.isValid());
	jassert(imageTree.isValid());

	addImagesInternal(contentTree, maxWidth);
}

void DatabaseCrawler::addImagesInternal(ValueTree cTree, float maxWidth)
{
	auto content = cTree.getProperty("Content").toString();

	if (content.isNotEmpty())
	{
		MarkdownParser p(content);

		p.setStyleData(styleData);

		for (auto ip : imageProviders)
			p.setImageProvider(ip->clone(&p));

		p.parse();
		auto imageLinks = p.getImageLinks();

		for (auto imgUrl : imageLinks)
		{
			if (imageTree.getChildWithProperty("URL", imgUrl).isValid())
			{
				continue;
			}

			auto img = p.resolveImage(imgUrl, maxWidth);

			if (img.isValid())
			{
				logMessage("Writing image " + imgUrl);

				juce::PNGImageFormat format;
				MemoryOutputStream output;
				format.writeImageToStream(img, output);

				ValueTree c("Image");
				c.setProperty("URL", imgUrl, nullptr);
				c.setProperty("Data", output.getMemoryBlock(), nullptr);

				imageTree.addChild(c, -1, nullptr);
			}
		}
	}

	for (auto c : cTree)
		addImagesInternal(c, maxWidth);
}

void DatabaseCrawler::createHtmlInternal(File currentRoot, ValueTree v)
{
	MarkdownDataBase::Item item;
	item.loadFromValueTree(v);

	if (item.type == MarkdownDataBase::Item::Headline)
		return;

	if (item.type == MarkdownDataBase::Item::Invalid)
		return;

	auto fileNameWithoutWhitespace = HtmlGenerator::getSanitizedFilename(item.fileName);

	if (fileNameWithoutWhitespace[0] == '/')
		fileNameWithoutWhitespace = fileNameWithoutWhitespace.substring(1);


	File f;

	if (item.type == MarkdownDataBase::Item::Folder)
	{
		
		f = currentRoot.getChildFile(fileNameWithoutWhitespace).getChildFile("index.html");
		logMessage("Create directory index" + f.getFullPathName());
		f.create();
	}
	else if (item.type == MarkdownDataBase::Item::Type::Keyword)
	{
		f = currentRoot.getChildFile(fileNameWithoutWhitespace).withFileExtension(".html");
		f.create();
		logMessage("Create HTML file" + f.getFullPathName());
	}
	else
		jassertfalse;

	auto markdownCode = v.getProperty("Content").toString();

	Markdown2HtmlConverter p(db, markdownCode);
	p.setStyleData(styleData);
	p.setLinkMode(linkMode, linkBaseURL);

	p.setHeaderFile(templateDirectory.getChildFile("header.html"));
	p.setFooterFile(templateDirectory.getChildFile("footer.html"));
	p.writeToFile(f, item.url);

	for (auto c : v)
		createHtmlInternal(currentRoot.getChildFile(fileNameWithoutWhitespace), c);
}

void DatabaseCrawler::createHtmlFiles(File root, File htmlTemplateDirectoy, Markdown2HtmlConverter::LinkMode m, const String& linkBase)
{
	linkMode = m;
	linkBaseURL = linkBase;

	root.deleteFile();
	root.createDirectory();

	templateDirectory = htmlTemplateDirectoy;

	for (auto c : contentTree)
		createHtmlInternal(root, c);
}

void DatabaseCrawler::createImageTree()
{
	if (imageTree.isValid())
		return;

	imageTree = ValueTree("Images");

	addImagesFromContent();
}

void DatabaseCrawler::writeImagesToSubDirectory(File htmlDirectory)
{
	File imageDirectory = htmlDirectory.getChildFile("images");

	for (auto c : imageTree)
	{
		auto fileName = c.getProperty("URL").toString().fromLastOccurrenceOf("/", false, false);

		File f = imageDirectory.getChildFile(fileName).withFileExtension(".png");

		PNGImageFormat format;

		if (auto mb = c.getProperty("Data").getBinaryData())
		{
			auto img = format.loadFrom(mb->getData(), mb->getSize());

			FileOutputStream fos(f);
			f.create();

			logMessage("Writing image file " + f.getFullPathName());

			format.writeImageToStream(img, fos);
		}
	}
}


void DatabaseCrawler::createDataFiles(File root, bool createImages)
{
	createContentTree();

	zstd::ZDefaultCompressor comp;

	File targetF(root.getChildFile("Content.dat"));
	targetF.deleteFile();

	comp.compress(contentTree, targetF);

	if (createImages)
	{
		createImageTree();

		File imageF(root.getChildFile("Images.dat"));
		imageF.deleteFile();
		comp.compress(imageTree, imageF);
	}
}

void DatabaseCrawler::loadDataFiles(File root)
{
	zstd::ZDefaultCompressor comp;

	File targetF(root.getChildFile("Content.dat"));
	File imageF(root.getChildFile("Images.dat"));

	comp.expand(targetF, contentTree);
	comp.expand(imageF, imageTree);

	linkResolvers.add(new Resolver(root));
	imageProviders.add(new Provider(root, nullptr));
}

}