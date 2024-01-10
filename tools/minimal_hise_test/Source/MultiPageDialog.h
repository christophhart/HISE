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

#pragma once

#include <JuceHeader.h>

#include "MultiPageIds.h"

namespace hise
{


namespace multipage {
    using namespace juce;


/* TODO:
 *
 *  - custom error message
 *  - help button attachment OK
 *  - column mode OK
 *  - combobox OK
 *  - add dynamic element
 *  - extractor component OK
 *  - show total progress OK
 *  - implement header stuff OK
 *  - add modal overlay (and use the help button text here!!!) OK
 *  - add Progress OK
 *  - add branching at next button (maybe make a container that shows one of its child items dependingon a global index variable) OK
 *  - add text input with autocomplete suggestions OK
 *  - broadcaster specific: add preview broadcaster with layouts
 * */
struct MultiPageDialog: public Component,
					    public PathFactory
{
    //using DefaultProperties = std::initializer_list<std::pair<Identifier, var>>;
    
    enum ColourIds
    {
        backgroundColour = HiseColourScheme::ComponentBackgroundColour,
        textColour = HiseColourScheme::ComponentTextColourId,
        signalColour = HiseColourScheme::ComponentFillTopColourId,
        numColourIds
    };
    
    struct PageInfo;
    
    struct PageBase: public Component
    {
        using CustomCheckFunction = std::function<Result(PageBase*, var)>;

        PageBase(MultiPageDialog& rootDialog, int width, const var& obj);

        virtual DefaultProperties getDefaultProperties() const
        {
            return {};
        }
        
        virtual void setStateObject(const var& newStateObject)
        {
            stateObject = newStateObject;
        }

        void init()
        {
	        if(!initValue.isUndefined() && !initValue.isVoid())
	        {
		        writeState(initValue);
                initValue = var();
	        }
        }
        
        template <typename PageSubType, typename StopType=juce::Toolbar> PageSubType* getRootPage()
        {
            auto p = this;
            
            while(auto otherP = p->findParentComponentOfClass<PageSubType>())
            {
                if(dynamic_cast<StopType*>(otherP) != nullptr)
                    return dynamic_cast<PageSubType*>(p);
                
                p = otherP;
            }
            
            return dynamic_cast<PageSubType*>(p);
        }
        
        void writeState(const var& newValue)
        {
            stateObject.getDynamicObject()->setProperty(id, newValue);
        }
        
    	var getValueFromGlobalState(var defaultState=var());
        virtual void postInit() {};
        
        virtual ~PageBase() {};
        
        Result check(const var& obj)
        {
	        if(cf)
            {
                auto ok = cf(this, obj);
                
                if(ok.failed())
                {
                    if(rootDialog.currentErrorElement == nullptr)
                        rootDialog.currentErrorElement = this;

                    return ok;
                }
            }

            auto ok = checkGlobalState(obj);

            if(ok.failed() && rootDialog.currentErrorElement == nullptr)
                rootDialog.currentErrorElement = this;

            return ok;
        }
        
        void clearCustomFunction()
        {
            cf = {};
        }

        void setCustomCheckFunction(const CustomCheckFunction& cf_)
        {
            if(!cf)
                cf = cf_;
        }

        Identifier id;

        bool isError() const { return rootDialog.currentErrorElement == this; }
        
        virtual void createEditorInfo(PageInfo* info) {};
        
        MultiPageDialog& rootDialog;

    protected:

        var initValue;

        struct HelpButton: public HiseShapeButton
	    {
		    HelpButton(String help, const PathFactory& factory);
	        
	    };

        ScopedPointer<HelpButton> helpButton;

        virtual Result checkGlobalState(var globalState) = 0;

        CustomCheckFunction cf;
        int padding = 0;
        var stateObject;
    };
    
    struct RunThread: public Thread
    {
        RunThread(const var& obj):
          Thread("Tasks"),
          currentError(Result::ok())
        {
            if(auto gs = obj[MultiPageIds::GlobalState].getDynamicObject())
                globalState = var(gs->clone());
            else
                globalState = var(new DynamicObject());
        };
        
        ~RunThread()
        {
            stopThread(1000);
        }
        
        double totalProgress = 0.0;
        
        
        
        void onFinish()
        {
            if(currentDialog.get() != nullptr)
            {
                currentDialog->nextButton.setEnabled(currentDialog->currentErrorElement == nullptr);
                currentDialog->prevButton.setEnabled(true);
            }
        }
        
        
        
        void run() override
        {
            for(int i = 0; i < jobs.size(); i++)
            {
                auto ok = jobs[i]->runJob();
                
                if(ok.failed())
                {
                    break;
                }
                
                totalProgress = (double)i / (double)jobs.size();
            }
            
            jobs.clear();
            
            MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(RunThread::onFinish));
        }
        
        struct Job: public ReferenceCountedObject
        {
            using Ptr = ReferenceCountedObjectPtr<Job>;
            using List = ReferenceCountedArray<Job>;
            using JobFunction = std::function<Result(Job&)>;
            
            Job(RunThread& rt, const var& obj):
              parent(rt),
              localObj(obj)
            {
                parent.addJob(this);
            }
            
            virtual ~Job() {};
            
            bool matches(const var& obj) const
            {
                return localObj[MultiPageIds::ID] == obj[MultiPageIds::ID];
            }
            
            Result runJob()
            {
                auto ok = run();
                
                if(auto p = parent.currentDialog)
                {
                    MessageManager::callAsync([p, ok]()
                    {
                        p->repaint();
                        p->errorComponent.setError(ok);
                    });
                }
                
                return ok;
            }
            
            double& getProgress() { return progress; }
            
        protected:
            
            virtual Result run() = 0;
            
            RunThread& parent;
            double progress = 0.0;
            var localObj;
        };
        
        Job::Ptr getJob(const var& obj)
        {
            for(auto j: jobs)
            {
                if(j->matches(obj))
                    return j;
            }
            
            return nullptr;
        }

        var getGlobalSubState(const Identifier& id)
        {
            if(globalState.hasProperty(id))
                return globalState[id];

	        var no = new DynamicObject();
            globalState.getDynamicObject()->setProperty(id, no);
            return no;
        }

        void addJob(Job::Ptr b, bool addFirst=false)
        {
            if(addFirst)
                jobs.insert(0, b);
            else
                jobs.add(b);
            
            if(!isThreadRunning())
            {
                if(currentDialog != nullptr)
                {
                    currentDialog->currentErrorElement = nullptr;
                    currentDialog->repaint();
                    currentDialog->errorComponent.setError(Result::ok());
                    currentDialog->nextButton.setEnabled(false);
                    currentDialog->prevButton.setEnabled(false);
                }
                
                startThread(6);
            }
        }
        
        Job::List jobs;
        
        Result currentError;
        
        WeakReference<MultiPageDialog> currentDialog;
        
        var globalState;
        
        int currentPageIndex = 0;
    };
    
    struct PositionInfo
    {
        var toJSON() const;

        void fromJSON(const var& obj);

        int TopHeight = 50;
        int ButtonTab = 40;
        int ButtonMargin = 5;
        int OuterPadding = 30;
        double LabelWidth = -0.25;
    };
    
    struct ErrorComponent: public Component
    {
        ErrorComponent(MultiPageDialog& parent_);;
        
        void paint(Graphics& g) override;
        void resized() override;

        void setError(const Result& r);
        void show(bool shouldShow);
        
        MultiPageDialog& parent;

        int height = 40;
        MarkdownRenderer parser;
        ScopedPointer<HiseShapeButton> closeButton;
        Result currentError;
        
    };
    
    struct PageInfo: public ReferenceCountedObject
    {
        PageInfo() = default;
        PageInfo(const var& obj): data(obj) {};

        using Ptr = ReferenceCountedObjectPtr<PageInfo>;
        using List = ReferenceCountedArray<PageInfo>;
        using CreateFunction = std::function<PageBase*(MultiPageDialog&, int width, const var&)>;

        operator bool() const noexcept { return (bool)pageCreator; }
        PageBase* create(MultiPageDialog& r, int currentWidth) const;;
        var& operator[](const Identifier& id) const;
        var getData() const { return data; }
        
        template <typename T> static PageInfo::Ptr createInfo()
        {
	        PageInfo::Ptr p = new PageInfo();
	        p->data = new DynamicObject();
	        p->data.getDynamicObject()->setProperty(MultiPageIds::Type, T::getStaticId().toString());
	        p->pageCreator = [](MultiPageDialog& r, int w, const var& d){ return new T(r, w, d); };
            return p;
        }
        
        template <typename T> PageInfo& addChild(DefaultProperties&& values={})
        {
	        childItems.add(createInfo<T>());
            
            for(auto& v: values)
                childItems.getLast()->data.getDynamicObject()->setProperty(v.first, v.second);
            
            return *childItems.getLast();
        }

        void setCreateFunction(const CreateFunction& f)
        {
	        pageCreator = f;
        }

        void setCustomCheckFunction(const PageBase::CustomCheckFunction& f)
        {
	        customCheck = f;
        }

        void setStateObject(const var& newStateObject)
        {
	        stateObject = newStateObject;
        }
        
        Ptr clone()
        {
            Ptr s = new PageInfo();
            s->data = data.clone();
            
            for(auto c: childItems)
                s->childItems.add(c->clone());
            
            s->pageCreator = pageCreator;
            s->customCheck = customCheck;
            s->stateObject = stateObject;
            
            return s;
        }

        var stateObject;
        var data;
        CreateFunction pageCreator;
        List childItems;
        PageBase::CustomCheckFunction customCheck;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageInfo);
    };

    struct Factory
    {
        Factory();

        PageInfo::Ptr create(const var& obj);

        StringArray getIdList() const
        {
            StringArray sa;
            
            for(const auto& i: items)
                sa.add(i.id.toString());
            
            return sa;
        }
        
    private:

        template <typename T> void registerPage()
	    {
		    Item item;
            item.id = T::getStaticId();
            item.f = [](MultiPageDialog& r, int width, const var& data) { return new T(r, width, data); };
            items.add(std::move(item));
	    }

        struct Item
	    {
		    Identifier id;
            PageInfo::CreateFunction f;
	    };

        Array<Item> items;
    };

    MultiPageDialog(const var& obj, RunThread& rt);

    ~MultiPageDialog()
    {
        runThread->currentDialog = nullptr;
    }
    
    template <typename T> PageInfo& addPage(DefaultProperties&& values={}, int index = -1)
    {
        PageInfo::Ptr p;

        if(index == -1)
			p = pages.add(PageInfo::createInfo<T>());
        else
            p = pages.insert(index, PageInfo::createInfo<T>());
        
        if(values.size())
        {
            for (const auto& v: values)
                (*p)[v.first] = v.second;
        }
        else
        {
            for (const auto& v: T::getStaticDefaultProperties())
                (*p)[v.first] = v.second;
        }
        
        return *p;
    }
    
    Result getCurrentResult() { return runThread->currentError; }
    
    void showFirstPage();
	void setFinishCallback(const std::function<void()>& f);

    static var getOrCreateChild(const var& obj, const Identifier& id);

    //static void setGlobalState(Component& page, const Identifier& id, var newValue);
    static var getGlobalState(Component& page, const Identifier& id, const var& defaultValue);
    static std::pair<Font, Colour> getDefaultFont(Component& c);

    RunThread::Job::Ptr getJob(const var& obj)
    {
        return runThread->getJob(obj);
    }
    
    Path createPath(const String& url) const override;

    MarkdownLayout::StyleData getStyleData() const { return styleData; }

    void setProperty(const Identifier& id, const var& newValue)
    {
	    properties.getDynamicObject()->setProperty(id, newValue);
    }
    
    void setStyleData(const MarkdownLayout::StyleData& sd);
    bool navigate(bool forward);

    bool getCurrentNavigationDirection() { return currentNavigation; }
    
    void paint(Graphics& g) override;
    void resized() override;

    RunThread& getRunThread() { return *runThread; }
    
    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() {};
        virtual void drawMultiPageHeader(Graphics& g, MultiPageDialog& d, Rectangle<int> area);
        virtual void drawMultiPageButtonTab(Graphics& g, MultiPageDialog& d, Rectangle<int> area);
        virtual void drawMultiPageModalBackground(Graphics& g, Rectangle<int> totalBounds, Rectangle<int> modalBounds)
        {
	        g.setColour(Colour(0xFF222222).withAlpha(0.9f));
            g.fillRect(totalBounds);
            
            DropShadow sh(Colours::black.withAlpha(0.7f), 30, { 0, 0 });
            sh.drawForRectangle(g, modalBounds);

            g.setColour(Colour(0xFF333333));
            g.fillRoundedRectangle(modalBounds.toFloat(), 3.0f);
            g.setColour(Colour(0xFFAAAAAA));
            g.drawRoundedRectangle(modalBounds.toFloat(), 3.0f, 1.0f);
        }


        
        virtual void drawMultiPageFoldHeader(Graphics& g, Component& c, Rectangle<float> area, const String& title, bool folded)
        {
            auto f = MultiPageDialog::getDefaultFont(c);
            
            g.setColour(Colours::black.withAlpha(0.2f));
            
            Path bg;
            bg.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), 8.0f, 8.0f, true, true, folded, folded);
            
            g.fillPath(bg);
            
            Path p;
            p.addTriangle({0.5, 0.0}, {1.0, 1.0}, {0.0, 1.0});
            
            p.applyTransform(AffineTransform::rotation(folded ? float_Pi * 0.5f : float_Pi));
            
            g.setFont(f.first.boldened());
            g.setColour(f.second.withAlpha(folded ? 0.7f : 1.0f));
            
            PathFactory::scalePath(p, area.removeFromLeft(area.getHeight()).reduced(10));
            
            g.fillPath(p);
            
            g.drawText(title, area, Justification::centredLeft);
        }
        
        virtual void drawMultiPageBackground(Graphics& g, MultiPageDialog& tb, Rectangle<int> errorBounds)
        {
            g.fillAll(tb.findColour(backgroundColour));
            
            if(!errorBounds.isEmpty())
            {
                g.setColour(Colour(HISE_ERROR_COLOUR).withAlpha(0.15f));
                auto eb = errorBounds.toFloat().expanded(-1.0f, 3.0f);
                g.fillRoundedRectangle(eb, 3.0f);
                g.setColour(Colour(HISE_ERROR_COLOUR));
                g.drawRoundedRectangle(eb, 3.0f, 2.0f);
            }
        }
        
        virtual PositionInfo getMultiPagePositionInfo(const var& pageData) const = 0;
    };
    
    struct DefaultLookAndFeel: public hise::GlobalHiseLookAndFeel,
                               public LookAndFeelMethods
    {
        void drawToggleButton(Graphics& g, ToggleButton& tb, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        PositionInfo getMultiPagePositionInfo(const var& pageData) const override;
        void layoutFilenameComponent (FilenameComponent& filenameComp,
                                      ComboBox* filenameBox, Button* browseButton) override;
        
        bool isProgressBarOpaque(ProgressBar&) override { return false; }
        
        void drawProgressBar (Graphics &g, ProgressBar &pb, int width, int height, double progress, const String &textToShow) override
        {
            
            auto f = getDefaultFont(pb);
            
            Rectangle<float> area = pb.getLocalBounds().toFloat();
            
            g.setColour(f.second);
            
            area = area.reduced(1.0f);
            
            g.drawRoundedRectangle(area, area.getHeight() / 2, 1.0f);
            
            area = area.reduced(3.0f);
            
            auto copy = area.reduced(2.0f);
            
            area = area.removeFromLeft(jmax<float>(area.getHeight(), area.getWidth() * progress));
            
            g.fillRoundedRectangle(area, area.getHeight() * 0.5f);
            
            g.setColour(f.second.contrasting().withAlpha(progress > 0.5f ? 0.6f : 0.2f));
            g.fillRoundedRectangle(copy.withSizeKeepingCentre(copy.getHeight() + f.first.getStringWidthFloat(textToShow), copy.getHeight()), copy.getHeight() * 0.5f);
            
            g.setColour(f.second);
            g.setFont(f.first);
            g.drawText(textToShow, copy, Justification::centred);
        }

        
        
        
        PositionInfo defaultPosition;
    } defaultLaf;

    struct ModalPopup: public Component
    {
        ModalPopup(MultiPageDialog& parent_):
          parent(parent_),
          modalError(parent),
    	  okButton("OK"),
    	  cancelButton("Cancel")
        {
            addChildComponent(modalError);
	        addAndMakeVisible(okButton);
            addAndMakeVisible(cancelButton);

            okButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::onOk);
            cancelButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::dismiss);
        };

        void onOk()
        {
	        if(contentComponent != nullptr)
	        {
		        auto r = contentComponent->check(getGlobalState(*this, {}, var()));

                if(!r.wasOk())
                {
	                modalError.setError(r);
                }
	        }
        }

        void dismiss()
        {
            Desktop::getInstance().getAnimator().fadeOut(this, 100);
        }

        void setContent(PageInfo::Ptr newContent)
        {
	        content = newContent;
        }

        void show(PositionInfo newInfo, bool addButtons)
        {
            info = newInfo;

            okButton.setVisible(addButtons);
            cancelButton.setVisible(addButtons);
            
            if(content != nullptr)
            {
	            addAndMakeVisible(contentComponent = content->create(parent, getWidth() - 4 * info.OuterPadding));

                contentComponent->postInit();
				resized();
                Desktop::getInstance().getAnimator().fadeIn(this, 100);
                

                toFront(true);
            }
        }

        void mouseDown(const MouseEvent& e) override
        {
            if(!modalBounds.contains(e.getPosition()) && !cancelButton.isVisible())
            {
                dismiss();
            }
        }
        
        void paint(Graphics& g) override
        {
	        if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		        laf->drawMultiPageModalBackground(g, getLocalBounds(), modalBounds);
        }

        void resized() override
        {
            auto b = getLocalBounds();

            int thisHeight = 2 * info.OuterPadding;

            if(contentComponent != nullptr)
                thisHeight += contentComponent->getHeight();

            if(modalError.isVisible())
                thisHeight += modalError.height;

            if(okButton.isVisible())
                thisHeight += info.ButtonTab;

            b = b.withSizeKeepingCentre(getWidth() - 2 * info.OuterPadding, thisHeight);

            modalBounds = b;

            b = b.reduced(info.OuterPadding);
            
            if(modalError.isVisible())
                modalError.setBounds(b.removeFromTop(modalError.height).expanded(6, 3));

            if(okButton.isVisible())
            {
                auto bottom = b.removeFromBottom(info.ButtonTab);
                
                okButton.setBounds(bottom.removeFromLeft(100).reduced(info.ButtonMargin));
                cancelButton.setBounds(bottom.removeFromRight(100).reduced(info.ButtonMargin));
            }
            
	        if(contentComponent != nullptr)
                contentComponent->setBounds(b);

            repaint();
        }

        MultiPageDialog& parent;
        PositionInfo info;

        ErrorComponent modalError;

        Rectangle<int> modalBounds;

        ScopedPointer<PageBase> contentComponent;
        PageInfo::Ptr content;

	    TextButton okButton, cancelButton;
    };

    template <typename T> PageInfo& createModalPopup()
    {
	    auto p = PageInfo::createInfo<T>();
        popup.setContent(p);
        return *p;
    }

    void showModalPopup(bool addButtons)
    {
        PositionInfo info;

        if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
            info = laf->getMultiPagePositionInfo(popup.content->getData());

        popup.show(info, addButtons);
	    
    }

    Result checkCurrentPage()
    {
	    if(currentPage != nullptr)
	    {
            currentErrorElement = nullptr;

		    auto ok = currentPage->check(runThread->globalState);

            errorComponent.setError(ok);

            repaint();
            return ok;
	    }

        return Result::fail("No page");
    }

    void setCurrentErrorPage(PageBase* b)
    {
        currentErrorElement = b;
    }

private:

    var assets;

    bool currentNavigation = true;

    var properties;
    MarkdownLayout::StyleData styleData;
    PageInfo::List pages;

    TextButton cancelButton;
    TextButton nextButton;
    TextButton prevButton;

    RunThread* runThread;
    Viewport content;
    ScopedPointer<PageBase> currentPage;
    Rectangle<int> top, bottom, center;
    std::function<void()> finishCallback;
    Factory factory;

    PageBase* currentErrorElement = nullptr;
    ErrorComponent errorComponent;
    ModalPopup popup;
    ScrollbarFader sf;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(MultiPageDialog);
};

}
}