/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "../../../hi_scripting/scripting/api/XmlApi.h"
#include "MainComponent.h"

struct FunkyStuff 
{
	static constexpr char apiWildcard[] = "Scripting API/";

	struct Data
	{
		Data()
		{
			v = ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize);
		}

		~Data()
		{

		}

		ValueTree v;
	};

	

	

	struct ItemGenerator: public hise::MarkdownDataBase::ItemGeneratorBase
	{
		MarkdownDataBase::Item createRootItem(MarkdownDataBase& parent) override
		{
			auto v = data->v;

			auto i = createFromValueTree(data->v);

			return i;
		}

		MarkdownDataBase::Item createFromValueTree(ValueTree& v)
		{
			const static Identifier root("Api");
			const static Identifier method("method");
			

			MarkdownDataBase::Item i;

			i.keywords.add("Scripting API");

			i.c = Colour(0xFFC65638).withMultipliedSaturation(0.8f);

			if (v.getType() == root)
			{
				i.type = hise::MarkdownDataBase::Item::Folder;
				i.fileName = "Scripting API";
				i.tocString = "Scripting API";
				i.description = "The scripting API reference";
				i.url << apiWildcard;
			}
			else if (v.getType() == method)
			{
				auto parent = v.getParent();
				auto className = parent.getType().toString();
				i.type = hise::MarkdownDataBase::Item::Headline;
				i.tocString = v.getProperty("name").toString();
				i.description << "`" << className << "." << i.tocString << "()`  ";
				i.description << v.getProperty("description").toString();
				i.url << apiWildcard << className << "#" << v.getProperty("name").toString().toLowerCase();
			}
			else
			{
				i.type = hise::MarkdownDataBase::Item::Keyword;
				i.description << "API class reference: `" << v.getType().toString() << "`";
				i.tocString = v.getType().toString();
				i.fileName = i.tocString;
				i.url << apiWildcard << v.getType().toString();
			}

			for (auto c : v)
				i.children.add(createFromValueTree(c));

			MarkdownDataBase::Item::Sorter sorter;

			i.children.sort(sorter);

			return i;
		}

		SharedResourcePointer<Data> data;
	};
		
	struct Resolver : public hise::MarkdownParser::LinkResolver
	{
		Resolver(File scriptingDocRoot):
			LinkResolver(),
			docRoot(scriptingDocRoot)
		{}

		Identifier getId() const override
		{
			RETURN_STATIC_IDENTIFIER("FunkyLinkResolver");
		}

		LinkResolver* clone(MarkdownParser* parent) const override
		{
			return new Resolver(docRoot);
		}

		String getContent(const String& url) override
		{
			if (url.startsWith(apiWildcard))
			{
				auto cleaned = url.fromFirstOccurrenceOf(apiWildcard, false, false);

				auto classLink = cleaned.upToFirstOccurrenceOf("#", false, false);
				auto anchor = cleaned.fromFirstOccurrenceOf("#", true, false);

				auto classTree = data->v.getChildWithName(classLink);

				if (classTree.isValid())
				{
					


					String s;

					s << "# API class reference `" << classLink << "`\n";

					auto classDoc = docRoot.getChildFile(classLink + "/README.md");

					if (classDoc.existsAsFile())
					{
						s << classDoc.loadFileAsString();
						s << "  \n";

						s << "# Class methods  \n";
					}

					for (auto c : classTree)
					{
						s << createMethodText(c);
					}

					return s;
				}

				String s;

				s << "# Scripting API";

				return s;
			}

			return {};
		}

		String createMethodText(ValueTree& mv)
		{
			String s;

			String className = mv.getParent().getType().toString();
			String methodName = mv.getProperty("name").toString();

			s << "## `" << methodName << "`\n";

			s << "> " << mv.getProperty("description").toString() << "  \n";

			s << "```javascript\n" << className << "." << methodName << mv.getProperty("arguments").toString() << "```  \n";

			
			

			File additionalDoc = docRoot.getChildFile(className + "/" + methodName + ".md");
			if (additionalDoc.existsAsFile())
			{
				s << additionalDoc.loadFileAsString();
				s << "  \n";
			}

			

			return s;
		}

		SharedResourcePointer<Data> data;

		File docRoot;
	};
};


class DatabaseCrawler
{
public:

	struct Provider : public MarkdownParser::ImageProvider
	{
		struct Data
		{
			void createFromFile(File root)
			{
				if (v.isValid())
					return;

				File contentFile = root.getChildFile("Images.dat");

				zstd::ZDefaultCompressor comp;

				comp.expand(contentFile, v);
			}

			ValueTree v;
		};

		Image findImageRecursive(ValueTree& t, const String& url)
		{
			if (t.getProperty("URL").toString() == url)
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

		Image getImage(const String& url, float width) override
		{
			return resizeImageToFit(findImageRecursive(data->v, url), width);
		}

		Provider(File root_, MarkdownParser* parent) :
			ImageProvider(parent),
			root(root_)
		{
			data->createFromFile(root);
		}

		Identifier getId() const override
		{
			return "DatabaseImageProvider";
		}

		ImageProvider* clone(MarkdownParser* newParent) const override
		{
			return new Provider(root, newParent);
		}

		SharedResourcePointer<Data> data;
		File root;
	};

	struct Resolver: public MarkdownParser::LinkResolver
	{
		struct Data
		{
			void createFromFile(File root)
			{
				if (v.isValid())
					return;

				File contentFile = root.getChildFile("Content.dat");

				zstd::ZDefaultCompressor comp;

				comp.expand(contentFile, v);
			}

			ValueTree v;
		};

		String findContentRecursive(ValueTree& t, const String& url)
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

		LinkResolver* clone(MarkdownParser* parent) const
		{
			return new Resolver(root);
		}

		virtual Identifier getId() const { return "CompressedDatabaseResolver"; };

		String getContent(const String& url) override
		{
			return findContentRecursive(data->v, url);
		}

		Resolver(File root)
		{
			data->createFromFile(root);
		}

		File root;
		SharedResourcePointer<Data> data;
	};

	DatabaseCrawler(const MarkdownDataBase& database) :
		db(database)
	{};

	void addContentToValueTree(ValueTree& v)
	{
		auto url = v.getProperty("URL").toString();

		if (v.getProperty("Type").toString() == "Headline")
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
			DBG("Unresolved: " + url);
			numUnresolved++;
		}
			

		for (auto& c : v)
			addContentToValueTree(c);
	}

	ValueTree createContentTree()
	{
		auto dbTree = db.rootItem.createValueTree();
		addContentToValueTree(dbTree);
	
		DBG("Num Resolved: " + String(numResolved));
		DBG("Num Unresolved: " + String(numUnresolved));

		return dbTree;
	}

	void addImagesFromContent(ValueTree& imageTree, const ValueTree& contentTree, float maxWidth=1000.0f)
	{
		auto content = contentTree.getProperty("Content").toString();

		if (content.isNotEmpty())
		{
			MarkdownParser p(content);

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
					DBG("Writing image " + imgUrl);

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

		for (auto c : contentTree)
			addImagesFromContent(imageTree, c);
	}

	void createInternal(File currentRoot, ValueTree v)
	{
		MarkdownDataBase::Item item;
		item.loadFromValueTree(v);

		if (item.type == MarkdownDataBase::Item::Folder)
		{
			DBG("Create directory " + currentRoot.getChildFile(item.fileName).getFullPathName());
		}
		if (item.type == MarkdownDataBase::Item::Type::Keyword)
		{
			File f = currentRoot.getChildFile(item.fileName).withFileExtension(".html");

			f.create();

			auto markdownCode = v.getProperty("Content").toString();

			MarkdownParser p(markdownCode);
			p.parse();

			f.replaceWithText(p.generateHtml(db, item.url));

			DBG("Create file" + f.getFullPathName());
		}

		for (auto c : v)
			createInternal(currentRoot.getChildFile(item.fileName), c);
	}

	void createHtmlFiles(File root, ValueTree content)
	{
		for (auto c : content)
			createInternal(root, c);
	}

	ValueTree createImageTree(const ValueTree& contentTree)
	{
		ValueTree imageTree("Images");

		addImagesFromContent(imageTree, contentTree);

		return imageTree;
	}

	void writeImagesToSubDirectory(const File& htmlDirectory, ValueTree imageTree)
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

				DBG("Writing image to " + f.getFullPathName());

				format.writeImageToStream(img, fos);
			}

			
			
		}
	}

	int numResolved = 0;
	int numUnresolved = 0;

	OwnedArray<MarkdownParser::LinkResolver> linkResolvers;
	OwnedArray<MarkdownParser::ImageProvider> imageProviders;

	const MarkdownDataBase& db;
};


//==============================================================================
MainContentComponent::MainContentComponent() :
	editor(doc, &tokeniser)
{
	
#if JUCE_WINDOWS
	File root("D:\\docdummy");
#elif JUCE_IOS
    File root = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory);
#else
    File root("/Volumes/Shared/docdummy");
    root.createDirectory();
#endif

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Tutorials"), Colours::orange));
	database.addItemGenerator(new FunkyStuff::ItemGenerator());
	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Reference Guides"), Colours::lightcyan));
	database.setRoot(root);
	database.buildDataBase();
	


	MarkdownLayout::StyleData l;
	l.textColour = Colour(0xFF222222);
	l.headlineColour = Colours::black;
	l.backgroundColour = Colour(0xFFEEEEEE);
	l.linkColour = Colour(0xFF000044);
	l.codeColour = Colour(0xFF333333);
	preview.internalComponent.styleData = l;


	editor.setLookAndFeel(&klaf);
	preview.setLookAndFeel(&klaf);
	preview.setDatabase(&database);

	preview.internalComponent.resolvers.add(new MarkdownParser::FileLinkResolver(database.getRoot()));
	preview.internalComponent.resolvers.add(new MarkdownParser::FolderTocCreator(database.getRoot()));
	preview.internalComponent.resolvers.add(new FunkyStuff::Resolver(root.getChildFile("Scripting API")));

	//preview.internalComponent.resolvers.add(new DatabaseCrawler::Resolver(root));
	//preview.internalComponent.providers.add(new DatabaseCrawler::Provider(root, nullptr));



	DatabaseCrawler crawler(database);
	crawler.linkResolvers.add(new MarkdownParser::FolderTocCreator(database.getRoot()));
	crawler.linkResolvers.add(new MarkdownParser::FileLinkResolver(database.getRoot()));
	crawler.linkResolvers.add(new FunkyStuff::Resolver(root.getChildFile("Scripting API")));
	auto v = crawler.createContentTree();
	//crawler.createHtmlFiles(root.getChildFile("html"), v);

	crawler.imageProviders.add(new MarkdownParser::URLImageProvider(File::getSpecialLocation(File::tempDirectory).getChildFile("TempImagesForMarkdown"), this));

	auto imageTree = crawler.createImageTree(v);

	crawler.writeImagesToSubDirectory(root.getChildFile("html"), imageTree);


#if 0

	auto v = crawler.createContentTree();

	zstd::ZDefaultCompressor comp;

	File targetF(root.getChildFile("Content.dat"));
	File imageF(root.getChildFile("Images.dat"));

	auto img = crawler.createImageTree(v);

	comp.compress(img, imageF);
	comp.compress(v, targetF);
#endif

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	editor.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFAAAAAA));
	editor.setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.2f));
	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
	editor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
	editor.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor.setFont(GLOBAL_MONOSPACE_FONT().withHeight(18.0f));
	

	preview.setNewText(" ", root);

	doc.addListener(this);

	//editor.setVisible(false);

#if JUCE_IOS
    editor.setVisible(false);
    //context.attachTo(*this);
    
    auto b = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
    
    setSize(b.getWidth(), b.getHeight());
    
#else
    setSize (1280, 800);
#endif
}

MainContentComponent::~MainContentComponent()
{
    //context.detach();
    doc.removeListener(this);
}

void MainContentComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(0xFF393939));

    
}

void MainContentComponent::resized()
{
	auto ar = getLocalBounds();

#if !JUCE_IOS
	editor.setBounds(ar.removeFromLeft(getWidth() / 3));
#endif
	preview.setBounds(ar);
}
