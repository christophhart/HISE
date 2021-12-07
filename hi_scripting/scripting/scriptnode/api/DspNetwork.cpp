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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;


struct DspNetwork::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(DspNetwork, processBlock);
	API_VOID_METHOD_WRAPPER_2(DspNetwork, prepareToPlay);
	API_METHOD_WRAPPER_2(DspNetwork, create);
	API_VOID_METHOD_WRAPPER_2(DspNetwork, clear);
	API_METHOD_WRAPPER_1(DspNetwork, get);
	API_METHOD_WRAPPER_1(DspNetwork, createTest);
	API_VOID_METHOD_WRAPPER_1(DspNetwork, setForwardControlsToParameters);
	API_METHOD_WRAPPER_1(DspNetwork, setParameterDataFromJSON);
	API_METHOD_WRAPPER_3(DspNetwork, createAndAdd);
	API_METHOD_WRAPPER_2(DspNetwork, createFromJSON);
	API_METHOD_WRAPPER_0(DspNetwork, undo);
	//API_VOID_METHOD_WRAPPER_0(DspNetwork, disconnectAll);
	//API_VOID_METHOD_WRAPPER_3(DspNetwork, injectAfter);
};

DspNetwork::DspNetwork(hise::ProcessorWithScriptingContent* p, ValueTree data_, bool poly, ExternalDataHolder* dataHolder_) :
	ConstScriptingObject(p, 2),
	ControlledObject(p->getMainController_()),
	data(data_),
	isPoly(poly),
	polyHandler(poly),
#if HISE_INCLUDE_SNEX
	codeManager(*this),
#endif
	parentHolder(dynamic_cast<Holder*>(p)),
	projectNodeHolder(*this)
{
	jassert(data.getType() == PropertyIds::Network);

	tempoSyncer.publicModValue = &networkModValue;

	polyHandler.setTempoSyncer(&tempoSyncer);
	getScriptProcessor()->getMainController_()->addTempoListener(&tempoSyncer);

	if(!data.hasProperty(PropertyIds::AllowCompilation))
		data.setProperty(PropertyIds::AllowCompilation, false, nullptr);

	if(!data.hasProperty(PropertyIds::AllowPolyphonic))
		data.setProperty(PropertyIds::AllowPolyphonic, isPoly, nullptr);

    if(!data.hasProperty(PropertyIds::CompileChannelAmount))
        data.setProperty(PropertyIds::CompileChannelAmount, 2, nullptr);
    
	if (dataHolder_ != nullptr)
		setExternalDataHolder(dataHolder_);
	else
		setExternalDataHolder(dynamic_cast<ExternalDataHolder*>(p));

	ownedFactories.add(new NodeContainerFactory(this));
	ownedFactories.add(new core::Factory(this));
	ownedFactories.add(new math::Factory(this));
	ownedFactories.add(new envelope::Factory(this));
	ownedFactories.add(new routing::Factory(this));
	ownedFactories.add(new analyse::Factory(this));
	ownedFactories.add(new fx::Factory(this));
	ownedFactories.add(new control::Factory(this));
	ownedFactories.add(new dynamics::Factory(this));
	ownedFactories.add(new filters::Factory(this));
	ownedFactories.add(new jdsp::Factory(this));

#if USE_BACKEND
	if (auto ah = dynamic_cast<Holder*>(p))
	{
        if(!getScriptProcessor()->getMainController_()->isFlakyThreadingAllowed())
        {
            ownedFactories.add(new dll::BackendHostFactory(this, ah->projectDll));

            if (ah->projectDll != nullptr)
                projectNodeHolder.init(ah->projectDll);
        }
	}
#else
	if (auto ah = dynamic_cast<Holder*>(p))
	{
		ownedFactories.add(new hise::FrontendHostFactory(this));
	}
#endif

	for (auto nf : ownedFactories)
		nodeFactories.add(nf);

	loader = new DspFactory::LibraryLoader(dynamic_cast<Processor*>(p));

	setRootNode(createFromValueTree(true, data.getChild(0), true));
	networkParameterHandler.root = getRootNode();

	ADD_API_METHOD_1(processBlock);
	ADD_API_METHOD_2(prepareToPlay);
	ADD_API_METHOD_2(create);
	ADD_API_METHOD_3(createAndAdd);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_1(setForwardControlsToParameters);
	ADD_API_METHOD_1(setParameterDataFromJSON);
	ADD_API_METHOD_1(createTest);
	ADD_API_METHOD_2(clear);
	ADD_API_METHOD_2(createFromJSON);
	ADD_API_METHOD_0(undo);
	//ADD_API_METHOD_0(disconnectAll);
	//ADD_API_METHOD_3(injectAfter);

	selectionUpdater = new SelectionUpdater(*this);

	setEnableUndoManager(enableUndo);

	idUpdater.setCallback(data, { PropertyIds::ID }, valuetree::AsyncMode::Synchronously, 
		[this](ValueTree v, Identifier id)
	{
		if (auto n = getNodeForValueTree(v))
		{
			auto oldId = n->getCurrentId();
			auto newId = v[PropertyIds::ID].toString();

			changeNodeId(data, oldId, newId, getUndoManager());
		}
	});

	exceptionResetter.setTypeToWatch(PropertyIds::Nodes);
	exceptionResetter.setCallback(data, valuetree::AsyncMode::Synchronously, [this](ValueTree v, bool wasAdded)
	{
		if (!wasAdded)
		{
			if (auto n = getNodeForValueTree(v))
				this->getExceptionHandler().removeError(n);
		}
	});
    
    sortListener.setTypeToWatch(PropertyIds::Nodes);
    sortListener.setCallback(data, valuetree::AsyncMode::Synchronously, [this](ValueTree v, bool )
    {
        struct Sorter
        {
            void addRecursive(NodeBase* n)
            {
                newList.add(n);
                
                if(auto c = dynamic_cast<NodeContainer*>(n))
                    newList.addArray(c->getChildNodesRecursive());
            }
            int compareElements(NodeBase* n1, NodeBase* n2) const
            {
                auto i1 = newList.indexOf(n1);
                auto i2 = newList.indexOf(n2);
                
                if(i1 == i2)
                    return 0;
                
                if(i1 < i2)
                    return -1;
                
                return 1;
            }
            
            NodeBase::List newList;
        } sorter;
        
        sorter.newList.ensureStorageAllocated(nodes.size());
        sorter.addRecursive(getRootNode());
        
        nodes.sort(sorter);
    });

	checkIfDeprecated();

	runPostInitFunctions();
}

DspNetwork::~DspNetwork()
{
	root = nullptr;
	selectionUpdater = nullptr;
	nodes.clear();
    nodeFactories.clear();
	
	

	getMainController()->removeTempoListener(&tempoSyncer);
}


void DspNetwork::setNumChannels(int newNumChannels)
{
    if(newNumChannels != currentSpecs.numChannels)
    {
        currentSpecs.numChannels = newNumChannels;
        prepareToPlay(currentSpecs.sampleRate, currentSpecs.blockSize);
    }
}

void DspNetwork::createAllNodesOnce()
{
	if (cppgen::CustomNodeProperties::isInitialised())
		return;

	for (auto f : nodeFactories)
	{
		for (auto id : f->getModuleList())
		{
			NodeBase::Holder s;

			currentNodeHolder = &s;
			create(id, "unused");
			exceptionHandler.removeError(nullptr);
			currentNodeHolder = nullptr;
		}
	}

#if USE_BACKEND
    
    // Now check whether the compiled nodes should be rendered with a template
    // argument for their voice count
    auto fileList = BackendDllManager::getNetworkFiles(getScriptProcessor()->getMainController_(), false);
    
    for(auto f: fileList)
    {
        using namespace snex::cppgen;
        
        if(auto xml = XmlDocument::parse(f))
        {
            auto v = ValueTree::fromXml(*xml);
            
            auto isPoly = ValueTreeIterator::forEach(v, ValueTreeIterator::Forward, [](ValueTree& v)
            {
                if(v.getType() == PropertyIds::Node)
                {
                    if(CustomNodeProperties::nodeHasProperty(v, PropertyIds::IsPolyphonic))
                    {
                        return true;
                    }
                }
                
                return false;
            });
            
            if(isPoly)
            {
                CustomNodeProperties::addNodeIdManually(f.getFileNameWithoutExtension(), PropertyIds::IsPolyphonic);
            }
        }
    }
    
    
    
#endif
    
	cppgen::CustomNodeProperties::setInitialised(true);
}

NodeBase* DspNetwork::getNodeForValueTree(const ValueTree& v)
{
	if (!v.isValid())
		return {};

	auto& nodeListToUse = currentNodeHolder != nullptr ? currentNodeHolder->nodes : nodes;

	for (auto n : nodeListToUse)
	{
		if (n->getValueTree() == v)
			return n;
	}

	if (currentNodeHolder != nullptr)
		return createFromValueTree(isPolyphonic(), v, true);

	return nullptr;
}

NodeBase::List DspNetwork::getListOfUnconnectedNodes() const
{
	NodeBase::List unconnectedNodes;
	unconnectedNodes.ensureStorageAllocated(nodes.size());

	for (auto n : nodes)
	{
		if (!n->isActive(false))
			unconnectedNodes.add(n);
	}

	return unconnectedNodes;
}


juce::ValueTree DspNetwork::getListOfAvailableModulesAsTree() const
{
	ValueTree v(PropertyIds::Nodes);

	for (auto nf : nodeFactories)
	{
		auto list = nf->getModuleList();

		ValueTree f("Factory");
		f.setProperty(PropertyIds::ID, nf->getId().toString(), nullptr);

		for (auto l : list)
		{
			ValueTree n(PropertyIds::Node);
			n.setProperty(PropertyIds::FactoryPath, f[PropertyIds::ID].toString(), nullptr);
			n.setProperty(PropertyIds::ID, l, nullptr);
			f.addChild(n, -1, nullptr);
		}

		v.addChild(f, -1, nullptr);
	}

	return v;
}

juce::StringArray DspNetwork::getListOfAllAvailableModuleIds() const
{
	StringArray sa;

	for (auto nf : nodeFactories)
	{
		nf->setNetworkToUse(const_cast<DspNetwork*>(this));
		sa.addArray(nf->getModuleList());
	}

	return sa;
}


juce::StringArray DspNetwork::getListOfUsedNodeIds() const
{
	StringArray sa;

	for (auto n : nodes)
	{
		if (isInSignalPath(n))
			sa.add(n->getId());
	}

	return sa;
}


juce::StringArray DspNetwork::getListOfUnusedNodeIds() const
{
	auto list = getListOfUnconnectedNodes();

	StringArray sa;

	for (auto l : list)
		sa.add(l->getId());

	return sa;
}

juce::StringArray DspNetwork::getFactoryList() const
{
	StringArray sa;

	for (auto nf : nodeFactories)
	{
		if (nf->getModuleList().isEmpty())
			continue;

		sa.add(nf->getId().toString());
	}
		

	return sa;
}



scriptnode::NodeBase::Holder* DspNetwork::getCurrentHolder() const
{
	if (currentNodeHolder != nullptr)
		return currentNodeHolder.get();


	return const_cast<DspNetwork*>(this);
}

void DspNetwork::registerOwnedFactory(NodeFactory* ownedFactory)
{
	ownedFactories.add(ownedFactory);
	nodeFactories.addIfNotAlreadyThere(ownedFactory);
}

void DspNetwork::reset()
{
	if (projectNodeHolder.isActive())
		projectNodeHolder.n.reset();
	else
		getRootNode()->reset();
}

void DspNetwork::handleHiseEvent(HiseEvent& e)
{
	if (projectNodeHolder.isActive())
		projectNodeHolder.n.handleHiseEvent(e);
	else
		getRootNode()->handleHiseEvent(e);
}

void DspNetwork::process(AudioSampleBuffer& b, HiseEventBuffer* e)
{
	ProcessDataDyn d(b.getArrayOfWritePointers(), b.getNumSamples(), b.getNumChannels());
	d.setEventBuffer(*e);

	process(d);
}

void DspNetwork::process(ProcessDataDyn& data)
{
	if (projectNodeHolder.isActive())
	{
		projectNodeHolder.process(data);
		return;
	}

	if (auto s = SimpleReadWriteLock::ScopedTryReadLock(getConnectionLock()))
	{
		if (exceptionHandler.isOk())
			getRootNode()->process(data);
	}
}

juce::Identifier DspNetwork::getParameterIdentifier(int parameterIndex)
{
	return getRootNode()->getParameterFromIndex(parameterIndex)->getId();
}

void DspNetwork::setForwardControlsToParameters(bool shouldForward)
{
	checkValid();

	forwardControls = shouldForward;
}

void DspNetwork::prepareToPlay(double sampleRate, double blockSize)
{
	runPostInitFunctions();

	if (sampleRate > 0.0)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(getConnectionLock(), isInitialised());

		try
		{
			originalSampleRate = sampleRate;

			currentSpecs.sampleRate = sampleRate;
			currentSpecs.blockSize = (int)blockSize;

            if(currentSpecs.numChannels == 0)
                return;
            
			if (auto rootNode = getRootNode())
			{
				currentSpecs.voiceIndex = getPolyHandler();

				getRootNode()->prepare(currentSpecs);
				getRootNode()->reset();

				if (projectNodeHolder.isActive())
					projectNodeHolder.prepare(currentSpecs);
			}
		}
		catch (String& errorMessage)
		{
			jassertfalse;
		}
	}
}

void DspNetwork::processBlock(var pData)
{
	if (auto ar = pData.getArray())
	{
		int numChannelsToUse = ar->size();
		int numSamplesToUse = 0;

		int index = 0;

		for (const auto& v : *ar)
		{
			if (auto bf = v.getBuffer())
			{
				int thisSamples = bf->buffer.getNumSamples();

				if (numSamplesToUse == 0)
					numSamplesToUse = thisSamples;
				else if (numSamplesToUse != thisSamples)
					reportScriptError("Buffer mismatch");

				currentData[index++] = bf->buffer.getWritePointer(0);
			}
		}

		ProcessDataDyn d(currentData, numSamplesToUse, numChannelsToUse);

		process(d);
	}
}


var DspNetwork::create(String path, String id)
{
	checkValid();

	var existing = get(id);

	if (auto existingNode = dynamic_cast<NodeBase*>(existing.getObject()))
		return var(existingNode);

	ValueTree newNodeData(PropertyIds::Node);

	if (id.isEmpty())
	{
		String nameToUse = path.contains(".") ? path.fromFirstOccurrenceOf(".", false, false) : path;
		StringArray unused;
		id = getNonExistentId(nameToUse, unused);
	}

	newNodeData.setProperty(PropertyIds::ID, id, nullptr);
	newNodeData.setProperty(PropertyIds::FactoryPath, path, nullptr);

	NodeBase::Ptr newNode = createFromValueTree(isPoly, newNodeData);

	return var(newNode);
}

juce::var DspNetwork::createAndAdd(String path, String id, var parent)
{
	if (id.isEmpty())
	{
		StringArray unused;
		id = getNonExistentId(path.fromFirstOccurrenceOf(".", false, false), unused);
	}

	auto node = create(path, id);

	if (auto n = dynamic_cast<NodeBase*>(node.getObject()))
	{
		n->setParent(parent, -1);
	}

	return node;
}

var DspNetwork::createFromJSON(var jsonData, var parent)
{
	if (auto obj = jsonData.getDynamicObject())
	{
		auto path = obj->getProperty(PropertyIds::FactoryPath).toString();
		auto id = obj->getProperty(PropertyIds::ID).toString();

		auto node = createAndAdd(path, id, parent);

		if (dynamic_cast<NodeBase*>(node.getObject()) == nullptr)
			return var();

		if (obj->hasProperty(PropertyIds::Nodes))
		{
			auto a = obj->getProperty(PropertyIds::Nodes).getArray();

			for (auto c : *a)
			{
				auto ok = createFromJSON(c, node);
				
				if (!ok.isObject())
					return var();
			}
		}

		return node;
	}

	return false;
}

var DspNetwork::get(var idOrNode) const
{
	checkValid();

	if(auto n = dynamic_cast<NodeBase*>(idOrNode.getObject()))
		return idOrNode;

	auto id = idOrNode.toString();

	if (id.isEmpty())
		return {};

	if (id == getId())
		return var(getRootNode());

	auto& listToUse = currentNodeHolder != nullptr ? currentNodeHolder->nodes : nodes;

	for (auto n : listToUse)
	{
		if (n->getId() == id)
			return var(n);
	}
	
	return {};
}

void DspNetwork::clear(bool removeNodesFromSignalChain, bool removeUnusedNodes)
{
	if (removeNodesFromSignalChain)
	{
		getRootNode()->getValueTree().getChildWithName(PropertyIds::Nodes).removeAllChildren(getUndoManager());
		getRootNode()->getParameterTree().removeAllChildren(getUndoManager());
	}
	
	if (removeUnusedNodes)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			if (!nodes[i]->isActive(true))
			{
				MessageManagerLock mm;
				deleteIfUnused(nodes[i]->getId());
				i--;
			}
		}
	}
}

bool DspNetwork::undo()
{
	return getUndoManager(true)->undo();
}

var DspNetwork::createTest(var testData)
{
#if USE_BACKEND
	if (auto obj = testData.getDynamicObject())
	{
		obj->setProperty(PropertyIds::NodeId, getId());
		return new ScriptNetworkTest(this, testData);
	}
#endif

	return var();
}



bool DspNetwork::deleteIfUnused(String id)
{
	for (auto n : nodes)
	{
		if (n->getId() == id)
		{
			if (!isInSignalPath(n))
			{
				nodes.removeObject(n);
				return true;
			}
		}
	}

	return false;
}

bool DspNetwork::setParameterDataFromJSON(var jsonData)
{
	auto ok = false;

	if (auto dyn = jsonData.getDynamicObject())
	{
		for (auto n : dyn->getProperties())
		{
			auto nId = n.name.toString().upToFirstOccurrenceOf(".", false, false);
			auto pId = n.name.toString().fromFirstOccurrenceOf(".", false, false);

			auto value = (double)n.value;

			if (nId.isNotEmpty() && pId.isNotEmpty())
			{
				if (auto node = getNodeWithId(nId))
				{
					if (auto p = node->getParameterFromName(pId))
					{
						p->data.setProperty(PropertyIds::Value, value, getUndoManager());
						p->isProbed = true;
						ok = true;
					}
				}
			}
		}
	}

	return true;
}

juce::Array<Parameter*> DspNetwork::getListOfProbedParameters()
{
	Array<Parameter*> list;

	auto l = getListOfNodesWithType<NodeBase>(false);

	for (auto node : l)
	{
		for(auto p: NodeBase::ParameterIterator(*node))
		{
			if (p->isProbed)
				list.add(p);
		}
	}

	return list;
}

NodeBase* DspNetwork::createFromValueTree(bool createPolyIfAvailable, ValueTree d, bool forceCreate)
{
	auto id = d[PropertyIds::ID].toString();

	if (!isPolyphonic())
		createPolyIfAvailable = false;

	auto existing = forceCreate ? var() : get(id);

	if (existing.isObject())
		return dynamic_cast<NodeBase*>(existing.getObject());

	auto childNodes = d.getChildWithName(PropertyIds::Nodes);

	for (auto c : childNodes)
	{
		auto n = createFromValueTree(createPolyIfAvailable, c, forceCreate);

		if (currentNodeHolder != nullptr)
		{
			currentNodeHolder->nodes.addIfNotAlreadyThere(n);
		}
			
	}

	NodeBase::Ptr newNode;

	for (auto nf : nodeFactories)
	{
        if(nf == nullptr)
        {
            jassertfalse;
            continue;
        }
        
		nf->setNetworkToUse(this);
		newNode = nf->createNode(d, createPolyIfAvailable);

		if (newNode != nullptr)
		{
            try
            {
			    if(originalSampleRate > 0.0)
				    newNode->prepare(currentSpecs);
            }
            catch(Error& e)
            {
                getExceptionHandler().addError(newNode, e);
            }

			if (currentNodeHolder == nullptr)
			{
				StringArray sa;
				auto newId = getNonExistentId(id, sa);
				newNode->setValueTreeProperty(PropertyIds::ID, newId);
				nodes.add(newNode);
			}
			else
				currentNodeHolder->nodes.add(newNode);
			
			return newNode.get();
		}
	}

	return nullptr;
}



bool DspNetwork::isInSignalPath(NodeBase* b) const
{
	if (getRootNode() == nullptr)
		return false;

	if (b == nullptr)
		return false;

	if (b == getRootNode())
		return true;

	return b->getValueTree().isAChildOf(getRootNode()->getValueTree());
}


scriptnode::NodeBase* DspNetwork::getNodeWithId(const String& id) const
{
	return dynamic_cast<NodeBase*>(get(id).getObject());
}

void DspNetwork::checkIfDeprecated()
{
#if USE_BACKEND
	using namespace cppgen;

	ValueTreeIterator::forEach(getValueTree(), ValueTreeIterator::IterationType::Forward, [&](ValueTree v)
	{
		DeprecationChecker d(this, v);
		return d.notOk;
	});
#endif
}

Result DspNetwork::checkBeforeCompilation()
{
#if USE_BACKEND
	for (auto id : getListOfUsedNodeIds())
	{
		auto mustBeWrapped = NodeComponent::PopupHelpers::isWrappable(getNodeWithId(id)) == 2;

		if (mustBeWrapped)
			return Result::fail(id + " needs to be wrapped into a compileable DSP network");
	}

	if (projectNodeHolder.dll != nullptr)
	{
		auto dll = projectNodeHolder.dll;
		auto fileList = BackendDllManager::getNetworkFiles(getScriptProcessor()->getMainController_(), false);

		for (auto nId : fileList)
		{
			auto id = nId.getFileNameWithoutExtension();
			auto prHash = BackendDllManager::getHashForNetworkFile(getScriptProcessor()->getMainController_(), id);
			bool found = false;

			for (int i = 0; i < dll->getNumNodes(); i++)
			{
				if (dll->getNodeId(i) == id)
				{
					found = true;

					if (prHash != dll->getHash(i))
						return Result::fail(id + " hash mismatch");
				}
			}

			if (!found)
				return Result::fail(id + " is not compiled");
		}
	}
#endif

	return Result::ok();
}

void DspNetwork::addToSelection(NodeBase* node, ModifierKeys mods)
{
	auto pNode = node->getParentNode();

	while (mods.isAnyModifierKeyDown() && pNode != nullptr)
	{
		if (isSelected(pNode))
			return;

		pNode = pNode->getParentNode();
	}

    if(selection.getSelectedItem(0) == node && selection.getNumSelected() == 1)
        selection.deselectAll();
    else
        selection.addToSelectionBasedOnModifiers(node, mods);
}


void DspNetwork::zoomToSelection(Component* c)
{
    using ButtonBase = DspNetworkGraph::ActionButton;
    
    Component::callRecursive<ButtonBase>(c->getTopLevelComponent(), [](ButtonBase* b)
    {
        if(b->getName() == "foldunselected")
        {
            auto g = b->findParentComponentOfClass<WrapperWithMenuBarBase>()->canvas.getContent<DspNetworkGraph>();
            
            jassert(g != nullptr);
            
            b->actionFunction(*g);
            return true;
        }
        
        return false;
    });
}

bool DspNetwork::updateIdsInValueTree(ValueTree& v, StringArray& usedIds)
{
	auto oldId = v[PropertyIds::ID].toString();
	auto newId = getNonExistentId(oldId, usedIds);

	if (oldId != newId)
		v.setProperty(PropertyIds::ID, newId, getUndoManager());

	auto nodeTree = v.getChildWithName(PropertyIds::Nodes);

	for (auto n : nodeTree)
	{
		updateIdsInValueTree(n, usedIds);
	}

	return true;
}


juce::String DspNetwork::getNonExistentId(String id, StringArray& usedIds) const
{
	if (getRootNode() == nullptr)
	{
		usedIds.add(id);
		return id;
	}

	if (!get(id).isObject())
		return id;

	int trailingIndex = id.getTrailingIntValue();

	auto idWithoutNumber = trailingIndex == 0 ? id : id.upToLastOccurrenceOf(String(trailingIndex), false, false);

	trailingIndex++;

	id = idWithoutNumber + String(trailingIndex);

	var existingNode = get(id);

	while (existingNode.isObject() || usedIds.contains(id))
	{
		trailingIndex++;
		id = idWithoutNumber + String(trailingIndex);
		existingNode = get(id);
	}

	usedIds.add(id);

	return id;
}

juce::ValueTree DspNetwork::cloneValueTreeWithNewIds(const ValueTree& treeToClone)
{
	struct IdChange
	{
		String oldId;
		String newId;
	};

	auto c = treeToClone.createCopy();

	StringArray sa;
	for (auto n : nodes)
		sa.add(n->getId());

	auto saRef = &sa;

	Array<IdChange> changes;

	auto changeRef = &changes;

	auto setNewId = [changeRef, saRef, this](ValueTree& v)
	{
		if (v.hasType(PropertyIds::Node))
		{
			auto thisId = v[PropertyIds::ID].toString();

			if (this->get(thisId))
			{
				auto newId = this->getNonExistentId(thisId, *saRef);
				changeRef->add({ thisId, newId });
				v.setProperty(PropertyIds::ID, newId, nullptr);
			}
		}

		return false;
	};

	valuetree::Helpers::foreach(c, setNewId);

	for (auto& ch : changes)
		changeNodeId(c, ch.oldId, ch.newId, nullptr);

	return c;
}

void DspNetwork::changeNodeId(ValueTree& c, const String& oldId, const String& newId, UndoManager* undoManager)
{
	auto updateConnection = [oldId, newId, undoManager](ValueTree& v)
	{
		if (v.hasType(PropertyIds::Connection) ||
			v.hasType(PropertyIds::ModulationTarget) ||
			v.hasType(PropertyIds::SwitchTarget))
		{
			auto nId = v[PropertyIds::NodeId].toString();

			if (nId == oldId)
			{
				v.setProperty(PropertyIds::NodeId, newId, undoManager);
			}
		}

		return false;
	};

	valuetree::Helpers::foreach(c, updateConnection);

	auto updateSendConnection = [oldId, newId, undoManager](ValueTree& v)
	{
		if (v.hasType(PropertyIds::Property) &&
			v[PropertyIds::ID].toString() == PropertyIds::Connection.toString())
		{
			auto oldValue = v[PropertyIds::Value].toString();

			if (oldValue == oldId)
			{
				v.setProperty(PropertyIds::Value, newId, undoManager);
			}
		}

		return false;
	};

	valuetree::Helpers::foreach(c, updateSendConnection);
}

void DspNetwork::setUseFrozenNode(bool shouldBeEnabled)
{
	projectNodeHolder.setEnabled(shouldBeEnabled);
	reset();
}

bool DspNetwork::hashMatches()
{
	return projectNodeHolder.hashMatches;
}

void DspNetwork::setExternalData(const snex::ExternalData& d, int index)
{
	projectNodeHolder.n.setExternalData(d, index);
}

hise::ScriptParameterHandler* DspNetwork::getCurrentParameterHandler()
{
	if (projectNodeHolder.isActive())
		return &projectNodeHolder;
	else
		return &networkParameterHandler;
}

void DspNetwork::runPostInitFunctions()
{
	for (int i = 0; i < postInitFunctions.size(); i++)
	{
		if (postInitFunctions[i]())
			postInitFunctions.remove(i--);
	}
}

DspNetwork::Holder::Holder()
{
	JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

	SimpleRingBuffer::Ptr rb = new SimpleRingBuffer();

	rb->registerPropertyObject<scriptnode::OscillatorDisplayProvider::OscillatorDisplayObject>();
}

void DspNetwork::Holder::unload()
{
#if USE_BACKEND
    auto& manager = dynamic_cast<BackendProcessor*>(dynamic_cast<ControlledObject*>(this)->getMainController())->workbenches;

    manager.setCurrentWorkbench(nullptr, false);
    embeddedNetworks.clear();
    networks.clear();
    setActiveNetwork(nullptr);
#endif
}

DspNetwork* DspNetwork::Holder::getOrCreate(const String& id)
{
	auto asScriptProcessor = dynamic_cast<ProcessorWithScriptingContent*>(this);

	for (auto n : networks)
	{
		if (n->getId() == id)
		{
			setActiveNetwork(n);
			return n;
		}
	}

	ValueTree v(scriptnode::PropertyIds::Network);
	v.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	ValueTree s(scriptnode::PropertyIds::Node);
	s.setProperty(scriptnode::PropertyIds::FactoryPath, "container.chain", nullptr);
	s.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	v.addChild(s, -1, nullptr);

#if USE_BACKEND
	for (auto f : BackendDllManager::getNetworkFiles(asScriptProcessor->getMainController_()))
	{
		if (f.getFileNameWithoutExtension() == id)
		{
			auto xml = XmlDocument::parse(f);

			if(xml->getTagName() != "empty")
				v = ValueTree::fromXml(*xml);

			break;
		}
	}
#endif

	auto newNetwork = new DspNetwork(asScriptProcessor, v, isPolyphonic());

	if (vk != nullptr)
		newNetwork->setVoiceKiller(vk);

	networks.add(newNetwork);

	setActiveNetwork(newNetwork);

	return newNetwork;
}


scriptnode::DspNetwork* DspNetwork::Holder::getOrCreate(const ValueTree& v)
{
	jassert(v.getType() == PropertyIds::Network);

	auto id = v[PropertyIds::ID].toString();

	for (auto n : networks)
	{
		if (n->getId() == id)
			return n;
	}

	auto newNetwork = new DspNetwork(dynamic_cast<ProcessorWithScriptingContent*>(this), v, isPolyphonic());

	if (vk != nullptr)
		newNetwork->setVoiceKiller(vk);


	networks.add(newNetwork);
	setActiveNetwork(newNetwork);
	return newNetwork;
}

juce::StringArray DspNetwork::Holder::getIdList()
{
	StringArray sa;

	for (auto n : networks)
		sa.add(n->getId());

	return sa;
}

void DspNetwork::Holder::saveNetworks(ValueTree& d) const
{
	if (networks.size() > 0)
	{
		ValueTree v("Networks");

		for (auto n : networks)
		{
			auto c = n->getValueTree().createCopy();

#if USE_BACKEND

			auto embedResources = dynamic_cast<const ControlledObject*>(this)->getMainController()->shouldEmbedAllResources();

			if (embedResources)
			{
				// Include all SNEX files here

				// implement me...
				jassertfalse;
			}
			else
            {
				// look if there's a network file that we 
				// need to replace with this content
                cppgen::ValueTreeIterator::forEach(c,
                    snex::cppgen::ValueTreeIterator::IterationType::Forward,
                    DspNetworkListeners::PatchAutosaver::stripValueTree);

                auto f = BackendDllManager::getSubFolder(dynamic_cast<const ControlledObject*>(this)->getMainController(), BackendDllManager::FolderSubType::Networks);

                auto nf = f.getChildFile(c[PropertyIds::ID].toString()).withFileExtension("xml");

                if (nf.existsAsFile())
                {
                    auto xml = c.createXml();
                    nf.replaceWithText(xml->createDocument(""));

                    debugToConsole(dynamic_cast<Processor*>(const_cast<Holder*>(this)), "Save network to " + nf.getFileName() + " from project folder");

                    c.removeAllChildren(nullptr);
                }
            }
#endif

			v.addChild(c, -1, nullptr);
		}

		d.addChild(v, -1, nullptr);
	}
}

void DspNetwork::Holder::restoreNetworks(const ValueTree& d)
{
	auto v = d.getChildWithName("Networks");

	if (v.isValid())
	{
		clearAllNetworks();

		for (auto c : v)
		{
			if (c.getNumChildren() == 0)
			{
				auto nid = c[PropertyIds::ID].toString();
				auto mc = dynamic_cast<ControlledObject*>(this)->getMainController();

				auto fh = mc->getActiveFileHandler();

				c = fh->getEmbeddedNetwork(nid);
				jassert(c.isValid());
			}

			auto newNetwork = new DspNetwork(dynamic_cast<ProcessorWithScriptingContent*>(this),
				c.createCopy(), isPolyphonic());

			if (vk != nullptr)
				newNetwork->setVoiceKiller(vk);

			networks.add(newNetwork);
			setActiveNetwork(newNetwork);
		}
	}
}

scriptnode::NodeBase* NodeFactory::createNode(ValueTree data, bool createPolyIfAvailable) const
{
	auto path = data[PropertyIds::FactoryPath].toString();
	auto className = Identifier(path.upToFirstOccurrenceOf(".", false, false));

	if (className != getId())
		return nullptr;

	auto id = Identifier(path.fromFirstOccurrenceOf(".", false, false));

	if (createPolyIfAvailable)
	{
		for (const auto& item : polyNodes)
		{
			if (item.id == id)
			{
				auto newNode = item.cb(network.get(), data);

				return newNode;
			}
		}
	}

	for (const auto& item : monoNodes)
	{
		if (item.id == id)
		{
			auto newNode = item.cb(network.get(), data);

			return newNode;
		}
	}

	return nullptr;
}

DspNetwork::SelectionUpdater::SelectionUpdater(DspNetwork& parent_) :
	parent(parent_)
{
	WeakReference<DspNetwork> weakParent = &parent;

	auto f = [weakParent, this]()
	{
		if (weakParent != nullptr)
			weakParent.get()->selection.addChangeListener(this);
	};

	MessageManager::callAsync(f);

#if 0
	deleteChecker.setCallback(parent.data, valuetree::AsyncMode::Asynchronously,
		[this](ValueTree v, bool wasAdded)
	{
		if (!wasAdded)
		{
			if (auto nodeThatWasRemoved = parent.getNodeForValueTree(v))
			{
				parent.deselect(nodeThatWasRemoved);
			}
		}
	});

	deleteChecker.setTypeToWatch(PropertyIds::Nodes);
#endif
}

DspNetwork::SelectionUpdater::~SelectionUpdater()
{
	parent.selection.removeChangeListener(this);
}

void DspNetwork::SelectionUpdater::changeListenerCallback(ChangeBroadcaster*)
{
	auto& list = parent.selection.getItemArray();

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->selectionChanged(list);
	}
}


#if HISE_INCLUDE_SNEX
juce::File DspNetwork::CodeManager::getCodeFolder() const
{
	File f = parent.getScriptProcessor()->getMainController_()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::DspNetworks).getChildFile("CodeLibrary");
	f.createDirectory();
	return f;
}

DspNetwork::CodeManager::SnexSourceCompileHandler::SnexSourceCompileHandler(snex::ui::WorkbenchData* d, ProcessorWithScriptingContent* sp_) :
	Thread("SNEX Compile Thread"),
	CompileHandler(d),
	ControlledObject(sp_->getMainController_()),
	sp(sp_)
{

}

void DspNetwork::CodeManager::SnexSourceCompileHandler::run()
{
	if (runTestNext)
	{
        auto r = getParent()->getLastResult();
		postCompile(r);
		return;
	}

	getParent()->handleCompilation();
}

bool DspNetwork::CodeManager::SnexSourceCompileHandler::triggerCompilation()
{
	getParent()->getGlobalScope().getBreakpointHandler().abort();

	if (isThreadRunning())
		stopThread(3000);

	auto currentThread = getMainController()->getKillStateHandler().getCurrentThread();

	if (currentThread == MainController::KillStateHandler::SampleLoadingThread ||
		currentThread == MainController::KillStateHandler::ScriptingThread)
	{
		getParent()->handleCompilation();
		return true;
	}

	startThread();
	return false;
}
#endif

DeprecationChecker::DeprecationChecker(DspNetwork* n_, ValueTree v_) :
	n(n_),
	v(v_)
{
	v.removeProperty("LockNumChannels", nullptr);
	v.removeProperty("CommentWidth", nullptr);
	v.removeProperty("Public", nullptr);
	v.removeProperty("BypassRampTimeMs", nullptr);

	auto isConnection = v.getType() == PropertyIds::Connection ||
		v.getType() == PropertyIds::ModulationTarget;

	if (isConnection)
	{
		throwIf(DeprecationId::ConverterNotIdentity);
		throwIf(DeprecationId::OpTypeNonSet);
	}
}



String DeprecationChecker::getErrorMessage(int id)
{
	switch ((DeprecationId)id)
	{
	case DeprecationId::ConverterNotIdentity:
		return "The Converter is not identity\n(use the control.xfader instead)";
	case DeprecationId::OpTypeNonSet:
		return "The OpType is not SetValue\n(use control.pma instead)";
	default:
		break;
	}

	return {};
}

void DeprecationChecker::throwIf(DeprecationId id)
{
#if USE_BACKEND
	if (notOk)
		return;

	if (!check(id))
	{
		Error e;
		e.error = Error::ErrorCode::DeprecatedNode;
		e.actual = (int)id;

		auto nodeTree = cppgen::ValueTreeIterator::findParentWithType(v, PropertyIds::Node);
		auto nId = nodeTree[PropertyIds::ID].toString();

		if (auto node = n->getNodeWithId(nId))
		{
			n->getExceptionHandler().addError(node, e);
		}

		notOk = true;
	}
#endif
}

bool DeprecationChecker::check(DeprecationId id)
{
	
	switch (id)
	{
	case DeprecationId::OpTypeNonSet:
		return !v.hasProperty("OpType") || v["OpType"] == var("SetValue");
	case DeprecationId::ConverterNotIdentity:
		return !v.hasProperty("Converter") || v["Converter"] == var("Identity");
    default: return false;
	}

	return false;
}

DspNetwork::AnonymousNodeCloner::AnonymousNodeCloner(DspNetwork& p, NodeBase::Holder* other):
	parent(p)
{
	if (other == &parent)
		other = nullptr;

	prevHolder = p.currentNodeHolder;
	p.currentNodeHolder = other;
}

DspNetwork::AnonymousNodeCloner::~AnonymousNodeCloner()
{
	parent.currentNodeHolder = prevHolder.get();
}

scriptnode::NodeBase::Ptr DspNetwork::AnonymousNodeCloner::clone(NodeBase::Ptr p)
{
	return parent.createFromValueTree(parent.isPolyphonic(), p->getValueTree(), false);
}

DspNetwork::ProjectNodeHolder::~ProjectNodeHolder()
{
	if (loaded && dll != nullptr)
	{
		dll->deInitNode(&n);
	}
}

void DspNetwork::ProjectNodeHolder::process(ProcessDataDyn& data)
{
	NodeProfiler np(network.getRootNode(), data.getNumSamples());

	n.process(data);
}

void DspNetwork::ProjectNodeHolder::init(dll::ProjectDll::Ptr dllToUse)
{
	dll = dllToUse;

	int getNumNodes = dll->getNumNodes();

	for (int i = 0; i < getNumNodes; i++)
	{
		auto dllId = dll->getNodeId(i);

		if (dllId == network.getId())
		{
			dll->initNode(&n, i, network.isPolyphonic());
			loaded = true;
		}
	}

#if USE_BACKEND
	if (network.data[PropertyIds::AllowCompilation])
	{
		auto fileHash = BackendDllManager::getHashForNetworkFile(network.getScriptProcessor()->getMainController_(), network.getId());

		if (dll != nullptr)
		{
			auto numNodes = dll->getNumNodes();

			for (int i = 0; i < numNodes; i++)
			{
				auto nid = dll->getNodeId(i);

				if (nid == network.getId())
				{
					auto dllHash = dll->getHash(i);

					hashMatches = dllHash == fileHash;
					return;
				}
			}
		}
	}
#endif
}

int HostHelpers::getNumMaxDataObjects(const ValueTree& v, snex::ExternalData::DataType t)
{
	auto id = Identifier(snex::ExternalData::getDataTypeName(t, false));

	int numMax = 0;

#if USE_BACKEND
	snex::cppgen::ValueTreeIterator::forEach(v, snex::cppgen::ValueTreeIterator::Forward, [&](ValueTree& v)
	{
		if (v.getType() == id)
			numMax = jmax(numMax, (int)v[PropertyIds::Index] + 1);

		return false;
	});
#endif

	return numMax;
}

void HostHelpers::setNumDataObjectsFromValueTree(OpaqueNode& on, const ValueTree& v)
{
	snex::ExternalData::forEachType([&](snex::ExternalData::DataType dt)
	{
		auto numMaxDataObjects = getNumMaxDataObjects(v, dt);
		on.numDataObjects[(int)dt] = numMaxDataObjects;
	});
}

void OpaqueNetworkHolder::createParameters(ParameterDataList& l)
{
	if (ownedNetwork != nullptr)
	{
		auto pTree = ownedNetwork->getRootNode()->getValueTree().getChildWithName(PropertyIds::Parameters);

		for (auto c : pTree)
		{
			parameter::data p;
			p.info = parameter::pod(c);
			setCallback(p, pTree.indexOf(c));
			l.add(std::move(p));
		}
	}
}

void OpaqueNetworkHolder::setCallback(parameter::data& d, int index)
{
	if (index == 0) d.callback = parameter::inner<OpaqueNetworkHolder, 0>(*this);
	if (index == 1) d.callback = parameter::inner<OpaqueNetworkHolder, 1>(*this);
	if (index == 2) d.callback = parameter::inner<OpaqueNetworkHolder, 2>(*this);
	if (index == 3) d.callback = parameter::inner<OpaqueNetworkHolder, 3>(*this);
	if (index == 4) d.callback = parameter::inner<OpaqueNetworkHolder, 4>(*this);
	if (index == 5) d.callback = parameter::inner<OpaqueNetworkHolder, 5>(*this);
	if (index == 6) d.callback = parameter::inner<OpaqueNetworkHolder, 6>(*this);
	if (index == 7) d.callback = parameter::inner<OpaqueNetworkHolder, 7>(*this);
	if (index == 8) d.callback = parameter::inner<OpaqueNetworkHolder, 8>(*this);
	if (index == 9) d.callback = parameter::inner<OpaqueNetworkHolder, 9>(*this);
	if (index == 10) d.callback = parameter::inner<OpaqueNetworkHolder, 10>(*this);
	if (index == 11) d.callback = parameter::inner<OpaqueNetworkHolder, 11>(*this);
	if (index == 12) d.callback = parameter::inner<OpaqueNetworkHolder, 12>(*this);
}

void OpaqueNetworkHolder::setNetwork(DspNetwork* n)
{
	ownedNetwork = n;

	for (const auto& d : deferredData)
	{
		if (d.d.obj != nullptr)
		{
			SimpleReadWriteLock::ScopedWriteLock sl(d.d.obj->getDataLock());
			ownedNetwork->setExternalData(d.d, d.index);
		}
	}
}

void OpaqueNetworkHolder::setExternalData(const ExternalData& d, int index)
{
	if (ownedNetwork != nullptr)
		ownedNetwork->setExternalData(d, index);
	else
		deferredData.add({ d, index });
}

void ScriptnodeExceptionHandler::validateMidiProcessingContext(NodeBase* b)
{
	if (b != nullptr)
	{
		auto pp = b->getParentNode();
		auto isInMidiChain = b->getRootNetwork()->isPolyphonic();

		while (pp != nullptr && !isInMidiChain)
		{
			isInMidiChain |= pp->getValueTree()[PropertyIds::FactoryPath].toString().contains("midichain");
			pp = pp->getParentNode();
		}

		if (!isInMidiChain)
		{
			Error e;
			e.error = Error::NoMatchingParent;
			throw e;
		}
	}
}

#if USE_BACKEND

ScriptNetworkTest::ScriptNetworkTest(DspNetwork* n, var testData) :
	ConstScriptingObject(n->getScriptProcessor(), 0),
	wb(new snex::ui::WorkbenchData())
{
	wb->setCompileHandler(new CHandler(wb, n));

	cProv = new CProvider(wb, n);

	wb->setCodeProvider(cProv);

	wb->getTestData().fromJSON(testData, dontSendNotification);

	ADD_API_METHOD_0(runTest);
	ADD_API_METHOD_2(setTestProperty);
	ADD_API_METHOD_3(setProcessSpecs);
	ADD_API_METHOD_3(expectEquals);
	ADD_API_METHOD_0(dumpNetworkAsXml);
	ADD_API_METHOD_1(setWaitingTime);
    ADD_API_METHOD_0(getLastTestException);
	ADD_API_METHOD_2(createBufferContentAsAsciiArt);
}

String ScriptNetworkTest::getLastTestException() const
{
    if(auto b = dynamic_cast<CHandler*>(wb->getCompileHandler()))
    {
        return b->lastTestResult.getErrorMessage();
    }
    
    jassertfalse;
    return {};
}

juce::var ScriptNetworkTest::runTest()
{
	wb->triggerRecompile();
	auto& td = wb->getTestData();
    
    auto numChannels = td.testOutputData.getNumChannels();
    
    if(numChannels == 1)
    {
    
	auto v = new VariantBuffer(td.testOutputData.getNumSamples());
	FloatVectorOperations::copy(v->buffer.getWritePointer(0), td.testOutputData.getReadPointer(0), v->size);
	return var(v);
    }
    else
    {
        Array<var> channels;
        
        for(int i = 0; i < numChannels; i++)
        {
            auto v = new VariantBuffer(td.testOutputData.getNumSamples());
            FloatVectorOperations::copy(v->buffer.getWritePointer(0), td.testOutputData.getReadPointer(i), v->size);
            
            channels.add(var(v));
        }
        
        return var(channels);
    }
}

String ScriptNetworkTest::dumpNetworkAsXml()
{
    auto v = dynamic_cast<CHandler*>(wb->getCompileHandler())->network->getValueTree();
    auto copy = v.createCopy();
    
#if USE_BACKEND
    DspNetworkListeners::PatchAutosaver::stripValueTree(copy);
#endif
    
    auto xml = copy.createXml();
    
    return xml->createDocument("");
}

String printBufferSlice(VariantBuffer::Ptr b, int i, int numSamplesPerLine, int numCols)
{
	String s;

	int numThisTime = jmin(numSamplesPerLine, b->size - i);

	auto magRange = b->buffer.findMinMax(0, i, numThisTime);

	auto absStart = hmath::abs(magRange.getStart());
	auto absEnd = hmath::abs(magRange.getEnd());

	float mag;

	if (absStart > absEnd)
		mag = magRange.getStart();
	if (absStart < absEnd)
		mag = magRange.getEnd();
	if (absStart == absEnd)
		mag = magRange.getStart() + magRange.getLength() * 0.5f;

	mag = jlimit(-1.0f, 1.0f, mag);

	for (int c = 0; c < numCols; c++)
	{
		float cValue = ((float)c / (float)(numCols - 1)) * 2.0f - 1.0f;

		Range<float> cRange(cValue - (1.0f / (float)(numCols - 1)), cValue + (1.0f / (float)(numCols - 1)));

		juce_wchar nc;

		if (mag == 0.0f)
		{
			nc = cRange.contains(0.0f) ? 'X' : ' ';
		}
		else if (mag < 0.0f)
		{
			if (cRange.contains(mag))
				nc = 'X';
			else if (cValue < mag)
				nc = ' ';
			else
			{
				if (cRange.contains(0.0f))
					nc = 'o';
				else if (cValue < 0.0f)
					nc = '-';
				else
					nc = ' ';
			}
		}
		else
		{
			if (cRange.contains(mag))
				nc = 'X';
			else if (cRange.contains(0.0f))
				nc = 'o';
			else if (cValue < 0.0f)
				nc = ' ';
			else
				nc = cValue > mag ? ' ' : '-';
		}

		s << nc;
	}

	return s;
}

String ScriptNetworkTest::createBufferContentAsAsciiArt(var buffer, int numLines)
{
	if (numLines == 0)
	{
		reportScriptError("numLines must not be zero");
		RETURN_IF_NO_THROW({});
	}

	String s;

	VariantBuffer::Ptr b1, b2;

	int numSamples = 0;

	if (auto b = buffer.getBuffer())
	{
		b1 = b;
		numSamples = b1->size;
	}
		

	if (auto ar = buffer.getArray())
	{
		b1 = buffer[0].getBuffer();
		b2 = buffer[1].getBuffer();

		if (b1 == nullptr || b2 == nullptr)
		{
			reportScriptError("not a stereo channel array");
			RETURN_IF_NO_THROW({});
		}

		if (b1->size != b2->size)
		{
			reportScriptError("Buffer size mismatch");
			RETURN_IF_NO_THROW({});
		}

		numSamples = b1->size;
	}

	if (b1 != nullptr)
	{
		s << "\n";
		auto numSamplesPerLine = numSamples / numLines;
		int numCols = b2 == nullptr ? 40 : 30;

		snex::Types::span<Range<int>, 5> eqRanges;

		eqRanges[0] = Range<int>(0, numSamplesPerLine / 2);
		eqRanges[1] = Range<int>(numSamples / 4 - numSamplesPerLine / 2, numSamples / 4 + numSamplesPerLine / 2);
		eqRanges[2] = Range<int>(numSamples / 2 - numSamplesPerLine / 2, numSamples / 2 + numSamplesPerLine / 2);
		eqRanges[3] = Range<int>(3 * numSamples / 4 - numSamplesPerLine / 2, 3 * numSamples / 4 + numSamplesPerLine / 2);
		eqRanges[4] = Range<int>(numSamples - numSamplesPerLine / 2, numSamples + numSamplesPerLine / 2);

		for (int i = 0; i < numSamples; i += numSamplesPerLine)
		{
			auto lineChar = '|';

			for (const auto& r : eqRanges)
			{
				if (r.contains(i))
				{
					lineChar = '=';
					break;
				}
			}
			
			s << "\n" << lineChar;
			s << printBufferSlice(b1, i, numSamplesPerLine, numCols);

			if (b2 != nullptr)
				s << lineChar << printBufferSlice(b2, i, numSamplesPerLine, numCols);

			s << lineChar;
		}
	}

	return s;
}

void ScriptNetworkTest::setTestProperty(String id, var value)
{
	auto v = wb->getTestData().toJSON();

	if (auto obj = v.getDynamicObject())
		obj->setProperty(Identifier(id), value);

	wb->getTestData().fromJSON(v, dontSendNotification);
}

void ScriptNetworkTest::setProcessSpecs(int numChannels, double sampleRate, int blockSize)
{
	auto c = dynamic_cast<CHandler*>(wb->getCompileHandler());
	c->ps.numChannels = numChannels;
	c->ps.blockSize = blockSize;
	c->ps.sampleRate = sampleRate;
}

juce::var ScriptNetworkTest::expectEquals(var data1, var data2, float errorDb)
{
	auto isNumeric = [](const var& v)
	{
		return v.isInt() || v.isDouble() || v.isInt64() || v.isBool();
	};

	if (data1.isArray() && (data2.isArray() || isNumeric(data2)))
	{
		auto size1 = data1.size();
		auto size2 = isNumeric(data2) ? data2.size() : size1;

		if (size1 != size2)
			return var("Array size mismatch");

		for (int i = 0; i < size1; i++)
		{
			auto v = isNumeric(data2) ? data2 : data2[i];
			auto ok = expectEquals(data1[i], v, errorDb);

			if (ok.isString())
				return ok;
		}

		return 0;
	}
	if (data1.isBuffer() && (data2.isBuffer() || isNumeric(data2)))
	{
		auto p1 = data1.getBuffer()->buffer.getReadPointer(0);
		auto p2 = !isNumeric(data2) ? data2.getBuffer()->buffer.getReadPointer(0) : nullptr;
		auto size1 = data1.getBuffer()->size;
		auto size2 = !isNumeric(data2) ? data2.getBuffer()->size : size1;

		if (size1 != size2)
			return var("Buffer size mismatch");

		for (int i = 0; i < size1; i++)
		{
			float v = p2 != nullptr ? p2[i] : (float)data2;
			auto result = expectEquals(p1[i], v, errorDb);

			if (result.isString())
				return result;
		}

		return 0;
	}
	if (isNumeric(data1) && isNumeric(data2))
	{
		auto v1 = (float)data1;
		auto v2 = (float)data2;

		auto diff = hmath::abs(v1 - v2);

		if (diff > Decibels::decibelsToGain(errorDb))
		{
			String error;
			error << "Value error: " << String(Decibels::gainToDecibels(diff), 1) << " dB";
			return var(error);
		}

		return 0;
	}
	
	return var("unsupported type");
}

void ScriptNetworkTest::setWaitingTime(int timeToWaitMs)
{
	dynamic_cast<CHandler*>(wb->getCompileHandler())->waitTimeMs = jlimit(0, 3000, timeToWaitMs);
}

ScriptNetworkTest::CHandler::CHandler(WorkbenchData::Ptr wb, DspNetwork* n) :
	ScriptnodeCompileHandlerBase(wb.get(), n)
{
	ps.voiceIndex = n->getPolyHandler();
	ps.numChannels = wb->getTestData().testSourceData.getNumChannels();
	ps.blockSize = 512;
	ps.numChannels = 1;
	ps.sampleRate = 44100.0;
}

Result ScriptNetworkTest::CHandler::prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters)
{
	lastTestResult = ScriptnodeCompileHandlerBase::prepareTest(ps, initialParameters);

	if (waitTimeMs > 0)
	{
		HiseJavascriptEngine::TimeoutExtender te(dynamic_cast<JavascriptProcessor*>(network->getScriptProcessor())->getScriptEngine());
		Thread::getCurrentThread()->wait(waitTimeMs);
	}

	return lastTestResult;
}

ScriptnodeCompileHandlerBase::ScriptnodeCompileHandlerBase(WorkbenchData* d, DspNetwork* network_) :
	CompileHandler(d),
	network(network_),
    lastTestResult(Result::ok())
{

}

snex::ui::WorkbenchData::CompileResult ScriptnodeCompileHandlerBase::compile(const String& codeToCompile)
{
	return {};
}

void ScriptnodeCompileHandlerBase::processTestParameterEvent(int parameterIndex, double value)
{
	SimpleReadWriteLock::ScopedReadLock sl(network->getConnectionLock());
	network->getCurrentParameterHandler()->setParameter(parameterIndex, value);
}

Result ScriptnodeCompileHandlerBase::prepareTest(PrepareSpecs ps, const Array<ParameterEvent>& initialParameters)
{
    network->getExceptionHandler().removeError(nullptr);
    
	network->setNumChannels(ps.numChannels);
	network->prepareToPlay(ps.sampleRate, ps.blockSize);

	for (auto pe : initialParameters)
	{
		if(pe.timeStamp == 0)
			processTestParameterEvent(pe.parameterIndex, pe.valueToUse);
	}
	
    if(!network->getExceptionHandler().isOk())
        return Result::fail(network->getExceptionHandler().getErrorMessage(nullptr));
    
	network->reset();
    
    return Result::ok();
}

void ScriptnodeCompileHandlerBase::initExternalData(ExternalDataHolder* h)
{

}

void ScriptnodeCompileHandlerBase::postCompile(ui::WorkbenchData::CompileResult& lastResult)
{
	lastTestResult = runTest(lastResult);
}

Result ScriptnodeCompileHandlerBase::runTest(ui::WorkbenchData::CompileResult& lastResult)
{
	auto& td = getParent()->getTestData();

	auto cs = getPrepareSpecs();

	if (cs.sampleRate <= 0.0 || cs.blockSize == 0)
	{
		cs.sampleRate = 44100.0;
		cs.blockSize = 512;
	}

	td.initProcessing(cs);
    
	return td.processTestData(getParent());
}

void ScriptnodeCompileHandlerBase::processTest(ProcessDataDyn& data)
{
	network->process(data);
}

bool ScriptnodeCompileHandlerBase::shouldProcessEventsManually() const
{
	return false;
}

void ScriptnodeCompileHandlerBase::processHiseEvent(HiseEvent& e)
{
	jassertfalse;
}


#endif

}


