/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"



//==============================================================================
MainComponent::MainComponent():
  Thread("Unit Test thread"),
  startButton("start", nullptr, f),
  cancelButton("cancel", nullptr, f)
{
#if USE_JUCE_WEBVIEW2_BROWSER
    WebView2Preferences pref;

    pref = pref.withStatusBarDisabled()
               .withBuiltInErrorPageDisabled();

    addAndMakeVisible(browser = new WindowsWebView2WebBrowserComponent(true, pref));
    browser->goToURL("https://ui.perfetto.dev");
#else

    data = new hise::WebViewData(File());
    addAndMakeVisible(browser = new WebViewWrapper(data));

    Timer::callAfterDelay(100, [&]()
    {
        browser->navigateToURL(URL("https://ui.perfetto.dev"));
        browser->refreshBounds(1.0f);
        resized();
    });
#endif

	

    
    
    //r.runTestsInCategory("dispatch");
    //addAndMakeVisible(webview);
    addAndMakeVisible(dragger);
    addAndMakeVisible(startButton);
    addAndMakeVisible(cancelButton);
    
    startButton.setToggleModeWithColourChange(true);
    
	startTimer(150);
	

#if PERFETTO
	startButton.onClick = [&]()
	{
		if(startButton.getToggleState())
		{
			MelatoninPerfetto::get().beginSession();
            startThread(7);
		}
		else
		{
			MelatoninPerfetto::get().endSession(true);
            dragger.setFile(MelatoninPerfetto::get().lastFile);
		}
        
        repaint();
	};
#endif
    
#if PERFETTO
    cancelButton.onClick = [&]()
    {
        if(startButton.getToggleState())
        {
            MelatoninPerfetto::get().endSession(false);
            dragger.setFile(File());
            startButton.setToggleStateAndUpdateIcon(false);
        }
        
        repaint();
    };
#endif

#if JUCE_WINDOWS
    context.attachTo(*this);
    setSize(1440, 900);
	//setSize (2560, 1080);
#else
	setSize(1440, 900);
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
	g.fillAll(Colour(0xFF19212b));
    
    if(startButton.getToggleState())
    {
        auto b = getLocalBounds().removeFromTop(48).toFloat();
        b.removeFromLeft(cancelButton.getRight() + 20);
        b.removeFromRight(dragger.getWidth() + 20);
        
        g.setColour(Colour(HISE_WARNING_COLOUR));
        g.fillRoundedRectangle(b.reduced(10), 3.0f);
        g.setColour(Colours::black);
        g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));
        g.drawText("Profiling in process...", b, Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    auto top = area.removeFromTop(48);
	startButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(10));
    cancelButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(10));
    dragger.setBounds(top.removeFromRight(180));
    browser->setBounds(area);

#if !USE_JUCE_WEBVIEW2_BROWSER
    browser->refreshBounds(1.0f);
#endif
    
    
}

void MainComponent::timerCallback()
{
#if 0
    TRACE_DISPATCH(perfetto::DynamicString(String(counter++).toStdString()));
    
	Random r;

	for(int i = 0; i < 100000 + r.nextInt(1000); i++)
	{
		if(i % 10000 == 0)
		{
			String s2;
			s2 << "SUBEVENT" << String(i);
            TRACE_DISPATCH("asdsa");

			for(int j = 0; j < r.nextInt(10000) + 100000; j++)
			{
				hmath::sin(0.5);
			}
		}

		

		hmath::sin((float)i);
	}
#endif
}



