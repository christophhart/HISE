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

struct GapCloseHelpers
{
	static Array<ValueTree> getPossibleLimiters(const Array<ValueTree>& candidates, const ValueTree& data, bool closeNoteGaps)
	{
		Identifier loIdLimit = closeNoteGaps ? SampleIds::LoVel : SampleIds::LoKey;
		Identifier hiIdLimit = closeNoteGaps ? SampleIds::HiVel : SampleIds::HiKey;

		Range<int> compareRange(data[loIdLimit], data[hiIdLimit]);

		Array<ValueTree> limiters;

		for (const auto& c : candidates)
		{
			if (c == data)
				continue;

			Range<int> r(c[loIdLimit], c[hiIdLimit]);

			if (r.contains(compareRange) || compareRange.contains(r))
				limiters.add(c);
		}
		return limiters;
	}

	static int getLimitValue(const Array<ValueTree>& candidates, int lowLimit, bool closeNoteGaps)
	{
		Identifier loIdLimit = closeNoteGaps ? SampleIds::LoKey : SampleIds::LoVel;

		int lowestLimit = 127;

		for (const auto& c : candidates)
		{
			auto thisLowLimit = (int)c[loIdLimit];

			if (thisLowLimit > lowLimit)
			{
				lowestLimit = jmin<int>(lowestLimit, thisLowLimit);
			}
		}

		return lowestLimit;
	}
};

void SampleImporter::closeGaps(Array<ModulatorSamplerSound*> &selection, bool closeNoteGaps, bool /*increaseUpperLimit*/)
{
	if (selection.isEmpty())
		return;

	Array<ValueTree> data;

	for (const auto& s : selection)
		data.add(s->getData());

	Identifier loId = closeNoteGaps ? SampleIds::LoKey : SampleIds::LoVel;
	Identifier hiId = closeNoteGaps ? SampleIds::HiKey : SampleIds::HiVel;

	auto undoManager = selection.getFirst()->getMainController()->getControlUndoManager();

	undoManager->beginNewTransaction("Close gaps");

	for (auto d : data)
	{
		auto limiters = GapCloseHelpers::getPossibleLimiters(data, d, closeNoteGaps);

		auto lowValue = (int)d[loId];

		auto limit = GapCloseHelpers::getLimitValue(limiters, closeNoteGaps, lowValue);

		

		if(limit > lowValue)
			d.setProperty(hiId, limit - 1, undoManager);
	}

#if 0
	// Search the biggest value of the whole selection ================================================================================

	int upperLimit = 0;
	int lowerLimit = 127;

	Identifier loId = closeNoteGaps ? SampleIds::LoKey : SampleIds::LoVel;
	Identifier hiId = closeNoteGaps ? SampleIds::HiKey : SampleIds::HiVel;

	for(int i = 0; i < selection.size(); i++)
	{
		lowerLimit = jmin(lowerLimit, (int)selection[i]->getSampleProperty(loId));
		upperLimit = jmax(upperLimit, (int)selection[i]->getSampleProperty(hiId));
	}



	Array<ModulatorSamplerSound*> interestingSounds;

	for(int i = 0; i < selection.size(); i++)
	{
		interestingSounds.clear();

		// Get the data from the current sound ================================================================================

		const int upperValue = selection[i]->getSampleProperty(hiId);

		const Range<int> interestingRegion = Range<int>(selection[i]->getSampleProperty(loId), (int)selection[i]->getSampleProperty(hiId) + 1);

		// Check each sound if it can be a possible candidate for the new upper limit ================================================================================

		for(int j = 0; j < selection.size(); j++)
		{
			if(i == j) continue; // Skip comparing to the same sample

			const int lowerValueToCheck = selection[j]->getSampleProperty(loId);

			const Range<int> regionToCheck = Range<int>(selection[j]->getSampleProperty(loId) ,
														(int)selection[j]->getSampleProperty(hiId) + 1);
			
			if(( regionToCheck == interestingRegion || regionToCheck.intersects(interestingRegion) ) && lowerValueToCheck > upperValue) interestingSounds.add(selection[j]);
		}

		// Skip the rest if no sound is found

		if(interestingSounds.size() == 0) continue;

		// Scan all candidates for the lowest value

		

		int newUpperValue = upperLimit;

		for(int j = 0; j < interestingSounds.size(); j++)
		{
			newUpperValue = jmin(newUpperValue, (int)interestingSounds[j]->getSampleProperty(loId) - 1);
		}

		// Set the value

		if(newUpperValue != upperValue)
		{
			selection[i]->setSampleProperty(hiId, newUpperValue);
		}
	};
#endif
}


#define SET(id, y) (v.setProperty(id, y, nullptr));

bool SampleImporter::createSoundAndAddToSampler(ModulatorSampler *sampler, const SamplerSoundBasicData &basicData)
{
	ValueTree v("sample");

	
	SET(SampleIds::Root, basicData.rootNote);
	SET(SampleIds::LoKey, basicData.lowKey);
	SET(SampleIds::HiKey, basicData.hiKey);
	SET(SampleIds::LoVel, basicData.lowVelocity);
	SET(SampleIds::HiVel, basicData.hiVelocity);
	SET(SampleIds::RRGroup, basicData.group);

	String allowedWildcards = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->afm.getWildcardForAllFormats();

	if (basicData.files.size() == 1)
	{
		SET(SampleIds::FileName, basicData.files.getFirst().getReferenceString());
	}
	else
	{
		for (auto f : basicData.files)
		{
			ValueTree fileChild("file");
			fileChild.setProperty(SampleIds::FileName, f.getReferenceString(), nullptr);

			v.addChild(fileChild, -1, nullptr);
		}
	}

	try
	{
		
		sampler->getSampleMap()->addSound(v);

	}
	catch(StreamingSamplerSound::LoadingError l)
	{
		String x;
		x << "Error at preloading sample " << l.fileName << ": " << l.errorDescription;
		sampler->getMainController()->getDebugLogger().logMessage(x);
		
#if USE_FRONTEND
		sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage, x);
#else
		debugError(sampler, x);
#endif

		return false;
	}

	return true;
}

#undef SET

void SampleImporter::importNewAudioFiles(Component *childComponentOfMainEditor, ModulatorSampler *sampler, const StringArray &fileNames, BigInteger draggedRootNotes/*=0*/)
{
	AlertWindowLookAndFeel laf;

	AlertWindow w("Wave File Import Settings", String(), AlertWindow::AlertIconType::NoIcon);

	w.setLookAndFeel(&laf);
	w.setUsingNativeTitleBar(true);

	ScopedPointer<FileImportDialog> fid = new FileImportDialog(sampler);

	if(draggedRootNotes != 0)
	{
		fid->setMode(FileImportDialog::DropPoint);
	}
	w.setColour(AlertWindow::backgroundColourId, Colour(0xff222222));
	w.setColour(AlertWindow::textColourId, Colours::white);
	w.addButton("OK", 1, KeyPress(KeyPress::returnKey));
	w.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	w.addCustomComponent(fid);

	if(w.runModalLoop())
	{
		switch(fid->getImportMode())
		{
		case FileImportDialog::FileName:		SampleImporter::loadAudioFilesUsingFileName(childComponentOfMainEditor,
																							sampler,
																							fileNames,
																							fid->useMetadata());
												break;
		case FileImportDialog::PitchDetection:	SampleImporter::loadAudioFilesUsingPitchDetection(childComponentOfMainEditor,
																									sampler,
																									fileNames,
																									fid->useMetadata());
												break;
		case FileImportDialog::DropPoint:		jassert(draggedRootNotes != 0);
												SampleImporter::loadAudioFilesUsingDropPoint(childComponentOfMainEditor,
																								sampler,
																								fileNames,
																								draggedRootNotes);
												break;
        case FileImportDialog::numImportModes:  break;

		}

		if (fid->useMetadata())
		{
			SampleEditHandler::SampleEditingActions::automapUsingMetadata(sampler);
		}
	}
}

void SampleImporter::loadAudioFilesUsingDropPoint(Component* /*childComponentOfMainEditor*/, ModulatorSampler *sampler, const StringArray &fileNames, BigInteger rootNotes)
{
	
	ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

	const int startIndex = sampler->getNumSounds();

	const bool mapToVelocity = fileNames.size() > 1 && rootNotes.countNumberOfSetBits() == 1;

	const float velocityDelta = 127.0f / (float)fileNames.size();

	const int startNote = rootNotes.findNextSetBit(0);
	int delta = mapToVelocity ? 1 : rootNotes.findNextSetBit(startNote +1) - startNote;

	if (fileNames.size() == 1)
	{
		delta = 1;
	}

	float velocity = 0.0f;
	int noteNumber = startNote;
	
	for(int i = 0; i < fileNames.size(); i++)
	{
		SamplerSoundBasicData data;

		PoolReference ref(sampler->getMainController(), fileNames[i], FileHandlerBase::Samples);

		data.files.add(ref);
		data.index = startIndex + i;
		data.rootNote = noteNumber;
		data.lowKey = noteNumber;
		
		if(mapToVelocity)
		{
			

			data.hiKey = noteNumber;
			data.lowVelocity = (int)(velocity);
			data.hiVelocity = (int)(velocity + velocityDelta - 1.0f);

			if (i == fileNames.size() - 1)
			{
				data.hiVelocity = 127;
			}

			velocity += velocityDelta;
		}
		else
		{
			data.hiKey = noteNumber + delta - 1;
			data.lowVelocity = 0;
			data.hiVelocity = 127;
			noteNumber += delta;
		}

		createSoundAndAddToSampler(sampler, data);		
	}

	sampler->refreshPreloadSizes();
	

	
}

void SampleImporter::loadAudioFilesUsingFileName(Component *childComponentOfMainEditor, ModulatorSampler *sampler, const StringArray &fileNames, bool /*useVelocityAutomap*/)
{
	
	FileImportDialogWindow *dialogWindow = new FileImportDialogWindow(sampler, fileNames);

	dialogWindow->setModalBaseWindowComponent(childComponentOfMainEditor);

	

	

}

void SampleImporter::loadAudioFilesUsingPitchDetection(Component* /*childComponentOfMainEditor*/, ModulatorSampler *sampler, const StringArray &fileNames, bool /*useVelocityAutomap*/)
{
	Array<Range<double>> freqRanges;

	freqRanges.add(Range<double>(0, MidiMessage::getMidiNoteInHertz(1)/2));

	for(int i = 1; i < 126; i++)
	{
		const double thisPitch = MidiMessage::getMidiNoteInHertz(i);
		const double nextPitch = MidiMessage::getMidiNoteInHertz(i+1);
		const double prevPitch = MidiMessage::getMidiNoteInHertz(i-1);

		const double lowerLimit = thisPitch - (thisPitch-prevPitch) * 0.5;
		const double upperLimit = thisPitch + (nextPitch - thisPitch) * 0.5;

		freqRanges.add(Range<double>(lowerLimit, upperLimit));		
	}

	const int numSamplesPerDetection = PitchDetection::getNumSamplesNeeded(sampler->Processor::getSampleRate());
	AudioSampleBuffer pitchDetectionBuffer(2, numSamplesPerDetection);

	const int startIndex = sampler->getNumSounds();

	ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

	for(int i = 0; i < fileNames.size(); i++)
	{
		const double pitch = PitchDetection::detectPitch(File(fileNames[i]), pitchDetectionBuffer, sampler->Processor::getSampleRate());
		int rootNote = -1;

		for(int j = 0; j <freqRanges.size(); j++)
		{
			if(freqRanges[j].contains(pitch))
			{
				debugToConsole(sampler, "Detected Root Note: " + MidiMessage::getMidiNoteName(j, true, true, 3));
				rootNote = j;
				break;
			}
		}

		if(rootNote == -1) debugError(sampler, "Root note cannot be detected, skipping sample " + fileNames[i]);
		
		SamplerSoundBasicData data;

		PoolReference ref(sampler->getMainController(), fileNames[i], FileHandlerBase::Samples);

		data.files.add(ref);
		data.index = startIndex + i;
		data.rootNote = rootNote;
		data.lowKey = rootNote;
		data.hiKey = rootNote;
		data.lowVelocity = 0;
		data.hiVelocity = 127;

		createSoundAndAddToSampler(sampler, data);
	}

	sampler->refreshPreloadSizes();
	sampler->refreshMemoryUsage();
}

void SampleImporter::loadAudioFilesRaw(Component* /*childComponentOfMainEditor*/, ModulatorSampler* sampler, const StringArray& fileNames)
{
	const int startIndex = sampler->getNumSounds();

	ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

	for (int i = 0; i < fileNames.size(); i++)
	{
		SamplerSoundBasicData data;

		PoolReference ref(sampler->getMainController(), fileNames[i], FileHandlerBase::Samples);

		data.files.add(ref);
		data.index = startIndex + i;
		data.rootNote = i % 127;
		data.lowKey = i % 127;

		data.hiKey = i % 127;
		data.lowVelocity = 0;
		data.hiVelocity = 127;
	
		createSoundAndAddToSampler(sampler, data);
	}

	//sampler->refreshPreloadSizes();
	//sampler->refreshMemoryUsage();

}

XmlElement *SampleImporter::createXmlDescriptionForFile(const File &f, int index)
{
	XmlElement *newSample = new XmlElement("sample");

	newSample->setAttribute(SampleIds::ID, index);

	jassert(f.getFileExtension() == ".wav");

	String fileName = f.getFileNameWithoutExtension();

	enum Properties
	{
		Name,
		Root,
		Velocity
	};

	StringArray properties = StringArray::fromTokens(fileName, "_", "");
	String name = properties[Name];

	newSample->setAttribute(SampleIds::FileName, f.getFullPathName());

	// Get Root note

	int root = -1;

	for(int i = 0; i < 127; i++)
	{
		if(properties[Root] == MidiMessage::getMidiNoteName(i, true, true, 3))
		{
			root = i;
			break;
		}
	}

	jassert(root != -1);

	newSample->setAttribute(SampleIds::Root, root);
	

	// Set midi note range

	BigInteger midiNotes;

	midiNotes.setRange(0, 128, false);
	midiNotes.setBit(root, true);
	midiNotes.setBit(root+1, true);
	midiNotes.setBit(root+2, true);
	midiNotes.setBit(root-1, true);

	newSample->setAttribute(SampleIds::LoKey, midiNotes.findNextSetBit(0));
	newSample->setAttribute(SampleIds::HiKey, midiNotes.getHighestBit());
	
	// Get Velocity

	enum VelocityRanges
	{
		Low,
		Middle,
		High
	};
	
	switch (properties[Velocity].getIntValue())
	{
	case Low:		newSample->setAttribute(SampleIds::LoVel, 0);
					newSample->setAttribute(SampleIds::HiVel, 29);
					break;
	case Middle:	newSample->setAttribute(SampleIds::LoVel, 30);
					newSample->setAttribute(SampleIds::HiVel, 69);
					break;
	case High:		newSample->setAttribute(SampleIds::LoVel, 70);
					newSample->setAttribute(SampleIds::HiVel, 127);
					break;
	}

	return newSample;
}

void SampleImporter::SampleCollection::cleanCollection(DialogWindowWithBackgroundThread *thread)
{
	thread->showStatusMessage("Cleaning file list");

#if 0
	for (int i = 0; i < dataList.size(); i++)
	{
		thread->setProgress((double)i / (double)dataList.size());

		if (File(dataList[i].fileNames[0]).isDirectory())
		{
			dataList.remove(i);
			i--;
		}
	}
#endif

	if (multiMicTokens.size() == 0) return; // nothing to do here...

	thread->showStatusMessage("Merging multimics");

	for (int i = 0; i < dataList.size(); i++)
	{
		thread->setProgress((double)i / (double)dataList.size());

		const int indexOfSameMicSample = getIndexOfSameMicSample(i);

		if (indexOfSameMicSample != -1)
		{
			dataList.getReference(indexOfSameMicSample).files.add(dataList[i].files[0]);
			dataList.remove(i);
			i--;
		}
	}
}

int SampleImporter::SampleCollection::getIndexOfSameMicSample(int currentIndex) const
{
	String fileName = dataList[currentIndex].files[0].getReferenceString();

	String token = "";

	for (int i = 0; i < multiMicTokens.size(); i++)
	{
		if (fileName.contains(multiMicTokens[i]))
		{
			token = multiMicTokens[i];
			break;
		}
	}

	jassert(token.isNotEmpty());

	String fileNameWithoutToken = fileName.replace(token, "");

	for (int i = 0; i < currentIndex; i++)
	{
		if (dataList[i].files[0].getReferenceString().contains(fileNameWithoutToken))
		{
			return i;
		}
	}

	return -1;
}

FileImportDialogWindow::FileImportDialogWindow(ModulatorSampler *sampler_, const StringArray &files_):
	DialogWindowWithBackgroundThread("File Name Pattern Settings"),
	sampler(sampler_),
	files(files_)
{
	fid = new FileNameImporterDialog(sampler, files);

	addCustomComponent(fid);

	StringArray poolOptions;
	poolOptions.add("Search Pool (slower)");
	poolOptions.add("Skip pool search for duplicate samples");

	addComboBox("poolSearch", poolOptions, "Pool Search Behaviour");
	getComboBoxComponent("poolSearch")->setSelectedItemIndex(0);

	addBasicComponents();

	
}

FileImportDialogWindow::~FileImportDialogWindow()
{
	
}

void FileImportDialogWindow::run()
{
	SampleImporter::SampleCollection collection;

	ModulatorSamplerSoundPool *pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
	
	if (getComboBoxComponent("poolSearch")->getSelectedItemIndex() == 1)
	{
		pool->setDeactivatePoolSearch(true);
	}

	showStatusMessage("Parsing file name tokens");

	sampler->setShouldUpdateUI(false);
	pool->setUpdatePool(false);

	fid->fillDataList(collection, sampler->getNumSounds());

	if (threadShouldExit())
	{
		sampler->setShouldUpdateUI(true);
		pool->setUpdatePool(true);
		pool->setDeactivatePoolSearch(false);
		return;
	}

	showStatusMessage("Merging multimics & clean file list");

	try
	{
		collection.cleanCollection(this);
	}
	catch (StreamingSamplerSound::LoadingError l)
	{
		String x;
		x << "Error at preloading sample " << l.fileName << ": " << l.errorDescription;
		sampler->getMainController()->getDebugLogger().logMessage(x);
		
#if USE_FRONTEND
		sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage, x);
#else
		debugError(sampler, x);
#endif

		sampler->setShouldUpdateUI(true);
		return;
	}

	if (threadShouldExit())
	{
		sampler->setShouldUpdateUI(true);
		pool->setUpdatePool(true);
		pool->setDeactivatePoolSearch(false);
		return;
	}

	showStatusMessage("Prepare sampler for multimics");

	sampler->setNumMicPositions(collection.multiMicTokens);

	{
		ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());
		MessageManagerLock mLock;

		for (int i = 0; i < collection.dataList.size(); i++)
		{
			showStatusMessage("Loading sample " + collection.dataList[i].files.getFirst().getReferenceString());

			setProgress((double)i / (double)collection.dataList.size());

			SampleImporter::createSoundAndAddToSampler(sampler, collection.dataList[i]);

			if (threadShouldExit())
			{
				sampler->setShouldUpdateUI(true);
				pool->setUpdatePool(true);
				pool->setDeactivatePoolSearch(false);
				return;
			}
		}
	}
	
	sampler->setShouldUpdateUI(true);
	pool->setUpdatePool(true);
	pool->setDeactivatePoolSearch(false);

	sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
	sampler->sendChangeMessage();
}



void FileImportDialogWindow::threadFinished()
{
	sampler->refreshPreloadSizes();
	sampler->refreshMemoryUsage();

	if (SampleEditHandler::SampleEditingActions::metadataWasFound(sampler) && PresetHandler::showYesNoWindow("Loop points found", "Do you want to set the loop points from the metadata"))
	{
		SampleEditHandler::SampleEditingActions::automapUsingMetadata(sampler);
	}

	int currentRRAmount = (int)sampler->getAttribute(ModulatorSampler::Parameters::RRGroupAmount);
	int maxRRIndex = 0;

	ModulatorSampler::SoundIterator sIter(sampler, false);

	while (auto sound = sIter.getNextSound())
	{
		const int thisRR = sound->getSampleProperty(SampleIds::RRGroup);

		maxRRIndex = jmax<int>(maxRRIndex, thisRR);
	};

	if (currentRRAmount != maxRRIndex)
	{
		if (PresetHandler::showYesNoWindow("RR Group amount changed", "The amount of RR groups has changed (Old: " + String(currentRRAmount) + ", New: " + String(maxRRIndex) + "). Do you want to adjust the group amount?"))
		{
			sampler->setAttribute(ModulatorSampler::Parameters::RRGroupAmount, (float)maxRRIndex, sendNotification);
		}
	}

}

} // namespace hise