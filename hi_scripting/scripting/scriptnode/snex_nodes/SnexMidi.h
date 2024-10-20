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
using namespace hise;
using namespace snex;

namespace midi_logic
{
struct dynamic
{
	using NodeType = control::midi<dynamic>;

	enum class Mode
	{
		Gate = 0,
		Velocity,
		NoteNumber,
		Frequency,
        Random
	};

	static StringArray getModes()
	{
		return { "Gate", "Velocity", "NoteNumber", "Frequency", "Random" };
	}

	dynamic();;

	class editor : public ScriptnodeExtraComponent<dynamic>,
                   public Value::Listener
	{
	public:

		editor(dynamic* t, PooledUIUpdater* updater);

		~editor()
		{
			if (auto obj = getObject())
			{
				midiMode.mode.asJuceValue().removeListener(this);
			}
		}

		void valueChanged(Value& value) override;

		void paint(Graphics& g) override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto typed = static_cast<NodeType*>(obj);
			return new editor(&typed->mType, updater);
		}

		void resized() override;

		void timerCallback() override;

		BlackTextButtonLookAndFeel blaf;
		GlobalHiseLookAndFeel claf;

		ComboBoxWithModeProperty midiMode;
		ModulationSourceBaseComponent dragger;
		VuMeterWithModValue meter;
	};	

	void prepare(PrepareSpecs ps);

	void initialise(NodeBase* n);

	void setMode(Identifier id, var newValue);

	bool getMidiValue(HiseEvent& e, double& v);

	bool getMidiValueWrapped(HiseEvent& e, double& v);

    NodeBase* parentNode = nullptr;
	ModValue lastValue;
	Mode currentMode = Mode::Gate;
	NodePropertyT<String> mode;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic);
};



}
}
