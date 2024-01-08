/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "MultiPageDialog.h"
#include "PageFactory.h"

struct ColourChooser: public PageFactory::LabelledComponent
{
    SN_NODE_ID("ColourChooser");
    
    ColourChooser(MultiPageDialog& r, int w, const var& obj):
      LabelledComponent(r, w, obj, new ColourSelector(ColourSelector::ColourSelectorOptions::showColourspace | ColourSelector::showColourAtTop, 2, 0))
    {
        auto& selector = getComponent<ColourSelector>();
        selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
        selector.setLookAndFeel(&laf);

        setSize(w, 130);
    }
    
    void postInit() override
    {
        auto& selector = getComponent<ColourSelector>();
        auto colour = (uint32)(int64)getValueFromGlobalState();
        selector.setCurrentColour(Colour(colour));
    };
    
    Result checkGlobalState(var globalState) override
    {
        auto& selector = getComponent<ColourSelector>();
        MultiPageDialog::setGlobalState(*this, id, (int64)selector.getCurrentColour().getARGB());
        
        return Result::ok();
    }
    
    LookAndFeel_V4 laf;
};

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

void MainComponent::build()
{
    var obj;

    //auto ok = JSON::parse(f.loadFileAsString(), obj);
    
    auto mp = new MultiPageDialog({}, rt);

    mp->setProperty(MultiPageIds::Header, "Broadcaster Builder");

    auto sd = mp->getStyleData();
    sd.backgroundColour = Colours::transparentBlack;
    sd.fontSize = 16.0f;
    mp->setStyleData(sd);

    mp->defaultLaf.defaultPosition.OuterPadding = 50;
    
    using namespace PageFactory;

    {
        auto& p1 = mp->addPage<List>();

        MultiPageDialog::setGlobalState(*mp, "Colour", 0xFF00FF09);
        
        p1[MultiPageIds::Padding] = 30;

        p1.addChild<MarkdownText>()[MultiPageIds::Text] = "This wizard will take you through the steps of creating a broadcaster. You can specify every property and connection and it will create a script definition at the end of the process that you then can paste into your `onInit` callback.";
        auto& idInput = p1.addChild<TextInput>();
        idInput[MultiPageIds::ID] = "id";
        idInput[MultiPageIds::Text] = "Broadcaster ID*:";
        idInput[MultiPageIds::Required] = true;
        idInput[MultiPageIds::Help] = "The ID will be used to create the script variable definition as well as act as a unique ID for every broadcaster of a single script processor. The name must be a valid HiseScript identifier.";

        auto& additional = p1.addChild<List>({
            { MultiPageIds::Foldable, true },
            { MultiPageIds::Folded, true },
            { MultiPageIds::Text, "Additional Properties" },
            { MultiPageIds::Padding, 20 }
        });
                                            
        
        
        
        auto& commentInput = additional.addChild<TextInput>();
        commentInput[MultiPageIds::ID] = "comment";
        commentInput[MultiPageIds::Text] = "Comment:";

        auto& tagsInput = additional.addChild<TextInput>();
        tagsInput[MultiPageIds::ID] = "tags";
        tagsInput[MultiPageIds::Help] = "Enter a comma separated list of strings that will be used as tags for the broadcaster. This lets you filter which broadcaster you want to show on the broadcaster map and is useful for navigating complex projects";
        tagsInput[MultiPageIds::Text] = "Tags:";
        
        additional.addChild<ColourChooser>({
            { MultiPageIds::Text, "Colour" },
            { MultiPageIds::Help, "The colour that will be used on the broadcaster map" },
            { MultiPageIds::ID, "Colour" }
        });
    }
    
    using namespace PageFactory;
    
    {
        auto& l3 = mp->addPage<List>({
            { MultiPageIds::Padding, 10 }
        });

        auto& c2 = l3.addChild<PageFactory::Column>({
            { MultiPageIds::Padding, 20}
        });
        c2.addChild<MarkdownText>({
            { MultiPageIds::Text, "### Event Source Type\nPlease select the event type that you want to attach the broadcaster to. There are multiple event sources which can trigger a broadcaster message.\n> You can specify the exact source in the next step."},
            { MultiPageIds::Width, -0.6 }
        });
        
        auto& p2 = c2.addChild<List>({
            {MultiPageIds::Width, -0.4},
            {MultiPageIds::Padding, 5},
        });
        
        auto& p2_ = mp->addPage<List>({
            { MultiPageIds::Padding, 20 }
        });
        
        p2_.addChild<MarkdownText>()[MultiPageIds::Text] = "The current data is downloaded. Please wait...";
        p2_.addChild<DummyWait>({
            { MultiPageIds::Text, "Download installer" },
            { MultiPageIds::ID, "downloadInstaller" }
        });
        p2_.addChild<DummyWait>({
            { MultiPageIds::Text, "Verify installer" },
            { MultiPageIds::ID, "downloadInstaller2" }
        });
        p2_.addChild<DummyWait>({
            { MultiPageIds::Text, "Extract installer" },
            { MultiPageIds::ID, "downloadInstaller3" }
        });
        
        //p2_.addChild<Skip>()[MultiPageIds::CallType] = "BackgroundThread";
        
        enum class SourceIndex
        {
            None,
            ComplexData,
            ComponentProperties,
            ComponentVisibility,
            ContextMenu,
            EqEvents,
            ModuleParameters,
            MouseEvents,
            ProcessingSpecs,
            RadioGroup,
            RoutingMatrix,
            numSourceIndexTypes
        };
        
        StringArray options(
        {
             "None::No event source. Use this option if you want to call the broadcaster manually or attach it to any other script callback slot (eg. TransportHandler callbacks).",
             "Complex Data::An event of a complex data object (Tables, Slider Packs or AudioFiles). This can be either:\n- content changes (eg. when loading in a new sample)\n- a display index change (eg. if the table ruler is moved)",
             "Component Properties::Script properties of a UI component selection (eg. the `visible` property).",
             "Component Visibility::The visibility of a UI component.\n>This also takes into account the visibility of parent components so it's a more reliable way than listen to the component's `visible` property.",
             "Context Menu::Adds a popup menu when the UI component is clicked",
             "EQ Events::Listens to band add / delete, reset events of a parametriq EQ",
             "Module Parameter::Listens to changes of a module attribute (when calling `setAttribute()`, eg. the **Reverb Width** or **Filter Frequency**",
             "Mouse Events::Mouse events for a UI component selection",
             "Processing Specs::Listens to changes of the processing specifications (eg. sample rate of audio buffer size)",
             "Radio Group::Listens to button clicks within a given radio group ID\n> This is especially useful for implementing your page switch logic",
             "Routing Matrix::Listens to changes of the routing matrix (the channel routing configuration) of a module"
        });
        
        for(auto sa: options)
        {
            p2.addChild<Tickbox>({
                { MultiPageIds::ID, "Source" },
                { MultiPageIds::Text, sa.upToFirstOccurrenceOf("::", false, false) },
                { MultiPageIds::Help, sa.fromFirstOccurrenceOf("::", false, false) },
                { MultiPageIds::Required, true }
            });
        }
        
        MultiPageDialog::setGlobalState(*mp, "Source", -1);
        MultiPageDialog::setGlobalState(*mp, "complexDataType", "SliderPack");
        MultiPageDialog::setGlobalState(*mp, "complexEventType", "Content");
        MultiPageDialog::setGlobalState(*mp, "complexSlotIndex", 0);
        
        auto& eventPages = mp->addPage<Branch>({
            { MultiPageIds::ID, "Source" }
        });
        
        SourceIndex sourceIndex = (SourceIndex)0;
        
        for(auto sa: options)
        {
            String header;
            header << "### " << sa.upToFirstOccurrenceOf("::", false, false);
            
            auto& ep = eventPages.addChild<List>({
                {MultiPageIds::Padding, 20 }
            });
            
            ep.addChild<MarkdownText>({
                {MultiPageIds::Text, header }
            });
            
            
            
            switch(sourceIndex)
            {
                case SourceIndex::None:
                {
                    auto& a = ep.addChild<Skip>();
                
                    break;
                }
                case SourceIndex::ComplexData:
                {
                    ep.addChild<MarkdownText>()[MultiPageIds::Text] = "Attaching a broadcaster to a complex data object lets you listen to table edit changes or playback position updates of one or multiple data sources. Please fill in the information below to proceed to the next step.";
                    
                    ep.addChild<Choice>({
                        { MultiPageIds::Items, "Table\nSliderPack\nAudioFile" },
                        { MultiPageIds::ID, "complexDataType" },
                        { MultiPageIds::Text, "Data Type" },
                        { MultiPageIds::Help, "The data type that you want to listen to" }
                    });
                    
                    ep.addChild<Choice>({
                        { MultiPageIds::Items, "Content\nDisplayIndex" },
                        { MultiPageIds::ID, "complexEventType" },
                        { MultiPageIds::Text, "Event Type" },
                        { MultiPageIds::Help, "The event type you want to listen to.\n-**Content** events will be triggered whenever the data changes (so eg. loading a new sample or editing a table will trigger this event).\n-**DisplayIndex** events will occur whenever the read position changes (so the playback position in the audio file or the table ruler in the table)." }
                    });
                    
                    auto& mid = ep.addChild<TextInput>({
                        { MultiPageIds::ID, "complexModuleId" },
                        { MultiPageIds::Text, "Module ID" },
                        { MultiPageIds::Help, "The ID of the module that you want to listen to. You can also listen to multiple modules at once, in this case just enter every ID separated by a comma" }
                    });
                    
                    mid.setCustomCheckFunction([](MultiPageDialog::PageBase* pb, var obj)
                    {
                        return Result::fail(pb->getValueFromGlobalState().toString() + " is not a module with a complex data object with the selected type.");
                    });
                    
                    ep.addChild<TextInput>({
                        { MultiPageIds::ID, "complexSlotIndex" },
                        { MultiPageIds::Text, "Slot Index" },
                        { MultiPageIds::Help, "The slot index of the complex data object that you want to listen to.\n> Some modules have multiple complex data objects (eg. the table envelope has two tables for the attack and release phase so if you want to listen to the release table, you need to pass in `1` here." }
                    });
                    
                    break;
                }
            }
            
            auto& additional = ep.addChild<List>({
                { MultiPageIds::Foldable, true },
                { MultiPageIds::Folded, true },
                { MultiPageIds::Text, "Additional Properties" },
                { MultiPageIds::Padding, 20 }
            });
            
            additional.addChild<TextInput>({
                { MultiPageIds::Text, "Comment" },
                { MultiPageIds::ID, "SourceComment" },
                { MultiPageIds::Help, "An additional comment that will be displayed on the broadcaster map\n> Pro tip: A comment is also a good hook for searching for the location in a big codebase!" }
            });
            
            sourceIndex = (SourceIndex)((int)sourceIndex + 1);
            
            
        }
        
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
}

//==============================================================================
MainComponent::MainComponent():
  Thread("Unit Test thread"),
  rt({})
{
    build();
	startTimer(150);

    //File f("D:\\Development\\test.json");

    
    
#if JUCE_WINDOWS
    context.attachTo(*this);
    setSize(850, 500);
	//setSize (2560, 1080);
#else
    setSize(850, 600);
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
    auto b = getLocalBounds();
    b.removeFromTop(32);
    
    if(c != nullptr)
        c->setBounds(b);
//    viewer.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
}



