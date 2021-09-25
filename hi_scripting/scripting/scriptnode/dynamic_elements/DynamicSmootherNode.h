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

	struct pma_editor : public ModulationSourceBaseComponent
	{
		using PmaBase = control::pimpl::combined_parameter_base<control::multilogic::pma>;
		using ParameterBase = control::pimpl::parameter_node_base<parameter::dynamic_base_holder>;

		pma_editor(mothernode* b, PooledUIUpdater* u) :
			ModulationSourceBaseComponent(u),
			obj(dynamic_cast<PmaBase*>(b))
		{
			setSize(100 * 3, 120);
		};

		void resized() override;

		void timerCallback() override
		{
			repaint();
		}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<mothernode*>(obj);
			return new pma_editor(typed, updater);
		}

		void paint(Graphics& g) override;

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


namespace smoothers
{
struct dynamic : public base
{
	using NodeType = control::smoothed_parameter<dynamic>;

	enum class SmoothingType
	{
		NoSmoothing,
		LinearRamp,
		LowPass,
		numSmoothingTypes
	};

	static StringArray getSmoothNames() { return { "NoSmoothing", "Linear Ramp", "Low Pass" }; }

	dynamic() :
		mode(PropertyIds::Mode, "Linear Ramp")
	{
		b = &r;
	}

	float get() const final override
	{
		return (float)lastValue.getModValue();
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
		lastValue.setModValueIfChanged(b->advance());

		return (float)lastValue.getModValue();
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

	void initialise(NodeBase* n) override
	{
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::setMode), true);
	}

	void setEnabled(double value) final override
	{
		b->setEnabled(value);
	}

	void setMode(Identifier id, var newValue)
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

	NodePropertyT<String> mode;

	ModValue lastValue;
	double value = 0.0;

	smoothers::no n;
	smoothers::linear_ramp r;
	smoothers::low_pass l;
	smoothers::base* b = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);

	struct editor : public ScriptnodeExtraComponent<dynamic>
	{
		editor(dynamic* p, PooledUIUpdater* updater);

		void paint(Graphics& g) override;

		void timerCallback();

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto v = static_cast<NodeType*>(obj);
			return new editor(&v->value, updater);
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
};
}

}
