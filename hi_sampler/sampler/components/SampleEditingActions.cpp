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

void SampleEditHandler::SampleEditingActions::deleteSelectedSounds(SampleEditHandler *handler)
{
	auto soundsToBeDeleted = handler->getSelection().getItemArray();

	const int numToBeDeleted = soundsToBeDeleted.size();

	for (int i = 0; i < numToBeDeleted; i++)
	{
		if (soundsToBeDeleted[i].get() != nullptr) handler->sampler->deleteSound(soundsToBeDeleted[i].get());
	}

	handler->getSelection().deselectAll();
	handler->getSelection().dispatchPendingMessages();
}

void SampleEditHandler::SampleEditingActions::duplicateSelectedSounds(SampleEditHandler *handler)
{
	ModulatorSampler *s = handler->sampler;

	const Array<WeakReference<ModulatorSamplerSound>> sounds = handler->getSelection().getItemArray();

	for (int i = 0; i < sounds.size(); i++)
	{
		ValueTree v = sounds[i].get()->exportAsValueTree();
		const int index = s->getNumSounds();

		s->addSamplerSound(v, index, true);
        
		handler->getSelection().addToSelection(s->getSound(index));
	}
    
    s->refreshPreloadSizes();

	handler->getSelection().dispatchPendingMessages();
}


void SampleEditHandler::SampleEditingActions::removeDuplicateSounds(SampleEditHandler *handler)
{
	if (PresetHandler::showYesNoWindow("Confirm", "Do you really want to remove all duplicates?"))
	{
		Array<WeakReference<ModulatorSamplerSound>> soundsInSampler = handler->getSelection().getItemArray();

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

		handler->getSelection().deselectAll();

		const int numDeleted = soundsToDelete.size();

		for (int i = 0; i < soundsToDelete.size(); i++)
		{
			handler->sampler->deleteSound(soundsToDelete[i]);
		}

		if (numDeleted != 0)
		{
			PresetHandler::showMessageWindow("Duplicates deleted", String(numDeleted) + " duplicate samples were deleted.", PresetHandler::IconType::Info);
		}
	}
}


void SampleEditHandler::SampleEditingActions::cutSelectedSounds(SampleEditHandler *handler)
{
	copySelectedSounds(handler);
	deleteSelectedSounds(handler);
}

void SampleEditHandler::SampleEditingActions::copySelectedSounds(SampleEditHandler *handler)
{
	const Array<WeakReference<ModulatorSamplerSound>> sounds = handler->getSelection().getItemArray();

	handler->sampler->getMainController()->getSampleManager().copySamplesToClipboard(sounds);
}

void SampleEditHandler::SampleEditingActions::automapVelocity(SampleEditHandler *handler)
{
	const Array<WeakReference<ModulatorSamplerSound>> sounds = handler->getSelection().getItemArray();

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
		peakValue = sounds[i]->getNormalizedPeak();
}


void SampleEditHandler::SampleEditingActions::checkMicPositionAmountBeforePasting(const ValueTree &v, ModulatorSampler * s)
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


void SampleEditHandler::SampleEditingActions::pasteSelectedSounds(SampleEditHandler *handler)
{
	ModulatorSampler *s = handler->sampler;

	const ValueTree &v = s->getMainController()->getSampleManager().getSamplesFromClipboard();
	
	checkMicPositionAmountBeforePasting(v, s);

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		const int index = s->getNumSounds();

		s->addSamplerSound(v.getChild(i), index);

		handler->getSelection().addToSelection(s->getSound(index));
	}

	s->refreshPreloadSizes();

	handler->getSelection().dispatchPendingMessages();
}

void SampleEditHandler::SampleEditingActions::refreshCrossfades(SampleEditHandler * handler)
{
	const Array<WeakReference<ModulatorSamplerSound>> sounds = handler->getSelection().getItemArray();

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

void SampleEditHandler::SampleEditingActions::selectAllSamples(SampleEditHandler * handler)
{
	handler->getSelection().deselectAll();

	ModulatorSampler *s = handler->sampler;

	int thisIndex = handler->getCurrentlyDisplayedRRGroup();  
	
	for (int i = 0; i < s->getNumSounds(); i++)
	{
		ModulatorSamplerSound* sound = s->getSound(i);

		if (thisIndex == -1 || sound->getRRGroup() == thisIndex)
		{
			handler->getSelection().addToSelection(s->getSound(i));
		}
	}
}

class NormalizeThread : public ThreadWithAsyncProgressWindow
{
public:

	NormalizeThread(SampleEditHandler *handler_):
		ThreadWithAsyncProgressWindow("Normalizing samples"),
		handler(handler_)
	{
		addBasicComponents(false);
	}

	void run() override
	{
		Array<WeakReference<ModulatorSamplerSound>> soundList = handler->getSelection().getItemArray();

		for (int i = 0; i < soundList.size(); i++)
		{
			if (soundList[i].get() == nullptr) continue;

			if (threadShouldExit())
				return;

			setProgress((double)i / (double)soundList.size());

			showStatusMessage("Normalizing " + soundList[i]->getProperty(ModulatorSamplerSound::FileName).toString());
			soundList[i].get()->toggleBoolProperty(ModulatorSamplerSound::Normalized, dontSendNotification);
		};
	}

	void threadFinished() override
	{
		handler->sendSelectionChangeMessage(true);
	}

private:

	SampleEditHandler* handler;
};


void SampleEditHandler::SampleEditingActions::normalizeSamples(SampleEditHandler *handler, Component* childOfRoot)
{
	NormalizeThread *nm = new NormalizeThread(handler);

	nm->setModalBaseWindowComponent(childOfRoot);
	
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
		FaultySample,
		TooMuchChannels
	};

	enum class DetectionMode
	{
		Mapping,
		FileName,
		MappingAndFileName,
		numDetectionModes
	};

	MultimicMergeDialogWindow(SampleEditHandler *handler_):
		ThreadWithAsyncProgressWindow("Merge sample files to multimic sounds", true),
		handler(handler_),
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

			String currentErrorMessage = getErrorMessage();

			showStatusMessage(currentErrorMessage);
		}
	}

	void run() override
	{
		if (errorStatus != Error::OK)
		{
			PresetHandler::showMessageWindow("Error", errorMessage + ".\nPress OK to quit merging", PresetHandler::IconType::Error);
			return;
		}


		handler->getSelection().deselectAll();

		while (handler->getSelection().getNumSelected() != 0)
		{
			wait(200);
		}

		ModulatorSampler *sampler = handler->getSampler();

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
		ModulatorSamplerSound *firstSound = handler->getSelection().getSelectedItem(0).get();

		String fileName = firstSound->getReferenceToSound()->getFileName().upToFirstOccurrenceOf(".", false, false);

		tokens = StringArray::fromTokens(fileName, separator, "");

		showStatusMessage(String(tokens.size()) + " tokens found.");

		getComboBoxComponent("token")->clear(dontSendNotification);
		getComboBoxComponent("token")->addItemList(tokens, 1);
	}

	void rebuildChannelList()
	{
		channelNames.clear();

		for (int i = 0; i < handler->getSelection().getNumSelected(); i++)
		{
			String thisChannel = getChannelNameFromSound(handler->getSelection().getSelectedItem(i).get());

			channelNames.addIfNotAlreadyThere(thisChannel);
		}

        channelNames.sort(false);
        
		showStatusMessage(String(channelNames.size()) + " channels found: " + channelNames.joinIntoString(", "));
	}

	

	void checkMultimicConfiguration()
	{
		if (channelNames.size() < 2) return;

		ModulatorSampler *sampler = handler->getSampler();

		setProgress(0.0);
		showStatusMessage("Creating collections");

		const DetectionMode mode = (DetectionMode)getComboBoxComponent("mode")->getSelectedItemIndex();



		const int numSounds = sampler->getNumSounds();

		collections.clear();

		for (int i = 0; i < numSounds; i++)
		{
			setProgress((double)i / (double)numSounds);

			const String thisFileName = sampler->getSound(i)->getReferenceToSound()->getFileName();

            const bool matchesChannel = thisFileName.startsWith(channelNames[0] + separator) ||
                                        thisFileName.contains(separator + channelNames[0] + separator) ||
										thisFileName.contains(separator + channelNames[0] + ".") ||
                                        thisFileName.endsWith(separator + channelNames[0]);
            
            
			if (matchesChannel)
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

                const bool matchesChannel = thisFileName.startsWith(channelNames[channels] + separator) ||
                                            thisFileName.contains(separator + channelNames[channels] + separator) ||
											thisFileName.contains(separator + channelNames[channels] + ".") ||
                                            thisFileName.endsWith(separator + channelNames[channels]);
                
				if (matchesChannel)
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
		return handler->getSelection().getNumSelected() == handler->getSampler()->getNumSounds();
	}

	bool noExistingMultimics()
	{		
		for (int i = 0; i < handler->getSelection().getNumSelected(); i++)
		{
            const int multimics = handler->getSelection().getSelectedItem(i).get()->getNumMultiMicSamples();
            
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

		int maxChannelAmount = 0;

		setProgress(0.0);

		for (int i = 0; i < collections.size(); i++)
		{
			setProgress((double)i / double(collections.size()));

			auto collectionChannelAmount = collections[i]->soundList.size();

			maxChannelAmount = jmax<int>(maxChannelAmount, collectionChannelAmount);

			if (collectionChannelAmount != numPerCollection)
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

		if (maxChannelAmount > 8)
		{
			errorStatus = Error::TooMuchChannels;
			errorMessage = "Too many channels: " + String(maxChannelAmount) + ". Max Channel Amount: " + String(NUM_MAX_CHANNELS / 2);
			return false;
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
		case Error::TooMuchChannels:
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

	SampleEditHandler* handler;

	String separator;

	StringArray tokens;

	int currentTokenIndex;

	StringArray channelNames;

	Error errorStatus;

	String errorMessage;

	OwnedArray<MultiMicCollection> collections;
};

void SampleEditHandler::SampleEditingActions::mergeIntoMultiSamples(SampleEditHandler * handler, Component* childOfRoot)
{
	MultimicMergeDialogWindow * dialogWindow = new MultimicMergeDialogWindow(handler);

	dialogWindow->setModalBaseWindowComponent(childOfRoot);
}



void SampleEditHandler::SampleEditingActions::extractToSingleMicSamples(SampleEditHandler * handler)
{
	if (PresetHandler::showYesNoWindow("Extract Multimics to Single mics", "Do you really want to extract the multimics to single samples?"))
	{
		handler->getSelection().deselectAll();

		ModulatorSampler *sampler = handler->sampler;

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

		loopEnabled = metadata.getValue("Loop0Type", "");
		
#if 1 // Doesn't support sample start / end, but more robust loop points.

		const int loopStartId = metadata.getValue("Loop0StartIdentifier", "-1").getIntValue();
		const int loopEndId = metadata.getValue("Loop0EndIdentifier", "-1").getIntValue();

		int loopStartIndex = -1;
		int loopEndIndex = -1;

		const int numCuePoints = metadata.getValue("NumCuePoints", "0").getIntValue();

		for (int i = 0; i < numCuePoints; i++)
		{
			const String idTag = "CueLabel" + String(i) + "Identifier";

			if (metadata.getValue(idTag, "-2").getIntValue() == loopStartId)
			{
				loopStartIndex = i;
				loopStart = metadata.getValue("Cue" + String(i) + "Offset", "");
			}
			else if (metadata.getValue(idTag, "-2").getIntValue() == loopEndId)
			{
				loopEndIndex = i;
				loopEnd = metadata.getValue("Cue" + String(i) + "Offset", "");
			}
		}

#else

		const static String loopStartTag = "LoopStart";
		const static String loopEndTag = "LoopEnd";
		const static String startTag = "Start";
		const static String endTag = "End";

		const int numCuePoints = metadata.getValue("NumCuePoints", "0").getIntValue();

		bool loopStartAlreadyThere = false;

		for (int i = 0; i < numCuePoints; i++)
		{
			const String thisId = "CueLabel" + String(i) + "Text";
			const String thisValue = metadata.getValue(thisId, "");

			if (thisValue == startTag) start = metadata.getValue("Cue" + String(i) + "Offset", "");
			else if (thisValue == endTag) end = metadata.getValue("Cue" + String(i) + "Offset", "");
			else if (thisValue == loopStartTag)
			{
				if (!loopStartAlreadyThere)
				{
					loopStart = metadata.getValue("Cue" + String(i) + "Offset", "");
					loopStartAlreadyThere = true;
				}
				else
				{
					// Somehow the LoopEnd tag is wrong on some Aiffs...

					loopEnd = metadata.getValue("Cue" + String(i) + "Offset", "");
					if (loopEnabled == "1")
						end = loopEnd;

				}
			}
			else if (thisValue == loopEndTag)
			{
				loopEnd = metadata.getValue("Cue" + String(i) + "Offset", "");
				if (loopEnabled == "1")
					end = loopEnd;
			}
		}

#endif

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

void SampleEditHandler::SampleEditingActions::automapUsingMetadata(ModulatorSampler* sampler)
{
	Array<WeakReference<ModulatorSamplerSound>> sounds;
	
	auto handler = sampler->getSampleEditHandler();

	if (handler != nullptr)
	{
		sounds = handler->getSelection().getItemArray();
	}

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


void SampleEditHandler::SampleEditingActions::trimSampleStart(SampleEditHandler * handler)
{
	Array<WeakReference<ModulatorSamplerSound>> sounds = handler->getSelection().getItemArray();

    int multiMicIndex = 0;
    
	int micPositions = handler->getSampler()->getNumMicPositions();

    if(micPositions > 1)
    {
        multiMicIndex = jlimit(0, micPositions-1, PresetHandler::getCustomName("Channel Index", "Enter the channel index (starting with 0) for the mic position that you want to use to detect the sample start").getIntValue());
    }
    
	float dBThreshold = PresetHandler::getCustomName("Threshold", "Enter the dB Value for the threshold (it will use this as difference to the peak level").getFloatValue();

	dBThreshold = jlimit<float>(-100.0f, 0.0f, dBThreshold);

	

	ModulatorSampler *sampler = handler->getSampler();

	sampler->getUndoManager()->beginNewTransaction();

	AudioSampleBuffer analyseBuffer;

	for (int i = 0; i < sounds.size(); i++)
	{
		if (sounds[i].get() != nullptr)
		{
			AudioFormatReader* reader = sounds[i]->getReferenceToSound(multiMicIndex)->createReaderForAnalysis();

			if (reader != nullptr)
			{
				const int numSamples = (int)reader->lengthInSamples;

				if (numSamples != 0)
				{
					analyseBuffer.setSize(2, numSamples, false, true, true);

					reader->read(&analyseBuffer, 0, numSamples, 0, true, true);

					float lLow, lHigh, rLow, rHigh;

					reader->readMaxLevels(0, numSamples, lLow, lHigh, rLow, rHigh);

					const float maxLevel = jmax<float>(std::fabs(lLow), std::fabs(lHigh), std::fabs(rLow), std::fabs(rHigh));
                    
					const float threshHoldLevel = Decibels::decibelsToGain(dBThreshold) * maxLevel;

					int sample = sounds[i]->getProperty(ModulatorSamplerSound::SampleStart);

					for (; sample < numSamples; sample++)
					{
						float l = analyseBuffer.getSample(0, sample);
						float r = analyseBuffer.getSample(0, sample);

						if (l > threshHoldLevel || r > threshHoldLevel)
						{
							break;
						}
					}

					jassert(sample < numSamples - 1);

					sounds[i]->setPropertyWithUndo(ModulatorSamplerSound::SampleStart, var(sample));
					sounds[i]->sendChangeMessage();
				}
			}
		}
	}
}
