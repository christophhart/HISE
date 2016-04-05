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


Component *MidiKeyboardFocusTraverser::getDefaultComponent(Component *parentComponent)
{
	if (FrontendProcessorEditor *editor = dynamic_cast<FrontendProcessorEditor*>(parentComponent))
	{
		return editor->getKeyboard();
	}

	return nullptr;
}

FrontendProcessorEditor::FrontendProcessorEditor(FrontendProcessor *fp) :
AudioProcessorEditor(fp)
{

	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(fp->getMainSynthChain(), nullptr));
	interfaceComponent->checkInterfaces();



	interfaceComponent->setCurrentContent(0, dontSendNotification);
	interfaceComponent->refreshContentBounds();
	interfaceComponent->setIsFrontendContainer(true);
	interfaceComponent->setBounds(0, 0, interfaceComponent->getContentWidth(), interfaceComponent->getContentHeight());

#if INCLUDE_BAR

	int barHeight = 24;
	addAndMakeVisible(mainBar = new FrontendBar(fp));
	mainBar->setBounds(0, 0, interfaceComponent->getWidth(), barHeight);
	mainBar->startTimer(50);

#else

	int barHeight = 0;

#endif

	interfaceComponent->setTopLeftPosition(0, barHeight);


	addAndMakeVisible(keyboard = new CustomKeyboard(fp->keyboardState));

	keyboard->setBounds(0, interfaceComponent->getBottom(), interfaceComponent->getWidth(), 72);

    keyboard->setAvailableRange(fp->getKeyboardState().getLowestKeyToDisplay(), 127);
    

	if (!fp->samplesCorrectlyLoaded || !fp->keyFileCorrectlyLoaded)
	{
		interfaceComponent->setVisible(false);
	}

	setSize(interfaceComponent->getContentWidth(), barHeight + interfaceComponent->getHeight() + 72);

	startTimer(4125);

	addAndMakeVisible(aboutPage = new AboutPage());

	aboutPage->setVisible(false);

	aboutPage->setBoundsInset(BorderSize<int>(80));

#if USE_COPY_PROTECTION
	aboutPage->setUserEmail(fp->unlocker.getUserEmail());
#endif
}
