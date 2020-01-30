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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once


namespace scriptnode
{
using namespace juce;

class SoulNodeComponent : public NodeComponent
{
public:

	SoulNodeComponent(SoulNode* s) :
		NodeComponent(s)
	{
		setSize(200, 200);
		rebuildListener.setCallback(s->getPropertyTree(), { PropertyIds::Value
			}, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(SoulNodeComponent::updatePosition));
	}

	void updatePosition(const ValueTree& v, const Identifier& id)
	{
		jassertfalse;
	}

	valuetree::RecursivePropertyListener rebuildListener;

};


struct SoulEditor : public FloatingTileContent,
					public Component,
					public ComboBox::Listener
{
	SoulEditor(FloatingTile* p) :
		FloatingTileContent(p),
		editor(doc, &tok)
	{
		addAndMakeVisible(editor);
		addAndMakeVisible(patchSelector);
		addAndMakeVisible(soulSelector);
		addAndMakeVisible(compileButton);

		fillComboBox();

		patchSelector.addListener(this);

	};

	void fillComboBox()
	{
		StringArray sa;

		auto soulRoot = getMainController()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile("soul");

		patchSelector.clear(dontSendNotification);

		int id = 1;
		int selected = -1;

		results = soulRoot.findChildFiles(File::findFiles, true, "*.soulpatch");

		for (auto patchFile : results)
		{
			patchSelector.addItem(patchFile.getRelativePathFrom(soulRoot), id++);
		}

		patchSelector.setSelectedItemIndex(results.indexOf(f), dontSendNotification);
	}

	void resized() override
	{
		auto b = getLocalBounds();

		patchSelector.setBounds(b.removeFromTop(24));
		compileButton.setBounds(b.removeFromBottom(32));
	}

	void comboBoxChanged(ComboBox* c) override
	{
		if (c == &patchSelector)
		{
			f = results[patchSelector.getSelectedItemIndex()];
			doc.replaceAllContent(f.loadFileAsString());
		}
	}

	Array<File> results;

	ComboBox patchSelector;
	ComboBox soulSelector;
	TextButton compileButton;

	CodeDocument doc;
	CPlusPlusCodeTokeniser tok;
	CodeEditorComponent editor;
	
	File f;
};


}
