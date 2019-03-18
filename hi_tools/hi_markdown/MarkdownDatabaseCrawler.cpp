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

	File contentFile = root.getChildFile("images.dat");

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

	File contentFile = root.getChildFile("content.dat");

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
	setLogger(new Logger(), true);

	linkResolvers.add(new MarkdownParser::FolderTocCreator(holder.getDatabaseRootDirectory()));
	linkResolvers.add(new MarkdownParser::FileLinkResolver(holder.getDatabaseRootDirectory()));
	addImageProvider(new MarkdownParser::GlobalPathProvider(nullptr));
}

void DatabaseCrawler::addContentToValueTree(ValueTree& v)
{
	currentLink++;

	if (progressCounter != nullptr && totalLinks > 0)
		*progressCounter = (double)currentLink / (double)totalLinks;

	if ((int)v.getProperty("Type") == MarkdownDataBase::Item::Type::Headline)
		return;

	auto url = MarkdownLink(getHolder().getDatabaseRootDirectory(), v.getProperty("URL").toString());

	auto f = url.getMarkdownFile(getHolder().getDatabaseRootDirectory());
	
	if (!f.existsAsFile())
	{
		for (auto lr : linkResolvers)
		{
			auto f_ = lr->getFileToEdit(url);

			if (f_ != File())
			{
				f = f_;
				break;
			}
		}
	}
	
	auto path = f.getRelativePathFrom(getHolder().getDatabaseRootDirectory());
	v.setProperty("FilePath", path, nullptr);

	jassert(url.getType() == MarkdownLink::Type::MarkdownFile || url.getType() == MarkdownLink::Folder);

#if 0
	if (!url.fileExists(getHolder().getDatabaseRootDirectory()))
	{
		auto f = url.toFile(MarkdownLink::FileType::ContentFile);

		logMessage("Create file: " + url.toFile(MarkdownLink::FileType::ContentFile).getFullPathName());
		MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(f.getParentDirectory(), f.getFileNameWithoutExtension(), "Autogenerated File");
	}
#endif
		
	v.setProperty("LinkType", (int)url.getType(), nullptr);

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
			MarkdownLink l(getHolder().getDatabaseRootDirectory(), imgUrl);
			auto existingChild = imageTree.getChildWithProperty("URL", l.toString(MarkdownLink::UrlFull));

			if (existingChild.isValid())
				continue;

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
				c.setProperty("URL", l.toString(MarkdownLink::UrlFull), nullptr);
				
				if (l.getType() == MarkdownLink::Image ||
					l.getType() == MarkdownLink::Icon)
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

void DatabaseCrawler::createHtmlInternal(ValueTree v)
{
	if (progressCounter != nullptr)
		*progressCounter = (double)currentLink++ / (double)totalLinks;


	MarkdownDataBase::Item item;
	item.loadFromValueTree(v);

	if (item.type == MarkdownDataBase::Item::Headline)
		return;

	if (item.type == MarkdownDataBase::Item::Invalid)
		return;

#if 0
	if (item.type == MarkdownDataBase::Item::Folder)
	{
		auto content = v.getProperty("Content").toString();

		Markdown2HtmlConverter converter(db, content);

		converter.generateHtml(item.url.toString(MarkdownLink::UrlFull));

		
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
#endif

	auto fPath = v.getProperty("FilePath").toString();

	auto type = (MarkdownLink::Type)(int)v.getProperty("LinkType", (int)MarkdownLink::Type::Invalid);

	auto url = item.url.withRoot(templateDirectory);
	url.setType(type);

	DBG(url.getHtmlStringForBaseURL(linkBaseURL));

#if 0
	auto fFile = templateDirectory.getChildFile(fPath).withFileExtension(".html");

	if (fFile.getFileNameWithoutExtension().toLowerCase() == "readme")
		fFile = fFile.getSiblingFile("index.html");

	auto f = item.url.toFile(MarkdownLink::FileType::HtmlFile, templateDirectory);

	auto markdownCode = v.getProperty("Content").toString();

	Markdown2HtmlConverter p(db, markdownCode);

	

	

	p.setLinkMode(linkMode, linkBaseURL);

	p.setHeaderFile(templateDirectory.getChildFile("template/header.html"));
	p.setFooterFile(templateDirectory.getChildFile("template/footer.html"));
	p.writeToFile(fFile, item.url.toString(MarkdownLink::Everything));
#endif

	for (auto c : v)
		createHtmlInternal(c);
}

void DatabaseCrawler::createHtmlFiles(File htmlTemplateDirectoy, Markdown2HtmlConverter::LinkMode m, const String& linkBase)
{
	linkMode = m;
	linkBaseURL = linkBase;

	templateDirectory = htmlTemplateDirectoy;

	totalLinks = db.getFlatList().size();
	currentLink = 0;
	logMessage("Create HTML files");

	for (auto c : contentTree)
		createHtmlInternal(c);
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
	styleData = MarkdownLayout::StyleData::createBrightStyle();

	imageTree = {};
	createImageTree();

	File imageDirectory = htmlDirectory.getChildFile("images");

	templateDirectory = htmlDirectory;

	int numTotal = imageTree.getNumChildren();
	int counter = 0;

	for (auto c : imageTree)
	{
		if (progressCounter != nullptr)
			*progressCounter = (double)counter++ / (double)numTotal;

		MarkdownLink l(templateDirectory, c.getProperty("URL"));

		auto f = l.toFile(MarkdownLink::FileType::ImageFile);

		if (l.getType() == MarkdownLink::SVGImage)
		{
			if (auto mb = c.getProperty("Data").getBinaryData())
			{
				f.replaceWithData(mb->getData(), mb->getSize());
			}
		}
		else
		{
			PNGImageFormat format;

			FileOutputStream fos(f);
			f.create();

			if (f.existsAsFile())
				continue;

			if (auto mb = c.getProperty("Data").getBinaryData())
			{
				auto img = format.loadFrom(mb->getData(), mb->getSize());

				logMessage("Writing image file " + f.getFullPathName());

				format.writeImageToStream(img, fos);
			}
		}
	}
}


void DatabaseCrawler::createDataFiles(File root, bool createImages)
{
	createContentTree();

	zstd::ZDefaultCompressor comp;

	File targetF(root.getChildFile("content.dat"));
	targetF.deleteFile();

	comp.compress(contentTree, targetF);

	File imageF(root.getChildFile("images.dat"));

	if (createImages)
	{
		createImageTree();
		imageF.deleteFile();
		comp.compress(imageTree, imageF);
	}

	DynamicObject::Ptr obj = new DynamicObject();
	obj->setProperty("content-hash", getHashFromFileContent(targetF));
	obj->setProperty("image-hash", getHashFromFileContent(imageF));

	auto metaF = root.getChildFile("hash.json");
	metaF.replaceWithText(JSON::toString(var(obj)));
}

void DatabaseCrawler::loadDataFiles(File root)
{
	if (contentTree.isValid() && imageTree.isValid())
		return;

	zstd::ZDefaultCompressor comp;

	File targetF(root.getChildFile("content.dat"));
	File imageF(root.getChildFile("images.dat"));

	comp.expand(targetF, contentTree);
	comp.expand(imageF, imageTree);

	linkResolvers.add(new Resolver(root));
	imageProviders.add(new Provider(root, nullptr));
}

void DatabaseCrawler::writeJSONTocFile(File htmlDirectory)
{
	auto tocVar = getHolder().getDatabase().getJSONObjectForToc();
	auto s = "var rootDb = " + JSON::toString(tocVar) + ";\n";
	auto f = htmlDirectory.getChildFile("template/scripts/toc.json");
	f.create();

	f.replaceWithText(s);

	auto searchVar = getHolder().getDatabase().getHtmlSearchDatabaseDump();
	auto s2 = JSON::toString(searchVar);
	auto f2 = htmlDirectory.getChildFile("template/scripts/search.json");
	f2.create();
	f2.replaceWithText(s2);
}

juce::int64 DatabaseCrawler::getHashFromFileContent(const File& f) const
{
	MemoryBlock mb;
	f.loadFileAsData(mb);
	return mb.toBase64Encoding().hashCode64();
}

juce::String DatabaseCrawler::getContentFromCachedTree(const MarkdownLink& l)
{
	jassert(!linkResolvers.isEmpty());
	return this->linkResolvers.getLast()->getContent(l);
}

}
