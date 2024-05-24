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
	faustManager(*this),
#if HISE_INCLUDE_SNEX
	codeManager(*this),
#endif
	parentHolder(dynamic_cast<Holder*>(p)),
	projectNodeHolder(*this)
{
	jassert(data.getType() == PropertyIds::Network);

	tempoSyncer.publicModValue = &networkModValue;

	auto mc_ = p->getMainController_();

	addPostInitFunction([mc_, this]()
	{
		if(auto rm = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(mc_->getGlobalRoutingManager()))
		{
			tempoSyncer.additionalEventStorage = &rm->additionalEventStorage;
			return true;
		}
		
		return false;
	});
	
	polyHandler.setTempoSyncer(&tempoSyncer);
	getScriptProcessor()->getMainController_()->addTempoListener(&tempoSyncer);

	if(!data.hasProperty(PropertyIds::AllowCompilation))
		data.setProperty(PropertyIds::AllowCompilation, false, nullptr);

	if(!data.hasProperty(PropertyIds::AllowPolyphonic))
		data.setProperty(PropertyIds::AllowPolyphonic, isPoly, nullptr);

    if(!data.hasProperty(PropertyIds::CompileChannelAmount))
        data.setProperty(PropertyIds::CompileChannelAmount, 2, nullptr);
    
	if (!data.hasProperty(PropertyIds::HasTail))
		data.setProperty(PropertyIds::HasTail, true, nullptr);

	if(!data.hasProperty(PropertyIds::SuspendOnSilence))
		data.setProperty(PropertyIds::SuspendOnSilence, false, nullptr);

	hasTailProperty.referTo(data, PropertyIds::HasTail, &um, true);
	canBeSuspendedProperty.referTo(data, PropertyIds::SuspendOnSilence, &um, false);

	if (!data.hasProperty(ExpansionIds::Version))
	{
		data.setProperty(ExpansionIds::Version, "0.0.0", nullptr);
	}

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

	ownedFactories.add(new TemplateNodeFactory(this));

#else
	if (auto ah = dynamic_cast<Holder*>(p))
	{
		auto nf = new hise::FrontendHostFactory(this);

		ownedFactories.add(nf);

		String selfId = "project." + getId();

		if (nf->getModuleList().contains(selfId))
		{
			// This network is supposed to be frozen
			projectNodeHolder.init(nf->staticFactory.get());
			projectNodeHolder.setEnabled(true);
		}
	}
#endif

	for (auto nf : ownedFactories)
		nodeFactories.add(nf);

	loader = new DspFactory::LibraryLoader(dynamic_cast<Processor*>(p));

	setRootNode(createFromValueTree(true, data.getChild(0), true));
	networkParameterHandler.root = getRootNode();

	initialId = getId();

	idGuard.setCallback(getRootNode()->getValueTree(), { PropertyIds::ID }, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(DspNetwork::checkId));

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
	stopTimer();

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
		auto isProjectFactory = f->getId() == Identifier("project");
            
		if (isProjectFactory)
			continue;

		MainController::ScopedBadBabysitter sb(getScriptProcessor()->getMainController_());

		for (auto id : f->getModuleList())
		{
			ScopedPointer<NodeBase::Holder> s = new NodeBase::Holder();

			currentNodeHolder = s;
			create(id, "unused");
			exceptionHandler.removeError(nullptr);
			currentNodeHolder = nullptr;

			s = nullptr;
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
            
            auto isPoly = ValueTreeIterator::forEach(v, [](ValueTree& v)
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

var DspNetwork::getAssignedValue(int index) const
{
	return var(nodes[index].get());
}

int DspNetwork::getCachedIndex(const var& indexExpression) const
{
	if (indexExpression.isString())
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			if (nodes[i]->getId() == indexExpression.toString())
				return i;
		}
	}

	return (int)indexExpression;
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

NodeBase::List DspNetwork::getListOfNodesWithPath(const NamespacedIdentifier& id, bool includeUnusedNodes)
{
	NodeBase::List list;

	for (auto n : nodes)
	{
		auto path = n->getPath();

		if ((includeUnusedNodes || isInSignalPath(n)) && path == id)
			list.add(n);
	}

	return list;
}

void DspNetwork::reset()
{
	SimpleReadWriteLock::ScopedWriteLock sl(getConnectionLock());
	
	if (projectNodeHolder.isActive())
		projectNodeHolder.n.reset();
	else if (auto rn = getRootNode())
		rn->reset();
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
    TRACE_DSP();
    
    if(!isInitialised())
        return;
    
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

bool DspNetwork::hasTail() const
{
	return hasTailProperty.get();
}

bool DspNetwork::isSuspendedOnSilence() const
{
	return canBeSuspendedProperty.get();
}

bool DspNetwork::handleModulation(double& v)
{
	if (isFrozen())
		return projectNodeHolder.handleModulation(v);
	else
		return networkModValue.getChangedValue(v);
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

				runPostInitFunctions();

				getRootNode()->reset();

				if (projectNodeHolder.isActive())
					projectNodeHolder.prepare(currentSpecs);
			}
            
            initialised = true;
		}
		catch (String& )
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

		if (n == nullptr)
		{
			String errorMessage;
			errorMessage << "Error at node `" << id << "`:  \n> ";
			errorMessage << "Can't create node with factory path `" << d[PropertyIds::FactoryPath].toString() << "`";

			if (MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
			{
				PresetHandler::showMessageWindow("Error", errorMessage, PresetHandler::IconType::Error);
			}
		}

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

	valuetree::Helpers::forEach(getValueTree(), [&](ValueTree v)
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
			return Result::fail(id + " needs to be wrapped into a compileable DSP network.  \n> If you've already compiled the network try using a Hardcoded module instead of `" + dynamic_cast<Processor*>(getScriptProcessor())->getId() + "` to avoid the scriptnode module overhead.");
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

void DspNetwork::deselect(NodeBase* node)
{
	selection.deselect(node);
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

void DspNetwork::fillSnexObjects(StringArray& indexList)
{
#if USE_BACKEND
	for (auto so : getSnexObjects())
	{
		indexList.add("SNEX Node: " + so->getId());
	}
#endif
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

void DspNetwork::checkId(const Identifier& id, const var& newValue)
{
	auto newId = newValue.toString();

	auto ok = newId == initialId;

	if (ok)
		getExceptionHandler().removeError(getRootNode(), scriptnode::Error::RootIdMismatch);
	else
	{
		Error e;
		e.error = Error::RootIdMismatch;
		getExceptionHandler().addError(getRootNode(), e, "ID mismatch between DSP network file and root container.  \n> Rename the root container back to `" + initialId + "` in order to clear this error.");
	}
}

juce::ValueTree DspNetwork::cloneValueTreeWithNewIds(const ValueTree& treeToClone, Array<IdChange>& changes, bool changeIds)
{
	auto c = treeToClone.createCopy();

	valuetree::Helpers::forEach(c, [](ValueTree& v)
	{
		if (v[PropertyIds::Automated])
			v.removeProperty(PropertyIds::Automated, nullptr);

		return false;
	});

	StringArray sa;
	for (auto n : nodes)
		sa.add(n->getId());

	// Also add the new IDs from the changes
	for(const auto& c: changes)
		sa.add(c.newId);

	auto saRef = &sa;
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

	valuetree::Helpers::forEach(c, setNewId);

    if(changeIds)
    {
        for (auto& ch : changes)
            changeNodeId(c, ch.oldId, ch.newId, nullptr);
    }
	

	return c;
}

void DspNetwork::setEnableUndoManager(bool shouldBeEnabled)
{
	enableUndo = shouldBeEnabled;
	if (enableUndo)
	{
		startTimer(1500);
	}
	else
		stopTimer();
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

	valuetree::Helpers::forEach(c, updateConnection);

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

	valuetree::Helpers::forEach(c, updateSendConnection);
}

UndoManager* DspNetwork::getUndoManager(bool returnIfPending)
{ 
	if (!enableUndo)
		return nullptr;

	if (!returnIfPending && um.isPerformingUndoRedo())
		return nullptr;
	else
		return &um;
}

void DspNetwork::setUseFrozenNode(bool shouldBeEnabled)
{
	if (projectNodeHolder.isActive() == shouldBeEnabled)
		return;

	if (shouldBeEnabled && currentSpecs)
		projectNodeHolder.prepare(currentSpecs);

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

PolyHandler* DspNetwork::getPolyHandler()
{
	if (auto pn = getParentNetwork())
		return pn->getPolyHandler();

	return &polyHandler;
}

const PolyHandler* DspNetwork::getPolyHandler() const
{
	if (auto pn = getParentNetwork())
		return pn->getPolyHandler();

	return &polyHandler;
}

void DspNetwork::runPostInitFunctions()
{
	for (int i = 0; i < postInitFunctions.size(); i++)
	{
		if (postInitFunctions[i]())
			postInitFunctions.remove(i--);
	}
}

void DspNetwork::initKeyPresses(Component* root)
{
	using namespace ScriptnodeShortcuts;
	String cat = "Scriptnode";

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_deselect_all, "Deselect all nodes", KeyPress(KeyPress::escapeKey));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_duplicate, "Duplicate nodes", KeyPress('d', ModifierKeys::commandModifier, 'd'));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_new_node, "Create Node", KeyPress('n'));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_fold, "Create Node", KeyPress('f'));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_add_bookmark, "Add selection bookmark", KeyPress(KeyPress::F11Key, ModifierKeys::commandModifier, 0));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_zoom_reset, "Show all nodes", KeyPress(KeyPress::F11Key, ModifierKeys::shiftModifier, 0));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_zoom_fit, "Fold unselected nodes", KeyPress(KeyPress::F11Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_edit_property, "Edit Node properties", KeyPress('p'));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_toggle_bypass, "Toggle Bypass", KeyPress('q'));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, sn_toggle_cables, "Show cables", KeyPress('c'));
}

DspNetwork::Holder::Holder()
{
	JUCE_ASSERT_MESSAGE_MANAGER_EXISTS;

	SimpleRingBuffer::Ptr rb = new SimpleRingBuffer();

	rb->registerPropertyObject<scriptnode::OscillatorDisplayProvider::OscillatorDisplayObject>();
}

DspNetwork::Holder::~Holder()
{}

bool DspNetwork::Holder::isPolyphonic() const
{ return false; }

void DspNetwork::Holder::clearAllNetworks()
{
	ReferenceCountedArray<DspNetwork> oldNetworks;

	{
		SimpleReadWriteLock::ScopedWriteLock l(getNetworkLock());
		std::swap(networks, oldNetworks);
		networks.clear();
		activeNetwork = nullptr;
	}
}

void DspNetwork::Holder::setActiveNetwork(DspNetwork* n)
{
	SimpleReadWriteLock::ScopedWriteLock l(getNetworkLock());
	activeNetwork = n;

	if(auto p = dynamic_cast<Processor*>(this))
		p->updateParameterSlots();
}

ScriptParameterHandler* DspNetwork::Holder::getCurrentNetworkParameterHandler(
	const ScriptParameterHandler* contentHandler) const
{
	if (auto n = getActiveOrDebuggedNetwork())
	{
		if (n->isForwardingControlsToParameters())
		{
			if(n->projectNodeHolder.isActive())
				return const_cast<ScriptParameterHandler*>(static_cast<const ScriptParameterHandler*>(&n->projectNodeHolder));
			else
				return const_cast<ScriptParameterHandler*>(static_cast<const ScriptParameterHandler*>(&n->networkParameterHandler));
					
		}
	}

	return const_cast<ScriptParameterHandler*>(contentHandler);
}

DspNetwork* DspNetwork::Holder::getActiveOrDebuggedNetwork() const
{
	if(activeNetwork.get() != nullptr)
		return activeNetwork;
            
	if(debuggedNetwork != nullptr)
		return debuggedNetwork;
            
	return nullptr;
}

DspNetwork* DspNetwork::Holder::getActiveNetwork() const
{
	return activeNetwork.get();
}

void DspNetwork::Holder::setProjectDll(dll::ProjectDll::Ptr pdll)
{
	projectDll = pdll;
}

ExternalDataHolder* DspNetwork::Holder::getExternalDataHolder()
{
	return dataHolder;
}

void DspNetwork::Holder::setExternalDataHolderToUse(ExternalDataHolder* newHolder)
{
	dataHolder = newHolder;
}

void DspNetwork::Holder::setVoiceKillerToUse(snex::Types::VoiceResetter* vk_)
{
	if (isPolyphonic())
	{
		vk = vk_;

		if (getActiveNetwork())
			getActiveNetwork()->setVoiceKiller(vk);
	}
}

SimpleReadWriteLock& DspNetwork::Holder::getNetworkLock()
{ return connectLock; }

DspNetwork* DspNetwork::Holder::addEmbeddedNetwork(DspNetwork* parent, const ValueTree& v,
	ExternalDataHolder* holderToUse)
{
	auto n = new DspNetwork(parent->getScriptProcessor(), v, parent->isPolyphonic(), holderToUse);
	embeddedNetworks.add(n);
	n->setParentNetwork(parent);
	return n;
}

DspNetwork* DspNetwork::Holder::getDebuggedNetwork()
{ return debuggedNetwork.get(); }

const DspNetwork* DspNetwork::Holder::getDebuggedNetwork() const
{ return debuggedNetwork.get(); }

void DspNetwork::Holder::toggleDebug()
{
	SimpleReadWriteLock::ScopedWriteLock l(getNetworkLock());

	std::swap(debuggedNetwork, activeNetwork);
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
				auto mc = dynamic_cast<const ControlledObject*>(this)->getMainController();
				auto snexRootFolder = BackendDllManager::getSubFolder(mc, BackendDllManager::FolderSubType::CodeLibrary);

				valuetree::Helpers::forEach(c, [snexRootFolder, mc](ValueTree& v)
				{
					if(v.getType() == PropertyIds::Node)
					{
						auto props = v.getChildWithName(PropertyIds::Properties);

						auto ct = props.getChildWithProperty(PropertyIds::ID, PropertyIds::ClassId.toString());

						if(ct.isValid())
						{
							auto fileName = ct[PropertyIds::Value].toString();
							auto rootDir = v[PropertyIds::FactoryPath].toString().fromFirstOccurrenceOf(".", false, false);

							auto extension = rootDir == "faust" ? ".dsp" : ".h";

							auto f = snexRootFolder.getChildFile(rootDir).getChildFile(fileName).withFileExtension(extension);
							jassert(f.existsAsFile());

							auto path = f.getRelativePathFrom(snexRootFolder);
							auto content = f.loadFileAsString();

							// just add it so it will be embedded into the snippet
							const_cast<MainController*>(mc)->getExternalScriptFile(f, true);

							auto pfile = f.withFileExtension(".xml");

							if(pfile.existsAsFile())
							{
								// add the parameter metadata file too...
								const_cast<MainController*>(mc)->getExternalScriptFile(pfile, true);
							}

						}
					}
					return false;
				});
			}
			else
            {
				DspNetworkListeners::PatchAutosaver::removeDanglingConnections(c);

				// look if there's a network file that we 
				// need to replace with this content
                valuetree::Helpers::forEach(c, DspNetworkListeners::PatchAutosaver::stripValueTree);

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
	MessageManagerLock mm;
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
	File f = parent.getScriptProcessor()->getMainController_()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::DspNetworks).getChildFile("CodeLibrary");
	f.createDirectory();
	return f;
}

DspNetwork::CodeManager::CodeManager(DspNetwork& p):
	parent(p)
{
			
}

DspNetwork::CodeManager::SnexSourceCompileHandler::SnexCompileListener::~SnexCompileListener()
{}

DspNetwork::CodeManager::SnexSourceCompileHandler::~SnexSourceCompileHandler()
{
	stopThread(1000);
}

void DspNetwork::CodeManager::SnexSourceCompileHandler::processTestParameterEvent(int parameterIndex, double value)
{}

Result DspNetwork::CodeManager::SnexSourceCompileHandler::prepareTest(PrepareSpecs ps,
	const Array<snex::ui::WorkbenchData::TestData::ParameterEvent>& initialParameters)
{ return Result::ok(); }

void DspNetwork::CodeManager::SnexSourceCompileHandler::processTest(ProcessDataDyn& data)
{}

void DspNetwork::CodeManager::SnexSourceCompileHandler::postCompile(ui::WorkbenchData::CompileResult& lastResult)
{
	auto currentThread = Thread::getCurrentThread();

	if (currentThread != this)
	{
		runTestNext.store(true);
		startThread();
		return;
	}

	if (lastResult.compiledOk() && test != nullptr && getParent()->getGlobalScope().isDebugModeEnabled())
	{
		getParent()->getGlobalScope().getBreakpointHandler().setExecutingThread(currentThread);
		lastResult.compileResult = test->runTest(lastResult);
		getParent()->getGlobalScope().getBreakpointHandler().setExecutingThread(nullptr);
	}

	runTestNext.store(false);
}

SimpleReadWriteLock& DspNetwork::CodeManager::SnexSourceCompileHandler::getCompileLock()
{ return compileLock; }

void DspNetwork::CodeManager::SnexSourceCompileHandler::addCompileListener(SnexCompileListener* l)
{
	compileListeners.addIfNotAlreadyThere(l);
}

void DspNetwork::CodeManager::SnexSourceCompileHandler::removeCompileListener(SnexCompileListener* l)
{
	compileListeners.removeAllInstancesOf(l);
}

void DspNetwork::CodeManager::SnexSourceCompileHandler::setTestBase(TestBase* ownedTest)
{
	test = ownedTest;
}

snex::ui::WorkbenchData::Ptr DspNetwork::CodeManager::getOrCreate(const Identifier& typeId, const Identifier& classId)
{
	using namespace snex::ui;

	for (auto e : entries)
	{
		if (e->wb->getInstanceId() == classId && e->type == typeId)
			return e->wb;
	}

	auto targetFile = getCodeFolder().getChildFile(typeId.toString()).getChildFile(classId.toString()).withFileExtension("h");

	auto ef = parent.getMainController()->getExternalScriptFile(targetFile, false);

	if(ef != nullptr)
		return entries.add(new Entry(typeId, ef, parent.getScriptProcessor()))->wb;
	else
		return entries.add(new Entry(typeId, targetFile, parent.getScriptProcessor()))->wb;
	
}

ValueTree DspNetwork::CodeManager::getParameterTree(const Identifier& typeId, const Identifier& classId)
{
	for (auto e : entries)
	{
		if (e->type == typeId && e->wb->getInstanceId() == classId)
			return e->parameterTree;
	}

	jassertfalse;
	return {};
}

StringArray DspNetwork::CodeManager::getClassList(const Identifier& id, const String& fileExtension)
{
	auto f = getCodeFolder();

	if (id.isValid())
		f = f.getChildFile(id.toString());

	StringArray sa;

	for (auto& l : f.findChildFiles(File::findFiles, true, fileExtension))
	{
		sa.add(l.getFileNameWithoutExtension());
	}

	return sa;
}

DspNetwork::CodeManager::Entry::Entry(const Identifier& t, const File& targetFile, ProcessorWithScriptingContent* sp):
	type(t),
	parameterFile(targetFile.withFileExtension("xml")),
	resourceType(ExternalScriptFile::ResourceType::FileBased)
{
	targetFile.create();

	ValueTree pTree;

	if (auto xml = XmlDocument::parse(parameterFile))
		pTree = ValueTree::fromXml(*xml);
	
	init(new snex::ui::WorkbenchData::DefaultCodeProvider(wb.get(), targetFile), pTree, sp);
}

struct EmbeddedSnippetCodeProvider: public snex::ui::WorkbenchData::CodeProvider
{
	EmbeddedSnippetCodeProvider(ExternalScriptFile::Ptr ef_):
	  CodeProvider(nullptr),
	  ef(ef_)
	{
		
	}

	String loadCode() const override
	{
		return ef->getFileDocument().getAllContent();
	};

	bool saveCode(const String& s) override
	{
        ef->getFileDocument().replaceAllContent(s);
		return true;
	}

	/** Override this method and return the instance id. This will be used to find the main class in nodes. */
	virtual Identifier getInstanceId() const override { return ef->getFile().getFileNameWithoutExtension(); }

	ExternalScriptFile::Ptr ef;
};

DspNetwork::CodeManager::Entry::Entry(const Identifier& t, const ExternalScriptFile::Ptr& embeddedFile, 
	ProcessorWithScriptingContent* sp):
	type(t),
    resourceType(ExternalScriptFile::ResourceType::EmbeddedInSnippet)
{
	parameterExternalFile = sp->getMainController_()->getExternalScriptFile(embeddedFile->getFile().withFileExtension(".xml"), true);

	ValueTree pTree;

	if(auto xml = XmlDocument::parse(parameterExternalFile->getFileDocument().getAllContent()))
		pTree = ValueTree::fromXml(*xml);

	init(new EmbeddedSnippetCodeProvider(embeddedFile), pTree, sp);
}

void DspNetwork::CodeManager::Entry::parameterAddedOrRemoved(ValueTree, bool)
{
	updateFile();
}

void DspNetwork::CodeManager::Entry::propertyChanged(ValueTree, Identifier)
{
	updateFile();
}

void DspNetwork::CodeManager::Entry::updateFile()
{
	auto xml = parameterTree.createXml();
	auto c = xml->createDocument("");

	if(resourceType == ExternalScriptFile::ResourceType::FileBased)
	{
		parameterFile.replaceWithText(c);
	}
	else
	{
		parameterExternalFile->getFileDocument().replaceAllContent(c);
	}
}

DspNetwork::CodeManager::SnexSourceCompileHandler::SnexSourceCompileHandler(snex::ui::WorkbenchData* d, ProcessorWithScriptingContent* sp_) :
	Thread("SNEX Compile Thread", HISE_DEFAULT_STACK_SIZE),
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

	if (currentThread == MainController::KillStateHandler::TargetThread::SampleLoadingThread ||
		currentThread == MainController::KillStateHandler::TargetThread::ScriptingThread)
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
		try
		{
			Error::throwError(Error::ErrorCode::DeprecatedNode, (int)id);
		}
		catch (Error& e)
		{
			auto nodeTree = cppgen::ValueTreeIterator::findParentWithType(v, PropertyIds::Node);
			auto nId = nodeTree[PropertyIds::ID].toString();

			if (auto node = n->getNodeWithId(nId))
			{
				n->getExceptionHandler().addError(node, e);
			}
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
}

bool ScriptnodeExceptionHandler::Item::operator==(const Item& other) const
{
	return node.get() == other.node.get();
}

String ScriptnodeExceptionHandler::Item::toString(const String& customErrorMessage) const
{
	if (node == nullptr || error.error == Error::OK)
		return {};
	else
	{
		String s;
		s << node->getCurrentId() << " - ";
				
		if (customErrorMessage.isNotEmpty())
			s << customErrorMessage;
		else
			s << getErrorMessage(error);
		return s;
	}
}

bool ScriptnodeExceptionHandler::isOk() const noexcept
{
	return items.isEmpty();
}

void ScriptnodeExceptionHandler::addCustomError(NodeBase* n, Error::ErrorCode c, const String& errorMessage)
{
	Error e;
	e.error = c;
	addError(n, e, errorMessage);
}

void ScriptnodeExceptionHandler::addError(NodeBase* n, Error e, const String& errorMessage)
{
	customErrorMessage = errorMessage;

	for (auto& i : items)
	{
		if (i.node == n)
		{
			i.error = e;
			return;
		}
	}

	items.add({ n, e });

	errorBroadcaster.sendMessage(sendNotificationAsync, n, e);
}

void ScriptnodeExceptionHandler::removeError(NodeBase* n, Error::ErrorCode errorToRemove)
{
	customErrorMessage = {};

	bool didSomething = false;

	for (int i = 0; i < items.size(); i++)
	{
		if(items[i].node == nullptr)
		{
			items.remove(i--);
			didSomething = true;
			continue;
		}

		auto e = items[i].error.error;

		auto isErrorCode = e == errorToRemove ||
		(errorToRemove == Error::numErrorCodes && 
			e != Error::ErrorCode::DeprecatedNode &&
			e != Error::ErrorCode::IllegalBypassConnection);

		if ((n == nullptr || (items[i].node == n)) && isErrorCode)
		{
			items.remove(i--);
			didSomething = true;
		}
	}

	auto lastItem = items.getLast();

	if(didSomething)
		errorBroadcaster.sendMessage(sendNotificationAsync, lastItem.node, lastItem.error);
}

String ScriptnodeExceptionHandler::getErrorMessage(Error e)
{
	String s;

	s << "**";

	switch (e.error)
	{
	case Error::ChannelMismatch: s << "Channel amount mismatch";  break;
	case Error::BlockSizeMismatch: s << "Blocksize mismatch"; break;
	case Error::IllegalFrameCall: s << "Can't be used in frame processing context"; return s;
	case Error::IllegalBlockSize: s << "Illegal block size: " << String(e.actual); return s;
	case Error::SampleRateMismatch: s << "Samplerate mismatch"; break;
	case Error::InitialisationError: return "Initialisation error";
	case Error::TooManyChildNodes: s << "Number of child nodes (" << e.actual << ") exceed channels (" << e.expected << ")."; return s;
	case Error::TooManyParameters: s << "Number of modulation sources (" << e.actual << ") exceed limit (" << e.expected << ")."; return s;
	case Error::NoMatchingParent:	 return "Can't find suitable parent node";
	case Error::RingBufferMultipleWriters: return "Buffer used multiple times";
	case Error::NodeDebuggerEnabled: return "Node is being debugged";
	case Error::NoGlobalManager: return "No global routing manager present.";
	case Error::DeprecatedNode:		 return DeprecationChecker::getErrorMessage(e.actual);
	case Error::IllegalPolyphony: return "Can't use this node in a polyphonic network";
	case Error::IllegalMonophony: return "Can't use this node in a monophonic network";
	case Error::IllegalFaustNode: return "Faust is disabled. Enable faust and recompile HISE.";
	case Error::IllegalFaustChannelCount: 
		s << "Faust node channel mismatch. Expected channels: `" << String(e.expected) << "`";
		s << "  \nActual input channels: `" << String(e.actual / 1000) << "`";
		s << "  \nActual output channels: `" << String(e.actual % 1000) << "`";
		return s;
	case Error::IllegalBypassConnection: return "Use a `container.soft_bypass` node";
	case Error::CloneMismatch:	return "Clone container must have equal child nodes";
	case Error::IllegalCompilation: return "Can't compile networks with this node. Uncheck the `AllowCompilation` flag to remove the error.";
	case Error::CompileFail:	s << "Compilation error** at Line " << e.expected << ", Column " << e.actual; return s;
	case Error::UncompiledThirdPartyNode: s << "Uncompiled Third Party Node. Export the DLL and restart HISE to load this node."; return s;
	case Error::UnscaledModRangeMismatch: s << "Unscaled mod range mismatch.  \n> Copy range to source"; return s;
	default:
		break;
	}

	s << "**:  \n`" << String(e.actual) << "` (expected: `" << String(e.expected) << "`)";

	return s;
}

String ScriptnodeExceptionHandler::getErrorMessage(const NodeBase* n) const
{
	for (auto& i : items)
	{
		if (i.node == n || n == nullptr)
		{
			return i.toString(customErrorMessage);
		}
	}

	return {};
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

DspNetwork::ProjectNodeHolder::ProjectNodeHolder(DspNetwork& parent):
	network(parent)
{

}

Identifier DspNetwork::ProjectNodeHolder::getParameterId(int index) const
{ return network.networkParameterHandler.getParameterId(index); }

int DspNetwork::ProjectNodeHolder::getNumParameters() const
{ return n.numParameters; }

void DspNetwork::ProjectNodeHolder::setParameter(int index, float newValue)
{
	if (auto p = n.getParameter(index))
	{
		parameterValues[index] = newValue;
		p->callback.call(newValue);
	}
}

float DspNetwork::ProjectNodeHolder::getParameter(int index) const
{ 
	if(isPositiveAndBelow(index, 16))
		return parameterValues[index]; 

	return 0.0f;
}

bool DspNetwork::ProjectNodeHolder::isActive() const
{ return forwardToNode; }

void DspNetwork::ProjectNodeHolder::prepare(PrepareSpecs ps)
{
	if(dll != nullptr)
		dll->clearError();

	n.prepare(ps);

	if (dll != nullptr)
	{
		auto e = dll->getError();

		if (!e.isOk())
			throw e;
	}

	n.reset();
}

bool DspNetwork::ProjectNodeHolder::handleModulation(double& modValue)
{
	return n.handleModulation(modValue);
}

void DspNetwork::ProjectNodeHolder::setEnabled(bool shouldBeEnabled)
{
	if (!loaded)
		return;

	if (shouldBeEnabled != forwardToNode)
	{
		forwardToNode = shouldBeEnabled;

		auto s1 = static_cast<ScriptParameterHandler*>(&network.networkParameterHandler);
		auto s2 = static_cast<ScriptParameterHandler*>(this);

		auto oh = forwardToNode ? s1 : s2;
		auto nh = forwardToNode ? s2 : s1;

		if (forwardToNode && network.currentSpecs)
		{
			prepare(network.currentSpecs);
			n.reset();
		}

		for (int i = 0; i < nh->getNumParameters(); i++)
			nh->setParameter(i, oh->getParameter(i));
	}
}

DspNetwork::ProjectNodeHolder::~ProjectNodeHolder()
{
	if (loaded && dll != nullptr)
	{
		dll->deInitOpaqueNode(&n);
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
			dll->initOpaqueNode(&n, i, network.isPolyphonic());
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

void DspNetwork::ProjectNodeHolder::init(dll::StaticLibraryHostFactory* staticLibrary)
{
	int numNodes = staticLibrary->getNumNodes();

	for (int i = 0; i < numNodes; i++)
	{
		if (network.getId() == staticLibrary->getId(i))
		{
			loaded = staticLibrary->initOpaqueNode(&n, i, network.isPolyphonic());
		}
	}
}

int HostHelpers::getNumMaxDataObjects(const ValueTree& v, snex::ExternalData::DataType t)
{
	auto id = Identifier(snex::ExternalData::getDataTypeName(t, false));

	int numMax = 0;

#if USE_BACKEND
	valuetree::Helpers::forEach(v, [&](ValueTree& v)
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

bool OpaqueNetworkHolder::isPolyphonic() const
{ return false; }

OpaqueNetworkHolder::OpaqueNetworkHolder()
{

}

OpaqueNetworkHolder::~OpaqueNetworkHolder()
{
	ownedNetwork = nullptr;
}

void OpaqueNetworkHolder::handleHiseEvent(HiseEvent& e)
{
	ownedNetwork->handleHiseEvent(e);
}

bool OpaqueNetworkHolder::handleModulation(double& modValue)
{
	return ownedNetwork->handleModulation(modValue);
}

void OpaqueNetworkHolder::process(ProcessDataDyn& d)
{
	ownedNetwork->process(d);
}

void OpaqueNetworkHolder::reset()
{
	ownedNetwork->reset();
}

void OpaqueNetworkHolder::prepare(PrepareSpecs ps)
{
	snex::Types::DllBoundaryTempoSyncer::ScopedModValueChange smvs(*ps.voiceIndex->getTempoSyncer(), ownedNetwork->getNetworkModValue());
	ownedNetwork->setNumChannels(ps.numChannels);
	ownedNetwork->prepareToPlay(ps.sampleRate, ps.blockSize);
}

DspNetwork* OpaqueNetworkHolder::getNetwork()
{
	return ownedNetwork.get();
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

		while (pp != nullptr)
		{
			isInMidiChain |= pp->getValueTree()[PropertyIds::FactoryPath].toString().contains("midichain");

			if(pp->getValueTree()[PropertyIds::FactoryPath].toString().contains("no_midi"))
			{
				isInMidiChain = false;
				break;
			}

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
void DspNetworkListeners::PatchAutosaver::removeDanglingConnections(ValueTree& v)
{
	valuetree::Helpers::forEach(v, [v](ValueTree& c)
	{
		auto nodeExists = [v](const String& id)
		{
			return cppgen::ValueTreeIterator::forEach(v, [id](ValueTree& n)
			{
				if (n.getType() == PropertyIds::Node)
				{
					if (n[PropertyIds::ID].toString() == id)
						return true;
				}

				return false;
			}, valuetree::Helpers::IterationType::ChildrenFirst);
		};

		if (c.getType() == PropertyIds::Property && c[PropertyIds::ID].toString() == PropertyIds::Connection.toString())
		{
			// A global cable will also store its cable ID as Connection property so we need to avoid stripping this
			// vital information...
			auto isGlobalCableConnection = c.getParent().getParent()[PropertyIds::FactoryPath].toString().startsWith("routing.global");

			if (!isGlobalCableConnection)
			{
				auto idList = StringArray::fromTokens(c[PropertyIds::Value].toString(), ";", "");

				int numBefore = idList.size();

				for (int i = 0; i < idList.size(); i++)
				{
					if (!nodeExists(idList[i]))
						idList.remove(i--);
				}

				if (idList.size() != numBefore)
				{
					c.setProperty(PropertyIds::Value, idList.joinIntoString(";"), nullptr);
				}
			}
		}

		return false;
	}, valuetree::Helpers::IterationType::ChildrenFirst);
}

bool DspNetworkListeners::PatchAutosaver::stripValueTree(ValueTree& v)
{
	
	// Remove all child nodes from a project node
	// (might be a leftover from the extraction process)
	if (v.getType() == PropertyIds::Node && v[PropertyIds::FactoryPath].toString().startsWith("project"))
	{
		v.removeChild(v.getChildWithName(PropertyIds::Nodes), nullptr);

		for (auto p : v.getChildWithName(PropertyIds::Parameters))
			p.removeChild(p.getChildWithName(PropertyIds::Connections), nullptr);
	}

	auto propChild = v.getChildWithName(PropertyIds::Properties);

	// Remove all properties with the default value
	for (int i = 0; i < propChild.getNumChildren(); i++)
	{
		if (removePropIfDefault(propChild.getChild(i), PropertyIds::IsVertical, 1))
			propChild.removeChild(i--, nullptr);
	}

	removeIfNoChildren(propChild);

	for (auto id : PropertyIds::Helpers::getDefaultableIds())
		removeIfDefault(v, id, PropertyIds::Helpers::getDefaultValue(id));

	removeIfDefined(v, PropertyIds::Value, PropertyIds::Automated);

	removeIfNoChildren(v.getChildWithName(PropertyIds::Bookmarks));
	removeIfNoChildren(v.getChildWithName(PropertyIds::ModulationTargets));

	return false;
}
#endif

DspNetwork::FaustManager::FaustManager(DspNetwork& n) :
	lastCompileResult(Result::ok()),
	processor(dynamic_cast<Processor*>(n.getScriptProcessor()))
{

}

void DspNetwork::FaustManager::sendPostCompileMessage()
{
	for (auto l : listeners)
	{
		if (l != nullptr)
		{
			l->faustCodeCompiled(lastCompiledFile, lastCompileResult);
		}
	}
}

void DspNetwork::FaustManager::addFaustListener(FaustListener* l)
{
	{
		hise::SimpleReadWriteLock::ScopedWriteLock sl(listenerLock);
		listeners.addIfNotAlreadyThere(l);
	}

	l->faustFileSelected(currentFile);
	l->faustCodeCompiled(lastCompiledFile, lastCompileResult);
}

void DspNetwork::FaustManager::removeFaustListener(FaustListener* l)
{
	hise::SimpleReadWriteLock::ScopedWriteLock sl(listenerLock);
	listeners.removeAllInstancesOf(l);
}

void DspNetwork::FaustManager::setSelectedFaustFile(Component* c, const File& f, NotificationType n)
{
#if USE_BACKEND
	GET_BACKEND_ROOT_WINDOW(c)->addEditorTabsOfType<FaustEditorPanel>();
#endif

	currentFile = f;

	if (n != dontSendNotification)
	{
		hise::SimpleReadWriteLock::ScopedReadLock sl(listenerLock);

		for (auto l : listeners)
		{
			if (l != nullptr)
			{
				l->faustFileSelected(currentFile);
			}
		}
	}
}

void DspNetwork::FaustManager::sendCompileMessage(const File& f, NotificationType n)
{
	WeakReference<FaustManager> safeThis(this);

	lastCompiledFile = f;
	lastCompileResult = Result::ok();

	{
		hise::SimpleReadWriteLock::ScopedReadLock sl(listenerLock);

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->preCompileFaustCode(lastCompiledFile);
		}
	}
	

	auto pf = [safeThis, n](Processor* p)
	{
		if (safeThis == nullptr)
			return SafeFunctionCall::Status::nullPointerCall;

		auto file = safeThis->lastCompiledFile;

		p->getMainController()->getSampleManager().setCurrentPreloadMessage("Compile Faust file " + file.getFileNameWithoutExtension());

		{
			Array<WeakReference<FaustListener>> thisListeners;
			thisListeners.ensureStorageAllocated(safeThis->listeners.size());

			{
				hise::SimpleReadWriteLock::ScopedReadLock sl(safeThis->listenerLock);

				for (auto l : safeThis->listeners)
				{
					if (l != nullptr)
						thisListeners.add(l);
				}
			}

			for (auto l : thisListeners)
			{
				if (l != nullptr)
				{
					auto thisOk = l->compileFaustCode(file);

					if (!thisOk.wasOk())
					{
						safeThis->lastCompileResult = thisOk;
						break;
					}
				}
			}
		}
		
		if (n != dontSendNotification)
		{
			SafeAsyncCall::call<FaustManager>(*safeThis, [](FaustManager& m)
			{
				m.sendPostCompileMessage();
			});
		}

		return SafeFunctionCall::OK;
	};

	

	processor->getMainController()->getKillStateHandler().killVoicesAndCall(processor, pf, 
		MainController::KillStateHandler::TargetThread::SampleLoadingThread);
}





}


