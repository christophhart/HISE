/*
  ==============================================================================

    BackendRootWindow.h
    Created: 15 May 2017 10:12:45pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef BACKENDROOTWINDOW_H_INCLUDED
#define BACKENDROOTWINDOW_H_INCLUDED


class BackendProcessorEditor;

class BackendRootWindow : public AudioProcessorEditor,
						  public BackendCommandTarget,
						  public Timer,
						  public ComponentWithKeyboard,
						  public ModalBaseWindow
{
public:

	BackendRootWindow(AudioProcessor *ownerProcessor, ValueTree& editorState);

	~BackendRootWindow();

	bool isFullScreenMode() const;

	void paint(Graphics& g) override
	{
		g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));

		//g.fillAll(Colour(0xFF333333));
	}

	void resized();

	void showSettingsWindow();

	void timerCallback() override;

	BackendProcessor* getBackendProcessor() { return owner; }
	const BackendProcessor* getBackendProcessor() const { return owner; }

	BackendProcessorEditor* getMainPanel() { return mainEditor; }

	CustomKeyboard* getKeyboard() const override
	{
		FloatingTile::Iterator<MidiKeyboardPanel> it(floatingRoot);

		while (auto kb = it.getNextPanel())
		{
			if (kb->isVisible())
				return kb->getKeyboard();
		}

		return nullptr;
	}

	const BackendProcessorEditor* getMainPanel() const { return mainEditor; }

	ModulatorSynthChain* getMainSynthChain() { return owner->getMainSynthChain(); }
	const ModulatorSynthChain* getMainSynthChain() const { return owner->getMainSynthChain(); }

	void loadNewContainer(ValueTree & v);

	void loadNewContainer(const File &f);
	
	FloatingTile* getRootFloatingTile() { return floatingRoot; }

private:

	friend class BackendCommandTarget;

	PopupLookAndFeel plaf;

	BackendProcessor *owner;

	Component::SafePointer<BackendProcessorEditor> mainEditor;

	StringArray menuNames;

	ScopedPointer<MenuBarComponent> menuBar;

	ScopedPointer<ThreadWithQuasiModalProgressWindow::Overlay> progressOverlay;

	ScopedPointer<AudioDeviceDialog> currentDialog;

	ScopedPointer<ComponentBoundsConstrainer> constrainer;

	ScopedPointer<ResizableBorderComponent> borderDragger;

#if PUT_FLOAT_IN_CODEBASE
	ScopedPointer<FloatingTile> floatingRoot;
#endif

};


#endif  // BACKENDROOTWINDOW_H_INCLUDED
