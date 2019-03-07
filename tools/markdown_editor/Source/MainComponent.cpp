/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

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

	root.getChildFile("Content.dat");

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Introduction"), Colours::orange));
	
	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Working with HISE"), Colours::lightblue));

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Project Management"), Colours::grey));

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Scripting"), Colours::lightcyan));

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("HISE Modules"), Colours::lightcoral));

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("UI Components"), Colours::aliceblue));

	database.addItemGenerator(new hise::ScriptingApiDatabase::ItemGenerator(root));

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Glossary"), Colours::honeydew));

	database.addItemGenerator(new MarkdownDataBase::DirectoryItemGenerator(root.getChildFile("Tutorials"), Colours::chocolate));

	database.setRoot(root);
	database.buildDataBase();

	

	database.getHtmlSearchDatabaseDump();

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
	preview.internalComponent.resolvers.add(new ScriptingApiDatabase::Resolver(root));

	preview.internalComponent.resolvers.add(new DatabaseCrawler::Resolver(root));
	preview.internalComponent.providers.add(new DatabaseCrawler::Provider(root, nullptr));

#if 0

	DatabaseCrawler crawler(database);
	
	crawler.addLinkResolver(new ScriptingApiDatabase::Resolver(root.getChildFile("Scripting API")));
	crawler.addImageProvider(new MarkdownParser::URLImageProvider(File::getSpecialLocation(File::tempDirectory).getChildFile("TempImagesForMarkdown"), nullptr));

	crawler.createDataFiles(root);

	//crawler.loadDataFiles(root);

	
	//crawler.writeImagesToSubDirectory(root.getChildFile("html"));
	crawler.createHtmlFiles(root.getChildFile("html"), root.getChildFile("template"), Markdown2HtmlConverter::LinkMode::LocalFile, root.getChildFile("html").getFullPathName());

#endif




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

	//context.attachTo(preview);

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
