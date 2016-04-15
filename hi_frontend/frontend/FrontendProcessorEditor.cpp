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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


FrontendProcessorEditor::FrontendProcessorEditor(FrontendProcessor *fp) :
AudioProcessorEditor(fp)
{

	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(fp->getMainSynthChain(), nullptr));
	interfaceComponent->checkInterfaces();

	interfaceComponent->setCurrentContent(0, dontSendNotification);
	interfaceComponent->refreshContentBounds();
	interfaceComponent->setIsFrontendContainer(true);

#if INCLUDE_BAR

	addAndMakeVisible(mainBar = new FrontendBar(fp));

#endif

	addAndMakeVisible(keyboard = new CustomKeyboard(fp->getKeyboardState()));

    keyboard->setAvailableRange(fp->getKeyboardState().getLowestKeyToDisplay(), 127);
    
	

	

	addAndMakeVisible(aboutPage = new AboutPage());
	aboutPage->setVisible(false);

	addAndMakeVisible(deactiveOverlay = new DeactiveOverlay());

	deactiveOverlay->setState(DeactiveOverlay::SamplesNotFound, !ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory());

#if USE_COPY_PROTECTION
	deactiveOverlay->setState(DeactiveOverlay::LicenceNotFound, !fp->unlocker.isUnlocked());
#endif

	setSize(interfaceComponent->getContentWidth(), (mainBar != nullptr ? mainBar->getHeight() : 0) + interfaceComponent->getContentHeight() + 72);

	startTimer(4125);

}

void FrontendProcessorEditor::resized()
{
	int y = 0;
	
	if (mainBar != nullptr)
	{
		mainBar->setBounds(0, y, getWidth(), mainBar->getHeight());
		y += mainBar->getHeight();
	}

	interfaceComponent->setBounds(0, y, getWidth(), interfaceComponent->getContentHeight());

	y = interfaceComponent->getBottom();

	keyboard->setBounds(0, y, getWidth(), 72);

	aboutPage->setBoundsInset(BorderSize<int>(80));
	deactiveOverlay->setBounds(getLocalBounds());
}

void DeactiveOverlay::buttonClicked(Button *b)
{
	if (b == resolveLicenceButton)
	{
#if USE_COPY_PROTECTION
		FileChooser fc("Load Licence key file", File::nonexistent, "*.licence", true);

		if (fc.browseForFileToOpen())
		{
			File f = fc.getResult();

			f.copyFileTo(ProjectHandler::Frontend::getLicenceKey());

			String keyContent = f.loadFileAsString();

			Unlocker *ul = &dynamic_cast<FrontendProcessor*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor())->unlocker;

			ul->applyKeyFile(keyContent);

			setState(LicenceNotFound, !ul->isUnlocked());
		}
#endif
	}
	else if (b == resolveSamplesButton)
	{
		FileChooser fc("Select Sample Location", File::nonexistent, "*.*", true);

		if (fc.browseForDirectory())
		{
			ProjectHandler::Frontend::setSampleLocation(fc.getResult());

			setState(SamplesNotFound, !ProjectHandler::Frontend::getSampleLocationForCompiledPlugin().isDirectory());
		}
	}
}
