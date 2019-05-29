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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;


struct SequencerNode::SequencerInterface : public HiseDspBase::ExtraComponent<SequencerNode>
{
	SequencerInterface(PooledUIUpdater* updater, SequencerNode* s) :
		ExtraComponent(s, updater),
		pack(s->packData),
		dragger(updater)
	{
		addAndMakeVisible(pack);
		addAndMakeVisible(dragger);
		setSize(512, 100);
		stop(); // hammertime
	}

	void timerCallback() override {};

	void resized() override
	{
		auto b = getLocalBounds();
		pack.setBounds(b.removeFromTop(80));
		dragger.setBounds(b);
	}

	ModulationSourceBaseComponent dragger;
	hise::SliderPack pack;
	ComboBox selector;
};


juce::Component* SequencerNode::createExtraComponent(PooledUIUpdater* updater)
{
	return new SequencerInterface(updater, this);
}

void SequencerNode::createParameters(Array<ParameterData>& data)
{
	jassert(sp != nullptr);

	{
		ParameterData p("SliderPack");

		double maxPacks = (double)sp.get()->getNumSliderPacks();

		p.range = { 0.0, jmax(1.0, maxPacks), 1.0 };
		p.db = BIND_MEMBER_FUNCTION_1(SequencerNode::setSliderPack);

		data.add(std::move(p));
	}
}

void SequencerNode::initialise(NodeBase* n)
{
	sp = dynamic_cast<SliderPackProcessor*>(n->getScriptProcessor());
}


void SequencerNode::setSliderPack(double indexAsDouble)
{
	jassert(sp != nullptr);
	auto index = (int)indexAsDouble;
	packData = sp.get()->getSliderPackData(index);
}

struct TableNode::TableInterface : public HiseDspBase::ExtraComponent<TableNode>
{
	TableInterface(PooledUIUpdater* updater, TableNode* t) :
		ExtraComponent(t, updater),
		editor(nullptr, t->tableData),
		dragger(updater)
	{
		addAndMakeVisible(editor);
		addAndMakeVisible(dragger);
		setSize(512, 90);
	}

	void timerCallback() override
	{
		if (editor.getEditedTable() != getObject()->tableData)
		{
			editor.setEditedTable(getObject()->tableData);
		}
	};

	void resized() override
	{
		auto b = getLocalBounds();

		editor.setBounds(b.removeFromTop(80));
		dragger.setBounds(b);
	}

	ModulationSourceBaseComponent dragger;
	hise::TableEditor editor;
	ComboBox selector;
};

juce::Component* TableNode::createExtraComponent(PooledUIUpdater* updater)
{
	return new TableInterface(updater, this);
}

void TableNode::createParameters(Array<ParameterData>& data)
{
	if (tp != nullptr)
	{
		ParameterData p("TableIndex");

		double maxPacks = (double)tp.get()->getNumTables();

		p.range = { 0.0, jmax(1.0, maxPacks), 1.0 };
		p.db = BIND_MEMBER_FUNCTION_1(TableNode::setTable);

		data.add(std::move(p));
	}
}


void TableNode::initialise(NodeBase* n)
{
	tp = dynamic_cast<LookupTableProcessor*>(n->getScriptProcessor());
}

void TableNode::setTable(double indexAsDouble)
{
	jassert(tp != nullptr);
	auto index = (int)indexAsDouble;
	tableData = dynamic_cast<SampleLookupTable*>(tp.get()->getTable(index));
}



}

