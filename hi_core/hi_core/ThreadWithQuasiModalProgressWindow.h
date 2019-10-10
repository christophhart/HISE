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

#ifndef JUCE_THREADWITHQUASIMODALPROGRESSWINDOW_H_INCLUDED
#define JUCE_THREADWITHQUASIMODALPROGRESSWINDOW_H_INCLUDED

namespace hise { using namespace juce;

//==============================================================================
/**
    A thread that automatically pops up a modal dialog box with a progress bar
    and cancel button while it's busy running.

    These are handy for performing some sort of task while giving the user feedback
    about how long there is to go, etc.

    E.g. @code
    class MyTask  : public ThreadWithProgressWindow
    {
    public:
        MyTask()    : ThreadWithProgressWindow ("busy...", true, true)
        {
        }

        void run()
        {
            for (int i = 0; i < thingsToDo; ++i)
            {
                // must check this as often as possible, because this is
                // how we know if the user's pressed 'cancel'
                if (threadShouldExit())
                    break;

                // this will update the progress bar on the dialog box
                setProgress (i / (double) thingsToDo);


                //   ... do the business here...
            }
        }
    };

    void doTheTask()
    {
        MyTask m;

        if (m.runThread())
        {
            // thread finished normally..
        }
        else
        {
            // user pressed the cancel button..
        }
    }

    @endcode

    @see Thread, AlertWindow
*/
class JUCE_API  ThreadWithQuasiModalProgressWindow  : public Thread,
													  private Timer
{
public:

	class Overlay : public Component
	{
	public:

		Overlay();
		~Overlay();

		void setDialog(AlertWindow *newWindow);
		void resized();
		void setTotalTasks(int numTasks);
		void incCurrentIndex();
		void clearIndexes();
		void paint(Graphics &g) override;;

	private:

		Image parentSnapshot;

		int currentTaskIndex;
		int totalTasks;

		ScopedPointer<LookAndFeel> alaf;
		Component::SafePointer<AlertWindow> window;

		ScopedPointer<ProgressBar> totalProgressBar;

		double totalProgress;
	};

	/** This class manages the lifetime of the ThreadWithQuasiModalProgressWindow.
	*
	*	In order to use it, subclass your main instance (eg. the AudioProcessor of a plugin) from this class. Then you'll need to call setMainComponent() as soon as you have a main window (eg. the AudioProcessorEditor constructor).
	*	The functionality of the ThreadWithQuasiModalProgressWindow does not depend on the main component (so you can launch the thread before having a plugin editor). 
	*	But if a main window is set, it will popup as asynchronous, quasi modal child component (instead of the modal desktop window of the original class).
	*	Whenever you want to create a ProgressWindow, just pass the pointer to this instance and everything else should be taken care of by this class.
	*
	*	This class will own the ProgressWindow and delete it if no longer needed (or if itself is deleted).
	*/
	class Holder : public ButtonListener,
                   public AsyncUpdater
	{
	public:

		Holder();

		virtual ~Holder();

		void buttonClicked(Button *) override;

		void setOverlay(Overlay *currentOverlay);

		Overlay *getOverlay();

		void showDialog();

        void handleAsyncUpdate();
        
		struct Listener
		{
			virtual ~Listener();;

			virtual void taskAdded() {};
			virtual void taskRemoved() {};
			virtual void lastTaskRemoved() {};

		private:

			friend class WeakReference<Listener>;
			WeakReference<Listener>::Master masterReference;
		};


		void addListener(Listener* newListener);
		void removeListener(Listener* listenerToRemove);;

		bool isBusy() const noexcept;


	private:

		class WindowDelayer : private Timer
		{
		public:
			WindowDelayer(ThreadWithQuasiModalProgressWindow::Holder &parent_);

			void postDelayMessage();;

			void timerCallback() override;;

		private:

			ThreadWithQuasiModalProgressWindow::Holder &parent;
		};

		friend class ThreadWithQuasiModalProgressWindow;

		void cancel();

		void clearDialog();

		void currentThreadHasFinished();
		
		void addThreadToQueue(ThreadWithQuasiModalProgressWindow *window);

		
		void runNextThread();

		/** AlertWindow, Y U no giving cancel button? */
		static TextButton *getCancelButton(AlertWindow *window);

		OwnedArray<ThreadWithQuasiModalProgressWindow> queue;

		WindowDelayer delayer;

		Component::SafePointer<Overlay> overlay;

		Array<WeakReference<Listener>> listeners;
	};

    //==============================================================================
    /** Creates the thread.

        Initially, the dialog box won't be visible, it'll only appear when the
        runThread() method is called.

        @param windowTitle              the title to go at the top of the dialog box
        @param hasProgressBar           whether the dialog box should have a progress bar (see
                                        setProgress() )
        @param hasCancelButton          whether the dialog box should have a cancel button
        @param timeOutMsWhenCancelling  when 'cancel' is pressed, this is how long to wait for
                                        the thread to stop before killing it forcibly (see
                                        Thread::stopThread() )
        @param cancelButtonText         the text that should be shown in the cancel button
                                        (if it has one). Leave this empty for the default "Cancel"
        @param componentToCentreAround  if this is non-null, the window will be positioned
                                        so that it's centred around this component.
    */
    ThreadWithQuasiModalProgressWindow (const String& windowTitle,
                              bool hasProgressBar,
                              bool hasCancelButton,
							  Holder *holder,
                              int timeOutMsWhenCancelling = 10000,
                              const String& cancelButtonText = String(),
                              Component* componentToCentreAround = nullptr);

    /** Destructor. */
    ~ThreadWithQuasiModalProgressWindow();

    //==============================================================================
   
    /** Starts the thread and waits for it to finish.

        This will start the thread, make the dialog box appear, and wait until either
        the thread finishes normally, or until the cancel button is pressed.

        Before returning, the dialog box will be hidden.

        @param priority   the priority to use when starting the thread - see
                          Thread::startThread() for values
        @returns true if the thread finished normally; false if the user pressed cancel
    */
	bool runThread(int priority = 5);

    /** The thread should call this periodically to update the position of the progress bar.

        @param newProgress  the progress, from 0.0 to 1.0
        @see setStatusMessage
    */
    void setProgress (double newProgress);

	double *getProgressValue() { return &progress; };

    /** The thread can call this to change the message that's displayed in the dialog box. */
    void setStatusMessage (const String& newStatusMessage);

    /** Returns the AlertWindow that is being used. */
    AlertWindow* getAlertWindow() const noexcept        { return alertWindow; }

    //==============================================================================
    /** This method is called (on the message thread) when the operation has finished.
        You may choose to use this callback to delete the ThreadWithProgressWindow object.
    */
    virtual void threadComplete (bool userPressedCancel);

private:
    //==============================================================================
    void timerCallback() override;

    double progress;
    ScopedPointer<AlertWindow> alertWindow;
    String message;
    CriticalSection messageLock;
    const int timeOutMsWhenCancelling;
    bool wasCancelledByUser;

	Holder *holder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThreadWithQuasiModalProgressWindow)
};

} // namespace hise

#endif   // JUCE_THREADWITHPROGRESSWINDOW_H_INCLUDED
