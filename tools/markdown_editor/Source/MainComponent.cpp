/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "../../../hi_scripting/scripting/api/XmlApi.h"
#include "MainComponent.h"

struct FunkyStuff 
{
	static constexpr char apiWildcard[] = "{SCRIPTING_API}";

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
				i.url << apiWildcard << "/";
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
				i.fileName = i.keywords[0];
				i.tocString = v.getType().toString();
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

				s << "# Works motherfucker";

				return s;
			}
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


//==============================================================================
MainContentComponent::MainContentComponent() :
	editor(doc, &tokeniser)
{
	
	File root("D:\\docdummy");

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

	addAndMakeVisible(editor);
	addAndMakeVisible(preview);

	editor.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFAAAAAA));
	editor.setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.2f));
	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
	editor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
	editor.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor.setFont(GLOBAL_MONOSPACE_FONT().withHeight(18.0f));
	//editor.setVisible(false);

	preview.setNewText(" ", root);

	doc.addListener(this);

    setSize (1280, 800);
}

MainContentComponent::~MainContentComponent()
{
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

	editor.setBounds(ar.removeFromLeft(getWidth() / 2));
	preview.setBounds(ar);
}
