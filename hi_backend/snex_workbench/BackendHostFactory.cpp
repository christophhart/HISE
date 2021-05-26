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


namespace scriptnode
{
using namespace juce;
using namespace hise;


struct OpaqueNetworkHolder
{
	SN_GET_SELF_AS_OBJECT(OpaqueNetworkHolder);

	bool isPolyphonic() const { return false; }

	HISE_EMPTY_INITIALISE;
	HISE_EMPTY_PROCESS_SINGLE;

	OpaqueNetworkHolder()
	{

	}

	~OpaqueNetworkHolder()
	{
		ownedNetwork = nullptr;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		ownedNetwork->getRootNode()->handleHiseEvent(e);
	}

	bool handleModulation(double& modValue)
	{
		return ownedNetwork->handleModulation(modValue);
	}

	void process(ProcessDataDyn& d)
	{
		ownedNetwork->process(d);
	}

	void reset()
	{
		ownedNetwork->getRootNode()->reset();
	}

	void prepare(PrepareSpecs ps)
	{
		ownedNetwork->prepareToPlay(ps.sampleRate, ps.blockSize);
	}

	void createParameters(ParameterDataList& l)
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

	void setCallback(parameter::data& d, int index)
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

	template <int P> static void setParameterStatic(void* obj, double v)
	{
		auto t = static_cast<OpaqueNetworkHolder*>(obj);
		t->ownedNetwork->getCurrentParameterHandler()->setParameter(P, (float)v);
	}

	void setNetwork(DspNetwork* n)
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

	DspNetwork* getNetwork() {
		return ownedNetwork;
	}

	void setExternalData(const ExternalData& d, int index)
	{
		if (ownedNetwork != nullptr)
			ownedNetwork->setExternalData(d, index);
		else
			deferredData.add({ d, index });
	}

private:

	struct DeferedDataInitialiser
	{
		ExternalData d;
		int index;
	};

	Array<DeferedDataInitialiser> deferredData;
	ReferenceCountedObjectPtr<DspNetwork> ownedNetwork;
};

struct HostHelpers
{
	static int getNumMaxDataObjects(const ValueTree& v, snex::ExternalData::DataType t)
	{
		auto id = Identifier(snex::ExternalData::getDataTypeName(t, false));

		int numMax = 0;

		snex::cppgen::ValueTreeIterator::forEach(v, snex::cppgen::ValueTreeIterator::Forward, [&](ValueTree& v)
			{
				if (v.getType() == id)
					numMax = jmax(numMax, (int)v[PropertyIds::Index] + 1);

				return false;
			});

		return numMax;
	}

	static void setNumDataObjectsFromValueTree(OpaqueNode& on, const ValueTree& v)
	{
		snex::ExternalData::forEachType([&](snex::ExternalData::DataType dt)
			{
				auto numMaxDataObjects = getNumMaxDataObjects(v, dt);
				on.numDataObjects[(int)dt] = numMaxDataObjects;
			});
	}

	template <typename WrapperType> static NodeBase* initNodeWithNetwork(DspNetwork* p, ValueTree nodeTree, const ValueTree& embeddedNetworkTree, bool useMod)
	{
		auto t = dynamic_cast<WrapperType*>(WrapperType::createNode<OpaqueNetworkHolder, NoExtraComponent, false, false>(p, nodeTree));

		auto& on = t->getWrapperType().getWrappedObject();
		setNumDataObjectsFromValueTree(on, embeddedNetworkTree);
		auto ed = t->setOpaqueDataEditor(useMod);

		auto onh = static_cast<OpaqueNetworkHolder*>(on.getObjectPtr());
		onh->setNetwork(p->getParentHolder()->addEmbeddedNetwork(p, embeddedNetworkTree, ed));

		ParameterDataList pList;
		onh->createParameters(pList);
		on.fillParameterList(pList);

		t->postInit();
		auto asNode = dynamic_cast<NodeBase*>(t);
		asNode->setEmbeddedNetwork(onh->getNetwork());

		return asNode;
	}
};

namespace dll
{

BackendHostFactory::BackendHostFactory(DspNetwork* n, ProjectDll::Ptr dll) :
	NodeFactory(n),
	dllFactory(dll)
{
	auto networks = BackendDllManager::getNetworkFiles(n->getScriptProcessor()->getMainController_());
	auto numNodes = networks.size();

	for (int i = 0; i < numNodes; i++)
	{
		auto f = networks[i];
		NodeFactory::Item item;
		item.id = f.getFileNameWithoutExtension();
		item.cb = [this, i, f](DspNetwork* p, ValueTree v)
		{
			auto nodeId = f.getFileNameWithoutExtension();
			auto networkFile = f;


			if (networkFile.existsAsFile())
			{
				if (ScopedPointer<XmlElement> xml = XmlDocument::parse(networkFile.loadFileAsString()))
				{
					auto nv = ValueTree::fromXml(*xml);

					auto useMod = cppgen::ValueTreeIterator::hasChildNodeWithProperty(nv, PropertyIds::IsPublicMod);

					if (useMod)
						return HostHelpers::initNodeWithNetwork<InterpretedModNode>(p, v, nv, useMod);
					else
						return HostHelpers::initNodeWithNetwork<InterpretedNode>(p, v, nv, useMod);
				}
			}
		};

		monoNodes.add(item);
	}

#if 0 // This might be reused in the FrontendHostFactory class...
	auto numNodes = dllFactory.getNumNodes();

	for (int i = 0; i < numNodes; i++)
	{
		NodeFactory::Item item;

		item.id = Identifier(dllFactory.getId(i));
		item.cb = [this, i](DspNetwork* p, ValueTree v)
		{
			auto nodeId = dllFactory.getId(i);
			auto networkFolder = BackendDllManager::getSubFolder(p->getScriptProcessor()->getMainController_(), BackendDllManager::FolderSubType::Networks);
			auto networkFile = networkFolder.getChildFile(nodeId).withFileExtension("xml");

			if (networkFile.existsAsFile())
			{
				if (ScopedPointer<XmlElement> xml = XmlDocument::parse(networkFile.loadFileAsString()))
				{
					auto t = dynamic_cast<InterpretedModNode*>(InterpretedModNode::createNode<OpaqueNetworkHolder, NoExtraComponent, false, false>(p, v));

					auto& on = t->getWrapperType().getWrappedObject();
					auto onh = static_cast<OpaqueNetworkHolder*>(on.getObjectPtr());

					auto nv = ValueTree::fromXml(*xml);

					onh->ownedNetwork = new DspNetwork(p->getScriptProcessor(), nv, p->isPolyphonic());

					ParameterDataList pList;
					onh->createParameters(pList);
					on.fillParameterList(pList);
					t->setOpaqueDataEditor(true);

					t->postInit();

					return dynamic_cast<NodeBase*>(t);
				}
			}

			if (dllFactory.getWrapperType(i) == 1)
			{
				auto t = new scriptnode::InterpretedModNode(p, v);
				t->initFromDll(dllFactory, i, true);
				return dynamic_cast<NodeBase*>(t);
			}

			else
			{
				auto t = new scriptnode::InterpretedNode(p, v);
				t->initFromDll(dllFactory, i, false);
				return dynamic_cast<NodeBase*>(t);
			}
		};

		monoNodes.add(item);
	}
#endif
}


}
}