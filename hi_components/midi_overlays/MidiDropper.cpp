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


namespace hise {
using namespace juce;

MidiFileDragAndDropper::MidiFileDragAndDropper(MidiFilePlayer* player) :
	MidiFilePlayerBaseType(player)
{

}

bool MidiFileDragAndDropper::isMidiFile(const String& s)
{
	File f(s);
	auto extension = f.getFileExtension();
	return extension.contains("mid");
}

void MidiFileDragAndDropper::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{
	currentSequence = newSequence;
	currentSequenceId = newSequence->getId();
	repaint();
}

void MidiFileDragAndDropper::sequencesCleared()
{
	currentSequence = nullptr;
	currentSequenceId = {};
	repaint();
}

bool MidiFileDragAndDropper::shouldDropFilesWhenDraggedExternally(const DragAndDropTarget::SourceDetails &sourceDetails, StringArray &files, bool &canMoveFiles)
{
	if (currentSequence != nullptr)
	{
		if (!draggedTempFile.existsAsFile())
		{
			draggedTempFile = currentSequence->writeToTempFile();
		}

		return true;
	}

	return false;
}

void MidiFileDragAndDropper::dragOperationEnded(const DragAndDropTarget::SourceDetails&)
{
	if (draggedTempFile.existsAsFile())
	{
		StringArray files;
		files.add(draggedTempFile.getFullPathName());
		bool success = performExternalDragDropOfFiles(files, true);

		draggedTempFile.deleteFile();
	}
}

void MidiFileDragAndDropper::mouseDown(const MouseEvent& event)
{
	startDragging(currentSequenceId.toString(), this, createSnapshot(), true);
}

juce::Image MidiFileDragAndDropper::createSnapshot() const
{
	Image img(Image::ARGB, getWidth(), getHeight(), true);

	Graphics g(img);
	g.fillAll(findColour(HiseColourScheme::ComponentBackgroundColour));
	g.setColour(findColour(HiseColourScheme::ComponentTextColourId));
	g.setFont(getFont());
	g.drawText(currentSequenceId.toString(), getLocalBounds().toFloat(), Justification::centred);
	g.setColour(findColour(HiseColourScheme::ComponentOutlineColourId));
	g.drawRect(getLocalBounds(), 2);

	return img;
}

bool MidiFileDragAndDropper::isInterestedInFileDrag(const StringArray& files)
{
	return files.size() == 1 && isMidiFile(files[0]);
}

void MidiFileDragAndDropper::fileDragEnter(const StringArray&, int, int)
{
	hover = true;
	repaint();
}

void MidiFileDragAndDropper::fileDragExit(const StringArray&)
{
	hover = false;
	repaint();
}

void MidiFileDragAndDropper::filesDropped(const StringArray& files, int, int)
{
	File f(files[0]);
	FileInputStream fis(f);

	HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
	newSequence->setId(f.getFileNameWithoutExtension());
	newSequence->loadFrom(fis);

	getPlayer()->addSequence(newSequence);
	getPlayer()->setAttribute(MidiFilePlayer::CurrentSequence, getPlayer()->getNumSequences(), sendNotification);

	hover = false;
	repaint();
}

void MidiFileDragAndDropper::paint(Graphics& g)
{
	g.fillAll(findColour(HiseColourScheme::ComponentBackgroundColour));

	if (hover)
	{
		g.setColour(findColour(HiseColourScheme::ComponentOutlineColourId));
		g.drawRect(getLocalBounds(), 3);
	}

	
	g.setFont(getFont());
	String message;

	if (currentSequenceId.isNull()) message << "Drop MIDI file here";
	else message << "Drop new MIDI file or drag Content";

	g.setColour(findColour(HiseColourScheme::ComponentTextColourId));
	g.drawText(message, getLocalBounds().toFloat(), Justification::centred);
}


}