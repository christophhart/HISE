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

	if (contentFile.existsAsFile())
	{
		zstd::ZDefaultCompressor comp;

		comp.expand(contentFile, v);
	}
}

DatabaseCrawler::Provider::Provider(File root_, MarkdownParser* parent) :
	ImageProvider(parent),
	root(root_)
{
	data->createFromFile(root);
}

juce::Image DatabaseCrawler::Provider::findImageRecursive(ValueTree& t, const MarkdownLink& url, float width)
{
	auto thisURL = t.getProperty("URL").toString();

	if (thisURL == url.toString(MarkdownLink::Everything))
	{
		if (url.getType() == MarkdownLink::SVGImage)
		{
			if (auto mb = t.getProperty("Data").getBinaryData())
			{
				ScopedPointer<XmlElement> xml = XmlDocument::parse(mb->toString());

				if (xml != nullptr)
				{
					ScopedPointer<Drawable> d = Drawable::createFromSVG(*xml);

					return MarkdownParser::FileBasedImageProvider::createImageFromSvg(d, width);
				}
			}
			else
				return {};
		}
		else
		{
			PNGImageFormat format;

			if (auto mb = t.getProperty("Data").getBinaryData())
				return format.loadFrom(mb->getData(), mb->getSize());
			else
				return {};
		}
	}

	for (auto c : t)
	{
		auto s = findImageRecursive(c, url, width);

		if (s.isValid())
			return s;
	}

	return {};
}

juce::Image DatabaseCrawler::Provider::getImage(const MarkdownLink& url, float width)
{
	updateWidthFromURL(url, width);

	return resizeImageToFit(findImageRecursive(data->v, url, width), width);
}

void DatabaseCrawler::Resolver::Data::createFromFile(File root)
{
	if (v.isValid())
		return;

	File contentFile = root.getChildFile("Content.dat");

	if (contentFile.existsAsFile())
	{
		zstd::ZDefaultCompressor comp;

		comp.expand(contentFile, v);
	}
}

DatabaseCrawler::Resolver::Resolver(File root_):
	root(root_)
{
	data->createFromFile(root);
}

juce::String DatabaseCrawler::Resolver::findContentRecursive(ValueTree& t, const MarkdownLink& url)
{
	if (t.getProperty("URL").toString() == url.toString(MarkdownLink::UrlWithoutAnchor))
		return t.getProperty("Content").toString();

	for (auto c : t)
	{
		auto s = findContentRecursive(c, url);

		if (s.isNotEmpty())
			return s;
	}

	return {};
}

juce::String DatabaseCrawler::Resolver::getContent(const MarkdownLink& url)
{
	return findContentRecursive(data->v, url);
}

DatabaseCrawler::DatabaseCrawler(MarkdownDatabaseHolder& holder) :
	MarkdownContentProcessor(holder),
	db(holder.getDatabase())
{
	setLogger(new Logger());

	linkResolvers.add(new MarkdownParser::FolderTocCreator(holder.getDatabaseRootDirectory()));
	linkResolvers.add(new MarkdownParser::FileLinkResolver(holder.getDatabaseRootDirectory()));
}

void DatabaseCrawler::addContentToValueTree(ValueTree& v)
{
	currentLink++;

	if (progressCounter != nullptr && totalLinks > 0)
		*progressCounter = (double)currentLink / (double)totalLinks;

	if ((int)v.getProperty("Type") == MarkdownDataBase::Item::Type::Headline)
		return;

	auto url = MarkdownLink(getHolder().getDatabaseRootDirectory(), v.getProperty("URL").toString());

	for (auto r : linkResolvers)
	{
		MessageManagerLock lock;
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
		logMessage("Can't resolve URL " + url.toString(MarkdownLink::Everything));
		numUnresolved++;
	}

	for (auto c : v)
		addContentToValueTree(c);
}

void DatabaseCrawler::createContentTree()
{
	if (contentTree.isValid())
		return;

	totalLinks = db.getFlatList().size();

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
	if (progressCounter != nullptr && totalLinks > 0)
		*progressCounter = (double)(currentLink++) / (double)totalLinks;

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

			MarkdownLink l(getHolder().getDatabaseRootDirectory(), imgUrl);

			Image img;

			{
				MessageManagerLock lock;
				img = p.resolveImage(l, maxWidth);
			}

			if (img.isValid())
			{
				logMessage("Writing image " + imgUrl);

				MarkdownLink l(getHolder().getDatabaseRootDirectory(), imgUrl);

				

				ValueTree c("Image");
				c.setProperty("URL", imgUrl, nullptr);
				

				if (l.getType() == MarkdownLink::Image)
				{
					juce::PNGImageFormat format;
					MemoryOutputStream output;
					format.writeImageToStream(img, output);
					c.setProperty("Data", output.getMemoryBlock(), nullptr);
				}
				if (l.getType() == MarkdownLink::SVGImage)
				{
					auto vectorFile = l.toFile(MarkdownLink::FileType::ImageFile);
					jassert(vectorFile.existsAsFile());

					MemoryBlock mb;

					vectorFile.loadFileAsData(mb);
					c.setProperty("Data", mb, nullptr);
				}

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

	File f;

	if (item.type == MarkdownDataBase::Item::Folder)
	{
		
		f = item.url.toFile(MarkdownLink::FileType::HtmlFile);
		logMessage("Create directory index" + f.getFullPathName());
		f.create();
	}
	else if (item.type == MarkdownDataBase::Item::Type::Keyword)
	{
		f = item.url.toFile(MarkdownLink::FileType::HtmlFile);
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
	p.writeToFile(f, item.url.toString(MarkdownLink::Everything));

	for (auto c : v)
		createHtmlInternal(item.url.getDirectory({}), c);
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

	if (progressCounter != nullptr)
		*progressCounter = 0.0;
	
	currentLink = 0;

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

	File imageF(root.getChildFile("Images.dat"));

	if (createImages)
	{
		createImageTree();
		imageF.deleteFile();
		comp.compress(imageTree, imageF);
	}

	DynamicObject::Ptr obj = new DynamicObject();
	obj->setProperty("content-hash", getHashFromFileContent(targetF));
	obj->setProperty("image-hash", getHashFromFileContent(imageF));

	auto metaF = root.getChildFile("Hash.json");
	metaF.replaceWithText(JSON::toString(var(obj)));
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

juce::int64 DatabaseCrawler::getHashFromFileContent(const File& f) const
{
	MemoryBlock mb;
	f.loadFileAsData(mb);
	return mb.toBase64Encoding().hashCode64();
}

}
