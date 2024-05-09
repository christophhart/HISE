/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "Exporter.h"

class CommandLineActions
{
private:
    
    static void throwErrorAndQuit(const String& errorMessage)
    {
#if JUCE_DEBUG
        DBG(errorMessage);
        jassertfalse;
#else
        print("ERROR: " + errorMessage);
        exit(1);
#endif
    }
    
    static void print(const String& message)
    {
#if JUCE_DEBUG
        DBG(message);
#else
        std::cout << message << std::endl;
#endif
    }
    
    static StringArray getCommandLineArgs(const String& commandLine)
    {
        return StringArray::fromTokens(commandLine, true);
    }
    
    
    
	static String getArgument(const StringArray& args, const String& prefix)
	{
		for (auto arg : args)
		{
			if (arg.unquoted().startsWith(prefix))
				return arg.unquoted().fromFirstOccurrenceOf(prefix, false, false);
		}

		return {};
	}

	

	

	static File getFilePathArgument(const StringArray& args, const File& root = File())
	{
		auto s = getArgument(args, "-p:");

		if (s.isNotEmpty() && File::isAbsolutePath(s))
			return File(s);

		if (root.isDirectory())
		{
			auto rel = root.getChildFile(s);

			if (rel.existsAsFile())
				return rel;
		}

		throwErrorAndQuit("`" + s + "` is not a valid path");

#if JUCE_DEBUG
		return File();
#endif
	}

public:

    static void compileProject(const String& commandLine)
    {
	    auto args = getCommandLineArgs(commandLine);

        auto jsonFile = getArgument(args, "--export:").unquoted();
        auto hisePath = getArgument(args, "--hisepath:").unquoted();
        auto teamID = getArgument(args, "--teamid:").unquoted();
        
        
        if(teamID.contains("("))
            teamID = teamID.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false).trim();

        auto tlength = teamID.length();
        
        if(tlength != 0 && tlength != 10)
            throwErrorAndQuit("Team Developer ID is invalid");
        
        if(jsonFile.isNotEmpty() && hisePath.isEmpty())
            throwErrorAndQuit("You must supply the HISE path when building from the command line");
        
        if(File::isAbsolutePath(jsonFile) && File::isAbsolutePath(hisePath))
        {
	        File j(jsonFile);
            File hp(hisePath);

            if(!hp.isDirectory())
                throwErrorAndQuit("HISE path does not exist");

            if(!hp.getChildFile("hi_core").isDirectory())
                throwErrorAndQuit("HISE path is not valid");

            if(!j.existsAsFile())
                throwErrorAndQuit("JSON file does not exist");

            auto obj = JSON::parse(j.loadFileAsString());

            multipage::State state(obj, j.getParentDirectory());
            
            struct RunJob: public State::Job
            {
                RunJob(State& s):
                  Job(s, var())
                {};

                void setMessage(const String& message) override
                {
	                print(message);
                }

	            Result run() override { return Result::ok(); };
            };

            RunJob job(state);

            DynamicObject::Ptr so = new DynamicObject();
            so->setProperty("hisePath", hp.getFullPathName());
            so->setProperty("skipCompilation", true);
            
            if(teamID.isNotEmpty())
                so->setProperty("teamID", teamID);

            projucer_exporter pe(j.getParentDirectory(), state);
            pe.exportObj = obj;
            auto returnCode = (int)pe.exportProjucerProject(job, var(so.get()));

            exit(returnCode);
        }
    }

	static void printHelp()
	{
		print("");
		print("HISE Multipage Creator");
        print("----------------------");
        print("");
        print("Usage: multipagecreator.exe --export:JSON_FILE [--hisepath:HISE_PATH] [--teamid::APPLE_DEV_ID]");
        print("");
        print("Exports the JSON File as a binary application");

		exit(0);
	}
};

//==============================================================================
class jit_playgroundApplication  : public JUCEApplication
{
public:
    //==============================================================================
    jit_playgroundApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        if(commandLine.isNotEmpty())
        {
	        if(commandLine.startsWith("--help"))
            {
                CommandLineActions::printHelp();
                return;
            }

            if(commandLine.startsWith("--export"))
            {
	            CommandLineActions::compileProject(commandLine);
                return;
            }
        }
        
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
		
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow (String name)  : DocumentWindow (name,
                                                    Desktop::getInstance().getDefaultLookAndFeel()
                                                                          .findColour (ResizableWindow::backgroundColourId),
                                                    DocumentWindow::allButtons)
        {
            UnitTestRunner r;
			r.runTestsInCategory("ui");

            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
           #endif
            setVisible (true);

            
        }

        void closeButtonPressed() override
        {
            //dynamic_cast<MainComponent*>(getContentComponent())->checkSave();
            
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};



//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (jit_playgroundApplication)

