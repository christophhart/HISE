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

#ifndef BACKGROUNDTHREADS_H_INCLUDED
#define BACKGROUNDTHREADS_H_INCLUDED

namespace hise { using namespace juce;

class MainController;

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

	ModalBaseWindow();
	virtual ~ModalBaseWindow();;

	void setModalComponent(Component *component, int fadeInTime=0);
	bool isCurrentlyModal() const;
	void clearModalComponent();

	/** This returns the main controller depending on the project type. */
	const MainController* getMainController() const;

	MainController* getMainController();

	virtual const MainController* getMainControllerToUse() const { return nullptr; }
	virtual MainController* getMainControllerToUse() { return nullptr; }

	ScopedPointer<Component> modalComponent;
	DropShadow s;
	ScopedPointer<DropShadower> shadow;
};



class ComponentWithHelp
{
public:

	struct GlobalHandler
	{

		virtual ~GlobalHandler() {};

		bool isHelpEnabled() const { return helpEnabled; }

		virtual void showHelp(ComponentWithHelp* h) = 0;
		

		void toggleHelp()
		{
			helpEnabled = !helpEnabled;

			for (auto c : registeredHelpers)
			{
				if (auto asComponent = dynamic_cast<Component*>(c.get()))
				{
					asComponent->repaint();
				}
			}
		}

		void registerHelper(ComponentWithHelp* c)
		{
			registeredHelpers.addIfNotAlreadyThere(c);
		}

		void removeHelper(ComponentWithHelp* c)
		{
			registeredHelpers.removeAllInstancesOf(c);
		}

	private:


		Array<WeakReference<ComponentWithHelp>> registeredHelpers;

		bool helpEnabled = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalHandler);
	};

	ComponentWithHelp(GlobalHandler* handler_):
		handler(handler_)
	{
		p.loadPathFromData(MainToolbarIcons::help, sizeof(MainToolbarIcons::help));

		if (handler != nullptr)
			handler->registerHelper(this);
	}

	virtual ~ComponentWithHelp()
	{
		if (handler != nullptr)
			handler->removeHelper(this);
	}

	virtual String getMarkdownHelpUrl() const = 0;

	void openHelp();

	void paintHelp(Graphics& g);

private:

	Path p;

	WeakReference<GlobalHandler> handler;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ComponentWithHelp);
};


/** A dialog window that performs an operation on a background thread.
*
*	- no modal windows
*	- a callback (in the message loop) when the thread is finished.
*	- unthreaded usage (everything in the message loop) if desired
*
*	In order to use it, subclass it and overwrite the two callbacks run() and threadFinished().
*
*	Then simply create a instance and call its method 'setModalComponentOfMainEditor()'
*/
class DialogWindowWithBackgroundThread : public AlertWindow,
									  public QuasiModalComponent,
									  public AsyncUpdater
{
public:

	using LogFunction = std::function<void(const String&)>;

	struct LogData
	{
		LogFunction logFunction;
		double progress = 0.0;
	};

	// ================================================================================================================

	/** This stops the thread. In order to avoid killing, check threadShouldExit() regularly in your run() method. */
	virtual ~DialogWindowWithBackgroundThread();


	// ================================================================================================================

	/** Overwrite this method and do your thing. */
	virtual void run() = 0;

	/** This will be called after the thread has finished its task. 
	*
	*	It is called from the message loop so you can do some interface stuff without caring about the MessageManagerLock. 
	*/
	virtual void threadFinished() = 0;

	/** This resets the window to the state before being started. */
	void reset();

	class AdditionalRow : public Component,
								public ButtonListener
	{
	public:

		
		AdditionalRow(DialogWindowWithBackgroundThread* parent_):
			parent(parent_)
		{

		}

		~AdditionalRow()
		{
			columns.clear();
		}

		void buttonClicked(Button* b) override
		{
			parent->buttonClicked(b);
		}

		void addComboBox(const String& name, const StringArray& items, const String& label, int width=0);

		void addCustomComponent(Component* newComponent, const String& name=String(), int width=0);

		void addButton(const String& name, const KeyPress& k=KeyPress(), int width=0);

		void setInfoTextForLastComponent(const String& infoToShow);

		void setStyleDataForMarkdownHelp(const MarkdownLayout::StyleData& sd);

		template <class ComponentType> ComponentType* getComponent(const String& name)
		{
			for (auto c : columns)
			{
				if (auto typed = dynamic_cast<ComponentType*>(c->component.get()))
				{
					if (typed->getName() == name)
						return typed;
				}
			}

			jassertfalse;
			return nullptr;
		}

		void resized() override;

		struct Column: public Component,
					   public ButtonListener
		{
			

			Column(Component* t, const String& name_, int width_);

			void buttonClicked(Button* /*b*/) override
			{
				
			}

			~Column()
			{
				component = nullptr;
				infoButton = nullptr;
			}

			void paint(Graphics& g) override
			{
				if (name.isNotEmpty())
				{
					auto area = getLocalBounds().removeFromTop(16);
					area.removeFromRight(18);
					g.setFont(GLOBAL_BOLD_FONT());
					g.setColour(Colours::white);
					g.drawText(name, area, Justification::centredLeft);
				}
			}

			

			void resized() override;

			ScopedPointer<Component> component;
			ScopedPointer<MarkdownHelpButton> infoButton;
			String name;
			int width = 0;
		};

		OwnedArray<Column> columns;

		DialogWindowWithBackgroundThread* parent;
	};


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
	void showStatusMessage(const String &message) const;
	
	/** Sets the progressbar during job execution. */
	void setProgress(double progressValue) { logData.progress = progressValue; };

	double& getProgressCounter() { return logData.progress; }

	void buttonClicked(Button* b) override;

	/** If you want to use a button for anything else than starting or cancelling, you can override this method and do whatever you need.
	*
	*	The name is the same String that you specified with 'addButton()'
	*/
	virtual void resultButtonClicked(const String &/*name*/) {};

	/** If you want to check something (on the message thread) before launching the actual task, overwrite this method.
	*
	*	This will be called when the user presses OK. If you return false, it won't launch the process.
	*/
	virtual bool checkConditionsBeforeStartingThread() { return true; }

	void wait(int milliSeconds)
	{
		if (thread != nullptr)
		{
			thread->wait(milliSeconds);
		}
	}
	
	void setAdditionalLogFunction(const LogFunction& additionalLogFunction)
	{
		logData.logFunction = additionalLogFunction;
	}

	/** You can add a lambda that will be executed on the message thread after the task
	    has been completed. */
	void setAdditionalFinishCallback(const std::function<void()>& f)
	{
		additionalFinishCallback = f;
	}

protected:

	LogData logData;

	// ================================================================================================================

	/** Creates a new instance with the given title. 
	*
	*	Don't forget to call 'addBasicComponents' after you added your stuff.
	*/
	DialogWindowWithBackgroundThread(const String &title, bool synchronous=false);

	/** Call this method in your constructor after you created all custom methods. */
	void addBasicComponents(bool addOkButton = true);
	
	Button* getButton(const String& name)
	{
		for (int i = 0; i < getNumChildComponents(); i++)
		{
			if (auto b = dynamic_cast<Button*>(getChildComponent(i)))
			{
				if(b->getName() == name)
					return b;
			}
		}

		return nullptr;
	}

	void setTimeoutMs(int newTimeout)
	{
		timeoutMs = newTimeout;
	}

	Thread* getCurrentThread()
	{
		return thread;
	}

private:

	ScopedPointer<LookAndFeel> laf;

	std::function<void()> additionalFinishCallback;

	mutable bool recursion = false;


	void handleAsyncUpdate() override;

	// ================================================================================================================

	class LoadingThread : public Thread
	{
	public:

		LoadingThread(DialogWindowWithBackgroundThread *parent_) :
			Thread(parent_->getName()),
			parent(parent_)
		{};

		void run() override
		{
			parent->run();
			parent->triggerAsyncUpdate();
		};

	private:

		DialogWindowWithBackgroundThread *parent;
	};


	// ================================================================================================================

	friend class LoadingThread;

	bool resetCalled = false;

	const bool synchronous;

	int timeoutMs = 6000;
	
	bool isQuasiModal;

	ScopedPointer<LoadingThread> thread;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DialogWindowWithBackgroundThread)
};

class MainController;
class ModulatorSynthChain;

class PresetLoadingThread : public DialogWindowWithBackgroundThread
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


class SampleDataExporter : public DialogWindowWithBackgroundThread,
						   public hlac::HlacArchiver::Listener,
						   public ControlledObject
					
{
public:

	enum PartSize
	{
		HalfGig = 0,
		OneGig,
		OneAndHalfGig,
		TwoGig,
		numPartSizes
	};

	SampleDataExporter(MainController* mc);;

	void logVerboseMessage(const String& verboseMessage) override;
	void logStatusMessage(const String& message) override;
	void criticalErrorOccured(const String& message) override;

	void run() override;;
	void threadFinished() override;

	void setTargetDirectory(const File& targetDirectory);

private:


	Array<File> collectMonoliths();

	String getExpansionName() const;

	String getMetadataJSON() const;

	String getProjectName() const;

	String getCompanyName() const;

	String getProjectVersion() const;

	File getTargetFile() const;

	ModulatorSynthChain* synthChain;

	String fullLog;
	String criticalError;

	ScopedPointer<FilenameComponent> hxiFile;
	ScopedPointer<FilenameComponent> targetFile;
	ScopedPointer<ProgressBar> totalProgressBar;
	double totalProgress = 0.0;
	int numExported = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleDataExporter)
};


class SampleDataImporter : public DialogWindowWithBackgroundThread,
	public hlac::HlacArchiver::Listener
{
public:

	enum class ErrorCode
	{
		OK = 0,
		ProductMismatch,
		VersionMismatch,
		FileError,
		numErrorCodes
	};

	SampleDataImporter(ModalBaseWindow* bpe_);

	void logVerboseMessage(const String& verboseMessage) override;

	void logStatusMessage(const String& message) override;

	void criticalErrorOccured(const String& message) override;

	void run() override;

	void threadFinished() override;

	bool checkConditionsBeforeStartingThread() override;

private:

	String getProjectName() const;

	String getCompanyName() const;

	String getProjectVersion() const;

	File getTargetDirectory() const;

	String getMetadata() const;

	File getSourceFile() const;

	void checkSanity(var metadata)
	{

	}

	Result result;

	String criticalError;

	File archiveFile;

	ScopedPointer<FilenameComponent> targetFile;
	ScopedPointer<FilenameComponent> sampleDirectory;

	ScopedPointer<ProgressBar> totalProgressBar;
	ScopedPointer<ProgressBar> partProgressBar;

	double partProgress = 0.0;
	double totalProgress = 0.0;

	ModalBaseWindow* modalBaseWindow;

	ModulatorSynthChain* synthChain;
};

} // namespace hise

#endif  // BACKGROUNDTHREADS_H_INCLUDED
