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

namespace hise { using namespace juce;

void SampleEditHandler::SampleEditingActions::deleteSelectedSounds(SampleEditHandler *handler)
{
	auto f = [handler](Processor* )
	{
		auto soundsToBeDeleted = handler->getSelection().getItemArray();

		const int numToBeDeleted = soundsToBeDeleted.size();

		for (int i = 0; i < numToBeDeleted; i++)
		{
			if (soundsToBeDeleted[i].get() != nullptr) handler->sampler->deleteSound(soundsToBeDeleted[i].get());
		}

		handler->getSelection().deselectAll();
		handler->getSelection().dispatchPendingMessages();

		return true;
	};

	handler->sampler->getMainController()->getKillStateHandler().killVoicesAndCall(handler->sampler, f, MainController::KillStateHandler::TargetThread::MessageThread);
}

void SampleEditHandler::SampleEditingActions::duplicateSelectedSounds(SampleEditHandler *handler)
{
	ModulatorSampler *s = handler->sampler;

	auto sounds = handler->getSelection().getItemArray();

	ScopedLock sl(s->getMainController()->getSampleManager().getSamplerSoundLock());

	handler->getSelection().deselectAll();

	Array<int> newSelectedIndexes;

	for (int i = 0; i < sounds.size(); i++)
	{
		ValueTree v = sounds[i].get()->exportAsValueTree();
		const int index = s->getNumSounds();

		newSelectedIndexes.add(index);

		s->addSamplerSound(v, index, true);
	}
    
    s->refreshPreloadSizes();

	auto f = [handler, newSelectedIndexes]()
	{
		for (auto i : newSelectedIndexes)
		{
			auto newSound = dynamic_cast<ModulatorSamplerSound*>(handler->getSampler()->getSound(i));

			if (newSound != nullptr)
				handler->getSelection().addToSelection(newSound);
		}
	};

	new DelayedFunctionCaller(f, 50);

}


void SampleEditHandler::SampleEditingActions::removeDuplicateSounds(SampleEditHandler *handler)
{
	if (PresetHandler::showYesNoWindow("Confirm", "Do you really want to remove all duplicates?"))
	{
		auto soundsInSampler = handler->getSelection().getItemArray();

		SampleSelection soundsToDelete;

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
	auto sounds = handler->getSelection().getItemArray();

	handler->sampler->getMainController()->getSampleManager().copySamplesToClipboard(&sounds);
}

void SampleEditHandler::SampleEditingActions::automapVelocity(SampleEditHandler *handler)
{
	auto sounds = handler->getSelection().getItemArray();

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

	{
		ScopedLock sl(s->getMainController()->getSampleManager().getSamplerSoundLock());

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			const int index = s->getNumSounds();

			auto newSound = s->addSamplerSound(v.getChild(i), index);

			handler->getSelection().addToSelection(newSound);
		}
	}

	s->refreshPreloadSizes();

	handler->getSelection().dispatchPendingMessages();
}

void SampleEditHandler::SampleEditingActions::refreshCrossfades(SampleEditHandler * handler)
{
	auto sounds = handler->getSelection().getItemArray();

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
	
	ModulatorSampler::SoundIterator sIter(s);

	while (auto sound = sIter.getNextSound())
	{
		if (thisIndex == -1 || sound->getRRGroup() == thisIndex)
		{
			handler->getSelection().addToSelection(sound.get());
		}
	}
}


void SampleEditHandler::SampleEditingActions::deselectAllSamples(SampleEditHandler* handler)
{
	handler->getSelection().deselectAll();
}


class NormalizeThread : public DialogWindowWithBackgroundThread
{
public:

	NormalizeThread(SampleEditHandler *handler_):
		DialogWindowWithBackgroundThread("Normalizing samples"),
		handler(handler_)
	{
		addBasicComponents(false);
	}

	void run() override
	{
		auto soundList = handler->getSelection().getItemArray();

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




class MultimicMergeDialogWindow : public DialogWindowWithBackgroundThread,
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
		DialogWindowWithBackgroundThread("Merge sample files to multimic sounds", true),
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

			ModulatorSamplerSound * s = new ModulatorSamplerSound(sampler->getMainController(), c->soundList, i);

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
		MappingData mappingData;
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



		collections.clear();

		{
			ModulatorSampler::SoundIterator sIter(sampler);

			int i = 0;

			while (auto sound = sIter.getNextSound())
			{
				setProgress((double)i / (double)sampler->getNumSounds());

				i++;

				const String thisFileName = sound->getReferenceToSound()->getFileName();

				const bool matchesChannel = thisFileName.startsWith(channelNames[0] + separator) ||
					thisFileName.contains(separator + channelNames[0] + separator) ||
					thisFileName.contains(separator + channelNames[0] + ".") ||
					thisFileName.endsWith(separator + channelNames[0]);


				if (matchesChannel)
				{
					const String thisFileNameWithoutToken = thisFileName.replace(channelNames[0], "", true);

					MultiMicCollection *newCollection = new MultiMicCollection(sound, thisFileNameWithoutToken);

					newCollection->mappingData.fillOtherProperties(sound);

					collections.add(newCollection);
				}
			}
		}

		showStatusMessage("Processing single sounds");

		for (int channels = 1; channels < channelNames.size(); channels++)
		{
			ModulatorSampler::SoundIterator sIter(sampler);

			while (auto sound = sIter.getNextSound())
			{
				setProgress((double)channels / (double)channelNames.size());

				const String thisFileName = sound->getReferenceToSound()->getFileName();

				const bool matchesChannel = thisFileName.startsWith(channelNames[channels] + separator) ||
					thisFileName.contains(separator + channelNames[channels] + separator) ||
					thisFileName.contains(separator + channelNames[channels] + ".") ||
					thisFileName.endsWith(separator + channelNames[channels]);

				if (matchesChannel)
				{
					const String thisFileNameWithoutToken = thisFileName.replace(channelNames[channels], "", true);

					for (int i = 0; i < collections.size(); i++)
					{
						if (collections[i]->fits(sound, thisFileNameWithoutToken, mode))
						{
							collections[i]->soundList.add(sound->getReferenceToSound());

							collections[i]->mappingData.fillOtherProperties(sound);

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

		ModulatorSampler::SoundIterator sIter(sampler);

		while (auto s = sIter.getNextSound())
		{
			for (int i = 0; i < s->getNumMultiMicSamples(); i++)
			{
				StreamingSamplerSound * single = s->getReferenceToSound(i);
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
			ModulatorSamplerSound * s = new ModulatorSamplerSound(sampler->getMainController(), singleList[i], i);

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

bool setSoundPropertiesFromMetadata(ModulatorSamplerSound *sound, const StringPairArray &metadata, bool readOnly=false)
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

	if (!readOnly)
	{
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
	}

	

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

bool SampleEditHandler::SampleEditingActions::metadataWasFound(ModulatorSampler* sampler)
{
	SampleSelection sounds;

	auto handler = sampler->getSampleEditHandler();

	if (handler != nullptr)
	{
		sounds = handler->getSelection().getItemArray();
	}

	ModulatorSampler::SoundIterator sIter(sampler, false);

	while (auto sound = sIter.getNextSound())
		sounds.add(sound.get());

	AudioFormatManager *afm = &(sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->afm);

	for (int i = 0; i < sounds.size(); i++)
	{

		File f = File(sounds[i].get()->getProperty(ModulatorSamplerSound::FileName).toString());

		ScopedPointer<AudioFormatReader> reader = afm->createReaderFor(f);

		if (reader != nullptr)
		{
			if (setSoundPropertiesFromMetadata(sounds[i].get(), reader->metadataValues, true))
			{
				return true;
			}
		}
	}

	return false;
}



void SampleEditHandler::SampleEditingActions::automapUsingMetadata(ModulatorSampler* sampler)
{
	SampleSelection sounds;
	
	auto handler = sampler->getSampleEditHandler();

	if (handler != nullptr)
	{
		sounds = handler->getSelection().getItemArray();
	}

	ModulatorSampler::SoundIterator sIter(sampler);

	while (auto sound = sIter.getNextSound())
	{
		sounds.add(sound.get());
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


class SampleStartTrimmer : public DialogWindowWithBackgroundThread
{
	static AudioSampleBuffer getBufferForAnalysis(ModulatorSamplerSound* sound, int multiMicIndex, int maxSize=INT_MAX)
	{
		ScopedPointer<AudioFormatReader> reader = sound->getReferenceToSound(multiMicIndex)->createReaderForPreview();

		AudioSampleBuffer analyseBuffer;

		if (reader != nullptr)
		{
			const int numSamples = jmin<int>(maxSize, (int)reader->lengthInSamples);

			if (numSamples != 0)
			{
				analyseBuffer.setSize(2, numSamples, false, true, true);

				reader->read(&analyseBuffer, 0, numSamples, 0, true, true);
			}
		}

		return analyseBuffer;
	}


	static int calculateSampleEnd(int offset, AudioSampleBuffer &analyseBuffer, ModulatorSampler * sampler, float dBThreshold, bool snapToZero)
	{
		int sample = offset-1;
		const int numSamples = (int)analyseBuffer.getNumSamples();

		if (numSamples != 0)
		{
			const float threshHoldLevel = Decibels::decibelsToGain(dBThreshold);

			int lastZero = offset;
			int lastSign = 0;

			for (; sample > 0; sample--)
			{
				float leftSample = analyseBuffer.getSample(0, sample);

				if (snapToZero)
				{
					const int thisSign = leftSample > 0.0f ? 1 : -1;

					if (lastSign != thisSign)
					{
						lastZero = sample;
					}

					lastSign = thisSign;
				}

				float l = std::fabs(leftSample);
				float r = std::fabs(analyseBuffer.getSample(1, sample));

				if (l > threshHoldLevel || r > threshHoldLevel)
					return snapToZero ? lastZero : sample;
			}

			return 0;
		}
		else
		{
			debugError(sampler, "Sample is empty.");
			return -1;
		}
	}

	static int calculateSampleTrimOffset(int offset, AudioSampleBuffer &analyseBuffer, ModulatorSampler * sampler, float dBThreshold, bool snapToZero)
	{
		int sample = offset;
		const int numSamples = (int)analyseBuffer.getNumSamples();

		int lastZero = offset;
		int lastSign = 0;

		if (numSamples != 0)
		{
			auto lr = analyseBuffer.findMinMax(0, 0, numSamples);
			auto rr = analyseBuffer.findMinMax(1, 0, numSamples);

			const float maxLevel = jmax<float>(std::fabs(lr.getStart()), std::fabs(lr.getEnd()), std::fabs(rr.getStart()), std::fabs(rr.getEnd()));

			if (maxLevel == 0.0f)
				debugError(sampler, "Empty sample content. Skipping sample");

			const float threshHoldLevel = Decibels::decibelsToGain(dBThreshold);



			for (; sample < numSamples; sample++)
			{
				
				float leftSample = analyseBuffer.getSample(0, sample);

				if (snapToZero)
				{
					const int thisSign = leftSample > 0.0f ? 1 : -1;

					if (lastSign != thisSign)
					{
						lastZero = sample;
					}

					lastSign = thisSign;
				}

				float l = std::fabs(leftSample);
				float r = std::fabs(analyseBuffer.getSample(1, sample));

				if (l > threshHoldLevel || r > threshHoldLevel)
					return snapToZero ? lastZero : sample;
			}

			return numSamples - 1;
		}
		else
		{
			debugError(sampler, "Sample is empty.");
			return -1;
		}
	}

	class Window : public Component,
				   public SampleEditHandler::Listener,
				   public Value::Listener,
				   public Timer
	{
	public:

		struct ThreshholdPainter : public Component
		{
		public:

			void paint(Graphics& g)
			{
				

				if (stereo)
				{
					auto total = getLocalBounds();

					auto upper = total.removeFromTop(total.getHeight() / 2);
					auto lower = total;

					drawThreshhold(g, upper);
					drawThreshhold(g, lower);
				}
				else
				{
					drawThreshhold(g, getLocalBounds());
				}

				
				auto b = getLocalBounds();
				

				
			}

			void setThreshhold(double newLevel)
			{
				threshhold = newLevel;
				repaint();
			}

			void setStereo(bool isStereo)
			{
				stereo = isStereo;
				repaint();
			}

		private:

			void drawThreshhold(Graphics& g, Rectangle<int> bounds)
			{
				float ratio = Decibels::decibelsToGain((float)threshhold);
				int height = (int)((float)bounds.getHeight() * ratio);

				bounds = bounds.withSizeKeepingCentre(bounds.getWidth(), height);

				g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.1f));

				g.fillRect(bounds);
			}

			bool stereo = false;
			double threshhold = -100.0;

		};

		Window(SampleEditHandler* handler_):
			handler(handler_)
		{
			handler->addSelectionListener(this);

			addAndMakeVisible(viewport = new Viewport());

			firstPreview = new SamplerSoundWaveform(handler_->getSampler());

			viewport->setViewedComponent(firstPreview, false);

			viewport->setScrollBarsShown(false, false, false, false);


			addAndMakeVisible(properties = new PropertyPanel());

			endThreshhold.setValue(-80.0);
			endThreshhold.addListener(this);

			threshhold.setValue(-40.0);
			
			max = 1000.0;
			max.addListener(this);
			threshhold.addListener(this);

			soundIndex = 1;

			soundIndex.addListener(this);

			snapToZero = 1;
			snapToZero.addListener(this);
			
			zoomLevel = 1000.0;

			zoomLevel.addListener(this);

			multimicIndex = 0;
			multimicIndex.addListener(this);

			properties->setLookAndFeel(&pplaf);

			addAndMakeVisible(thressholdPainter = new ThreshholdPainter());

			setSize(800, 600);

			updatePreview();
			updatePropertyList();
		}

		~Window()
		{
			handler->removeSelectionListener(this);
		}

		void soundSelectionChanged(SampleSelection& /*newSelection*/) override
		{
			updatePreview();
		}

		void calculateNewSampleStartForPreview()
		{
			auto offset = 0;// (int)currentlyDisplayedSound->getProperty(ModulatorSamplerSound::SampleStart);

			auto newSampleStart = SampleStartTrimmer::calculateSampleTrimOffset(offset, analyseBuffer, handler->getSampler(), threshhold.getValue(), shouldSnapToZero());

			newSampleStart = jmin<int>(newSampleStart, max.getValue());

			auto wholeArea = firstPreview->getSampleArea(SamplerSoundWaveform::AreaTypes::PlayArea);


			

			previewRange.setStart(offset + newSampleStart);

			wholeArea->setSampleRange(previewRange);
			firstPreview->refreshSampleAreaBounds();
		}

		bool shouldSnapToZero() const
		{
			return (int)snapToZero.getValue() == 1;
		}

		void calculateNewSampleEndForPreview()
		{
			auto offset = (int)currentlyDisplayedSound->getReferenceToSound()->getLengthInSamples();

			AudioSampleBuffer endBuffer = SampleStartTrimmer::getBufferForAnalysis(currentlyDisplayedSound, multimicIndex.getValue());

			auto newSampleEndDelta = SampleStartTrimmer::calculateSampleEnd(offset, endBuffer, handler->getSampler(), endThreshhold.getValue(), shouldSnapToZero());

			auto wholeArea = firstPreview->getSampleArea(SamplerSoundWaveform::AreaTypes::PlayArea);

			previewRange.setEnd(newSampleEndDelta);

			wholeArea->setSampleRange(previewRange);
			firstPreview->refreshSampleAreaBounds();
		}

		void timerCallback() override
		{
			if (currentlyDisplayedSound.get() == nullptr)
			{
				stopTimer();
				return;
			}

			if (updateStart)
			{
				calculateNewSampleStartForPreview();
			}
			else
			{
				calculateNewSampleEndForPreview();
			}

			stopTimer();
		}

		void resized() override
		{
			auto all = getLocalBounds();

			auto previewArea = all.removeFromTop(400);

			viewport->setBounds(previewArea);
			thressholdPainter->setBounds(previewArea);

			all.removeFromTop(20);

			properties->setBounds(all);

			updateZoomLevel();
		}

		void valueChanged(Value& value) override
		{
			if (value == max)
			{
				updateMaxArea();
			}
			else if (value == threshhold)
			{
				thressholdPainter->setThreshhold((double)threshhold.getValue());
				
				if (!updateStart)
				{
					updateStart = true;
					updateZoomLevel();
				}

				updateStart = true;

				startTimer(100);
			}
			else if (value == endThreshhold)
			{
				thressholdPainter->setThreshhold((double)endThreshhold.getValue());

				if (updateStart)
				{
					updateStart = false;
					updateZoomLevel();
				}

				updateStart = false;
					

				
				startTimer(100);
			}
			else if (value == soundIndex || value == multimicIndex)
			{
				updatePreview();
			}
			else if (value == zoomLevel)
			{
				updateStart = true;
				updateZoomLevel();
			}
		}

		void updatePropertyList()
		{
			

			if (currentlyDisplayedSound != nullptr)
			{

				auto s = currentlyDisplayedSound->getReferenceToSound(multimicIndex.getValue());

				if (s != nullptr)
				{
					const bool isStereo = s->getPreloadBuffer().getNumChannels() == 2;

					thressholdPainter->setStereo(isStereo);
				}


				properties->clear();

				Array<PropertyComponent*> props;

				
				updateMaxArea();


				auto r = currentlyDisplayedSound->getPropertyRange(ModulatorSamplerSound::SampleStart);

				Array<var> values;
				StringArray choices;

				for (int i = 0; i < handler->getSelection().getNumSelected(); i++)
				{
					values.add(i+1);
					auto currentSound = handler->getSelection().getSelectedItem(i);
					if (currentSound != nullptr)
					{
						choices.add(currentSound->getPropertyAsString(ModulatorSamplerSound::Property::FileName));
					}
					else
					{
						choices.add("Deleted Sample");
					}
				}

				
				props.add(new ChoicePropertyComponent(soundIndex, "Displayed Sample", choices, values));
				props.add(new SliderPropertyComponent(zoomLevel, "Zoom", 100.0, 3000.0, 1.0));

				

				props.add(new SliderPropertyComponent(max, "Max Offset", 0.0, 5000.0, 1.0));
				props.add(new ChoicePropertyComponent(snapToZero, "Snap to zero", { "Yes", "No" }, { var(1), var(2) }));

				props.add(new SliderPropertyComponent(threshhold, "Start Thresshold", -100.0, 0.0, 0.1, 4.0));
				props.add(new SliderPropertyComponent(endThreshhold, "End Thresshold", -100.0, 0.0, 0.1, 4.0));

				if (currentlyDisplayedSound->getNumMultiMicSamples() > 1)
				{
					Array<var> multiMicValues;
					
					auto allMics = handler->getSampler()->getStringForMicPositions();
					StringArray multiMicChoices = StringArray::fromTokens(allMics, ";", "");
					multiMicChoices.removeEmptyStrings(true);

					for (int i = 0; i < handler->getSampler()->getNumMicPositions(); i++)
						multiMicValues.add(i);

					props.add(new ChoicePropertyComponent(multimicIndex, "Mic Position to analyze", multiMicChoices, multiMicValues));
				}

				for (auto p : props)
				{
					if ((dynamic_cast<SliderPropertyComponent*>(p) != nullptr))
					{
						p->getChildComponent(0)->setLookAndFeel(&plaf);
						p->getChildComponent(0)->setColour(Slider::ColourIds::textBoxTextColourId, Colours::white);
					}
					else
					{
						p->getChildComponent(0)->setLookAndFeel(&klaf);

						p->getChildComponent(0)->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colour(0x66333333));
						p->getChildComponent(0)->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colour(0xfb111111));
						p->getChildComponent(0)->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::white.withAlpha(0.3f));
						p->getChildComponent(0)->setColour(MacroControlledObject::HiBackgroundColours::textColour, Colours::white);

					}
				}
				
				properties->addProperties(props);
			}
		}

		void updateMaxArea()
		{
			auto startArea = firstPreview->getSampleArea(SamplerSoundWaveform::SampleStartArea);

			startArea->setAreaEnabled(false);

			startArea->setSampleRange(Range<int>(0, max.getValue()));

			analyseBuffer = SampleStartTrimmer::getBufferForAnalysis(currentlyDisplayedSound, multimicIndex.getValue(), max.getValue());

			firstPreview->refreshSampleAreaBounds();
		}

		Value min;
		Value max;
		Value threshhold;
		Value multimicIndex;
		Value soundIndex;
		Value zoomLevel;
		Value endThreshhold;
		Value snapToZero;

	private:

		Range<int> previewRange;

		bool updateStart = false;

		void updatePreview()
		{
			auto currentSelectionId = (int)soundIndex.getValue();

			currentlyDisplayedSound = handler->getSelection().getSelectedItem(jmax<int>(0, currentSelectionId-1));

			firstPreview->setSoundToDisplay(currentlyDisplayedSound, multimicIndex.getValue());
			firstPreview->getSampleArea(SamplerSoundWaveform::PlayArea)->setAreaEnabled(false);
			firstPreview->getSampleArea(SamplerSoundWaveform::LoopArea)->setAreaEnabled(false);

			auto start = 0;
			auto end = (int)currentlyDisplayedSound->getProperty(ModulatorSamplerSound::SampleEnd);

			previewRange = { start, end };

			updateMaxArea();

			calculateNewSampleStartForPreview();
			calculateNewSampleEndForPreview();

			
			updateZoomLevel();
		}

		void updateZoomLevel()
		{
			const int zl = updateStart ? (int)zoomLevel.getValue() / 100 : 1;

			firstPreview->setSize(viewport->getWidth() * zl, viewport->getHeight());
		}

		Component::SafePointer<SliderPropertyComponent> maxSlider;
		Component::SafePointer<ChoicePropertyComponent> sampleSelector;

		HiPropertyPanelLookAndFeel pplaf;

		BiPolarSliderLookAndFeel plaf;
		KnobLookAndFeel klaf;

		ScopedPointer<ThreshholdPainter> thressholdPainter;

		AudioSampleBuffer analyseBuffer;
		

		ModulatorSamplerSound::Ptr currentlyDisplayedSound;

		ScopedPointer<Viewport> viewport;
		ScopedPointer<SamplerSoundWaveform> firstPreview;

		ScopedPointer<PropertyPanel> properties;

		

		SampleEditHandler * handler;
	};

public:



	SampleStartTrimmer(SampleEditHandler* handler_) :
		DialogWindowWithBackgroundThread("Trim Samplestart", false),
		handler(handler_)
	{
		addCustomComponent(window = new Window(handler));

		addBasicComponents(true);

		showStatusMessage("Set the threshhold and the max sample offset and press OK to trim the selection");

	}

	~SampleStartTrimmer()
	{
		window = nullptr;
	}

	void run() override
	{
		trimSampleStart();
	}

	void threadFinished() override
	{
		auto avg = (double)sum / (double)numSamples;

		String report;
		NewLine nl;

		report << "Trim Statistic: min offset: " << String(minTrim) << ", max offset: " << String(maxTrim) << ", average: " << String((int)avg) << nl;
		report << "Press Cancel to undo or OK to save the changes";

		if (PresetHandler::showYesNoWindow("Sample Start trim applied", report))
		{
			applyTrim();
		}

	}

private:

	int minTrim = INT_MAX;
	int maxTrim = -1;
	int sum = 0;
	int numSamples = 0;

	struct TrimAction
	{
		ModulatorSamplerSound::Ptr s;
		int trimStart;
		int trimEnd;

		TrimAction(ModulatorSamplerSound::Ptr s_, int trimAmount_, int trimEnd_):
			s(s_),
			trimStart(trimAmount_),
			trimEnd(trimEnd_)
		{}

	};

	Array<TrimAction> trimActions;

	void applyTrim()
	{
		ModulatorSampler *sampler = handler->getSampler();

		auto tmp = std::move(trimActions);

		auto f = [tmp](Processor* p)
		{
			auto s = dynamic_cast<ModulatorSampler*>(p);

			s->getUndoManager()->beginNewTransaction();

			for (auto t : tmp)
			{
				if (t.s.get() != nullptr)
				{
					t.s->setPropertyWithUndo(ModulatorSamplerSound::SampleStart, t.trimStart);
					t.s->setPropertyWithUndo(ModulatorSamplerSound::SampleEnd, t.trimEnd);
				}
			}

			return true;
		};

		sampler->killAllVoicesAndCall(f);
		
	}

	void trimSampleStart()
	{
		trimActions.clear();

		auto sounds = handler->getSelection().getItemArray();

		int multiMicIndex = 0;

		multiMicIndex = window->multimicIndex.getValue();
		
		float startThreshhold = window->threshhold.getValue();
		float endThreshhold = window->endThreshhold.getValue();

		ModulatorSampler *sampler = handler->getSampler();


		

		if (auto s = sounds.getFirst())
		{
			s->startPropertyChange(ModulatorSamplerSound::SampleStart, 0);
		}

		numSamples = sounds.size();

		for (int i = 0; i < numSamples; i++)
		{
			if (sounds[i].get() != nullptr)
			{
				auto sound = sounds[i].get();

				AudioSampleBuffer analyseBuffer = getBufferForAnalysis(sound, multiMicIndex);

				auto startOffset = 0;// sound->getProperty(ModulatorSamplerSound::SampleStart);

				auto endOffset = sound->getReferenceToSound()->getLengthInSamples();
				
				progress = (double)i / (double)numSamples;

				if (threadShouldExit())
				{
					trimActions.clear();
					break;
				}
					

				int trimStart = calculateSampleTrimOffset(startOffset, analyseBuffer, sampler, startThreshhold, (int)window->snapToZero.getValue() == 1);
				int trimEnd = calculateSampleEnd((int)endOffset, analyseBuffer, sampler, endThreshhold, (int)window->snapToZero.getValue() == 1);

				trimStart = jmin<int>(trimStart, (int)window->max.getValue());

				minTrim = jmin<int>(trimStart, minTrim);
				maxTrim = jmax<int>(trimStart, maxTrim);

				sum += trimStart;

				if(trimStart != -1)
					trimActions.add({ sound, trimStart, trimEnd});
			}
		}

		if (auto s = sounds.getFirst())
		{
			s->sendChangeMessage();
		};
	}

	

	ScopedPointer<Window> window;
	SampleEditHandler * handler;
};


void SampleEditHandler::SampleEditingActions::trimSampleStart(Component* childComponentOfMainEditor, SampleEditHandler * body)
{
	
	auto trimmer = new SampleStartTrimmer(body);
	trimmer->setModalBaseWindowComponent(childComponentOfMainEditor);
}

} // namespace hise
