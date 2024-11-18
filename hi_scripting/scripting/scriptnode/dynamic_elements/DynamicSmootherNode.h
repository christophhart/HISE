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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace control
{
	struct logic_op_editor : public ScriptnodeExtraComponent<pimpl::combined_parameter_base<multilogic::logic_op>>
	{
		using LogicBase = pimpl::combined_parameter_base<multilogic::logic_op>;

		logic_op_editor(LogicBase* b, PooledUIUpdater* u);

		void paint(Graphics& g) override;

		void timerCallback() override;

		void resized() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new logic_op_editor(dynamic_cast<LogicBase*>(typed), updater);
		}

		ModulationSourceBaseComponent dragger;

		control::multilogic::logic_op lastData;
	};

    struct compare_editor : public ScriptnodeExtraComponent<pimpl::combined_parameter_base<multilogic::compare>>
	{
		using CompareBase = pimpl::combined_parameter_base<multilogic::compare>;

		compare_editor(CompareBase* b, PooledUIUpdater* u);

        void paint (Graphics& g) override;

        void timerCallback () override;

        void resized () override;

        static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new compare_editor(dynamic_cast<CompareBase*>(typed), updater);
		}

		ModulationSourceBaseComponent dragger;

		control::multilogic::compare lastData;
	};

	struct blend_editor : public ScriptnodeExtraComponent<pimpl::combined_parameter_base<multilogic::blend>>
	{
		using LogicBase = pimpl::combined_parameter_base<multilogic::blend>;

		blend_editor(LogicBase* b, PooledUIUpdater* u):
		  ScriptnodeExtraComponent<LogicBase>(b, u),
		  dragger(u)
		{
			addAndMakeVisible(dragger);

			setSize(256, 50);
		};

		void paint(Graphics& g) override
		{
			auto alpha = (float)lastData.alpha * 2.0f - 1.0f;

			auto b = getLocalBounds().removeFromRight((getWidth() * 2) / 3).toFloat();

			auto area = b.reduced(JUCE_LIVE_CONSTANT(40), 15).toFloat();

			ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, area, true);

			

			area = area.reduced(4);

			auto w = (area.getWidth() - area.getHeight()) * 0.5f;

			auto tb = area.translated(0, 20);

			area = area.withSizeKeepingCentre(area.getHeight(), area.getHeight());

			area = area.translated(alpha * w, 0);

			g.setColour(getHeaderColour());

			g.fillEllipse(area);

			g.setFont(GLOBAL_BOLD_FONT());

			g.drawText(String(lastData.getValue(), 2), tb, Justification::centred);
		}

		void timerCallback() override
		{
			auto thisData = getObject()->getUIData();

			if (!(thisData == lastData))
			{
				lastData = thisData;
				repaint();
			}
		}

		void resized() override
		{
			auto b = getLocalBounds();


			dragger.setBounds(b.removeFromLeft(getWidth()/3).withSizeKeepingCentre(32, 32));


		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new blend_editor(dynamic_cast<LogicBase*>(typed), updater);
		}

		ModulationSourceBaseComponent dragger;

		control::multilogic::blend lastData;
	};

	struct intensity_editor : public ScriptnodeExtraComponent<pimpl::combined_parameter_base<multilogic::intensity>>
	{
		using IntensityBase = pimpl::combined_parameter_base<multilogic::intensity>;

		intensity_editor(IntensityBase* b, PooledUIUpdater* u);

		void paint(Graphics& g) override;

		void rebuildPaths();

		void timerCallback() override;

		void resized() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new intensity_editor(dynamic_cast<IntensityBase*>(typed), updater);
		}

		Rectangle<float> pathArea;

		Path fullPath, valuePath;

		multilogic::intensity lastData;

		ModulationSourceBaseComponent dragger;
	};

	struct minmax_editor : public ScriptnodeExtraComponent<pimpl::combined_parameter_base<multilogic::minmax>>
	{
		using MinMaxBase = pimpl::combined_parameter_base<multilogic::minmax>;

		minmax_editor(MinMaxBase* b, PooledUIUpdater* u);

		void paint(Graphics& g) override;

		void timerCallback() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new minmax_editor(dynamic_cast<MinMaxBase*>(typed), updater);
		}

		void setRange(InvertableParameterRange newRange);

		void rebuildPaths();

		void resized() override;

		multilogic::minmax lastData;

		Path fullPath, valuePath;

		ComboBox rangePresets;
		ModulationSourceBaseComponent dragger;
		Rectangle<float> pathArea;
		ScriptnodeComboBoxLookAndFeel slaf;
		RangePresets presets;
	};

	struct midi_cc_editor : public ScriptnodeExtraComponent<midi_cc<parameter::dynamic_base_holder>>
	{
		using ObjectType = midi_cc<parameter::dynamic_base_holder>;

		midi_cc_editor(ObjectType* obj, PooledUIUpdater* u) :
			ScriptnodeExtraComponent<ObjectType>(obj, u),
			dragger(u)
		{
			addAndMakeVisible(dragger);
			setSize(256, 32);
		};

		void checkValidContext()
		{
			if (contextChecked)
				return;

			if (auto nc = findParentComponentOfClass<NodeComponent>())
			{
				NodeBase* n = nc->node.get();
				
				try
				{
					ScriptnodeExceptionHandler::validateMidiProcessingContext(n);
					n->getRootNetwork()->getExceptionHandler().removeError(n);
				}
				catch (Error& e)
				{
					n->getRootNetwork()->getExceptionHandler().addError(n, e);
				}

				contextChecked = true;
			}
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();
			b.removeFromBottom((float)dragger.getHeight());

			g.setColour(Colours::white.withAlpha(alpha));
			g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);
			
			b = b.reduced(2.0f);
			b.setWidth(jmax<float>(b.getHeight(), lastValue.getModValue() * b.getWidth()));
			g.fillRoundedRectangle(b, b.getHeight() / 2.0f);
		}

		void resized() override
		{
			dragger.setBounds(getLocalBounds().removeFromBottom(24));
		}

		void timerCallback() override
		{
			checkValidContext();

			if (ObjectType* o = getObject())
			{
				auto lv = o->getParameter().getDisplayValue();

				if (lastValue.setModValueIfChanged(lv))
					alpha = 1.0f;
				else
					alpha = jmax(0.5f, alpha * 0.9f);

				repaint();
			}
		}

		static Component* createExtraComponent(void* o, PooledUIUpdater* u)
		{
			auto mn = static_cast<mothernode*>(o);
			auto typed = dynamic_cast<ObjectType*>(mn);

			return new midi_cc_editor(typed, u);
		}

		float alpha = 0.5f;
		ModValue lastValue;

		ModulationSourceBaseComponent dragger;
		bool contextChecked = false;
	};

	struct bipolar_editor : public ScriptnodeExtraComponent<pimpl::combined_parameter_base<multilogic::bipolar>>
	{
		using BipolarBase = pimpl::combined_parameter_base<multilogic::bipolar>;

		bipolar_editor(BipolarBase* b, PooledUIUpdater* u) :
			ScriptnodeExtraComponent<BipolarBase>(b, u),
			dragger(u)
		{
			setSize(256, 256);
			addAndMakeVisible(dragger);
		};

		void timerCallback() override
		{
			auto obj = getObject();

			if (obj == nullptr)
				return;

			auto thisData = getObject()->getUIData();

			if (!(thisData == lastData))
			{
				lastData = thisData;
				rebuild();
			}
		}

		void rebuild()
		{
			outlinePath.clear();
			
			valuePath.clear();
			outlinePath.startNewSubPath(0.0f, 0.0f);
			outlinePath.startNewSubPath(1.0f, 1.0f);

			valuePath.startNewSubPath(0.0f, 0.0f);
			valuePath.startNewSubPath(1.0f, 1.0f);

			auto copy = lastData;

			auto numPixels = pathArea.getWidth();

			bool outlineEmpty = true;
			bool valueEmpty = true;

			bool valueBiggerThanHalf = copy.value > 0.5;
			auto v = lastData.value;

			for (float i = 0.0; i < numPixels; i++)
			{
				float x = i / numPixels;

				copy.value = x;
				float y = 1.0f - copy.getValue();

				if (outlineEmpty)
				{
					outlinePath.startNewSubPath(x, y);
					outlineEmpty = false;
				}
				else
					outlinePath.lineTo(x, y);

				bool drawBiggerValue = valueBiggerThanHalf && x > 0.5f && x < v;
				bool drawSmallerValue = !valueBiggerThanHalf && x < 0.5f && x > v;

				if (drawBiggerValue || drawSmallerValue)
				{
					if (valueEmpty)
					{
						valuePath.startNewSubPath(x, y);
						valueEmpty = false;
					}
					else
						valuePath.lineTo(x, y);
				}
			}

			PathFactory::scalePath(outlinePath, pathArea.reduced(UIValues::NodeMargin));
			PathFactory::scalePath(valuePath, pathArea.reduced(UIValues::NodeMargin));

			repaint();
		}

		void paint(Graphics& g) override;

		void resized() override
		{
			auto b = getLocalBounds();
			dragger.setBounds(b.removeFromBottom(28));
			b.removeFromBottom(UIValues::NodeMargin);
			auto bSize = jmin(b.getWidth(), b.getHeight());
			pathArea = b.withSizeKeepingCentre(bSize, bSize).toFloat();

		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new bipolar_editor(dynamic_cast<BipolarBase*>(typed), updater);
		}

		Path outlinePath;
		Path valuePath;

		multilogic::bipolar lastData;

		Rectangle<float> pathArea;
		ModulationSourceBaseComponent dragger;
	};

	template <typename pma_type> struct pma_editor : public ModulationSourceBaseComponent
	{
		using PmaBase = control::pimpl::combined_parameter_base<pma_type>;
		using ParameterBase = control::pimpl::parameter_node_base<parameter::dynamic_base_holder>;

		pma_editor(mothernode* b, PooledUIUpdater* u) :
			ModulationSourceBaseComponent(u),
			obj(dynamic_cast<PmaBase*>(b))
		{
			setSize(100 * 3, 120);
		};

		void resized() override
		{
			setRepaintsOnMouseActivity(true);

			dragPath.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

			auto b = getLocalBounds().toFloat();
			b = b.withSizeKeepingCentre(28.0f, 28.0f);

			b = b.translated(0.0f, JUCE_LIVE_CONSTANT_OFF(5.0f));

			getProperties().set("circleOffsetY", -0.5f * (float)getHeight() + 2.0f);

			PathFactory::scalePath(dragPath, b);
		}

		void timerCallback() override
		{
			repaint();
		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new pma_editor(typed, updater);
		}

		void paint(Graphics& g) override
		{
			g.setFont(GLOBAL_BOLD_FONT());

			auto r = obj->currentRange;

			if (auto n = findParentComponentOfClass<NodeComponent>()->node)
			{
				r = RangeHelpers::getDoubleRange(n->getParameterFromName("Value")->data).rng;
			}

			String start, mid, end;

			int numDigits = jmax<int>(1, -1 * roundToInt(log10(r.interval)));

			start = String(r.start, numDigits);

			mid = String(r.convertFrom0to1(0.5), numDigits);

			end = String(r.end, numDigits);

			auto b = getLocalBounds().toFloat();

			float w = JUCE_LIVE_CONSTANT_OFF(85.0f);

			auto midCircle = b.withSizeKeepingCentre(w, w).translated(0.0f, 5.0f);

			float r1 = JUCE_LIVE_CONSTANT_OFF(3.0f);
			float r2 = JUCE_LIVE_CONSTANT_OFF(5.0f);

			float startArc = JUCE_LIVE_CONSTANT_OFF(-2.5f);
			float endArc = JUCE_LIVE_CONSTANT_OFF(2.5f);

			Colour trackColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xff4f4f4f));

			auto createArc = [startArc, endArc](Rectangle<float> b, float startNormalised, float endNormalised)
			{
				Path p;

				auto s = startArc + jmin(startNormalised, endNormalised) * (endArc - startArc);
				auto e = startArc + jmax(startNormalised, endNormalised) * (endArc - startArc);

				s = jlimit(startArc, endArc, s);
				e = jlimit(startArc, endArc, e);

				p.addArc(b.getX(), b.getY(), b.getWidth(), b.getHeight(), s, e, true);

				return p;
			};

			auto oc = midCircle;
			auto mc = midCircle.reduced(5.0f);
			auto ic = midCircle.reduced(10.0f);

			auto outerTrack = createArc(oc, 0.0f, 1.0f);
			auto midTrack = createArc(mc, 0.0f, 1.0f);
			auto innerTrack = createArc(ic, 0.0f, 1.0f);

			if (isMouseOver())
				trackColour = trackColour.withMultipliedBrightness(1.1f);

			if (isMouseButtonDown())
				trackColour = trackColour.withMultipliedBrightness(1.1f);

			g.setColour(trackColour);

			g.strokePath(outerTrack, PathStrokeType(r1));
			g.strokePath(midTrack, PathStrokeType(r2));
			g.strokePath(innerTrack, PathStrokeType(r1));

			g.fillPath(dragPath);

			auto data = obj->getUIData();

			auto nrm = [r](double v)
			{
				return r.convertTo0to1(v);
			};

			auto mulValue = nrm(data.value * data.mulValue);
			auto totalValue = nrm(data.getValue());

			auto outerRing = createArc(oc, mulValue, totalValue);
			auto midRing = createArc(mc, 0.0f, totalValue);
			auto innerRing = createArc(ic, 0.0f, mulValue);
			auto valueRing = createArc(ic, 0.0f, nrm(data.value));

			auto c1 = MultiOutputDragSource::getFadeColour(0, 2).withAlpha(0.8f);
			auto c2 = MultiOutputDragSource::getFadeColour(1, 2).withAlpha(0.8f);

			auto ab = getLocalBounds().removeFromBottom(5).toFloat();
			ab.removeFromLeft(ab.getWidth() / 3.0f);
			auto ar2 = ab.removeFromLeft(ab.getWidth() / 2.0f).withSizeKeepingCentre(5.0f, 5.0f);
			auto ar1 = ab.withSizeKeepingCentre(5.0f, 5.0f);

			g.setColour(Colour(c1));
			g.strokePath(outerRing, PathStrokeType(r1 - 1.0f));
			g.setColour(c1.withMultipliedAlpha(data.addValue == 0.0 ? 0.2f : 1.0f));
			g.fillEllipse(ar1);
			g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffd7d7d7)));
			g.strokePath(midRing, PathStrokeType(r2 - 1.0f));

			g.setColour(c2.withMultipliedAlpha(JUCE_LIVE_CONSTANT_OFF(0.4f)));
			g.strokePath(valueRing, PathStrokeType(r1 - 1.0f));
			g.setColour(c2.withMultipliedAlpha(data.mulValue == 1.0 ? 0.2f : 1.0f));
			g.fillEllipse(ar2);
			g.setColour(c2);
			g.strokePath(innerRing, PathStrokeType(r1));

			b.removeFromTop(18.0f);

			g.setColour(Colours::white.withAlpha(0.3f));

			Rectangle<float> t((float)getWidth() / 2.0f - 35.0f, 0.0f, 70.0f, 15.0f);

			g.drawText(start, t.translated(-70.0f, 80.0f), Justification::centred);
			g.drawText(mid, t, Justification::centred);
			g.drawText(end, t.translated(70.0f, 80.0f), Justification::centred);
		}

		WeakReference<PmaBase> obj;
		bool colourOk = false;
		Path dragPath;
	};

	struct sliderbank_pack : public data::dynamic::sliderpack
	{
		sliderbank_pack(data::base& t, int index=0) :
			data::dynamic::sliderpack(t, index)
		{};

		void initialise(NodeBase* n) override
		{
			sliderpack::initialise(n);

			outputListener.setCallback(n->getValueTree().getChildWithName(PropertyIds::SwitchTargets), 
									   valuetree::AsyncMode::Synchronously,
								       BIND_MEMBER_FUNCTION_2(sliderbank_pack::updateNumSliders));

			updateNumSliders({}, false);
		}

		void updateNumSliders(ValueTree v, bool wasAdded)
		{
			if (auto sp = dynamic_cast<SliderPackData*>(currentlyUsedData))
				sp->setNumSliders((int)outputListener.getParentTree().getNumChildren());
		}

		valuetree::ChildListener outputListener;
	};

	using dynamic_sliderbank = wrap::data<sliderbank<parameter::dynamic_list>, sliderbank_pack>;

	struct sliderbank_editor : public ScriptnodeExtraComponent<dynamic_sliderbank>
	{
		using NodeType = dynamic_sliderbank;

		sliderbank_editor(ObjectType* b, PooledUIUpdater* updater) :
			ScriptnodeExtraComponent<ObjectType>(b, updater),
			p(updater, &b->i),
			r(&b->getWrappedObject().p, updater)
		{
			addAndMakeVisible(p);
			addAndMakeVisible(r);

			setSize(256, 200);
			stop();
		};

		void resized() override
		{
			auto b = getLocalBounds();

			p.setBounds(b.removeFromTop(130));
			r.setBounds(b);
		}

		void timerCallback() override
		{
			jassertfalse;
		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto v = static_cast<NodeType*>(obj);
			return new sliderbank_editor(v, updater);
		}

		scriptnode::data::ui::sliderpack_editor p;
		parameter::ui::dynamic_list_editor r;
	};

}

namespace conversion_logic
{
struct dynamic
{
    using NodeType = control::converter<parameter::dynamic_base_holder, dynamic>;
    
    enum class Mode
    {
        Ms2Freq,
        Freq2Ms,
        Freq2Samples,
        Ms2Samples,
        Samples2Ms,
        Ms2BPM,
        Pitch2St,
        St2Pitch,
		Pitch2Cent,
		Cent2Pitch,
        Midi2Freq,
		Freq2Norm,
        Gain2dB,
        dB2Gain,
        numModes
    };
    
    dynamic() :
        mode(PropertyIds::Mode, getConverterNames()[0])
    {
        
    }
    
    static StringArray getConverterNames()
    {
        return { "Ms2Freq", "Freq2Ms", "Freq2Samples", "Ms2Samples", "Samples2Ms", "Ms2BPM",
                 "Pitch2St", "St2Pitch", "Pitch2Cent", "Cent2Pitch", "Midi2Freq", "Freq2Norm", "Gain2dB", "db2Gain" };
    }
    
    void initialise(NodeBase* n)
    {
        mode.initialise(n);
        mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::setMode), true);
    }
    
    void prepare(PrepareSpecs ps)
    {
        m.prepare(ps);
        s.prepare(ps);
        fs.prepare(ps);
    }
    
    double getValue(double input)
    {
        switch(currentMode)
        {
            case Mode::Ms2Freq:    return ms2freq().getValue(input);
            case Mode::Freq2Ms:    return freq2ms().getValue(input);
            case Mode::Ms2Samples: return m.getValue(input);
            case Mode::Samples2Ms: return s.getValue(input);
            case Mode::Freq2Samples: return fs.getValue(input);
            case Mode::Ms2BPM:     return ms2bpm().getValue(input);
            case Mode::Pitch2St:   return pitch2st().getValue(input);
            case Mode::St2Pitch:   return st2pitch().getValue(input);
			case Mode::Pitch2Cent: return pitch2cent().getValue(input);
			case Mode::Cent2Pitch: return cent2pitch().getValue(input);
            case Mode::Midi2Freq:  return midi2freq().getValue(input);
			case Mode::Freq2Norm:  return freq2norm().getValue(input);
            case Mode::Gain2dB:    return gain2db().getValue(input);
            case Mode::dB2Gain:    return db2gain().getValue(input);
            default: return input;
        }
    }
    
    void setMode(Identifier id, var newValue)
    {
        currentMode = (Mode)getConverterNames().indexOf(newValue.toString());
    }

    struct editor : public ScriptnodeExtraComponent<dynamic>,
                    public ComboBox::Listener
    {
        editor(dynamic* p, PooledUIUpdater* updater):
            ScriptnodeExtraComponent<dynamic>(p, updater),
            plotter(updater),
            modeSelector(getConverterNames()[0])
        {
            addAndMakeVisible(modeSelector);
            addAndMakeVisible(plotter);
            setSize(128, 24 + 28 + 30);
            
            modeSelector.addListener(this);
        }
        
        void setRange(NormalisableRange<double> nr, double center = -90.0)
        {
            auto n = findParentComponentOfClass<NodeComponent>()->node;
            auto p = n->getParameterFromIndex(0);
            
            if(center != -90.0)
                nr.setSkewForCentre(center);
            
            InvertableParameterRange r;
            r.rng = nr;
            
            RangeHelpers::storeDoubleRange(p->data, r, n->getUndoManager());
        }
        
        void paint(Graphics& g) override
        {
            g.setColour(Colours::white.withAlpha(0.5f));
            g.setFont(GLOBAL_BOLD_FONT());
            
            auto n = findParentComponentOfClass<NodeComponent>()->node;
            auto v = n->getParameterFromIndex(0)->getValue();
            auto output = getObject()->getValue(v);
            
            auto m = (Mode)getConverterNames().indexOf(modeSelector.getText());
            
            String inputDomain, outputDomain;
            
            switch(m)
            {
                case Mode::Ms2Freq: inputDomain = "ms"; outputDomain = "Hz"; break;
                case Mode::Freq2Ms: inputDomain = "Hz"; outputDomain = "ms"; break;
                case Mode::Freq2Samples: inputDomain = "Hz"; outputDomain = "smp"; break;
                case Mode::Ms2Samples:  inputDomain = "ms"; outputDomain = " smp"; break;
                case Mode::Samples2Ms:  inputDomain = "smp"; outputDomain = "ms"; break;
                case Mode::Ms2BPM:    inputDomain = "ms"; outputDomain = "BPM"; break;
                case Mode::Pitch2St:  inputDomain = ""; outputDomain = "st"; break;
                case Mode::St2Pitch:  inputDomain = "st"; outputDomain = ""; break;
				case Mode::Cent2Pitch: inputDomain = "ct"; outputDomain = ""; break;
				case Mode::Pitch2Cent: inputDomain = ""; outputDomain = "ct"; break;
                case Mode::Midi2Freq:  inputDomain = ""; outputDomain = "Hz"; break;
				case Mode::Freq2Norm:  inputDomain = "Hz"; outputDomain = ""; break;
                case Mode::Gain2dB: inputDomain = ""; outputDomain = "dB"; break;
                case Mode::dB2Gain: inputDomain = "dB"; outputDomain = ""; break;
                default: break;
            }
            
            String s;
            s << snex::Types::Helpers::getCppValueString(v);
            s << inputDomain << " -> ";
            s << snex::Types::Helpers::getCppValueString(output) << outputDomain;
            g.drawText(s, textArea, Justification::centred);
        }
        
        void comboBoxChanged(ComboBox* b)
        {
            auto m = (Mode)getConverterNames().indexOf(b->getText());
            
            switch(m)
            {
                case Mode::Ms2Freq: setRange({0.0, 1000.0, 1.0}); break;
                case Mode::Freq2Ms: setRange({20.0, 20000.0, 0.1}, 1000.0); break;
                case Mode::Freq2Samples: setRange({20.0, 20000.0, 0.1}, 1000.0); break;
                case Mode::Ms2Samples:  setRange({0.0, 1000.0, 1.0}); break;
                case Mode::Samples2Ms:  setRange({0.0, 44100.0, 1.0}); break;
                case Mode::Pitch2St:  setRange({0.5, 2.0}, 1.0); break;
                case Mode::Ms2BPM:    setRange({0.0, 2000.0, 1.0}); break;
                case Mode::St2Pitch:  setRange({-12.0, 12.0, 1.0}); break;
				case Mode::Pitch2Cent: setRange({ 0.5, 2.0 }, 1.0); break;
				case Mode::Cent2Pitch: setRange({ -100.0, 100.0, 0.0 }); break;
                case Mode::Midi2Freq:  setRange({0, 127.0, 1.0}); break;
				case Mode::Freq2Norm:  setRange({0.0, 20000.0}); break;
                case Mode::Gain2dB: setRange({0.0, 1.0, 0.0}); break;
                case Mode::dB2Gain: setRange({-100.0, 0.0, 0.1}, -12.0); break;
                default: break;
            }
        }

        void timerCallback() override
        {
            modeSelector.initModes(getConverterNames(), plotter.getSourceNodeFromParent());
            repaint();
        };

        static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
        {
            auto v = static_cast<NodeType*>(obj);
            return new editor(&v->obj, updater);
        }

        void resized() override
        {
            auto b = getLocalBounds();

            modeSelector.setBounds(b.removeFromTop(24));
            
            plotter.setBounds(b.removeFromBottom(28));
            
            textArea = b.toFloat();
        }

        Rectangle<float> textArea;
        ModulationSourceBaseComponent plotter;
        ComboBoxWithModeProperty modeSelector;

        Colour currentColour;
    };
    
    NodePropertyT<String> mode;
    
    Mode currentMode = Mode::Ms2Freq;
    
    conversion_logic::ms2samples m;
    conversion_logic::samples2ms s;
    conversion_logic::freq2samples fs;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};
}

namespace smoothers
{

struct dynamic_base : public base
{
	using NodeType = control::smoothed_parameter_base;

	enum class SmoothingType
	{
		NoSmoothing,
		LinearRamp,
		LowPass,
		numSmoothingTypes
	};

	static StringArray getSmoothNames() { return { "NoSmoothing", "Linear Ramp", "Low Pass" }; }

	dynamic_base() :
		mode(PropertyIds::Mode, "Linear Ramp")
	{};

	void initialise(NodeBase* n) override
	{
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic_base::setMode), true);
	}

	virtual void setMode(Identifier id, var newValue) {};
	
	virtual ~dynamic_base() {};

	struct editor : public ScriptnodeExtraComponent<dynamic_base>
	{
		editor(dynamic_base* p, PooledUIUpdater* updater);

		void paint(Graphics& g) override;

		void timerCallback();

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto v = static_cast<NodeType*>(obj);

			auto o = v->getSmootherObject();

			return new editor(dynamic_cast<dynamic_base*>(o), updater);
		}

		void resized() override
		{
			auto b = getLocalBounds();

			modeSelector.setBounds(b.removeFromTop(24));
			b.removeFromTop(UIValues::NodeMargin);
			plotter.setBounds(b);
		}

		ModulationSourceBaseComponent plotter;
		ComboBoxWithModeProperty modeSelector;

		Colour currentColour;
	};

	float get() const final override
	{
		return (float)lastValue.getModValue();
	}

protected:

	double value = 0.0;
	NodePropertyT<String> mode;
	ModValue lastValue;
	smoothers::base* b = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_base);
};

template <int NV> struct dynamic : public dynamic_base
{
	static constexpr int NumVoices = NV;
	
	dynamic()	
	{
		b = &r;
	}

	void reset() final override
	{
		b->reset();
	};

	void set(double nv) final override
	{
		value = nv;
		b->set(nv);
	}

	float advance() final override
	{
		if (enabled)
			lastValue.setModValueIfChanged(b->advance());
		
		return get();
	}

	void prepare(PrepareSpecs ps) final override
	{
		l.prepare(ps);
		r.prepare(ps);
		n.prepare(ps);
	}

	void refreshSmoothingTime() final override
	{
		b->setSmoothingTime(smoothingTimeMs);
	};

	float v = 0.0f;

	void setMode(Identifier id, var newValue) override
	{
		auto m = (SmoothingType)getSmoothNames().indexOf(newValue.toString());

		switch (m)
		{
		case SmoothingType::NoSmoothing: b = &n; break;
		case SmoothingType::LinearRamp: b = &r; break;
		case SmoothingType::LowPass: b = &l; break;
		default: b = &r; break;
		}

		refreshSmoothingTime();
		b->set(value);
		b->reset();
	}

	smoothers::no<NumVoices> n;
	smoothers::linear_ramp<NumVoices> r;
	smoothers::low_pass<NumVoices> l;
};
}

}
