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
	if (prepareFunc)
	{
		prepareFunc(getObjectPtr(), &ps);
		resetFunc(getObjectPtr());
	}
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
	if(externalDataFunc != nullptr)
		externalDataFunc(getObjectPtr(), &b, index);
}

void OpaqueNode::createParameters(ParameterDataList& l)
{
	for (int i = 0; i < numParameters; i++)
	{
		parameter::data d;
		d.info = parameters[i];
		d.callback.referTo(parameterObjects[i], parameterFunctions[i]);
		d.parameterNames = parameterNames[i];
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

	ExternalData::forEachType(initAll);
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

void OpaqueNode::fillParameterList(ParameterDataList& pList)
{
	numParameters = pList.size();

	for (int i = 0; i < numParameters; i++)
	{
		parameters[i] = pList[i].info;
		parameterFunctions[i] = pList[i].callback.getFunction();
		parameterObjects[i] = pList[i].callback.getObjectPtr();
		parameterNames[i] = pList[i].parameterNames;
	}
}

namespace dll
{

String StaticLibraryHostFactory::getId(int index) const
{
	return items[index].id;
}

int StaticLibraryHostFactory::getNumNodes() const
{
	return items.size();
}

int StaticLibraryHostFactory::getWrapperType(int index) const
{
	return items[index].isModNode ? 1 : 0;
}

bool StaticLibraryHostFactory::initOpaqueNode(scriptnode::OpaqueNode* n, int index, bool polyphonicIfPossible)
{
	if (polyphonicIfPossible && items[index].pf)
		items[index].pf(n);
	else
		items[index].f(n);

	if (items[index].pf)
		n->setCanBePolyphonic();

	memcpy(n->numDataObjects, items[index].numDataObjects, sizeof(int) * (int)ExternalData::DataType::numDataTypes);
	return true;
}

int StaticLibraryHostFactory::getNumDataObjects(int index, int dataTypeAsInt) const
{
	return items[index].numDataObjects[dataTypeAsInt];
}

Error StaticLibraryHostFactory::getError() const
{
#if THROW_SCRIPTNODE_ERRORS
	// must never happen - a static library factory either
	// resides in the dynamic library and thus THROW_SCRIPTNODE_ERRORS==0
	// or it will be in the compiled project and never ask this question
	jassertfalse;
	return {};
#else
	return DynamicLibraryErrorStorage::currentError;
#endif
}

void StaticLibraryHostFactory::clearError() const
{
#if THROW_SCRIPTNODE_ERRORS
	jassertfalse;
#else
	DynamicLibraryErrorStorage::currentError = {};
#endif
}

DynamicLibraryHostFactory::DynamicLibraryHostFactory(ProjectDll::Ptr dll_) :
	projectDll(dll_)
{
	
}

DynamicLibraryHostFactory::~DynamicLibraryHostFactory()
{
	projectDll = nullptr;
}

int DynamicLibraryHostFactory::getNumNodes() const
{
	if (projectDll != nullptr)
		return projectDll->getNumNodes();

	return 0;
}

String DynamicLibraryHostFactory::getId(int index) const
{
	if (projectDll != nullptr)
		return projectDll->getNodeId(index);

	return {};
}

bool DynamicLibraryHostFactory::initOpaqueNode(scriptnode::OpaqueNode* n, int index, bool polyphonicIfPossible)
{
	if (projectDll != nullptr)
	{
		auto ok = projectDll->initOpaqueNode(n, index, polyphonicIfPossible);

		if (ok && n->canBePolyphonic())
		{
			auto id = projectDll->getNodeId(index);
			cppgen::CustomNodeProperties::addNodeIdManually(Identifier(id), PropertyIds::IsPolyphonic);
		}

		return ok;
	}

	return false;
}

int DynamicLibraryHostFactory::getNumDataObjects(int index, int dataTypeAsInt) const
{
	if (projectDll != nullptr)
		return projectDll->getNumDataObjects(index, dataTypeAsInt);

	return 0;
}

int DynamicLibraryHostFactory::getWrapperType(int index) const
{
	if (projectDll != nullptr)
		return projectDll->getWrapperType(index);

	return 0;
}

void DynamicLibraryHostFactory::clearError() const
{
	if (projectDll != nullptr)
		projectDll->clearError();
}

Error DynamicLibraryHostFactory::getError() const
{
	if (projectDll != nullptr)
		return projectDll->getError();

	return {};
}

String ProjectDll::getFuncName(ExportedFunction f)
{
	switch (f)
	{
	case ExportedFunction::GetHash:				return "getHash";
	case ExportedFunction::GetWrapperType:		return "getWrapperType";
	case ExportedFunction::GetNumNodes:			return "getNumNodes";
	case ExportedFunction::GetNodeId:			return "getNodeId";
	case ExportedFunction::InitOpaqueNode:		return "initOpaqueNode";
	case ExportedFunction::DeInitOpaqueNode:	return "deInitOpaqueNode";
	case ExportedFunction::GetNumDataObjects:	return "getNumDataObjects";
	case ExportedFunction::GetError:			return "getError";
	case ExportedFunction::ClearError:			return "clearError";
	default: jassertfalse; return "";
	}
}

#define DLL_FUNCTION(x) ((x)functions[(int)ExportedFunction::x])

int ProjectDll::getWrapperType(int i) const
{
	if (*this)
		return DLL_FUNCTION(GetWrapperType)(i);
    
    return 0;
}

int ProjectDll::getNumNodes() const
{
	if (*this)
		return DLL_FUNCTION(GetNumNodes)();

	return 0;
}

int ProjectDll::getNumDataObjects(int nodeIndex, int dataTypeAsInt) const
{
	if (*this)
		return DLL_FUNCTION(GetNumDataObjects)(nodeIndex, dataTypeAsInt);

	return 0;
}

String ProjectDll::getNodeId(int index) const
{
	if (*this)
	{
		char buffer[256];

		auto length = DLL_FUNCTION(GetNodeId)(index, buffer);
		return String(buffer, length);
	}

	return {};
}

bool ProjectDll::initOpaqueNode(OpaqueNode* n, int index, bool polyphonicIfPossible)
{
	if (*this)
	{
		DLL_FUNCTION(InitOpaqueNode)(n, index, polyphonicIfPossible);

		for(int i = 0; i < (int)ExternalData::DataType::numDataTypes; i++)
			n->numDataObjects[i] = DLL_FUNCTION(GetNumDataObjects)(index, i);

		return true;
	}

	return false;
}


void ProjectDll::deInitOpaqueNode(OpaqueNode* n)
{
	if (*this)
		DLL_FUNCTION(DeInitOpaqueNode)(n);
}

void ProjectDll::clearError() const
{
	if (*this)
		DLL_FUNCTION(ClearError)();
}

Error ProjectDll::getError() const
{
	if (*this)
		return DLL_FUNCTION(GetError)();

	return {};
}

int ProjectDll::getHash(int index) const
{
	if (*this)
		return DLL_FUNCTION(GetHash)(index);

	jassertfalse;
	return 0;
}

#undef DLL_FUNCTION

ProjectDll::ProjectDll(const File& f):
	r(Result::fail("Can't find DLL file "  + f.getFullPathName()))
{
	dll = new DynamicLibrary();

	if (dll->open(f.getFullPathName()))
	{
		r = Result::ok();

		for (int i = 0; i < (int)ExportedFunction::numFunctions; i++)
			functions[i] = getFromDll((ExportedFunction)i, f);
	}
	else
	{
		clearAllFunctions();
		dll->close();
		dll = nullptr;
	}
}

ProjectDll::~ProjectDll()
{
	clearAllFunctions();
	dll->close();
	dll = nullptr;
}





}

}
