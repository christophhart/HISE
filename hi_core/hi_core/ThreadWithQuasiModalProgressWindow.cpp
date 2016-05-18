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
	AlertWindowLookAndFeel alaf;

    alertWindow = alaf.createAlertWindow (title, String(),
                                        cancelButtonText.isEmpty() ? TRANS("Cancel")
                                                                   : cancelButtonText,
                                        String(), String(),
                                        AlertWindow::NoIcon, hasCancelButton ? 1 : 0,
                                        componentToCentreAround);

    // if there are no buttons, we won't allow the user to interrupt the thread.
    alertWindow->setEscapeKeyCancels (false);

    if (hasProgressBar)
        alertWindow->addProgressBarComponent (progress);

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
