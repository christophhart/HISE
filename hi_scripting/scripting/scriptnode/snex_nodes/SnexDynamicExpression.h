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


struct dynamic_expression : public snex::DebugHandler
{
	using ControlNodeType = control::cable_expr<dynamic_expression, parameter::dynamic_base_holder>;
	using MathNodeType = math::OpNodeBase<dynamic_expression>;

	static constexpr int NumMessages = 6;
	using IndexType = index::wrapped<NumMessages>;

	static Identifier getId() { RETURN_STATIC_IDENTIFIER("expr"); };

    static String getDescription() { return "A JIT compiled math expression using SNEX."; }
    
	struct graph : public simple_visualiser
	{
		graph(PooledUIUpdater* u, dynamic_expression* e);;

		static float intersectsPath(Path& path, Rectangle<float> b);
		NormalisableRange<double> getXRange();
		double getInputValue();
		float getValue(double x);
		void rebuildPath(Path& path) override;

		float yMax = 0.0;
		float yMin = 0.0;

		WeakReference<dynamic_expression> expr;
	};

	struct editor : public ScriptnodeExtraComponent<dynamic_expression>,
		public TextEditor::Listener,
		public SettableTooltipClient,
		public ButtonListener
	{
		editor(dynamic_expression* e, PooledUIUpdater* u, bool isMathNode);

		void textEditorReturnKeyPressed(TextEditor&) override;
		void buttonClicked(Button* b) override;;

		void resized() override;
		void paint(Graphics& g) override;
		void timerCallback() override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater);

	private:

		struct Factory : public PathFactory
		{
			Path createPath(const String& id) const override;
		} f;

		static String getValueString(float v);
		int getYAxisLabelWidth() const;
		void drawYAxisValues(Graphics& g);

		juce::TextEditor te;
		juce::TextEditor logger;

		bool showError = false;
		CodeDocument doc;
		ModulationSourceBaseComponent d;

		HiseShapeButton debugButton;
		data::ui::pimpl::complex_ui_laf laf;

		int codeHeight = 24;
		int ywidth = 0;

		graph eg;

		const bool mathNode;
	};

	dynamic_expression();;

	bool isMathNode = false;

	void initialise(NodeBase* n);

	void logMessage(int level, const juce::String& s) override;

	String createMessageList() const;

	void updateCode(Identifier id, var newValue);

	template <typename T> void opSingle(T& data, float value)
	{
		SimpleReadWriteLock::ScopedReadLock sl(lock);

		if (expr != nullptr)
		{
			for (auto& s : data)
				s = expr->getFloatValueWithInputUnchecked(s, value);
		}
	}

	template <typename T> void op(T& data, float value)
	{
		SimpleReadWriteLock::ScopedReadLock sl(lock);

		if (expr != nullptr)
		{
			auto thisInput = data[0][0];
			lastInput = jmax(thisInput, lastInput * 0.97f);

			for (auto& ch : data)
			{
				for (auto& s : data.toChannelData(ch))
					s = expr->getFloatValueWithInputUnchecked(s, value);
			}

			lastValue = value;

			updateUIValue();
		}
	}

	void updateUIValue();

	double op(double input);

private:

	float lastValue = 1.0f;
	float lastInput = 0.0f;

	int updateCounter = 0;

	Result warning;

	span<String, NumMessages> messages;
	IndexType messageIndex;

	Result r;
	SimpleReadWriteLock lock;
	NodePropertyT<bool> debugEnabled;
	NodePropertyT<String> code;
	snex::JitExpression::Ptr expr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_expression);
};

}

