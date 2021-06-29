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


#pragma once

namespace hise {
using namespace juce;



class MidiFileDragAndDropper : public Component,
	public MidiPlayerBaseType,
	public FileDragAndDropTarget,
	public DragAndDropContainer,
	public DragAndDropTarget
{
public:

	ENABLE_OVERLAY_FACTORY(MidiFileDragAndDropper, "Drag 'n Drop");

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {};

		virtual void drawMidiDropper(Graphics& g, Rectangle<float> area, const String& text, MidiFileDragAndDropper& d);
	};

	MidiFileDragAndDropper(MidiPlayer* player);;

	static bool isMidiFile(const String& s);

	int getPreferredHeight() const override { return 40; }
	void sequenceLoaded(HiseMidiSequence::Ptr newSequence) override;
	void sequencesCleared() override;;

	bool shouldDropFilesWhenDraggedExternally(const DragAndDropTarget::SourceDetails &sourceDetails, StringArray &files, bool &canMoveFiles);
	void dragOperationEnded(const DragAndDropTarget::SourceDetails&) override;
	void mouseDown(const MouseEvent& event) override;
	Image createSnapshot() const;

	bool isInterestedInFileDrag(const StringArray& files) override;
	void fileDragEnter(const StringArray&, int, int) override;
	void fileDragExit(const StringArray&) override;
	void filesDropped(const StringArray& files, int, int) override;

	void itemDragEnter(const SourceDetails& dragSourceDetails) override;
	void itemDragExit(const SourceDetails& dragSourceDetails) override;

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;

	void itemDropped(const SourceDetails& dragSourceDetails) override;

	void paint(Graphics& g) override;

	void setEnableExternalDrag(bool shouldDragToExternalTarget)
	{
		externalDrag = shouldDragToExternalTarget;
	}

	bool isActive() const { return currentSequence != nullptr; }

	bool hover = false;
	bool externalDrag = false;

private:
	
	bool enableExternalDrag = true;

	int lastPlayedIndex = -1;
	File draggedTempFile;
	HiseMidiSequence::Ptr currentSequence;
	Identifier currentSequenceId;
};



}