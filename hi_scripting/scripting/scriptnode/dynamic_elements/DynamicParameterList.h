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

namespace parameter
{

struct clone_holder : public dynamic_base_holder
{
    clone_holder()
    {
        lastValues.ensureStorageAllocated(16);
    }

    static dynamic_base_holder* getParameterFunctionStatic(void* b);
    
    void callEachClone(int index, double v);

    void setParameter(NodeBase* n, dynamic_base::Ptr b) override;

    void rebuild(NodeBase* targetContainer);

    ReferenceCountedArray<dynamic_base> cloneTargets;
    Array<double> lastValues;

    NodeBase::Ptr connectedCloneContainer;

    void setParentNumClonesListener(scriptnode::wrap::clone_manager::Listener* pl)
    {
        parentListener = pl;
    }

    WeakReference<scriptnode::wrap::clone_manager::Listener> parentListener;

    JUCE_DECLARE_WEAK_REFERENCEABLE(clone_holder);
};
}

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
			Triangle,
			Fixed,
			Nyquist,
			Ducker
		};

		using NodeType = control::clone_cable<parameter::clone_holder, dynamic>;

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

		static constexpr bool isProcessingHiseEvent() { return true; }

		bool getMidiValue(HiseEvent& e, double& v)
		{
			switch (currentMode)
			{
			case DupliMode::Random:     return random().getMidiValue(e, v);
			case DupliMode::Fixed:      return fixed().getMidiValue(e, v);
			case DupliMode::Harmonics:  return harmonics().getMidiValue(e, v);
			case DupliMode::Nyquist:    return nyquist().getMidiValue(e, v);
            default:                    return false;
			}

			return false;
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
			case DupliMode::Fixed: return fixed().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Nyquist: return nyquist().getValue(index, numDuplicates, input, gamma);
			case DupliMode::Ducker: return ducker().getValue(index, numDuplicates, input, gamma);
			}

			return 0.0;
		}

		static StringArray getSpreadModes()
		{
			return { "Spread", "Scale", "Harmonics", "Random", "Triangle", "Fixed", "Nyquist", "Ducker"};
		}

		NodePropertyT<String> mode;
		DupliMode currentMode;

		struct editor : public ScriptnodeExtraComponent<NodeType>
		{
			editor(NodeType* obj, PooledUIUpdater* u);;

			void timerCallback() override;

			static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
			{
				auto mn = static_cast<mothernode*>(obj);
				auto typed = dynamic_cast<NodeType*>(mn);
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
	
	struct dynamic_list
	{
		static constexpr int size = 2;
		static constexpr bool isStaticList() { return false; }

		dynamic_list() :
			numParameters(PropertyIds::NumParameters, 0)
		{};

		virtual ~dynamic_list() {};

		struct MultiOutputSlot: public ConnectionSourceManager
		{
            MultiOutputSlot(NodeBase* n, ValueTree switchTarget);

			static ValueTree getConnectionTree(NodeBase* n, ValueTree switchTarget);

			void rebuildCallback() override;

			bool isInitialised() const
			{
				return p.base != nullptr || getConnectionTree(parentNode, switchTarget).getNumChildren() == 0;
			}

			ValueTree switchTarget;
			NodeBase::Ptr parentNode;
			parameter::dynamic_base_holder p;
            
			JUCE_DECLARE_WEAK_REFERENCEABLE(MultiOutputSlot);
		};

		valuetree::ChildListener connectionUpdater;

		NodePropertyT<int> numParameters;

		void initialise(NodeBase* n);

		bool rebuildMultiOutputSlots();

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
		OwnedArray<MultiOutputSlot> targets;

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

		WrapperNode* n;

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
