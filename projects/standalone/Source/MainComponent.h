/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class FloatingTile;

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component
{
public:
    //==============================================================================
    MainContentComponent(const String &commandLine);

	void handleCommandLineArguments(const String& args)
	{
		if (args.isNotEmpty())
		{
			String presetFilename = args.trimCharactersAtEnd("\"").trimCharactersAtStart("\"");

            if(File::isAbsolutePath(presetFilename))
            {
                File presetFile(presetFilename);
                
                File projectDirectory = File(presetFile).getParentDirectory().getParentDirectory();
                BackendRootWindow* bpe = dynamic_cast<BackendRootWindow*>(editor.get());
                ModulatorSynthChain* mainSynthChain = bpe->getBackendProcessor()->getMainSynthChain();
                const File currentProjectFolder = GET_PROJECT_HANDLER(mainSynthChain).getWorkDirectory();
                
                if ((currentProjectFolder != projectDirectory) &&
                    PresetHandler::showYesNoWindow("Switch Project", "The file you are about to load is in a different project. Do you want to switch projects?", PresetHandler::IconType::Question))
                {
                    GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(projectDirectory, nullptr);
                }
                
                if (presetFile.getFileExtension() == ".hip")
                {
                    mainSynthChain->getMainController()->loadPreset(presetFile, editor);
                }
                else if (presetFile.getFileExtension() == ".xml")
                {
                    BackendCommandTarget::Actions::openFileFromXml(bpe, presetFile);
                }
            }
		}
	}

	~MainContentComponent();

    void paint (Graphics&);
    void resized();

private:

	

	ScopedPointer<AudioProcessorEditor> editor;
	ScopedPointer<StandaloneProcessor> standaloneProcessor;

	ScopedPointer<FloatingTile> root;

	OpenGLContext open;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
