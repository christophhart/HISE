/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#define S(x) String(x, 2)


#include "MainComponent.h"

// Use this to quickly scale the window
#define SCALE_2 0




//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine)
{
	standaloneProcessor = new hise::StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	setSize(editor->getWidth(), editor->getHeight());

	handleCommandLineArguments(commandLine);
}

void MainContentComponent::handleCommandLineArguments(const String& args)
{
	if (args.isNotEmpty())
	{
		String presetFilename = args.trimCharactersAtEnd("\"").trimCharactersAtStart("\"");

		if (File::isAbsolutePath(presetFilename))
		{
			File presetFile(presetFilename);


			if (presetFile.getFileExtension() == ".hiseproject")
			{
				if (!presetFile.existsAsFile())
					return;

				auto newFolder = presetFile.getParentDirectory().getChildFile(presetFile.getFileNameWithoutExtension());
				
				if (newFolder.isDirectory())
				{
					PresetHandler::showMessageWindow("The folder already exists", "The directory " + newFolder.getFullPathName() + " already exists. Aborting project extraction.");
					return;
				}

				newFolder.createDirectory();

				auto bpe = dynamic_cast<hise::BackendRootWindow*>(editor.get());

				bpe->setProjectIsBeingExtracted();

				auto f = [bpe, newFolder, presetFile]()
				{
					BackendCommandTarget::Actions::extractProject(bpe, newFolder, presetFile);
				};

				MessageManager::callAsync(f);

				return;
			}

			File projectDirectory = File(presetFile).getParentDirectory().getParentDirectory();
			auto bpe = dynamic_cast<hise::BackendRootWindow*>(editor.get());
			auto mainSynthChain = bpe->getBackendProcessor()->getMainSynthChain();
			const File currentProjectFolder = GET_PROJECT_HANDLER(mainSynthChain).getWorkDirectory();

			if ((currentProjectFolder != projectDirectory) &&
				hise::PresetHandler::showYesNoWindow("Switch Project", "The file you are about to load is in a different project. Do you want to switch projects?", hise::PresetHandler::IconType::Question))
			{
				GET_PROJECT_HANDLER(mainSynthChain).setWorkingProject(projectDirectory);
			}

			if (presetFile.getFileExtension() == ".hip")
			{
				mainSynthChain->getMainController()->loadPresetFromFile(presetFile, editor);
			}
			else if (presetFile.getFileExtension() == ".xml")
			{
				hise::BackendCommandTarget::Actions::openFileFromXml(bpe, presetFile);
			}
		}
	}
}

MainContentComponent::~MainContentComponent()
{
	
	root = nullptr;
	editor = nullptr;

	standaloneProcessor = nullptr;
}

void MainContentComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
}

void MainContentComponent::resized()
{

#if SCALE_2
	editor->setSize(getWidth()*2, getHeight()*2);
#else
    editor->setSize(getWidth(), getHeight());
#endif

}

void MainContentComponent::requestQuit()
{
	standaloneProcessor->requestQuit();
}
