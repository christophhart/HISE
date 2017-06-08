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

#ifndef BACKGROUNDTHREADS_H_INCLUDED
#define BACKGROUNDTHREADS_H_INCLUDED



class QuasiModalComponent
{
public:

	virtual ~QuasiModalComponent() {}

	/** This uses the given component to show this dialog as quasi modal window on the main editor. */
	void setModalBaseWindowComponent(Component * childComponentOfModalBaseWindow, int fadeInTime=0);

	void showOnDesktop();

	void destroy();

private:

	bool isQuasiModal = false;
};

class ModalBaseWindow
{
public:

	ModalBaseWindow()
	{
		s.colour = Colours::black;
		s.radius = 20;
		s.offset = Point<int>();
	}

	virtual ~ModalBaseWindow()
	{	
		shadow = nullptr;
		clearModalComponent();
		
	};


	void setModalComponent(Component *component, int fadeInTime=0)
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

	bool isCurrentlyModal() const { return modalComponent != nullptr; }

	void clearModalComponent()
	{
		shadow = nullptr;
		modalComponent = nullptr;
	}

	ScopedPointer<Component> modalComponent;

	DropShadow s;

	ScopedPointer<DropShadower> shadow;
};


/** An replacement for the ThreadWithProgressWindow with the following differences:
*
*	- no modal windows
*	- a callback (in the message loop) when the thread is finished.
*	- unthreaded usage (everything in the message loop) if desired
*
*	In order to use it, subclass it and overwrite the two callbacks run() and threadFinished().
*
*	Then simply create a instance and call its method 'setModalComponentOfMainEditor()'
*/
class ThreadWithAsyncProgressWindow : public AlertWindow,
									  public QuasiModalComponent,
									  public AsyncUpdater
{
public:

	// ================================================================================================================

	/** This stops the thread. In order to avoid killing, check threadShouldExit() regularly in your run() method. */
	virtual ~ThreadWithAsyncProgressWindow();


	// ================================================================================================================

	/** Overwrite this method and do your thing. */
	virtual void run() = 0;

	/** This will be called after the thread has finished its task. 
	*
	*	It is called from the message loop so you can do some interface stuff without caring about the MessageManagerLock. 
	*/
	virtual void threadFinished() = 0;


	// ================================================================================================================
	
	// ================================================================================================================

	/** Starts the thread. */
	void runThread();

	/** Starts the task synchronously. */
	void runSynchronous();

	/** Checks if the thread should exist. */
	bool threadShouldExit() const;


	// ================================================================================================================

	/** Shows a status message during job execution. */
	void showStatusMessage(const String &message);
	
	/** Sets the progressbar during job execution. */
	void setProgress(double progressValue) { progress = progressValue; };

	double& getProgressCounter() { return progress; }

	void buttonClicked(Button* b) override;

	/** If you want to use a button for anything else than starting or cancelling, you can override this method and do whatever you need.
	*
	*	The name is the same String that you specified with 'addButton()'
	*/
	virtual void resultButtonClicked(const String &/*name*/) {};

	void wait(int milliSeconds)
	{
		if (thread != nullptr)
		{
			thread->wait(milliSeconds);
		}
	}
	
protected:

	// ================================================================================================================

	/** Creates a new instance with the given title. 
	*
	*	Don't forget to call 'addBasicComponents' after you added your stuff.
	*/
	ThreadWithAsyncProgressWindow(const String &title, bool synchronous=false);

	/** Call this method in your constructor after you created all custom methods. */
	void addBasicComponents(bool addOkButton = true);
	
	double progress;

private:

	void handleAsyncUpdate() override;

	// ================================================================================================================

	class LoadingThread : public Thread
	{
	public:

		LoadingThread(ThreadWithAsyncProgressWindow *parent_) :
			Thread(parent_->getName()),
			parent(parent_)
		{};

		void run() override
		{
			parent->run();
			parent->triggerAsyncUpdate();
		};

	private:

		ThreadWithAsyncProgressWindow *parent;
	};


	// ================================================================================================================

	friend class LoadingThread;

	const bool synchronous;

	
	bool isQuasiModal;
	AlertWindowLookAndFeel laf;

	ScopedPointer<LoadingThread> thread;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreadWithAsyncProgressWindow)
};

class MainController;

class PresetLoadingThread : public ThreadWithAsyncProgressWindow
{
public:

	PresetLoadingThread(MainController *mc, const ValueTree v);

	PresetLoadingThread(MainController *mc, const File &presetFile);

	void run() override;

	void threadFinished() override;

private:

	ValueTree v;
	MainController *mc;

	const File file;

	double sampleRate;
	int bufferSize;

	bool fileNeedsToBeParsed;
};

class ProjectHandler;



#endif  // BACKGROUNDTHREADS_H_INCLUDED
