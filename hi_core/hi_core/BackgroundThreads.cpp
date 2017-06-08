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


ThreadWithAsyncProgressWindow::ThreadWithAsyncProgressWindow(const String &title, bool synchronous_) :
AlertWindow(title, String(), AlertWindow::AlertIconType::NoIcon),
progress(0.0),
isQuasiModal(false),
synchronous(synchronous_)
{
	setLookAndFeel(&laf);

	setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	setColour(AlertWindow::textColourId, Colours::white);

}

ThreadWithAsyncProgressWindow::~ThreadWithAsyncProgressWindow()
{
	if (thread != nullptr)
	{
		thread->stopThread(6000);
	}
}

void ThreadWithAsyncProgressWindow::addBasicComponents(bool addOKButton)
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

bool ThreadWithAsyncProgressWindow::threadShouldExit() const
{
	if (thread != nullptr)
	{
		return thread->threadShouldExit();
	};

	return false;
}

void ThreadWithAsyncProgressWindow::handleAsyncUpdate()
{
	threadFinished();
	destroy();
}

void ThreadWithAsyncProgressWindow::buttonClicked(Button* b)
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

void ThreadWithAsyncProgressWindow::runSynchronous()
{
	// Obviously only available in the message loop!
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	run();
	threadFinished();
	destroy();
}

void ThreadWithAsyncProgressWindow::showStatusMessage(const String &message)
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



void ThreadWithAsyncProgressWindow::runThread()
{
	thread = new LoadingThread(this);
	thread->startThread();
}



PresetLoadingThread::PresetLoadingThread(MainController *mc, const ValueTree v_) :
ThreadWithAsyncProgressWindow("Loading Preset " + v_.getProperty("ID").toString()),
v(v_),
mc(mc),
fileNeedsToBeParsed(false)
{
	addBasicComponents(false);
}

PresetLoadingThread::PresetLoadingThread(MainController *mc, const File &presetFile) :
ThreadWithAsyncProgressWindow("Loading Preset " + presetFile.getFileName()),
file(presetFile),
fileNeedsToBeParsed(true),
mc(mc)
{


	addBasicComponents(false);
}

void PresetLoadingThread::run()
{
	if (fileNeedsToBeParsed)
	{
		setProgress(0.0);
		showStatusMessage("Parsing preset file");
		FileInputStream fis(file);

		v = ValueTree::readFromStream(fis);

		if (v.isValid() && v.getProperty("Type", var::undefined()).toString() == "SynthChain")
		{
			if (v.getType() != Identifier("Processor"))
			{
				v = PresetHandler::changeFileStructureToNewFormat(v);
			}
		}

		const int presetVersion = v.getProperty("BuildVersion", 0);

		if (presetVersion > BUILD_SUB_VERSION)
		{
			PresetHandler::showMessageWindow("Version mismatch", "The preset was built with a newer the build of HISE: " + String(presetVersion) + ". To ensure perfect compatibility, update to at least this build.", PresetHandler::IconType::Warning);
		}

		setProgress(0.5);
		if (threadShouldExit())
		{
			mc->clearPreset();
			return;
		}

	}

	ModulatorSynthChain *synthChain = mc->getMainSynthChain();



	sampleRate = synthChain->getSampleRate();
	bufferSize = synthChain->getBlockSize();

	synthChain->setBypassed(true);

	// Reset the sample rate so that prepareToPlay does not get called in restoreFromValueTree
	synthChain->setCurrentPlaybackSampleRate(-1.0);

	synthChain->setId(v.getProperty("ID", "MainSynthChain"));

	if (threadShouldExit()) return;

	showStatusMessage("Loading modules");

    
    mc->skipCompilingAtPresetLoad = true;
	synthChain->restoreFromValueTree(v);
    mc->skipCompilingAtPresetLoad = false;
    
	if (threadShouldExit()) return;

}

void PresetLoadingThread::threadFinished()
{
	ModulatorSynthChain *synthChain = mc->getMainSynthChain();

	ScopedLock sl(synthChain->getMainController()->getLock());

	synthChain->prepareToPlay(sampleRate, bufferSize);
	synthChain->compileAllScripts();

	mc->getSampleManager().getAudioSampleBufferPool()->clearData();

	synthChain->setBypassed(false);

	Processor::Iterator<ModulatorSampler> iter(synthChain, false);

	int i = 0;

	while (ModulatorSampler *sampler = iter.getNextProcessor())
	{
		showStatusMessage("Loading samples from " + sampler->getId());
		setProgress((double)i / (double)iter.getNumProcessors());
		sampler->refreshPreloadSizes();
		sampler->refreshMemoryUsage();
		if (threadShouldExit())
		{
			mc->clearPreset();
			return;
		}
	}
}

