/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

namespace hise { using namespace juce;

ThreadWithQuasiModalProgressWindow::ThreadWithQuasiModalProgressWindow (const String& title,
                                                    const bool hasProgressBar,
                                                    const bool hasCancelButton,
													Holder *holder_,
                                                    const int cancellingTimeOutMs,
                                                    const String& cancelButtonText,
                                                    Component* componentToCentreAround)
   : Thread ("ThreadWithQuasiModalProgressWindow"),
     progress (0.0),
	 holder(holder_),
     timeOutMsWhenCancelling (cancellingTimeOutMs),
     wasCancelledByUser (false)
{
	ScopedPointer<LookAndFeel> alaf = PresetHandler::createAlertWindowLookAndFeel();
    alertWindow = alaf->createAlertWindow (title, String(),
                                        cancelButtonText.isEmpty() ? TRANS("Cancel")
                                                                   : cancelButtonText,
                                        String(), String(),
                                        AlertWindow::NoIcon, hasCancelButton ? 1 : 0,
                                        componentToCentreAround);

    // if there are no buttons, we won't allow the user to interrupt the thread.
    alertWindow->setEscapeKeyCancels (false);

	alertWindow->setOpaque(true);

	if (hasProgressBar)
	{
		alertWindow->addProgressBarComponent(progress);
	}

	holder->addThreadToQueue(this);
}

ThreadWithQuasiModalProgressWindow::~ThreadWithQuasiModalProgressWindow()
{
    stopThread (timeOutMsWhenCancelling);
}

void ThreadWithQuasiModalProgressWindow::setProgress (const double newProgress)
{
    progress = newProgress;
}

void ThreadWithQuasiModalProgressWindow::setStatusMessage (const String& newStatusMessage)
{
    const ScopedLock sl (messageLock);
    message = newStatusMessage;
}

void ThreadWithQuasiModalProgressWindow::timerCallback()
{
    bool threadStillRunning = isThreadRunning();

    if (!threadStillRunning)
    {
        stopTimer();
        stopThread (timeOutMsWhenCancelling);
        
        wasCancelledByUser = threadStillRunning;
        threadComplete (threadStillRunning);
        return; // (this may be deleted now)
    }

    const ScopedLock sl (messageLock);
    alertWindow->setMessage (message);
}

void ThreadWithQuasiModalProgressWindow::threadComplete(bool) 
{ 
	holder->currentThreadHasFinished();
}

bool ThreadWithQuasiModalProgressWindow::runThread (const int priority)
{
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	startThread(priority);
	startTimer(100);

	{
		const ScopedLock sl(messageLock);
		alertWindow->setMessage(message);
	}

    return ! wasCancelledByUser;
}

ThreadWithQuasiModalProgressWindow::Overlay::Overlay() :
	currentTaskIndex(0),
	totalTasks(0),
	totalProgress(0.0)
{
	alaf = PresetHandler::createAlertWindowLookAndFeel();
	setInterceptsMouseClicks(true, true);
	addAndMakeVisible(totalProgressBar = new ProgressBar(totalProgress));

	totalProgressBar->setLookAndFeel(alaf);
	totalProgressBar->setOpaque(true);
}

ThreadWithQuasiModalProgressWindow::Overlay::~Overlay()
{
	totalProgressBar->setLookAndFeel(nullptr);
}

void ThreadWithQuasiModalProgressWindow::Overlay::setDialog(AlertWindow *newWindow)
{
	toFront(false);

	setVisible(newWindow != nullptr);

	window = newWindow;

	if (window != nullptr)
	{
		window->toFront(false);

		removeAllChildren();
		addAndMakeVisible(window);
		addAndMakeVisible(totalProgressBar);

		resized();
	}
}

void ThreadWithQuasiModalProgressWindow::Overlay::resized()
{
	if (window.getComponent() != nullptr)
	{
		window->centreWithSize(window->getWidth(), window->getHeight());

		totalProgressBar->setBounds(window->getX(), window->getBottom() + 20, window->getWidth(), 24);
	}
}

void ThreadWithQuasiModalProgressWindow::Overlay::setTotalTasks(int numTasks)
{
	totalTasks = jmax<int>(totalTasks, numTasks);

	if (totalTasks == 0)
	{
		totalProgress = 0.0;
	}
	else
	{
		totalProgress = (double(currentTaskIndex - 1) / (double)totalTasks);
	}
}

void ThreadWithQuasiModalProgressWindow::Overlay::incCurrentIndex()
{
	currentTaskIndex++;

	if (totalTasks == 0.0)
	{
		totalProgress = 0.0;
	}
	else
	{
		totalProgress = (double(currentTaskIndex - 1) / (double)totalTasks);
	}
}

void ThreadWithQuasiModalProgressWindow::Overlay::clearIndexes()
{
	currentTaskIndex = 0;
	totalTasks = 0;
	totalProgress = 0.0;
	parentSnapshot = Image();
}

void ThreadWithQuasiModalProgressWindow::Overlay::paint(Graphics &g)
{
#if USE_BACKEND
	g.fillAll(Colours::grey.withAlpha(0.5f));

	if (window.getComponent() != nullptr)
	{
		Rectangle<int> textArea(0,
			0,
			getWidth(),
			42);

		g.setColour(Colours::black.withAlpha(0.7f));
		g.fillRect(textArea);

		g.setColour(Colours::white);



		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText("Task: " + String(currentTaskIndex) + "/" + String(totalTasks), textArea, Justification::centred);
	}
#else
    ignoreUnused(g);
#endif
}

ThreadWithQuasiModalProgressWindow::Holder::Holder() :
	overlay(nullptr),
	delayer(*this)
{

}

ThreadWithQuasiModalProgressWindow::Holder::~Holder()
{
	cancel();
}

void ThreadWithQuasiModalProgressWindow::Holder::buttonClicked(Button *)
{
	cancel();
}

void ThreadWithQuasiModalProgressWindow::Holder::setOverlay(Overlay *currentOverlay)
{
	overlay = currentOverlay;
}

ThreadWithQuasiModalProgressWindow::ThreadWithQuasiModalProgressWindow::Overlay * ThreadWithQuasiModalProgressWindow::Holder::getOverlay()
{
	return overlay.getComponent();
}

void ThreadWithQuasiModalProgressWindow::Holder::showDialog()
{
	ThreadWithQuasiModalProgressWindow *window = queue.getFirst();

	if (getOverlay() && window != nullptr)
	{
		getOverlay()->setTotalTasks(queue.size());
		getOverlay()->incCurrentIndex();

		AlertWindow *w = window->getAlertWindow();
		getCancelButton(w)->addListener(this);
		getOverlay()->setDialog(w);
	}
}

void ThreadWithQuasiModalProgressWindow::Holder::handleAsyncUpdate()
{
	ThreadWithQuasiModalProgressWindow *window = queue[0];



	showDialog();

	window->runThread();
}

void ThreadWithQuasiModalProgressWindow::Holder::addListener(Listener* newListener)
{
	listeners.add(newListener);
}

void ThreadWithQuasiModalProgressWindow::Holder::removeListener(Listener* listenerToRemove)
{
	listeners.removeAllInstancesOf(listenerToRemove);
}

bool ThreadWithQuasiModalProgressWindow::Holder::isBusy() const noexcept
{
	return queue.size() > 0;
}

void ThreadWithQuasiModalProgressWindow::Holder::cancel()
{
	clearDialog();

	if (getOverlay())
	{
		getOverlay()->clearIndexes();
	}

	queue.clear();
}

void ThreadWithQuasiModalProgressWindow::Holder::clearDialog()
{
	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->lastTaskRemoved();
		}
	}

	if (getOverlay() != nullptr)
	{
		getOverlay()->setDialog(nullptr);
	}
}

void ThreadWithQuasiModalProgressWindow::Holder::currentThreadHasFinished()
{
	queue.remove(0, true);

	if (queue.size() == 0)
	{
		clearDialog();
	}
	else
	{
		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
			{
				listeners[i]->taskRemoved();
			}
		}

		runNextThread();
	}

	if (queue.size() == 0 && getOverlay())
	{
		getOverlay()->clearIndexes();
	}
}

void ThreadWithQuasiModalProgressWindow::Holder::addThreadToQueue(ThreadWithQuasiModalProgressWindow *window)
{
	queue.add(window);

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->taskAdded();
		}
	}

	if (queue.size() == 1)
	{
		runNextThread();
	}
}

void ThreadWithQuasiModalProgressWindow::Holder::runNextThread()
{
	triggerAsyncUpdate();
}

TextButton * ThreadWithQuasiModalProgressWindow::Holder::getCancelButton(AlertWindow *window)
{
	for (int i = 0; i < window->getNumChildComponents(); i++)
	{
		if (TextButton *b = dynamic_cast<TextButton*>(window->getChildComponent(i)))
		{
			return b;
		}
	}

	jassertfalse;
	return nullptr;
}

ThreadWithQuasiModalProgressWindow::Holder::Listener::~Listener()
{
	masterReference.clear();
}

ThreadWithQuasiModalProgressWindow::Holder::WindowDelayer::WindowDelayer(ThreadWithQuasiModalProgressWindow::Holder &parent_) :
	parent(parent_)
{

}

void ThreadWithQuasiModalProgressWindow::Holder::WindowDelayer::postDelayMessage()
{
	if (!isTimerRunning())
	{
		startTimer(200);
	}
}

void ThreadWithQuasiModalProgressWindow::Holder::WindowDelayer::timerCallback()
{
	parent.showDialog();
	stopTimer();
}

} // namespace hise