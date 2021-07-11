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


namespace duplilogic
{
	struct dynamic
	{
		enum class DupliMode
		{
			Spread,
			Scale,
			Harmonics,
			Random,
			Triangle
		};

		using NodeType = control::dupli_cable<parameter::dynamic_base_holder, dynamic>;

		dynamic() :
			mode(PropertyIds::Mode, "Spread")
		{};

		void initialise(NodeBase* n)
		{
			mode.initialise(n);
			mode.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::updateMode), true);
		}

		void updateMode(Identifier id, var newValue)
		{
			auto m = newValue.toString();
			currentMode = (DupliMode)getSpreadModes().indexOf(m);
		}

		double getValue(int index, int numDuplicates, double input, double gamma)
		{
			switch (currentMode)
			{
			case DupliMode::Spread: return spread().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Scale: return scale().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Harmonics: return harmonics().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Random: return random().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Triangle: return triangle().getValue(index, numDuplicates, input, gamma);
			}

			return 0.0;
		}

		static StringArray getSpreadModes()
		{
			return { "Spread", "Scale", "Harmonics", "Random", "Triangle"};
		}

		NodePropertyT<String> mode;
		DupliMode currentMode;

		struct editor : public ScriptnodeExtraComponent<NodeType>
		{
			editor(NodeType* obj, PooledUIUpdater* u);;

			void timerCallback() override;

			static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
			{
				auto typed = static_cast<NodeType*>(obj);

				return new editor(typed, updater);
			}

			void paint(Graphics& g) override;

			void resized() override
			{
				auto b = getLocalBounds();

				mode.setBounds(b.removeFromTop(28));
				b.removeFromTop(UIValues::NodeMargin);

				b.removeFromBottom(UIValues::NodeMargin);
				dragger.setBounds(b.removeFromBottom(28));
				b.removeFromBottom(UIValues::NodeMargin);
				area = b.toFloat();
			}

			Rectangle<float> area;
			ModulationSourceBaseComponent dragger;
			ComboBoxWithModeProperty mode;
			bool initialised = false;
		};
	};
}

namespace parameter
{
	struct dynamic_duplispread : public dynamic_base,
								 public scriptnode::wrap::duplicate_sender::Listener
	{
		static dynamic_base_holder* getParameterFunctionStatic(void* b)
		{
			using Type = control::dupli_cable<dynamic_base_holder, duplilogic::dynamic>;
			auto typed = static_cast<Type*>(b);
			return &typed->getParameter();
		};

		~dynamic_duplispread()
		{
			originalCallback = nullptr;

			if (connectedDuplicateObject != nullptr)
				connectedDuplicateObject->removeNumVoiceListener(this);
		}

		double getDisplayValue() const override
		{
			if (originalCallback != nullptr)
				return originalCallback->getDisplayValue();

			return lastValue;
		}

		void callWithDuplicateIndex(int index, double v) final override
		{
			lastValue = v;

			if (auto obj = targets[index])
			{
				originalCallback->obj = obj;
				originalCallback->call(v);
			}
		}

		void call(double v) final override
		{
			lastValue = v;
		}

		void updateUI() override
		{

		}

		void connect(NodeBase* newUnisonoMode, dynamic_base* cb);

		int getNumDuplicates() const
		{
			if (connectedDuplicateObject != nullptr)
			{
				return connectedDuplicateObject->getNumVoices();
			}

			return 1;
		}

		void numVoicesChanged(int newNumVoices) override
		{
			rebuildTargets();

			if (parentListener != nullptr)
				parentListener->numVoicesChanged(newNumVoices);
		}

		void setParentNumVoiceListener(DupliListener* l) override
		{
			parentListener = l;
		}

		void rebuildTargets();

		NodeBase::Ptr duplicateParentNode;
		ScopedPointer<dynamic_base> originalCallback;
		Array<void*> targets;
		WeakReference<scriptnode::wrap::duplicate_sender> connectedDuplicateObject;
		WeakReference<DupliListener> parentListener;
	};

	struct dynamic_list
	{
		static constexpr int size = 2;
		static constexpr bool isStaticList() { return false; }

		dynamic_list() :
			numParameters(PropertyIds::NumParameters, 0)
		{};

		virtual ~dynamic_list() {};

		struct MultiOutputConnection : public ConnectionBase
		{
			valuetree::ChildListener connectionListener;
			valuetree::RecursivePropertyListener connectionPropertyListener;

			MultiOutputConnection(NodeBase* n, ValueTree switchTarget);

			virtual ~MultiOutputConnection()
			{

			}

			void updateConnectionProperty(ValueTree, Identifier);

			bool rebuildConnections(bool tryAgainIfFail);

			void updateConnections(ValueTree v, bool wasAdded);

			bool matchesTarget(NodeBase::Parameter* np);

			bool ok = false;
			ValueTree connectionTree;
			NodeBase::Ptr parentNode;
			parameter::dynamic_base_holder p;

			JUCE_DECLARE_WEAK_REFERENCEABLE(MultiOutputConnection);
		};

		valuetree::ChildListener connectionUpdater;

		NodePropertyT<int> numParameters;

		void initialise(NodeBase* n);

		bool rebuildMultiOutputConnections();

		int getNumParameters() const;

		void updateConnections(ValueTree v, bool wasAdded);

		void updateParameterAmount(Identifier id, var newValue);

		template <int P> dynamic_base_holder& getParameter()
		{
			jassert(isPositiveAndBelow(P, getNumParameters()));
			return targets[P]->p;
		}

		template <int P> void call(double v)
		{
			lastValues.set(P, v);
			jassert(isPositiveAndBelow(P, getNumParameters()));
			targets[P]->p.call(v);
		}

		bool deferUpdate = false;

		ValueTree switchTree;
		NodeBase* parentNode;

		String missingNodes;

		Array<double> lastValues;
		ReferenceCountedArray<MultiOutputConnection> targets;

		

		JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_list);
};

namespace ui
{
	struct UIConstants
	{
		static constexpr int GraphHeight = 70;
		static constexpr int ButtonHeight = 22;
		static constexpr int DragHeight = 48;
	};

	struct Factory : public PathFactory
	{
		String getId() const override { return {}; }

		Path createPath(const String& url) const override;
	};

struct dynamic_list_editor : public ScriptnodeExtraComponent<parameter::dynamic_list>,
	public ButtonListener
{
	struct MultiConnectionEditor : public Component
	{
		static constexpr int ViewportWidth = 400 + 16;

		struct ConnectionEditor;
		struct OutputEditor;
		struct WrappedOutputEditor;

		MultiConnectionEditor(parameter::dynamic_list* l);

		void resized() override;

		OwnedArray<WrappedOutputEditor> editors;
	};

	struct DragComponent : public Component,
						   public MultiOutputDragSource
	{
		DragComponent(parameter::dynamic_list* parent, int index_);

		NodeBase* getNode() const override {
			return n;
		}

		int getNumOutputs() const override
		{
			return pdl->getNumParameters();
		}

		int getOutputIndex() const override
		{
			return index;
		}

		bool matchesParameter(NodeBase::Parameter* p) const override;

		ModulationSourceNode* n;

		void mouseDown(const MouseEvent& e) override;

		void mouseDrag(const MouseEvent& e);

		void mouseUp(const MouseEvent& event) override;

		void resized() override;

		void paint(Graphics& g) override;

		static String getDefaultText(int index)
		{
			return String(index + 1);
		}

		std::function<String(int)> textFunction = DragComponent::getDefaultText;
		const int index;
		Path p;
		WeakReference<parameter::dynamic_list> pdl;
	};

	dynamic_list_editor(parameter::dynamic_list* l, PooledUIUpdater* updater);

	void timerCallback() override;
	void buttonClicked(Button* b) override;
	void resized() override;

	Factory f;
	HiseShapeButton addButton;
	HiseShapeButton removeButton;
	HiseShapeButton editButton;
	OwnedArray<DragComponent> dragSources;
};
}


}






}
