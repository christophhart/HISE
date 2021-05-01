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


namespace control
{

struct input_toggle_editor : public ScriptnodeExtraComponent<input_toggle<parameter::dynamic_base_holder>>
{
	using ObjType = input_toggle<parameter::dynamic_base_holder>;

	input_toggle_editor(ObjType* t, PooledUIUpdater* u) :
		ScriptnodeExtraComponent<ObjType>(t, u),
		dragger(u)
	{
		setSize(300, 24 + 3* UIValues::NodeMargin + 5);
		addAndMakeVisible(dragger);
	};

	void resized() override
	{
		auto b = getLocalBounds();
		b.removeFromTop(UIValues::NodeMargin);
		dragger.setBounds(b.removeFromTop(24));
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u)
	{
		auto typed = static_cast<ObjType*>(obj);
		return new input_toggle_editor(typed, u);
	}

	void timerCallback() override 
	{
		repaint();
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds().toFloat();
		b = b.removeFromBottom(5.0f + UIValues::NodeMargin * 2).reduced(0, UIValues::NodeMargin);
		
		b.removeFromLeft(b.getWidth() / 3.0f);

		auto l = b.removeFromLeft(b.getWidth() / 2.0f).reduced(3.0f, 0.0f);
		auto r = b.reduced(3.0f, 0.0f);

		auto c = findParentComponentOfClass<NodeComponent>()->header.colour;
		if (c == Colours::transparentBlack)
			c = Colour(0xFFADADAD);

		g.setColour(c.withAlpha(getObject()->useValue1 ? 1.0f : 0.2f));
		g.fillRoundedRectangle(l, l.getHeight() / 2.0f);
		g.setColour(c.withAlpha(!getObject()->useValue1 ? 1.0f : 0.2f));
		g.fillRoundedRectangle(r, r.getHeight() / 2.0f);

		
	}

	ModulationSourceBaseComponent dragger;
};

struct xy : 
	public pimpl::parameter_node_base<parameter::dynamic_list>,
	public pimpl::no_processing
{
	SET_HISE_NODE_ID("xy");
	SN_GET_SELF_AS_OBJECT(xy);

	enum class Parameters
	{
		X,
		Y
	};

	void initialise(NodeBase* n)
	{
		this->p.initialise(n);
		
		if constexpr (!parameter::dynamic_list::isStaticList())
		{
			getParameter().numParameters.storeValue(2, n->getUndoManager());
			getParameter().updateParameterAmount({}, 2);
		}
			
	}

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(X, xy);
		DEF_PARAMETER(Y, xy);
	};
	PARAMETER_MEMBER_FUNCTION;

	void setX(double v)
	{
		if(getParameter().getNumParameters() > 0)
			getParameter().call<0>(v);
	}

	void setY(double v)
	{
		if (getParameter().getNumParameters() > 1)
			getParameter().call<1>(v);
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(xy, X);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
		{
			DEFINE_PARAMETERDATA(xy, Y);
			p.setRange({ -1.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

	struct editor : public ScriptnodeExtraComponent<xy>
	{
		editor(xy* o, PooledUIUpdater* p) :
			ScriptnodeExtraComponent<xy>(o, p),
			xDragger(&o->getParameter(), 0),
			yDragger(&o->getParameter(), 1)
		{
			addAndMakeVisible(xDragger);
			addAndMakeVisible(yDragger);

			xDragger.textFunction = getAxis;
			yDragger.textFunction = getAxis;

			setSize(200, 200 + UIValues::NodeMargin);

			setRepaintsOnMouseActivity(true);
		};

		static String getAxis(int index)
		{
			return index == 0 ? "X" : "Y";
		}

		Array<Point<float>> lastPositions;

		Point<float> normalisedPosition;

		void mouseDrag(const MouseEvent& e) override
		{
			auto a = getXYArea().reduced(circleWidth / 2.0f);
			auto pos = e.getPosition().toFloat();

			auto xValue = (pos.getX() - a.getX()) / a.getWidth();
			auto yValue = 1.0f - (pos.getY() - a.getY()) / a.getHeight();

			findParentComponentOfClass<NodeComponent>()->node->getParameter(0)->setValueAndStoreAsync(xValue);
			findParentComponentOfClass<NodeComponent>()->node->getParameter(1)->setValueAndStoreAsync(yValue);
		}

		void timerCallback() override
		{
			for (auto o : getObject()->p.targets)
				o->p.updateUI();

			auto x = jlimit(0.0f, 1.0f, (float)getObject()->p.getParameter<0>().lastValue);
			auto y = jlimit(0.0f, 1.0f, (float)getObject()->p.getParameter<1>().lastValue);

			lastPositions.insert(0, normalisedPosition);

			if (lastPositions.size() >= 20)
				lastPositions.removeLast();

			normalisedPosition = { x, (1.0f - y) };
			repaint();
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromBottom(UIValues::NodeMargin);
			auto y = b.removeFromRight(28);
			y.removeFromBottom(28);

			yDragger.setBounds(y.reduced(2));
			xDragger.setBounds(b.removeFromBottom(28).reduced(2));
		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* u)
		{
			auto t = static_cast<xy*>(obj);
			return new editor(t, u);
		}

		static constexpr float circleWidth = 24.0f;

		Rectangle<float> getXYArea() const
		{
			auto b = getLocalBounds();
			b.removeFromRight(28);
			b.removeFromBottom(28 + UIValues::NodeMargin);
			
			return b.reduced(1).toFloat();
		}

		Rectangle<float> getDot(Point<float> p) const
		{
			auto a = getXYArea();
			auto aCopy = a.reduced(1.0f);
			auto c = aCopy.removeFromLeft(circleWidth).removeFromTop(circleWidth);

			c = c.withX(a.getX() + p.getX() * (a.getWidth() - c.getWidth()));
			c = c.withY(a.getY() + p.getY() * (a.getHeight() - c.getHeight()));

			return c;
		}

		void paint(Graphics& g) override
		{
			auto a = getXYArea();

			g.setColour(Colours::black.withAlpha(0.3f));
			g.fillRoundedRectangle(a, circleWidth / 2.0f);
			g.drawRoundedRectangle(a, circleWidth / 2.0f, 1.0f);

			g.drawVerticalLine(a.getCentreX(), a.getY() + 4.0f, a.getBottom() - 4.0f);
			g.drawHorizontalLine(a.getCentreY(), a.getX() + 4.0f, a.getRight() - 4.0f);

			auto c = getDot(normalisedPosition);

			auto fc = findParentComponentOfClass<NodeComponent>()->header.colour;

			if(fc == Colours::transparentBlack)
				fc = Colour(0xFFAAAAAA);

			g.setColour(fc);
			g.drawEllipse(c, 2.0f);
			g.fillEllipse(c.reduced(4.0f));

			float alpha = 0.3f;
			float s = 4.0f;

			Path p;
			p.startNewSubPath(c.getCentre());

			auto maxDistance = 0.0f;
			auto maxP = c.getCentre();

			for (auto pos : lastPositions)
			{
				auto pp = getDot(pos).getCentre();

				auto d = pp.getDistanceFrom(c.getCentre());

				if (d > maxDistance)
				{
					maxDistance = d;
					maxP = pp;
				}
				
				p.lineTo(pp);
			}
			
			p = p.createPathWithRoundedCorners(circleWidth);
			g.setGradientFill(ColourGradient(fc.withAlpha(0.5f), c.getCentre(), fc.withAlpha(0.0f), maxP, true));
			g.strokePath(p, PathStrokeType(3.0f, PathStrokeType::curved, PathStrokeType::rounded));
		}

		parameter::ui::dynamic_list_editor::DragComponent xDragger, yDragger;
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(xy);
};

}



using namespace juce;
using namespace hise;



namespace analyse
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<wrap::data<fft, data::dynamic::displaybuffer>, ui::fft_display>();
	registerNode<wrap::data<oscilloscope, data::dynamic::displaybuffer>, ui::osc_display>();
	registerNode<wrap::data<goniometer, data::dynamic::displaybuffer>, ui::gonio_display>();
}

}


namespace dynamics
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerModNode<dp<gate>, data::ui::displaybuffer_editor>();
	registerModNode<dp<comp>, data::ui::displaybuffer_editor >();
	registerModNode<dp<limiter>, data::ui::displaybuffer_editor >();
	registerModNode<dp<envelope_follower>, data::ui::displaybuffer_editor >();
}

}

namespace fx
{
	struct simple_visualiser : public ScriptnodeExtraComponent<NodeBase>
	{
		simple_visualiser(NodeBase*, PooledUIUpdater* u) :
			ScriptnodeExtraComponent<NodeBase>(nullptr, u)
		{
			setSize(100, 60);
		}

		NodeBase* getNode()
		{
			return findParentComponentOfClass<NodeComponent>()->node.get();
		}

		double getParameter(int index)
		{
			jassert(isPositiveAndBelow(index, getNode()->getNumParameters()));
			return getNode()->getParameter(index)->getValue();
		}

		bool stroke = true;

		Colour getNodeColour()
		{
			auto c = findParentComponentOfClass<NodeComponent>()->header.colour;

			if (c == Colours::transparentBlack)
				return Colour(0xFFAAAAAA);

			return c;
		}

		void timerCallback() override
		{
			original.clear();
			p.clear();
			rebuildPath(p);

			auto b = getLocalBounds().toFloat().reduced(4.0f);
			p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);

			

			if (!original.isEmpty())
			{
				original.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);

				auto cp = original;

				float l[2] = { 2.0f, 2.0f };
				PathStrokeType(1.5f).createDashedStroke(original, cp, l, 2);
			}

			repaint();
		}

		virtual void rebuildPath(Path& path) = 0;
		
		void paint(Graphics& g) override
		{
			ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, getLocalBounds().toFloat(), false);
			g.setColour(getNodeColour());

			if (!original.isEmpty())
				g.fillPath(original);

			g.strokePath(p, PathStrokeType(1.0f));
		}

		Path original;

	private:

		Path p;
		
	};

	struct bitcrush_editor : simple_visualiser
	{
		bitcrush_editor(PooledUIUpdater* u) :
			simple_visualiser(nullptr, u)
		{};

		void rebuildPath(Path& p) override
		{
			span<float, 100> x;
			
			for (int i = 0; i < 100; i++)
				x[i] = (float)i / 100.0f;
			
			getBitcrushedValue(x, getParameter(0) / 2.5);
			
			p.startNewSubPath(0, 1.0f - x[0]);

			for (int i = 1; i < 100; i++)
				p.lineTo(i, 1.0f - x[i]);
		}

		static Component* createExtraComponent(void* , PooledUIUpdater* u)
		{
			return new bitcrush_editor(u);
		}
	};

	struct sampleandhold_editor : simple_visualiser
	{
		sampleandhold_editor(PooledUIUpdater* u) :
			simple_visualiser(nullptr, u)
		{};

		void rebuildPath(Path& p) override
		{
			span<float, 100> x;

			for (int i = 0; i < 100; i++)
				x[i] = hmath::sin(float_Pi * 2.0f * (float)i / 100.0f);

			auto delta = (int)(getNode()->getParameter(0)->getValue() / JUCE_LIVE_CONSTANT(10.0f));
			int counter = 0;
			float v = 0.0;

			for (int i = 0; i < 100; i++)
			{
				if (counter++ >= delta)
				{
					counter = 0;
					v = x[i];
				}

				x[i] = v;
			}
			
			

			//getBitcrushedValue(x,  / JUCE_LIVE_CONSTANT_OFF(2.5f));

			p.startNewSubPath(0, 1.0f - x[0]);

			for (int i = 1; i < 100; i++)
				p.lineTo(i, 1.0f - x[i]);
		}

		static Component* createExtraComponent(void*, PooledUIUpdater* u)
		{
			return new sampleandhold_editor(u);
		}
	};

	struct phase_delay_editor : public simple_visualiser
	{
		phase_delay_editor(PooledUIUpdater* u) :
			simple_visualiser(nullptr, u)
		{};

		void rebuildPath(Path& p) override
		{
			span<float, 100> x;

			for (int i = 0; i < 100; i++)
				x[i] = hmath::sin(float_Pi * 2.0f * (float)i / 100.0f);

			auto v = getParameter(0);

			NormalisableRange<double> fr(20.0, 20000.0);
			fr.setSkewForCentre(500.0);

			auto nv = fr.convertTo0to1(v);

			original.startNewSubPath(0, 1.0f - x[0]);

			for (int i = 1; i < 100; i++)
				original.lineTo(i, 1.0f - x[i]);

			p.startNewSubPath(0, 1.0f - x[50 + roundToInt(nv * 49)]);

			for (int i = 1; i < 100; i++)
			{
				auto index = (i + 50 + roundToInt(nv * 49)) % 100;

				p.lineTo(i, 1.0f - x[index]);
			}
		}

		static Component* createExtraComponent(void*, PooledUIUpdater* u)
		{
			return new phase_delay_editor(u);
		}
	};

	struct reverb_editor : simple_visualiser
	{
		reverb_editor(PooledUIUpdater* u) :
			simple_visualiser(nullptr, u)
		{};

		void rebuildPath(Path& p) override
		{
			auto damp = getParameter(0);
			auto width = getParameter(1);
			auto size = getParameter(2);

			p.startNewSubPath(0.0f, 0.0f);
			p.startNewSubPath(1.0f, 1.0f);

			Rectangle<float> base(0.5f, 0.5f, 0.0f, 0.0f);

			for (int i = 0; i < 8; i++)
			{
				auto ni = (float)i / 8.0f;
				ni = hmath::pow(ni, 1.0f + (float)damp);

				auto a = base.withSizeKeepingCentre(width * ni * 2.0f, size * ni);
				p.addRectangle(a);
			}
		}

		static Component* createExtraComponent(void*, PooledUIUpdater* u)
		{
			return new reverb_editor(u);
		}
	};


Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<reverb, reverb_editor>();
	registerPolyNode<sampleandhold, sampleandhold_poly, sampleandhold_editor>();
	registerPolyNode<bitcrush, bitcrush_poly, bitcrush_editor>();
	registerPolyNode<wrap::fix<2, haas>, wrap::fix<2, haas_poly>>();
	registerPolyNode<phase_delay, phase_delay_poly, phase_delay_editor>();
}

}

namespace stk_factory
{
Factory::Factory(DspNetwork* n):
	NodeFactory(n)
{
	registerNode<stk::nodes::jcrev>();
	registerNode<stk::nodes::delay_a>();
	registerNode<stk::nodes::banded_wg>();
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

namespace control
{
	using dynamic_cable_table = wrap::data<control::cable_table<parameter::dynamic_base_holder>, data::dynamic::table>;
	using dynamic_cable_pack = wrap::data<control::cable_pack<parameter::dynamic_base_holder>, data::dynamic::sliderpack>;

	using dynamic_smoother_parameter = control::smoothed_parameter<smoothers::dynamic>;

	using dynamic_dupli_pack = wrap::data<control::dupli_pack<parameter::dynamic_base_holder>, data::dynamic::sliderpack>;

	Factory::Factory(DspNetwork* network) :
		NodeFactory(network)
	{
		registerNoProcessNode<pma_editor::NodeType, pma_editor>();
		registerNoProcessNode<control::sliderbank_editor::NodeType, control::sliderbank_editor, false>();
		registerNoProcessNode<dynamic_cable_pack, data::ui::sliderpack_editor>();
		registerNoProcessNode<dynamic_cable_table, data::ui::table_editor>();
		
		registerNoProcessNode<control::input_toggle<parameter::dynamic_base_holder>, input_toggle_editor>();

		registerNoProcessNode<duplilogic::dynamic::NodeType, duplilogic::dynamic::editor>();
		registerNoProcessNode<dynamic_dupli_pack, data::ui::sliderpack_editor>();

		registerNoProcessNode<faders::dynamic::NodeType, faders::dynamic::editor>();
		
		registerNoProcessNode<control::xy, control::xy::editor>();

		registerModNode<smoothers::dynamic::NodeType, smoothers::dynamic::editor>();

#if HISE_INCLUDE_SNEX
		registerModNode<midi_logic::dynamic::NodeType, midi_logic::dynamic::editor>();
		registerPolyModNode<control::timer<snex_timer>, timer_poly<snex_timer>, snex_timer::editor>();
#endif

		registerNoProcessNode<file_analysers::dynamic::NodeType, file_analysers::dynamic::editor, false>(); //>();

		registerNodeRaw<InterpretedUnisonoWrapperNode>();
	}
}

namespace core
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;


struct osc_display : public Component,
					 public RingBufferComponentBase,
					 public ComponentWithDefinedSize
{
	void refresh() override
	{
		waveform.clear();

		if (rb != nullptr)
		{
			auto b = getLocalBounds().reduced(70, 3).toFloat();

			waveform.startNewSubPath(0.0f, 0.0f);

			const auto& bf = rb->getReadBuffer();

			for (int i = 0; i < 256; i++)
			{
				waveform.lineTo((float)i, -1.0f * bf.getSample(0, i));
			}

			waveform.lineTo(255.0f, 0.0f);

			if (!waveform.getBounds().isEmpty())
				waveform.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), false);
		}

		repaint();
	}

	void paint(Graphics& g) override
	{
		auto laf = getSpecialLookAndFeel<RingBufferComponentBase::LookAndFeelMethods>();
		auto b = getLocalBounds().reduced(70, 3).toFloat();

		laf->drawOscilloscopeBackground(g, *this, b.expanded(3.0f));

		Path grid;
		grid.addRectangle(b);

		laf->drawAnalyserGrid(g, *this, grid);

		if(!waveform.getBounds().isEmpty())
			laf->drawOscilloscopePath(g, *this, waveform);
	}

	Rectangle<int> getFixedBounds() const override { return { 0, 0, 300, 60 }; }

	Colour getColourForAnalyserBase(int colourId) override { return Colours::transparentBlack; }

	Path waveform;
};

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	

	registerNode<fix_delay>();
	registerNode<fm>();
	
	registerModNode<wrap::data<core::table,       data::dynamic::table>,     data::ui::table_editor>();

	using mono_file_player = wrap::data<core::file_player<1>, data::dynamic::audiofile>;
	using poly_file_player = wrap::data<core::file_player<NUM_POLYPHONIC_VOICES>, data::dynamic::audiofile>;

	registerPolyNode<mono_file_player, poly_file_player, data::ui::audiofile_editor>();
	registerNode   <wrap::data<core::recorder,    data::dynamic::audiofile>, data::ui::audiofile_editor>();

	registerPolyNode<gain, gain_poly>();

	registerModNode<tempo_sync, TempoDisplay>();

#if HISE_INCLUDE_SNEX
	registerPolyNode<snex_osc<SnexOscillator>, snex_osc_poly<SnexOscillator>, NewSnexOscillatorDisplay>();
	registerNode<core::snex_node, core::snex_node::editor>();
	registerNode<waveshapers::dynamic::NodeType, waveshapers::dynamic::editor>();
#endif

	registerModNode<hise_mod>();
	
	registerModNode<dp<peak>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<ramp>, dp<ramp_poly>, data::ui::displaybuffer_editor>();

	registerNode<core::mono2stereo>();

	using osc_display_ = data::ui::pimpl::editorT<data::dynamic::displaybuffer,
		hise::SimpleRingBuffer,
		osc_display,
		false>;

	registerPolyNode<dp<core::oscillator>, dp<core::oscillator_poly>, osc_display_>();
}
}

namespace filters
{
template <typename FilterType> using df = wrap::data<FilterType, data::dynamic::filter>;

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	using namespace data::ui;

	registerPolyNode<df<one_pole>,		df<one_pole_poly>,		filter_editor>();
	registerPolyNode<df<svf>,			df<svf_poly>,			filter_editor>();
	registerPolyNode<df<svf_eq>,		df<svf_eq_poly>,		filter_editor>();
	registerPolyNode<df<biquad>,		df<biquad_poly>,		filter_editor>();
	registerPolyNode<df<ladder>,		df<ladder_poly>,		filter_editor>();
	registerPolyNode<df<ring_mod>,		df<ring_mod_poly>,		filter_editor>();
	registerPolyNode<df<moog>,			df<moog_poly>,			filter_editor>();
	registerPolyNode<df<allpass>,		df<allpass_poly>,		filter_editor>();
	registerPolyNode<df<linkwitzriley>,	df<linkwitzriley_poly>, filter_editor>();

	registerNode<wrap::data<convolution, data::dynamic::audiofile>, data::ui::audiofile_editor>();
	//registerPolyNode<fir, fir_poly>();
}
}

}
