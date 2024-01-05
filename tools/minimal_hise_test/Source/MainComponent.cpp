/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

struct MultiPageDialog: public Component
{
    enum ColourIds
    {
        backgroundColour = HiseColourScheme::ComponentBackgroundColour,
        textColour = HiseColourScheme::ComponentTextColourId,
        signalColour = HiseColourScheme::ComponentFillTopColourId,
        numColourIds
    };
    
    struct PageBase: public Component
    {
        PageBase(int width, const var& obj)
        {};
        
        virtual Result checkGlobalState(var globalState) = 0;
        
        virtual ~PageBase() {};
    };
    
    struct PositionInfo
    {
        var toJSON() const
        {
            auto obj = new DynamicObject();
            
            obj->setProperty("TopHeight", TopHeight);
            obj->setProperty("ButtonTab", ButtonTab);
            obj->setProperty("ButtonMargin", ButtonMargin);
            obj->setProperty("OuterPadding", OuterPadding);
            obj->setProperty("ErrorHeight", ErrorHeight);
            
            return var(obj);
        }
        
        void fromJSON(const var& obj)
        {
            TopHeight = obj.getProperty("TopHeight", 0);
            ButtonTab = obj.getProperty("ButtonTab", 0);
            ButtonMargin = obj.getProperty("ButtonMargin", 0);
            OuterPadding = obj.getProperty("OuterPadding", 0);
            ErrorHeight = obj.getProperty("ErrorHeight", 0);
        }
        
        int TopHeight = 50;
        int ButtonTab = 40;
        int ButtonMargin = 5;
        int OuterPadding = 30;
        int ErrorHeight = 50;
    };
    
    struct ErrorComponent: public Component,
                           public PathFactory
    {
        ErrorComponent():
          currentError(Result::ok()),
          parser(""),
          closeButton("close", nullptr, *this)
        {
            addAndMakeVisible(closeButton);
            closeButton.onClick = [this]()
            {
                this->show(false);
            };
        };
        
        void paint(Graphics& g) override
        {
            g.setColour(Colour(HISE_ERROR_COLOUR));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
            auto x = getLocalBounds().toFloat();
            x.removeFromRight(40.0f);
            x.removeFromLeft(10.0f);
            x.removeFromTop(3.0f);
            
            parser.draw(g, x);
        }
        
        int height = 40;
        
        void setError(const Result& r)
        {
            currentError = r;
            
            if(!r.wasOk())
            {
                parser.setNewText(r.getErrorMessage());
                parser.parse();
                height = jmax<int>(40, parser.getHeightForWidth(getWidth() - 50));
            }
            
            show(!currentError.wasOk());
        }
        
        void show(bool shouldShow)
        {
            setVisible(shouldShow);
            getParentComponent()->resized();
            setVisible(!shouldShow);
            
            auto& animator = Desktop::getInstance().getAnimator();
            
            if(shouldShow)
                animator.fadeIn(this, 50);
            else
                animator.fadeOut(this, 50);

            repaint();
        }
        
        void resized() override
        {
            auto b = getLocalBounds();
            auto cb = b.removeFromRight(40).removeFromTop(40).reduced(10);
            closeButton.setBounds(cb);
        }
        
        Path createPath(const String&) const override
        {
            Path p;
            p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, HiBinaryData::ProcessorEditorHeaderIcons::closeIcon_Size);
            
            return p;
        };
        
        MarkdownRenderer parser;
        
        HiseShapeButton closeButton;
        Result currentError;
        
    } errorComponent;
    
    struct PageInfo
    {
        PageBase* create(int currentWidth) const
        {
            if(pageCreator)
                return pageCreator(currentWidth, data);
            
            return nullptr;
        };
        
        var& operator[](const char* id)
        {
            if(auto obj = data.getDynamicObject())
            {
                obj->setProperty(Identifier(id), var());
                return *obj->getProperties().getVarPointer(Identifier(id));
            }
            
            static var nullValue;
            return nullValue;
        }
        
        var data;
        std::function<PageBase*(int width, const var&)> pageCreator;
    };
    
    MultiPageDialog():
      cancelButton("Cancel"),
      nextButton("Next"),
      prevButton("Previous"),
      globalState(new DynamicObject()),
      currentError(Result::ok())
    {
        setColour(backgroundColour, Colours::black.withAlpha(0.2f));
        
        styleData = MarkdownLayout::StyleData::createDarkStyle();
        
        addAndMakeVisible(cancelButton);
        addAndMakeVisible(nextButton);
        addAndMakeVisible(prevButton);
        addAndMakeVisible(content);
        addChildComponent(errorComponent);
        setLookAndFeel(&defaultLaf);
        
        setSize(600, 400);
        
        nextButton.onClick = [this]()
        {
            navigate(true);
        };
        
        prevButton.onClick = [this]()
        {
            navigate(false);
        };
        
        cancelButton.onClick = [this]()
        {
            if(NativeMessageBox::showOkCancelBox(MessageBoxIconType::QuestionIcon, "Cancel", "Do you want to cancel the process"))
            {
                MessageManager::callAsync(finishCallback);
            };
        };
    }
    
    template <typename T> PageInfo& addPage()
    {
        PageInfo p;
        p.data = styleData.toDynamicObject();
        p.pageCreator = [](int w, const var& d){ return new T(w, d); };
        pages.add(std::move(p));
        
        return pages.getReference(pages.size()-1);
    }
    
    void showFirstPage()
    {
        currentPage = nullptr;
        currentPageIndex = -1;
        navigate(true);
    }
    
    static void setGlobalState(Component& page, const Identifier& id, var newValue)
    {
        if(auto m = page.findParentComponentOfClass<MultiPageDialog>())
        {
            m->globalState.getDynamicObject()->setProperty(id, newValue);
        }
    }
    
    void setStyleData(const MarkdownLayout::StyleData& sd)
    {
        styleData = sd;
        errorComponent.parser.setStyleData(sd);
    }
    
    MarkdownLayout::StyleData styleData;
    
    static var getGlobalState(Component& page, const Identifier& id, const var& defaultValue)
    {
        if(auto m = page.findParentComponentOfClass<MultiPageDialog>())
        {
            return m->globalState.getProperty(id, defaultValue);
        }
        
        return defaultValue;
    }
    
    static std::pair<Font, Colour> getDefaultFont(Component& c)
    {
        if(auto m = c.findParentComponentOfClass<MultiPageDialog>())
        {
            return { m->styleData.getFont(), m->styleData.textColour };
        }
        
        return { Font(), Colours::white };
    }
    
    void setFinishCallback(const std::function<void()>& f)
    {
        finishCallback = f;
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(backgroundColour));
        if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
        {
            laf->drawHeader(g, *this, top);
            laf->drawButtonTab(g, *this, bottom);
        }
    }
    
    bool navigate(bool forward)
    {
        auto newIndex = currentPageIndex + (forward ? 1 : -1);
        
        prevButton.setEnabled(newIndex != 0);
        
        if(isPositiveAndBelow(newIndex, pages.size()))
        {
            if(forward && currentPage != nullptr)
            {
                auto ok = currentPage->checkGlobalState(globalState);
                errorComponent.setError(ok);
                
                if(!ok.wasOk())
                    return false;
            }
            
            if((currentPage = pages[newIndex].create(content.getWidth() - content.getScrollBarThickness())))
            {
                content.setViewedComponent(currentPage);
                currentPageIndex = newIndex;
                
                currentPage->setLookAndFeel(&getLookAndFeel());
                
                nextButton.setButtonText(currentPageIndex == pages.size() - 1 ? "Finish" : "Next");
                
                return true;
            }
        }
        else if (newIndex == pages.size())
        {
            MessageManager::callAsync(finishCallback);
            return true;
        }
        
        return false;
    }
    
    void resized() override
    {
        PositionInfo position;
        
        if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
        {
            position = laf->getPositionInfo();
        }
        
        auto b = getLocalBounds().reduced(position.OuterPadding);
        
        top = b.removeFromTop(position.TopHeight);
        b.removeFromTop(position.OuterPadding);
        bottom = b.removeFromBottom(position.ButtonTab);
        b.removeFromBottom(position.OuterPadding);
        center = b;
        
        auto copy = bottom;
        
        
        cancelButton.setBounds(copy.removeFromLeft(140).reduced(position.ButtonMargin));
        
        nextButton.setBounds(copy.removeFromRight(140).reduced(position.ButtonMargin));
        prevButton.setBounds(copy.removeFromRight(140).reduced(position.ButtonMargin));
        
        copy = center;

        errorComponent.setBounds(copy.removeFromTop(errorComponent.isVisible() ? errorComponent.height : 0));
        
        if(errorComponent.isVisible())
            copy.removeFromTop(position.OuterPadding);
        
        content.setBounds(copy);
    }
    
    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() {};
        virtual void drawHeader(Graphics& g, MultiPageDialog& d, Rectangle<int> area) = 0;
        virtual void drawButtonTab(Graphics& g, MultiPageDialog& d, Rectangle<int> area) = 0;
        
        virtual PositionInfo getPositionInfo() const = 0;
    };
    
    struct DefaultLookAndFeel: public hise::GlobalHiseLookAndFeel,
                               public LookAndFeelMethods
    {
        void drawHeader(Graphics& g, MultiPageDialog& d, Rectangle<int> area) override
        {
            g.setColour(d.styleData.textColour);
            
            auto f = d.styleData.getBoldFont().withHeight(area.getHeight() - 10);
            
            g.fillRect(area.removeFromBottom(2));
            g.setFont(f);
            g.setColour(d.styleData.headlineColour);
            g.drawText(d.getName(), area.toFloat(), Justification::left);
        }
        
        void drawButtonTab(Graphics& g, MultiPageDialog& d, Rectangle<int> area)
        {
            
        }
        
        PositionInfo getPositionInfo() const override
        {
            return defaultPosition;
        }
        
        void layoutFilenameComponent (FilenameComponent& filenameComp,
                                                      ComboBox* filenameBox, Button* browseButton) override
        {
            if (browseButton == nullptr || filenameBox == nullptr)
                return;

            auto b = filenameComp.getLocalBounds();
            
            browseButton->setBounds(b.removeFromRight(100));
            b.removeFromRight(getPositionInfo().OuterPadding);

            filenameBox->setBounds(b);
        }
        
        PositionInfo defaultPosition;
    } defaultLaf;
    
    Array<PageInfo> pages;
    
    int currentPageIndex = -1;
    TextButton cancelButton;
    TextButton nextButton;
    TextButton prevButton;
    
    Result currentError;
    
    Viewport content;
    
    ScopedPointer<PageBase> currentPage;
    
    Rectangle<int> top, bottom, center;
    
    std::function<void()> finishCallback;
    
    var globalState;
};

namespace PageFactory
{

struct MarkdownText: public MultiPageDialog::PageBase
{
    MarkdownText(int width, const var& d):
      PageBase(width, d),
      r(d["Text"].toString())
    {
        auto sd = r.getStyleData();
        sd.fromDynamicObject(d, [](const String& fn){ return Font(fn, 13.0f, Font::plain); });
        r.setStyleData(sd);
        r.parse();
        auto h = roundToInt(r.getHeightForWidth(width));
        setSize(width, h);
    }
    
    void paint(Graphics& g) override
    {
        r.draw(g, getLocalBounds().toFloat());
    }
    
    Result checkGlobalState(var) override
    {
        return Result::ok();
    }
    
    MarkdownRenderer r;
};

struct FileSelector: public MultiPageDialog::PageBase
{
    static FilenameComponent* createFileComponent(const var& obj)
    {
        bool isDirectory = obj["isDirectory"];
        auto name = obj["Description"].toString();
        if(name.isEmpty())
            name = isDirectory ? "Directory" : "File";
        auto wildcard = obj["wildcard"].toString();
        auto defaultFile = getInitialFile(obj);
        auto save = (bool)obj["writeAccess"];
        
        return new FilenameComponent(name, defaultFile, true, isDirectory, save, wildcard, "", "");
    }
    
    FileSelector(int width, const var& obj):
      PageBase(width, obj),
      fileSelector(createFileComponent(obj)),
      fileId(obj["ID"].toString())
    {
        isDirectory = obj["isDirectory"];
        addAndMakeVisible(fileSelector);
        
        fileSelector->setBrowseButtonText("Browse");
        hise::GlobalHiseLookAndFeel::setDefaultColours(*fileSelector);
        
        setSize(width, 32);
    }

    static File getInitialFile(const var& obj)
    {
        auto path = obj["defaultFile"];
            
        if(path.isString())
            return File(path);
        if(path.isInt() || path.isInt64())
        {
            auto specialLocation = (File::SpecialLocationType)(int)path;
            return File::getSpecialLocation(specialLocation);
        }
        
        return File();
    }
    
    void resized() override
    {
        fileSelector->setBounds(getLocalBounds());
    }
    
    Result checkGlobalState(var globalState) override
    {
        auto f = fileSelector->getCurrentFile();
        
        if(f != File() && !f.isRoot() && (f.isDirectory() || f.existsAsFile()))
        {
            MultiPageDialog::setGlobalState(*this, fileId, f.getFullPathName());
            return Result::ok();
        }
        
        String message;
        message << "You need to select a ";
        if(isDirectory)
            message << "directory";
        else
            message << "file";
        
        return Result::fail(message);
    }
    
    bool isDirectory = false;
    ScopedPointer<juce::FilenameComponent> fileSelector;
    Identifier fileId;
};

struct Container: public MultiPageDialog::PageBase
{
    Container(int width, const var& obj):
      PageBase(width, obj)
    {
        auto l = obj["Children"];
        
        if(l.isArray())
        {
            for(auto& r: *l.getArray())
                addChild(width, r);
        }
    };
    
    virtual ~Container() {};
    
    OwnedArray<PageBase> childItems;
    
    Result checkGlobalState(var globalState) override
    {
        for(auto c: childItems)
        {
            auto ok = c->checkGlobalState(globalState);
            
            if(!ok.wasOk())
                return ok;
        }
        
        return Result::ok();
    }

    void addChild(int width, const var& r)
    {
        auto type = r["Type"].toString();
        
        if(type == "FileSelector")
        {
            childItems.add(new PageFactory::FileSelector(width, r));
            addAndMakeVisible(childItems.getLast());
        }
        if(type == "Markdown")
        {
            childItems.add(new PageFactory::MarkdownText(width, r));
            addAndMakeVisible(childItems.getLast());
        }
    }
};

struct Tickbox: public MultiPageDialog::PageBase
{
    Tickbox(int width, const var& obj):
      PageBase(width, obj)
    {
        label = obj["Label"].toString();
        id = obj["ID"].toString();
        
        if(obj.hasProperty("Required"))
        {
            required = true;
            requiredOption = obj["Required"];
        }
    };
    
    void paint(Graphics& g) override
    {
        auto b = getLocalBounds();
        

        auto df = MultiPageDialog::getDefaultFont(*this);
        
        g.setFont(df.first);
        g.setColour(df.second);
        
        
        g.drawText(label, b.toFloat(), Justification::left);
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        button.setBounds(b.removeFromRight(100));
    }
  
    Result checkGlobalState(var globalState) override
    {
        if(required)
        {
            if(button.getToggleState() != required)
            {
                return Result::fail("You need to tick " + label);
            }
        }
    }
    
    bool required = false;
    bool requiredOption = false;
    
    String label;
    Identifier id;
    ToggleButton button;
};

struct List: public Container
{
    List(int width, const var& obj):
      Container(width, obj)
    {
        int h = 0;
        
        padding = (int)obj["Padding"];
        
        for(auto& c: childItems)
            h += c->getHeight() + padding;
        
        setSize(width, h);
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        
        for(auto c: childItems)
        {
            c->setBounds(b.removeFromTop(c->getHeight()));
            b.removeFromTop(padding);
        }
    }
    
    int padding = 0;
};

}

//==============================================================================
MainComponent::MainComponent():
  Thread("Unit Test thread")
{
	startTimer(150);

    auto mp = new MultiPageDialog();
    
    auto sd = MarkdownLayout::StyleData::createDarkStyle();
    
    sd.headlineColour = Colours::yellow;
    sd.f = Font("Arial", 18.0f, Font::plain);
    
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
    
    mp->showFirstPage();
    
    mp->setFinishCallback([](){ JUCEApplication::getInstance()->systemRequestedQuit();
    });
    
    addAndMakeVisible(c = mp);
    
    mp->setName("Headline");
    
#if JUCE_WINDOWS
    context.attachTo(*this);
    setSize(1440, 900);
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
	
}

void MainComponent::resized()
{
    c->setBounds(getLocalBounds());
//    viewer.setBounds(getLocalBounds());
}

void MainComponent::timerCallback()
{
}



