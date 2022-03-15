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

MidiFileDragAndDropper::MidiFileDragAndDropper(MidiPlayer* player) :
	MidiPlayerBaseType(player)
{
	setColour(HiseColourScheme::ComponentBackgroundColour, Colour(0x11000000));
	setColour(HiseColourScheme::ComponentTextColourId, Colours::white);
	setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white);

	sequenceLoaded(getPlayer()->getCurrentSequence());
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
	currentSequenceId = newSequence != nullptr ? newSequence->getId() : Identifier();

	setMouseCursor(newSequence != nullptr ? MouseCursor::DraggingHandCursor : MouseCursor());
	repaint();
}

void MidiFileDragAndDropper::sequencesCleared()
{
	currentSequence = nullptr;
	currentSequenceId = {};
	setMouseCursor(MouseCursor());
	repaint();
}

bool MidiFileDragAndDropper::shouldDropFilesWhenDraggedExternally(const DragAndDropTarget::SourceDetails &, StringArray &, bool &)
{
	return false;
}

void MidiFileDragAndDropper::dragOperationEnded(const DragAndDropTarget::SourceDetails&)
{
	
}

void MidiFileDragAndDropper::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		FileChooser fc("Open MIDI File", getPlayer()->getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::MidiFiles), "*.mid");

		if (fc.browseForFileToOpen())
		{
			auto f = fc.getResult();
			
			PoolReference ref(getPlayer()->getMainController(), f.getFullPathName(), FileHandlerBase::MidiFiles);
			getPlayer()->loadMidiFile(ref);
		}
	}
	else
	{
		if (currentSequence != nullptr)
		{
			auto c = currentSequence->clone();
			c->setCurrentTrackIndex(getPlayer()->getAttribute(MidiPlayer::CurrentTrack) - 1);
			c->trimInactiveTracks();

			auto tmp = c->writeToTempFile();
			externalDrag = true;
			repaint();

			performExternalDragDropOfFiles({ tmp.getFullPathName() }, false, this, [tmp, this]()
			{
				this->externalDrag = false;
				this->repaint();

				auto f = [tmp]()
				{
					tmp.deleteFile();
				};

				new DelayedFunctionCaller(f, 2000);
			});
		}
	}
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
	PoolReference ref(getPlayer()->getMainController(), files[0], FileHandlerBase::MidiFiles);

	getPlayer()->loadMidiFile(ref);

	

	hover = false;
	repaint();
}


void MidiFileDragAndDropper::itemDragEnter(const SourceDetails& )
{
	hover = true;
	repaint();
}


void MidiFileDragAndDropper::itemDragExit(const SourceDetails& )
{
	hover = false;
	repaint();
}

bool MidiFileDragAndDropper::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	PoolReference ref(dragSourceDetails.description);
	return ref.isValid() && ref.getFileType() == FileHandlerBase::MidiFiles;
}


void MidiFileDragAndDropper::itemDropped(const SourceDetails& dragSourceDetails)
{
	PoolReference ref(dragSourceDetails.description);
	getPlayer()->loadMidiFile(ref);

	hover = false;
	repaint();
}

void MidiFileDragAndDropper::paint(Graphics& g)
{
	auto ar = getLocalBounds().toFloat();
	String message;

	if (!isActive()) 
		message << "Drop MIDI file here";
	else 
		message << "Drop MIDI file or Drag to external target";

	ScriptingObjects::ScriptedLookAndFeel::Laf slaf(getPlayer()->getMainController());

	slaf.drawMidiDropper(g, ar, message, *this);
}


void MidiFileDragAndDropper::LookAndFeelMethods::drawMidiDropper(Graphics& g, Rectangle<float> area, const String& text, MidiFileDragAndDropper& d)
{
	g.fillAll(d.findColour(HiseColourScheme::ComponentBackgroundColour));

	if (d.hover)
	{
		g.setColour(d.findColour(HiseColourScheme::ComponentOutlineColourId));
		g.drawRect(area, 3.0f);
	}

	g.setFont(d.getFont());
	g.setColour(d.findColour(HiseColourScheme::ComponentTextColourId));
	g.drawText(text, area, Justification::centred);
}

}