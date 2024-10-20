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

#ifndef SAMPLEIMPORTER_H_INCLUDED
#define SAMPLEIMPORTER_H_INCLUDED

namespace hise { using namespace juce;

class FileNameImporterDialog;

class FileImportDialogWindow : public DialogWindowWithBackgroundThread
{
public:

	FileImportDialogWindow(ModulatorSampler *sampler, const StringArray &files);

	~FileImportDialogWindow();

	void threadFinished() override;

	void run() override;

	void resized() override
	{
		lookAndFeelChanged();
	}

private:
	
	ScopedPointer<FileNameImporterDialog> fid;
	ModulatorSampler *sampler;
	const StringArray &files;
};



/** This class handles all import logic for different sample formats.
*	@ingroup sampler
*
*	It converts all supported formats to an xml element called samplemap, which can be fed to the ModulatorSampler.
*/
class SampleImporter
{
public:


	/** This scans all sounds in the array and closes existing gaps in the sample map.
	*
	*	@param selection an array with the pointers to all sounds that should be processed. It is ok to only pass a selection of the sounds here.
	*	@param closeNoteGaps if true, then it closes gaps that occur between notes. If false, it searches for gaps in the velocity mapping.
	*	@param increaseUpperLimit if true, then the upper limit of the lower sound is increased until the gap is filled.
	*/
	static void closeGaps(const SampleSelection &selection, bool closeNoteGaps, bool increaseUpperLimit=true);

	/** Loads audio files into the sampler by searching the file name for root note information. */
	static void loadAudioFilesUsingFileName(Component *childComponentOfMainEditor, ModulatorSampler *sampler, const StringArray &fileNames, bool useVelocityAutomap);

	/** Loads audio files into the sampler by using the drop point to specify the root notes. 
	*
	*	Since all samples are mapped along the note scale, the velocity automap is not possible.
	*/
	static void loadAudioFilesUsingDropPoint(Component *childComponentOfMainEditor, ModulatorSampler *sampler, const StringArray &fileNames, BigInteger rootNotes);

	/** Loads audio files into the sampler by using a pitch detection algorithm that sets the root note automatically. */
	static void loadAudioFilesUsingPitchDetection(Component *childComponentOfMainEditor, ModulatorSampler *sampler, const StringArray &fileNames, bool useVelocityAutomap);

	/** Loads audio files without any mapping. */
	static void loadAudioFilesRaw(Component* childComponentOfMainEditor, ModulatorSampler* sampler, const StringArray& fileNames);

	/** Imports audio files into the sampler. It opens an dialog which lets the user specify the import mode.
	*
	*	@param sampler the sampler that loads the files
	*	@param fileNames the absolute file names of the files. They must be ".wav" files.
	*	@param draggedRootNotes if the files were dragged onto the sample map editor, you can pass the root notes here for FileImportDialog::DropPoint
	*	@see FileImportDialog
	*/
	static void importNewAudioFiles(Component *childComponentOfMainEditor, ModulatorSampler *sampler, const StringArray &fileNames, BigInteger draggedRootNotes=0);


	struct SamplerSoundBasicData
	{
		SamplerSoundBasicData():
			index(-1),
			rootNote(0),
			lowKey(0),
			hiKey(0),
			lowVelocity(0),
			hiVelocity(127),
			group(1),
			multiMic(1)
		{};

		int index;

		Array<PoolReference> files;

		int rootNote;
		int lowKey;
		int hiKey;
		int lowVelocity;
		int hiVelocity;
		int group;
		int multiMic;

		String toString()
		{
			String s;

			s << "Index: " << String(index) << ", ";
			s << "FileName: " << files.getFirst().getReferenceString() << ", ";
			s << "Group: " << String(group) << ", ";
			s << "RootNote: " << MidiMessage::getMidiNoteName(rootNote, true, true, 3) << ", ";
			s << "NoteMap: " << String(lowKey) << " - " << String(hiKey) << ", ";
			s << "Velocity: " << String(lowVelocity) << " - " << String(hiVelocity);

			return s;
		};
	};

	struct SampleCollection
	{
		/** Deletes duplicate entries when using multimic import mode*/
		void cleanCollection(DialogWindowWithBackgroundThread *thread);

		int getIndexOfSameMicSample(int currentIndex) const;

		Array<SampleImporter::SamplerSoundBasicData> dataList;

		int numMicPositions;
		StringArray multiMicTokens;
	};

	static bool createSoundAndAddToSampler(ModulatorSampler *sampler, const SamplerSoundBasicData &basicData);

private:

	/** Creates a xml element from the filename with the most basic sound properties.
	*
	*	The filename must have the structure 'NAME_ROOTNOTE_0.wav"
	*/
	static XmlElement *createXmlDescriptionForFile(const File &f, int index);

};

} // namespace hise
#endif  // SAMPLEIMPORTER_H_INCLUDED
