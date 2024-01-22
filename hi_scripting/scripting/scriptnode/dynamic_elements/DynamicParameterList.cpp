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

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace duplilogic
{

duplilogic::dynamic::editor::editor(NodeType* obj, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<NodeType>(obj, u),
	dragger(u),
	mode("Spread")
{
	addAndMakeVisible(mode);

	

	setSize(150, 150 + 2 * 28 + 3 * UIValues::NodeMargin);
	addAndMakeVisible(dragger);
}

void duplilogic::dynamic::editor::paint(Graphics& g)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, area, false);

	auto numVoices = getObject()->numClones;
	auto v = getObject()->lastValue;
	auto gm = getObject()->lastGamma;
	auto b = area.reduced(5.0f);

	Colour lineColour = Colours::white.withAlpha(0.6f);

	if (auto nc = findParentComponentOfClass<NodeComponent>())
	{
		lineColour = nc->header.colour;

		if (lineColour == Colours::transparentBlack)
			lineColour = Colour(0xFFDADADA);
	}

	auto minY = (float)getHeight();
	auto maxY = 0.0f;

	UnblurryGraphics ug(g, *this, false);

	Array<Line<float>> lines;

	for (int i = 0; i < numVoices; i++)
	{
		auto thisV = getObject()->obj.getValue(i, numVoices, v, gm);

		auto thisY = (1.0f - (float)thisV) * b.getHeight() + b.getY();

		maxY = jmax(maxY, thisY);
		minY = jmin(minY, thisY);

		lines.add({ b.getX(), thisY, b.getRight(), thisY });

		g.setColour(Colours::white.withAlpha(0.03f));
		ug.draw1PxHorizontalLine(thisY, b.getX(), b.getRight());

		//g.drawLine(lines.getLast());
	}

	Rectangle<float> ar(b.getX(), minY, b.getWidth(), (maxY - minY));

	g.setColour(Colours::white.withAlpha(0.02f));
	g.fillRect(ar);


	if (numVoices > 1)
	{
		auto xWidth = b.getWidth() / (float)(numVoices - 1);

		int i = 0;

		Path linePath;

		for (float x = b.getX(); x < getWidth(); x += xWidth)
		{
			Line<float> pos(x, b.getY(), x, b.getBottom());

			g.setColour(Colours::white.withAlpha(0.03f));

			ug.draw1PxVerticalLine(x, b.getY(), b.getBottom());

			//g.drawLine(pos, 1.0f);

			auto p = pos.getIntersection(lines[i++]);

			if (i == 1)
				linePath.startNewSubPath(p);
			else
				linePath.lineTo(p);

			auto circle = Rectangle<float>(p, p).withSizeKeepingCentre(4.0f, 4.0f);

			g.setColour(lineColour);
			g.fillEllipse(circle);
		}

		g.strokePath(linePath, PathStrokeType(1.0f));
	}
}

void dynamic::editor::timerCallback()
{
	if (!initialised)
	{
		if (auto nc = findParentComponentOfClass<NodeComponent>())
		{
			mode.initModes(duplilogic::dynamic::getSpreadModes(), nc->node.get());
			initialised = true;
		}
	}

	repaint();
}

}

namespace parameter
{
	void clone_holder::callEachClone(int index, double v, bool)
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		lastValues.set(index, v);

		if (auto p = cloneTargets[index])
		{
			if(isNormalised)
				v = p->getRange().convertFrom0to1(v, true);

			p->call(v);
		}
	}

	Parameter* getParameterForDynamicParameter(NodeBase::Ptr root, dynamic_base::Ptr b)
	{
		Parameter* p = nullptr;

		root->forEach([&, b](NodeBase::Ptr n)
		{
			for (auto tp: NodeBase::ParameterIterator(*n))
			{
				if (tp->getDynamicParameter() == b)
				{
					p = tp;
					return true;
				}
			}

			return false;
		});

		return p;
	}

	ReferenceCountedArray<dynamic_base> getCloneParameters(Parameter* p)
	{
		ReferenceCountedArray<dynamic_base> list;

		if (auto root = p->parent->findParentNodeOfType<CloneNode>())
		{
			CloneNode::CloneIterator cit(*root, p->data, false);
			cit.resetError();

			if (cit.getCloneIndex() != 0)
			{
				cit.throwError("You need to connect the first clone");
				return list;
			}

			for (const auto& cv : cit)
			{
                if(auto cp = cit.getParameterForValueTree(cv))
                {
                    jassert(cv.getType() == PropertyIds::Parameter);
                    list.add(cp->getDynamicParameter());
                }
                else
                    jassertfalse;
			}
		}

		return list;
	}

	void clone_holder::setParameter(NodeBase* n, dynamic_base::Ptr b)
	{
		base = b;

        
        
        
		if (auto cn = dynamic_cast<CloneNode*>(connectedCloneContainer.get()))
		{
			cn->cloneChangeBroadcaster.removeListener(*this);
			cn->obj.removeNumClonesListener(parentListener);
		}

		if (auto c = dynamic_cast<dynamic_chain<true>*>(base.get()))
		{
			if (c->targets.size() == 1)
				base = c->targets.getFirst();
		}

		if (n != nullptr && b != nullptr)
		{
			if (!n->isClone())
			{
                n->getRootNetwork()->getExceptionHandler().addCustomError(n, Error::CloneMismatch, "Can't connect clone source to uncloned node");
				setParameter(nullptr, nullptr);
				return;
			}
			
			auto cloneParent = n->findParentNodeOfType<CloneNode>();
			connectedCloneContainer = cloneParent;

			rebuild(cloneParent);
			cloneParent->cloneChangeBroadcaster.addListener(*this, [](clone_holder& c, NodeBase* tc) {c.rebuild(tc); });
			cloneParent->obj.addNumClonesListener(parentListener);
		}

	}

	void clone_holder::rebuild(NodeBase* targetContainer)
	{
		if (auto ch = dynamic_cast<dynamic_chain<true>*>(base.get()))
		{
			ReferenceCountedArray<dynamic_base> chains;

			auto b = ch->targets.getFirst();

			{
				auto p = getParameterForDynamicParameter(targetContainer, b);
				auto firstTargets = getCloneParameters(p);

				auto isUnscaled = cppgen::CustomNodeProperties::isUnscaledParameter(p->data);

				for (auto ft : firstTargets)
				{
					auto newChain = new parameter::dynamic_chain<true>();
					newChain->addParameter(ft, isUnscaled);
					chains.add(newChain);
				}
			}
			

			for (int i = 1; i < ch->targets.size(); i++)
			{
				auto c = getParameterForDynamicParameter(targetContainer, ch->targets[i]);
				auto cTargets = getCloneParameters(c);

				auto isUnscaled = cppgen::CustomNodeProperties::isUnscaledParameter(c->data);

				for (int j = 0; j < chains.size(); j++)
				{
					dynamic_cast<parameter::dynamic_chain<true>*>(chains[j].get())->addParameter(cTargets[j], isUnscaled);
				}
			}

			SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);
			cloneTargets = chains;
		}
		else
		{
			auto p = getParameterForDynamicParameter(targetContainer, base);

			if (p == nullptr)
			{
				SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);
				cloneTargets.clear();
				return;
			}

			auto newTargets = getCloneParameters(p);

			{
				SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);
				cloneTargets = newTargets;
			}
		}

		for (int i = 0; i < lastValues.size(); i++)
		{
			callEachClone(i, lastValues[i], false);
		}
	}

dynamic_list::MultiOutputSlot::MultiOutputSlot(NodeBase* n, ValueTree switchTarget_) :
	ConnectionSourceManager(n->getRootNetwork(), getConnectionTree(n, switchTarget_)),
    switchTarget(switchTarget_),
	parentNode(n)
{
	jassert(switchTarget.getType() == PropertyIds::SwitchTarget);
	
	initConnectionSourceListeners();
}

juce::ValueTree dynamic_list::MultiOutputSlot::getConnectionTree(NodeBase* n, ValueTree st)
{
	return st.getOrCreateChildWithName(PropertyIds::Connections, n->getUndoManager(true));
}

void dynamic_list::MultiOutputSlot::rebuildCallback()
{
	auto dc = ConnectionBase::createParameterFromConnectionTree(parentNode, getConnectionTree(parentNode, switchTarget), true);
	p.setParameter(nullptr, dc);
}

void dynamic_list::initialise(NodeBase* n)
{
	parentNode = n;

	switchTree = n->getValueTree().getOrCreateChildWithName(PropertyIds::SwitchTargets, n->getUndoManager());

	auto modTree = n->getValueTree().getChildWithName(PropertyIds::ModulationTargets);

	if(modTree.isValid())
	{
		modTree.getParent().removeChild(modTree, n->getUndoManager(true));
	}


	connectionUpdater.setCallback(switchTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(dynamic_list::updateConnections));

	numParameters.initialise(n);
	numParameters.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic_list::updateParameterAmount));

	if (!rebuildMultiOutputSlots())
	{
		WeakReference<dynamic_list> safeThis(this);

		n->getRootNetwork()->addPostInitFunction([safeThis]()
		{
			if (safeThis.get() != nullptr)
			{
				return safeThis.get()->rebuildMultiOutputSlots();
			}

			return true;
		});
	}
}

bool dynamic_list::rebuildMultiOutputSlots()
{
	targets.clear();

 	for (auto c : switchTree)
		targets.add(new MultiOutputSlot(parentNode, c));

	for (auto t : targets)
	{
		if (!t->isInitialised())
			return false;
	}

	for (int i = 0; i < getNumParameters(); i++)
	{
		if (auto t = targets[i])
			targets[i]->p.call(lastValues[i]);
	}

	return true;
}

int dynamic_list::getNumParameters() const
{
	return targets.size();
}

void dynamic_list::updateConnections(ValueTree v, bool wasAdded)
{
	if (deferUpdate)
		return;

	rebuildMultiOutputSlots();
}

void dynamic_list::updateParameterAmount(Identifier, var newValue)
{
	auto numToUse = numParameters.getValue();

	lastValues.ensureStorageAllocated(numToUse);

	int numToRemove = switchTree.getNumChildren() - numToUse;
	int numToAdd = -numToRemove;

	if (numToAdd == 0)
		return;

	ScopedValueSetter<bool> svs(deferUpdate, true);

	if (numToRemove > 0)
	{
		for (int i = 0; i < numToRemove; i++)
		{
			switchTree.removeChild(switchTree.getNumChildren() - 1, parentNode->getUndoManager());
		}
	}
	else
	{
		for (int i = 0; i < numToAdd; i++)
		{
			ValueTree sv(PropertyIds::SwitchTarget);
			ValueTree cv(PropertyIds::Connections);
			sv.addChild(cv, -1, nullptr);
			switchTree.addChild(sv, -1, parentNode->getUndoManager());
		}
	}

	rebuildMultiOutputSlots();
}

juce::Path ui::Factory::createPath(const String& url) const
{
	Path p;
	LOAD_EPATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_EPATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	LOAD_PATH_IF_URL("drag", ColumnIcons::targetIcon);
	LOAD_PATH_IF_URL("edit", ColumnIcons::moveIcon);

	return p;
}



namespace ui
{
	struct dynamic_list_editor::MultiConnectionEditor::ConnectionEditor : public Component,
		public ButtonListener
	{
		ConnectionEditor(NodeBase* n_, ValueTree connectionTree, int index_, int numParameters_) :
			deleteButton("delete", this, f),
			n(n_),
			editor(n_, true, connectionTree, RangeHelpers::getHiddenIds()),
			c(connectionTree),
			index(index_),
			numParameters(numParameters_)
		{
			addAndMakeVisible(deleteButton);
			addAndMakeVisible(editor);

			setSize(editor.getWidth(), editor.p.getTotalContentHeight() + 24);
		}

		void buttonClicked(Button* b) override
		{
			c.getParent().removeChild(c, n->getUndoManager());
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds();
			auto top = b.removeFromTop(24);

			g.setColour(MultiOutputDragSource::getFadeColour(index, numParameters));
			g.fillRoundedRectangle(top.removeFromLeft(top.getHeight()).toFloat().reduced(1.0f), 2.0f);
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			String title;
			title << c[PropertyIds::NodeId].toString() << "." << c[PropertyIds::ParameterId].toString();
			g.drawText(title, top.toFloat(), Justification::centred);
		}

		void resized() override
		{
			auto b = getLocalBounds();

			auto top = b.removeFromTop(24);
			deleteButton.setBounds(top.removeFromRight(24).reduced(2));
			editor.setBounds(b);
		}

		int index;
		int numParameters;

		NodeBase::Ptr n;
		Factory f;
		ValueTree c;
		PropertyEditor editor;
		HiseShapeButton deleteButton;
	};

	struct dynamic_list_editor::MultiConnectionEditor::OutputEditor : public Component
	{
		valuetree::ChildListener removeListener;

		void rebuildEditors(ValueTree v, bool wasAdded)
		{
			editors.clear();

			int y = 0;

			for (auto v : c->getConnectionTree(c->parentNode, c->switchTarget))
			{
				editors.add(new ConnectionEditor(c->parentNode, v, index, numParameters));

				auto e = editors.getLast();
				e->setTopLeftPosition(0, y);
				addAndMakeVisible(e);
				y += e->getHeight();
			}

			setSize(MultiConnectionEditor::ViewportWidth - 16, y);
		}

		OutputEditor(parameter::dynamic_list::MultiOutputSlot* c_, int index_, int numParameters_) :
			c(c_),
			index(index_),
			numParameters(numParameters_)
		{
			rebuildEditors({}, true);
			auto cTree = c->getConnectionTree(c->parentNode, c->switchTarget);

			removeListener.setCallback(cTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(OutputEditor::rebuildEditors));
		}

		int index;
		int numParameters;

		parameter::dynamic_list::MultiOutputSlot* c;
		OwnedArray<ConnectionEditor> editors;
	};

	struct dynamic_list_editor::MultiConnectionEditor::WrappedOutputEditor : public Component
	{
		WrappedOutputEditor(parameter::dynamic_list::MultiOutputSlot* c)
		{
			index = c->switchTarget.getParent().indexOf(c->switchTarget);

			setName("Output " + String(index + 1));

			auto n = new OutputEditor(c, index, c->switchTarget.getParent().getNumChildren());
			vp.setViewedComponent(n);
			addAndMakeVisible(vp);

			containsSomething = n->editors.size() > 0;

			setSize(ViewportWidth, jmin(500, vp.getViewedComponent()->getHeight()));
		}

		bool containsSomething = false;

		void resized() override
		{
			vp.setBounds(getLocalBounds());
		}

		int index = 0;
		Viewport vp;
	};

	dynamic_list_editor::MultiConnectionEditor::MultiConnectionEditor(parameter::dynamic_list* l)
	{
		setName("Edit Connections");

		int maxHeight = 0;

		for (auto t : l->targets)
		{
			ScopedPointer<WrappedOutputEditor> n = new WrappedOutputEditor(t);

			if (n->containsSomething)
			{
				maxHeight = jmax(maxHeight, n->getHeight());
				addAndMakeVisible(n);
				editors.add(n.release());
			}
		}

		setSize(editors.size() * ViewportWidth, jmin(500, maxHeight));
	}

	void dynamic_list_editor::MultiConnectionEditor::resized()
	{
		auto b = getLocalBounds();
		for (auto v : editors)
		{
			v->setBounds(b.removeFromLeft(ViewportWidth));
		}
	}

	dynamic_list_editor::dynamic_list_editor(parameter::dynamic_list* l, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<parameter::dynamic_list>(l, updater),
		addButton("add", this, f),
		removeButton("delete", this, f),
		editButton("edit", this, f)
	{
		addButton.setTooltip("Add a connection output");
		removeButton.setTooltip("Remove the last connection output");

		addAndMakeVisible(addButton);
		addAndMakeVisible(removeButton);
		addAndMakeVisible(editButton);
	}

    bool dynamic_list_editor::rebuildDraggers()
    {
        if (getObject() == nullptr)
            return 0;

        bool changedSomething = false;
        
        if (dragSources.size() != getObject()->getNumParameters())
        {
            dragSources.clear();

            for (int i = 0; i < getObject()->getNumParameters(); i++)
            {
                dragSources.add(new DragComponent(getObject(), i));
                addAndMakeVisible(dragSources.getLast());
            }

            resized();
            changedSomething = true;
        }
        
        for(int i = 0; i < dragSources.size(); i++)
        {
            if(externalTextFunctions[i])
                dragSources[i]->textFunction = externalTextFunctions[i];
        }
        
        return changedSomething;
    }

    void dynamic_list_editor::showEditButtons(bool shouldShowButtons)
    {
	    addButton.setVisible(shouldShowButtons);
	    removeButton.setVisible(shouldShowButtons);
	    editButton.setVisible(shouldShowButtons);
	    resized();
    }

    void dynamic_list_editor::setTextFunction(int index, const std::function<String(int)>& f)
    {
	    externalTextFunctions.set(index, f);
    }

    void dynamic_list_editor::timerCallback()
	{
        rebuildDraggers();
	}

	void dynamic_list_editor::buttonClicked(Button* b)
	{
		if (b == &editButton)
		{
			auto n = new MultiConnectionEditor(getObject());

			NodeBase::showPopup(this, n);

			return;
		}

		int newValue = 0;

		if (b == &addButton)
			newValue = jmin(getObject()->getNumParameters() + 1, 8);

		if (b == &removeButton)
		{
			newValue = jmax(0, getObject()->getNumParameters() - 1);
		}

		getObject()->parentNode->setNodeProperty(PropertyIds::NumParameters, newValue);
	}

	void dynamic_list_editor::resized()
	{
		auto b = getLocalBounds();

		b.removeFromTop(5);
        
        if(addButton.isVisible())
        {
            auto top = b.removeFromTop(UIConstants::ButtonHeight);

            b.removeFromTop(5);
            b.removeFromBottom(10);

            addButton.setBounds(top.removeFromLeft(UIConstants::ButtonHeight).reduced(2));
            removeButton.setBounds(top.removeFromLeft(UIConstants::ButtonHeight).reduced(2));
            editButton.setBounds(top.removeFromLeft(UIConstants::ButtonHeight).reduced(2));
        }
        
		if (dragSources.size() > 0)
		{
			auto wPerDrag = b.getWidth() / dragSources.size();

			for (auto d : dragSources)
				d->setBounds(b.removeFromLeft(wPerDrag));
		}
	}

	dynamic_list_editor::DragComponent::DragComponent(parameter::dynamic_list* parent, int index_) :
		index(index_),
		pdl(parent)
	{
		n = dynamic_cast<WrapperNode*>(pdl->parentNode);
		p = Factory().createPath("drag");

		setRepaintsOnMouseActivity(true);

		setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());
	}

	bool dynamic_list_editor::DragComponent::matchesParameter(NodeBase::Parameter* p) const
	{
		if (auto t = pdl->targets[index])
		{
			return t->isConnectedToSource(p);
		}

		return false;
	}

	void dynamic_list_editor::DragComponent::mouseDown(const MouseEvent& e)
	{
		CHECK_MIDDLE_MOUSE_DOWN(e);

		if (e.mods.isRightButtonDown())
		{
			if (auto t = pdl->targets[index])
			{
				NodeBase::showPopup(this, new MultiConnectionEditor::WrappedOutputEditor(t));
			}
		}
	}

	void dynamic_list_editor::DragComponent::mouseDrag(const MouseEvent& e)
	{
		CHECK_MIDDLE_MOUSE_DRAG(e);

		if (e.mods.isRightButtonDown())
		{
			return;
		}

		auto container = findParentComponentOfClass<ContainerComponent>();

		auto root = pdl->parentNode->getRootNetwork()->getRootNode();

		while (container != nullptr && container->node.get() != root)
		{
			container = container->findParentComponentOfClass<ContainerComponent>();
		}

		if (container != nullptr)
		{
			auto details = new DynamicObject();
			auto nodeId = pdl->parentNode->getId();

			details->setProperty(PropertyIds::ID, nodeId);
			details->setProperty(PropertyIds::ParameterId, index);
			details->setProperty(PropertyIds::SwitchTarget, true);

			container->startDragging(var(details), this, ScaledImage(ModulationSourceBaseComponent::createDragImageStatic(false)));
			findParentComponentOfClass<DspNetworkGraph>()->repaint();
		}
	}

	void dynamic_list_editor::DragComponent::mouseUp(const MouseEvent& event)
	{
		CHECK_MIDDLE_MOUSE_UP(event);

		findParentComponentOfClass<DspNetworkGraph>()->repaint();
	}

		

		

	void dynamic_list_editor::DragComponent::resized()
	{
#if 0
		auto b = getLocalBounds().withSizeKeepingCentre(24, 24);

		
		auto pathBounds = b.toFloat().reduced(2.0f).translated(isHigher ? -12.0f : 0, isHigher ? 0.0f : -12.0f);
#endif
      
        auto isHigher = getWidth() < getHeight();
        
        auto b = getLocalBounds().withSizeKeepingCentre(jmax(20, getWidth() - UIValues::NodeMargin * 2), 20);
        
        auto pathBounds = b.removeFromLeft(20).toFloat();
        
        if(isHigher)
        {
            b.removeFromTop(UIValues::NodeMargin);
            textArea = pathBounds.translated(0.0, 26.0);
        }
        else
        {
            b.removeFromLeft(UIValues::NodeMargin);
            
            textArea = b.toFloat();
        }
        
		Factory::scalePath(p, pathBounds);

		auto xOffset = pathBounds.getCentreX() - (float)getWidth() / 2.0f;
		auto yOffset = pathBounds.getCentreY() - (float)getHeight() - JUCE_LIVE_CONSTANT_OFF(3.f);

		getProperties().set("circleOffsetX", xOffset);
		getProperties().set("circleOffsetY", yOffset);
	}

	void dynamic_list_editor::DragComponent::paint(Graphics& g)
	{
		auto b = getLocalBounds().toFloat().reduced(1.0f);

		g.setColour(Colours::black.withAlpha(0.1f));

		auto cornerSize = jmin(b.getWidth(), b.getHeight()) / 2.0f;

		g.fillRoundedRectangle(b, cornerSize);

		float alpha = 0.5f;

		if (isMouseOver())
			alpha += 0.1f;

		if (isMouseButtonDown())
			alpha += 0.2f;

		auto c = getFadeColour(index, getNumOutputs());

		g.setColour(c.withAlpha(alpha));
		g.fillPath(p);
		g.setFont(GLOBAL_BOLD_FONT());

		auto isHigher = getWidth() < getHeight();

        g.drawText(textFunction(index), textArea, isHigher ? Justification::centredTop : Justification::left);//p.getBounds().translated(isHigher ? 24.0f : 0.0f, isHigher ? 0.0f : 24.0f), Justification::centred);
	}
}


scriptnode::parameter::dynamic_base_holder* clone_holder::getParameterFunctionStatic(void* b)
{
	auto mn = static_cast<mothernode*>(b);
	auto typed = dynamic_cast<control::pimpl::parameter_node_base<parameter::clone_holder>*>(mn);
	jassert(typed != nullptr);
	return &typed->getParameter();
}

}

}
