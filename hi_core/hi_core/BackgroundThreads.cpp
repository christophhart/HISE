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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


void QuasiModalComponent::setModalBaseWindowComponent(Component * childComponentOfModalBaseWindow, int fadeInTime)
{
	ModalBaseWindow *editor = dynamic_cast<ModalBaseWindow*>(childComponentOfModalBaseWindow);

	if (editor == nullptr) editor = childComponentOfModalBaseWindow->findParentComponentOfClass<ModalBaseWindow>();

	jassert(editor != nullptr);

	if (editor != nullptr)
	{
		editor->setModalComponent(dynamic_cast<Component*>(this), fadeInTime);

		isQuasiModal = true;
	}
}

void QuasiModalComponent::showOnDesktop()
{
	Component *t = dynamic_cast<Component*>(this);

	isQuasiModal = false;
	t->setVisible(true);
	t->setOpaque(true);
	t->addToDesktop(ComponentPeer::windowHasCloseButton);
}

void QuasiModalComponent::destroy()
{
	Component *t = dynamic_cast<Component*>(this);

	if (isQuasiModal)
	{
		t->findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
	}
	else
	{
		t->removeFromDesktop();
		delete this;
	}
}


DialogWindowWithBackgroundThread::DialogWindowWithBackgroundThread(const String &title, bool synchronous_) :
AlertWindow(title, String(), AlertWindow::AlertIconType::NoIcon),
progress(0.0),
isQuasiModal(false),
synchronous(synchronous_)
{
	setLookAndFeel(&laf);

	setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	setColour(AlertWindow::textColourId, Colours::white);

}

DialogWindowWithBackgroundThread::~DialogWindowWithBackgroundThread()
{
	if (thread != nullptr)
	{
		thread->stopThread(6000);
	}
}

void DialogWindowWithBackgroundThread::addBasicComponents(bool addOKButton)
{
	addTextEditor("state", "", "Status", false);

	getTextEditor("state")->setReadOnly(true);

	addProgressBarComponent(progress);
	
	if (addOKButton)
	{
		addButton("OK", 1, KeyPress(KeyPress::returnKey));
	}
	
	addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

}

bool DialogWindowWithBackgroundThread::threadShouldExit() const
{
	if (thread != nullptr)
	{
		return thread->threadShouldExit();
	};

	return false;
}

void DialogWindowWithBackgroundThread::handleAsyncUpdate()
{
	threadFinished();
	destroy();
}

void DialogWindowWithBackgroundThread::buttonClicked(Button* b)
{
	if (b->getName() == "OK")
	{
		if (synchronous)
		{
			runSynchronous();
		}
		else if (thread == nullptr)
		{
			thread = new LoadingThread(this);
			thread->startThread();
		}
		
	}
	else if (b->getName() == "Cancel")
	{
		if (thread != nullptr)
		{
			thread->signalThreadShouldExit();
			thread->notify();
			destroy();
		}
		else
		{
			destroy();
		}
	}
	else
	{
		resultButtonClicked(b->getName());
	}
}

void DialogWindowWithBackgroundThread::runSynchronous()
{
	// Obviously only available in the message loop!
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	run();
	threadFinished();
	destroy();
}

void DialogWindowWithBackgroundThread::showStatusMessage(const String &message)
{
	MessageManagerLock lock(thread);

	if (lock.lockWasGained())
	{
		if (getTextEditor("state") != nullptr)
		{
			getTextEditor("state")->setText(message, dontSendNotification);
		}
		else
		{
			// Did you just call this method before 'addBasicComponents()' ?
			jassertfalse;

		}
	}
}



void DialogWindowWithBackgroundThread::runThread()
{
	thread = new LoadingThread(this);
	thread->startThread();
}


ModalBaseWindow::ModalBaseWindow()
{
	s.colour = Colours::black;
	s.radius = 20;
	s.offset = Point<int>();
}

ModalBaseWindow::~ModalBaseWindow()
{

	shadow = nullptr;
	clearModalComponent();
}

void ModalBaseWindow::setModalComponent(Component *component, int fadeInTime/*=0*/)
{
	if (modalComponent != nullptr)
	{
		shadow = nullptr;
		modalComponent = nullptr;
	}


	shadow = new DropShadower(s);
	modalComponent = component;


	if (fadeInTime == 0)
	{
		dynamic_cast<Component*>(this)->addAndMakeVisible(modalComponent);
		modalComponent->centreWithSize(component->getWidth(), component->getHeight());
	}
	else
	{
		dynamic_cast<Component*>(this)->addChildComponent(modalComponent);
		modalComponent->centreWithSize(component->getWidth(), component->getHeight());
		Desktop::getInstance().getAnimator().fadeIn(modalComponent, fadeInTime);

	}



	shadow->setOwner(modalComponent);
}

bool ModalBaseWindow::isCurrentlyModal() const
{
	return modalComponent != nullptr;
}

void ModalBaseWindow::clearModalComponent()
{
	shadow = nullptr;
	modalComponent = nullptr;
}
