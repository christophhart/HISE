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

namespace scriptnode
{
using namespace juce;
using namespace hise;

OpaqueNode::OpaqueNode()
{
}

OpaqueNode::~OpaqueNode()
{
	callDestructor();
}

void OpaqueNode::allocateObjectSize(int numBytes)
{
	object.setSize(numBytes);
}

void OpaqueNode::prepare(PrepareSpecs ps)
{
	prepareFunc(getObjectPtr(), &ps);
	resetFunc(getObjectPtr());
}

void OpaqueNode::process(ProcessDataDyn& data)
{
	processFunc(getObjectPtr(), &data);
}

void OpaqueNode::processFrame(MonoFrame& d)
{
	monoFrame(getObjectPtr(), &d);
}

void OpaqueNode::processFrame(StereoFrame& d)
{
	stereoFrame(getObjectPtr(), &d);
}

void OpaqueNode::reset()
{
	resetFunc(getObjectPtr());
}

void OpaqueNode::handleHiseEvent(HiseEvent& e)
{
	eventFunc(getObjectPtr(), &e);
}

void OpaqueNode::setExternalData(const ExternalData& b, int index)
{
	externalDataFunc(getObjectPtr(), &b, index);
}

void OpaqueNode::createParameters(ParameterDataList& l)
{
	for (int i = 0; i < numParameters; i++)
	{
		parameter::data d;
		d.info = parameters[i];
		d.callback.referTo(parameterObjects[i], parameterFunctions[i]);
		l.add(d);
	}
}

void OpaqueNode::initExternalData(ExternalDataHolder* externalDataHolder)
{
	int totalIndex = 0;

	if (externalDataHolder == nullptr)
		return;

	auto initAll = [this, &totalIndex, externalDataHolder](ExternalData::DataType d)
	{
		auto numRequired = externalDataHolder->getNumDataObjects(d);

		for (int i = 0; i < numRequired; i++)
		{
			auto b = externalDataHolder->getData(d, i);
			setExternalData(b, totalIndex++);
		}
	};

	initAll(ExternalData::DataType::Table);
	initAll(ExternalData::DataType::SliderPack);
	initAll(ExternalData::DataType::AudioFile);
}

void OpaqueNode::setExternalPtr(void* externPtr)
{
	callDestructor();

	object.setExternalPtr(externPtr);
}

void OpaqueNode::callDestructor()
{
	if (destructFunc != nullptr && getObjectPtr() != nullptr)
	{
		destructFunc(getObjectPtr());

		object.free();
		destructFunc = nullptr;
	}
}

bool OpaqueNode::handleModulation(double& d)
{
	return modFunc(getObjectPtr(), &d) > 0;
}

namespace dll
{

String PluginFactory::getId(int index) const
{
	return items[index].id;
}

int PluginFactory::getNumNodes() const
{
	return items.size();
}

bool PluginFactory::initOpaqueNode(scriptnode::OpaqueNode* n, int index)
{
	items[index].f(n);
	return true;
}

HostFactory::HostFactory(ProjectDll::Ptr dll_) :
	projectDll(dll_)
{
	jassert(projectDll != nullptr);
}

HostFactory::~HostFactory()
{
	projectDll = nullptr;
}

int HostFactory::getNumNodes() const
{
	if (projectDll != nullptr)
		return projectDll->getNumNodes();

	return 0;
}

String HostFactory::getId(int index) const
{
	if (projectDll != nullptr)
		return projectDll->getNodeId(index);

	return {};
}

bool HostFactory::initOpaqueNode(scriptnode::OpaqueNode* n, int index)
{
	if (projectDll != nullptr)
		return projectDll->initNode(n, index);

	return false;
}

int ProjectDll::getNumNodes() const
{
	if (gnnf != nullptr)
		return gnnf();

	return 0;
}

String ProjectDll::getNodeId(int index) const
{
	if (gnif != nullptr)
	{
		char buffer[256];

		auto length = gnif(index, buffer);
		return String(buffer, length);
	}

	return {};
}

bool ProjectDll::initNode(OpaqueNode* n, int index)
{
	if (inf != nullptr)
	{
		inf(n, index);
		return true;
	}

	return false;
}


ProjectDll::ProjectDll(const File& f)
{
	dll = new DynamicLibrary();

	if (dll->open(f.getFullPathName()))
	{
		gnnf = (GetNumNodesFunc)dll->getFunction("getNumNodes");
		gnif = (GetNodeIdFunc)dll->getFunction("getNodeId");
		inf = (InitNodeFunc)dll->getFunction("initOpaqueNode");


		if (auto f = (int(*)())dll->getFunction("getHash"))
			hash = f();

		ok = gnnf != nullptr && gnif != nullptr && inf != nullptr && hash != 0;
	}
	else
	{
		gnnf = nullptr;
		gnif = nullptr;
		inf = nullptr;
		dnf = nullptr;

		ok = false;
	}
}

ProjectDll::~ProjectDll()
{
	dll->close();
	ok = false;
	dll = nullptr;
}

}

}