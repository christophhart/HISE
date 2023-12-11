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

#if !USE_BACKEND
#error "Only include with USE_BACKEND"
#endif

namespace hise {
using namespace juce;

struct PerfettoWebviewer::Dragger: public Component,
								   public juce::DragAndDropContainer
{
    Dragger()
    {
        Paths f;
        p = f.createPath("drag");
        setMouseCursor(MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity(true);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        if(fileToDrag.existsAsFile())
        {
            DragAndDropContainer::performExternalDragDropOfFiles ({fileToDrag.getFullPathName()}, false, this);
        }
    }
    
    void resized() override
    {
        Paths f;
        f.scalePath(p, getLocalBounds().removeFromLeft(getHeight()).reduced(10).toFloat());
    }
    
    void setFile(const File& f)
    {
        fileToDrag = f;
        repaint();
    }
    
    File fileToDrag;
    
    void paint(Graphics& g) override
    {
        auto ok = fileToDrag.existsAsFile();
        
        if(ok)
        {
            g.setColour(Colour(HISE_OK_COLOUR).withAlpha(0.4f));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(10.0f), 3.0f);
        }
        
        float alpha = ok ? 0.5f : 0.2f;
        if(ok && isMouseOver())
            alpha += 0.2f;
        if(ok && isMouseOverOrDragging() && isMouseButtonDown())
            alpha += 0.3f;
        
        
        
        g.setFont(GLOBAL_BOLD_FONT());
        g.setColour(Colours::white.withAlpha(alpha));
        g.drawText("Drag me down!", getLocalBounds().toFloat().reduced(20), Justification::right);
        g.fillPath(p);
    }
    Path p;
};

PerfettoWebviewer::PerfettoWebviewer(BackendRootWindow* unused):
	startButton("start", nullptr, f),
	cancelButton("cancel", nullptr, f)
{
#if JUCE_USE_WIN_WEBVIEW2
	WebView2Preferences pref;
	pref = pref.withStatusBarDisabled()
	           .withBuiltInErrorPageDisabled();

	addAndMakeVisible(browser = new WindowsWebView2WebBrowserComponent(true, pref));
	browser->goToURL("https://ui.perfetto.dev");
#elif (JUCE_MAC && USE_BACKEND) || JUCE_LINUX
    
    addAndMakeVisible(browser = new WebBrowserComponent(true));
    browser->goToURL("https://ui.perfetto.dev");
    
#else
    data = new hise::WebViewData(File());
    addAndMakeVisible(browser = new WebViewWrapper(data));

    browser->navigateToURL(URL("https://ui.perfetto.dev"));

    SafeAsyncCall::call<PerfettoWebviewer>(*this, [](PerfettoWebviewer& v)
    {
	    v.browser->refreshBounds(1.0f);
        v.resized();
    });
    
#endif

    dragger = new Dragger();
	addAndMakeVisible(dragger);
	addAndMakeVisible(startButton);
	addAndMakeVisible(cancelButton);
	startButton.setToggleModeWithColourChange(true);
        
#if PERFETTO
	startButton.onClick = [&]()
	{
		if(startButton.getToggleState())
		{
			MelatoninPerfetto::get().beginSession();
            MelatoninPerfetto::get().tempFile = new juce::TemporaryFile();
			if(onStart)
				onStart();
		}
		else
		{
			MelatoninPerfetto::get().endSession(true);
			dragger->setFile(MelatoninPerfetto::get().lastFile);
		}
	        
		repaint();
	};

	cancelButton.onClick = [&]()
	{
		if(startButton.getToggleState())
		{
			MelatoninPerfetto::get().endSession(false);
			dragger->setFile(File());
			startButton.setToggleStateAndUpdateIcon(false);
		}
	        
		repaint();
	};
#endif
}

PerfettoWebviewer::~PerfettoWebviewer()
{
    delete dragger;
    dragger = nullptr;
}

Path PerfettoWebviewer::Paths::createPath(const String& url) const
{
	Path p;
            
	LOAD_EPATH_IF_URL("start", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_EPATH_IF_URL("cancel", EditorIcons::cancelIcon);
	LOAD_EPATH_IF_URL("drag", EditorIcons::dragIcon);
            
	return p;
}

void PerfettoWebviewer::start(bool shouldStart)
{
	startButton.setToggleState(shouldStart, sendNotificationSync);
}

void PerfettoWebviewer::resized()
{
	auto area = getLocalBounds();
    
	auto top = area.removeFromTop(48);
	startButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(10));
	cancelButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(10));
	dragger->setBounds(top.removeFromRight(180));
	browser->setBounds(area);

#if !JUCE_USE_WIN_WEBVIEW2 && !JUCE_MAC && !JUCE_LINUX
	    browser->refreshBounds(1.0f);
#endif

#ifndef PERFETTO
	browser->setVisible(false);
#endif

}

void PerfettoWebviewer::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF19212b));

#ifndef PERFETTO
    g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("PERFETTO is diabled. Recompile HISE with the PERFETTO=1 compiler flag...", getLocalBounds().toFloat(), Justification::centred);
#endif

	if(startButton.getToggleState())
	{
		auto b = getLocalBounds().removeFromTop(48).toFloat();
		b.removeFromLeft(cancelButton.getRight() + 20);
		b.removeFromRight(dragger->getWidth() + 20);
	        
		g.setColour(Colour(HISE_WARNING_COLOUR));
		g.fillRoundedRectangle(b.reduced(10), 3.0f);
		g.setColour(Colours::black);
		g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));
		g.drawText("Profiling in process...", b, Justification::centred);
	}
}
} // hise
