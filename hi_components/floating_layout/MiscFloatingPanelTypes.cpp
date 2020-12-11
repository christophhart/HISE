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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

void EmptyComponent::paint(Graphics& g)
{
	g.fillAll(c);
}

void EmptyComponent::mouseDown(const MouseEvent& event)
{
	getParentShell()->mouseDown(event);
}

#if USE_BACKEND
void ApplicationCommandButtonPanel::setCommand(int commandID)
{
	Path p = BackendCommandIcons::getIcon(commandID);

	b->setCommandToTrigger(getParentShell()->getMainController()->getCommandManager(), commandID, true);
	b->setShape(p, false, true, true);
	b->setVisible(true);
}
#endif


void SliderPackPanel::resized()
{
	PanelWithProcessorConnection::resized();

	if (auto sp = getContent<SliderPack>())
	{
		int numSliders = sp->getNumSliders();

		int wPerSlider = getWidth() / numSliders;

		int newWidth = numSliders * wPerSlider;

		int y = sp->getY();
		int height = sp->getHeight();

		sp->setBounds((getWidth() - newWidth) / 2, y, newWidth, height);
	}
}

void SpacerPanel::paint(Graphics& g)
{
	g.setColour(findPanelColour(FloatingTileContent::PanelColourId::bgColour));
	g.fillRect(getParentShell()->getContentBounds());
}

VisibilityToggleBar::VisibilityToggleBar(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setDefaultPanelColour(PanelColourId::bgColour, HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));

	setControlledContainer(getParentShell()->getParentContainer());
}

void VisibilityToggleBar::setControlledContainer(FloatingTileContainer* containerToControl)
{
	controlledContainer = dynamic_cast<Component*>(containerToControl);
	refreshButtons();
}

void VisibilityToggleBar::addIcon(FloatingTile* ft)
{
	if (ft == getParentShell()) // don't show this, obviously
		return;

	if (ft->isEmpty())
		return;

	if (dynamic_cast<SpacerPanel*>(ft->getCurrentFloatingPanel()))
		return;

	auto icon = new Icon(ft);

	addAndMakeVisible(icon);

	buttons.add(icon);
}



void VisibilityToggleBar::refreshButtons()
{
	buttons.clear();

	if (customPanels.isEmpty())
	{
		if (controlledContainer.getComponent() != nullptr)
		{
			auto c = dynamic_cast<FloatingTileContainer*>(controlledContainer.getComponent());

			for (int i = 0; i < c->getNumComponents(); i++)
			{
				addIcon(c->getComponent(i));
			}

			
			resized();
		}
	}
	else
	{
		for (int i = 0; i < customPanels.size(); i++)
		{
			if (customPanels[i].getComponent() != nullptr)
			{
				addIcon(customPanels[i]);
			}
			else
				customPanels.remove(i--);
		}

		resized();
	}

	
}

var VisibilityToggleBar::toDynamicObject() const
{
	var obj = FloatingTileContent::toDynamicObject();

	Array<var> iconList;

	for (int i = 0; i < customPanels.size(); i++)
	{
		if (customPanels[i].getComponent() != nullptr)
		{
			iconList.add(customPanels[i]->getLayoutData().getID().toString());
		}
		else
		{
			jassertfalse;
		}
	}

	


	storePropertyInObject(obj, SpecialPanelIds::IconIds, iconList);
	storePropertyInObject(obj, SpecialPanelIds::Alignment, var(alignment.getFlags()));

	return obj;
}

void VisibilityToggleBar::fromDynamicObject(const var& object)
{
	FloatingTileContent::fromDynamicObject(object);

	alignment = Justification(getPropertyWithDefault(object, SpecialPanelIds::Alignment));

	auto l = getPropertyWithDefault(object, SpecialPanelIds::IconIds);

	if (auto list = l.getArray())
	{
		if (!list->isEmpty())
		{
			for (int i = 0; i < list->size(); i++)
			{
				const String name = list->getUnchecked(i).toString();

				// The other panels do not exist yet, so store them in a StringArray and add them on sibling change
				pendingCustomPanels.add(name);

			}
		}
	}

}

void VisibilityToggleBar::siblingAmountChanged()
{
	if (!pendingCustomPanels.isEmpty())
	{
		customPanels.clear();

		auto rootPanel = getParentShell()->getParentContainer();

		jassert(rootPanel != nullptr);

		for (int i = 0; i < pendingCustomPanels.size(); i++)
		{
			auto name = pendingCustomPanels[i];

			auto panel = FloatingTileHelpers::findTileWithId<FloatingTileContent>(rootPanel->getParentShell(), Identifier(name));

			if (panel != nullptr)
			{
				addCustomPanel(panel->getParentShell());
			}
			else
			{
				// Delete all buttons

				customPanels.clear();
				refreshButtons();
				return;
			}
		}

		pendingCustomPanels.clear();
	}

	refreshButtons();
}

void VisibilityToggleBar::resized()
{
	auto c = dynamic_cast<ResizableFloatingTileContainer*>(getParentShell()->getParentContainer());

	bool arrangeHorizontal = true;

	if (c != nullptr && !c->isVertical())
		arrangeHorizontal = false;

	int buttonSize = arrangeHorizontal ? getHeight() : getWidth();

	buttonSize = jmin<int>(buttonSize, 40);

	const int totalSize = arrangeHorizontal ? getWidth() : getHeight();

	const int totalButtonSize = buttons.size() * buttonSize;

	int offset = alignment == Justification::centred ? (totalSize-totalButtonSize)/2 :  0;

	if (getParentShell()->getParentContainer())
	{
		for (int i = 0; i < buttons.size(); i++)
		{
			if (arrangeHorizontal)
				buttons[i]->setBounds(offset, 0, buttonSize, buttonSize);
			else
				buttons[i]->setBounds(0, offset, buttonSize, buttonSize);

			offset += buttonSize + 5;
		}
	}
}

VisibilityToggleBar::Icon::Icon(FloatingTile* controlledTile_) :
	controlledTile(controlledTile_)
{
	addAndMakeVisible(button = new ShapeButton("button", colourOff, overColourOff, downColourOff));

	

	if (controlledTile.getComponent() != nullptr)
	{
		on = controlledTile->getLayoutData().isVisible();

		button->setShape(controlledTile->getIcon(), false, true, true);
	}

	refreshColour();

	button->addListener(this);
}

void VisibilityToggleBar::Icon::refreshColour()
{
	if (controlledTile.getComponent() != nullptr)
		button->setTooltip((on ? "Hide " : "Show ") + controlledTile->getCurrentFloatingPanel()->getBestTitle());

	if (on)
		button->setColours(colourOn, overColourOn, downColourOn);
	else
		button->setColours(colourOff, overColourOff, downColourOff);
}

void VisibilityToggleBar::Icon::buttonClicked(Button*)
{
	on = !controlledTile->getLayoutData().isVisible();

	controlledTile->getLayoutData().setVisible(on);
	controlledTile->getParentContainer()->refreshLayout();

	refreshColour();
}

void PopoutButtonPanel::buttonClicked(Button* /*b*/)
{
	ScopedPointer<FloatingTile> popout = new FloatingTile(getMainController(), nullptr, popoutData);

	popout->setSize(width, height);

	popout->setName(popout->getLayoutData().getID().toString());

	auto p = Point<int>(button->getLocalBounds().getCentreX(), button->getLocalBounds().getBottom());

	getParentShell()->showComponentInRootPopup(popout.release(), button, p);
}

void PopoutButtonPanel::resized()
{
	button->setBounds(getParentShell()->getContentBounds());
}


InterfaceContentPanel::InterfaceContentPanel(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	setOpaque(true);

	if (!connectToScript())
	{
		addAndMakeVisible(refreshButton = new TextButton("Refresh"));
		refreshButton->setLookAndFeel(&laf);
		refreshButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
		refreshButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);
		refreshButton->addListener(this);
	}

	dynamic_cast<GlobalSettingManager*>(getMainController())->addScaleFactorListener(this);
	getMainController()->addScriptListener(this);
	getMainController()->getLockFreeDispatcher().addPresetLoadListener(this);
	getMainController()->getExpansionHandler().addListener(this);
}


InterfaceContentPanel::~InterfaceContentPanel()
{
	dynamic_cast<GlobalSettingManager*>(getMainController())->removeScaleFactorListener(this);
	getMainController()->removeScriptListener(this);
	getMainController()->getLockFreeDispatcher().removePresetLoadListener(this);
	getMainController()->getExpansionHandler().removeListener(this);

	content = nullptr;
}

void InterfaceContentPanel::paint(Graphics& g)
{
	g.fillAll(Colours::black);

	if (content == nullptr)
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);
		g.drawText("No interface found", getLocalBounds().toFloat().removeFromTop(40.0f), Justification::centred);
	}
}

void InterfaceContentPanel::resized()
{
	if (content != nullptr)
	{
		content->setBounds(getLocalBounds());
	}
	else if (refreshButton != nullptr)
	{
		refreshButton->centreWithSize(200, 30);
	}
}

void InterfaceContentPanel::newHisePresetLoaded()
{
	
	content = nullptr;
	connectToScript();

	resized();
}

void InterfaceContentPanel::expansionPackLoaded(Expansion* currentExpansion)
{
#if USE_BACKEND
	Component::SafePointer<InterfaceContentPanel> safeThis(this);

	MessageManager::callAsync([safeThis]()
	{
		if (safeThis.getComponent() != nullptr)
		{
			safeThis.getComponent()->content = nullptr;
			safeThis.getComponent()->resized();
		}
	});
#endif
}

void InterfaceContentPanel::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (processor == dynamic_cast<JavascriptProcessor*>(connectedProcessor.get()))
	{
		updateSize();

		
	}
}


void InterfaceContentPanel::buttonClicked(Button* /*b*/)
{
	connectToScript();
	resized();
}


void InterfaceContentPanel::scaleFactorChanged(float /*newScaleFactor*/)
{
	updateSize();
}

bool InterfaceContentPanel::connectToScript()
{
	if (content != nullptr)
		return true;

	if (auto jsp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getMainController()))
	{
		addAndMakeVisible(content = new ScriptContentComponent(jsp));
		connectedProcessor = jsp;
		
		if (refreshButton != nullptr)
		{
			refreshButton->setVisible(false);
		}

		updateSize();

		repaint();

		return true;
	}
	else
	{
		return false;
	}
}

void InterfaceContentPanel::updateSize()
{

#if USE_BACKEND
	if (auto pwsc = dynamic_cast<ProcessorWithScriptingContent*>(connectedProcessor.get()))
	{
		auto c = pwsc->getScriptingContent();

		if (c != nullptr)
		{
			auto topLevel = findParentComponentOfClass<FloatingTileDocumentWindow>();

			if (topLevel != nullptr)
			{
				topLevel->setName("Preview: " + connectedProcessor->getId());

				auto scaleFactor = dynamic_cast<GlobalSettingManager*>(connectedProcessor->getMainController())->getGlobalScaleFactor();

				setTransform(AffineTransform::scale((float)scaleFactor));
				
				topLevel->setSize((int)((float)c->getContentWidth() * scaleFactor), (int)((float)c->getContentHeight() * scaleFactor));
			}

			getParentShell()->setVital(true);
		}
	}
#endif
}


ActivationWindow::ActivationWindow(FloatingTile* parent) :
	FloatingTileContent(parent),
	mc(parent->getMainController())
{
	addAndMakeVisible(productKey = new Label());
	addAndMakeVisible(statusLabel = new Label());
	addAndMakeVisible(submitButton = new TextButton("Deactivate Computer"));
	submitButton->addListener(this);
	submitButton->setLookAndFeel(&tblaf);
	submitButton->setColour(TextButton::ColourIds::textColourOnId, Colours::white);
	submitButton->setColour(TextButton::ColourIds::textColourOffId, Colours::white);

#if USE_TURBO_ACTIVATE
	const String pKey = String(dynamic_cast<FrontendProcessor*>(mc)->unlocker.getProductKey());
#else
	const String pKey = "1234-1234-1234-1234";
#endif

	productKey->setFont(GLOBAL_MONOSPACE_FONT());
	productKey->setText(pKey, dontSendNotification);
	productKey->setJustificationType(Justification::centred);
	productKey->setColour(Label::ColourIds::backgroundColourId, Colours::white);

	statusLabel->setJustificationType(Justification::centred);
	statusLabel->setEditable(false, false, false);

	refreshStatusLabel();

	setSize(300, 150);
	startTimer(3000);
}

void ActivationWindow::buttonClicked(Button*)
{
	if (PresetHandler::showYesNoWindow("Deactivate this computer", "Do you really want to deactivate this computer?", PresetHandler::IconType::Question))
	{
#if USE_TURBO_ACTIVATE
		TurboActivateUnlocker* ul = &dynamic_cast<FrontendProcessor*>(mc)->unlocker;

		ul->deactivateThisComputer();
		refreshStatusLabel();

		const bool noInternet = ul->unlockState == TurboActivateUnlocker::State::NoInternet;

		if (noInternet)
		{
			if (PresetHandler::showYesNoWindow("Deactivate using request file", "Do you want to deactivate this computer using a offline request file"))
			{
				FileChooser fc("Create Deactivation Request file", File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory), "*.xml", true);

				if (fc.browseForFileToSave(true))
				{
					File f = fc.getResult();

#if JUCE_WINDOWS
					TurboActivateCharPointerType path = f.getFullPathName().toUTF16().getAddress();
#else
					TurboActivateCharPointerType path = f.getFullPathName().toUTF8().getAddress();
#endif
					ul->deactivateWithFile(path);


					if (ul->unlockState == TurboActivateUnlocker::State::Deactivated)
					{
						PresetHandler::showMessageWindow("Deactivation request file created", "Submit this file to customer support to complete the deactivation procedure for this machine");
					}

					refreshStatusLabel();
				}
			}
		}

		auto x = findParentComponentOfClass<FrontendProcessorEditor>();

		if (x != nullptr && !good)
		{
			addOverlayListener(x);
			sendOverlayMessage(DeactiveOverlay::State::CopyProtectionError);
			dynamic_cast<AudioProcessor*>(mc)->suspendProcessing(true);

			getParentComponent()->setVisible(false);
		}
#endif
	}
}

void ActivationWindow::refreshStatusLabel()
{
#if USE_TURBO_ACTIVATE
	auto state = dynamic_cast<FrontendProcessor*>(mc)->unlocker.unlockState;

	String message;
	good = true;

	switch (state)
	{
	case TurboActivateUnlocker::State::Activated:
		message = "Activation Successful";
		good = true;
		break;
	case TurboActivateUnlocker::State::ActivatedButFailedToConnect:
		message = "Activation Successful";
		good = true;
		break;
	case TurboActivateUnlocker::State::Deactivated:
		message = "Deactivated";
		good = false;
		break;
	case TurboActivateUnlocker::State::TrialExpired:
		message = "Trial expired";
		good = false;
		break;
	case TurboActivateUnlocker::State::Trial:
		message = "Trial period";
		good = true;
		break;
	case TurboActivateUnlocker::State::Invalid:
		message = "Invalid Product Key";
		good = false;
		break;
	case TurboActivateUnlocker::State::KeyFileFailedToOpen:
		message = "License file not found";
		good = false;
		break;
	case TurboActivateUnlocker::State::NoInternet:
		message = "Can't deactivate when offline";
		good = true;
		break;
	case TurboActivateUnlocker::State::numStates:
		break;
	default:
		break;
	}

	auto key = dynamic_cast<FrontendProcessor*>(mc)->unlocker.getProductKey();
#else
	good = true;
	const String key = "1234 1234 1234";
	const String message = "Dummy mode";
#endif

	productKey->setText(key, dontSendNotification);

	statusLabel->setColour(Label::backgroundColourId, (good ? Colour(0xFF168000) : Colour(0xFFE90303)));
	statusLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	statusLabel->setText(message, dontSendNotification);

}

juce::Identifier PlotterPanel::getProcessorTypeId() const
{
	return LfoModulator::getClassType();
}

} // namespace hise