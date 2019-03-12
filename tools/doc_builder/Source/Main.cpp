/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"


REGISTER_STATIC_DSP_LIBRARIES()
{
	REGISTER_STATIC_DSP_FACTORY(HiseCoreDspFactory);

#if ENABLE_JUCE_DSP
	REGISTER_STATIC_DSP_FACTORY(JuceDspModuleFactory);
#endif
}

AudioProcessor* hise::StandaloneProcessor::createProcessor()
{
	return new hise::BackendProcessor(deviceManager, callback);
}



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
	l << "cache:   Creates the cached .dat files for the inbuild help.";
	l << "html:    Creates the HTML files.";
	l << "clean:   Deletes the cached files and the html directory.";
	l << "full:    cache + html in one go.";
	l << "";
	l << "# Arguments:";
	l << "------------";
	l << "";
	l << "-skip_image:  Skip the creation of images.";
	l << "-use_cache:   Use the cached files instead of the raw markdown files";
	l << "-local_links: Create the links that work for a local copy of the HTML doc";
	l << "              Use this option when building your offline documentation.";
	l << "-web:         Create links that work for the online copy. This is hardwired to";
	l << "              http://docs.hise.audio";
	l << "";
	l << "--------------------------------------------------------------------------------";
}

int createHtml(const StringArray& args)
{
#if 0
	BackendProcessor bp(nullptr, nullptr);
	//BackendRootWindow b(&bp, {});
	b.


	StandardLogger l;

	if (args.size() < 4)
	{
		l << "ERROR: You have to specify a markdown directory and a HTML output directory;";
		return 1;
	}

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

		if (File::isAbsolutePath(fileName2))
		{
			File htmlDir(fileName2);

			htmlDir.deleteRecursively();
			
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

			bp.getDatabase().getDatabaseFile().deleteFile();

			hise::DatabaseCrawler crawler(bp);

			crawler.setLogger(new StandardLogger());

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

			return 0;
		}
		else
		{
			l << "ERROR: the HTML path is not a valid path";
			return 1;
		}

		
	}
#endif
	return 1;
}

int createCache(const StringArray& args)
{
	BackendProcessor bp;
	
	StandardLogger l;

	if (args.size() < 3)
	{
		l << "ERROR: You have to specify a markdown directory;";
		return 1;
	}

	String fileName = args[2];

	bool skipImages = args.contains("-skip_image");

	if (File::isAbsolutePath(fileName))
	{
		File root(fileName);

		if (!root.isDirectory())
		{
			l << "ERROR: The path you supplied is not a directory";
		}

		hise::DatabaseCrawler crawler(bp);

		bp.setDatabaseRootDirectory(root);
		bp.addContentProcessor(&crawler);

		bp.rebuildDatabase();

		crawler.setLogger(new StandardLogger());
		crawler.createDataFiles(root, !skipImages);

		return 0;
	}
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

	if (action == "cache")
	{
		return createCache(args);
	}

	if (action == "html")
	{
		return createHtml(args);
	}
	

    return 0;
}
