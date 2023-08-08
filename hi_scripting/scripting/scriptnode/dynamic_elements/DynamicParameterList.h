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
	struct MultiConnectionEditor : public ComponentWithMiddleMouseDrag
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
        Rectangle<float> textArea;
		WeakReference<parameter::dynamic_list> pdl;
	};

	dynamic_list_editor(parameter::dynamic_list* l, PooledUIUpdater* updater);

	void timerCallback() override;
	void buttonClicked(Button* b) override;
	void resized() override;

    bool rebuildDraggers();
    
    void showEditButtons(bool shouldShowButtons);

	void setTextFunction(int index, const std::function<String(int)>& f);

	Array<std::function<String(int)>> externalTextFunctions;
    
	Factory f;
	HiseShapeButton addButton;
	HiseShapeButton removeButton;
	HiseShapeButton editButton;
	OwnedArray<DragComponent> dragSources;
};
}


}






}
