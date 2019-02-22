/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_5178326AFA08B92A__
#define __JUCE_HEADER_5178326AFA08B92A__

//[Headers]     -- You can add your own extra header files here --
 namespace hise { using namespace juce;




//[/Headers]

#include "ValueSettingComponent.h"


//==============================================================================
/**
                                                                    //[Comments]

	\cond HIDDEN_SYMBOLS
	An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SampleMapEditor  : public Component,
                         public SamplerSubEditor,
                         public ApplicationCommandTarget,
                         public FileDragAndDropTarget,
                         public DragAndDropTarget,
                         public LabelListener,
						 public PoolBase::Listener,
						 public ComboBox::Listener,
						 public SampleMap::Listener
{
public:
    //==============================================================================
    SampleMapEditor (ModulatorSampler *s, SamplerBody *b);
    ~SampleMapEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	class Factory : public PathFactory
	{
	public:
		
		Path createPath(const String& name) const override;
	};


	/** All application commands are collected here. */
	enum SampleMapCommands
	{
		// View

		ZoomIn = 0x12000,
		ZoomOut,
		ToggleVerticalSize,
		PopOutMap,

		// Sample Map Handling
		NewSampleMap,
		LoadSampleMap,
		SaveSampleMap,
		SaveSampleMapAsXml,
		SaveSampleMapAsMonolith,
		DuplicateSampleMapAsReference,
		RevertSampleMap,
		ImportSfz,
		ImportFiles,

		Undo,
		Redo,

		// Sample Editing
		DuplicateSamples,
		DeleteDuplicateSamples,
		CutSamples,
		CopySamples,
		PasteSamples,
		DeleteSamples,
		SelectAllSamples,
		DeselectAllSamples,
		MergeIntoMultisamples,
		CreateMultiMicSampleMap,
		ExtractToSingleMicSamples,
		ReencodeMonolith,
		EncodeAllMonoliths,
		FillNoteGaps,
		FillVelocityGaps,
		AutomapVelocity,
		RefreshVelocityXFade,
		AutomapUsingMetadata,
		TrimSampleStart,
		numCommands
	};

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void getAllCommands (Array<CommandID>& commands) override
	{
		const CommandID id[] = {ZoomIn,
								ZoomOut,
								ToggleVerticalSize,
								PopOutMap,
								NewSampleMap,
								LoadSampleMap,
								SaveSampleMap,
								SaveSampleMapAsXml,
								SaveSampleMapAsMonolith,
								DuplicateSampleMapAsReference,
								RevertSampleMap,
								ImportSfz,
								ImportFiles,
								Undo,
								Redo,
								DuplicateSamples,
								DeleteDuplicateSamples,
								CutSamples,
								CopySamples,
								PasteSamples,
								DeleteSamples,
								SelectAllSamples,
								DeselectAllSamples,
								CreateMultiMicSampleMap,
								MergeIntoMultisamples,
								ExtractToSingleMicSamples,
								ReencodeMonolith,
								EncodeAllMonoliths,
								FillNoteGaps,
								FillVelocityGaps,
								AutomapVelocity,
								RefreshVelocityXFade,
								AutomapUsingMetadata,
								TrimSampleStart
								};

		commands.addArray(id, numElementsInArray(id));
	};

	void getCommandInfo (CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform (const InvocationInfo &info) override;

	void importSfz();

	void loadSampleMap();

	void soundsSelected(const SampleSelection &selectedSounds) override
	{
		selectionIsNotEmpty = selectedSounds.size() != 0;

		selectedSoundList.clear();
		selectedSoundList.addArray(selectedSounds);

		map->map->setSelectedIds(selectedSounds);

		getCommandManager()->commandStatusChanged();

		refreshRootNotes();

		rrGroupSetter->setCurrentSelection(selectedSounds);
		rootNoteSetter->setCurrentSelection(selectedSounds);
		lowKeySetter->setCurrentSelection(selectedSounds);
		highKeySetter->setCurrentSelection(selectedSounds);
		lowVelocitySetter->setCurrentSelection(selectedSounds);
		highVelocitySetter->setCurrentSelection(selectedSounds);
		lowXFadeSetter->setCurrentSelection(selectedSounds);
		highXFadeSetter->setCurrentSelection(selectedSounds);

		if (popoutCopy != nullptr && popoutCopy->isVisible())
		{
			popoutCopy->soundsSelected(selectedSounds);
		}

	}

	void updateInterface() override
	{
		auto& x = sampler->getSamplerDisplayValues();
		setPressedKeys(x.currentNotes);
		updateSoundData();
	}

	void refreshRootNotes();

	void itemDragEnter(const SourceDetails& dragSourceDetails)
	{
		if (dragSourceDetails.description.isString())
		{
			String firstName = dragSourceDetails.description.toString().upToFirstOccurrenceOf(";", false, true);

			File f(firstName);

			if (f.isDirectory())
			{
				filesInFolder.clear();

				StringArray directories = StringArray::fromTokens(dragSourceDetails.description.toString(), ";", "");

				for (int i = 0; i < directories.size(); i++)
				{
					File dir(directories[i]);

					if (!dir.isDirectory()) continue;

					DirectoryIterator iter(dir, true, "*.wav;*.aif;*.mp3;*.aiff;", File::TypesOfFileToFind::findFiles);

					while (iter.next())
					{
						filesInFolder.add(iter.getFile().getFullPathName());
					}
				}
			}
		}
		
		mapIsHovered = true;
		repaint();
	}

	void itemDragExit(const SourceDetails& /*dragSourceDetails*/)
	{
		filesInFolder.clear();

		mapIsHovered = false;
		repaint();
	}

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
	{
		if (dynamic_cast<FileTreeComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr)
		{
			String firstName = dragSourceDetails.description.toString().upToFirstOccurrenceOf(";", false, true);

			File f(firstName);

			return f.isDirectory() || AudioSampleBufferComponent::isAudioFile(firstName) || f.hasFileExtension("xml");
		}
		else
		{
			PoolReference ref(dragSourceDetails.description);

			return ref && ref.getFileType() == FileHandlerBase::SubDirectories::SampleMaps;
		}
		
	}

	void sampleMapWasChanged(PoolReference newSampleMap) override;
	
	void sampleAmountChanged() override;

	void samplePropertyWasChanged(ModulatorSamplerSound* /*s*/, const Identifier& id, const var& /*newValue*/) override;

	void updateWarningButton();

	void itemDropped(const SourceDetails &dragSourceDetails) override
	{
		if (dragSourceDetails.description.isString())
		{
			if (filesInFolder.size() != 0)
				filesDropped(filesInFolder, dragSourceDetails.localPosition.getX(), dragSourceDetails.localPosition.getY());
			else
				filesDropped(StringArray::fromTokens(dragSourceDetails.description.toString(), ";", ""), dragSourceDetails.localPosition.getX(), dragSourceDetails.localPosition.getY());

		}
		else if(auto ref = PoolReference(dragSourceDetails.description))
		{
			auto f = [ref](Processor* p)
			{
				auto s = static_cast<ModulatorSampler*>(p);
				s->loadSampleMap(ref);

				return SafeFunctionCall::OK;
			};

			sampler->killAllVoicesAndCall(f);
		}

		mapIsHovered = false;
		resized();
		repaint();
	}

	void comboBoxChanged(ComboBox* b) override;

	bool isInterestedInFileDrag(const StringArray &files) override
	{
		if (files.size() == 0) return false;

		File f(files[0]);

		if (f.hasFileExtension(".wav")) return true;
		if (f.hasFileExtension(".aif")) return true;
		if (f.hasFileExtension(".aiff")) return true;
		if (f.hasFileExtension(".xml")) return true;
		if (f.hasFileExtension(".sfz")) return true;

		return false;
	}

	void itemDragMove(const SourceDetails &dragSourceDetails) override
	{
		if (dragSourceDetails.description.isObject())
		{
			return;
		}

		if (filesInFolder.size() != 0)
		{
			fileDragMove(filesInFolder, dragSourceDetails.localPosition.getX(), dragSourceDetails.localPosition.getY());
		}
		else
		{
			fileDragMove(StringArray::fromTokens(dragSourceDetails.description.toString(), ";", ""), dragSourceDetails.localPosition.getX(), dragSourceDetails.localPosition.getY());
		}

		
	}

	void poolEntryAdded() override { updateSampleMapSelector(true); }
	void poolEntryRemoved() override { updateSampleMapSelector(true); }
	void poolEntryReloaded(PoolReference ref) override { updateSampleMapSelector(true); }

	

	void fileDragEnter(const StringArray& /*files*/, int /*x*/, int /*y*/) override
	{
		mapIsHovered = true;
		repaint();
	}

	void fileDragMove(const StringArray &files, int x, int y)
	{
		//Point<int> p = map->getLocalPoint(this, Point<int>((int)((float)x / SCALE_FACTOR()), (int)((float)y / SCALE_FACTOR())));

		Point<int> p(x, y);

		if (isInDragArea(p))
		{

			Point<int> dropPoint = getDropPoint(p);

			const int x_relative = dropPoint.getX();
			const int y_relative = dropPoint.getY();

			if (isDraggingSampleMap(files))
			{
				drawSamplemapForDragPosition();
			}
			else
			{
				drawSampleComponentsForDragPosition(files.size(), x_relative, y_relative);
			}
		}
	};

	bool isDraggingSampleMap(const StringArray& files) const
	{
		return files.size() > 0 && File(files[0]).hasFileExtension("xml");
	}

	void filesDropped(const StringArray &files, int /*x*/, int /*y*/) override
	{

		File f(files[0]);

		if (f.getFileExtension() == ".xml")
		{
			PoolReference ref(sampler->getMainController(), f.getFullPathName(), FileHandlerBase::SampleMaps);

			sampler->loadSampleMap(ref);
		}
		else if (f.getFileExtension() == ".sfz")
		{
			try
			{

				sampler->clearSampleMap(sendNotificationAsync);

				SfzImporter sfz(sampler, f);
				sfz.importSfzFile();

			}
			catch (SfzImporter::SfzParsingError error)
			{
				debugError(sampler, error.getErrorMessage());
			}
		}
		else if (f.getFileExtension() == ".wav" || ".aif")
		{
			SampleImporter::importNewAudioFiles(this, sampler, files, getRootNotesForDragPosition());
		}

		clearDragPosition();
		resized();
	}

	BigInteger getRootNotesForDragPosition()
	{
		return map->map->getRootNotesForDraggedFiles();
	};

	void drawSampleComponentsForDragPosition(int numDraggedFiles, int x, int y)
	{
		map->map->drawSampleComponentsForDragPosition(numDraggedFiles, x, y);
	}

	void drawSamplemapForDragPosition()
	{
		map->map->drawSampleMapForDragPosition();
	}

	void clearDragPosition()
	{
		map->map->clearDragPosition();
		mapIsHovered = false;
		repaint();
	}
	void setPressedKeys(const uint8 *pressedKeyData)
	{
		map->map->setPressedKeys(pressedKeyData);

		if (popoutCopy != nullptr)
		{
			popoutCopy->map->map->setPressedKeys(pressedKeyData);
		}
	}

	bool isInDragArea(Point<int> testPoint)
	{
		return viewport->getBounds().contains(testPoint);
	}

	Point<int> getDropPoint(Point<int> dragPoint)
	{
		jassert(isInDragArea(dragPoint));

		return map->getLocalPoint(this, dragPoint);

	};

	void updateSoundData()
	{
		map->map->updateSoundData();

		groupDisplay->clearOptions();

		groupDisplay->addOption("All Groups");
		for(int i = 0; i < sampler->getAttribute(ModulatorSampler::RRGroupAmount); i++)
		{
			groupDisplay->addOption("Group " + String(i + 1));
		}


	}

	SamplerSoundMap *getMapComponent()
	{
		return map->map;
	};

	void zoom(bool zoomOut)
	{
		if(zoomOut) zoomFactor = jmax (1.0f, zoomFactor / 2.0f);
		else		zoomFactor = jmin (4.0f, zoomFactor * 2.0f);

		updateMapInViewport();

	};

	void updateMapInViewport()
	{
		const int newWidth = (int)(viewport->getWidth() * zoomFactor);

		double midPoint = (double)(viewport->getViewPositionX() + viewport->getViewWidth() / 2) / (double)map->getWidth();

		map->setSize(newWidth, viewport->getHeight());

		viewport->setViewPositionProportionately(midPoint, 0);
	}

	ApplicationCommandManager *getCommandManager();

	bool keyPressed(const KeyPress& key) override;

	int getCurrentRRGroup() const
	{
		int index = groupDisplay->getCurrentIndex();

		if (index == 0) index = -1; // display all values;

		return index;
	}

	void setCurrentRRGroup(int newGroupIndex)
	{
		groupDisplay->setItemIndex(newGroupIndex, sendNotification);
	}

	void fillPopupMenu(PopupMenu &p)
	{
		ApplicationCommandManager *a = getCommandManager();

		p.addSectionHeader("Sample Map Handling");

		p.addCommandItem(a, NewSampleMap);
		p.addCommandItem(a, LoadSampleMap);
		p.addCommandItem(a, SaveSampleMap);
		p.addCommandItem(a, RevertSampleMap);

		PopupMenu saveAs;

		saveAs.addCommandItem(a, SaveSampleMapAsXml);
		saveAs.addCommandItem(a, SaveSampleMapAsMonolith);
		saveAs.addCommandItem(a, DuplicateSampleMapAsReference);

		p.addSubMenu("Save as", saveAs, true);

		p.addCommandItem(a, ImportFiles);

		p.addSectionHeader("Sample Editing");

		PopupMenu toolsMenu;

		toolsMenu.addCommandItem(a, FillNoteGaps);
		toolsMenu.addCommandItem(a, FillVelocityGaps);
		toolsMenu.addCommandItem(a, AutomapUsingMetadata);
		toolsMenu.addCommandItem(a, TrimSampleStart);
		toolsMenu.addSeparator();
		toolsMenu.addCommandItem(a, MergeIntoMultisamples);
		toolsMenu.addCommandItem(a, CreateMultiMicSampleMap);
		toolsMenu.addCommandItem(a, ExtractToSingleMicSamples);
		toolsMenu.addCommandItem(a, ReencodeMonolith);
		toolsMenu.addCommandItem(a, EncodeAllMonoliths);
		

		p.addSubMenu("Tools", toolsMenu, true);

		p.addSeparator();

		p.addCommandItem(a, CutSamples);
		p.addCommandItem(a, CopySamples);
		p.addCommandItem(a, PasteSamples);

		p.addSeparator();

		p.addCommandItem(a, DeleteSamples);
		p.addCommandItem(a, DuplicateSamples);
		p.addCommandItem(a, DeleteDuplicateSamples);
	}

	void mouseDown(const MouseEvent &e) override
	{
		getCommandManager()->setFirstCommandTarget(this);
		getCommandManager()->commandStatusChanged();

		if(e.mods.isRightButtonDown())
		{
			PopupMenu p;

			ScopedPointer<PopupLookAndFeel> laf = new PopupLookAndFeel();

			p.setLookAndFeel(laf);

			getCommandManager()->commandStatusChanged();

			fillPopupMenu(p);

			p.show();
		}
	};

	void enablePopoutMode(SampleMapEditor *parent)
	{
		parentCopy = parent;
		popoutMode = true;
		resized();
	}

	void refreshSampleMapPool();

	void deletePopup()
	{
		if (parentCopy.getComponent() != nullptr)
		{
			parentCopy->popoutCopy = nullptr;
		}
	}

	void toggleVerticalSize();

	void paintOverChildren(Graphics& g) override;

	void updateSampleMapSelector(bool rebuild);

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);

	void addMenuButton(SampleMapCommands commandId);

	HiseShapeButton* getButton(SampleMapCommands id) { return menuButtons[id].getComponent(); }

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	
	friend class SampleMapEditorToolbarFactory;

	ModulatorSampler *sampler;

	SamplerBody *body;

	bool selectionIsNotEmpty;

	float zoomFactor;

	bool mapIsHovered = false;

	Array<ModulatorSamplerSound*> selectedSoundList;

	ScopedPointer<ApplicationCommandManager> sampleMapEditorCommandManager;

	MapWithKeyboard *map;

	ScopedPointer<SampleMapEditorToolbarFactory> toolbarFactory;

	ScopedPointer<SampleMapEditor> popoutCopy;

	Component::SafePointer<SampleMapEditor> parentCopy;

	bool verticalBigSize;

	bool popoutMode;

	bool isDraggingFolder;
	StringArray filesInFolder;

	ScopedPointer<ValueSettingComponent> lowXFadeSetter;
	ScopedPointer<ValueSettingComponent> highXFadeSetter;
	
	OwnedArray<HiseShapeButton> ownedMenuButtons;
	Array<Component::SafePointer<HiseShapeButton>> menuButtons;

	ScopedPointer<MarkdownHelpButton> helpButton;

	ScopedPointer<ComboBox> sampleMaps;

	ScopedPointer<MarkdownHelpButton> warningButton;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<ValueSettingComponent> rootNoteSetter;
    ScopedPointer<ValueSettingComponent> lowKeySetter;
    ScopedPointer<ValueSettingComponent> highKeySetter;
    ScopedPointer<ValueSettingComponent> lowVelocitySetter;
    ScopedPointer<ValueSettingComponent> highVelocitySetter;
    ScopedPointer<ValueSettingComponent> rrGroupSetter;
    ScopedPointer<Label> displayGroupLabel;
    ScopedPointer<PopupLabel> groupDisplay;
    ScopedPointer<Viewport> viewport;
    ScopedPointer<Toolbar> toolbar;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleMapEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */

} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_5178326AFA08B92A__
