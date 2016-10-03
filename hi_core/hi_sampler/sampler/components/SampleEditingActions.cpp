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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

void SamplerBody::SampleEditingActions::deleteSelectedSounds(SamplerBody *body)
{
	ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(body->getProcessor());

	const Array<WeakReference<ModulatorSamplerSound>> soundsToBeDeleted = body->getSelection().getItemArray();

	const int numToBeDeleted = soundsToBeDeleted.size();

	for (int i = 0; i < numToBeDeleted; i++)
	{
		if (soundsToBeDeleted[i].get() != nullptr) sampler->deleteSound(soundsToBeDeleted[i].get());
	}

	body->getSelection().deselectAll();
	body->getSelection().dispatchPendingMessages();
}

void SamplerBody::SampleEditingActions::duplicateSelectedSounds(SamplerBody *body)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(body->getProcessor());

	const Array<WeakReference<ModulatorSamplerSound>> sounds = body->getSelection().getItemArray();

	for (int i = 0; i < sounds.size(); i++)
	{
		ValueTree v = sounds[i].get()->exportAsValueTree();
		const int index = s->getNumSounds();

		s->addSamplerSound(v, index, true);
        
		body->getSelection().addToSelection(s->getSound(index));
	}
    
    s->refreshPreloadSizes();

	body->getSelection().dispatchPendingMessages();
}


void SamplerBody::SampleEditingActions::removeDuplicateSounds(SamplerBody *body)
{
	if (PresetHandler::showYesNoWindow("Confirm", "Do you really want to remove all duplicates?"))
	{
		Array<WeakReference<ModulatorSamplerSound>> soundsInSampler = body->getSelection().getItemArray();

		Array<WeakReference<ModulatorSamplerSound>> soundsToDelete;

		StringArray fileNames;

		for (int i = 0; i < soundsInSampler.size(); i++)
		{
			if (soundsInSampler[i].get() != nullptr)
			{
				ModulatorSamplerSound *sound = soundsInSampler[i].get();

				String fileName = sound->getProperty(ModulatorSamplerSound::FileName);

				if (fileNames.contains(fileName))
				{
					soundsToDelete.add(sound);
				}
				else
				{
					fileNames.add(fileName);
				}
			}
		}

		body->getSelection().deselectAll();

		const int numDeleted = soundsToDelete.size();

		ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(body->getProcessor());

		for (int i = 0; i < soundsToDelete.size(); i++)
		{
			sampler->deleteSound(soundsToDelete[i]);
		}

		if (numDeleted != 0)
		{
			PresetHandler::showMessageWindow("Duplicates deleted", String(numDeleted) + " duplicate samples were deleted.", PresetHandler::IconType::Info);
		}
	}
}


void SamplerBody::SampleEditingActions::cutSelectedSounds(SamplerBody *body)
{
	copySelectedSounds(body);
	deleteSelectedSounds(body);
}

void SamplerBody::SampleEditingActions::copySelectedSounds(SamplerBody *body)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(body->getProcessor());
	const Array<WeakReference<ModulatorSamplerSound>> sounds = body->getSelection().getItemArray();

	s->getMainController()->getSampleManager().copySamplesToClipboard(sounds);
}

void SamplerBody::SampleEditingActions::automapVelocity(SamplerBody *body)
{
	const Array<WeakReference<ModulatorSamplerSound>> sounds = body->getSelection().getItemArray();

	int upperLimit = 0;
	int lowerLimit = 127;

	for (int i = 0; i < sounds.size(); i++)
	{
		lowerLimit = jmin(lowerLimit, (int)sounds[i]->getProperty(ModulatorSamplerSound::VeloLow));
		upperLimit = jmax(upperLimit, (int)sounds[i]->getProperty(ModulatorSamplerSound::VeloHigh));
	}

	Array<ModulatorSamplerSound*> sortedList;

	float peakValue = 0.0f;

	for (int i = 0; i < sounds.size(); i++)
	{
		peakValue = sounds[i]->getNormalizedPeak();

	}
}


void SamplerBody::SampleEditingActions::checkMicPositionAmountBeforePasting(const ValueTree &v, ModulatorSampler * s)
{
	int numMics = 1;

	if (v.getNumChildren() != 0)
	{
		jassert(v.getChild(0).getType() == Identifier("sample"));

		numMics = v.getChild(0).getNumChildren();

		if (numMics == 0) numMics = 1;
	}

	if (s->getNumMicPositions() == numMics)
	{
		return;
	}
	else
	{
		if (s->getNumSounds() == 0)
		{
			s->setNumChannels(numMics);
			return;
		}
		else if (PresetHandler::showYesNoWindow("Different mic amount detected.", "Do you want to replace all existing samples in this sampler?"))
		{
			s->clearSounds();

			s->setNumChannels(numMics);

			return;
		}
	}
}


void SamplerBody::SampleEditingActions::pasteSelectedSounds(SamplerBody *body)
{
	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(body->getProcessor());

	const ValueTree &v = s->getMainController()->getSampleManager().getSamplesFromClipboard();
	
	checkMicPositionAmountBeforePasting(v, s);

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		const int index = s->getNumSounds();

		s->addSamplerSound(v.getChild(i), index);

		body->getSelection().addToSelection(s->getSound(index));
	}

	s->refreshPreloadSizes();

	body->getSelection().dispatchPendingMessages();
}

void SamplerBody::SampleEditingActions::refreshCrossfades(SamplerBody * body)
{
	const Array<WeakReference<ModulatorSamplerSound>> sounds = body->getSelection().getItemArray();

	for (int i = 0; i < sounds.size(); i++)
	{
		sounds[i]->setProperty(ModulatorSamplerSound::UpperVelocityXFade, 0);
		sounds[i]->setProperty(ModulatorSamplerSound::LowerVelocityXFade, 0);
	}

	for (int i = 0; i < sounds.size(); i++)
	{
		ModulatorSamplerSound *referenceSound = sounds[i].get();

		const Range<int> referenceNoteRange = referenceSound->getNoteRange();

		Range<int> referenceVelocityRange = referenceSound->getVelocityRange();

		const int referenceGroup = referenceSound->getProperty(ModulatorSamplerSound::RRGroup);

		for (int j = 0; j < sounds.size(); j++)
		{

			ModulatorSamplerSound *thisSound = sounds[j].get();

			if (thisSound == referenceSound) continue;

			const int thisGroup = thisSound->getProperty(ModulatorSamplerSound::RRGroup);

			if (thisGroup != referenceGroup) continue;

			const Range<int> thisNoteRange = thisSound->getNoteRange();



			if (!thisNoteRange.intersects(referenceNoteRange)) continue;

			Range<int> thisVelocityRange = thisSound->getVelocityRange();

			Range<int> intersection = referenceVelocityRange.getIntersectionWith(thisVelocityRange);

			if (!intersection.isEmpty())
			{
				ModulatorSamplerSound *lowerSound = (thisVelocityRange.getEnd() == intersection.getEnd()) ? thisSound : referenceSound;

				ModulatorSamplerSound *upperSound = (lowerSound == thisSound) ? referenceSound : thisSound;

				lowerSound->setVelocityXFade(intersection.getLength(), false);

				upperSound->setVelocityXFade(intersection.getLength(), true);

				break;
			}

		}
	};
}

void SamplerBody::SampleEditingActions::selectAllSamples(SamplerBody * body)
{
	body->getSelection().deselectAll();

	ModulatorSampler *s = dynamic_cast<ModulatorSampler*>(body->getProcessor());

	for (int i = 0; i < s->getNumSounds(); i++)
	{
		body->getSelection().addToSelection(s->getSound(i));
	}
}

class NormalizeThread : public ThreadWithAsyncProgressWindow
{
public:

	NormalizeThread(SamplerBody *body_):
		ThreadWithAsyncProgressWindow("Normalizing samples"),
		body(body_)
	{
		addBasicComponents(false);
	}

	void run() override
	{
		Array<WeakReference<ModulatorSamplerSound>> soundList = body->getSelection().getItemArray();

		//ScopedLock sl(dynamic_cast<ModulatorSampler*>(body->getProcessor())->getSamplerLock());

		for (int i = 0; i < soundList.size(); i++)
		{
			if (soundList[i].get() == nullptr) continue;

			if (threadShouldExit())
			{
				return;
			}

			setProgress((double)i / (double)soundList.size());

			showStatusMessage("Normalizing " + soundList[i]->getProperty(ModulatorSamplerSound::FileName).toString());
			soundList[i].get()->toggleBoolProperty(ModulatorSamplerSound::Normalized, dontSendNotification);
		};
	}

	void threadFinished() override
	{
		body->soundSelectionChanged();
	}

private:

	Component::SafePointer<SamplerBody> body;
};


void SamplerBody::SampleEditingActions::normalizeSamples(SamplerBody *body)
{
	NormalizeThread *nm = new NormalizeThread(body);

	nm->setModalBaseWindowComponent(body);
	
	nm->runThread();
}




class MultimicMergeDialogWindow : public ThreadWithAsyncProgressWindow,
								  public TextEditor::Listener,
								  public ComboBox::Listener
{
public:

	enum class Error
	{
		OK = 0,
		UnequalLength,
		UnselectedSamples,
		ExistingMultimicSamples,
		SampleCollectionNotSameSize,
		FaultySample
	};

	enum class DetectionMode
	{
		Mapping,
		FileName,
		MappingAndFileName,
		numDetectionModes
	};

	MultimicMergeDialogWindow(SamplerBody *body_):
		ThreadWithAsyncProgressWindow("Merge sample files to multimic sounds", true),
		body(body_),
		separator("_")
	{
		if (sanityCheck())
		{
			addTextEditor("separator", "_", "Separator");
			getTextEditor("separator")->addListener(this);

			addComboBox("token", tokens, "Select Token");
			getComboBoxComponent("token")->addListener(this);

			StringArray modes;
			modes.add("Mapping Data");
			modes.add("Filename only");
			modes.add("Mapping and filename");
			addComboBox("mode", modes, "Select detection mode");

			addBasicComponents(true);

			rebuildTokenList();
		}
		else
		{
			addBasicComponents(false);

			String errorMessage = getErrorMessage();

			showStatusMessage(errorMessage);
		}
	}

	void run() override
	{
		if (errorStatus != Error::OK)
		{
			PresetHandler::showMessageWindow("Error", errorMessage + ".\nPress OK to quit merging", PresetHandler::IconType::Error);
			return;
		}


		body.getComponent()->getSelection().deselectAll();

		while (body.getComponent()->getSelection().getNumSelected() != 0)
		{
			wait(200);
		}

		ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(body->getProcessor());

		sampler->setBypassed(true);

		ScopedLock sl(sampler->getMainController()->getLock());

		sampler->clearSounds();

		sampler->setNumMicPositions(channelNames);

		for (int i = 0; i < collections.size(); i++)
		{
			MultiMicCollection * c = collections[i];

			ModulatorSamplerSound * s = new ModulatorSamplerSound(c->soundList, i);

			s->setMappingData(c->mappingData);

			sampler->addSound(s);

			s->setUndoManager(sampler->getUndoManager());
			s->addChangeListener(sampler->getSampleMap());
		}

		sampler->setBypassed(false);


		sampler->sendChangeMessage();
	}

	void threadFinished() override
	{
		
	}

	void textEditorTextChanged(TextEditor& t)
	{
		separator = t.getText();

		rebuildTokenList();
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged)
	{
		currentTokenIndex = comboBoxThatHasChanged->getSelectedItemIndex();

		rebuildChannelList();

		checkMultimicConfiguration();	
	}

private:

	struct MultiMicCollection
	{
		MultiMicCollection(ModulatorSamplerSound *firstSound, const String &fileNameWithoutToken_):
			mappingData((int)firstSound->getProperty(ModulatorSamplerSound::RootNote),
						(int)firstSound->getProperty(ModulatorSamplerSound::KeyLow),
						(int)firstSound->getProperty(ModulatorSamplerSound::KeyHigh),
						(int)firstSound->getProperty(ModulatorSamplerSound::VeloLow),
						(int)firstSound->getProperty(ModulatorSamplerSound::VeloHigh),
						(int)firstSound->getProperty(ModulatorSamplerSound::RRGroup)),
			fileNameWithoutToken(fileNameWithoutToken_)
		{
			soundList.add(firstSound->getReferenceToSound());
		}

		bool fits(ModulatorSamplerSound *otherSound, const String &otherFileNameWithoutToken, DetectionMode mode) const
		{
			switch (mode)
			{
			case MultimicMergeDialogWindow::DetectionMode::Mapping:
				return appliesToCollection(otherSound);
				break;
			case MultimicMergeDialogWindow::DetectionMode::FileName:
				return appliesToFileName(otherFileNameWithoutToken);
				break;
			case MultimicMergeDialogWindow::DetectionMode::MappingAndFileName:
				return appliesToCollection(otherSound) && appliesToFileName(otherFileNameWithoutToken);
				break;
			case MultimicMergeDialogWindow::DetectionMode::numDetectionModes:
				break;
			default:
				break;
			}

			return false;
		}

	private:

		bool appliesToFileName(const String &otherFileNameWithoutToken) const
		{
			return  otherFileNameWithoutToken == fileNameWithoutToken;
		}

		bool appliesToCollection(ModulatorSamplerSound *otherSound) const
		{
			return  mappingData.rootNote == (int)otherSound->getProperty(ModulatorSamplerSound::RootNote) &&
				    mappingData.loKey == (int)otherSound->getProperty(ModulatorSamplerSound::KeyLow) &&
				    mappingData.hiKey == (int)otherSound->getProperty(ModulatorSamplerSound::KeyHigh) &&
					mappingData.loVel == (int)otherSound->getProperty(ModulatorSamplerSound::VeloLow) &&
					mappingData.hiVel == (int)otherSound->getProperty(ModulatorSamplerSound::VeloHigh) &&
					mappingData.rrGroup == (int)otherSound->getProperty(ModulatorSamplerSound::RRGroup);
		}

	public:


		// ============================================================================================================

		StreamingSamplerSoundArray soundList;
		const MappingData mappingData;
		const String fileNameWithoutToken;

		// ============================================================================================================
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiMicCollection)
	};

	void rebuildTokenList()
	{
		ModulatorSamplerSound *firstSound = body->getSelection().getSelectedItem(0).get();

		String fileName = firstSound->getReferenceToSound()->getFileName().upToFirstOccurrenceOf(".", false, false);

		tokens = StringArray::fromTokens(fileName, separator, "");

		showStatusMessage(String(tokens.size()) + " tokens found.");

		getComboBoxComponent("token")->clear(dontSendNotification);
		getComboBoxComponent("token")->addItemList(tokens, 1);
	}

	void rebuildChannelList()
	{
		channelNames.clear();

		for (int i = 0; i < body->getSelection().getNumSelected(); i++)
		{
			String thisChannel = getChannelNameFromSound(body->getSelection().getSelectedItem(i).get());

			channelNames.addIfNotAlreadyThere(thisChannel);
		}

		showStatusMessage(String(channelNames.size()) + " channels found: " + channelNames.joinIntoString(", "));
	}

	

	void checkMultimicConfiguration()
	{
		if (channelNames.size() < 2) return;

		ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(body->getProcessor());

		setProgress(0.0);
		showStatusMessage("Creating collections");

		const DetectionMode mode = (DetectionMode)getComboBoxComponent("mode")->getSelectedItemIndex();



		const int numSounds = sampler->getNumSounds();

		collections.clear();

		for (int i = 0; i < numSounds; i++)
		{
			setProgress((double)i / (double)numSounds);

			const String thisFileName = sampler->getSound(i)->getReferenceToSound()->getFileName();

			if (thisFileName.contains(channelNames[0]))
			{
				const String thisFileNameWithoutToken = thisFileName.replace(channelNames[0], "", true);

				MultiMicCollection *newCollection = new MultiMicCollection(sampler->getSound(i), thisFileNameWithoutToken);

				collections.add(newCollection);
			}
		}

		showStatusMessage("Processing single sounds");

		for (int channels = 1; channels < channelNames.size(); channels++)
		{
			for (int i = 0; i < numSounds; i++)
			{
				setProgress((double)channels / (double)channelNames.size());

				const String thisFileName = sampler->getSound(i)->getReferenceToSound()->getFileName();

				if (thisFileName.contains(channelNames[channels]))
				{
					const String thisFileNameWithoutToken = thisFileName.replace(channelNames[channels], "", true);

					for (int j = 0; j < collections.size(); j++)
					{
						if (collections[j]->fits(sampler->getSound(i), thisFileNameWithoutToken, mode))
						{
							collections[j]->soundList.add(sampler->getSound(i)->getReferenceToSound());
							break;
						}
					}
				}
			}
		}

		showStatusMessage("Checking Collection Sanity");

		if (!checkCollectionSanity())
		{
			showStatusMessage(getErrorMessage());
		}
		else
		{
			showStatusMessage(String(collections.size()) + " multisamples with " + String(channelNames.size()) + " channels found. Press OK to convert.");
		}

		setProgress(1.0);
	}
	

	bool checkAllSelected()
	{
		return body->getSelection().getNumSelected() == dynamic_cast<ModulatorSampler*>(body->getProcessor())->getNumSounds();
	}

	bool noExistingMultimics()
	{		
		for (int i = 0; i < body->getSelection().getNumSelected(); i++)
		{
            const int multimics = body->getSelection().getSelectedItem(i).get()->getNumMultiMicSamples();
            
			if (multimics != 1)
			{
				return false;
			}
		}
        
        return true;
	}

	bool checkCollectionSanity()
	{
		const int numPerCollection = channelNames.size();

		setProgress(0.0);

		for (int i = 0; i < collections.size(); i++)
		{
			setProgress((double)i / double(collections.size()));

			if (collections[i]->soundList.size() != numPerCollection)
			{
				int faultyIndex = -1;

				for (int channelIndex = 0; channelIndex < channelNames.size(); channelIndex++)
				{
					bool found = false;

					for (int j = 0; j < collections[i]->soundList.size(); j++)
					{			
						if (collections[i]->soundList[j]->getFileName(false).contains(channelNames[channelIndex]))
						{
							found = true;
							break;
						}
					}

					if (!found)
					{
						faultyIndex = channelIndex;
						break;
					}
				}

				String faultyChannel = channelNames[faultyIndex];

				errorStatus = Error::SampleCollectionNotSameSize;
				errorMessage = (collections[i]->soundList.size() != 0) ?
					String(String(numPerCollection - collections[i]->soundList.size()) + " missing channel(s) - " + faultyChannel + " at Sample " + String(i) + ": " + collections[i]->soundList[0]->getFileName(false)) :
					String("Sample Nr. " + String(i) + "has no channels");
				return false;

				
			}

			if (!checkSampleSize(collections[i]))
			{
				errorStatus = Error::UnequalLength;
				errorMessage = (collections[i]->soundList.size() != 0) ? 
							   String("Unequal length at Sample " + String(i) + ": " + collections[i]->soundList[0]->getFileName(false)) :
							   String("Unequal length at Sample Nr. " + String(i));
				return false;
			}
		}

		return true;
	}

	String getErrorMessage()
	{
		switch (errorStatus)
		{
		case MultimicMergeDialogWindow::Error::OK:
			return "OK.";
			break;
		case MultimicMergeDialogWindow::Error::UnequalLength:
			return errorMessage;
			break;
		case MultimicMergeDialogWindow::Error::UnselectedSamples:
			return "You have to select all samples for the merge.";
			break;
		case MultimicMergeDialogWindow::Error::ExistingMultimicSamples:
			return "There are already multimic samples in this sampler. Extract them back to single mics and remerge them.";
			break;
		case Error::SampleCollectionNotSameSize:
			return errorMessage;
		default:
			break;
		}

		return "";
	}

	bool checkSampleSize(MultiMicCollection *collection)
	{
		StreamingSamplerSound *firstSound = collection->soundList.getFirst();

		const int length = firstSound->getSampleLength();

		for (int i = 0; i < collection->soundList.size(); i++)
		{
			const int thisLength = collection->soundList[i]->getSampleLength();

			if (thisLength != length) return false;
		}

		return true;
	}

	bool sanityCheck()
	{
		if (!checkAllSelected())
		{
			errorStatus = Error::UnselectedSamples;
			return false;
		}

		if (!noExistingMultimics())
		{
			errorStatus = Error::ExistingMultimicSamples;
			return false;
		}

		errorStatus = Error::OK;
		return true;
	}

	String getChannelNameFromSound(ModulatorSamplerSound *sound)
	{
		String fileName = sound->getReferenceToSound()->getFileName().upToFirstOccurrenceOf(".", false, false);

		for (int i = 0; i < currentTokenIndex; i++)
		{
			fileName = fileName.fromFirstOccurrenceOf(separator, false, false);
		}

		fileName = fileName.upToFirstOccurrenceOf(separator, false, false);

		return fileName;
	}

	Component::SafePointer<SamplerBody> body;

	String separator;

	StringArray tokens;

	int currentTokenIndex;

	StringArray channelNames;

	Error errorStatus;

	String errorMessage;

	OwnedArray<MultiMicCollection> collections;
};

void SamplerBody::SampleEditingActions::mergeIntoMultiSamples(SamplerBody * body)
{
	MultimicMergeDialogWindow * dialogWindow = new MultimicMergeDialogWindow(body);

	dialogWindow->setModalBaseWindowComponent(body);
}



void SamplerBody::SampleEditingActions::extractToSingleMicSamples(SamplerBody * body)
{
	if (PresetHandler::showYesNoWindow("Extract Multimics to Single mics", "Do you really want to extract the multimics to single samples?"))
	{
		body->getSelection().deselectAll();

		ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(body->getProcessor());

		sampler->setBypassed(true);

		StreamingSamplerSoundArray singleList;

		Array<MappingData> singleData;

		for (int i = 0; i < sampler->getNumSounds(); i++)
		{
			ModulatorSamplerSound *s = sampler->getSound(i);

			for (int j = 0; j < s->getNumMultiMicSamples(); j++)
			{
				StreamingSamplerSound * single = s->getReferenceToSound(j);
				MappingData newData((int)s->getProperty(ModulatorSamplerSound::RootNote),
									(int)s->getProperty(ModulatorSamplerSound::KeyLow),
									(int)s->getProperty(ModulatorSamplerSound::KeyHigh),
									(int)s->getProperty(ModulatorSamplerSound::VeloLow),
									(int)s->getProperty(ModulatorSamplerSound::VeloHigh),
									(int)s->getProperty(ModulatorSamplerSound::RRGroup));

				singleList.add(single);
				singleData.add(newData);
			}
		}

		jassert(singleList.size() == singleData.size());

		ScopedLock sl(sampler->getMainController()->getLock());

		sampler->clearSounds();

		StringArray channels;
		channels.add("SingleMic");

		sampler->setNumMicPositions(channels);

		for (int i = 0; i < singleList.size(); i++)
		{
			ModulatorSamplerSound * s = new ModulatorSamplerSound(singleList[i], i);

			s->setMappingData(singleData[i]);

			sampler->addSound(s);

			s->setUndoManager(sampler->getUndoManager());
			s->addChangeListener(sampler->getSampleMap());
		}

		sampler->setBypassed(false);

		sampler->sendChangeMessage();
	}
}

#define SET_PROPERTY_FROM_METADATA_STRING(string, prop) if (string.isNotEmpty()) sound->setProperty(prop, string.getIntValue(), sendNotification);

bool setSoundPropertiesFromMetadata(ModulatorSamplerSound *sound, const StringPairArray &metadata)
{
	const String format = metadata.getValue("MetaDataSource", "");
	
	String lowVel, hiVel, loKey, hiKey, root, start, end, loopEnabled, loopStart, loopEnd;

	DBG(metadata.getDescription());

	if (format == "AIFF")
	{
		lowVel = metadata.getValue("LowVelocity", "");
		hiVel = metadata.getValue("HighVelocity", "");
		loKey = metadata.getValue("LowNote", "");
		hiKey = metadata.getValue("HighNote", "");
		root = metadata.getValue("MidiUnityNote", "");
		start = metadata.getValue("Cue0Offset", "");
		end = metadata.getValue("Cue1Offset", "");
		loopEnabled = metadata.getValue("Loop0Type", "");
		loopStart = metadata.getValue("Cue2Offset", "");
		loopEnd = metadata.getValue("Cue3Offset", "");
	}
	else if (format == "WAV")
	{
		loopStart = metadata.getValue("Loop0Start", "");
		loopEnd = metadata.getValue("Loop0End", "");
		loopEnabled = (loopStart.isNotEmpty() && loopStart != "0" && loopEnd.isNotEmpty() && loopEnd != "0") ? "1" : "";
	}

	SET_PROPERTY_FROM_METADATA_STRING(lowVel, ModulatorSamplerSound::VeloLow);
	SET_PROPERTY_FROM_METADATA_STRING(hiVel, ModulatorSamplerSound::VeloHigh);
	SET_PROPERTY_FROM_METADATA_STRING(loKey, ModulatorSamplerSound::KeyLow);
	SET_PROPERTY_FROM_METADATA_STRING(hiKey, ModulatorSamplerSound::KeyHigh);
	SET_PROPERTY_FROM_METADATA_STRING(root, ModulatorSamplerSound::RootNote);
	SET_PROPERTY_FROM_METADATA_STRING(start, ModulatorSamplerSound::SampleStart);
	SET_PROPERTY_FROM_METADATA_STRING(end, ModulatorSamplerSound::SampleEnd);
	SET_PROPERTY_FROM_METADATA_STRING(loopEnabled, ModulatorSamplerSound::LoopEnabled);
	SET_PROPERTY_FROM_METADATA_STRING(loopStart, ModulatorSamplerSound::LoopStart);
	SET_PROPERTY_FROM_METADATA_STRING(loopEnd, ModulatorSamplerSound::LoopEnd);

	return 	lowVel != "" ||
		hiVel != "" ||
		loKey != "" ||
		hiKey != "" ||
		root != "" ||
		start != "" ||
		end != "" ||
		loopEnabled != "" ||
		loopStart != "" ||
		loopEnd != "";
}

#undef SET_PROPERTY_FROM_METADATA_STRING

void SamplerBody::SampleEditingActions::automapUsingMetadata(SamplerBody * body)
{
	Array<WeakReference<ModulatorSamplerSound>> sounds = body->getSelection().getItemArray();

	ModulatorSampler *sampler = dynamic_cast<ModulatorSampler*>(body->getProcessor());

	if (sounds.size() == 0)
	{
		for (int i = 0; i < sampler->getNumSounds(); i++)
		{
			sounds.add(sampler->getSound(i));
		}
	}

	AudioFormatManager *afm = &(sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->afm);

	bool metadataWasFound = false;

	for (int i = 0; i < sounds.size(); i++)
	{
	
		File f = File(sounds[i].get()->getProperty(ModulatorSamplerSound::FileName).toString());

		ScopedPointer<AudioFormatReader> reader = afm->createReaderFor(f);

		if (reader != nullptr)
		{
			if (setSoundPropertiesFromMetadata(sounds[i].get(), reader->metadataValues))
			{
				metadataWasFound = true;
			}
		}
	}

	if (metadataWasFound) debugToConsole(sampler, "Metadata was found for imported samples");
}
