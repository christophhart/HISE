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

namespace file_analysers
{
struct dynamic
{
	using NodeType = wrap::data<control::file_analyser<parameter::dynamic_base_holder, dynamic>, data::dynamic::audiofile>;

	enum AnalyserMode
	{
		Peak,
		Pitch,
		Length,
		numAnalyserModes
	};

	static StringArray getAnalyserModes()
	{
		return { "Peak", "Pitch", "Length" };
	}

	dynamic() :
		mode(PropertyIds::Mode, "Peak")
	{};

	void initialise(NodeBase* n)
	{
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::updateMode), true);

		parentNode = n;
	}

	void updateMode(Identifier, var newValue)
	{
		m = (AnalyserMode)getAnalyserModes().indexOf(newValue.toString());
		
		if (auto o = lastData.obj)
			o->getUpdater().sendContentChangeMessage(sendNotificationAsync, 90);
	}

	double getValue(const ExternalData& d)
	{
		lastData = d;

		switch (m)
		{
		case Length: lastValue = file_analysers::milliseconds().getValue(d); break;
		case Pitch: lastValue = file_analysers::pitch().getValue(d); break;
		case Peak: lastValue = file_analysers::peak().getValue(d); break;
        default: break;
		}

		return lastValue;
	}

	double lastValue = 0.0;
	ExternalData lastData;
	NodePropertyT<String> mode;
	AnalyserMode m;
	WeakReference<NodeBase> parentNode;

	struct editor : public ScriptnodeExtraComponent<NodeType>
	{
		editor(NodeType* n, PooledUIUpdater* u) :
			ScriptnodeExtraComponent<NodeType>(n, u),
			audioEditor(u, &n->i),
			modeSelector("Peak")
		{
			addAndMakeVisible(audioEditor);
			addAndMakeVisible(modeSelector);

			modeSelector.initModes(dynamic::getAnalyserModes(), getObject()->obj.analyser.parentNode.get());

			setSize(500, 128);
			stop();
		};

		void resized() override
		{
			auto b = getLocalBounds();
			modeSelector.setBounds(b.removeFromTop(28));
			audioEditor.setBounds(b);
		}

		void timerCallback() override
		{}

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<NodeType*>(obj);
			return new editor(typed, updater);
		}

		ComboBoxWithModeProperty modeSelector;
		data::ui::audiofile_editor_with_mod audioEditor;
	};
};
}


namespace faders
{
struct dynamic
{
	using NodeType = control::xfader<parameter::dynamic_list, dynamic>;

	static constexpr int NumMaxFaders = 8;

	enum FaderMode
	{
		Switch,
		Linear,
		Overlap,
		Squared,
		RMS,
        Cosine,
        CosineHalf,
		Harmonics,
        Threshold
	};

	dynamic() :
		mode(PropertyIds::Mode, "Linear")
	{}

	void initialise(NodeBase* n)
	{
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::updateMode), true);

		if (n->getValueTree().getChildWithName(PropertyIds::SwitchTargets).getNumChildren() == 0)
		{
			n->setNodeProperty(PropertyIds::NumParameters, 2);
		}
	}

	void updateMode(Identifier id, var newValue)
	{
		currentMode = (FaderMode)getFaderModes().indexOf(newValue.toString());
	}

	static StringArray getFaderModes()
	{
		return { "Switch", "Linear", "Overlap", "Squared", "RMS", "Cosine", "Cosine Half", "Harmonics", "Threshold" };
	}

	template <int Index> double getFadeValue(int numElements, double v)
	{
		switch (currentMode)
		{
		case FaderMode::Switch:		return sf.getFadeValue<Index>(numElements, v);
		case FaderMode::Linear:		return lf.getFadeValue<Index>(numElements, v);
		case FaderMode::Overlap:	return of.getFadeValue<Index>(numElements, v);
		case FaderMode::Squared:	return qf.getFadeValue<Index>(numElements, v);
		case FaderMode::RMS:		return rf.getFadeValue<Index>(numElements, v);
		case FaderMode::Harmonics:  return hf.getFadeValue<Index>(numElements, v);
        case FaderMode::Threshold:  return tr.getFadeValue<Index>(numElements, v);
        case FaderMode::Cosine:     return cs.getFadeValue<Index>(numElements, v);
        case FaderMode::CosineHalf: return ch.getFadeValue<Index>(numElements, v);
		}

		return 0.0;
	}

	NodePropertyT<String> mode;

	FaderMode currentMode = FaderMode::Linear;

	harmonics hf;
	switcher sf;
	linear lf;
	rms rf;
	overlap of;
	squared qf;
    threshold tr;
    cosine cs;
    cosine_half ch;

	struct editor : public ScriptnodeExtraComponent<NodeType>
	{
		struct FaderGraph : public ScriptnodeExtraComponent<NodeType>
		{
			FaderGraph(NodeType* f, PooledUIUpdater* updater);

			valuetree::RecursivePropertyListener graphRebuilder;

			void paint(Graphics& g) override;

			void setInputValue(double v);

			template <int Index> Path createPath() const
			{
				int numPaths = getObject()->p.getNumParameters();
				int numPixels = 256;

				Path p;
				p.startNewSubPath(0.0f, 0.0f);

				if (numPixels > 0)
				{
					for (int i = 0; i < numPixels; i++)
					{
						double inputValue = (double)i / (double)numPixels;
						auto output = (float)getObject()->fader.getFadeValue<Index>(numPaths, inputValue);

						p.lineTo((float)i, -1.0f * output);
					}
				}

				p.lineTo(numPixels - 1, 0.0f);
				p.closeSubPath();

				return p;
			}

			void rebuildFaderCurves();

			void resized() override;

			void timerCallback() override;

			double inputValue = 0.0;

			Array<Path> faderCurves;
		};

		editor(NodeType* v, PooledUIUpdater* updater_);;

		void timerCallback() override
		{
			jassertfalse;
		};

		void resized() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater);

		parameter::ui::dynamic_list_editor dragRow;

		ComboBoxWithModeProperty faderSelector;

		ScriptnodeComboBoxLookAndFeel plaf;
		FaderGraph graph;
	};
};
}

}
