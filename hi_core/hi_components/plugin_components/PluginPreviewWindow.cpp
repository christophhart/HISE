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


PluginPreviewWindow::PluginPreviewWindow(BackendProcessorEditor *editor_) :
DocumentWindow("Preview: : " + editor_->getMainSynthChain()->getId(), Colours::black, DocumentWindow::closeButton, true),
editor(editor_)
{

	setContentOwned(new Content(editor), true);

	setUsingNativeTitleBar(true);


	centreWithSize(getContentComponent()->getWidth(), getContentComponent()->getHeight());

	setResizable(false, false);

	setVisible(true);
}


void PluginPreviewWindow::closeButtonPressed()
{
	if (editor.getComponent() != nullptr)
	{
		editor->setPluginPreviewWindow(nullptr);
	}
}

PluginPreviewWindow::Content::Content(BackendProcessorEditor *editor_) :
editor(editor_),
mainSynthChain(editor->getMainSynthChain())
{

	setName("Interface Preview");

	setOpaque(true);

	addAndMakeVisible(container = new ScriptContentContainer(mainSynthChain));
	addAndMakeVisible(frontendBar = new DefaultFrontendBar(editor->getBackendProcessor()));

	container->checkInterfaces();
	container->setIsFrontendContainer(true);
	container->setIsFrontendContainer(true);

    
    DynamicObject::Ptr toolbarSettings = editor->getMainSynthChain()->getMainController()->getToolbarPropertiesObject();
    
    const bool showKeyboard = (bool)toolbarSettings->hasProperty("keyboard") ? (bool)toolbarSettings->getProperty("keyboard") : true;
    
    addAndMakeVisible(keyboard = new CustomKeyboard(editor->getBackendProcessor()->getKeyboardState()));
    keyboard->setAvailableRange(editor->getBackendProcessor()->getKeyboardState().getLowestKeyToDisplay(), 127);
    
    keyboard->setVisible(showKeyboard);
    

	const int xDelta = 2;

#if HISE_IOS

	frontendBar->setVisible(false);
	keyboard->setVisible(false);

	setSize(container->getContentWidth(), container->getContentHeight());
#else
    
    setSize(container->getContentWidth() + xDelta, container->getContentHeight() + (frontendBar->isOverlaying() ? 0 : frontendBar->getHeight()) + (showKeyboard ? (72 + xDelta) : 0));
#endif
}

PluginPreviewWindow::Content::~Content()
{
	frontendBar = nullptr;
	container = nullptr;
	keyboard = nullptr;

	editor = nullptr;
}

void PluginPreviewWindow::Content::resized()
{
#if HISE_IOS
	container->setBounds(0, 0, container->getContentWidth(), container->getContentHeight());
#else
	frontendBar->setBounds(2, 2, container->getContentWidth(), frontendBar->getHeight());
	container->setBounds(2, (frontendBar->isOverlaying() ? frontendBar->getY() : frontendBar->getBottom()), container->getContentWidth(), container->getContentHeight());
	keyboard->setBounds(2, container->getBottom(), container->getContentWidth(), 72);
#endif
}
