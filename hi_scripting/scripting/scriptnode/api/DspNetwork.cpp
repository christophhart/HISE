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
	API_METHOD_WRAPPER_1(DspNetwork, get);
	API_VOID_METHOD_WRAPPER_1(DspNetwork, setForwardControlsToParameters);
	API_METHOD_WRAPPER_1(DspNetwork, setParameterDataFromJSON);
	//API_VOID_METHOD_WRAPPER_0(DspNetwork, disconnectAll);
	//API_VOID_METHOD_WRAPPER_3(DspNetwork, injectAfter);
};

DspNetwork::DspNetwork(hise::ProcessorWithScriptingContent* p, ValueTree data_, bool poly) :
	ConstScriptingObject(p, 2),
	data(data_),
	isPoly(poly),
	voiceIndex(poly),
#if HISE_INCLUDE_SNEX
	codeManager(*this),
#endif
	parentHolder(dynamic_cast<Holder*>(p))
{
	setExternalDataHolder(dynamic_cast<ExternalDataHolder*>(p));

	ownedFactories.add(new NodeContainerFactory(this));
	ownedFactories.add(new core::Factory(this));
	ownedFactories.add(new math::Factory(this));
	ownedFactories.add(new routing::Factory(this));
	ownedFactories.add(new analyse::Factory(this));
	ownedFactories.add(new fx::Factory(this));
	ownedFactories.add(new control::Factory(this));
	ownedFactories.add(new examples::Factory(this));
	ownedFactories.add(new dynamics::Factory(this));
	ownedFactories.add(new filters::Factory(this));
	ownedFactories.add(new stk_factory::Factory(this));

#if HISE_INCLUDE_SNEX
	if (auto ah = dynamic_cast<Holder*>(p))
	{
		if (ah->projectDll != nullptr)
		{
			ownedFactories.add(new dll::FunkyHostFactory(this, ah->projectDll));
		}
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
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_1(setForwardControlsToParameters);
	ADD_API_METHOD_1(setParameterDataFromJSON);
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

	checkIfDeprecated();
}

DspNetwork::~DspNetwork()
{
	selectionUpdater = nullptr;
	nodes.clear();
    nodeFactories.clear();
}


void DspNetwork::setNumChannels(int newNumChannels)
{
	getRootNode()->getValueTree().setProperty(PropertyIds::NumChannels, newNumChannels, nullptr);
}

void DspNetwork::rightClickCallback(const MouseEvent& e, Component* c)
{

#if USE_BACKEND

	auto* d = new DspNetworkGraph(this);

	d->setSize(600, 600);

	auto editor = GET_BACKEND_ROOT_WINDOW(c);

	MouseEvent ee = e.getEventRelativeTo(editor);

	editor->getRootFloatingTile()->showComponentInRootPopup(d, editor, ee.getMouseDownPosition());
#else
	ignoreUnused(e, c);
#endif
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
		if (!n->isActive())
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

void DspNetwork::process(AudioSampleBuffer& b, HiseEventBuffer* e)
{
	ScopedLock sl(getConnectionLock());

	if (exceptionHandler.isOk())
	{
		ProcessDataDyn d(b.getArrayOfWritePointers(), b.getNumSamples(), b.getNumChannels());
		d.setEventBuffer(*e);

		getRootNode()->process(d);
	}
}

juce::Identifier DspNetwork::getParameterIdentifier(int parameterIndex)
{
	return getRootNode()->getParameter(parameterIndex)->getId();
}

void DspNetwork::setForwardControlsToParameters(bool shouldForward)
{
	checkValid();

	forwardControls = shouldForward;
}

void DspNetwork::prepareToPlay(double sampleRate, double blockSize)
{
	if (sampleRate > 0.0)
	{
		ScopedLock sl(getConnectionLock());

		try
		{


			originalSampleRate = sampleRate;

			currentSpecs.sampleRate = sampleRate;
			currentSpecs.blockSize = (int)blockSize;
			currentSpecs.numChannels = getRootNode()->getNumChannelsToProcess();
			currentSpecs.voiceIndex = &voiceIndex;

			getRootNode()->prepare(currentSpecs);
			getRootNode()->reset();
		}
		catch (String& errorMessage)
		{
			jassertfalse;
		}

		
	}
}

void DspNetwork::processBlock(var pData)
{
	ScopedLock sl(getConnectionLock());

	if (exceptionHandler.isOk())
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
			getRootNode()->process(d);
		}
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
					if (auto p = node->getParameter(pId))
					{
						p->getTreeWithValue().setProperty(PropertyIds::Value, value, getUndoManager());
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
		for (int i = 0; i < node->getNumParameters(); i++)
		{
			if (node->getParameter(i)->isProbed)
				list.add(node->getParameter(i));
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
			if(originalSampleRate > 0.0)
				newNode->prepare(currentSpecs);

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

void DspNetwork::addToSelection(NodeBase* node, ModifierKeys mods)
{
	auto pNode = node->getParentNode();

	while (mods.isAnyModifierKeyDown() && pNode != nullptr)
	{
		if (isSelected(pNode))
			return;

		pNode = pNode->getParentNode();
	}

	selection.addToSelectionBasedOnModifiers(node, mods);
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
		if (v.hasType(PropertyIds::Connection))
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

DspNetwork* DspNetwork::Holder::getOrCreate(const String& id)
{
	auto asScriptProcessor = dynamic_cast<ProcessorWithScriptingContent*>(this);

	for (auto n : networks)
	{
		if (n->getId() == id)
			return n;
	}

	ValueTree v(scriptnode::PropertyIds::Network);
	v.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	ValueTree s(scriptnode::PropertyIds::Node);
	s.setProperty(scriptnode::PropertyIds::FactoryPath, "container.chain", nullptr);
	s.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	v.addChild(s, -1, nullptr);

	auto newNetwork = new DspNetwork(asScriptProcessor, v, isPolyphonic());

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
			v.addChild(n->getValueTree().createCopy(), -1, nullptr);
		}

		d.addChild(v, -1, nullptr);
	}
}

void DspNetwork::Holder::restoreNetworks(const ValueTree& d)
{
	auto v = d.getChildWithName("Networks");

	if (v.isValid())
	{
		networks.clear();

		for (auto c : v)
		{
			auto newNetwork = new DspNetwork(dynamic_cast<ProcessorWithScriptingContent*>(this),
				c.createCopy(), isPolyphonic());

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

}

