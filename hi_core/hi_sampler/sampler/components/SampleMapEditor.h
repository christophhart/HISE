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
						 public ExpansionHandler::Listener,
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
		
		Array<Description> getDescription() const override
		{
			Array<Description> dm;

			dm.add({ "Zoom In", "Zoom in the sample map" });
			dm.add({ "Zoom Out", "Zoom out the sample map" });
			dm.add({ "New SampleMap", "Create a new SampleMap" });
			dm.add({ "Load SampleMap", "Load a SampleMap from the pool." });
			dm.add({ "Save SampleMap", "Save the current SampleMap" });
			dm.add({ "Convert to Monolith", "Convert the current samplemap to HLAC monolith format" });
			dm.add({ "Import SFZ file format", "Import SFZ file format" });
			dm.add({ "Undo", "Undo the last operation" });
			dm.add({ "Redo", "Redo the last operation" });
			dm.add({ "AutoPreview", "Autopreview the selected sample" });
			
			dm.add({ "Duplicate", "Duplicate all selected samples" });
			dm.add({ "Cut", "Cut selected samples"});
			dm.add({ "Copy", "Copy samples to clipboard"});
			dm.add({ "Paste", "Paste samples from clipboard" });
			dm.add({ "Delete", "Delete all selected samples" });
			dm.add({ "Select all Samples", "Select all Samples" });
			dm.add({ "Deselect all Samples", "Deselect all Samples" });
			dm.add({ "Fill Note Gaps", "Fill note gaps in SampleMap" });
			dm.add({ "Fill Velocity Gaps", "Fill velocity gaps in SampleMap" });
			dm.add({ "Automap Velocity", "Sort the sounds along the velocity range according to their volume" });
			dm.add({ "Refresh Velocity Crossfades.", "Adds a crossfade to overlapping sounds in a group." });
			dm.add({ "Trim Sample Start", "Removes the silence at the beginning of samples" });

			return dm;
		}

		Array<KeyMapping> getKeyMapping() const override
		{
			Array<KeyMapping> km;

			km.add({ "zoom-in", '+', ModifierKeys::commandModifier});
			km.add({ "zoom-out", '-', ModifierKeys::commandModifier });
			km.add({ "new-samplemap", 'n', ModifierKeys::commandModifier });
			km.add({ "load-samplemap", 'l', ModifierKeys::commandModifier });
			km.add({ "save-samplemap", 's', ModifierKeys::commandModifier });
			km.add({ "undo", 'z', ModifierKeys::commandModifier });
			km.add({ "redo", 'y', ModifierKeys::commandModifier });
			km.add({ "duplicate", 'd', ModifierKeys::commandModifier });
			km.add({ "cut", 'x', ModifierKeys::commandModifier });
			km.add({ "copy", 'c', ModifierKeys::commandModifier });
			km.add({ "paste", 'v', ModifierKeys::commandModifier });
			km.add({ "delete", KeyPress::deleteKey });
			km.add({ "select-all-samples", 'a', ModifierKeys::commandModifier });
			km.add({ "deselect-all-samples", KeyPress::escapeKey });

			return km;
		}

		String getId() const override { return "Sample Map Editor"; }

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
		ExportAiffWithMetadata,
		RemoveNormalisationInfo,
		RedirectSampleMapReference,
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
								TrimSampleStart,
								ExportAiffWithMetadata,
								RemoveNormalisationInfo,
								RedirectSampleMapReference
								};

		//ScopedPointer<SomeClass> ptr = new SomeClass();

		commands.addArray(id, numElementsInArray(id));
	};

	void getCommandInfo (CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform (const InvocationInfo &info) override;

	void importSfz();

	void loadSampleMap();

	void soundsSelected(int numSelected) override;

	void updateInterface() override
	{
		if (!sampler->shouldUpdateUI())
			return;

		updateSoundData();
	}

	static void autoPreviewCallback(SampleMapEditor& m, ModulatorSamplerSound::Ptr s, int micIndex);

	static void refreshRootNotes(SampleMapEditor& sme, int numSelected);

	void itemDragEnter(const SourceDetails& dragSourceDetails)
	{
		if (dragSourceDetails.description.isString())
		{
			String firstName = dragSourceDetails.description.toString().upToFirstOccurrenceOf(";", false, true);

			File file(firstName);

			if (file.isDirectory())
			{
				filesInFolder.clear();

				StringArray directories = StringArray::fromTokens(dragSourceDetails.description.toString(), ";", "");

				for (int i = 0; i < directories.size(); i++)
				{
					File dir(directories[i]);

					if (!dir.isDirectory()) continue;

					for(auto f: RangedDirectoryIterator(dir, true, "*.wav;*.aif;*.mp3;*.aiff;", File::TypesOfFileToFind::findFiles))
                        filesInFolder.add(f.getFile().getFullPathName());
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

	void expansionPackLoaded(Expansion* currentExpansion) override;

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override
	{
		if (dynamic_cast<FileTreeComponent*>(dragSourceDetails.sourceComponent.get()) != nullptr)
		{
			String firstName = dragSourceDetails.description.toString().upToFirstOccurrenceOf(";", false, true);

			File file(firstName);

			return file.isDirectory() || MultiChannelAudioBufferDisplay::isAudioFile(firstName) || file.hasFileExtension("xml");
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

	std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override;

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
			auto f2 = [ref](Processor* p)
			{
				auto s = static_cast<ModulatorSampler*>(p);
				s->loadSampleMap(ref);

				return SafeFunctionCall::OK;
			};

			sampler->killAllVoicesAndCall(f2);
		}

		mapIsHovered = false;
		resized();
		repaint();
	}

	void comboBoxChanged(ComboBox* b) override;

	bool isInterestedInFileDrag(const StringArray &files) override
	{
		if (files.size() == 0) return false;

		File file(files[0]);

		if (file.hasFileExtension(".wav")) return true;
		if (file.hasFileExtension(".aif")) return true;
		if (file.hasFileExtension(".aiff")) return true;
		if (file.hasFileExtension(".xml")) return true;
		if (file.hasFileExtension(".sfz")) return true;

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

	void mouseDown(const MouseEvent &e);

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

		File file(files[0]);

		if (file.getFileExtension() == ".xml")
		{
			PoolReference ref(sampler->getMainController(), file.getFullPathName(), FileHandlerBase::SampleMaps);

			sampler->killAllVoicesAndCall([ref](Processor* p)
			{
				auto s = static_cast<ModulatorSampler*>(p);

				s->loadSampleMap(ref);

				return SafeFunctionCall::OK;
			});
		}
		else if (file.getFileExtension() == ".sfz")
		{
			try
			{

				sampler->clearSampleMap(sendNotificationAsync);

				SfzImporter sfz(sampler, file);
				sfz.importSfzFile();

			}
			catch (SfzImporter::SfzParsingError error)
			{
				debugError(sampler, error.getErrorMessage());
			}
		}
		else if (file.getFileExtension() == ".wav" || ".aif")
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
		toolsMenu.addCommandItem(a, RemoveNormalisationInfo);
		toolsMenu.addSeparator();
		toolsMenu.addCommandItem(a, MergeIntoMultisamples);
		toolsMenu.addCommandItem(a, CreateMultiMicSampleMap);
		toolsMenu.addCommandItem(a, ExtractToSingleMicSamples);
		toolsMenu.addCommandItem(a, ReencodeMonolith);
		toolsMenu.addCommandItem(a, EncodeAllMonoliths);
		toolsMenu.addCommandItem(a, ExportAiffWithMetadata);
		toolsMenu.addCommandItem(a, RedirectSampleMapReference);
		

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

	void refreshSampleMapPool();

	void toggleVerticalSize();

	void paintOverChildren(Graphics& g) override;

	void updateSampleMapSelector(bool rebuild);

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();
    void labelTextChanged (Label* labelThatHasChanged);

	HiseShapeButton* addSimpleToggleButton(const String& title);

	void addMenuButton(SampleMapCommands commandId);

	HiseShapeButton* getButton(SampleMapCommands id) { return menuButtons[id].getComponent(); }

private:
    //[UserVariables]   -- You can add your own custom variables in this section.

	ModulatorSamplerSound::WeakPtr nextSelection;

	friend class SampleMapEditorToolbarFactory;

	ModulatorSampler *sampler;

	HiseEvent previewNote;

	SamplerBody *body;

	bool selectionIsNotEmpty;

	float zoomFactor;

	HiseShapeButton* autoPreviewButton;

	bool mapIsHovered = false;

	SampleSelection selection;

	ScopedPointer<ApplicationCommandManager> sampleMapEditorCommandManager;

	MapWithKeyboard *map;

	ScopedPointer<SampleMapEditorToolbarFactory> toolbarFactory;

	bool verticalBigSize;

	bool isDraggingFolder;
	StringArray filesInFolder;

	ScopedPointer<ValueSettingComponent> lowXFadeSetter;
	ScopedPointer<ValueSettingComponent> highXFadeSetter;
	
	OwnedArray<HiseShapeButton> ownedMenuButtons;
	Array<Component::SafePointer<HiseShapeButton>> menuButtons;

	ScopedPointer<ComboBox> sampleMaps;

	ScopedPointer<MarkdownHelpButton> warningButton;

	Factory f;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<ValueSettingComponent> rootNoteSetter;
    ScopedPointer<ValueSettingComponent> lowKeySetter;
    ScopedPointer<ValueSettingComponent> highKeySetter;
    ScopedPointer<ValueSettingComponent> lowVelocitySetter;
    ScopedPointer<ValueSettingComponent> highVelocitySetter;
    ScopedPointer<ValueSettingComponent> rrGroupSetter;
    ScopedPointer<ValueSettingComponent> numQuartersSetter;
    ScopedPointer<Viewport> viewport;
    ScopedPointer<Toolbar> toolbar;

	ScopedPointer<Component> newRRDisplay;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SampleMapEditor);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleMapEditor)
};

//[EndFile] You can add extra defines here...
/** \endcond */

} // namespace hise
//[/EndFile]

#endif   // __JUCE_HEADER_5178326AFA08B92A__
