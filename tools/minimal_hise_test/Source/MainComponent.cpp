/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "MultiPageDialog.h"
#include "PageFactory.h"



struct CustomResultPage: public MultiPageDialog::PageBase
{
	SN_NODE_ID("Result");

    CustomResultPage(MultiPageDialog& r, int width, const var& obj):
      PageBase(r, width, obj),
      textDoc(doc),
      codeEditor(textDoc)
    {
        addAndMakeVisible(codeEditor);
        codeEditor.setReadOnly(true);
        codeEditor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF222222));
        setSize(width, 400);
    };

    void postInit() override
    {
        auto gs = MultiPageDialog::getGlobalState(*this, {}, var());

        String b;

        b << "const var " << MultiPageDialog::getGlobalState(*this, "id", "bc").toString() << " = Engine.createBroadcaster(";
        b << JSON::toString(gs, false) << ");\n";

	    doc.replaceAllContent(b);
    }

    void resized() override
    {
	    codeEditor.setBounds(getLocalBounds());
    }

    Result checkGlobalState(var globalState) override { return Result::ok(); }

    CodeDocument doc;
    mcl::TextDocument textDoc;
    mcl::TextEditor codeEditor;

};

//==============================================================================
MainComponent::MainComponent():
  Thread("Unit Test thread")
{
	startTimer(150);

    File f("D:\\Development\\test.json");

    var obj;

	auto ok = JSON::parse(f.loadFileAsString(), obj);

    

    auto mp = new MultiPageDialog({});

    mp->setProperty(MultiPageIds::Header, "Broadcaster Builder");

    auto sd = mp->getStyleData();
    sd.backgroundColour = Colours::transparentBlack;
    sd.fontSize = 16.0f;
    mp->setStyleData(sd);

    using namespace PageFactory;

	{
		auto& p1 = mp->addPage<List>();

        p1[MultiPageIds::Padding] = 20;

        p1.addChild<MarkdownText>()[MultiPageIds::Text] = "Please enter the ID of the broadcaster:\n> The name must be a valid HiseScript identifier\nYou can also enter a description and a comma-separated list of tags to be associated with this broadcaster.";
        auto& idInput = p1.addChild<TextInput>();
        idInput[MultiPageIds::ID] = "id";
        idInput[MultiPageIds::Text] = "Broadcaster ID:";
        idInput[MultiPageIds::Required] = true;
        idInput[MultiPageIds::Help] = "The name must be a valid HiseScript identifier.";

        auto& commentInput = p1.addChild<TextInput>();
        commentInput[MultiPageIds::ID] = "comment";
        commentInput[MultiPageIds::Text] = "Description:";

        auto& tagsInput = p1.addChild<TextInput>();
        tagsInput[MultiPageIds::ID] = "tags";
        tagsInput[MultiPageIds::Text] = "Tags:";
        tagsInput.setCustomCheckFunction([](MultiPageDialog::PageBase* b, var obj)
        {
            MultiPageDialog::getGlobalState(*b, b->id, var());
	        auto ti = dynamic_cast<TextInput*>(b);

            auto v = ti->editor.getText();

            auto list = StringArray::fromTokens(v, ",", "");
            list.trim();

            Array<var> items;

            for(auto l: list)
                items.add(l);

            MultiPageDialog::setGlobalState(*b, b->id, var(items));

            return Result::ok();
        });
	}
    

    mp->addPage<CustomResultPage>();

    
    


    /*
    DBG(JSON::toString(mp->defaultLaf.defaultPosition.toJSON()));

    auto sd = MarkdownLayout::StyleData::createDarkStyle();

    DBG(JSON::toString(sd.toDynamicObject()));

    sd.headlineColour = Colours::yellow;
    
    mp->setStyleData(sd);

    auto& p = mp->addPage<PageFactory::MarkdownText>();
    
    p["Text"] = "b labl";
    
    auto& fb = mp->addPage<PageFactory::FileSelector>();
    fb["isDirectory"] = true;
    fb["ID"] = "CURRENT_DIRECTORY";

    auto& second = mp->addPage<PageFactory::List>();
    
    Array<var> data;
    
    auto obj = new DynamicObject();
    obj->setProperty("Type", "Markdown");
    obj->setProperty("Text", "Dudel Dudel");
    
    data.add(obj);
    
    auto obj2 = new DynamicObject();
    obj2->setProperty("Type", "FileSelector");
    obj2->setProperty("ID", "SomeID");
    obj2->setProperty("defaultFile", (int)File::SpecialLocationType::userDesktopDirectory);
    
    data.add(obj2);
    data.add(obj);
    
    second["Children"] = data;
    
    auto& p2 = mp->addPage<PageFactory::MarkdownText>();
    
    p2["Text"] = "# Chapter 1\n> ";
    auto& p3 = mp->addPage<PageFactory::MarkdownText>();
    
    p3["Text"] = "# Chapter 2.\n ";
    p3["headlineColour"] = (int64)0xFFFF0000;

*/

    mp->showFirstPage();
    
    mp->setFinishCallback([](){ JUCEApplication::getInstance()->systemRequestedQuit();
    });
    
    addAndMakeVisible(c = mp);
    
#if JUCE_WINDOWS
    context.attachTo(*this);
    setSize(700, 500);
	//setSize (2560, 1080);
#else
    setSize(700, 600);
#endif
    
    
    
    
}

MainComponent::~MainComponent()
{
#if JUCE_WINDOWS
	context.detach();
#endif
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
}

void MainComponent::resized()
{
    c->setBounds(getLocalBounds());
//    viewer.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
}



