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



namespace routing
{

#if USE_BACKEND

struct SelectorEditor: public ScriptnodeExtraComponent<selector_base>
{
    SelectorEditor(selector_base* s, PooledUIUpdater* u):
        ScriptnodeExtraComponent<selector_base>(s, u)
    {
        setSize(256, 80);
        
        static const unsigned char pathData[] = { 110,109,233,38,145,63,119,190,39,64,108,0,0,0,0,227,165,251,63,108,0,0,0,0,20,174,39,63,108,174,71,145,63,0,0,0,0,108,174,71,17,64,20,174,39,63,108,174,71,17,64,227,165,251,63,108,115,104,145,63,119,190,39,64,108,115,104,145,63,143,194,245,63,98,55,137,
    145,63,143,194,245,63,193,202,145,63,143,194,245,63,133,235,145,63,143,194,245,63,98,164,112,189,63,143,194,245,63,96,229,224,63,152,110,210,63,96,229,224,63,180,200,166,63,98,96,229,224,63,43,135,118,63,164,112,189,63,178,157,47,63,133,235,145,63,178,
    157,47,63,98,68,139,76,63,178,157,47,63,84,227,5,63,43,135,118,63,84,227,5,63,180,200,166,63,98,84,227,5,63,14,45,210,63,168,198,75,63,66,96,245,63,233,38,145,63,143,194,245,63,108,233,38,145,63,119,190,39,64,99,101,0,0 };

        p.loadPathFromData(pathData, sizeof(pathData));
    }
    
    void paint(Graphics& g) override
    {
        ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, getLocalBounds().reduced(10).toFloat(), false);
        
        if(auto obj = getObject())
        {
            auto size = obj->numProcessingChannels;
            
            Array<Rectangle<float>> inputs, outputs;
            
            int w = 30;
            
            auto b = getLocalBounds().withSizeKeepingCentre(size * w, getHeight());
            
            for(int i = 0; i < size; i++)
            {
                auto r = b.removeFromLeft(w);
                
                inputs.add(r.removeFromTop(r.getHeight()/2).withSizeKeepingCentre(10, 10).toFloat());
                outputs.add(r.withSizeKeepingCentre(10, 10).toFloat());
            }
            
            auto c = Colour(0xFF999999);
            
            g.setColour(c);
            
            for(auto &r: inputs)
            {
                PathFactory::scalePath(p, r);
                g.fillPath(p);
            }
            
            for(auto &r: outputs)
            {
                PathFactory::scalePath(p, r);
                g.fillPath(p);
            }
            
            for(int i = 0; i < obj->numChannels; i++)
            {
                auto sourceIndex = jlimit(0, size-1, obj->getChannelIndex() + i);
                auto targetIndex = jlimit(0, size-1, i);
                
                if(obj->selectOutput)
                    std::swap(sourceIndex, targetIndex);
                
                Line<float> l(inputs[sourceIndex].getCentre(), outputs[targetIndex].getCentre());
                
                g.setColour(Colour(0xFF444444));
                g.drawLine(l, 2.5f);
                g.setColour(Colour(0xFFAAAAAA));
                g.drawLine(l, 2.0f);
            }
        }
    }
    
    static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
    {
		auto mn = static_cast<mothernode*>(obj);
		auto t = dynamic_cast<selector_base*>(mn);
        return new SelectorEditor(t, updater);
    }
    
    void timerCallback() override
    {
        
    }
    
    Path p;
};

struct MatrixEditor : public ScriptnodeExtraComponent<matrix<dynamic_matrix>>
{
	MatrixEditor(matrix<dynamic_matrix>* r, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<matrix<dynamic_matrix>>(r, updater),
		editor(&r->m.getMatrix())
	{
		addAndMakeVisible(editor);
		setSize(600, 200);
		stop();
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new MatrixEditor(static_cast<ObjectType*>(obj), updater);
	}

	void timerCallback() override
	{
		
	}

	void resized() override
	{
		editor.setBounds(getLocalBounds());
	}

	hise::RouterComponent editor;
};
#else
using MatrixEditor = HostHelpers::NoExtraComponent;
using SelectorEditor = HostHelpers::NoExtraComponent;
#endif


struct ProcessingCheck
{
	void initialise(NodeBase* n)
	{
		b = n;
	}

	void prepare(PrepareSpecs ps)
	{
		ScriptnodeExceptionHandler::validateMidiProcessingContext(b);
	}

	NodeBase* b;
};

void local_cable_base::Manager::registerCable(WeakReference<local_cable_base> c)
{
	for(auto ex: registeredCables)
	{
		if(ex->cable == c)
			return;
	}

	registeredCables.add(new Item(*this, c));
	refreshAllConnections(c->getVariableId());
}

void local_cable_base::Manager::deregisterCable(WeakReference<local_cable_base> c)
{
	for(auto ex: registeredCables)
	{
		if(ex->cable == c)
		{
			registeredCables.removeObject(ex);
			break;
		}
	}

	refreshAllConnections(c->getVariableId());
}

void local_cable_base::Manager::refreshAllConnections(const String& v)
{
	ScopedLock sl(lock);

	variables.clear();

	for(auto con: registeredCables)
	{
		auto vid = con->cable->getVariableId();
		if(vid.isNotEmpty())
			variables.addIfNotAlreadyThere(vid);
	}

	for(auto con: registeredCables)
	{
		

		if(con->cable->getVariableId() == v || v.isEmpty())
		{
			con->variableIndex = variables.indexOf(con->cable->getVariableId());
			con->cable->refreshConnection(con->variableIndex);
		}
			
	}
}

local_cable_base::Manager::Item::Item(Manager& p, local_cable_base* c):
	parent(p),
	cable(c),
	propTree(c->node->getPropertyTree().getChild(0)),
	paramTree(c->node->getParameterTree().getChild(0))
{
	jassert(paramTree.isValid());
	idListener.setCallback(propTree, { PropertyIds::Value }, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Item::updateId));
	rangeListener.setCallback(paramTree, RangeHelpers::getRangeIds(), valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Item::updateRanges));
}

void local_cable_base::Manager::Item::updateRanges(const Identifier& id, const var& newValue)
{
	auto cid = cable->getVariableId();

	for(auto ex: parent.registeredCables)
	{
		if(ex == this)
			continue;

		auto eid = ex->cable->getVariableId();
					
		if(cid.isEmpty() || cid != eid)
			continue;

		ex->paramTree.setProperty(id, newValue, parent.um);
	}
}

void local_cable_base::Manager::Item::updateId(const Identifier&, const var& newValue)
{
	parent.refreshAllConnections(newValue.toString());
}

local_cable_base::editor::editor(local_cable_base* obj, PooledUIUpdater* u):
	ScriptnodeExtraComponent<scriptnode::routing::local_cable_base>(obj, u),
	name("", PropertyIds::LocalId),
	dragger(u),
	newButton("new", nullptr, *this),
	viewButton("debug", nullptr, *this)
{
	newButton.setTooltip("Create new local variable slot");
	viewButton.setTooltip("Show all connected local_cable nodes");

	newButton.onClick = [this]()
	{
		auto nn = PresetHandler::getCustomName("localVariableId", "Please enter the name of the local variable");

		if(nn.isNotEmpty())
		{
			findParentComponentOfClass<NodeComponent>()->node->setNodeProperty(PropertyIds::LocalId, nn);
		}
	};

	viewButton.onClick = [this]()
	{
		if(auto n = findParentComponentOfClass<DspNetworkGraph>())
		{
			Helpers::showAllOccurrences(n->network.get(), name.getText());
		}
	};

	addAndMakeVisible(name);
	addAndMakeVisible(newButton);
	addAndMakeVisible(viewButton);
	addAndMakeVisible(dragger);
	setSize(128, 28 * 2 + UIValues::NodeMargin);
}

void local_cable_base::editor::resized()
{
	auto b = getLocalBounds();

	auto top = b.removeFromTop(28);

	newButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(4));
	viewButton.setBounds(newButton.getBounds());

	newButton.setVisible(name.getText().isEmpty());
	viewButton.setVisible(name.getText().isNotEmpty());
			
	name.setBounds(top);

	b.removeFromTop(UIValues::NodeMargin);

	dragger.setBounds(b);
}

void local_cable_base::editor::timerCallback()
{
	if(auto n = findParentComponentOfClass<NodeComponent>())
	{
		if(!name.initialised)
		{
			auto sa = Helpers::getListOfLocalVariableNames(n->node->getRootNetwork()->getValueTree());
			name.initModes(sa, n->node.get());
			return;
		}

		if(++counter > 10)
		{
			auto sa = Helpers::getListOfLocalVariableNames(n->node->getRootNetwork()->getValueTree());
			counter = 0;
			if(name.getNumItems() != sa.size())
			{
				auto cv = name.getText();
				name.clear(dontSendNotification);
				name.addItemList(sa, 1);
				name.setText(cv, dontSendNotification);

						
			}
		}

		if(name.getText().isEmpty() != newButton.isVisible())
			resized();
	}
}

Component* local_cable_base::editor::createExtraComponent(void* obj, PooledUIUpdater* u)
{
	auto s = reinterpret_cast<mothernode*>(obj);
	auto typed = dynamic_cast<local_cable_base*>(s);
	return new editor(typed, u);
}

void local_cable_base::setValue(double v)
{
	if(!recursion)
	{
		ScopedValueSetter<bool> svs(recursion, true);
		sendValue(v);

		getManager()->setVariableValue(variableIndex, v);

		if (this->getParameter().isConnected())
			this->getParameter().call(v);
	}
}

void LocalCableHelpers::create(DspNetwork* network, const ValueTree& connectionData)
{
	// store the parent of the parent of the connection for the source logic
	auto sourceConnectionType = connectionData.getParent().getParent().getType();
	auto sourceConnectionIndex = connectionData.getParent().getParent().getParent().indexOf(connectionData.getParent().getParent());

	enum class Source
	{
		Unknown = -1,
		Parameter,
		Modulation,
		MultiOutput,
		numSources
	};

	static const Array<Identifier> ids = { PropertyIds::Parameter, PropertyIds::Node, PropertyIds::SwitchTarget };
	auto source = (Source)ids.indexOf(sourceConnectionType);
	
	auto um = network->getUndoManager();
	network->clear(false, true);

	String name;
	auto targetNodeId = connectionData[PropertyIds::NodeId].toString();
	auto targetParameterId = connectionData[PropertyIds::ParameterId].toString();

	auto rootNode = valuetree::Helpers::findParentWithType(connectionData, PropertyIds::Network);
	auto sourceNode = valuetree::Helpers::findParentWithType(connectionData, PropertyIds::Node);

	name << sourceNode[PropertyIds::ID].toString();

	if(source == Source::Parameter)
	{
		name << "_" << connectionData.getParent().getParent()[PropertyIds::ID].toString() << "_v";
	}
	else if (source == Source::Modulation)
	{
		name << "_mod_v";
	}
	else if (source == Source::MultiOutput)
	{
		name << "multi_" << String(sourceConnectionIndex) << "_v";
	}

	name = PresetHandler::getCustomName(name, "Please enter the name of the local cable you want to create from this connection");

	

	if(name.isNotEmpty())
	{
		
		ValueTree targetParameterNode;

		

		valuetree::Helpers::forEach(rootNode, [&](const ValueTree& v)
		{
			if(v.getType() == PropertyIds::Parameter &&
               v.getParent().getParent()[PropertyIds::ID].toString() == targetNodeId &&
			   v[PropertyIds::ID].toString() == targetParameterId)
			{
				targetParameterNode = v;
				return true;
			}

			return false;
		});

		auto sourceIsNormalised = [&]()
		{
			if(source == Source::Parameter)
			{
				auto targetRange = RangeHelpers::getDoubleRange(targetParameterNode);
				auto sourceRange = RangeHelpers::getDoubleRange(connectionData.getParent().getParent());

				return RangeHelpers::isIdentity(sourceRange) || !RangeHelpers::isEqual(sourceRange, targetRange);
			}
			else
			{
				if(auto modNode = dynamic_cast<ModulationSourceNode*>(network->getNodeForValueTree(sourceNode)))
				{
					return modNode->isUsingNormalisedRange();
				}
			}

			return true;
		}();

		if(targetParameterNode.isValid())
		{
			auto sourceExists = getListOfConnectedNodeTrees(rootNode, name).size() > 0;

			um->beginNewTransaction();

			ValueTree l1(PropertyIds::Node);
			ValueTree l2(PropertyIds::Node);

			// Setup the variable ID
			{
				ValueTree p(PropertyIds::Properties);
				ValueTree pv(PropertyIds::Property);
				pv.setProperty(PropertyIds::ID, PropertyIds::LocalId.toString(), nullptr);
				pv.setProperty(PropertyIds::Value, name, nullptr);
				p.addChild(pv, -1, nullptr);

				l1.addChild(p.createCopy(), -1, nullptr);
				l2.addChild(p.createCopy(), -1, nullptr);
			}

			// Setup the parameter
			{
				ValueTree parameters(PropertyIds::Parameters);

				auto tp = targetParameterNode.createCopy();

				if(sourceIsNormalised)
				{
					InvertableParameterRange r(0.0, 1.0);
					RangeHelpers::storeDoubleRange(tp, r, nullptr);
				}

				parameters.addChild(tp, -1, nullptr);
				parameters.getChild(0).setProperty(PropertyIds::ID, "Value", nullptr);

				auto pt1 = parameters;
				auto pt2 = parameters.createCopy();

				pt2.getChild(0).removeProperty(PropertyIds::Automated, nullptr);

				l1.addChild(pt1, -1, nullptr);
				l2.addChild(pt2, -1, nullptr);

			}

			// Add the modulation target trees (to avoid using the undo manager for that)
			{
				ValueTree mts(PropertyIds::ModulationTargets);
				l1.addChild(mts.createCopy(), -1, nullptr);
				l2.addChild(mts, -1, nullptr);
			}
			
			// Set the node properties
			{
				String path("routing.local_cable");

				if(!sourceIsNormalised)
					path << "_unscaled";

				l1.setProperty(PropertyIds::ID, name.replace("_v", "_source"), nullptr);
				l2.setProperty(PropertyIds::ID, name.replace("_v", "_target"), nullptr);
				l1.setProperty(PropertyIds::FactoryPath, path, nullptr);
				l2.setProperty(PropertyIds::FactoryPath, path, nullptr);
			}
			

			// Create and add the nodes
			{
				connectionData.getParent().removeChild(connectionData, um);

				auto targetNode = valuetree::Helpers::findParentWithType(targetParameterNode, PropertyIds::Node);

				auto sourceParent = sourceNode.getParent();

				if(source == Source::Parameter)
					sourceParent = sourceNode.getChildWithName(PropertyIds::Nodes);

				auto targetParent = targetNode.getParent();

				if(!sourceExists)
				{
					network->createFromValueTree(false, l1, true);
					sourceParent.addChild(l1, sourceParent.indexOf(sourceNode) + 1, um);
				}
					
				network->createFromValueTree(false, l2, true);
				targetParent.addChild(l2, targetParent.indexOf(targetNode), um);
			}

			// Add the source connection
			if(!sourceExists)
			{
				ValueTree sc(PropertyIds::Connection);
				sc.setProperty(PropertyIds::NodeId, l1[PropertyIds::ID], nullptr);
				sc.setProperty(PropertyIds::ParameterId, PropertyIds::Value.toString(), nullptr);

				ValueTree connectionParent;

				if(source == Source::Parameter)
				{
					auto parameterTree = sourceNode.getChildWithName(PropertyIds::Parameters).getChild(sourceConnectionIndex);
					connectionParent = parameterTree.getChildWithName(PropertyIds::Connections);
				}
				else if(source == Source::Modulation)
				{
					connectionParent = sourceNode.getChildWithName(PropertyIds::ModulationTargets);
				}
				else if (source == Source::MultiOutput)
				{
					auto switchTree = sourceNode.getChildWithName(PropertyIds::SwitchTargets).getChild(sourceConnectionIndex);
					connectionParent = switchTree.getChildWithName(PropertyIds::Connections);
				}
				else
				{
					jassertfalse;
				}

				if(connectionParent.isValid())
					connectionParent.addChild(sc, -1, um);
			}

			// Add the target connection 
			{
				l2.getChildWithName(PropertyIds::ModulationTargets).addChild(connectionData.createCopy(), -1, um);
			}
		}
	}
}

void LocalCableHelpers::replaceAllLocalCables(ValueTree& networkTree)
{
	auto localCables = getListOfLocalVariableNames(networkTree);

	for(auto lc: localCables)
	{
		auto c = getListOfConnectedNodeTrees(networkTree, lc);

		if(!c.isEmpty())
		{
			explode(c.getFirst(), nullptr);
		}
	}
}

void LocalCableHelpers::explode(ValueTree nodeTree, UndoManager* um)
{
	auto network = valuetree::Helpers::findParentWithType(nodeTree, PropertyIds::Network);
	auto idToExplode = nodeTree.getChildWithName(PropertyIds::Properties).getChildWithProperty(PropertyIds::ID, PropertyIds::LocalId.toString())[PropertyIds::Value].toString();
			
	auto list = getListOfConnectedNodeTrees(network, idToExplode);

	ValueTree source; // the parameter (or modulation target node in the connection source)
	Array<ValueTree> targets; // the parameter target
	ValueTree sourceConnection; // the connection from the source to the local cable


	for(auto n: list)
	{
		bool isModulating = n.getChildWithName(PropertyIds::ModulationTargets).isValid() && n.getChildWithName(PropertyIds::ModulationTargets).getNumChildren() > 0;
		bool isAutomated = (bool)n.getChildWithName(PropertyIds::Parameters).getChild(0)[PropertyIds::Automated];

		if(isModulating)
		{
			for(auto m: n.getChildWithName(PropertyIds::ModulationTargets))
			{
				jassert(m.getType() == PropertyIds::Connection);
				targets.add(m);
			}
		}

		if(isAutomated)
		{
			auto nid = n[PropertyIds::ID].toString();

			valuetree::Helpers::forEach(network, [&](ValueTree& v)
			{
				if(v.getType() == PropertyIds::Connection)
				{
					auto tid = v[PropertyIds::NodeId].toString();

					if(tid == nid)
					{
						sourceConnection = v;
						source = v.getParent();
						return true;
					}
				}

				return false;
			});
		}
	}

	if(source.isValid() && !targets.isEmpty())
	{
		source.removeChild(sourceConnection, um);

		for(auto n: list)
		{
			n.getParent().removeChild(n, um);
		}

		for(auto& t: targets)
		{
			source.addChild(t.createCopy(), -1, um);
		}
	}
}

void LocalCableHelpers::showAllOccurrences(DspNetwork* network, const String& variableName)
{
	auto occ = getListOfConnectedNodes(network, ValueTree(), variableName);

	network->deselectAll();

	for(auto n: occ)
	{
		auto vt = n->getValueTree();

		valuetree::Helpers::forEachParent(vt, [network](ValueTree& p)
		{
			if(p.getType() == PropertyIds::Node)
			{
				if(p[PropertyIds::Folded])
					p.setProperty(PropertyIds::Folded, false, network->getUndoManager());
			}

			return false;
		});

		network->addToSelection(n, ModifierKeys::shiftModifier);
	}
}

Array<ValueTree> LocalCableHelpers::getListOfConnectedNodeTrees(const ValueTree& networkTree,
	const String& variableName)
{
	Array<ValueTree> list;

	if(variableName.isEmpty())
		return list;

	valuetree::Helpers::forEach(networkTree, [&](ValueTree& v)
	{
		if(v.getType() == PropertyIds::Property && 
			v[PropertyIds::ID].toString() == PropertyIds::LocalId.toString() &&
			v[PropertyIds::Value].toString() == variableName)
		{
			auto nt = valuetree::Helpers::findParentWithType(v, PropertyIds::Node);

			list.addIfNotAlreadyThere(nt);
		}

		return false;
	});

	return list;
}

Array<WeakReference<NodeBase>> LocalCableHelpers::getListOfConnectedNodes(DspNetwork* network, const ValueTree& nodeTreeToSkip,
	const String& variableName)
{
	Array<WeakReference<NodeBase>> newConnections;

	auto list = getListOfConnectedNodeTrees(network->getValueTree(), variableName);

	for(auto nt: list)
	{
		if(nt == nodeTreeToSkip)
			continue;

		if(auto nb = network->getNodeForValueTree(nt, false))
			newConnections.addIfNotAlreadyThere(nb);
	}

	return newConnections;
}

StringArray LocalCableHelpers::getListOfLocalVariableNames(const ValueTree& networkTree)
{
	StringArray sa;

	valuetree::Helpers::forEach(networkTree, [&](ValueTree& v)
	{
		if(v.getType() == PropertyIds::Property && v[PropertyIds::ID].toString() == PropertyIds::LocalId.toString())
		{
			sa.addIfNotAlreadyThere(v[PropertyIds::Value].toString());
		}

		return false;
	});

	sa.sortNatural();
	sa.removeEmptyStrings();

	return sa;
}

void local_cable_base::initialise(NodeBase* n)
{
	node = n;

	if(node->getParameterTree().getNumChildren() == 0)
	{
		ParameterDataList list;
		this->createParameters(list);

		auto np = list[0].createValueTree();
		node->getParameterTree().addChild(np, -1, node->getUndoManager());
	}

	currentId.initialise(n);
	getManager()->registerCable(this);
}

local_cable_base::Manager::Ptr local_cable_base::getManager() const
{
	return dynamic_cast<Manager*>(node->getRootNetwork()->getLocalCableManager());
}

void local_cable_base::sendValue(double v)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	for(auto c: connections)
	{
		if(c != nullptr)
			c->getParameterFromIndex(0)->setValueAsync(v);
	}
}

void local_cable_base::refreshConnection(int newVariableIndex)
{
	for(int i = 0; i < connections.size(); i++)
	{
		if(connections[i] == nullptr)
		{
			hise::SimpleReadWriteLock::ScopedWriteLock sl(lock);
			connections.remove(i--);
		}
	}

	variableIndex = newVariableIndex;

	auto currentId = getVariableId();
	auto c = GlobalRoutingManager::Helpers::getColourFromId(currentId);
	node->setValueTreeProperty(PropertyIds::NodeColour, (int64)c.getARGB());

	auto newConnections = Helpers::getListOfConnectedNodes(node->getRootNetwork(), node->getValueTree(), currentId);

	jassert(!newConnections.contains(node));

	{
		hise::SimpleReadWriteLock::ScopedWriteLock sl(lock);
		connections.swapWith(newConnections);
	}

	sendValue(getManager()->getCurrentVariableValue(getVariableId()));
}

void dynamic_matrix::updateData()
{
	if (recursion)
		return;

	ScopedValueSetter<bool> svs(recursion, true);

	auto matrixData = ValueTreeConverters::convertValueTreeToBase64(getMatrix().exportAsValueTree(), true);

	internalData.storeValue(matrixData, um);

	memset(channelRouting, -1, NUM_MAX_CHANNELS);
	memset(sendRouting, -1, NUM_MAX_CHANNELS);

	for (int i = 0; i < getMatrix().getNumSourceChannels(); i++)
	{
		channelRouting[i] = (int8)getMatrix().getConnectionForSourceChannel(i);
		sendRouting[i] = (int8)getMatrix().getSendForSourceChannel(i);
	}
}

Factory::Factory(DspNetwork* n) :
	NodeFactory(n)
{
	// create this once to set the property...
	public_mod once;

	registerNode<matrix<dynamic_matrix>, MatrixEditor>();

	registerNode<dynamic_send, cable::dynamic::editor>();
	registerNode<dynamic_receive, cable::dynamic::editor>();
	registerNode<ms_encode>();
	registerNode<ms_decode>();
	registerNode<public_mod>();
    registerPolyNode<selector<1>, selector<NUM_POLYPHONIC_VOICES>, SelectorEditor>();
    
	registerNodeRaw<GlobalSendNode>();
	registerPolyNodeRaw<GlobalReceiveNode<1>, GlobalReceiveNode<NUM_POLYPHONIC_VOICES>>();
	registerNodeRaw<GlobalCableNode>();

	registerNoProcessNode<local_cable, local_cable_base::editor>();
	registerNoProcessNode<local_cable_unscaled, local_cable_base::editor>();

	registerPolyModNode<event_data_reader<1, ProcessingCheck>, event_data_reader<NUM_POLYPHONIC_VOICES, ProcessingCheck>, ModulationSourceBaseComponent>();
	registerPolyNode<event_data_writer<1, ProcessingCheck>, event_data_writer<NUM_POLYPHONIC_VOICES, ProcessingCheck>>();
}

}

namespace cable
{
snex::NamespacedIdentifier dynamic::getReceiveId()
{
	return NamespacedIdentifier("routing").getChildId(dynamic_receive::getStaticId());
}

void dynamic::prepare(PrepareSpecs ps)
{
	sendSpecs = ps;

	checkSourceAndTargetProcessSpecs();

	numChannels = ps.numChannels;

	if (ps.blockSize == 1)
	{
		useFrameDataForDisplay = true;
		frameData.referTo(data_, ps.numChannels);
		buffer.setSize(0);
	}
	else
	{
		useFrameDataForDisplay = false;

		frameData.referTo(data_, ps.numChannels);
		DspHelpers::increaseBuffer(buffer, ps);


		auto ptr = buffer.begin();
		FloatVectorOperations::clear(ptr, ps.blockSize * ps.numChannels);

		for (int i = 0; i < ps.numChannels; i++)
		{
			channels[i].referToRawData(ptr, ps.blockSize);
			ptr += ps.blockSize;
		}
	}
}

void dynamic::restoreConnections(Identifier id, var newValue)
{
	WeakReference<dynamic> safePtr(this);

	auto f = [safePtr, id, newValue]()
	{
		if (safePtr.get() == nullptr)
			return true;

        auto ok = false;
        
		if (id == PropertyIds::Value && safePtr->parentNode != nullptr)
		{
			auto ids = StringArray::fromTokens(newValue.toString(), ";", "");
			ids.removeDuplicates(false);
			ids.removeEmptyStrings(true);

			auto network = safePtr->parentNode->getRootNetwork();
			auto list = network->getListOfNodesWithPath(getReceiveId(), false);

			for (auto n : list)
			{
				if (auto rn = dynamic_cast<InterpretedNode*>(n.get()))
				{
					auto& ro = rn->getWrappedObject();

					auto source = ro.as<dynamic_receive>().source;

					if (ids.contains(rn->getId()))
					{
						source = safePtr.get();
						source->connect(ro.as<dynamic_receive>());
                        ok = true;
					}
					else
					{
						if (source == safePtr.get())
						{
							source = &(ro.as<dynamic_receive>().null);
                            ok = true;
						}
							
					}
				}
			}
		}

		return ok;
	};

	parentNode->getRootNetwork()->addPostInitFunction(f);
}

void dynamic::setConnection(dynamic_receive& receiveTarget, bool addAsConnection)
{
	receiveTarget.source = addAsConnection ? this : &receiveTarget.null;

	if (sendSpecs)
	{
		prepare(sendSpecs);
	}

	if (parentNode != nullptr)
	{
		auto l = parentNode->getRootNetwork()->getListOfNodesWithPath(getReceiveId(), true);

		for (auto n : l)
		{
			if (auto typed = dynamic_cast<InterpretedNode*>(n.get()))
			{
				if (&typed->getWrappedObject().as<dynamic_receive>() == &receiveTarget)
				{
					auto rIds = StringArray::fromTokens(receiveIds.getValue(), ";", "");

					rIds.removeEmptyStrings(true);
					rIds.removeDuplicates(false);
					rIds.sort(false);

					if (addAsConnection)
						rIds.addIfNotAlreadyThere(n->getId());
					else
						rIds.removeString(n->getId());

					receiveIds.storeValue(rIds.joinIntoString(";"), n->getUndoManager());
				}
			}
		}
	}
}



void dynamic::checkSourceAndTargetProcessSpecs()
{
	if (!sendSpecs)
		return;

	if (!receiveSpecs)
		return;
	
	if (postPrepareCheckActive || parentNode == nullptr)
		return;

	if (!(sendSpecs == receiveSpecs))
	{
		WeakReference<dynamic> safeThis(this);

		postPrepareCheckActive = true;

		parentNode->getRootNetwork()->addPostInitFunction([safeThis]()
		{
			if (safeThis == nullptr)
				return true;

			auto pn = safeThis->parentNode;
			auto& exp = pn->getRootNetwork()->getExceptionHandler();

			try
			{
				exp.removeError(pn);
				DspHelpers::validate(safeThis->sendSpecs, safeThis->receiveSpecs);
				safeThis->postPrepareCheckActive = false;
				return true;
			}
			catch (Error& e)
			{
				auto pn = safeThis->parentNode;
				exp.addError(pn, e);
				return false;
			}

			return true;
		});
	}
}

dynamic::dynamic() :
	receiveIds(PropertyIds::Connection, "")
{

}

void dynamic::reset()
{
	for (auto& d : frameData)
		d = 0.0f;

	for (auto& v : buffer)
		v = 0.0f;
}

void dynamic::validate(PrepareSpecs receiveSpecs_)
{
	receiveSpecs = receiveSpecs_;

	checkSourceAndTargetProcessSpecs();
}

void dynamic::initialise(NodeBase* n)
{
	parentNode = n;

	receiveIds.initialise(n);
	receiveIds.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(dynamic::restoreConnections), true);
}

void dynamic::connect(routing::receive<cable::dynamic>& receiveTarget)
{
	setConnection(receiveTarget, true);
}

template <typename T> static void callForEach(Component* root, const std::function<void(T*)>& f)
{
	if (auto typed = dynamic_cast<T*>(root))
	{
		f(typed);
	}

	for (int i = 0; i < root->getNumChildComponents(); i++)
	{
		callForEach(root->getChildComponent(i), f);
	}
}

dynamic::editor::editor(routing::base* b, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<routing::base>(b, u),
	levelDisplay(0.0f, 0.0f, VuMeter::StereoHorizontal)
{
	addAndMakeVisible(levelDisplay);
	levelDisplay.setInterceptsMouseClicks(false, false);
	levelDisplay.setForceLinear(true);
	levelDisplay.setColour(VuMeter::backgroundColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xff383838)));
	levelDisplay.setColour(VuMeter::ColourId::ledColour, JUCE_LIVE_CONSTANT_OFF(Colour(0xFFAAAAAA)));

	setSize(100, 18);

	setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());

	start();

	updatePeakMeter();
}

void dynamic::editor::resized()
{


	bool isSend = getAsSendNode() != nullptr;

	auto b = getLocalBounds();

	b.removeFromRight(15);
	b.removeFromLeft(15);

	levelDisplay.setBounds(b.reduced(1));

	float deltaY = JUCE_LIVE_CONSTANT_OFF(-11.5f);
	float deltaXS = JUCE_LIVE_CONSTANT_OFF(41.0f);
	float deltaXE = JUCE_LIVE_CONSTANT_OFF(-41.0f);

	b = getLocalBounds();

	auto iconBounds = isSend ? b.removeFromRight(getHeight()) : b.removeFromLeft(getHeight());

	icon.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

	PathFactory::scalePath(icon, iconBounds.toFloat().reduced(2.0f));

	getProperties().set("circleOffsetX", isSend ? deltaXS : deltaXE);
	getProperties().set("circleOffsetY", deltaY);
}

Error dynamic::editor::checkConnectionWhileDragging(const SourceDetails& dragSourceDetails)
{
	try
	{
		PrepareSpecs sp, rp;

		auto other = dynamic_cast<editor*>(dragSourceDetails.sourceComponent.get());

		if (auto rn = other->getAsReceiveNode())
		{
			if (auto sn = getAsSendNode())
			{
				sp = sn->cable.sendSpecs;
				rp = sn->cable.receiveSpecs;
			}
		}
		if (auto rn = getAsReceiveNode())
		{
			if (auto sn = other->getAsSendNode())
			{
				sp = sn->cable.sendSpecs;
				rp = sn->cable.receiveSpecs;
			}
		}

		DspHelpers::validate(sp, rp);
	}
	catch (Error& e)
	{
		return e;
	}

	return Error();
}

bool dynamic::editor::isValidDragTarget(editor* other)
{
	if (other == this)
		return false;

	auto srcIsSend = other->getAsSendNode() != nullptr;
	auto thisIsSend = getAsSendNode() != nullptr;

	return srcIsSend != thisIsSend;
}

bool dynamic::editor::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
	if (auto src = dynamic_cast<editor*>(dragSourceDetails.sourceComponent.get()))
	{
		return isValidDragTarget(src);
	}

	return false;
}

void dynamic::editor::itemDragEnter(const SourceDetails& dragSourceDetails)
{
	dragOver = true;
	currentDragError = checkConnectionWhileDragging(dragSourceDetails);
	repaint();
}

void dynamic::editor::itemDragExit(const SourceDetails& dragSourceDetails)
{
	dragOver = false;
	repaint();
}


void dynamic::editor::paintOverChildren(Graphics& g)
{
	if (dragMode)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(.1f));
		g.fillRoundedRectangle(getLocalBounds().toFloat(), getHeight() / 2);
	}

	if (isMouseOver(true))
	{
		if (auto rn = getAsReceiveNode())
		{
			if (rn->isConnected())
			{
				g.setColour(Colours::red.withAlpha(0.2f));
				g.fillRoundedRectangle(getLocalBounds().toFloat(), getHeight() / 2);
			}
		}
	}
}

void dynamic::editor::timerCallback()
{
	cable::dynamic* c = nullptr;
	float feedbackValue = 1.0f;

	if (auto sn = getAsSendNode())
		c = &sn->cable;

	if (auto rn = getAsReceiveNode())
	{
		c = rn->source;
		feedbackValue = rn->feedback;
	}

	if (c == nullptr)
	{
		levelDisplay.setPeak(0.0f, 0.0f);
		return;
	}

	int numChannels = c->getNumChannels();


	if (c->useFrameDataForDisplay)
	{
		auto l = c->frameData[0];
		auto r = numChannels == 2 ? c->frameData[1] : l;

		levelDisplay.setPeak(l * feedbackValue, r * feedbackValue);
	}
	else
	{
		int numSamples = c->channels[0].size();

		float l = DspHelpers::findPeak(c->channels[0].begin(), numSamples);
		float r = numChannels == 2 ? DspHelpers::findPeak(c->channels[1].begin(), numSamples) : l;

		levelDisplay.setPeak(l * feedbackValue, r * feedbackValue);
	}
}

juce::DragAndDropContainer* dynamic::editor::getDragAndDropContainer()
{
	auto c = findParentComponentOfClass<NodeComponent>();
	DragAndDropContainer* dd = nullptr;

	while (c != nullptr)
	{
		c = c->findParentComponentOfClass<NodeComponent>();

		if (auto thisDD = dynamic_cast<DragAndDropContainer*>(c))
			dd = thisDD;
	}

	return dd;
}

juce::Image dynamic::editor::createDragImage(const String& m, Colour bgColour)
{
	Path p;

	float margin = 10.0f;

	p.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));
	p.scaleToFit(5.0f, 5.0f, 15.0f, 15.0f, true);

	MarkdownRenderer mp(m, nullptr);

	mp.getStyleData().fontSize = 13.0f;

	mp.parse();
	auto height = (int)mp.getHeightForWidth(200.0f, true);

	Rectangle<float> b(0.0f, 0.0f, 240.0f, (float)height + 2 * margin);

	Image img(Image::ARGB, 240, height + 2 * margin, true);

	Graphics g(img);
	g.setColour(bgColour);
	g.fillRoundedRectangle(b, 3.0f);
	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT());

	g.fillPath(p);

	mp.draw(g, b.reduced(margin));

	return img;
}




scriptnode::cable::dynamic::dynamic_send* dynamic::editor::getAsSendNode()
{
	if (auto c = dynamic_cast<dynamic_send*>(getObject()))
	{
		return c;
	}

	return nullptr;
}

scriptnode::cable::dynamic::dynamic_receive* dynamic::editor::getAsReceiveNode()
{
	if (auto c = dynamic_cast<dynamic_receive*>(getObject()))
	{
		return c;
	}

	return nullptr;
}

void dynamic::editor::itemDropped(const SourceDetails& dragSourceDetails)
{
	auto src = dynamic_cast<editor*>(dragSourceDetails.sourceComponent.get());

	jassert(src != nullptr);



	if (auto thisAsCable = getAsSendNode())
	{
		if (auto srcAsReceive = src->getAsReceiveNode())
			thisAsCable->connect(*srcAsReceive);
	}
	if (auto thisAsReceive = getAsReceiveNode())
	{
		if (auto srcAsSend = src->getAsSendNode())
			srcAsSend->connect(*thisAsReceive);
	}

	dynamic_cast<Component*>(getDragAndDropContainer())->repaint();
	dragOver = false;

	src->updatePeakMeter();
	updatePeakMeter();
}

void dynamic::editor::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (e.mods.isRightButtonDown())
	{
		if (auto rn = getAsReceiveNode())
		{
			if (rn->isConnected())
			{
				rn->source->setConnection(*rn, false);
				findParentComponentOfClass<DspNetworkGraph>()->repaint();
			}
		}
	}
	else
	{
		auto dd = getDragAndDropContainer();
		dd->startDragging(var(), this, ScaledImage(ModulationSourceBaseComponent::createDragImageStatic(false)));

		findParentComponentOfClass<DspNetworkGraph>()->repaint();

		auto f = [this](editor* fc)
		{
			if (fc->isValidDragTarget(this))
			{
				fc->dragMode = true;
				fc->repaint();
			}
		};

		auto root = dynamic_cast<Component*>(getDragAndDropContainer());
		callForEach<editor>(root, f);
	}
}

void dynamic::editor::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);
	auto root = dynamic_cast<Component*>(getDragAndDropContainer());

	callForEach<editor>(root, [](editor* fc)
		{
			fc->dragMode = false;
			fc->repaint();
		});

	ZoomableViewport::checkDragScroll(e, true);
	findParentComponentOfClass<DspNetworkGraph>()->repaint();
}

void dynamic::editor::mouseDrag(const MouseEvent& event)
{
	CHECK_MIDDLE_MOUSE_DRAG(event);

	ZoomableViewport::checkDragScroll(event, false);

	findParentComponentOfClass<DspNetworkGraph>()->repaint();
}



void dynamic::editor::mouseDoubleClick(const MouseEvent& event)
{
	if (auto rn = getAsReceiveNode())
	{
		if (rn->isConnected())
		{
			rn->source->setConnection(*rn, false);
			findParentComponentOfClass<DspNetworkGraph>()->repaint();
		}
	}

	updatePeakMeter();
}

bool dynamic::editor::isConnected()
{
	if (auto rn = getAsReceiveNode())
		return rn->isConnected();

	if (auto sn = getAsSendNode())
		return sn->cable.receiveIds.getValue().isNotEmpty();

	return false;
}

void dynamic::editor::updatePeakMeter()
{
	levelDisplay.setVisible(isConnected());
	repaint();
}

void dynamic::editor::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), getHeight() / 2, 1.0f);

	g.fillPath(icon);

	if (!levelDisplay.isVisible())
	{
		String s = "Drag to ";

		if (getAsSendNode() != nullptr)
			s << "receive";
		else
			s << "send";

		g.setFont(GLOBAL_BOLD_FONT().withHeight(12.0f));
		g.drawText(s, levelDisplay.getBoundsInParent().toFloat(), Justification::centred);
	}

	if (dragOver)
	{
		auto c = !currentDragError.isOk() ? Colours::red : Colour(SIGNAL_COLOUR);
		g.setColour(c);
		g.drawRect(getLocalBounds().toFloat(), 1.0f);
	}
}

}



}
