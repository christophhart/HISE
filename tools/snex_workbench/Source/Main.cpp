/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"

REGISTER_STATIC_DSP_LIBRARIES()
{
}

AudioProcessor* hise::StandaloneProcessor::createProcessor()
{
	return new hise::BackendProcessor(deviceManager, callback);
}

class HISEStandaloneApplication : public JUCEApplication
{
public:
	//==============================================================================
	HISEStandaloneApplication() {}

	const String getApplicationName() override { return ProjectInfo::projectName; }
	const String getApplicationVersion() override { return ProjectInfo::versionString; }
	bool moreThanOneInstanceAllowed() override { return true; }

	//==============================================================================
	void initialise(const String& commandLine) override
	{
		mainWindow = new MainWindow(commandLine);
		mainWindow->setUsingNativeTitleBar(true);
		mainWindow->toFront(true);
	}

	void shutdown() override
	{
		// Add your application's shutdown code here..
		mainWindow = nullptr; // (deletes our window)
	}

	//==============================================================================
	void systemRequestedQuit() override
	{
		// This is called when the app is being asked to quit: you can ignore this
		// request and let the app carry on running, or call quit() to allow the app to close.
		quit();
	}

	void anotherInstanceStarted(const String&) override
	{
		// When another instance of the app is launched while this one is running,
		// this method is invoked, and the commandLine parameter tells you what
		// the other instance's command-line arguments were.
	}

	//==============================================================================
	/*
		This class implements the desktop window that contains an instance of
		our MainContentComponent class.
	*/
	class MainWindow : public DocumentWindow
	{
	public:
		MainWindow(const String &commandLine) : DocumentWindow("SNEX Workbench",
			Colours::lightgrey,
			DocumentWindow::TitleBarButtons::closeButton | DocumentWindow::maximiseButton | DocumentWindow::TitleBarButtons::minimiseButton)
		{
			setContentOwned(standaloneProcessor.createEditor(), true);
			setResizable(true, true);
			setUsingNativeTitleBar(true);
			centreWithSize(getWidth(), getHeight() - 28);
			setVisible(true);
		}

		~MainWindow()
		{
			clearContentComponent();
			
			
		}

		void closeButtonPressed()
		{
			auto mw = dynamic_cast<SnexWorkbenchEditor*>(getContentComponent());
			mw->requestQuit();
			standaloneProcessor.requestQuit();
			
		}

		/* Note: Be careful if you override any DocumentWindow methods - the base
		   class uses a lot of them, so by overriding you might break its functionality.
		   It's best to do all your work in your content component instead, but if
		   you really have to override any DocumentWindow methods, make sure your
		   subclass also calls the superclass's method.
		*/

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow);

		hise::StandaloneProcessor standaloneProcessor;
	};

private:
	ScopedPointer<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (HISEStandaloneApplication)
