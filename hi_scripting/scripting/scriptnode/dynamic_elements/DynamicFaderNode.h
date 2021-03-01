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
		Harmonics
	};

	dynamic() :
		mode(PropertyIds::Mode, "Linear")
	{}

	void initialise(NodeBase* n)
	{
		mode.initialise(n);
		mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::updateMode), true);
	}

	void updateMode(Identifier id, var newValue)
	{
		currentMode = (FaderMode)getFaderModes().indexOf(newValue.toString());
	}

	static StringArray getFaderModes()
	{
		return { "Switch", "Linear", "Overlap", "Squared", "RMS", "Harmonics" };
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

		PopupLookAndFeel plaf;
		FaderGraph graph;
	};
};
}

}
