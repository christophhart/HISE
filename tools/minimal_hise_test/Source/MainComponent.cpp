/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"





using namespace hise;
using namespace scriptnode;
using namespace snex::Types;
using namespace snex::jit;
using namespace snex;

//==============================================================================
MainComponent::MainComponent():
  b("Run")
{
	startTimer(150);
	addAndMakeVisible(b);

	b.setClickingTogglesState(true);

	b.onClick = [&]()
	{
		if(b.getToggleState())
		{
			MelatoninPerfetto::get().beginSession();
		}
		else
		{
			MelatoninPerfetto::get().endSession();
			URL("https://ui.perfetto.dev").withFileToUpload("", MelatoninPerfetto::get().lastFile, "application/octet-stream").launchInDefaultBrowser();
		}
	};

#if JUCE_WINDOWS
    context.attachTo(*this);
	setSize (2560, 1080);
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
	g.fillAll(Colour(0xFF333336));
}

void MainComponent::resized()
{
	b.setBounds(getLocalBounds().removeFromTop(32));
}

void MainComponent::timerCallback()
{
	String s;
	s << "EVENT" << String(counter++);
	TRACE_DISPATCH(perfetto::DynamicString{s.toStdString()});
	

	Random r;

	for(int i = 0; i < 100000 + r.nextInt(100000); i++)
	{
		if(i % 1000000 == 0)
		{
			String s2;
			s2 << "SUBEVENT" << String(i);
			TRACE_DISPATCH(perfetto::DynamicString{s2.toStdString()});

			for(int j = 0; j < r.nextInt(10000000) + 10000000; j++)
			{
				hmath::sin(0.5);
			}
		}

		

		hmath::sin((float)i);
	}
}
