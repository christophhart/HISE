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
	auto thisURL = t.getProperty(MarkdownContentIds::URL).toString();

	if (thisURL == url.toString(MarkdownLink::UrlFull))
	{
		if (url.getType() == MarkdownLink::SVGImage)
		{
			if (auto mb = t.getProperty(MarkdownContentIds::Data).getBinaryData())
			{
				auto xml = XmlDocument::parse(mb->toString());

				if (xml != nullptr)
				{
					auto d = Drawable::createFromSVG(*xml);

					return MarkdownParser::FileBasedImageProvider::createImageFromSvg(d.get(), width);
				}
			}
			else
				return {};
		}
		else
		{
			PNGImageFormat format;

			if (auto mb = t.getProperty(MarkdownContentIds::Data).getBinaryData())
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
	if (t.getProperty(MarkdownContentIds::URL).toString() == url.toString(MarkdownLink::UrlWithoutAnchor))
		return t.getProperty("Content").toString();

	for (auto c : t)
	{
		auto s = findContentRecursive(c, url);

		if (s.isNotEmpty())
			return s;
	}

	return {};
}

MarkdownLink DatabaseCrawler::Resolver::resolveURL(const MarkdownLink& hyperLink)
{
	auto copy = MarkdownLink(hyperLink);
	
	findURLRecursive(data->v, copy);

	return copy;
}

bool DatabaseCrawler::Resolver::findURLRecursive(ValueTree& t, MarkdownLink& hyperLink)
{
	if (t.getProperty(MarkdownContentIds::URL).toString() == hyperLink.toString(MarkdownLink::UrlWithoutAnchor))
	{
		auto type = (MarkdownLink::Type)(int)t.getProperty(MarkdownContentIds::LinkType);
		hyperLink.setType(type);

		return true;
	}

	for (auto c : t)
	{
		if (findURLRecursive(c, hyperLink))
			return true;
	}

	return false;
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
	if (getHolder().shouldAbort())
		return;

	currentLink++;

	if (progressCounter != nullptr && totalLinks > 0)
		*progressCounter = (double)currentLink / (double)totalLinks;

	auto url = MarkdownLink(getHolder().getDatabaseRootDirectory(), v.getProperty(MarkdownContentIds::URL).toString());
	url.setType((MarkdownLink::Type)(int)v.getProperty(MarkdownContentIds::LinkType, 0));

	if (url.hasAnchor())
		return;

	auto f = url.getMarkdownFile(getHolder().getDatabaseRootDirectory());
	
#if 0
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
#endif
	
	auto path = f.getRelativePathFrom(getHolder().getDatabaseRootDirectory());
	v.setProperty(MarkdownContentIds::FilePath, path, nullptr);

	jassert(url.getType() == MarkdownLink::Type::MarkdownFile || url.getType() == MarkdownLink::Folder);

#if 0
	if (!url.fileExists(getHolder().getDatabaseRootDirectory()))
	{
		auto f = url.toFile(MarkdownLink::FileType::ContentFile);

		logMessage("Create file: " + url.toFile(MarkdownLink::FileType::ContentFile).getFullPathName());
		MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(f.getParentDirectory(), f.getFileNameWithoutExtension(), "Autogenerated File");
	}
#endif
		
	v.setProperty(MarkdownContentIds::LinkType, (int)url.getType(), nullptr);

	for (auto r : linkResolvers)
	{
		if (getHolder().shouldAbort())
			return;

		MessageManagerLock lock;
		String content = r->getContent(url);

		if (content.isNotEmpty())
		{
			v.setProperty(MarkdownContentIds::Content, content, nullptr);
			numResolved++;
			break;
		}
	}

	if (!v.hasProperty(MarkdownContentIds::Content))
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

void DatabaseCrawler::createImagesInHtmlFolder(File htmlRoot, MarkdownDatabaseHolder& holder,
	DatabaseCrawler::Logger* nonOwnedLogger, double* progress)
{
	DatabaseCrawler crawler(holder);

	auto contentDirectory = htmlRoot;

	crawler.setLogger(nonOwnedLogger, false);
	crawler.setProgressCounter(progress);
	crawler.loadDataFiles(holder.getCachedDocFolder());
	crawler.writeImagesToSubDirectory(contentDirectory);
}

void DatabaseCrawler::createHtmlFilesInHtmlFolder(File htmlRoot, MarkdownDatabaseHolder& holder,
	DatabaseCrawler::Logger* nonOwnedLogger, double* progress)
{
	DatabaseCrawler crawler(holder);

	auto contentDirectory = htmlRoot;

	crawler.setLogger(nonOwnedLogger, false);
	crawler.setProgressCounter(progress);

	crawler.loadDataFiles(holder.getCachedDocFolder());
	crawler.writeJSONTocFile(htmlRoot);
	crawler.createHtmlFilesInternal(contentDirectory, Markdown2HtmlConverter::LinkMode::LocalFile, htmlRoot.getFullPathName());
}

void DatabaseCrawler::addImagesInternal(ValueTree cTree, float maxWidth)
{
	if (getHolder().shouldAbort())
		return;

	if (progressCounter != nullptr && totalLinks > 0)
		*progressCounter = (double)(currentLink++) / (double)totalLinks;

	auto content = cTree.getProperty(MarkdownContentIds::Content).toString();

	if (content.isNotEmpty())
	{
		
		ScopedPointer<MarkdownRenderer> p = new MarkdownRenderer("");
	
		p->setTargetComponent(getHolder().getRootComponent());
		p->setStyleData(styleData);
		p->setDatabaseHolder(&getHolder());

		for (auto ip : imageProviders)
			p->setImageProvider(ip->clone(p));

		p->setNewText(content);

		auto imageLinks = p->getImageLinks();

		for (auto imgUrl : imageLinks)
		{
			if (getHolder().shouldAbort())
				return;

			auto l = imgUrl.withRoot(getHolder().getDatabaseRootDirectory(), true);
			auto existingChild = imageTree.getChildWithProperty(MarkdownContentIds::URL, l.toString(MarkdownLink::UrlFull));

			if (existingChild.isValid())
				continue;

			Image img;

			{
#if !HISE_HEADLESS
				MessageManagerLock lock;
#endif
				img = p->resolveImage(l, maxWidth);
			}

			if (img.isValid())
			{
				

				logMessage("Writing image " + imgUrl.toString(MarkdownLink::UrlFull));

				
				ValueTree c("Image");
				c.setProperty(MarkdownContentIds::URL, l.toString(MarkdownLink::UrlFull), nullptr);
				
				if (l.getType() == MarkdownLink::Image ||
					l.getType() == MarkdownLink::Icon ||
					l.getType() == MarkdownLink::WebContent)
				{
					juce::PNGImageFormat format;
					MemoryOutputStream output;
					format.writeImageToStream(img, output);
					c.setProperty(MarkdownContentIds::Data, output.getMemoryBlock(), nullptr);
				}
				if (l.getType() == MarkdownLink::SVGImage)
				{
					auto vectorFile = l.toFile(MarkdownLink::FileType::ImageFile);
					jassert(vectorFile.existsAsFile());

					MemoryBlock mb;

					vectorFile.loadFileAsData(mb);
					c.setProperty(MarkdownContentIds::Data, mb, nullptr);
				}

				imageTree.addChild(c, -1, nullptr);
			}
		}

#if !HISE_HEADLESS
		MessageManagerLock mmLock;
#endif
		p = nullptr;
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

	if (!item)
		return;

	if (item.url.hasAnchor())
		return;

	auto type = (MarkdownLink::Type)(int)v.getProperty(MarkdownContentIds::LinkType, (int)MarkdownLink::Type::Invalid);

	auto url = item.url.withRoot(templateDirectory, true);
	url.setType(type);


	auto f = url.toFile(MarkdownLink::FileType::HtmlFile);


	DBG(url.getTypeString() + ": " + f.getFullPathName());


	auto markdownCode = v.getProperty(MarkdownContentIds::Content).toString();

	Markdown2HtmlConverter p(db, markdownCode);

	p.setLinkWithoutAction(item.url);
	p.setDatabaseHolder(&getHolder());

	for (auto lr : linkResolvers)
	{
		p.setLinkResolver(lr->clone(&p));
	}

	try
	{
		p.setLinkMode(linkMode, linkBaseURL);
	}
	catch (String& s)
	{
		logMessage("ERROR: " + f.getFullPathName() + " : " + s);
	}
	

	p.setHeaderFile(templateDirectory.getChildFile("template/header.html"));
	p.setFooterFile(templateDirectory.getChildFile("template/footer.html"));
	p.writeToFile(f, item.url.toString(MarkdownLink::Everything));

	for (auto c : v)
		createHtmlInternal(c);
}

void DatabaseCrawler::createHtmlFilesInternal(File htmlTemplateDirectoy, Markdown2HtmlConverter::LinkMode m, const String& linkBase)
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

void DatabaseCrawler::addPathResolver()
{
	for (auto lr : linkResolvers)
	{
		if (lr->getPriority() == MarkdownParser::ResolveType::EmbeddedPath)
			return;
	}

	addImageProvider(new MarkdownParser::GlobalPathProvider(nullptr));
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

#if !HISE_HEADLESS
	imageTree = {};
	createImageTree();
#endif

	File imageDirectory = htmlDirectory.getChildFile("images");

	templateDirectory = htmlDirectory;

	int numTotal = imageTree.getNumChildren();
	int counter = 0;

	for (auto c : imageTree)
	{
		if (progressCounter != nullptr)
			*progressCounter = (double)counter++ / (double)numTotal;

		MarkdownLink l(templateDirectory, c.getProperty(MarkdownContentIds::URL));

		auto f = l.toFile(MarkdownLink::FileType::ImageFile);

		if (l.getType() == MarkdownLink::SVGImage)
		{
			if (auto mb = c.getProperty(MarkdownContentIds::Data).getBinaryData())
			{
				f.replaceWithData(mb->getData(), mb->getSize());
			}
		}
		else
		{
			PNGImageFormat format;

			if (f.existsAsFile())
				f.deleteFile();

			f.create();
			FileOutputStream fos(f);
			
			if (auto mb = c.getProperty(MarkdownContentIds::Data).getBinaryData())
			{
				auto img = format.loadFrom(mb->getData(), mb->getSize());

				logMessage("Writing image file " + f.getFullPathName());

				format.writeImageToStream(img, fos);
			}

			fos.flush();
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
	metaF.replaceWithText(JSON::toString(var(obj.get())));
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

void DatabaseCrawler::setLogger(Logger* l, bool ownThisLogger)
{
	if (ownThisLogger)
		logger = l;
	else
		nonOwnedLogger = l;
}

void DatabaseCrawler::setStyleData(MarkdownLayout::StyleData& newStyleData)
{
	styleData = newStyleData;
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
