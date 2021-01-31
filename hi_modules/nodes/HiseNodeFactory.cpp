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

namespace scriptnode
{

using namespace juce;
using namespace hise;

namespace faders
{
	struct switcher
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			auto numParameters = (double)(numElements - 1);
			auto indexToActivate = roundToInt(normalisedInput * numParameters);

			return (double)(indexToActivate == Index);
		}
	};

	struct overlap
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			if (isPositiveAndBelow(Index, numElements))
			{
				switch (numElements)
				{
				case 2:
				{
					switch (Index)
					{
					case 0: return jlimit(0.0, 1.0, 2.0 - 2.0 * normalisedInput);
					case 1: return jlimit(0.0, 1.0, 2.0 * normalisedInput);
					}
				}
				case 3:
				{
					jassertfalse;
				}
				}
			}

			return 0.0;
		}
	};

	struct linear
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			if (isPositiveAndBelow(Index, numElements))
			{
				switch (numElements)
				{
				case 2:
				{
					switch (Index)
					{
					case 0: return 1.0 - normalisedInput;
					case 1: return normalisedInput;
					}
				}
				case 3:
				{
					switch (Index)
					{
					case 0: return jlimit(0.0, 1.0, 1.0 - normalisedInput * 2.0);
					case 1: return jlimit(0.0, 1.0, 1.0 - Math.abs(1.0 - 2.0 * normalisedInput));
					case 2: return jlimit(0.0, 1.0, normalisedInput * 2.0 - 1.0);
					}
					jassertfalse;
				}
				}
			}

			return 0.0;
		}

		hmath Math;
	};

	struct equal_power
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			auto v = lf.getFadeValue<Index>(numElements, normalisedInput);

			hmath::sin(hmath::PI / 2.0 * v);

			return hmath::sqrt(v);
		}

		linear lf;
	};

	struct rms
	{
		HISE_EMPTY_INITIALISE;

		template <int Index> double getFadeValue(int numElements, double normalisedInput)
		{
			auto v = lf.getFadeValue<Index>(numElements, normalisedInput);

			if (v == 0.0)
				return v;

			return hmath::sqrt(v);
		}

		linear lf;
	};

	struct dynamic
	{
		enum FaderMode
		{
			Switch,
			Linear,
			Overlap,
			EqualPower,
			RMS,
		};

		dynamic():
			mode(PropertyIds::Mode, FaderMode::Linear)
		{}

		void initialise(NodeBase* n)
		{
			mode.initialise(n);
		}

		static StringArray getFaderModes()
		{
			return { "Switch", "Linear", "Overlap", "EqualPower", "RMS" };
		}

		template <int Index> double getFadeValue(int numElements, double v)
		{
			switch (mode.getValue())
			{
			case FaderMode::Switch:		return sf.getFadeValue<Index>(numElements, v);
			case FaderMode::Linear:		return lf.getFadeValue<Index>(numElements, v);
			case FaderMode::Overlap:	return of.getFadeValue<Index>(numElements, v);
			case FaderMode::EqualPower: return ef.getFadeValue<Index>(numElements, v);
			case FaderMode::RMS:		return rf.getFadeValue<Index>(numElements, v);
			}

			return 0.0;
		}

		NodePropertyT<int> mode;

		switcher sf;
		linear lf;
		rms rf;
		overlap of;
		equal_power ef;
	};
}

namespace parameter
{

struct dynamic_list
{
	dynamic_list() :
		numParameters(PropertyIds::NumParameters, 0)
	{};

	valuetree::RecursiveTypedChildListener connectionUpdater;

	NodePropertyT<int> numParameters;

	void initialise(NodeBase* n)
	{
		parentNode = n;

		switchTree = n->getValueTree().getOrCreateChildWithName(PropertyIds::SwitchTargets, n->getUndoManager());

		connectionUpdater.setTypeToWatch(PropertyIds::SwitchTarget);
		connectionUpdater.setCallback(switchTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(dynamic_list::updateConnections));

		numParameters.initialise(n);
		numParameters.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic_list::updateParameterAmount));
		rebuildParameters();
	}

	void updateConnections(ValueTree& v, bool wasAdded)
	{
		auto outputIndex = v.getParent().indexOf(v);
		auto ok = rebuildDynamicParameters(outputIndex);
		
		if (!ok)
		{
			WeakReference<dynamic_list> safeThis(this);

			auto f = [safeThis, outputIndex]()
			{
				if (safeThis.get() == nullptr)
					return;

				safeThis.get()->rebuildDynamicParameters(outputIndex);
			};

			MessageManager::callAsync(f);
		}
	}

	bool rebuildDynamicParameters(int outputIndex)
	{
		auto cTree = switchTree.getChild(outputIndex).getChild(0);

		try
		{
			auto p = ConnectionBase::createParameterFromConnectionTree(parentNode, cTree, true);
			targets.set(outputIndex, p);
			return true;
		}
		catch (String& s)
		{
			missingNodes = s;
			targets.set(outputIndex, new parameter::dynamic_chain());
			return false;
		}
	}

	void updateParameterAmount(Identifier id, var newValue)
	{
		rebuildParameters();
	}

	void rebuildParameters()
	{
		size = numParameters.getValue();

		int numToRemove = targets.size() - size;
		int numToAdd = -numToRemove;

		if (numToAdd == 0)
			return;

		if (numToRemove > 0)
		{
			for (int i = size; i < numToRemove; i++)
			{
				targets.remove(i);
				switchTree.removeChild(i, parentNode->getUndoManager());
			}
		}
		else
		{
			for (int i = 0; i < numToAdd; i++)
			{
				ValueTree sv(PropertyIds::SwitchTarget);
				ValueTree cv(PropertyIds::Connections);
				sv.addChild(cv, -1, nullptr);

				switchTree.addChild(sv, -1, parentNode->getUndoManager());
				targets.add(new dynamic_chain());
			}
		}

		for (int i = 0; i < targets.size(); i++)
			rebuildDynamicParameters(i);
	}

	template <int P> dynamic_chain& getParameter()
	{
		jassert(isPositiveAndBelow(P, size));
		return *targets[P];
	}

	template <int P> void call(double v)
	{
		if (isPositiveAndBelow(P, size))
			targets[P]->call(v);
	}

	ValueTree switchTree;
	NodeBase* parentNode;
	int size = 0;
	
	String missingNodes;

	OwnedArray<dynamic_chain> targets;
	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_list);
};





}



namespace core
{

template <typename ParameterClass, typename FaderClass> struct switcher
{
	SET_HISE_NODE_ID("switcher");
	SN_GET_SELF_AS_OBJECT(switcher);

	enum Parameters
	{
		Value
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Value, switcher);
	}
	PARAMETER_MEMBER_FUNCTION;

	bool isPolyphonic() const { return false; };

	void initialise(NodeBase* n)
	{
		p.initialise(n);
		fader.initialise(n);
	}

	HISE_EMPTY_HANDLE_EVENT;
	HISE_EMPTY_MOD;
	HISE_EMPTY_PROCESS;
	HISE_EMPTY_PREPARE;
	HISE_EMPTY_PROCESS_SINGLE;
	HISE_EMPTY_RESET;

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(switcher, Value);
			p.setRange({ 0.0, 1.0 });
			data.add(std::move(p));
		}
	}

	void setValue(double v)
	{
		lastValue.setModValueIfChanged(v);

		using FaderType = faders::switcher;

		p.call<0>(fader.getFadeValue<0>(p.size, v));
		p.call<1>(fader.getFadeValue<1>(p.size, v));
		p.call<2>(fader.getFadeValue<2>(p.size, v));
		p.call<3>(fader.getFadeValue<3>(p.size, v));
		p.call<4>(fader.getFadeValue<4>(p.size, v));
	}

	ModValue lastValue;

	ParameterClass p;
	FaderClass fader;

	JUCE_DECLARE_WEAK_REFERENCEABLE(switcher);
};

using dynamic_xfader = core::switcher<parameter::dynamic_list, faders::dynamic>;

struct Funky : public ScriptnodeExtraComponent<dynamic_xfader>,
	public ButtonListener,
	public ComboBoxListener
{
	static constexpr int GraphHeight = 70;
	static constexpr int ButtonHeight = 22;
	static constexpr int DragHeight = 48;

	struct Factory : public PathFactory
	{
		String getId() const override { return {}; }

		Path createPath(const String& url) const override
		{
			Path p;
			LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
			LOAD_PATH_IF_URL("delete", SampleMapIcons::deleteSamples);
			LOAD_PATH_IF_URL("drag", ColumnIcons::targetIcon);

			return p;
		}
	};

	struct DragComponent : public Component,
		public MultiOutputDragSource
	{
		DragComponent(Funky& parent, int index_) :
			index(index_)
		{
			n = dynamic_cast<ModulationSourceNode*>(parent.getObject()->p.parentNode);
			p = Factory().createPath("drag");
		}



		NodeBase* getNode() const override {
			return n;
		}

		int getNumOutputs() const override
		{
			if (auto f = findParentComponentOfClass<Funky>())
				return f->getObject()->p.size;

			return 0;
		}

		int getOutputIndex() const override
		{
			return index;
		}

		bool matchesParameter(NodeBase::Parameter* p) const override
		{
			if (auto t = findParentComponentOfClass<Funky>()->getObject()->p.targets[index])
			{
				for (auto d : t->targets)
				{
					if (d->dataTree == p->data)
						return true;
				}
			}

			return false;
		}

		ModulationSourceNode* n;

		void mouseDrag(const MouseEvent& e)
		{
			if (auto container = findParentComponentOfClass<DragAndDropContainer>())
			{
				var d;

				DynamicObject::Ptr details = new DynamicObject();

				auto obj = findParentComponentOfClass<Funky>()->getObject();

				auto nodeId = obj->p.parentNode->getId();

				details->setProperty(PropertyIds::ID, nodeId);
				details->setProperty(PropertyIds::ParameterId, index);
				details->setProperty(PropertyIds::SwitchTarget, true);

				container->startDragging(var(details), this);
			}
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromTop(15);
			Factory::scalePath(p, b.toFloat().reduced(2.0f));
		}

		void paint(Graphics& g) override
		{
			float alpha = 0.7f;

			if (isMouseOver())
				alpha += 0.1f;

			if (isMouseButtonDown())
				alpha += 0.2f;

			auto c = getFadeColour(index, getNumOutputs());

			g.setColour(c);
			g.fillPath(p);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(String(index + 1), getLocalBounds().toFloat(), Justification::centredTop);
		}

		const int index;
		Path p;
	};

	struct FaderGraph : public Component
	{
		FaderGraph(dynamic_xfader* f):
			fader(f)
		{
			rebuildFaderCurves();
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.drawRect(getLocalBounds().toFloat(), 1.0f);

			auto xPos = (float)getWidth() * inputValue;

			Line<float> posLine(xPos, 0.0f, xPos, (float)getHeight());
			Line<float> scanLine(xPos, 5.0f, xPos, (float)getHeight() -5.0f);

			int index = 0;

			g.setColour(Colours::white.withAlpha(0.7f));
			g.drawLine(posLine);

			for (auto& p : faderCurves)
			{
				auto c = MultiOutputDragSource::getFadeColour(index++, faderCurves.size());
				g.setColour(c.withMultipliedAlpha(0.7f));

				g.fillPath(p);
				g.setColour(c);
				g.strokePath(p, PathStrokeType(1.0f));

			}

			index = 0;

			for (auto& p : faderCurves)
			{
				auto c = MultiOutputDragSource::getFadeColour(index++, faderCurves.size());
				auto clippedLine = p.getClippedLine(scanLine, false);

				Point<float> circleCenter;

				if (clippedLine.getStartY() > clippedLine.getEndY())
					circleCenter = clippedLine.getEnd();
				else
					circleCenter = clippedLine.getStart();

				if (!circleCenter.isOrigin())
				{
					Rectangle<float> circle(circleCenter, circleCenter);
					g.setColour(c.withAlpha(1.0f));
					g.fillEllipse(circle.withSizeKeepingCentre(5.0f, 5.0f));
				}
			}
		}

		void setInputValue(double v)
		{
			inputValue = v;
			repaint();
		}

		template <int Index> Path createPath() const
		{
			int numPaths = fader->p.size;
			int numPixels = getWidth();

			Path p;
			p.startNewSubPath(0.0f, 0.0f);

			if (numPixels > 0)
			{
				for (int i = 0; i < numPixels; i++)
				{
					double inputValue = (double)i / (double)numPixels;
					auto output = (float)fader->fader.getFadeValue<Index>(numPaths, inputValue);

					p.lineTo((float)i, -1.0f * output);
				}
			}

			p.lineTo(numPixels-1, 0.0f);
			p.closeSubPath();

			return p;
		}

		void rebuildFaderCurves()
		{
			int numPaths = fader->p.size;

			faderCurves.clear();

			if (numPaths > 0)
				faderCurves.add(createPath<0>());
			if (numPaths > 1)
				faderCurves.add(createPath<1>());
			if (numPaths > 2)
				faderCurves.add(createPath<2>());
			if (numPaths > 3)
				faderCurves.add(createPath<3>());

			resized();
		}

		void resized() override
		{
			auto b = getLocalBounds().reduced(2).toFloat();

			if (!b.isEmpty())
			{
				for (auto& p : faderCurves)
				{
					if(!p.getBounds().isEmpty())
						p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);
				}

				repaint();
			}
		}

		double inputValue = 0.0;

		dynamic_xfader* fader;
		Array<Path> faderCurves;
	};
	

	Funky(dynamic_xfader* v, PooledUIUpdater* updater_) :
		ScriptnodeExtraComponent(v, updater_),
		updater(updater_),
		addButton("add", this, f),
		removeButton("delete", this, f),
		graph(v)
	{
		addAndMakeVisible(addButton);
		addAndMakeVisible(removeButton);

		fadeSelector.addItemList(faders::dynamic::getFaderModes(), 1);
		//fadeSelector.setLookAndFeel(&plaf);

		addAndMakeVisible(fadeSelector);
		fadeSelector.addListener(this);

		addAndMakeVisible(graph);

		setSize(256, ButtonHeight + DragHeight + GraphHeight);

		setRepaintsOnMouseActivity(true);
	};

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
	{
		getObject()->fader.mode.storeValue(comboBoxThatHasChanged->getSelectedItemIndex(), getObject()->p.parentNode->getUndoManager());

		graph.rebuildFaderCurves();
	}

	void timerCallback() override
	{
		for (auto o : getObject()->p.targets)
			o->updateUI();

		double v = 0.0;

		if (getObject()->lastValue.getChangedValue(v))
			graph.setInputValue(v);

		if (dragSources.size() != getObject()->p.size)
		{
			dragSources.clear();

			for (int i = 0; i < getObject()->p.size; i++)
			{
				auto n = new DragComponent(*this, i);
				dragSources.add(n);
				addAndMakeVisible(n);
			}

			resized();
		}
	};

	void resized() override
	{
		auto b = getLocalBounds();

		graph.setBounds(b.removeFromTop(GraphHeight));

		auto top = b.removeFromTop(ButtonHeight);

		b.removeFromBottom(10);

		addButton.setBounds(top.removeFromLeft(ButtonHeight).reduced(2));
		removeButton.setBounds(top.removeFromLeft(ButtonHeight).reduced(2));

		fadeSelector.setBounds(top.removeFromRight(128));

		if (dragSources.size() > 0)
		{
			auto wPerDrag = b.getWidth() / dragSources.size();

			for (auto d : dragSources)
				d->setBounds(b.removeFromLeft(wPerDrag));
		}
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds();
		b.removeFromTop(ButtonHeight);
		g.setColour(Colours::black.withAlpha(0.1f));
		g.fillRoundedRectangle(b.toFloat(), 2.0f);
	}

	void buttonClicked(Button* b) override
	{
		int newValue = 0;

		if (b == &addButton)
			newValue = jmin(getObject()->p.size + 1, 8);

		if (b == &removeButton)
		{
			newValue = jmax(0, getObject()->p.size - 1);
		}

		getObject()->p.parentNode->setNodeProperty(PropertyIds::NumParameters, newValue);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto v = static_cast<dynamic_xfader*>(obj);
		return new Funky(v, updater);
	}

	Factory f;
	HiseShapeButton addButton;
	HiseShapeButton removeButton;
	ComboBox fadeSelector;
	PooledUIUpdater* updater;
	OwnedArray<DragComponent> dragSources;
	PopupLookAndFeel plaf;
	FaderGraph graph;
};

}

namespace analyse
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
#if NOT_JUST_OSC
	registerNode<fft>({});
	registerNode<oscilloscope>({});
#endif
}

}


namespace dynamics
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
#if NOT_JUST_OSC
	registerNode<gate, ModulationSourcePlotter>();
	registerNode<comp, ModulationSourcePlotter>();
	registerNode<limiter, ModulationSourcePlotter>();
	registerNode<envelope_follower, ModulationSourcePlotter>();
#endif
}

}

namespace fx
{

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<reverb>();

#if NOT_JUST_OSC
	registerPolyNode<sampleandhold, sampleandhold_poly>({});
	registerPolyNode<bitcrush, bitcrush_poly>({});
	registerPolyNode<fix<2, haas>, fix<2, haas_poly>>({});
	registerPolyNode<phase_delay, phase_delay_poly>({});
	
#endif
}

}

namespace math
{

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	registerPolyNode<add, add_poly>();

	registerNode<clear>();

	registerPolyNode<tanh, tanh_poly>();

	registerPolyNode<mul, mul_poly>();

	
	registerPolyNode<sub, sub_poly>();
	registerPolyNode<div, div_poly>();
	
	registerPolyNode<clip, clip_poly>();
	registerNode<sin>();
	registerNode<pi>();
	registerNode<sig2mod>();
	registerNode<abs>();
	registerNode<square>();
	registerNode<sqrt>();
	registerNode<pow>();

	sortEntries();
}
}

namespace core
{

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerPolyNode<seq, seq_poly>();
	
	

	registerModNode<wrap::data<core::table, dynamic_table>, core::dynamic_table::display>();
	registerNode<fix_delay>();
	registerNode<file_player>();
	
	registerModNode<dynamic_xfader, Funky>();

	registerNode<fm>();
	
	registerPolyNode<gain, gain_poly>();
	//registerPolyNode<smoother, smoother_poly>();
	registerNode<new_jit>();

	registerNodeRaw<ParameterTableNode<wrap::data<cable_table<parameter::dynamic_base_holder>, dynamic_table>>>();

	registerModNode<core::midi<DynamicMidiEventProcessor>, MidiDisplay>();
	registerPolyModNode<timer<SnexEventTimer>, timer_poly<SnexEventTimer>, TimerDisplay>();
	registerPolyNode<snex_osc<SnexOscillator>, snex_osc_poly<SnexOscillator>, SnexOscillatorDisplay>();
	registerNodeRaw<ParameterMultiplyAddNode>();
	registerModNode<hise_mod>();
	registerModNode<tempo_sync, TempoDisplay>();
	registerModNode<peak>();
	registerPolyNode<ramp, ramp_poly>();
	registerNode<core::mono2stereo>();
	registerPolyNode<core::oscillator, core::oscillator_poly, OscDisplay>();

	registerModNode<smoothed_parameter>();
}
}

namespace filters
{

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	registerPolyNode<one_pole, one_pole_poly, FilterNodeGraph>();
	registerPolyNode<svf, svf_poly, FilterNodeGraph>();
	registerPolyNode<svf_eq, svf_eq_poly, FilterNodeGraph>();
	registerPolyNode<biquad, biquad_poly, FilterNodeGraph>();
	registerPolyNode<ladder, ladder_poly, FilterNodeGraph>();
	registerPolyNode<ring_mod, ring_mod_poly, FilterNodeGraph>();
	registerPolyNode<moog, moog_poly, FilterNodeGraph>();
	registerPolyNode<allpass, allpass_poly, FilterNodeGraph>();
	registerPolyNode<linkwitzriley, linkwitzriley_poly, FilterNodeGraph>();
	registerNode<convolution>();
	//registerPolyNode<fir, fir_poly>();
}
}

}
