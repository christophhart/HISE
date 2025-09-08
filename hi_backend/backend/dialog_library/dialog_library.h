// Put in the header definitions of every dialog here...

#pragma once

namespace hise {

class ChildProcessManager: public AsyncUpdater
{
public:

	struct ScopedLogger: public juce::Logger
	{
		ScopedLogger(ChildProcessManager& cm_):
		 cm(cm_),
		 prevLogger(Logger::getCurrentLogger()) 
		{
			Logger::setCurrentLogger(this);
		}

		~ScopedLogger()
		{
			Logger::setCurrentLogger(prevLogger);
		}

		Logger* prevLogger;

		void logMessage(const String& message) override
		{
			cm.logMessage(message);
		}

		ChildProcessManager& cm;
	};

	ChildProcessManager()
	{
		log.setDisableUndo(true);
	}

	virtual ~ChildProcessManager() {};

	virtual void setProgress(double progress) = 0;

	void handleAsyncUpdate() override
	{
		CodeDocument::Position pos(log, log.getNumCharacters());

		log.insertText(pos, currentMessage);
		
		SimpleReadWriteLock::ScopedReadLock sl(logLock);
		currentMessage = {};
	}

	void logMessage(String message)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(logLock);

#if JUCE_MAC
            if(message.contains("[!]"))
            {
                message = message.fromFirstOccurrenceOf("[!]", false, false);
                
                auto isWarning = message.contains("[-W") ||
                                 message.contains("was built for newer macOS version"); // AAX linker warning
                
                if(isWarning)
                    currentMessage << "\t";
                else
                    currentMessage << "!";
            }
#elif JUCE_WINDOWS
            if(message.contains("warning"))
                currentMessage << "\t";
            if(message.contains("error"))
                currentMessage << "!";
#endif
            auto isLinkerMessage = message.contains("Generating code") ||
								   message.contains("Linking ");

			if(isLinkerMessage)
			{
				currentMessage << "> ";
				setProgress(0.8);
			}
               
			currentMessage << message;
			if(!message.containsChar('\n'))
				currentMessage << '\n';
		}
        
        triggerAsyncUpdate();
	}

	SimpleReadWriteLock logLock;
    String currentMessage;

	CodeDocument log;

	//std::function<void(const String&)> logFunction;
	std::function<void()> killFunction;
};

namespace multipage {
namespace library {
using namespace juce;

struct CleanDspNetworkFiles: public EncodedDialogBase
{
	CleanDspNetworkFiles(BackendRootWindow* bpe);

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(CleanDspNetworkFiles, setItems);
		MULTIPAGE_BIND_CPP(CleanDspNetworkFiles, clearFile);
	}

	BackendDllManager::FolderSubType getType(const var::NativeFunctionArgs& args);

	var setItems(const var::NativeFunctionArgs& args);

	File getFolder(BackendDllManager::FolderSubType t)
	{
		return BackendDllManager::getSubFolder(getMainController(), t);
	}

	void removeNodeProperties(const Array<File>& filesToBeDeleted);

	var clearFile(const var::NativeFunctionArgs& args);
};


struct ExportSetupWizard: public EncodedDialogBase
{
	ExportSetupWizard(BackendRootWindow* bpe);

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(ExportSetupWizard, prevDownload);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, skipIfDesired);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, checkIDE);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, checkHisePath);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, checkSDK);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, onPost);
	}

	var checkHisePath(const var::NativeFunctionArgs& args);
	var checkIDE(const var::NativeFunctionArgs& args);
	var checkSDK(const var::NativeFunctionArgs& args);
	var prevDownload(const var::NativeFunctionArgs& args);
	var skipIfDesired(const var::NativeFunctionArgs& args);
	var onPost(const var::NativeFunctionArgs& args);
};

struct EncodedBroadcasterWizard: public EncodedDialogBase
{
	EncodedBroadcasterWizard(BackendRootWindow* bpe_);;

	void bindCallbacks() override;

	BackendRootWindow* bpe;

	var checkSelection(const var::NativeFunctionArgs& args);
	
	StringArray getAutocompleteItems(const Identifier& textEditorId);

	bool addListenersOnly = false;
    String customId;
};

struct WelcomeSnippetBrowserScreen: public EncodedDialogBase
{
	WelcomeSnippetBrowserScreen(BackendRootWindow* bpe):
	  EncodedDialogBase(bpe)
	{
		setWantsBackdrop(true);
		loadFrom("989.sNB..D...............35H...oi...wN.........J09R+f8rA8zA.FAuim.tE4F.U0OoT4rY8kEgvrRa00LQo6nUdSgX.hewHu3R6NFFioXHvBvH.ABPe.78bBkTa+5n1vm1SmtATJcMxWQF55IUW8HgKYtrYgLQKtesDhgzkoMSRz1haK5XEOkYClLJrQyzlKX17e5OyBaxfYSIRXbRyDlEzDQa4NEtLMgYJAMWvP07nKAVkTjlD5SfTNbAoMUkpjoZvJ5Qf.DkiZXMGq+LFPKGnW5T.sAfxxAhetSG2dJncC.k.uecq+6KYOqlusdmZsyHiBujsF8o9zSC+fft5QIHX47x9kt6+zxM1NI6DPlhTDFZoK4rwwYEZ8hHEAEoH0ZSwb5rRje+E.Ano1z2HxV928i507MFCH2bBQjq7XEP6FkkPoHonDjNrR6O1j7LykXKiLVi20jfwYWzwG9UZTc0iSiXC6e0QouGbvF0eWSgcN3fk8ma6J2wM1nDhpsC9Nt1U6.S20TON+zz1h5qDxYK8SK25beYS6qKn0Zwdr9cbe98W99UIb6mo7rqKuRnmN1MZBpKrX622YtK1f7o6P5wsJgxYTg8bqiPZpUD9jPRZgGFrsPR58VPPv7HC67vbqv..oDvHL7YJoQgky6xxAqxmNdMBjAUpzuWfX1+h9zVRnkZjgT+0dKNzRii5J7EUaLdtRG97K6LMmaER+basgz2i+VKIuUulWwYqjdw5fAVhnSWgVXgZYhknJs2CwrXgSc9+qfQfUe1zZHYN8Zqcblwese9499R5+qUI83cJwAitM5RE+AnonFQnA4bHpDY..FPAvXD.EqtZgxhgjPCcPPHABHo.DJ.hPNsB3vt157myNF.gPB9Vj+tVZBqzLJlKLhamVU4ptM14zwg9MChyOwMokCbeMmLD2g2OCyyt1VOQdjWk3ED.Gg4fFq+kXLnHBpAcVv9MAOjK4KhD4YgmJk+IT+l77Q47nTcdWUsbpgqZ4nDn18qNRHP.q4WyI+vfS0cUsAUarDQ.Rdsct+1Txu+fvQhjYpg0QApxV.KCAjMqt2roWfAjDofEMAX4a3gUCtBUzfnEhcT2Qo4hYGTHdzRw64g6ag5OgTrJp.leDGrRXBYtpUbi7kvatAx7LIkKxHsIWC9CKXMspOGvoVg+ODJSaIXHEY9.NAlF+LiWjEPMjTInzvQ+g5+Q8Pjn6qJP+UbgbSPtLJ27mo4qFygPJWHdWvsl3+HnwmEEGNAKfIvy6JnaD.rEyQfMlGotYVXfUFiTLLAzOvX4yHGvUOPoi...lNB..v5H...");
	}

	void bindCallbacks() override
	{
		
	}
};

struct WelcomeScreen: public multipage::EncodedDialogBase
{
	WelcomeScreen(BackendRootWindow* bpe_);

	BackendRootWindow* bpe;
	
	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(WelcomeScreen, populateProjectSelector);
		MULTIPAGE_BIND_CPP(WelcomeScreen, browseSnippets);
		MULTIPAGE_BIND_CPP(WelcomeScreen, createProject);
		MULTIPAGE_BIND_CPP(WelcomeScreen, openProject);
		MULTIPAGE_BIND_CPP(WelcomeScreen, loadPresetFile);
		MULTIPAGE_BIND_CPP(WelcomeScreen, startupSetter);
		MULTIPAGE_BIND_CPP(WelcomeScreen, setupExport);
		MULTIPAGE_BIND_CPP(WelcomeScreen, showDocs);
	}

	var populateProjectSelector(const var::NativeFunctionArgs& args);
	var setupExport(const var::NativeFunctionArgs& args);
	
	var showDocs(const var::NativeFunctionArgs& args);
	var browseSnippets(const var::NativeFunctionArgs& args);
	var createProject(const var::NativeFunctionArgs& args);
	var openProject(const var::NativeFunctionArgs& args);
	var loadPresetFile(const var::NativeFunctionArgs& args);
	var startupSetter(const var::NativeFunctionArgs& args);

	Array<File> fileList;
};

struct HiseAudioExporter: public EncodedDialogBase,
						  public DebugLogger::Listener
{
	HiseAudioExporter(BackendRootWindow* bpe);

	enum class RecordState
	{
		Idle,
		Waiting,
		RecordingMidi,
		RecordingAudio,
		WritingToDisk,
		Done,
		numRecordStates
	};

	RecordState currentState = RecordState::Idle;
	uint32 recordStart = 0;

	void recordStateChanged(Listener::RecordState isRecording) override;

	void bindCallbacks() override
    {
        MULTIPAGE_BIND_CPP(HiseAudioExporter, onExport);
		MULTIPAGE_BIND_CPP(HiseAudioExporter, onComplete);
    }

	var onComplete(const var::NativeFunctionArgs& args);

	var onExport(const var::NativeFunctionArgs& args);
};



struct ScriptnodeTemplateExporter: public EncodedDialogBase
{
	ScriptnodeTemplateExporter(BackendRootWindow* bpe, scriptnode::NodeBase* n);

	void bindCallbacks() override
    {
        MULTIPAGE_BIND_CPP(ScriptnodeTemplateExporter, onCreate);
    }

    var onCreate(const var::NativeFunctionArgs& args)
	{
		auto isLocal = (bool)this->state->globalState["Location"];

		zstd::ZDefaultCompressor comp;
		MemoryBlock mb;

		auto copy = node->getValueTree().createCopy();

		DspNetworkListeners::PatchAutosaver::stripValueTree(copy);

		auto t = isLocal ? BackendDllManager::FolderSubType::ProjectNodeTemplates : BackendDllManager::FolderSubType::GlobalNodeTemplates;
		auto root = BackendDllManager::getSubFolder(getMainController(), t);
		auto xml = copy.createXml();
		root.getChildFile(node->getName()).withFileExtension("xml").replaceWithText(xml->createDocument(""));
		return var();
	}

	NodeBase::Ptr node;
};

struct AboutWindow: public multipage::EncodedDialogBase
{
    AboutWindow(BackendRootWindow* bpe):
      EncodedDialogBase(bpe)
    {
        loadFrom("2356.sNB..D...............35H...oi...Hj.........J09R+fkGG0eD.pvFvSzBrt51FxS.GJgHRHiPo0VZsovYzaj2Fo6TvC3gTyQd7C19ewiPqeP+Gz8yVfF.NALR.zDPusXP9misArhhI60WWKzgIT5iHMrUYQEy0a5wLJXsUUqCBOcaIpGicJlWV6VHEX.QhHQwn8WsIn5Xq2ZQ.EbAATL+i+ma8avOA7vvKBEfe9Cyo0Ak80GgWDtvKBRIGy7HDhae+EvCGvIzwvsW9yrlne5m1ONrfvLtsxcU3POYwo395HTQnj77VFYMN8V3lqgkcGdaA8V7Dx1MONbe+USQbD4oi9SuJiCEiCI2ejgdDCGXP0.vR2h411hghXYFRlbYhDVtvu1csrF+5OQlTQhJQh70xcXW09WjfxEHjHhJRLghD+F7ewhDWnHAEdsqRDUnXIhI3K2BhIUpPohjHlPQw3HNcmqIqJBUDRTLNXKzhd33.XcjEAQ9VQF3x4S7vg+6GGJFG+yovgppFfKnXbr+tgcmdQNTM.WnFPdHsEYn51niUorbYGGU10P+UNNkJSgcyeJk+7+ZrVcSLVN0StmKasQ5AnjNuZ6jTylU7kd8VUZgdqenq+z091j9tdV83GOKUYqYqCS+U6ULOPWEYLAmMj3nIiDT.JTnH.8pctT2.F4qnWs4+6UPnXf9C40YwcSKvFSgRhhgsPK0Bb47JEioMNyrzKWyN3VKRBNbb09wAEoBFzHqlc9tB84lbMQO++4Dczjql981laoeWw1z4ZhDwE6p1GGO+F7HGX.ClMf7LwEtbMRM990add+VuIEOriVxszlfW1efmF4PUHvwKWqoIRDKbmHQXPl61dCBfpr0d0R8W+2ZRm9I02HkrErzcZ1hM1imwO5PSbqgVVG+I70a.iDwDqEJ8muoXN4R1yctTeHudtEK0JTRxfZgNTfBk2xauatX7kdZBkxM8Q1jC0WN2z2iRIYGbakR45rT90tnio7V+5o0wW5qOKBhJUr7ACKxHhDGK7LhjCx7gyFq94ZvSNGCI2.Jt3K25JtczWyUu6jpad4aQ1bgEjZ.Ej1vYyEZ1DJSGkhkSG+dqlktM25M1sIP7gCHPY5ngTCxXQz+MxONSkZjgJ+qk6bqjsZujh45+l91pr1HKUlHhiFOWnAIMd.cQGLbvmKvfjEY.xswICZzmLgxfCHRi39e+SHNdFM5hOgL4WulA5yc8VZaPOE2XU6TLvHCJGf.4h+8BNevBkKVd3AuulKES8vCXjv+0ZPBVMzecaFcLHr4QLqY7ku1j6bukikVM045GLbJ0WNWJVSQV6YMFLRTFYnfDmMY7I..LD.IHgnKkrkGgOV9rjk8FrJWKpuubR+WqWuYJjqT4awXXrF4qm5.b5wJft+Qj9nBkAMnxStS96kIzEdBwYCHNgHUAnKBw.VlgCQnpPAAWoYuFzROtsdq9V9WNZ8lznTYuQ1KbCN9bo0jCspY0Zlh+C7LQlQbzHCCHMwAAX4gJvj3RwoEIRXkjSRPp.CwpiOuswE9PjITt7uszy8pU4nuoWKGhiyehwatTu3rsYkd5aWWA4Jtlh76ZL8ZmUnqTJX4ubpeu.Nj3nQnLzPJzETPhBMDQIyd8e43teMFuYi+Fbds5fkKUhfRDI5iHhXBkIUzGTnHYhkI5iIWhHABIMnkzHmk0U5pNK6MNA8ATiOMY+7nl7aUxR2cWuUuXHbszmydH4AIWr06jTc3F+5Vf1hZbYLiRgHQF.L.DDDCB.fghA4TlGHJrzjnvXPLEvvD...EH.fAjnJbBIf.5BLVm6QDYPFdXCXfjojfHv1gSCDEwDiZnWFg8GFdLJWDC3RzaEQHcMTzvFCFAyj8p.zXz5LPSpTjPkvqWffJHHObn2gyK33M4pJiUbxk0hX9dzufcucn.8QzeONFiNJLsOZPQt2ApgMpe.zT5Rxbi72ltm.xQKH+Ahb60z306Y0rSZWp34Yx.n21fJMgiGsT84snA..ZZIXnQcvnG5Y8rAm1SBEUvmHD8CGpgZ2dEvCXqQVnSzFtEGvZd0Dqt9dtQX.vlMb.81DzqRLuWDpESyL.oVi81muG2mNRjt1oclOEo2h8UkrMKdL25zKQE.YrXLBFA4QQMPzORpB680CKocGU+AvXlZ2w070bt6eQq.Q.S21ePBaa+VciRFNvaBYJrQk82Pj2uG4iB948ryhEZ4E05wwB7F8msIXR7RTh27CgBYZDZEPnxZxrtDYCvrxf.wyP1LjUFa1tQdxlzOpV+mMSFFDy1+zAwtucUiZ6YLHSS63bqIHxlssTnpz2YhFky6kvIbLgpVQxnSQuNnBeUXj3AcR6.8St8PG2DZ9HfDaFx0GjTwfayFkrpEezB0vMeE+IaeTP8ANVKc8UY1oroFcBCzHSSFJwhH6vnCcwoypvnHa6fa2FY3ZRsUCpsIKPGNg45a.yr9ifqAK4A6aG6P4djN6wyLNrgHCT8yB9KLwSZ6pYnbmWK.5GRn3jwCDn370IXqlDT0hcNY7YIpUEZr0EpkIldpYy4ByGD.yovWnLuRJdWjhamoUEAanIEid1F7uPJxdWBq3ANsEpP0bAf9GMeEXNtzql4ggHBeIjx.2axfM1kPffnOUV+6FrNHcLMfkw8ZWyeBvnH.wAAIj9rRnJtpPKt0IDKUH0NgAKNtIyJ8yGfRBthT.JyNRc4S135q4K3hUnIviubcqxXHgxUN6CIG34SlLG7Q.HLcFNRbckITK3ayaqv6ImyQV3CjvBRlRv3k.VCX3rHXhJNCAK0d.VSX69y9PwuOhQFiNDd8q0CMSpRsavnrOuMZ7iJHTQDUNnp+wjmW.dbeQosjl1CjLycrt30TmQ5lADmR7Nb2bQ6wHI.+oO9FZ6qVUQSjZUs2iSBzqj.vK6zdKyObAJKh8sNRbdrYqwnj4imygBgKWDi3J5HejgA4WqaqSyfAa8Z94.d7CtgFlPDXn27Gewc5NFo+SSN1HCsx.tiDVAeHPm62RDDXZW5T3MhjlGfsltebmOZiOJl7FFmdJrbr5EGTE6uDD.a9irQ+UqzBQMnSJl5KMDI37ADx9.NJ4XngBsl4yCVcbsfB3oIdQVL9GN27UTkT+uIqaJQeBXuCT5H..foi...rNB...");
    }
    
    void bindCallbacks() override
    {
        MULTIPAGE_BIND_CPP(AboutWindow, initValues);
        MULTIPAGE_BIND_CPP(AboutWindow, showCommit);
    }

    var initValues(const var::NativeFunctionArgs& args);
    var showCommit(const var::NativeFunctionArgs& args);
    
    URL commitLink;

	JUCE_DECLARE_WEAK_REFERENCEABLE(AboutWindow);
};



struct ReleaseStartOptionDialog: public multipage::EncodedDialogBase
					      
{
	ReleaseStartOptionDialog(hise::BackendRootWindow* bpe_, ModulatorSampler* sampler_);

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(ReleaseStartOptionDialog, initValues);
		MULTIPAGE_BIND_CPP(ReleaseStartOptionDialog, onPropertyUpdate);
		MULTIPAGE_BIND_CPP(ReleaseStartOptionDialog, onCreateScriptCode);
		
	}

	var initValues(const var::NativeFunctionArgs& args);
	var onPropertyUpdate(const var::NativeFunctionArgs& args);
	var onCreateScriptCode(const var::NativeFunctionArgs& args);

	WeakReference<ModulatorSampler> sampler;
	Component* root;
};

class NewProjectCreator: public ImporterBase,
						  public multipage::EncodedDialogBase
					      
{
public:

	NewProjectCreator(hise::BackendRootWindow* bpe_);

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(NewProjectCreator, initFolder);
		MULTIPAGE_BIND_CPP(NewProjectCreator, importHxiTask);
		MULTIPAGE_BIND_CPP(NewProjectCreator, extractRhapsody);
		MULTIPAGE_BIND_CPP(NewProjectCreator, onTemplateSelector);
		MULTIPAGE_BIND_CPP(NewProjectCreator, writeDefaultLocation);
		MULTIPAGE_BIND_CPP(NewProjectCreator, onProjectNameUpdate);
		MULTIPAGE_BIND_CPP(NewProjectCreator, createEmptyProject);
	}

	var onTemplateSelector(const var::NativeFunctionArgs& args);
	var initFolder(const var::NativeFunctionArgs& args);
	var onProjectNameUpdate(const var::NativeFunctionArgs& args);
	var writeDefaultLocation(const var::NativeFunctionArgs& args);
	var createEmptyProject(const var::NativeFunctionArgs& args);
	var importHxiTask(const var::NativeFunctionArgs& args);
	var extractRhapsody(const var::NativeFunctionArgs& args);
	void showStatusMessageBase(const String& message) override;

	DialogWindowWithBackgroundThread::LogData* getLogData() override { return &logData; }
	Thread* getThreadToUse() override { return state.get(); }
	double& getJobProgress() override { return state->currentJob->getProgress(); }

	bool didSomething = false;

	void threadFinished();

	File getProjectFolder() const override
	{
		return File(state->globalState[HiseSettings::Compiler::DefaultProjectFolder].toString()).getChildFile(state->globalState["ProjectName"].toString());
	}
	
	DialogWindowWithBackgroundThread::LogData logData;
};

struct CompileLogger: public Component,
					  public CodeDocument::Listener
{
	Console::ConsoleTokeniser tokeniser; 

	CompileLogger(ChildProcessManager* manager):
	  parent(manager),
	  console(manager->log, &tokeniser)
	{
		tokeniser.errorIsWholeLine = true;

		parent->log.addListener(this);

		console.setReadOnly(true);
		console.setLineNumbersShown(false);
		console.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFF999999));
		console.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF222222));
		console.setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
		addAndMakeVisible(console);
	}

	~CompileLogger()
	{
		parent->log.removeListener(this);
	}

	void codeDocumentTextDeleted(int startIndex, int endIndex) override {};

	void codeDocumentTextInserted(const String& newText, int insertIndex) override
	{
		auto numLines = parent->log.getNumLines();
		console.scrollToKeepLinesOnScreen({numLines - 1, numLines});
	}

	void resized() override
	{
		console.setBounds(getLocalBounds());
	}

	void paint(Graphics& g) override
	{
		
	}

	

	ChildProcessManager* parent;
	
	CodeEditorComponent console;

	
};

struct NetworkCompiler: public EncodedDialogBase,
						public ChildProcessManager
{
	NetworkCompiler(BackendRootWindow* bpe_);

	~NetworkCompiler();

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(NetworkCompiler, compileTask);
		MULTIPAGE_BIND_CPP(NetworkCompiler, onInit);
		MULTIPAGE_BIND_CPP(NetworkCompiler, onClipboard);
		MULTIPAGE_BIND_CPP(NetworkCompiler, updateNodeProperties);
		MULTIPAGE_BIND_CPP(NetworkCompiler, checkProperties);
		MULTIPAGE_BIND_CPP(NetworkCompiler, toggleIsPolyphonic);
		MULTIPAGE_BIND_CPP(NetworkCompiler, toggleAllowPolyphonic);
	}

	void setProgress(double progress) override
	{
		if(auto j = state->currentJob.get())
		{
			j->getProgress() = progress;
		}
	}

	var onInit(const var::NativeFunctionArgs& args);
	var compileTask(const var::NativeFunctionArgs& args);
	var onClipboard(const var::NativeFunctionArgs& args);
	var updateNodeProperties(const var::NativeFunctionArgs& args);
	var checkProperties(const var::NativeFunctionArgs& args);
	var toggleIsPolyphonic(const var::NativeFunctionArgs& args);
	var toggleAllowPolyphonic(const var::NativeFunctionArgs& args);

	File getNodePropertyFile() const;

	bool checkPropertyMismatch() const;

	var allNodeList;

	bool rebuildNodes = false;

	BackendRootWindow* bpe;
	ScopedPointer<ControlledObject> compileExporter;
};

struct CompileProjectDialog: public EncodedDialogBase,
						public ChildProcessManager
{
	CompileProjectDialog(BackendRootWindow* bpe_);

	~CompileProjectDialog();

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(CompileProjectDialog, compileTask);
		MULTIPAGE_BIND_CPP(CompileProjectDialog, onInit);
		MULTIPAGE_BIND_CPP(CompileProjectDialog, onExportType);
		MULTIPAGE_BIND_CPP(CompileProjectDialog, onPluginType);

		MULTIPAGE_BIND_CPP(CompileProjectDialog, onComplete);
		MULTIPAGE_BIND_CPP(CompileProjectDialog, onShowPluginFolder);
		MULTIPAGE_BIND_CPP(CompileProjectDialog, onShowCompiledFile);

		MULTIPAGE_BIND_CPP(CompileProjectDialog, onCopyToClipboard);
	}

	void setProgress(double progress) override
	{
		if(auto j = state->currentJob.get())
		{
			j->getProgress() = progress;
		}
	}

    void refreshOutputFile()
    {
        auto targetFile = getTargetFile();
        auto content = targetFile.getFullPathName();
        
        auto w = GLOBAL_BOLD_FONT().getStringWidth(content);
        
        if(w > 480)
        {
            auto root = getMainController()->getActiveFileHandler()->getRootFolder();
            content = targetFile.getRelativePathFrom(root);
        }
            
        setElementProperty("OutputFile", mpid::Text, content);
    }

	ScopedPointer<DialogWindowWithBackgroundThread> dllCompiler;

	var onInit(const var::NativeFunctionArgs& args);
	var compileTask(const var::NativeFunctionArgs& args);
	var onExportType(const var::NativeFunctionArgs& args);
	var onPluginType(const var::NativeFunctionArgs& args);

	var onComplete(const var::NativeFunctionArgs& args);
	var onShowPluginFolder(const var::NativeFunctionArgs& args);
	var onShowCompiledFile(const var::NativeFunctionArgs& args);
	var onCopyToClipboard(const var::NativeFunctionArgs& args);

	File getTargetFile() const;

	int getBuildFlag() const;

	BackendRootWindow* bpe;
	ScopedPointer<ControlledObject> compileExporter;
};

struct ScriptModuleReplacer: public EncodedDialogBase
{
	ScriptModuleReplacer(BackendRootWindow* bpe_);

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(ScriptModuleReplacer, onInit);
		MULTIPAGE_BIND_CPP(ScriptModuleReplacer, onReplace);
		MULTIPAGE_BIND_CPP(ScriptModuleReplacer, selectAll);
	}

	Array<var> allValues;

	var onInit(const var::NativeFunctionArgs& args);
	var onReplace(const var::NativeFunctionArgs& args);
	var selectAll(const var::NativeFunctionArgs& args);

	BackendRootWindow* bpe;
};



struct DebugSessionOptions: public EncodedDialogBase
{
	DebugSessionOptions(BackendRootWindow* bpe_);

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(DebugSessionOptions, refresh);
		MULTIPAGE_BIND_CPP(DebugSessionOptions, onExport);
	}

	var refresh(const var::NativeFunctionArgs& args);

	var onExport(const var::NativeFunctionArgs& args);
};

struct SnippetBrowser: public EncodedDialogBase
{
	SnippetBrowser(BackendRootWindow* bpe_):
	  EncodedDialogBase(bpe_, false),
	  bpe(bpe_)
	{
		auto c = "6950.sNB..D...............35H...oi...5qA........J09R+fQ3bEdM.paPTx3BnlAF7NH4vrPzVdGYhBQJZqxgqWqSKGdUX9B9QEjkEk5N882VqMEdc30ArVfyWVLvIC.x.5+n12Ql8lnJLI0R.MhfpfqA83NDrPiKZGfKf3uUQlYhX8ZVZe2xjzXXYPwwzHpk+lLjntwRMmA4m3P3aq1XB1oDMGn5HhrI7dfhUS79o2LpdD2osLNuC6DQvIRMhS197yr0pIpLmWJZbWUbBDCSN2PRbsDcfvgEPrStU8tMqZQ+ZwHAPKNxQXUOXHgrI0EEcZ8v8ZDoTottffRta4xAgls5GGbH1V3obiAR8O9k1VfnQiFlUJFcza0nNsysMqM6knnIypmJDra0DnzNmzQhFBOYtVg3nsDdsbrnxds5HQqhqU.sU2beNQXG4hFQIQUpc6iGVf2PXRlHdqrXAmpBhRT1jxItlfT0QVGaRd1GaCeIppJBVfpPW.UU3GCFlnJ9e4cAkibMEuHryiyjEfO++v3sAX3ZqC6J3S50edYNc4vNC.KgAxabG422W9i92gUbPNmoXV97mm6nJJ3CJ+MX35yweeeriN.POn+KwTEgeOsIGGWmP9NX9i6jp.w97mdnhOO6wijbr+yK4JHuAA6eIpBxROOOeeaPAn8uA97r1C6AKvi81cNyC5jmj1YN7t.oqqqBf5o2cxN8+uyUfO64onJ9gf5bHEE0Ab.zru+k4TnvhhpXr+K3fM2EnQCzeljqoJL6d3vdUUUAbv2i+jE.Ju+nJH6j+5B3f5MFjq87vied9m4cNUQH7djT6Ep.97u2+JfCP.DTE35hx.XNPOUgGur6A4zg4HK65eJ7e9BGfuvegC.MdMQUjO0jE748u+B7EV.9Bg75bJ.Ab7XaNGV4W1CgkPP23X1Fy02NHfFRKjGy0FjLHMw6ZReFCiJPBwwic5URytewOONp4cvS9cYQPbGHJHHu+5OWYEEDPl0eedr6uDb.Jk.xja72Fq6xiUxjWlnPPk3.hLWu8v4vLWKw.0UhKC9rGN+Hu4veDIrPTk.6JAkUhOn7GwgDHrRvgq2WXWVGvVj85cUTWWVWSXTVTED3K66dEjyfD1DUAA4ScEucegBPfGuKAyIP6erDBnJH3y6MHfRoTBXAnJH.+w17NzuBATJAr.Q1PSp4zp8n7VxowVVHQKqT04MCB8s5UBYVsLtOGKkIdp64MgRK2CWvagIth3wzawhkqANVnNbCEuI.YLEhQ+bqlYtop4+HiVBSZ1X2zn8.q6CGFrYciBR.sUQv7mgi6t.xvG0VVnjMfQRLsUK+5XNX3q8xXavn4E4d0EbrXVXAZnd.yexqRLDmIhVaJ8.ynkH6pqDlJPwcvXwV1kmiPiF.sI4IIMEEgXSppSgah31hKNzzfhJzdbjfEx3n4bRjqHWxdq5KthUwC5Iclj9rkUZFwRiHa+yi0bKgC6c+IsUIcTwINwSlUVSMyJ4d36ZMoJj30r9yLqXFJBYw1BEjxZXPeO2jt7OCWG7VZbpZxTh7rQTqC1zzJwSIzu5foUyx2XnENS6jPVbrPdqZ2tFUwsM37nZHKy4jMDnrwRgGIJRyi0TFOh2HKGrPHRh3nmDFIghS9zsf1sGbZnjt1Xug57l.4.wRwg+5PlBf0OWFX1pXfTUfDREryOofgQlMq1FQMHuIFBuX9aYDuLoOn85AxAv54EOgVxXorg4CwBYxe5h.uCbiD1FqiNM8KlXFE.MIqF.ubtk0sObBR92xR.yFysKM1x1jvAQJYRW1XsQQzbOjX2UngEw6EbR2eOvU6hWHfjro5ir8XSBzpVkswlcPiLLJDjNNWV1pBlUWqtKe6UCTEqpXBgl1jrvPHjZwUhPay5WEMgaRjXoSsIMR1r0a2m6zug1doHoBjv6DjsP161GMX96zAco8lgDyptYBA2VFGACQMPDQlIn48CsYej4qrWal8wkpG2SGUVrWwOQLp2L3cawGhMa1ObwFKwEHW++Wb9cv17377DbwiqsNXKEbTjlMxkuiKVYDhy63gSH0oLYPfTMHpqr3CUiF1Hhb1Icb2J7dMgRB7wG6FCNIvHxlEijzV8yeG29jfnBYXqFB3NPrHT4z30LA8Td2P3fAuaKlHt4TSRJCz9.bg+dZIeI7dR94D+.o04uVQ131aDLV8FemSt7vRWSBRv1IZxOYRR2KrG1mTl1QnIHRcLmQLWdaKpOLLYfZwngwB35NYP7XAik+LYmb3eBip.ZYwQjoMxgDrRr7HIz9CkFdsEDWJN6Ei2Qs6icRBJZST8MrcBBPl.ZsRWoaEkFi16MSRVEOquONjPZ99rkovGNq1lvZ1kpRFHu3E1iSU7hgmrOXKkKTldXvdwrDovASvp4EkORxKKdEP7w7fYQwccI+OLFiQf.Q0DNSq8GMcQrwof.XlkkoPLvzrsXyFJhTjjwfW+7h3Biu8dN9cfW4sWKodV0STlOaUMM5j3XClHf8odkCZeTIycuDmf2Pro5wLhlHW90NR1xPnDaOMVCq4rpB7IOD0Rni.QnChMaCxCeEn9VYQfU3YJ61BMZHh31tVYJwva0OaZulTnjUy7npspEABKB0NZieUfVHeTeprshZSzbytTXQRBkraYWYlVGbGVMEVXwRnpMrJT.qk1L+1qoinVTNjYnWFMKNs.kzjk4k8gUI0YXgYmX2jxn3gMuRMjxrmtrRn.g1bJ9L4Eyd3EeGwhfg167fErDheXJVBE1UnJgtCExNsTAxA4E+sBunyxsRRmUkDiyqW7yljT0xBoSDBIJh0NKIemOoLRZkQUVt6bDXDGOortKBd3nFahhZXn5q6vicu7lCqviyVrrn6QDFYVvv4iWt7iCEIUk8lgBnrpGUdizkUqZaeVQCiLInsYkXgw0NAzDSlU6RCDGWHR59yrZWxRTbhcBmqgMHFbBFKOvVJUvpfsS5roQl09g6du1h7vdqjYlsAQgrLX3bW91pb1IpDgtF3VNkLO0R.VuUG+PVq1hcjVqgEjL6hD.84d5Iu16iFbvpEJUeW934Lh30RAC0FvHYH.Fp4nDzewJ6rvdi7ZY0oDbO1r1KtkhBdF8boRiHEMeL.sWDf9tXaRvZ2rUH.qmFL0.odZuvyGzjRjnZqaj+f+99NHQhDE3AU4gn6rhEv7l+dMQZTDyWKCDIQQTlS2PQQ4ZWUhOcxQVDyntroBFdw9HsU2TojrTjzvx5hv1raYhRbo7n3saLxQVUONMJQXHz1WXSbzJDVVVUTTPctB7v8M2r6ccAB2AOwLyw1YNdr.RoP99uK601bN+qxj8c2Uc7hBaGvNuXuR7fE.fCP.FfQLHk9Ay0a8eWpQQOvm+bPtCOR14JBs0cveT5ATGvLq+j827G6MVRseek+rG7hufEegJ9BT7EnEPmjvTucYOj7BAnYPuL0ruM.chxyYy+mKUJyQpqFuTIAwTLI9m6P8x583XY3uCESyevjz8VaR+PnRG3py6u1B1.CLfL6QfEZFtn+xNG1.jjY8IePZLVxwYs2zm+P4ChA+hAi5SItF2w.RCH+xdCxezFrvlI0OLv64wfN8NItxvuFz.CTPN9+gecGtxJbHwVjPfPd3reqI6iZGTRL8i9+b.3tLGMzNyoiA9G.u68o+7KTIPBPwfx1DHA3YZnP6MtS+CG6zQpW24n6eNLCAfxdSg0uuSF.vc7XNcJ88O+WSybvZzON663djZIRxrOGZR2V7J8jSXmqX.9nUnn44QJzC9e4sD9iG.TuJSGX.FLXXhp2X8G+AGghYkjjvTNc5cGsr6Qyj379xwi05bRL8.P2YuROduiW1XxZhlUvASTX1SPyK6s6MzfcI7E33aH79la3wy3z6R3KP2XNGBegNdnJcRpOOtyOnjc5vQr2AeAN9tYtC1S26Ru1K6K+3buRh67vc9K8SlWvW.xM6SBhsLoXDCxA0jYOJjKzKZXLoxmrD2f5wwTxxaEKuBIYtq6Goz9em8PrP5PoeRduS1i9wgeJYdW500Lf5wKojF16LxO1m4YvRZ+iRqGfyoX.6.VJ4xdEJrSfrXA75+xQNDuNRIfmG2kI7iu4O5OdmSn2SP.z4K6bNtqR3uDyIXAEcxeH4O9WI2AyqP5yWC.EsxhJ48XGcEucOL2g22LMz16K6nYFdd5udq4zjZLU48FW4TH3yyeXLp4TwmAA09xMFyWygvQ87xF0+cuKoOgjD64gsP9fx7Nk6dcLjkEn.vydusNrZffGFLXkAruwZKAXrbrORI0geZGs+olzxdHZ2RRl0J8+ZGu9i2qlYMlu1209anEG3jEgnaUwBBxWQi8SFSoZaw+DzpkHKsY6d.gK1KmC9VsSB4JE3XmfASfgRf5XHFmIpjoXL1Lx..A..LQ3ffDItXwhmRbQ09vbSELR9rPTfPTPTL.LHAFAFiAfHBX...PHLCPCLFG.CWeB5wMzy6sXx7MXjeM3t+Rr8qnAyj8M3VZmDGirAY82anIW2WVjjFU.dSNroTks+h4UPDQvA2ho5cmA06hv85jjpydDRAR5Z8iGkVTK.Xv1NkmBOuO.8jSOQkBK1tCUXZUhF2xTd0sbcD.sRLGEA2VNAXu3+yI0Loof4O1m0IJTR1A+4l3hhZBNW9FtQcuN.3rDb+bS7yQLqHDnwbiu7xfowYyzYbSaoZBllSjnnQYD1HqObGck2vi7MIQUanXite8ujJyeEqBhWTQAAmWx+6bp60U...HMY7fykFf2e5wV3si3AVhVdK7DWTfBWb.rXfF+oKpRd600v4xvyy2XPt3f4I.Z4an.bsBmCdI.cwac00..7W4BV34rM.afiEjnVNXq4uQ1EsMOwFiA9bPYgIvLf1ILGP5vrQUYxdPqaZJBjBDxKOTGX1OicjqfK6iza4Oohe3ay9gMQF3AmmCj1bBGGkAN3i.9ZuIvOaFc3x61nhdRiCFyvT5UidlJEsiZsuCxAWSXsQV7Dj01rsPBaLiXFfbbP2Rd0OR00LHCdjXM.hpvGlv2AafYlituG.zNykyVTRrUxhGvlwaoKHfcmzxmGkJarpAzcXrbAEw+lj8iN4t00LVQeWnN4x6PZ54EGd+wgVYbqZ+dxpj.4WY37iwVp16mIH5+X1k0+GEZUNeyjL7+YvDnht2I4PYdFJnf3ILPJb1xPNUrcvvu3RVcAVH8yuvIeSfxMAvS+I9n4O1Br.bFAyJbwj9kPWTVgWBNkSN64o9sItvXl90eREhKHxM.9S7FLGTHGshjCmj52Cl8ZM7eQIICkFWnO+EhVKUMrLgk9JsdRiNNtYmIXDFVkbot.oVQGX.GDyVnsipXNq0qCySItoDUeSf71iy0hiWRVIKbh1zW4iaub2BhRDvZnTl2pggGgy0ZErNzLdzEmP2UplSB7XDZLQhAT+cef5jcEO1tYcKskp9Z0rRbtDR+xbVVLvTtx1zbtQ+N6UTG4SK2SEXP76wSfiJM9Ea5xed88+z122W82Wr7uuMcMRKidtgfd5r67dmMXIKPTvbadcA24xBfgZcIEM9q7J0tRuSG8fIC2wSsDe+N2HWA3nBuHtCLVn2EF74vg+vFEYmzE60a7utIoBAZ+iAG2LKdbk3Xd.LUYIjdGltqBjrNsE5cQ7MctFXn.klIEzr1kfktgjkyDaLTFyGTBDhjtUuRP6ZpHox8KBkyt4Dji34WmqyB2CC1ZxzpoWFsGy1N7ymBa7t1Bw+0KRQG0ghFbC4s0bN4rkSNSO+xSXDdYaxyhhAATGtjRYUmKNdyMKCc.PIGqA3QwXyjo0XVG9SrxtHBZjfuVDF.qzixPIBRCm.nlbT55OIQQamFqljKvGKVWHemeCvSRHg2bBwiIw34a1XCe7yBPSGk72pfTmNDi0lwB.Zhf79BPzNtpLZyQ6LMTS2WX3bj7FpaZyvkObt3LCV.dj2trzR3HPTB7bwKk7qZPKDPxtVmunBqMdrdgqjzcc+nVDIfhJNIiOKcrccWP8V5M9btuybSiX39L6JsmJCpBGuiMQFRWROiFZJskREMQRBwWfaH51j+6.mrVciUQmvF1TC2SWJeWmte+5nAI.QHD5lGbBNzm.i5.jJvEY3k7DkOAqxr+nsI5qOadAIvCsW4UMMgThweSrja9ThBBPjNPP6fi3xNdG9Aers.mFiAaoiDCrO.YEi1Qax9T4jhkJSmvgh3ZwKhz7f4ZQ.2SHpmlhBlm.sGFB3RyX1a0cFgAqLX55iaL+PiZQp8aGfkuaGQRt6yKQwqbtks0Pp0G0wTodqyvcXxr51OZDbkCgF6guVpVRUDqBuWRZGXRYEI6fY1T4czdoYwEkilcbSJEvoPewGryu2Nnz6DmqRwzW.RT7RfeW2jtUs7ODR0csTvVXaG7VlUwBhWd1a.C9zYVw8XIm4GNyi8NiY0ZBjZw1+HiWrCoJlQW7VsAdittBCNbGFLFRRxGOQ+sW3OPGCmnhBBDV+RZm8hS4AFQbVR0LHrHzrHpxbcy4gqLj7.gYuYhal0.bRwOe6+.LAdzassOO1e+7SH2NOv7k6EnS70Tf.DqU0RnGS.laO60nBD3qmNsQ3sTjGc6whKXDdAuR9tY6yZT7kJyedsFh1xDPUQeZK4rtfCzAnuk7b1T40IACKwToQwkov..QfyG0HkxnIzMbHXY1uRjDBHqD9bh09s3VnaaqB0arYOvjXOOjnigUx4SDDe9GWbs3.vH7Q5LoXuOf5WX7DKq79BKVIA+zqLtrkAbofdCr7kVFVJXMbSPUErv.TRdrZPLjo+Ks36iF0Y97IXEhhgh3AKjVfAsSL2gDS3Cq3m6cp985Dgt+8GkhZPEVF2KCZTTEfgJ+Cu6OG.TkNDg+.ZXF5EiJveGOmMt6G5XYtdMNsQP4G2e4a8xGEUxpkBvf.Wr4XF8w9CNPcSSrHjdG0fzss2c+7SnYYOh+JkklA44boKt5H04PD3ky5vRAeihste6mQFNl9CJagRDzEEc.jz9DX2k2D7S6HYlvdH.DGdzDnKr.lRIzAiin3s1oOvvtRqd1oT9aoSbeI2hvuLW1HyuIvoo6RN5aVR3wLfEzxeHFB3r8FGAx30TdfQkGD6hV.Pr3ZeZ6jkJrOR4kgqb2R7uTk2ceG5X10q6sHSW.koC4jCv2e8vo9xMHcnU7jOJhsptqXVuNvBCm1itf6qIaBWp11uVnh4nuZOFhbsOeaKt96NaiVyP42AuDx+Cp6sAf+F9TRv.knhnDatun36Njd1R8KvOVL9CCzoCjXVkGQ5MNnL6u9.JsjTMFHAXTKIdB3.SPpYONbcq51SvvTa0IiwfYvanLuooXSZnGDDIzEWM9MZiptuonyP76c9SUpt4F7Pe.z2uLQwYnYJ83y.XxZY5q7zmG2HSMMq8IsP8H+z7DjU4jd1.DyhWElrWu7gn3NH+z8Bh89sWOeh.KpEn2cIFjSSnVVg1Fzjxr4Pfqv9+8yaFtW9H1aCLlvFk4hnbWAiEfOt+HOLhplSfoDQ70J4yj9vO6Eq90c4OapVcRYxJyc6315OLxi1CcxikPW3YnYtp4Jx4M2bQBFb51C8.qmhfDns1vvEoCif+ONMh4Y2TIAdMg.O6PjmOJBsLE1YVolop6lmqGnsnf.XfFk5PLq9PCSQVj44.SvTsTqX8.uPRKf5yeubEUghtt1LVsBNFNqFTt.OOarQzUN52svW51Q6+8wp399yJv5eY2cvF3CnaeWieRVL8JKn.vKs1mWqOcE1iDRszORZmsohYx4OhFIC9MT2YpU4vVw0BB4Ukqxx7M17WdmTCGn5IRSnAWWw+JA92psg7kJW4L8OU5mMf+fp7zhNtZHlCc25BbUr6fPyrIW+D4mqeQBuc2WGnu4.bd+7uP9IZy1.qms7dhyvbevlMOy7uYRzrenVIm+779qR3ZwsXdM+4k2GgQ7m5uL6ofrIetLMukLkjN0+UlPXjEWPmhHthPUlDKNwv5CEqma3dinLyCmVA3KZ.qVF+LsPny4oiThXlD70rJtsvdlyUTVmpAdm2yCCp8clmb4RmZi+B53LYdksi01+cEB8BukxvbS.OUmNnIHzgCSk8nT+JAOPTI6KGmHTyyA9p8aQJd2GOtXd7+.tfoYEx44B7zf9ZR5MFzymWdxqvcBN2k48+lG6CSOkLD08PZtxire9Jjrm.tAaIOdIlQzyHJVRt4AE3tXAIqUUvQpiekp9zuC0ezuwb13KjOq72Irmy74XKOP7EuzWHIPRp9fb5qFi.v+kdoOKZO4p4+doY4V82EmzYyZVo47fLh1RfWShClJUbbZs1aX+7HIuYYp0UeeXan.mcseBk6E6+30a1iMIGB7SzEmGgXaZG9OPKNpSS.WLON8GicVZMr41FHl4abZbaBgyCWQxopuz4HqH3G7eRuuvVgXWrzbB1lH.pe+tdE69kSnsXBB67BtywMf9j.WlRQltFYOYnLB6HAn.rnmxQeAFDC4+zuiOv4O8YbFLJ.z1Jvl3qs+NTZ4oKI5EfjClAe5aE423ctmuY9Txdqs8pt6hVDrQ5DTj2QIsp9Se4RRLLzsdH4ZObSadqmKIZ0mOrvkk4KIHl3WFZmwK3j.VnSxoEd7oFjf7d0W8tE2NF7.vsPTCYPowwLPOs3Pi9Yel9U98XA.+u+KMEGKLD9DAI2phJgvjm8bLoFHayTqLjls7Slbi4i9aQmrFOGfEVIgGnnS59twGQdtmDe3pgyC33WcCFAQwImdyPb8yumXZ2IEvqfg.RdIht+BBsh2LeJeGOFRGt+4.5bwM9wX+YCWAG+7Wtg8u1X4uN4nlRPYXLPWISDQsFqnBdjPUtNGPe8d+AZBN0mRKEspuoJL.WvPFjfM7KA2Bn7B8lgxDJgMq8.fN1xB6uUk9ulrREZFP7KWnzHlPE9vG1XuM3OmLKHGO5KMjb0eyLCMqnOAc5DtkjrRvHj9KR4.uca2wp.gtf9ipa8.qneDE24QlmPnZMXX2atYSt81an5J6mwYNKF84Z4gD.twhQHBwO6hcUHytc.Infq1exxFTUHr+NaLNTJ00XWM+rknKCaIvRbwkJ2KM2ZfxE6oIWVDkpAkmM5xTrtf3pB0sS4K2XXBoyofXVKcbX+qUiUftNSvqLXjh1+5yawGtYMx726lFTWg5BwvlqyBqpN1HIRwJgAvnvJdMYNbHll5DHuNBgGjkkqhf5pZwZYHWpjUE5kolb9mHzvD6In82Hwe8EH+vSQjWQQ3U4O5EpnbilvBSaI.Um5oVPrcYbGmyQG94lfyog63n.lWCuHmcf5QdEJQ5lTFcfi7AjeVCC+Poi...lNB..v5H...";
		closeOnEscape = false;
		loadFrom(c);
		
	}

	BackendRootWindow* bpe;

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(SnippetBrowser, rebuildTable);
		MULTIPAGE_BIND_CPP(SnippetBrowser, clearFilter);
		MULTIPAGE_BIND_CPP(SnippetBrowser, onTable);
		MULTIPAGE_BIND_CPP(SnippetBrowser, showItem);
		MULTIPAGE_BIND_CPP(SnippetBrowser, saveSnippet);
		MULTIPAGE_BIND_CPP(SnippetBrowser, updatePreview);
		MULTIPAGE_BIND_CPP(SnippetBrowser, initAddPage);
	}

	bool initialised = false;

	var rebuildTable(const var::NativeFunctionArgs& args);
	var clearFilter(const var::NativeFunctionArgs& args);
	var onTable(const var::NativeFunctionArgs& args);
	var loadSnippet(const var::NativeFunctionArgs& args);
	var showItem(const var::NativeFunctionArgs& args);
	var saveSnippet(const var::NativeFunctionArgs& args);
	var initAddPage(const var::NativeFunctionArgs& args);
	var updatePreview(const var::NativeFunctionArgs& args);

	Array<var> parsedData;
	var currentlyLoadedData;
};


} // namespace library
} // namespace multipage
} // namespace hise
