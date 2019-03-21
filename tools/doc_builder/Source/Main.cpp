/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"


struct StandardLogger : public hise::DatabaseCrawler::Logger
{
	virtual void logMessage(const String& m)
	{
		DBG(m);
		std::cout << m << "\n";
	}

	String operator<<(const String& m)
	{
		logMessage(m);
		return m;
	}
};


void printHelp()
{
	StandardLogger l;
	NewLine nl;



	l << R"( _   _  _  ____   _____      _              )";
	l << R"(| | | || |/ ___| | ____|  __| |  ___    ___ )";
	l << R"(| |_| || | \__ \ |  _|   / _` | / _ \  / __|)";
	l << R"(|  _  || | ___) || |___ | (_| || (_) || (__ )";
	l << R"(|_| |_||_||____/ |_____| \__,_| \___/  \___|)";
	l << "";
	l << "HISE doc builder help v1.0.0";
	l << "--------------------------------------------------------------------------------";
	l << "Creates the HISE documentation from the GitHub repository.";
	l << "--------------------------------------------------------------------------------";
	l << "";
	l << "# Syntax:";
	l << "---------";
	l << "";
	l << "doc_builder [command] [input] [output] [args] ";
	l << "";
	l << "command: the action you want to do (see below).";
	l << "input:   the path to the markdown documentation folder. You have to create this";
	l << "         directory before running this application.";
	l << "output:  the path for the html root directory. It will delete the content or";
	l << "         create a new directory at the given location.";
	l << "args:    additional arguments (see below)";
	l << "";
	l << "# Commands:";
	l << "-----------";
	l << "";
	l << "help:    Show this help.";
	l << "html:    Creates the HTML files.";
	l << "clean:   Deletes the cached files and the html directory.";
	l << "";
	l << "# Arguments:";
	l << "------------";
	l << "";
	l << "-skip_image:  Skip the creation of images.";
	l << "-skip-if-old-hash: Skips the building if the hash file in the app's directory";
	l << "                   is the same as the hash in the cache folder.";
	l << "";
	l << "--------------------------------------------------------------------------------";
}

class CachedHolder : public MarkdownDatabaseHolder
{
public:

	CachedHolder(File cacheRoot_) :
		cacheRoot(cacheRoot_)
	{};

	virtual void registerItemGenerators()
	{
		
	}

	virtual void registerContentProcessor(MarkdownContentProcessor* processor)
	{
		processor->addLinkResolver(new DatabaseCrawler::Resolver(getCachedDocFolder()));
		processor->addImageProvider(new DatabaseCrawler::Provider(getCachedDocFolder(), nullptr));

		//registerGlobalPathFactory(c, {});
	}

	virtual File getCachedDocFolder() const
	{
		return cacheRoot;
	}

	virtual File getDatabaseRootDirectory() const
	{
		return cacheRoot;
	}

	File cacheRoot;
};

int createHtml(const StringArray& args)
{
	StandardLogger l;

	if (args.size() < 4)
	{
		l << "ERROR: You have to specify a markdown directory and a HTML output directory;";
		return 1;
	}

	l << "Building new HTML docs";
	l << Time::getCurrentTime().toString(true, true, true, true);

	String fileName = args[2];
	String fileName2 = args[3];

	

	bool skipImages = args.contains("-skip_image");

	if (File::isAbsolutePath(fileName))
	{
		File root(fileName);

		if (!root.isDirectory())
		{
			l << "ERROR: The path you supplied is not a directory";
			return 1;
		}

		bool skipHash = args.contains("skip-if-old-hash");

		bool skipContent = false;
		

		if (skipHash)
		{
			auto executableRoot = File::getSpecialLocation(File::invokedExecutableFile).getParentDirectory();

			auto oldHash = executableRoot.getChildFile("hash.json").loadFileAsString();
			

			auto oh = JSON::parse(oldHash);

			if (oh.getDynamicObject() != nullptr)
			{
				auto newHash = root.getChildFile("hash.json").loadFileAsString();

				auto nh = JSON::parse(newHash);

				if (nh.getDynamicObject() == nullptr)
				{
					l << "ERROR: Can't load hash.json from cached folder";
				}

				skipContent = oh.getProperty("content-hash", 9) == nh.getProperty("content-hash", 12);
				skipImages |= oh.getProperty("image-hash", 5) == nh.getProperty("image-hash", 41);
			}   
		}

		CachedHolder holder(root);
		holder.setForceCachedDataUse(true, true);
		holder.rebuildDatabase();

		if (File::isAbsolutePath(fileName2))
		{
			File htmlDir(fileName2);

			//htmlDir.deleteRecursively();
			
#if 0
			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Introduction"), Colours::orange));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Working with HISE"), Colours::lightblue));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Project Management"), Colours::grey));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Scripting"), Colours::lightcyan));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("HISE Modules"), Colours::lightcoral));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("UI Components"), Colours::aliceblue));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Glossary"), Colours::honeydew));

			database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Tutorials"), Colours::chocolate));

			database.addItemGenerator(new ScriptingApiDatabase::ItemGenerator());

			
#endif

			ScopedPointer<DatabaseCrawler::Logger> logger = new StandardLogger();

			if (skipImages)
				l << "Skipping image file creation";
			else
			{
				l << "Creating image files";
				hise::DatabaseCrawler::createImagesInHtmlFolder(htmlDir, holder, logger, nullptr);
			}
				

			if (skipContent)
				l << "Skipping HTML files because hash didn't change.";
			else
			{
				l << "Creating HTML files";
				hise::DatabaseCrawler::createHtmlFilesInHtmlFolder(htmlDir, holder, logger, nullptr);
			}

#if 0
			hise::DatabaseCrawler crawler(holder);

			crawler.setLogger(new StandardLogger(), true);

			MarkdownLayout::StyleData l;
			l.textColour = Colour(0xFF333333);
			l.headlineColour = Colour(0xFF444444);
			l.backgroundColour = Colour(0xFFEEEEEE);
			l.linkColour = Colour(0xFF000044);
			l.codeColour = Colour(0xFF333333);

			crawler.setStyleData(l);

			crawler.createContentTree();
			crawler.createImageTree();

			crawler.createDataFiles(root, true);

			crawler.writeImagesToSubDirectory(htmlDir);

			//crawler.createHtmlFiles(htmlDir, root.getChildFile("template"), Markdown2HtmlConverter::LinkMode::LocalFile, htmlDir.getFullPathName());
#endif

			return 0;
		}
		else
		{
			l << "ERROR: the HTML path is not a valid path";
			return 1;
		}
	}

	return 1;
}


//==============================================================================
int main (int argc, char* argv[])
{
	StringArray args;

	StandardLogger logger;

	for (int i = 0; i < argc; i++)
	{
		args.add(String(argv[i]).trim());

		logger << args[i];
	}
    
	String action = args[1];

	if(action == "help" || args.size() == 1)
	{
		printHelp();
		return 0;
	}

	if (action == "html")
	{
		return createHtml(args);
	}

    return 0;
}

