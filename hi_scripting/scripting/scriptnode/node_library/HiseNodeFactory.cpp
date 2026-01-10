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







namespace scriptnode
{






namespace control
{

struct input_toggle_editor : public ScriptnodeExtraComponent<input_toggle_base>
{
	using ObjType = input_toggle_base;

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

		g.setColour(c.withAlpha(getObject()->getUIData().useValue1 ? 1.0f : 0.2f));
		g.fillRoundedRectangle(l, l.getHeight() / 2.0f);
		g.setColour(c.withAlpha(!getObject()->getUIData().useValue1 ? 1.0f : 0.2f));
		g.fillRoundedRectangle(r, r.getHeight() / 2.0f);
	}

	ModulationSourceBaseComponent dragger;
};







struct TransportDisplay : public juce::ComponentWithMiddleMouseDrag,
						  public PooledUIUpdater::SimpleTimer
{
	TransportDisplay(PooledUIUpdater* updater) :
		SimpleTimer(updater),
		dragger(updater)
	{
		addAndMakeVisible(dragger);

		setSize(128, 32);
	};

	void resized() override
	{
		auto b = getLocalBounds();

		iconBounds = b.removeFromLeft(b.getHeight()).toFloat().reduced(4);

		dragger.setBounds(b);
		repaint();
	}

	void paint(Graphics& g) override
	{
		MidiPlayerBaseType::TransportPaths f;
		auto p = f.createPath(isPlaying ? "Start" : "Stop");
		f.scalePath(p, iconBounds);
		g.setColour(Colours::white.withAlpha(0.8f));
		g.fillPath(p);

	}

	bool isPlaying = false;

	void timerCallback() override
	{
		if (auto nc = findParentComponentOfClass<NodeComponent>())
		{
            auto mc = nc->node->getScriptProcessor()->getMainController_();
            
			auto shouldBePlaying = mc->getMasterClock().isPlaying();

			if (isPlaying != shouldBePlaying)
			{
				isPlaying = shouldBePlaying;
				repaint();
			}
		}
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new TransportDisplay(updater);
	}

	ModulationSourceBaseComponent dragger;

	Rectangle<float> iconBounds;
};

struct TempoDisplay : public ModulationSourceBaseComponent
{
	using ObjectType = tempo_sync_base;

	TempoDisplay(PooledUIUpdater* updater, ObjectType* p_) :
		ModulationSourceBaseComponent(updater),
		p(p_)
	{
		setSize(200, 40);
	}

	static Component* createExtraComponent(void *p, PooledUIUpdater* updater)
	{
		auto t = static_cast<mothernode*>(p);

		return new TempoDisplay(updater, dynamic_cast<ObjectType*>(t));
	}

	void timerCallback() override
	{
		if (p == nullptr)
			return;

		auto td = p->getUIData();

		auto thisValue = td.currentTempoMilliseconds;

		if (thisValue != lastValue)
		{
			lastValue = thisValue;
			repaint();
		}

		auto now = Time::getMillisecondCounter();

		if (now - lastTime > thisValue)
		{
			on = !on;
			repaint();
			lastTime = now;
		}
	}

	void paint(Graphics& g) override
	{
		String n = String((int)lastValue) + " ms";

		auto b = getLocalBounds().toFloat().reduced(6.0f);

		g.setColour(Colours::black.withAlpha(0.1f));
		g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

		auto c = findParentComponentOfClass<NodeComponent>()->header.colour;

		if (c == Colours::transparentBlack)
			c = Colour(0xFFAAAAAA);

		g.setColour(c);
		g.setFont(GLOBAL_BOLD_FONT());

		Path p;
		p.loadPathFromData(ColumnIcons::targetIcon, SIZE_OF_PATH(ColumnIcons::targetIcon));

		PathFactory::scalePath(p, b.removeFromLeft(b.getHeight()).reduced(3));

		g.fillPath(p);

		auto r = b.removeFromRight(b.getHeight()).reduced(6.0f);

		g.drawText(n, b, Justification::centred);

		g.drawEllipse(r, 2.0f);

		if (on)
			g.fillEllipse(r.reduced(4.0f));
	}

	double lastValue = 0.0;

	bool on = false;

	uint32_t lastTime;

	WeakReference<ObjectType> p;
};

struct resetter_editor: public ScriptnodeExtraComponent<control::resetter<parameter::dynamic_base_holder>>
{
	using NodeType = control::resetter<parameter::dynamic_base_holder>;

	resetter_editor(NodeType* n, PooledUIUpdater* u):
		ScriptnodeExtraComponent<NodeType>(n, u),
		dragger(u)
	{
		setSize(100, 30);
		addAndMakeVisible(dragger);
	}

	void timerCallback() override
	{
		auto lastAlpha = flashAlpha;

		if (lastCounter != getObject()->flashCounter)
			flashAlpha = 0.7f;
		else
			flashAlpha = jmax(0.1f, flashAlpha * 0.8f);

		lastCounter = getObject()->flashCounter;

		if(flashAlpha != lastAlpha)
			repaint();
	}

	void paint(Graphics& g) override
	{
		Colour c;

		if (auto nc = findParentComponentOfClass<NodeComponent>())
			c = nc->header.colour;

		if (c == Colours::transparentBlack)
			c = Colours::white;

		g.setColour(c.withAlpha(0.2f));
		g.drawEllipse(area.reduced(2.0f), 1.0f);

		g.setColour(c.withAlpha(flashAlpha));
		g.fillEllipse(area.reduced(6.0f));
	}

	void resized() override
	{
		auto b = getLocalBounds();
		area = b.removeFromRight(b.getHeight()).toFloat().reduced(3.0f);
		dragger.setBounds(getLocalBounds());
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u)
	{
		auto mn = static_cast<mothernode*>(obj);
		return new resetter_editor(dynamic_cast<NodeType*>(mn), u);
	}

	int lastCounter = 0;
	float flashAlpha = 0.0f;
	Rectangle<float> area;

	ModulationSourceBaseComponent dragger;
};


struct xy_editor : public ScriptnodeExtraComponent<control::xy<parameter::dynamic_list>>
{
	using NodeType = control::xy<parameter::dynamic_list>;

	xy_editor(NodeType* o, PooledUIUpdater* p) :
		ScriptnodeExtraComponent<NodeType>(o, p),
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

		findParentComponentOfClass<NodeComponent>()->node->getParameterFromIndex(0)->setValueSync(xValue);
		findParentComponentOfClass<NodeComponent>()->node->getParameterFromIndex(1)->setValueSync(yValue);
	}

	void timerCallback() override
	{
		auto x = jlimit(0.0f, 1.0f, (float)getObject()->p.getParameter<0>().getDisplayValue());
		auto y = jlimit(0.0f, 1.0f, (float)getObject()->p.getParameter<1>().getDisplayValue());

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
		auto t = static_cast<NodeType*>(obj);
		return new xy_editor(t, u);
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

		if (fc == Colours::transparentBlack)
			fc = Colour(0xFFAAAAAA);

		g.setColour(fc);
		g.drawEllipse(c, 2.0f);
		g.fillEllipse(c.reduced(4.0f));

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

}



using namespace juce;
using namespace hise;



namespace analyse
{

using osc_display = data::ui::pimpl::editorT<data::dynamic::displaybuffer,
	hise::SimpleRingBuffer,
	ui::simple_osc_display,
	false>;

using fft_display = data::ui::pimpl::editorT<data::dynamic::displaybuffer,
	hise::SimpleRingBuffer,
	ui::simple_fft_display,
	false>;

using gonio_display = data::ui::pimpl::editorT<data::dynamic::displaybuffer,
	hise::SimpleRingBuffer,
	ui::simple_gon_display,
	false>;

struct SpecNode: public NodeBase
{
	SN_NODE_ID("specs");

	struct Comp : public NodeComponent,
				  public PooledUIUpdater::SimpleTimer
	{
		Comp(SpecNode* b) :
			NodeComponent(b),
			SimpleTimer(b->getRootNetwork()->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
		{
			start();
		};

		void timerCallback() override
		{

		}

		void paint(Graphics& g) override
		{
			NodeComponent::paint(g);
			
			Colour w1 = Colours::white.withAlpha(0.6f);
			Colour w2 = Colours::white.withAlpha(0.9f);

			Font f1 = GLOBAL_BOLD_FONT();
			Font f2 = GLOBAL_MONOSPACE_FONT();

			auto specs = dynamic_cast<SpecNode*>(node.get())->lastSpecs;

			AttributedString as;
			as.append("Channel Amount: ", f1, w1);
			as.append(String(roundToInt(specs.numChannels)) + "\n", f2, w2);
			as.append("Samplerate: ", f1, w1);
			as.append(String(roundToInt(specs.sampleRate)) + " | ", f2, w2);
			as.append("Block Size: ", f1, w1);
			as.append(String(roundToInt(specs.blockSize)) + "\n", f2, w2);
			as.append("MIDI: ", f1, w1);
			as.append(String(dynamic_cast<SpecNode*>(node.get())->processMidi ? "true | " : "false |"), f2, w2);
			as.append("Polyphony: ", f1, w1);

			auto polyEnabled = specs.voiceIndex != nullptr && specs.voiceIndex->isEnabled();

			as.append(polyEnabled ? "true\n" : "false\n", f2, w2);

			if (polyEnabled)
			{
				if (auto vr = specs.voiceIndex->getVoiceResetter())
				{
					as.append("NumActiveVoices: ", f1, w1);
					as.append(String(vr->getNumActiveVoices()) + "\n", f2, w2);
				}
			}
            
            as.append("Values: ", f1, w1);
            
            String valueString;
            valueString << "[ ";
            
            auto peaks = dynamic_cast<SpecNode*>(node.get())->peaks;
            
            for(int i = 0; i < specs.numChannels; i++)
            {
                valueString << snex::Types::Helpers::getCppValueString(peaks[i]);
                
                if(i != specs.numChannels-1)
                    valueString << ", ";
            }

            valueString << " ]\n";
            as.append(valueString, f2, w2);
            
			auto b = getLocalBounds();
			b.removeFromTop(header.getHeight());
			b = b.reduced(UIValues::NodeMargin);

			ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, b.toFloat(), false);

			as.draw(g, b.toFloat().reduced(UIValues::NodeMargin));
		}
	};

	SpecNode(DspNetwork* n, ValueTree v) :
		NodeBase(n, v, 0)
	{
		cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::UncompileableNode);
	};

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		return new SpecNode(n, v);
	}

	void process(ProcessDataDyn& data) override
	{
		lastMs = Time::getMillisecondCounter();
        
        int idx = 0;
        
        for(auto& ch: data)
        {
            auto b = data.toChannelData(ch);
            
            auto r = FloatVectorOperations::findMinAndMax(b.begin(), b.size());
            
            if(hmath::abs(r.getStart()) > hmath::abs(r.getEnd()))
                peaks[idx++] = r.getStart();
            else
                peaks[idx++] = r.getEnd();
        }
	}

	void reset() override
	{
        FloatVectorOperations::clear(peaks.begin(), peaks.size());
	};

	void processFrame(FrameType& data) override
	{
		lastMs = Time::getMillisecondCounter();
        
        FloatVectorOperations::copy(peaks.begin(), data.begin(), data.size());
	}

	void prepare(PrepareSpecs ps) override
	{
		lastSpecs = ps;

		try
		{
			ScriptnodeExceptionHandler::validateMidiProcessingContext(this);
			processMidi = true;
		}
		catch (scriptnode::Error& )
		{
			processMidi = false;
		}
	}

	NodeComponent* createComponent() override
	{
		return new Comp(this);
	}

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		Rectangle<int> x = { topLeft.getX(), topLeft.getY(), 256, 150 };
		return getBoundsToDisplay(x);
	}

	uint32 lastMs;
	PrepareSpecs lastSpecs;
	bool processMidi = false;
    
    span<float, NUM_MAX_CHANNELS> peaks;
};

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerNode<wrap::data<fft, data::dynamic::displaybuffer>, fft_display>();
	registerNode<wrap::data<oscilloscope, data::dynamic::displaybuffer>, osc_display>();
	registerNode<wrap::data<goniometer, data::dynamic::displaybuffer>, gonio_display>();

	registerPolyNodeRaw<SpecNode, SpecNode>();

}

}


namespace dynamics
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;

struct updown_editor: public ScriptnodeExtraComponent<data::pimpl::dynamic_base>
{
	struct viz: public simple_visualiser
	{
		viz(PooledUIUpdater* updater):
		  simple_visualiser(nullptr, updater)
		{};

		void rebuildPath(Path& p) override
		{
			span<float, 100> x;
			
			for (int i = 0; i < 100; i++)
				x[i] = (float)i / 100.0f;

			thickness = 2.0f;

			updown_comp copy;
			
			copy.setParameter<0>(getParameter(0));
			copy.setParameter<1>(getParameter(1));
			copy.setParameter<2>(getParameter(2));
			copy.setParameter<3>(getParameter(3));
			copy.setParameter<4>(getParameter(4));
			copy.setParameter<7>(getParameter(7));

			copy.calculateGraph(block(x));

			FloatSanitizers::sanitizeArray(x.begin(), x.size());

			p.startNewSubPath(0.0, 0.0);
			p.startNewSubPath(1.0, 1.0);

			p.startNewSubPath(0, 1.0f - jlimit(0.0f, 1.0f, x[0]));

			for (int i = 1; i < 100; i++)
			{
				auto v = jlimit(0.0f, 1.0f, x[i]);
				p.lineTo(i, 1.0f - v);
			}
				

			gridPath.clear();

			gridPath.startNewSubPath(0.25, 0.0); gridPath.lineTo(0.25, 1.0);
			gridPath.startNewSubPath(0.5, 0.0); gridPath.lineTo(0.5, 1.0);
			gridPath.startNewSubPath(0.75, 0.0); gridPath.lineTo(0.75, 1.0);

			gridPath.startNewSubPath(0.0, 0.25); gridPath.lineTo(1.0, 0.25);
			gridPath.startNewSubPath(0.0, 0.5); gridPath.lineTo(1.0, 0.5);
			gridPath.startNewSubPath(0.0, 0.75); gridPath.lineTo(1.0, 0.75);

			original.clear();
			original.startNewSubPath(0.0, 1.0f);
			original.lineTo(1.0, 0.0);

		}
	};

	updown_editor(PooledUIUpdater* u, data::pimpl::dynamic_base* bPtr) :
		ScriptnodeExtraComponent<data::pimpl::dynamic_base>(bPtr, u),
		graph(u),
	    editor(u)
	{
		addAndMakeVisible(graph);
		addAndMakeVisible(editor);
		
		setSize(330 + 28, 330);
	};

	float gainRed = 0.0f;

	void timerCallback() override
	{
		if(auto obj = getObject())
		{
			auto ptr = obj->getDisplayBuffer(0);

			auto thisPeak = jlimit(0.0f, 1.0f, ptr->getReadBuffer().getMagnitude(0, 0, roundToInt(44100.0 * 0.03)));

			if(hmath::abs(thisPeak) > hmath::abs(gainRed))
				gainRed = thisPeak;
			else
				gainRed *= 0.97f;

			repaint();
		}
	};

	static Component* createExtraComponent(void* ptr, PooledUIUpdater* u)
	{
		auto bPtr = reinterpret_cast<data::pimpl::dynamic_base*>(ptr);
		return new updown_editor(u, bPtr);
	}

	void paint(Graphics& g) override
	{
		auto b = graph.getBoundsInParent().removeFromRight(18).translated(20, 0).toFloat();

		g.setColour(Colours::black);
		g.fillRect(b);
		g.setColour(Colours::white);
		g.fillRect(b.removeFromTop(b.getHeight() * gainRed));

	}

	void resized()
	{
		auto b = getLocalBounds();
		editor.setBounds(b.removeFromBottom(28));
		graph.setBounds(b.withSizeKeepingCentre(b.getHeight(), b.getHeight()).reduced(10));
	}

	viz graph;
	ModulationSourceBaseComponent editor;
};

Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerPolyModNode<dp<gate>, dp<wrap::illegal_poly<gate>>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<comp>, dp<wrap::illegal_poly<comp>>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<limiter>, dp<wrap::illegal_poly<limiter>>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<updown_comp>, dp<wrap::illegal_poly<updown_comp>>, updown_editor>();
	registerPolyModNode<dp<envelope_follower<1>>, dp<envelope_follower<NUM_POLYPHONIC_VOICES>>, data::ui::displaybuffer_editor >();
}

}

namespace fx
{
	


Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	registerPolyNode<reverb, wrap::illegal_poly<reverb>>();
	registerPolyNode<sampleandhold<1>, sampleandhold<NUM_POLYPHONIC_VOICES>>();
	registerPolyNode<bitcrush<1>, bitcrush<NUM_POLYPHONIC_VOICES>>();
	registerPolyNode<wrap::fix<2, haas<1>>, wrap::fix<2, haas<NUM_POLYPHONIC_VOICES>>>();
	registerPolyNode<pitch_shift<1>, pitch_shift<NUM_POLYPHONIC_VOICES>>();
	registerPolyNode<phase_delay<1>, phase_delay<NUM_POLYPHONIC_VOICES>>();
}

}

namespace math
{



struct NeuralComp : public ScriptnodeExtraComponent<NodeBase>
{
    NeuralComp(NodeBase* n, PooledUIUpdater* updater) :
        ScriptnodeExtraComponent<NodeBase>(n, updater),
        networkSelector("", PropertyIds::Model),
        hpfSelector("Off", PropertyIds::HpfFreq)
    {
#if HISE_INCLUDE_RT_NEURAL
        auto& holder = n->getScriptProcessor()->getMainController_()->getNeuralNetworks();
        networkSelector.initModes(holder.getIdList(), n);
        addAndMakeVisible(networkSelector);

        hpfSelector.initModes({ "Off", "1 Hz", "5 Hz" }, n);
        addAndMakeVisible(hpfSelector);
#endif

        setSize(128, 64);
    };

    ComboBoxWithModeProperty networkSelector;
    ComboBoxWithModeProperty hpfSelector;

    void resized() override
    {
#if HISE_INCLUDE_RT_NEURAL
        auto area = getLocalBounds().reduced(0, 4);
        const int rowHeight = 24;

        networkSelector.setBounds(area.removeFromTop(rowHeight));
        area.removeFromTop(4);
        hpfSelector.setBounds(area.removeFromTop(rowHeight));
#else
        networkSelector.setBounds(getLocalBounds());
#endif
    }

    void timerCallback() override
    {

    }

    void paint(Graphics& g) override
    {
#if !HISE_INCLUDE_RT_NEURAL
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("This node is disabled.", getLocalBounds().toFloat(), Justification::centredTop);
		g.drawText("Recompile HISE with HISE_INCLUDE_RT_NEURAL.", getLocalBounds().toFloat(), Justification::centredBottom);
#endif
    }
};

template <int NV> struct NeuralNode: public NodeBase
{
    SN_NODE_ID("neural");
    
    NeuralNode(DspNetwork* root, const ValueTree& data):
      NodeBase(root, data, 0),
      networkId(PropertyIds::Model, ""),
      hpfFrequency(PropertyIds::HpfFreq, "Off")
    {
        cppgen::CustomNodeProperties::setPropertyForObject(*this, PropertyIds::IsFixRuntimeTarget);
        
        networkId.initialise(this);
        networkId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(NeuralNode::updateModel), true);

        hpfFrequency.initialise(this);
        hpfFrequency.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(NeuralNode::updateHpf), true);
    }
    
    AttributedString getDescription() const override
    {
        return AttributedString(obj.getWrappedObject().getDescription());
    }

    NodeComponent* createComponent() override
    {
        auto n = new DefaultParameterNodeComponent(this);
        n->setExtraComponent(new NeuralComp(this, getRootNetwork()->getMainController()->getGlobalUIUpdater()));
        return n;
    }

    void reset() final
    {
        obj.reset();
    }

    void processFrame(FrameType& data) override
    {
        obj.processFrame(data);
    }
    
    void process(ProcessDataDyn& data) override
    {
        obj.process(data);
    }
    
    static NodeBase* createNode(DspNetwork* n, ValueTree v)
    {
        return new NeuralNode(n, v);
    }

    void prepare(PrepareSpecs ps) override
    {
        NodeBase::prepare(ps);

		

#if USE_BACKEND && HISE_INCLUDE_RT_NEURAL
		obj.getWrappedObject().warmup = getRootNetwork()->getMainController()->getExtraDefinitionsValue("HISE_NEURAL_NETWORK_WARMUP_TIME", 0);
#endif
        obj.prepare(ps);
    }
    
    Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
    {
        return getBoundsToDisplay(Rectangle<int>(topLeft, topLeft.translated(128, 100)));
    }
    
    void updateModel(Identifier, var value)
    {
#if HISE_INCLUDE_RT_NEURAL
        auto v = value.toString();

        if(v.isNotEmpty())
        {
            auto newId = Identifier(value.toString());


            auto nn = getScriptProcessor()->getMainController_()->getNeuralNetworks().getOrCreate(newId);

            // make sure it matches when connecting
            auto& neuralObj = obj.getWrappedObject();
            neuralObj.getIndex().currentHash = nn->getRuntimeHash();
            neuralObj.connectToRuntimeTarget(true, nn->createConnection());

        }
        else
        {
            auto& neuralObj = obj.getWrappedObject();

            if(auto nn = neuralObj.getCurrentNetwork())
            {
                neuralObj.connectToRuntimeTarget(false, nn->createConnection());
            }
        }
#endif
    }

    void updateHpf(Identifier, var value)
    {
#if HISE_INCLUDE_RT_NEURAL
        auto text = value.toString().trim();
        auto lower = text.toLowerCase();

        auto& neuralObj = obj.getWrappedObject();
        auto freq = HpfFrequency::Off;

        if(lower == "1 hz" || lower == "1hz" || lower == "1")
            freq = HpfFrequency::Hz1;
        else if(lower == "5 hz" || lower == "5hz" || lower == "5")
            freq = HpfFrequency::Hz5;

        neuralObj.setHpfFrequency(freq);
#else
        ignoreUnused(value);
#endif
    }

    void setBypassed(bool shouldBeBypassed) override
    {
        NodeBase::setBypassed(shouldBeBypassed);
        obj.setBypassed(shouldBeBypassed);
    }

    using NeuralType = neural<NV, runtime_target::indexers::dynamic>;
    using BypassWrapper = bypass::simple<NeuralType>;
    
    BypassWrapper obj;
    
    NodePropertyT<String> networkId;
    NodePropertyT<String> hpfFrequency;
};


    

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
#define REGISTER_POLY_MATH_NODE(x) registerPolyNode<x<1>, x<NUM_POLYPHONIC_VOICES>>();
#define REGISTER_MONO_MATH_NODE(x) registerNode<x<1>>();
    
    REGISTER_POLY_MATH_NODE(add);
    REGISTER_MONO_MATH_NODE(fill1);
    REGISTER_POLY_MATH_NODE(tanh);
	REGISTER_POLY_MATH_NODE(fmod);
    REGISTER_POLY_MATH_NODE(mul );
    REGISTER_POLY_MATH_NODE(sub );
    REGISTER_POLY_MATH_NODE(div );
    REGISTER_POLY_MATH_NODE(clip);
    
    REGISTER_MONO_MATH_NODE(clear);
    REGISTER_MONO_MATH_NODE(sin);
    REGISTER_MONO_MATH_NODE(pi);
    REGISTER_MONO_MATH_NODE(sig2mod);
    REGISTER_MONO_MATH_NODE(mod2sig);
    REGISTER_MONO_MATH_NODE(rect);
    REGISTER_MONO_MATH_NODE(mod_inv);
    REGISTER_MONO_MATH_NODE(inv);
    REGISTER_MONO_MATH_NODE(abs);
	REGISTER_POLY_MATH_NODE(square);
	REGISTER_POLY_MATH_NODE(sqrt);
	REGISTER_POLY_MATH_NODE(pow);
    REGISTER_POLY_MATH_NODE(intensity);
    
    registerNode<map>();
    
    registerNode<wrap::data<table, data::dynamic::table>, data::ui::table_editor_without_mod>();

	

    registerNode<wrap::data<pack, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();

	registerPolyNodeRaw<NeuralNode<1>, NeuralNode<NUM_POLYPHONIC_VOICES>>();
	
#undef REGISTER_POLY_MATH_NODE
#undef REGISTER_MONO_MATH_NODE

#if HISE_INCLUDE_SNEX
	registerPolyNode<OpNode<dynamic_expression, 1>, OpNode<dynamic_expression, NUM_POLYPHONIC_VOICES>, dynamic_expression::editor>();
#endif

	sortEntries();
}
}




namespace control
{
	using dynamic_cable_table = wrap::data<control::cable_table<parameter::dynamic_base_holder>, data::dynamic::table>;
	using dynamic_cable_pack = wrap::data<control::cable_pack<parameter::dynamic_base_holder>, data::dynamic::sliderpack>;
    using dynamic_pack_resizer = wrap::data<control::pack_resizer, data::dynamic::sliderpack>;

	template <int NV> using dynamic_smoother_parameter = control::smoothed_parameter<NV, smoothers::dynamic<NV>>;

	template <int NV> using dynamic_smoother_parameter_unscaled = control::smoothed_parameter_unscaled<NV, smoothers::dynamic<NV>>;

 	Factory::Factory(DspNetwork* network) :
		NodeFactory(network)
	{

		registerPolyNoProcessNode<control::bipolar<1, parameter::dynamic_base_holder>, control::bipolar<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, bipolar_editor_wrapped>();

		registerPolyNoProcessNode<control::blend<1, parameter::dynamic_base_holder>, control::blend<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, blend_editor_wrapped>();

		registerPolyNoProcessNode<control::intensity<1, parameter::dynamic_base_holder>, control::intensity<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, intensity_editor_wrapped>();

		

		registerPolyNoProcessNode<control::pma<1, parameter::dynamic_base_holder>, control::pma<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, extra_drag_wrapper<pma_editor, Justification::bottom>>();
		registerPolyNoProcessNode<control::pma_unscaled<1, parameter::dynamic_base_holder>, control::pma_unscaled<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, extra_drag_wrapper<pma_editor, Justification::bottom>>();

		registerPolyNoProcessNode<control::minmax<1, parameter::dynamic_base_holder>, control::minmax<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, minmax_editor_wrapped>();

		registerPolyNoProcessNode<control::logic_op<1, parameter::dynamic_base_holder>, control::logic_op<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, logic_op_editor_wrapped>();

		registerPolyNoProcessNode<control::bang<1, parameter::dynamic_base_holder>, control::bang<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();

		registerPolyNoProcessNode<control::compare<1, parameter::dynamic_base_holder>, control::compare<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, compare_editor_wrapped>();

		registerPolyNoProcessNode<control::change<1, parameter::dynamic_base_holder>, control::change<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();
        
        registerPolyNoProcessNode<control::delay_cable<1, parameter::dynamic_base_holder>, control::delay_cable<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();
        

        registerNoProcessNode<dynamic_pack_resizer, data::ui::sliderpack_editor>();
        
		registerPolyNoProcessNode<branch_cable<1, parameter::dynamic_list>, branch_cable<NUM_POLYPHONIC_VOICES, parameter::dynamic_list>, branch_editor_wrapped>()

        ;
        
        registerNoProcessNode<wrap::data<pack2_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        registerNoProcessNode<wrap::data<pack3_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        registerNoProcessNode<wrap::data<pack4_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        registerNoProcessNode<wrap::data<pack5_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        registerNoProcessNode<wrap::data<pack6_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        registerNoProcessNode<wrap::data<pack7_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        registerNoProcessNode<wrap::data<pack8_writer, data::dynamic::sliderpack>, data::ui::sliderpack_editor_without_mod>();
        
		registerNoProcessNode<control::sliderbank_editor::NodeType, control::sliderbank_editor, false>();
		registerNoProcessNode<dynamic_cable_pack, data::ui::sliderpack_editor>();
		registerNoProcessNode<dynamic_cable_table, data::ui::table_editor>();
		
		registerNoProcessNode<control::normaliser<parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();
		registerNoProcessNode<control::unscaler<parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();
		registerNoProcessNode<control::locked_mod<parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();
		registerNoProcessNode<control::locked_mod_unscaled<parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();

		registerNoProcessNode<control::random<parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();

		registerPolyNoProcessNode<control::input_toggle<1, parameter::dynamic_base_holder>, control::input_toggle<NUM_POLYPHONIC_VOICES, parameter::dynamic_base_holder>, input_toggle_editor>();

        registerNoProcessNode<conversion_logic::DynamicNodeType, conversion_logic::DynamicEditor>();

		registerNoProcessNode<control::clone_forward<parameter::clone_holder>, ModulationSourceBaseComponent>();

		registerNoProcessNode<duplilogic::dynamic::NodeType, duplilogic::dynamic::editor>();
		registerNoProcessNode<dynamic_dupli_pack, data::ui::sliderpack_editor>();
		registerNoProcessNode<faders::NodeType, faders::Editor>();
		registerNoProcessNode<control::xy_editor::NodeType, control::xy_editor>();
		registerNoProcessNode<control::resetter_editor::NodeType, control::resetter_editor>();
		registerPolyModNode<dynamic_smoother_parameter<1>, dynamic_smoother_parameter<NUM_POLYPHONIC_VOICES>, smoothers::dynamic_base::editor>();

		registerPolyModNode<dynamic_smoother_parameter_unscaled<1>, dynamic_smoother_parameter_unscaled<NUM_POLYPHONIC_VOICES>, smoothers::dynamic_base::editor>();

#if HISE_INCLUDE_SNEX
		registerNoProcessNode<dynamic_expression::ControlNodeType, dynamic_expression::editor>();
		
#endif

		registerModNode<midi_logic::dynamic::NodeType, midi_logic::dynamic::editor>();
		registerPolyModNode<control::timer<1, snex_timer>, timer<NUM_POLYPHONIC_VOICES, snex_timer>, snex_timer::editor>();

		registerNoProcessNode<control::midi_cc<parameter::dynamic_base_holder>, midi_cc_editor>();

		registerNoProcessNode<control::voice_bang<parameter::dynamic_base_holder>, ModulationSourceBaseComponent>();

		registerNoProcessNode<file_analysers::dynamic::NodeType, file_analysers::dynamic::editor, false>(); //>();

		registerPolyModNode<tempo_sync<1>, tempo_sync<NUM_POLYPHONIC_VOICES>, TempoDisplay>();

		registerPolyModNode<transport<1>, transport<NUM_POLYPHONIC_VOICES>, TransportDisplay>();

		registerPolyModNode<ppq<1>, ppq<NUM_POLYPHONIC_VOICES>, TransportDisplay>();
	}
}

namespace envelope
{
namespace dynamic
{
	using envelope_base = pimpl::envelope_base<parameter::dynamic_list>;
	using simple_ar = envelope::pimpl::simple_ar_base;

	struct envelope_display_base : public ScriptnodeExtraComponent<envelope_base>
	{
		using Dragger = parameter::ui::dynamic_list_editor::DragComponent;

		virtual ~envelope_display_base() {};

		envelope_display_base(envelope_base* d, PooledUIUpdater* u) :
			ScriptnodeExtraComponent<envelope_base>(d, u),
			modValue(&d->p, 0), //
			activeValue(&d->p, 1)
		{
			addAndMakeVisible(modValue);
			addAndMakeVisible(activeValue);

			modValue.textFunction = BIND_MEMBER_FUNCTION_1(envelope_display_base::getAxis);
			activeValue.textFunction = BIND_MEMBER_FUNCTION_1(envelope_display_base::getAxis);
		};

		String getAxis(int index)
		{
			auto id = getObject()->getId();
			return cppgen::CustomNodeProperties::getModOutputs(id)[index];
		}

		Dragger modValue, activeValue;
	};

	struct ahdsr_display : public envelope_display_base
	{
		struct DisplayType : public data::ui::pimpl::editorT<data::dynamic::displaybuffer, SimpleRingBuffer, AhdsrGraph, false>
		{
			static data::dynamic::displaybuffer* getDynamicRingBuffer(envelope_base* b)
			{
				if (auto mn = dynamic_cast<mothernode*>(b))
				{
					auto dataObject = mn->getDataProvider()->getDataObject();
					auto typed = dynamic_cast<data::dynamic::displaybuffer*>(dataObject);
					return typed;
				}

				return nullptr;
			}

			DisplayType(envelope_base* e, PooledUIUpdater* u) :
				editorT(u, getDynamicRingBuffer(e))
			{
				if(dragger != nullptr)
					dragger->setVisible(false);

				resized();
			};

			void paintOverChildren(Graphics& g) override
			{
				auto idx = getObject()->getIndex();

				if (idx != -1)
				{
					auto b = editor->getBounds().toFloat();

					String x;
					x << "#";
					x << (idx + 1);

					g.setColour(Colours::white.withAlpha(0.9f));
					g.setFont(GLOBAL_BOLD_FONT());
					g.fillPath(dashPath);
					g.drawText(x, b.reduced(5.0f), Justification::topRight);
				}
			}

			void resized() override
			{
				auto b = getLocalBounds();

				externalButton.setBounds(b.removeFromRight(28).removeFromBottom(28).reduced(3));
				editor->setBounds(b);
				refreshDashPath();
			}
		};

		

		ahdsr_display(envelope_base* b, PooledUIUpdater* updater):
			envelope_display_base(b, updater),
			display(b, updater)
		{
			addAndMakeVisible(display);
			setSize(200, 100);
		}

		void paint(Graphics& g) override
		{
			
		}

		void timerCallback() override
		{

		}

		static Component* createExtraComponent(void* o, PooledUIUpdater* updater)
		{
			auto t = static_cast<mothernode*>(o);
			auto typed = dynamic_cast<envelope_base*>(t);

			return new ahdsr_display(typed, updater);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromBottom(UIValues::NodeMargin);

			auto r = b.removeFromRight(100);
			b.removeFromRight(UIValues::NodeMargin);
			display.setBounds(b);
			modValue.setBounds(r.removeFromTop(32));
			activeValue.setBounds(r.removeFromBottom(32));
		}

		DisplayType display;
	};

	struct flex_ahdsr_display : public envelope_display_base
	{
		struct internal_display: public data::ui::pimpl::editorT<data::dynamic::displaybuffer, SimpleRingBuffer, flex_ahdsr_base::FlexAhdsrGraph, false>
		{
			static data::dynamic::displaybuffer* getDynamicRingBuffer(envelope_base* b)
			{
				if (auto mn = dynamic_cast<mothernode*>(b))
				{
					auto dataObject = mn->getDataProvider()->getDataObject();
					auto typed = dynamic_cast<data::dynamic::displaybuffer*>(dataObject);
					return typed;
				}

				return nullptr;
			}

			internal_display(envelope_base* o, PooledUIUpdater* u):
			  editorT(u, getDynamicRingBuffer(o))
			{
				if(dragger != nullptr)
					dragger->setVisible(false);

				resized();
			}

			void resized() override
			{
				auto b = getLocalBounds();

				externalButton.setBounds(b.removeFromRight(28).removeFromBottom(28).reduced(3));
				editor->setBounds(b);
				refreshDashPath();
			}
		};

		flex_ahdsr_display(envelope_base* o, PooledUIUpdater* u):
		  envelope_display_base(o, u),
		  display(o, u)
		{
			addAndMakeVisible(display);
			setSize(200, 180);
		}

		void timerCallback() override
		{
			
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromBottom(UIValues::NodeMargin);

			auto r = b.removeFromRight(100);
			b.removeFromRight(UIValues::NodeMargin);
			display.setBounds(b);
			modValue.setBounds(r.removeFromTop(32));
			activeValue.setBounds(r.removeFromBottom(32));
		}

		static Component* createExtraComponent(void* o, PooledUIUpdater* updater)
		{
			auto t = static_cast<mothernode*>(o);
			auto typed = dynamic_cast<envelope_base*>(t);
			return new flex_ahdsr_display(typed, updater);
		}

		internal_display display;
	};

	struct env_display : envelope_display_base
	{
		struct visualiser : public simple_visualiser
		{
			visualiser(PooledUIUpdater* u, env_display& parent) :
				simple_visualiser(nullptr, u),
				p(parent)
			{};

			void rebuildPath(Path& path) override
			{
				auto env = dynamic_cast<pimpl::simple_ar_base*>(p.getObject());

				auto modValue = p.getObject()->getParameter().getParameter<0>().getDisplayValue();

				

				if (auto rb = dynamic_cast<SimpleRingBuffer*>(env->externalData.obj))
				{
					auto isOn = rb->getUpdater().getLastDisplayValue();

					path.startNewSubPath(0.0f, 1.0f);

					const auto& b = rb->getReadBuffer();

					int minI = INT_MAX;
					int maxI = 0;
					

					auto lv = 0.0f;

					for (int i = 0; i < b.getNumSamples(); i++)
					{
						auto v = b.getSample(0, i);
						path.lineTo(i, 1.0f - v);

						if (std::abs(v - modValue) < 0.01)
						{
							minI = jmin(minI, i);
							maxI = jmax(maxI, i);
						}

						// put the ball at the sustain position
						if (modValue > 0.999 && lv < v)
						{
							minI = i;
							maxI = i;
						}

						lv = v;
					}

					
					if (modValue > 0.0 && maxI != 0)
					{
						

						auto iToUse = isOn > 0.5 ? minI : maxI;

						path.startNewSubPath(iToUse, 1.0f - modValue);
						path.lineTo(iToUse, 1.0f);
					}
				}
			}

			env_display& p;
		} visualiser;

		env_display(envelope_base* d, PooledUIUpdater* u):
			envelope_display_base(d, u),
			visualiser(u, *this)
		{
			addAndMakeVisible(visualiser);
			setSize(300, 32 * 2 + 2 * UIValues::NodeMargin);
		}

		void timerCallback() override
		{

		}

		void resized() override
		{
			auto b = getLocalBounds();
			
			b.removeFromBottom(UIValues::NodeMargin);

			auto r = b.removeFromRight(100);
			r.removeFromLeft(UIValues::NodeMargin);

			visualiser.setBounds(b);;

			modValue.setBounds(r.removeFromTop(32));
			r.removeFromTop(UIValues::NodeMargin);
			activeValue.setBounds(r.removeFromTop(32));
		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* u)
		{
			auto typed = static_cast<mothernode*>(obj);
			auto typedBase = dynamic_cast<envelope_base*>(typed);
			return new env_display(typedBase, u);
		}
	};

}
}

namespace core
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;


#if !HISE_INCLUDE_FAUST_JIT
struct faust : public mothernode
{
	SNEX_NODE(faust);

	constexpr bool isPolyphonic() const { return false; }

	SN_EMPTY_CREATE_PARAM;
	
	SN_EMPTY_MOD;
	SN_EMPTY_PROCESS;
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_RESET;
	SN_EMPTY_HANDLE_EVENT;
    
    template <int P> void setParameter(double){};
    
	void prepare(PrepareSpecs )
	{
		Error::throwError(Error::IllegalFaustNode);
	}
};
#endif


Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	

	registerNode<fix_delay>();
	registerNode<fm>();
	
	registerModNode<wrap::data<core::table,       data::dynamic::table>,     data::ui::table_editor>();

	using mono_file_player = wrap::data<core::file_player<1>, data::dynamic::audiofile>;
	using poly_file_player = wrap::data<core::file_player<NUM_POLYPHONIC_VOICES>, data::dynamic::audiofile>;

	registerPolyNode<mono_file_player, poly_file_player, data::ui::xyz_audio_editor>();
    
    using mono_stretch_player = wrap::data<core::stretch_player<1>, data::dynamic::audiofile>;
    using poly_stretch_player = wrap::data<core::stretch_player<NUM_POLYPHONIC_VOICES>, data::dynamic::audiofile>;

    registerPolyNode<mono_stretch_player, mono_stretch_player, data::ui::xyz_audio_editor>();
    
	registerNode   <wrap::data<core::recorder,    data::dynamic::audiofile>, data::ui::audiofile_editor>();

	registerPolyNode<gain<1>, gain<NUM_POLYPHONIC_VOICES>>();

	registerPolyNode<smoother<1>, smoother<NUM_POLYPHONIC_VOICES>>();

	

#if HISE_INCLUDE_SNEX
	registerPolyNode<snex_osc<1, SnexOscillator>, snex_osc<NUM_POLYPHONIC_VOICES, SnexOscillator>, NewSnexOscillatorDisplay>();
	registerModNode<core::snex_node, core::snex_node::editor>();
	registerNode<waveshapers::dynamic::NodeType, waveshapers::dynamic::editor>();
#endif

#if HISE_INCLUDE_FAUST_JIT
	registerPolyNodeRaw<faust::faust_jit_node<1>, faust::faust_jit_node<NUM_POLYPHONIC_VOICES>>();
#else
	registerNode<faust>();
#endif // HISE_INCLUDE_FAUST_JIT

	using fi = runtime_target::indexers::fix_hash<1>;
	using pci = modulation::config::PitchIndexer;
	using mc = modulation::config::dynamic_with_display;

	struct ec: public modulation::config::extra_config_with_display
	{
		void prepare(PrepareSpecs ps) override
		{
			if(parentNode != nullptr)
				ScriptnodeExceptionHandler::validateMidiProcessingContext(parentNode);
		}

		void checkIndex(const Identifier& id, const var& newValue)
		{
			if(parentNode != nullptr)
			{
				auto root = parentNode->getRootNetwork();

				root->getExceptionHandler().removeError(parentNode, Error::ErrorCode::RootIdMismatch);
				auto mi = (int)newValue;
				auto nn = root->getParameterProperties();
				auto ok = nn.isUsed(mi);

				if(!ok)
				{
					root->getExceptionHandler().addCustomError(parentNode, Error::ErrorCode::RootIdMismatch, "No parameter assigned to modulation slot #" + String(mi+1));
				}
			}
		}

		void initialise(ObjectWithValueTree* n) override
		{
			parentNode = dynamic_cast<NodeBase*>(n);

			if(n != nullptr)
			{
				auto ptree = parentNode->getParameterTree().getChildWithProperty(PropertyIds::ID, "Index");

				indexListener.setCallback(ptree, 
									      { PropertyIds::Value }, 
										  valuetree::AsyncMode::Asynchronously, 
										  BIND_MEMBER_FUNCTION_2(ec::checkIndex));
			}
		}


		valuetree::PropertyListener indexListener;
		WeakReference<NodeBase> parentNode;
	};

	using pc = modulation::config::pitch_config_with_display;

	using ei = modulation::config::ExtraIndexer;
	


	registerPolyModNode<dp<global_mod<1, fi, mc>>, dp<global_mod<NUM_POLYPHONIC_VOICES, fi, mc>>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<pitch_mod<1, pci, pc>>, dp<pitch_mod<NUM_POLYPHONIC_VOICES, pci, pc>>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<extra_mod<1, ei, ec>>, dp<extra_mod<NUM_POLYPHONIC_VOICES, ei, ec>>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<matrix_mod<1>>, dp<matrix_mod<NUM_POLYPHONIC_VOICES>>, data::ui::displaybuffer_editor>();

	registerModNode<dp<peak>, data::ui::displaybuffer_editor>();
	registerModNode<dp<peak_unscaled>, data::ui::displaybuffer_editor>();
	registerPolyModNode<dp<ramp<1, true>>, dp<ramp<NUM_POLYPHONIC_VOICES, true>>, data::ui::displaybuffer_editor>();

	registerPolyModNode<dp<clock_ramp<1, true>>, dp<clock_ramp<NUM_POLYPHONIC_VOICES, true>>, data::ui::displaybuffer_editor>();

	registerNode<core::mono2stereo>();

	using osc_display_ = data::ui::pimpl::editorT<data::dynamic::displaybuffer,
		hise::SimpleRingBuffer,
		OscillatorDisplayProvider::osc_display,
		false>;

	registerPolyNode<dp<core::oscillator<1>>, dp<core::oscillator<NUM_POLYPHONIC_VOICES>>, osc_display_>();
	registerPolyNode<core::phasor<1>, core::phasor<NUM_POLYPHONIC_VOICES>>();
	registerPolyNode<core::phasor_fm<1>, core::phasor_fm<NUM_POLYPHONIC_VOICES>>();
	registerNode<wrap::data<granulator, data::dynamic::audiofile>, data::ui::xyz_audio_editor>();
}
}

namespace envelope
{
template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;



Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
	struct parameter_handler: public flex_ahdsr_base::DragHandlerBase
	{
		void initialise(ObjectWithValueTree* n)
		{
			parentNode = dynamic_cast<NodeBase*>(n);
		}

		bool handleAdditionalDrag(int parameterIndex, double value) override
		{
			if(auto p = parentNode->getParameterFromIndex(parameterIndex))
			{
				p->setValueSync(value);
				return true;
			}

			return false;
		}

		WeakReference<NodeBase> parentNode;
	};

	registerPolyModNode<dp<simple_ar<1, parameter::dynamic_list>>, 
						dp<simple_ar<NUM_POLYPHONIC_VOICES, parameter::dynamic_list>>, 
						dynamic::env_display, 
						false>();

	registerPolyModNode<dp<ahdsr<1, parameter::dynamic_list>>, 
						dp<ahdsr<NUM_POLYPHONIC_VOICES, parameter::dynamic_list>>, 
						dynamic::ahdsr_display, 
						false>();

	registerPolyModNode<dp<flex_ahdsr<1, parameter::dynamic_list, parameter_handler>>,
						dp<flex_ahdsr<NUM_POLYPHONIC_VOICES, parameter::dynamic_list, parameter_handler>>,
						dynamic::flex_ahdsr_display,
						false>();

	registerNode<voice_manager, voice_manager_base::editor>();

	using gi = runtime_target::indexers::fix_hash<1>;
	using ei = modulation::config::ExtraIndexer;
	using ph = parameter::dynamic_base_holder;
	registerPolyModNode<global_mod_gate<1, gi>, global_mod_gate<NUM_POLYPHONIC_VOICES, gi>, ModulationSourceBaseComponent>();
	registerPolyModNode<extra_mod_gate<1, ei>, extra_mod_gate<NUM_POLYPHONIC_VOICES, ei>, ModulationSourceBaseComponent>();

	registerPolyNode<silent_killer<1>, silent_killer<NUM_POLYPHONIC_VOICES>, voice_manager_base::editor>();
}
}

namespace jdsp
{

template <typename T> using dp = wrap::data<T, data::dynamic::displaybuffer>;

Factory::Factory(DspNetwork* network):
	NodeFactory(network)
{
	registerPolyNode<jchorus, wrap::illegal_poly<jchorus>>();
	registerNode<wrap::data<jlinkwitzriley, data::dynamic::filter>, data::ui::filter_editor>();

	registerPolyNode<jdelay<1>, jdelay<NUM_POLYPHONIC_VOICES>>();
	registerPolyNode<jdelay_thiran<1>, jdelay_thiran<NUM_POLYPHONIC_VOICES>>();
	registerPolyNode<jdelay_cubic<1>, jdelay_cubic<NUM_POLYPHONIC_VOICES>>();
	
	registerPolyModNode<dp<jcompressor>, wrap::illegal_poly<dp<jcompressor>>, data::ui::displaybuffer_editor>();

	registerPolyNode<jpanner<1>, jpanner<NUM_POLYPHONIC_VOICES>>();
}
}

namespace generator
{
Factory::Factory(DspNetwork* network) :
	NodeFactory(network)
{
}
}

namespace filters
{
template <typename FilterType> using df = wrap::data<FilterType, data::dynamic::filter>;

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	using namespace data::ui;

	registerPolyNode<df<one_pole<1>>,		df<one_pole<NUM_POLYPHONIC_VOICES>>,	filter_editor>();
	registerPolyNode<df<svf<1>>,			df<svf<NUM_POLYPHONIC_VOICES>>,			filter_editor>();
	registerPolyNode<df<svf_eq<1>>,			df<svf_eq<NUM_POLYPHONIC_VOICES>>,		filter_editor>();
	registerPolyNode<df<biquad<1>>,			df<biquad<NUM_POLYPHONIC_VOICES>>,		filter_editor>();
	registerPolyNode<df<ladder<1>>,			df<ladder<NUM_POLYPHONIC_VOICES>>,		filter_editor>();
	registerPolyNode<df<ring_mod<1>>,		df<ring_mod<NUM_POLYPHONIC_VOICES>>,	filter_editor>();
	registerPolyNode<df<moog<1>>,			df<moog<NUM_POLYPHONIC_VOICES>>,		filter_editor>();
	registerPolyNode<df<allpass<1>>,		df<allpass<NUM_POLYPHONIC_VOICES>>,		filter_editor>();
	registerPolyNode<df<linkwitzriley<1>>,	df<linkwitzriley<NUM_POLYPHONIC_VOICES>>, filter_editor>();

	registerNode<wrap::data<convolution, data::dynamic::audiofile>, data::ui::audiofile_editor>();
	//registerPolyNode<fir, fir_poly>();
}
}

#if USE_BACKEND
namespace dll
{




BackendHostFactory::BackendHostFactory(DspNetwork* n, ProjectDll::Ptr dll) :
	NodeFactory(n),
	dllFactory(dll)
{
	auto mc = n->getScriptProcessor()->getMainController_();
	auto networks = BackendDllManager::getNetworkFiles(mc, false);
	auto numNetworks = networks.size();

	int numNodesInDll = dllFactory.getNumNodes();

	int thirdPartyOffset = 0;

	Array<Identifier> idsFromJSON;

	if(numNodesInDll == 0)
	{
		std::pair<Array<Identifier>, int> rv = BackendDllManager::initialiseThirdPartyProperties(mc);

		thirdPartyOffset = rv.second;
		idsFromJSON = rv.first;
	}
	else
	{
		for (int i = 0; i < numNodesInDll; i++)
		{
			if (dllFactory.isThirdPartyNode(i))
				thirdPartyOffset = i + 1;
		}
	}
	
	// Cater in the scenario where we have third party effects...
	auto numNodesToCreate = jmax(numNetworks + thirdPartyOffset, numNodesInDll);

	for (int i = 0; i < numNodesToCreate; i++)
	{

#if HISE_CREATE_DSP_NETWORKS_FOR_HARDCODED_NODES
		// Create a network and toggle between frozen and interpreted state...
		auto createHardcodedNodeOnly = dllFactory.isThirdPartyNode(i);
#else
		// Don't create a DSP network but treat it like a compiled third party node
		auto createHardcodedNodeOnly = i < numNodesInDll || i < thirdPartyOffset;
#endif

		if (createHardcodedNodeOnly)
		{
			if(numNodesInDll == 0)
			{
				NodeFactory::Item item;

				item.id = idsFromJSON[i];
				item.cb = [](DspNetwork* p, ValueTree v)
				{
					return new UncompiledNode(p, v);
				};

				monoNodes.add(item);
			}
			else
			{
				NodeFactory::Item item;
				item.id = dllFactory.getId(i);

				dll::FactoryBase* f = &dllFactory;

				item.cb = [this, i, f](DspNetwork* p, ValueTree v)
				{
					auto isModNode = f->getWrapperType(i) == 1;

					NodeBase* n;

					if (isModNode)
					{
						auto mn = new InterpretedModNode(p, v);
						mn->initFromDll(f, i, true);
						n = mn;
					}
					else
					{
						auto in = new InterpretedNode(p, v);
						in->initFromDll(f, i, false);
						n = in;
					}

					return n;
				};

				monoNodes.add(item);
			}
		}
		else
		{
			NodeFactory::Item item;

			if(i < thirdPartyOffset)
			{
				jassert(isPositiveAndBelow(i, idsFromJSON.size()));
				item.id = idsFromJSON[i];
			}
			else
			{
				auto networkIndex = i - thirdPartyOffset;
				jassert(isPositiveAndBelow(networkIndex, networks.size()));
				item.id = networks[networkIndex].getFileNameWithoutExtension();
			}
			
			item.cb = [](DspNetwork* p, ValueTree v)
			{
				return new UncompiledNode(p, v);
			};

			monoNodes.add(item);
		}
	}
}
}
#endif

}

