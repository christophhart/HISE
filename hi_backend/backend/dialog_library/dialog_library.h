// Put in the header definitions of every dialog here...

#pragma once

namespace hise {
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


struct SnippetBrowser: public EncodedDialogBase
{
	SnippetBrowser(BackendRootWindow* bpe_):
	  EncodedDialogBase(bpe_, false),
	  bpe(bpe_)
	{
		auto c = "6937.jNB..DvF........nT6K8CFYy060.n5AEJyKPhFXv6fjCyBQa4cjIJDy.JJGtdsNsb3UgYwPYwJj817aica6IoIRD.....iHE7xFCjx.iLf12Ql8lnJLI0R.MhfpfqA83NDrPiKZGfKf3uUQlYhX8ZVZe2xjzXXYPwwzHpk+lLjntwRMmA4m3P3aq1XB1oDMGn5HhrI7dfhUS79o2LpdD2osLNuC6DQvIRMhS197yr0pIpLmWJZbWUbBDCSN2PRbsDcfvgEPrStU8tMqZQ+ZwHAPKNxQXUOXHgrI0EEcZ8v8ZDoTottffRta4xAgls5GGbH1V3obiAR8O9k1VfnQiFlUJFcza0nNsysMqM6knnIypmJDra0DnzNmzQhFBOYtVg3nsDdsbrnxds5HQqhqU.sU2beNQXG4hFQIQUpc6iGVf2PXRlHdqrXAmpBhRT1jxItlfT0QVGaRd1GaCeIppJBVfpPW.UU3GCFlnJ9e4cAkibMEuHryiyjGfO++v38.vv0VG1Uvmzq+7xb5xgcC.rDIPdi6H+99xez+NrhCx4LEyxm+7bGUQAeP4uACWeN966iczA.5A8eIlpH76oM4335Dx2AyebmTEH1m+zCU74YOdjji8edIWA4MHX+KQUPV54446aCFf1+Ivmm0dXOXAdr2tyYdPm7jzNyg2EHcccc.TO8tS1o++ctB7YOOEUwODTmConnP.Dfl88uLmBEVTTEi8eAGr4t.MZf9yjbMUgY2CG1qppNfC9d7mr.P48GUAYm7WW.GTuwfbsmGd7yy+LuyoJBg2ijZuPC3y+d+6.Nn.ffZfqKpD.yA5oJ73kcOHmNLGYYW+Sg+yW3.7E9Kb.nwqIph7olrfOu+8WfuvBvWHjWmy.Dvwis4bXkeYODVBAciiYaLWe6f.ZHsPdLWaPxfzDuqI8YLLp.IDGO1oWIM69E+73nl2AO42kEAwcfnff79q+bkUTP.YV+84wt+RvAnTCHStweartKOVISdYhBAUhCHxb81CmCybsDCTWItL3yd37i7lC+QjvBQUBrqDTVI9fxeDGRfvJAGtdegcYc.aQ1q2UQccYcMgQYQUPfuru6UPNCRXSTEDjO0UL.AGfOsOWo.RNthAva2WX.B5erDBnJH3y6MHfRoTCXAnJH.+w17NzuBATpAr.woDOXjMzjZNsZOJukbZrkERzxJUcdyfPeqdkPlUKi6ywRYhm5ddSnzx8vE7VXhqHdL8VrX4ZfiEpC2PwaBPFSgXzO2pYlapZ9OxnkvjlM1MMZOv59vgAaV2nfDPaUDL+Y33tKfL7QskEJYCXjDSa0xuNlCF9ZuL1FLZdQtWcAGKlEVfFpGv7m7pDCwYhn0lROvLZIxt5JgoBTbGLVrkc44HznAPaRdRRSQQH1jp5T3lHts3hCMMnnBsGGIXgLNZNmD4Jxkr2p9hqXU7fdRmIoOaYklQrzHx1+7XM2R3vd2eRaURGUbhS7jYk0TyrRtG9tVSpBIdMq+LyJlghPVrsPAorFFz2yMoK+yv0AukFmplLkHOaD05fMMsR7TB8qNXZ0r7MFZgyzNIjEGKj2p1sqQUbaCNOpFxxbNYCAJarT3Qhhz7XMkwi3MxxAKDhjHN5IgQRn3jOcKnc6AmFJoqM1anNuIPNPrTb3uNjo.X8ykAlsZUARHUvN+jBFFY1rZaD0f7lXH7h4ukQ7xj9f1qGHG.qmW7DZIikxFlODKjI+oKB7NvMRXarN5zzuXhYT.zjrZ.7x4VV29vIH4eKKALaL2tzXKaSBGDojIcYi0FEQy8PhcWgFVDuWvIc+8.WsKdg.RxlpOx1iMIPqZU1Fa1AMxvnPP53bYYqJXVcs5t7sWMPUrphIDZZSxBCgPpEWIBsMqeUzDtIQhkN0lzHYyVucetS+FZ6khjJPBuSP1BYuaezf4uSGzk1aFRLq5lIDbaYbDLD0.QDYlfl2Ozl8QluxdsY1GWpdbOcTYwdE+Dwndyf2sEeH1rY+vEarDWfb8+ew42AayiyySvEOt15fsTvQQZ1HW9NtXkQHNuiGNgTmxjAAR0fntxhOTMZXiHxYmzwcqv60DJIvGeraL3j.iHaVLRRa0O+cb6SBhJjgsZHf6.whPkSiWyDzS4cCgCF7tsXh3lSMIoLP6CvE96ok7kv6I4mS7CjVm+ZEYiauQvX0a7cN4xCKcMIHAamnI+jIIcuvdXeRYZGgj5XNiXt71VTeXXx.0hQCiEv0cxf3wBFK+YxN4v+DF0An+ikPtRAN1IXvXYwQjoMxgDrRr7HIz9CkFdsEDWJN6Ei2Qs6icRBJZST8MrcBBPl.ZsRWoaEkFi16MSRVEOquONjPZ99rkovGNq1lvZ1kpRFHu3E1iSU7hgmrOXKkKTldXvdwrDovASvp4EkORxKKdEP7w7fYQwccI+OLFiQf.Q0DNSq8GMcQrwof.XlkkoPLvzrsXyFJhTjjwfW+7h3Biu8dN9cfW4sWKodV0STlOaUMM5j3XClHf8odkCZeTIycuDmf2Pro5wLhlHW90NR1xPnDaOMVCq4rpB7IOD0Rni.QnChMaCxCeEn9VYQfU3YJ61BMZHh31tVYJwva0OaZulTnjUy7npspEABKB0NZieUfVHeTeprshZSzbytTXQRBkraYWYlVGbGVMEVXwRnpMrJT.qk1L+1qoinVTNjYnWFMKNs.kzjk4k8gUI0YXgYmX2jxn3gMuRMjxrmtrRXfPaNEel7hYOj7huiXQvP6cdvBVBwOLEKgB6JTkP2gBYmVp.4f7h+VgWzY4VIoypRhw40K9YSRpZYgzIBgDEwZmkjuymTFIsxnJK2cNBLhimTV2EAObTiMQQMLT80c3wtWdygU3wYKVVz8HBiLKX37wKW9wghjpxdyPATV0iJuQ5xpUssOqngQlDz1rRrv3Zm.ZhIypcoAhiKDIc+YVsKYIJNwNgy0vFDCNAikGXKkJXUv1Ic1zHyZ+vcuWaQdXuUxLy1fnPVFLbtKeaUN6DUhPWCbKmRlmZI.q2pieHqUawNRq0vBRlcQBf9bO8jW68QCNX0BkpuKe7bFQ7ZofgZCXjLD.C0bTB5uXkcVXuQdsr5TBtGaV6E2RQAOidtToQjhlOFf1KBPeWrMIXsa1JDf0SClZfTOsW34CZRIRTs0MxeveeeGjHQhB7fp7PzcVwBXdyeulHMJh4qkAhjnHJyoannnbsqJwmN4HKhYTW1TACuXej1papTRVJRZXYcQXa1sLQItTdT71MF4HqpGmFkHLDZ6KrINZEBKKqJJJnNWAd39la18tt.g6fmXl4X6LGOV.oTHe+2k8ZaNm+UYx9t6pNdQgsCXA.fCX.JfQLHk9Ay0a8eWpQQOvm+bPtCOR14JBs0cveT5ATGvLq+j827G6MVRseek+rG7hufEegJ9BT7EnEPmjvTOFS81k8PxqB.MC5kol8cBPmn7b17+4Rkxbj5pwKURPLESh+4NTurdONVF96Pwz7GLIcu0lzODpzAt579qsfMv.CHy9BvBMCWzeYmCm.jjY8IePZLVxwYKs2zm+P4ChA+hAi5SItF2w.RCH+xdCxezFrvlI0OLv64wfN8NItxvuFz.CTPN9+gecGtxJbHwVjJ.g7vY+VS1G0NnjX5G8+AAvcYNZnclSmD3i.7t2m9yuPYPAT.JFT1p.J.OSCEZuwc5e3XmNR85NGc+ygYH.T1aJr98cx..tiGyoSou+4+ZZlCVi9wYeG2iTKQlIy9bnIcawqzSNgcthB3iVghlmGoPO3+k2R3Ol.P8pLcv.vfACST8Fq+3O3HTLqjjDlxoSu6nkcOZlDm2WNdrVmShoK.5N6U5w6c7xFSFrlnYEbvDEbydBZdYucuQmFrKgu.GeCg22bCOdFmdWBeA5Fy4P3KzwCUoSR84wc9AkrSGNh8N3Kvw2Mycvd5dW50dYe4Gm6URbmGtyeoex7B9BPtYeRPrkIEiXPNnlL6QgbgdQCiIU9jk3FTONlRVdqX4UHIycc+Hk1+6rGhERGJ8Sx6cxdzON7SIy6RutlATOdIkzvdmQ9w9LOCVR6jeTZ8.bNECXGvRIW1qPgcBjEKfW+WNxg30QRA773tLge7M+Q+w6bB8dBBfNeYmywcUB+kXNAKnnS9CI+w+J4NXdERe9p.nHoUVTIuG6nq3s6g4N79log1deYGMyvyS+0aMmlTioJu23JmBAed9CiQMmJ9LHn1WtwX9ZNDNpmW1n9u6cI8IjjXOOrExGTl2ob2qigrr.I.d16s0gUCD7vfAqLf8MVaQ.iki8QJoN7S6n8O0jV1CQ6VRxrVo+W630e7d0Lqw709t1eCs3.mrHDcqJVPP9JZrexXJUaK9mfVsDYoMa2CHbwd4bv2pcBXHDnNDlwYhJYJFiYlY.f...SDNHHQhKUr3wDmTsOL1LAijOLHLLNJHFDFFl.i.iAAPDAL....CiDPiLVGvv0gfuVq7PZHVLMCFlMEbWXF11yYLdXbikny4NgQxTPeTvDjaNWUTRZJ.6POvTsxswWREWejZPTzH89gPzyKgSkOonc5eDDj30ZHOJ03Pv5gmcu4BWUGZNaIl.D.DqttDL5vfVTa7comoBjdFTy0ifoyO.3Q8GLCY1bqS3igkKAgLj0WchIbWsi.GQ1faK5k6Mmh.yIWFHtWsBOLckSajuWvrH4iWQsrXznTheld4Wq3duMikw0m59Pr1qs1SW55Zp58XBqiKWTuyeezA+qYidmbz+znuuC.Kc8c3ts8U2BAFtDM61a8srQ4sJn.hlX1atdKA8qeB4BFmmujHb0Fyitzx2mP2hDNGHNI5h0xmqjJ72gdV3owe+BZhEjnXHXg92Hil1l1AiA6eNBagvaLr2IjMH6d11sMxpgqaJKBjkFrKihmV1BwNdxvJSJGaAlch+delYsMgMzCNaQH6PILDTVhGNxOWqiA9G4nCfqagY7IU8P7BSGxZTFoDlF6UCQHGDivZeu3QKqsZXg1XLCeZ.Jpht0PMdj1pY+O73TN.hdFHivpG1flMm+eO.bNCalkaRrU1hOvji2ZVve6fpkGITbyXcLl6vHwUDhkLoltbR5pqYbXmWgZyRug30mWT67fCc34VU98zUEYruZiikhsTI2Og654XN8u+9GZUW2lAY3+KmIvgtOl3P.RFJnPzXNPJb2piLUrjvve5OV8LJflxutIeS.1MQcQ+I9y7OQA1WOijPKbI2oFnKMmvK4lltNG+nD2Dm0UV80+ho3VD4Fr+3c.pb5iSQsgm766go2ZBHunZ7PoKIpJmBcYLpPGSyE+BTOAcmTuoVwWTQjlbCeFoFeWWfCDy5ttyono0pNlAjTFDhdqIDdlwY2wwy5sj6QEanS9HnZtCrf7DXRTpkshBvSjvUBDDYVYL7hSUcIkbRBrFE5IhjAN2c5TcJRES2atwqFjpp50DYbjex8hBrLpTrAs4k4gtM50NV4C2rGFYPbvwSf3xXrY0zcxZ4aOwmu9+oecNZ+5lUz3HR5mEB.+9CX3mQa3TS.kxRkwiBFbY0WQzpUKhBJtGp0ldQld52guhmHhKbmmRq.qndrHdBXx4hBi9Bb.HwVYuSxz50y4W+hoLA+eW.uarfmsREMTclO+Aed2RxQ.9UNsl38v+sbqCXQTRvmxvslIXRumxrehRHTh40jWkhGU6WQncoI8T7ZQnzracfxQbyTmaXU6fB2hxDM0qJ1d9aXOmV0fms8p+fA4Vt0QDMWCMjKYcuIetb1t+ukPSnx1jP8bCdrK77UJBC9D2MiwD5i.k.Vv6fhDhIi2UFM+SJ4tDBZSkcsOM.TEwwPEBRaf8mZdTpfeRVQxnFBlD68WJREkvcbFcGNz3cQX9tQCfGq1rGu6uni5DNhyp70HcNiAnE6GtDgy5kNMbj5qylK2Y5yQ2Q+rbb5MBFQMgUeLLEmbMzcSEtjk3PPnEU2g8HU70yfdQvw5NcVghoUMrYl6XyceemLHF.yJxjQrJc73EWDYUBX7couS9lgCrkO6LtG+gGmL.lszLO29aQwLTRK7DZvjDPVfYiy0rc2AOSs5n7IhVaXuzaGsXCh5XGKWPHpF7UoThA.HEINDYnNZzC.CKzxbptmfTRnk7xt9ewT6X3s1w75gOGA1fWSMr86FJ4LBoin1vEluoKF+J09bWvsDqomDJw.OGrZnD6bMt7wBIpmB21.kMXM908Xh14Dqf4cP66g.Q0rl8V5jQH1JYFu93GyYyX6cV4s5Y4qpw0l6NrEGbBN2pZZaxsWYRp3u0gxQXxzW6GQB9XGZ8d3Cjp2AH.jg+HYqimtWQRMwbAUdyOuzzRfxgDK2jWINExz2CVataG3aNwEwTLxF.sQurz2k5saUU8CtTsBKEfKrQIdKaUDm7x6+Gvnf5L3tOxvYxmw7XnyXVZlfIRw1Pjv6LfTWO5hcUSlGnqa4fCfigMjjXCcRnV0KrCni4ApfUfD3GrvYO6AdfEiyMpl1pKhFHh1lqSFObyRm28I6MAYyMHvsy970+CvL0Pu809jZ+8PMgXH4Al4whLCmYDDhwRYrnTYBvc9iLM5MA9b0oMxuklzQGGr3BmvKbvjeZ1Bs1Mtzwbk827KWlliNBN2RzCH.L5P3srYlM4IAIovReyznj5TtTfHY3ipjTpUPP.GDv28M2jXT4JqeknD3USHzM3UB4MVEHB7XOyxrqWqjSpHuY6OLbsfCaF8WymI+m8EQ+J+DIWYpE1bEDySup4xNCHiBrIfGWZwaoe7PY7MkPKFfBySIYHFx1Zw+5CW0glOB1ZoXHJdvJQqvf1o2ZHwUYXUGu2L3WiQDl162fD0.JrztVFD4lRANj5e3hObf8lNX++yzvJ3aYHvGHOGsd2e7iQY844zF4vQe3ymxkePTpjLAOlfKBzmIIVi552aNfEhw63FPtsLg6mhCEF1iTXpDhM3FmH.wMYp.BQ9NOxCCk5KQsD2ZZIjQ5tdTFHKoph+f.VLcKcmsWG9Y9HKQX2m.3G1HOCDlIPiRyAC8U785oE2whutxrMk3EtkXhH2i2uLQp481myuT0krusYYLEy0HPKgI9PfT1qdEHAdCEQLpLQbEzhCJKty6nS9cZeDLAoErNr+ud2V0ON8r06y+VtnKfjoS3IKfaU8vhZ9Mq+oK3Be741yA2EfRelBFZoiBtaTNtIjwDwbkpF6Qty8XQxsZZDr3i5MnqQMq9mB7xx9eb2VI.jKPRYsJ+le0RPHJAZdxxcYqZ3R4XIxOen7AL4CAOhTMNnX1eoATdIgjFPRlSMPyi9.WHnlg0vMvp2f.QPsYvY7OLynLkcZMcXR2VDAAuq3ZlNrT3t71PmgR1a0eK5glMnJ5.q7eqIiyPGl7RtbhNqE.ulbmM3VsqoMvmBqzn7yQO8wkyAzFBeW9uRi85XOzH2b4mFbrl2evxXS6QQMle1OwfZJGlTOJ2in+ylLXHJPuH+e.w8xA0eKsII6lrhHH45ajCRv8W58RT0Yhm1k3osBRKG9PWDoWSV+jNe0ABShdN49OKC33vxckoP2HrkGfAyqAWub3beYRHfS29nGXYvEj3s0fLr3bHJK5QSit9QEv2PQSXzKADw6SinTl1Vl0fYZ3m7Y8PoJJLbA1npAiIreT5Uj4M7rXDHRKCIlc3E9KAzb+80Wqan.JqMhUmfiwmzfDGXtuQaQWwyi7EPn62sC1G+myaSzdm969t6fGY.nR+b+2iEiPYgBGuPYekiNvpvNKH0R2TRqEiTbIqtK2lX9ts5tDWk6jJtjQHGf8.Kylvl8I6Oi3iUOUZBLH+J6Wzq+EJa37SjUFR+At92Q8m2xyqZtp4XNZ7VWfSh8QEZdNYV4Ha84CA4caemp9RjfCE8+R9NQis4KN6E+S9EyzyKZ5YV8cY595QyJY5mm6uJgEVbKzW6OK+93Zhun5keOcB6rmKr+shAI0w8+xD5BYwY6MFwU7jRXa4JFV6nJbgCNLINm73ssSeQTZIdL+zByeNi.jRT1nVXxpXQHvyz2hEXpIWhoOdX+v2gYx8uDzl7WLXJHyGkcbz7uGTnuBVhCwMY4o5Y.MgrRbXZwiRcpDthQkgQ4jgPwwyDoquklbyB+ro9wT0wylwkaOqtieEL+jxWCUuwgd9bkmzDtS.ht74+2Dm7vcnjSR0qLSqxH++W4rcBTIf4aFAwW48rrvdD2L9i6VjIYI1L33+SGESenx+7e5KZms55tm77GG6OF+Pw.+s7ZkoqVBjOd+tmL0p35AOveOpEcy8x0OjN.Jd+1hCMA2lUGNOH3qL0jWanASQ3Nt7sI.UeJqLXnG35WdecO4gyT0+fx8r9O31a5wlnDB7CzqDi.nMcC+mnElySwfEMON+GikVjGlb1lOY9VJJUYBMhCWkiDHG9WXNBUV+6+OckWHZbhKKIkbrINh5WjqWsteo.sEagvnuDlcbCnO4UwQf7oqgxSFJCaGYijEK5oU5qBBwFfDl22D07y8Lh9kB5ZtR6InZOvgfVDew3Ggm7ir9QYqficcyoDZi+Ik7gslzMLGlhaDVAY2aKDPpulKjjDi0cqPjXqGtlzafGJhNbTFKbkLe2.jTaHz+GzthOOjPIQcx4W7rmFZYui2+T2HWGiX.suPxiL90cfNPChNzMdxlo7N+9CA3s8eySwo.MkuvAICaifDhKOqQaohSdy2pYX.P4eS4lDlzoEcxBdJKarR0SffOov28AHpO8jwFW8wB.Gqz8sL7Vl34sSw0v++iKwNKf2sCgrxK9hcz1IR740w+F5X2N7+yGz4hf3my9ywbSJ+7Y+F1fowvbIwQqFAEfQdQA9HhHXrJ43sEUMm3.HlKKaTVBqNNaNGR+F8XfixPpH31RSnu+AbJNG29gSBahhWWnCNVL+oD1+AjExKyLb9pNuqrRHNe11ZLqArGRFpBh+9mFGv5taFufDAWQmrpKsyBLESZGQXBvZux1lUbstW4QlVOyBy5GGrdjvIznVwj2812D01R8FIukEhwvyloO2EdHDfubLVUHtY2uqBw6ZAfFULR9ErrofCQi9rs2PsKWCb0jfHggLdo.qvECr720fnABM1+IMtHhq4hGE.x66bKm6uIsCzWtwzBRmkBhxZgkCaaMerFzw+CuxXqJhGoukBS6VaN1eoaZvaEpKECz0NftJ1PaTuWdX..YaoxlfBegijVTkeCgV8ZoxbQnMisoh+kauIp9CcJJtORDiXh8DH8OSZpi8W+xyLfWWffo8Gkbc9oQ6qnwT4GpDkcCjBnRFg41.Bl8JB46L6DZ.zfwuvtIsBXP6m+c5lLs3M2eCGy9XMr7CT5H..foi...rNB...";

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
