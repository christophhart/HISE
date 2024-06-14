
#include <JuceHeader.h>
#include "Dialog.h"

//==============================================================================
class MainWrapper: public JUCEApplication
{
public:
    //==============================================================================
    MainWrapper() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
	{
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override {}

    class MainWindow    : public DocumentWindow,
						  public hise::multipage::ComponentWithSideTab
    {
    public:
        MainWindow (String name)  : DocumentWindow (name, Colour(0xFF333333), DocumentWindow::closeButton | DocumentWindow::minimiseButton)
        {
            setUsingNativeTitleBar (true);
            auto nc = new DialogClass();
			nc->setOnCloseFunction(BIND_MEMBER_FUNCTION_0(MainWindow::closeButtonPressed));

            state = &nc->state;

            setContentOwned (nc, true);
            setResizable(false, false);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

		void addCodeEditor(const var& infoObject, const Identifier& codeId) override {}

        multipage::State* getMainState() override { return state; }

		bool setSideTab(multipage::State* dialogState, multipage::Dialog* newDialog) override
        {
	        delete dialogState;
            delete newDialog;
            return false;
        }

		void refreshDialog() override {};

    private:

        multipage::State* state;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (MainWrapper)

