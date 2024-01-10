/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "MultiPageDialog.h"
#include "PageFactory.h"

namespace hise {
namespace multipage {
using namespace juce;



struct CustomResultPage: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(CustomResultPage)
    {
        return {
            { mpid::ID, "custom" }
        };
    }

    CustomResultPage(Dialog& r, int width, const var& obj):
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
        auto gs = Dialog::getGlobalState(*this, {}, var());

        String b;

        b << "const var " << Dialog::getGlobalState(*this, "id", "bc").toString() << " = Engine.createBroadcaster(";
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

}
}

void MainComponent::build()
{
    var obj;

    //auto ok = JSON::parse(f.loadFileAsString(), obj);

    using namespace multipage;
    using namespace factory;

    auto mp = new Dialog({}, rt);

    mp->setProperty(mpid::Header, "Multipage Dialog Wizard");

    auto sd = mp->getStyleData();
    sd.backgroundColour = Colours::transparentBlack;
    sd.fontSize = 16.0f;
    mp->setStyleData(sd);

    mp->defaultLaf.defaultPosition.OuterPadding = 50;
    

    {
        // TODO: Fix setting properties in nested JSON
        auto& introPage = mp->addPage<List>({
            { mpid::Padding, 10 },
        });

        introPage.addChild<MarkdownText>({
            { mpid::Text, "Welcome to the JSON builder. You can use this wizard to create a JSON data object that will create a wizard like this!  \n> You can toggle between the editor, a raw JSON viewer and a preview of the wizard that you're about to create by clicking on the buttons in the top row.\nAs first step please enter the name that it should show in the header (like the **JSON Builder** above, then specify the number of pages." }
        });

        auto& propList = introPage.addChild<List>({
            { mpid::Padding, 10 },
            { mpid::ID, mpid::Properties.toString() }
        });
        
        propList.addChild<TextInput>({
            { mpid::ID, mpid::Header.toString() },
            { mpid::Text, mpid::Header.toString() },
            { mpid::Required, true },
            { mpid::Help, "This will be shown as big title at the top" }
        });

        propList.addChild<TextInput>({
            { mpid::ID, mpid::Subtitle.toString() },
            { mpid::Text, mpid::Subtitle.toString() },
            { mpid::Help, "This will be shown as small title below" }
        });
        
        auto& styleProperties = introPage.addChild<List>({
            { mpid::ID, mpid::StyleData.toString(), },
            { mpid::Text, mpid::StyleData.toString(), },
            { mpid::Padding, 10 },
            { mpid::Foldable, true },
            { mpid::Folded, true }
        });
        
        auto sdData = sd.toDynamicObject();

        const Array<Identifier> hiddenProps({
          Identifier("codeBgColour"),
	      Identifier("linkBgColour"),
	      Identifier("codeColour"),
	      Identifier("linkColour"),
	      Identifier("tableHeaderBgColour"),
	      Identifier("tableLineColour"),
	      Identifier("tableBgColour")
        });

        for(auto& nv: sdData.getDynamicObject()->getProperties())
        {
            if(hiddenProps.contains(nv.name))
                continue;
    
            if(nv.name.toString().contains("Colour"))
	        {
		        auto& ed = styleProperties.addChild<ColourChooser>({
                    { mpid::ID, nv.name.toString() },
                    { mpid::Text, nv.name.toString() },
                    { mpid::Value, nv.value }
		        });
	        }
            else
            {
	            auto& ed = styleProperties.addChild<TextInput>({
                    { mpid::ID, nv.name.toString() },
                    { mpid::Text, nv.name.toString() },
                    { mpid::Value, nv.value }
	            });

                if(nv.name == Identifier("Font") || nv.name == Identifier("BoldFont"))
	            {
		            ed[mpid::Items] = Font::findAllTypefaceNames().joinIntoString("\n");
	            }

            }
        }

        

        {
            auto& layoutProperties = introPage.addChild<List>({
	            { mpid::ID, mpid::LayoutData.toString() },
	            { mpid::Text, mpid::LayoutData.toString() },
	            { mpid::Padding, 10 },
	            { mpid::Foldable, true },
	            { mpid::Folded, true }
	        });

			auto layoutObj = mp->defaultLaf.getMultiPagePositionInfo({}).toJSON();

            std::map<Identifier, String> help;
	        help["OuterPadding"] = "The distance between the content and the component bounds in pixel.";
            help["ButtonTab"] = "The height of the bottom tab with the Cancel / Prev / Next buttons.";
            help["ButtonMargin"] = "The distance between the buttons in the bottom tab in pixel.";
            help["TopHeight"] = "The height of the top bar with the title and the step progress in pixel.";
            help["LabelWidth"] = "The width of the text labels of each property component. You can use either relative or absolute size values:\n- Negative values are relative to the component width (minus the `OuterPadding` property)\n- positive values are absolute pixel values.";
            
	        for(auto& v: layoutObj.getDynamicObject()->getProperties())
	        {
		        layoutProperties.addChild<TextInput>({
	                { mpid::ID, v.name.toString() },
					{ mpid::Text, v.name.toString() },
	                { mpid::Value, v.value },
                    { mpid::Help, help[v.name] }
		        });
	        }
        }
        
        {
            auto& numPageCreator = mp->addPage<List>({
                { mpid::Padding, 30 }
            });

            numPageCreator.addChild<MarkdownText>({
                { mpid::Text, "Now please enter the number of pages that you want to add to your dialog. You can then design each page in the next steps. " }
            });

	        auto& te = numPageCreator.addChild<TextInput>({
		        { mpid::Text, "Number of pages" },
	            { mpid::Required, true },
                { mpid::ID, "NumPages" },
	            { mpid::Help, "You can specify a number of pages that the wizard will have. This will be displayed at the top and you can navigate with the Next / previous page buttons at the bottom" }
			});

            

            te.setStateObject(var(new DynamicObject()));

            te.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj)
            {
                auto gs = b->getParentDialog().getState().globalState;

                var list = gs[mpid::Children];

                if(!list.isArray())
                {
	                gs.getDynamicObject()->setProperty(mpid::Children, var(Array<var>()));
                    list = gs[mpid::Children];
                }
                    
                auto te = dynamic_cast<TextInput*>(b)->getComponent<TextEditor>().getText().getIntValue();

                if(te == 0)
                    return Result::fail("The number of pages cannot be zero. Please enter a number bigger than 0 in order to proceed.");

                auto currentNumPages = (int)b->getParentDialog().getProperties()["NumPages"];

                if(currentNumPages != te)
                {
		            obj.getDynamicObject()->setProperty(mpid::Children, var(list));

		            for(int i = 0; i < te; i++)
		            {
			            auto& xxx = b->getParentDialog().addPage<List>({}, 2);

                        String text;

                        text << "### Edit Page " << String((te - i)) << " / " << String(te) <<  "\nYou can define the appearance of this Page by setting the properties and adding UI elements to the `Children` list below.";

                        xxx.addChild<MarkdownText>({
							{ mpid::Text, text }
                        });

                        xxx[mpid::Value] = "page" + String(te-i);

	                    DynamicObject* no = new DynamicObject();
	                    xxx.setStateObject(var(no));
		                ScopedPointer<Dialog::PageBase> c2 = xxx.create(b->getParentDialog(), 0);
	                    
		                list.insert(0, var(no));
                        
				        c2->createEditorInfo(&xxx);
                        
                        
		            }

                    b->getParentDialog().getProperties().set("NumPages", te);
                }

	            
	            return Result::ok();
            });


        }

        


        

        
        
        

#if 0
        auto& p1 = mp->addPage<List>();


        
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
        
        //MultiPageDialog::setGlobalState(*mp, "Source", -1);
        //MultiPageDialog::setGlobalState(*mp, "complexDataType", "SliderPack");
        //MultiPageDialog::setGlobalState(*mp, "complexEventType", "Content");
        //MultiPageDialog::setGlobalState(*mp, "complexSlotIndex", 0);
        
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
#endif

    }

    mp->addPage<MarkdownText>({
		{ mpid::Text, "Press finish in order to copy the JSON to the clipboard." }
    });

#if 0
    mp->addPage<CustomResultPage>();
#endif

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
  rt({}),
  pt({}),
  editButton("Edit"),
  codeButton("State"),
  previewButton("Preview"),
  doc(),
  stateDoc(doc),
  stateViewer(stateDoc)
{
    build();
	startTimer(150);

    addChildComponent(stateViewer);

    addAndMakeVisible(editButton);
    addAndMakeVisible(codeButton);
    addAndMakeVisible(previewButton);
    
    editButton.setLookAndFeel(&alaf);
    codeButton.setLookAndFeel(&alaf);
    previewButton.setLookAndFeel(&alaf);

    editButton.addListener(this);
    codeButton.addListener(this);
    previewButton.addListener(this);

    //File f("D:\\Development\\test.json");

    
    
#if JUCE_WINDOWS
    
    setSize(1000, 900);
	//setSize (2560, 1080);
#else
    setSize(850, 600);
#endif
    
    
    
    
}

MainComponent::~MainComponent()
{
#if JUCE_WINDOWS
	//context.detach();
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
    auto buttonRow = b.removeFromTop(40);

    editButton.setBounds(buttonRow.removeFromLeft(getWidth() / 3).reduced(10));
    codeButton.setBounds(buttonRow.removeFromLeft(getWidth() / 3).reduced(10));
    previewButton.setBounds(buttonRow.reduced(10));

    if(c != nullptr)
        c->setBounds(b);

    stateViewer.setBounds(b);

    if(preview != nullptr)
        preview->setBounds(b);
//    viewer.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
}



