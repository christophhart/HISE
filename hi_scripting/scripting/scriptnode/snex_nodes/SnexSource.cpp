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

namespace SnexIcons
{
static const unsigned char bugIcon[] = { 110,109,197,160,17,66,82,88,16,67,108,84,227,138,65,12,66,10,67,108,0,0,0,0,166,155,143,66,108,16,88,180,65,123,212,131,66,98,0,0,218,65,47,221,167,66,240,167,255,65,227,229,203,66,240,167,18,66,152,238,239,66,108,195,117,81,66,207,247,249,66,108,20,
238,40,67,207,247,249,66,108,203,161,56,67,152,238,239,66,108,197,192,70,67,123,212,131,66,98,90,68,78,67,137,193,135,66,240,199,85,67,152,174,139,66,199,75,93,67,166,155,143,66,108,27,239,75,67,12,66,10,67,108,10,55,54,67,242,50,17,67,98,248,51,54,67,
145,173,28,67,170,49,53,67,109,199,39,67,176,82,51,67,49,72,50,67,108,27,239,75,67,162,37,58,67,108,199,75,93,67,219,153,124,67,108,197,192,70,67,184,62,129,67,108,203,161,56,67,33,112,76,67,108,74,172,45,67,158,239,72,67,98,43,103,33,67,53,126,110,67,
199,43,9,67,186,9,132,67,59,159,218,66,186,9,132,67,98,180,72,163,66,186,9,132,67,63,53,102,66,16,216,110,67,223,207,52,66,195,181,73,67,108,240,167,18,66,33,112,76,67,108,16,88,180,65,184,62,129,67,108,0,0,0,0,219,153,124,67,108,84,227,138,65,162,37,
58,67,108,168,198,29,66,10,23,51,67,98,59,223,21,66,246,72,40,67,190,159,17,66,70,214,28,67,190,159,17,66,238,252,16,67,98,190,159,17,66,37,198,16,67,190,159,17,66,27,143,16,67,197,160,17,66,82,88,16,67,99,109,68,107,0,67,8,172,99,66,98,70,86,8,67,193,
202,1,66,53,62,25,67,205,204,44,65,219,89,49,67,96,229,72,64,108,244,93,59,67,0,0,0,0,98,246,104,61,67,117,147,208,64,248,115,63,67,141,151,80,65,250,126,65,67,164,112,156,65,98,164,176,56,67,31,133,178,65,219,185,47,67,135,22,200,65,106,188,40,67,6,
129,246,65,98,59,223,27,67,213,248,37,66,164,48,20,67,100,187,104,66,182,51,17,67,197,224,147,66,108,199,139,16,67,248,83,155,66,98,84,163,21,67,215,99,174,66,127,234,25,67,160,154,197,66,178,29,29,67,199,203,223,66,108,4,86,125,66,199,203,223,66,98,
74,12,134,66,135,150,193,66,205,76,144,66,135,86,167,66,35,155,156,66,35,219,146,66,98,6,1,152,66,217,78,125,66,252,105,145,66,246,40,86,66,219,121,133,66,213,248,48,66,98,172,28,114,66,174,71,10,66,160,154,78,66,162,69,210,65,156,68,33,66,201,118,181,
65,108,82,184,242,65,176,114,156,65,108,59,223,17,66,0,0,0,0,98,94,58,31,66,25,4,134,63,129,149,44,66,182,243,5,64,164,240,57,66,96,229,72,64,98,82,184,138,66,68,139,38,65,6,193,172,66,90,100,248,65,33,112,189,66,57,180,91,66,98,6,65,199,66,176,114,78,
66,127,170,209,66,236,81,71,66,51,115,220,66,236,81,71,66,98,219,57,233,66,236,81,71,66,100,123,245,66,248,83,81,66,68,107,0,67,8,172,99,66,99,101,0,0 };



static const unsigned char asmIcon[] = { 110,109,176,114,148,64,8,172,122,65,98,74,12,134,64,47,221,130,65,14,45,90,64,31,133,134,65,55,137,33,64,31,133,134,65,98,156,196,144,63,31,133,134,65,0,0,0,0,170,241,122,65,0,0,0,0,240,167,100,65,98,0,0,0,0,53,94,78,65,156,196,144,63,186,73,60,65,55,
137,33,64,186,73,60,65,98,14,45,90,64,186,73,60,65,74,12,134,64,154,153,67,65,176,114,148,64,240,167,78,65,108,102,102,190,64,240,167,78,65,108,102,102,190,64,139,108,41,65,98,102,102,190,64,80,141,11,65,72,225,238,64,190,159,230,64,223,79,21,65,190,
159,230,64,108,240,167,78,65,190,159,230,64,108,240,167,78,65,176,114,148,64,98,154,153,67,65,74,12,134,64,186,73,60,65,14,45,90,64,186,73,60,65,55,137,33,64,98,186,73,60,65,156,196,144,63,53,94,78,65,0,0,0,0,240,167,100,65,0,0,0,0,98,170,241,122,65,
0,0,0,0,31,133,134,65,156,196,144,63,31,133,134,65,55,137,33,64,98,31,133,134,65,14,45,90,64,47,221,130,65,74,12,134,64,8,172,122,65,176,114,148,64,108,8,172,122,65,190,159,230,64,108,199,75,169,65,190,159,230,64,108,199,75,169,65,176,114,148,64,98,156,
196,163,65,74,12,134,64,172,28,160,65,14,45,90,64,172,28,160,65,55,137,33,64,98,172,28,160,65,156,196,144,63,246,40,169,65,0,0,0,0,211,77,180,65,0,0,0,0,98,164,112,191,65,0,0,0,0,238,124,200,65,156,196,144,63,238,124,200,65,55,137,33,64,98,238,124,200,
65,14,45,90,64,254,212,196,65,74,12,134,64,211,77,191,65,176,114,148,64,108,211,77,191,65,190,159,230,64,108,150,67,235,65,190,159,230,64,108,150,67,235,65,176,114,148,64,98,106,188,229,65,74,12,134,64,123,20,226,65,14,45,90,64,123,20,226,65,55,137,33,
64,98,123,20,226,65,156,196,144,63,197,32,235,65,0,0,0,0,162,69,246,65,0,0,0,0,98,63,181,0,66,0,0,0,0,94,58,5,66,156,196,144,63,94,58,5,66,55,137,33,64,98,94,58,5,66,14,45,90,64,102,102,3,66,74,12,134,64,209,162,0,66,176,114,148,64,108,209,162,0,66,190,
159,230,64,108,86,14,14,66,190,159,230,64,98,37,134,21,66,190,159,230,64,129,149,27,66,80,141,11,65,129,149,27,66,139,108,41,65,108,129,149,27,66,240,167,78,65,108,119,190,33,66,240,167,78,65,98,68,139,35,66,154,153,67,65,252,169,38,66,186,73,60,65,57,
52,42,66,186,73,60,65,98,168,198,47,66,186,73,60,65,205,76,52,66,53,94,78,65,205,76,52,66,240,167,100,65,98,205,76,52,66,170,241,122,65,168,198,47,66,31,133,134,65,57,52,42,66,31,133,134,65,98,252,169,38,66,31,133,134,65,68,139,35,66,47,221,130,65,119,
190,33,66,8,172,122,65,108,129,149,27,66,8,172,122,65,108,129,149,27,66,199,75,169,65,108,119,190,33,66,199,75,169,65,98,68,139,35,66,156,196,163,65,252,169,38,66,172,28,160,65,57,52,42,66,172,28,160,65,98,168,198,47,66,172,28,160,65,205,76,52,66,246,
40,169,65,205,76,52,66,211,77,180,65,98,205,76,52,66,164,112,191,65,168,198,47,66,238,124,200,65,57,52,42,66,238,124,200,65,98,252,169,38,66,238,124,200,65,68,139,35,66,254,212,196,65,119,190,33,66,211,77,191,65,108,129,149,27,66,211,77,191,65,108,129,
149,27,66,150,67,235,65,108,119,190,33,66,150,67,235,65,98,68,139,35,66,106,188,229,65,252,169,38,66,123,20,226,65,57,52,42,66,123,20,226,65,98,168,198,47,66,123,20,226,65,205,76,52,66,197,32,235,65,205,76,52,66,162,69,246,65,98,205,76,52,66,63,181,0,
66,168,198,47,66,94,58,5,66,57,52,42,66,94,58,5,66,98,252,169,38,66,94,58,5,66,68,139,35,66,102,102,3,66,119,190,33,66,209,162,0,66,108,129,149,27,66,209,162,0,66,108,129,149,27,66,162,69,10,66,98,129,149,27,66,106,188,17,66,37,134,21,66,199,203,23,66,
86,14,14,66,199,203,23,66,108,209,162,0,66,199,203,23,66,108,209,162,0,66,119,190,33,66,98,102,102,3,66,68,139,35,66,94,58,5,66,252,169,38,66,94,58,5,66,57,52,42,66,98,94,58,5,66,168,198,47,66,63,181,0,66,205,76,52,66,162,69,246,65,205,76,52,66,98,197,
32,235,65,205,76,52,66,123,20,226,65,168,198,47,66,123,20,226,65,57,52,42,66,98,123,20,226,65,252,169,38,66,106,188,229,65,68,139,35,66,150,67,235,65,119,190,33,66,108,150,67,235,65,199,203,23,66,108,211,77,191,65,199,203,23,66,108,211,77,191,65,119,
190,33,66,98,254,212,196,65,68,139,35,66,238,124,200,65,252,169,38,66,238,124,200,65,57,52,42,66,98,238,124,200,65,168,198,47,66,164,112,191,65,205,76,52,66,211,77,180,65,205,76,52,66,98,246,40,169,65,205,76,52,66,172,28,160,65,168,198,47,66,172,28,160,
65,57,52,42,66,98,172,28,160,65,252,169,38,66,156,196,163,65,68,139,35,66,199,75,169,65,119,190,33,66,108,199,75,169,65,199,203,23,66,108,8,172,122,65,199,203,23,66,108,8,172,122,65,119,190,33,66,98,47,221,130,65,68,139,35,66,31,133,134,65,252,169,38,
66,31,133,134,65,57,52,42,66,98,31,133,134,65,168,198,47,66,170,241,122,65,205,76,52,66,240,167,100,65,205,76,52,66,98,53,94,78,65,205,76,52,66,186,73,60,65,168,198,47,66,186,73,60,65,57,52,42,66,98,186,73,60,65,252,169,38,66,154,153,67,65,68,139,35,
66,240,167,78,65,119,190,33,66,108,240,167,78,65,199,203,23,66,108,223,79,21,65,199,203,23,66,98,72,225,238,64,199,203,23,66,102,102,190,64,106,188,17,66,102,102,190,64,162,69,10,66,108,102,102,190,64,209,162,0,66,108,176,114,148,64,209,162,0,66,98,74,
12,134,64,102,102,3,66,14,45,90,64,94,58,5,66,55,137,33,64,94,58,5,66,98,156,196,144,63,94,58,5,66,0,0,0,0,63,181,0,66,0,0,0,0,162,69,246,65,98,0,0,0,0,197,32,235,65,156,196,144,63,123,20,226,65,55,137,33,64,123,20,226,65,98,14,45,90,64,123,20,226,65,
74,12,134,64,106,188,229,65,176,114,148,64,150,67,235,65,108,102,102,190,64,150,67,235,65,108,102,102,190,64,211,77,191,65,108,176,114,148,64,211,77,191,65,98,74,12,134,64,254,212,196,65,14,45,90,64,238,124,200,65,55,137,33,64,238,124,200,65,98,156,196,
144,63,238,124,200,65,0,0,0,0,164,112,191,65,0,0,0,0,211,77,180,65,98,0,0,0,0,246,40,169,65,156,196,144,63,172,28,160,65,55,137,33,64,172,28,160,65,98,14,45,90,64,172,28,160,65,74,12,134,64,156,196,163,65,176,114,148,64,199,75,169,65,108,102,102,190,
64,199,75,169,65,108,102,102,190,64,8,172,122,65,108,176,114,148,64,8,172,122,65,99,109,14,45,6,66,123,20,1,66,108,14,45,6,66,127,106,76,65,108,6,129,53,65,127,106,76,65,108,6,129,53,65,123,20,1,66,108,14,45,6,66,123,20,1,66,99,101,0,0 };

static const unsigned char optimizeIcon[] = { 110,109,12,2,148,65,141,151,197,65,108,70,182,101,65,164,112,164,65,108,250,126,180,65,154,153,69,65,108,70,182,101,65,166,155,132,64,108,12,2,148,65,0,0,0,0,108,129,149,246,65,233,38,69,65,108,41,92,246,65,154,153,69,65,108,129,149,246,65,49,8,70,65,
108,12,2,148,65,141,151,197,65,99,109,166,155,132,64,141,151,197,65,108,0,0,0,0,164,112,164,65,108,174,71,3,65,154,153,69,65,108,0,0,0,0,166,155,132,64,108,166,155,132,64,0,0,0,0,108,94,186,131,65,233,38,69,65,108,6,129,131,65,154,153,69,65,108,94,186,
131,65,49,8,70,65,108,166,155,132,64,141,151,197,65,99,101,0,0 };

}


using namespace juce;
using namespace hise;
using namespace snex;


void SnexSource::recompiled(WorkbenchData::Ptr wb)
{
	getParameterHandler().reset();
	getComplexDataHandler().reset();

	lastResult = wb->getLastResult().compileResult;

	if (auto objPtr = wb->getLastResult().mainClassPtr)
	{
		objPtr->initialiseObjectStorage(object);

		if (lastResult.wasOk())
			lastResult = getCallbackHandler().recompiledOk(objPtr);

		if (lastResult.wasOk())
			lastResult = getParameterHandler().recompiledOk(objPtr);

		if (lastResult.wasOk())
			lastResult = getComplexDataHandler().recompiledOk(objPtr);

		lastCompiledObject = getWorkbench()->getLastJitObject();
	}

	for (auto l : compileListeners)
	{
		l->wasCompiled(lastResult.wasOk());
	}
}

void SnexSource::logMessage(WorkbenchData::Ptr wb, int level, const String& s)
{
	if (wb->getGlobalScope().isDebugModeEnabled() && level > 4)
	{
		if (auto p = dynamic_cast<Processor*>(parentNode->getScriptProcessor()))
		{
			parentNode->getScriptProcessor()->getMainController_()->writeToConsole(s, 0);
		}
	}
}

void SnexSource::ParameterHandler::addParameterCode(String& code)
{
	using namespace snex::cppgen;

	cppgen::Base c(cppgen::Base::OutputType::AddTabs);

	c.addComment("Adding parameter methods", cppgen::Base::CommentType::RawWithNewLine);

	String fDef;
	fDef << "void initMainObject(" << parent.getWorkbench()->getInstanceId() << "& obj)";
	c << fDef;

	{
		cppgen::StatementBlock sb(c);

		for (auto p : parameterTree)
		{
			String def;
			def << "obj.setParameter<" << p.getParent().indexOf(p) << ">(";
			def << Types::Helpers::getCppValueString((double)p[PropertyIds::Value]);
			def << ");";

			c << def;
			c.addComment(p[PropertyIds::ID].toString(), cppgen::Base::CommentType::AlignOnSameLine);
		}
	}

	code << c.toString();

	DBG(code);
}

Result SnexSource::ComplexDataHandler::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
{
	ExternalData::forEachType([this, objectClass](ExternalData::DataType t)
		{
			auto typeTree = getDataRoot().getChild((int)t);
			auto id = ExternalData::getNumIdentifier(t);
			int numRequired = (int)objectClass->getInternalProperty(id, var(0));

			while (numRequired > typeTree.getNumChildren())
				addOrRemoveDataFromUI(t, true);

			for (int i = 0; i < getNumDataObjects(t); i++)
				getComplexBaseType(t, i)->getUpdater().addEventListener(this);
		});

	if (!hasComplexData())
		return Result::ok();

	auto r = ComplexDataHandlerLight::recompiledOk(objectClass);

	if (!r.wasOk())
		return r;

	callExternalDataForAll(*this, *this);

	return r;
}

void SnexSource::ComplexDataHandler::initialise(NodeBase* n)
{
	dataTree = n->getValueTree().getOrCreateChildWithName(PropertyIds::ComplexData, n->getUndoManager());

	ExternalData::forEachType([this, n](ExternalData::DataType t)
		{
			auto name = ExternalData::getDataTypeName(t);

			auto typeTree = dataTree.getOrCreateChildWithName(name + "s", n->getUndoManager());

			dataListeners[(int)t].setCallback(typeTree, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(SnexSource::ComplexDataHandler::dataAddedOrRemoved));
		});

}

void SnexSource::ComplexDataHandler::addOrRemoveDataFromUI(ExternalData::DataType t, bool shouldAdd)
{
	auto pTree = dataListeners[(int)t].getParentTree();

	if (shouldAdd)
	{
		ValueTree newEntry(ExternalData::getDataTypeName(t));
		newEntry.setProperty(PropertyIds::Index, -1, nullptr);
		newEntry.setProperty(PropertyIds::EmbeddedData, "", nullptr);
		pTree.addChild(newEntry, -1, parent.parentNode->getUndoManager());
	}
	else
	{
		pTree.removeChild(pTree.getNumChildren() - 1, parent.parentNode->getUndoManager());
	}
}

void SnexSource::ComplexDataHandler::dataAddedOrRemoved(ValueTree v, bool wasAdded)
{
	auto typeId = v.getType().toString();

	ExternalData::DataType t = ExternalData::DataType::numDataTypes;

	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::Table)) t = ExternalData::DataType::Table;
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::SliderPack)) t = ExternalData::DataType::SliderPack;
	if (typeId == ExternalData::getDataTypeName(ExternalData::DataType::AudioFile)) t = ExternalData::DataType::AudioFile;

	if (t != ExternalData::DataType::numDataTypes)
	{
		if (wasAdded)
			getComplexBaseType(t, v.getParent().indexOf(v));
		else
		{
			for (int i = 0; i < getNumDataObjects(t); i++)
			{
				if (dynamic_cast<data::pimpl::dynamic_base*>(getDynamicDataHolder(t, i))->getValueTree() == v)
				{
					removeDataObject(t, i);
					return;
				}
			}

			jassertfalse;
		}

	}
}

int SnexSource::ComplexDataHandler::getNumDataObjects(ExternalData::DataType t) const
{
	switch (t)
	{
	case snex::ExternalData::DataType::Table: return tables.size();
	case snex::ExternalData::DataType::AudioFile: return audioFiles.size();
	case snex::ExternalData::DataType::SliderPack: return sliderPacks.size();
	}
}

hise::Table* SnexSource::ComplexDataHandler::getTable(int index)
{
	if (isPositiveAndBelow(index, tables.size()))
	{
		return tables[index]->getTable(0);
	}

	auto n = new data::dynamic::table(*this, index);
	n->initialise(getNode());
	tables.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				for (auto l : safeThis.get()->compileListeners)
				{
					if (l != nullptr)
						l->complexDataAdded(ExternalData::DataType::Table, index);
				}
			}
		});

	return n->getTable(0);
}

hise::SliderPackData* SnexSource::ComplexDataHandler::getSliderPack(int index)
{
	if (isPositiveAndBelow(index, sliderPacks.size()))
		return sliderPacks[index]->getSliderPack(0);

	auto n = new data::dynamic::sliderpack(*this, index);
	n->initialise(getNode());
	sliderPacks.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				for (auto l : safeThis.get()->compileListeners)
				{
					if (l != nullptr)
						l->complexDataAdded(ExternalData::DataType::SliderPack, index);
				}
			}
		});

	return n->getSliderPack(0);
}

hise::MultiChannelAudioBuffer* SnexSource::ComplexDataHandler::getAudioFile(int index)
{
	if (isPositiveAndBelow(index, audioFiles.size()))
		return audioFiles[index]->getAudioFile(0);

	auto n = new data::dynamic::audiofile(*this, index);
	n->initialise(getNode());
	audioFiles.add(n);

	WeakReference<SnexSource> safeThis(&parent);

	MessageManager::callAsync([safeThis, index]()
		{
			if (safeThis != nullptr)
			{
				for (auto l : safeThis.get()->compileListeners)
				{
					if (l != nullptr)
						l->complexDataAdded(ExternalData::DataType::AudioFile, index);
				}
			}
		});

	return n->getAudioFile(0);
}

bool SnexSource::ComplexDataHandler::removeDataObject(ExternalData::DataType t, int index)
{
	ExternalData empty;

	auto i = getAbsoluteIndex(t, index);

	ScopedPointer<ExternalDataHolder> pendingDelete;

	{
		SimpleReadWriteLock::ScopedWriteLock l(getComplexBaseType(t, index)->getDataLock());
		setExternalData(empty, i);

		switch (t)
		{
		case ExternalData::DataType::Table: pendingDelete = tables.removeAndReturn(index); break;
		case ExternalData::DataType::SliderPack: pendingDelete = sliderPacks.removeAndReturn(index); break;
		case ExternalData::DataType::AudioFile: pendingDelete = audioFiles.removeAndReturn(index); break;
		}
	}

	return true;
}



snex::ExternalDataHolder* SnexSource::ComplexDataHandler::getDynamicDataHolder(snex::ExternalData::DataType t, int index)
{
	switch (t)
	{
	case snex::ExternalData::DataType::Table: return tables[index];
	case snex::ExternalData::DataType::SliderPack: return sliderPacks[index];
	case snex::ExternalData::DataType::AudioFile: return audioFiles[index];
	}
}

SnexSource::SnexParameter::SnexParameter(SnexSource* n, NodeBase* parent, ValueTree dataTree) :
	Parameter(parent, dataTree),
	pIndex(dataTree.getParent().indexOf(dataTree)),
	snexSource(n)
{
	auto& pHandler = n->getParameterHandler();

	switch (pIndex)
	{
	case 0: p = parameter::inner<SnexSource::ParameterHandler, 0>(pHandler); break;
	case 1: p = parameter::inner<SnexSource::ParameterHandler, 1>(pHandler); break;
	case 2: p = parameter::inner<SnexSource::ParameterHandler, 2>(pHandler); break;
	case 3: p = parameter::inner<SnexSource::ParameterHandler, 3>(pHandler); break;
	case 4: p = parameter::inner<SnexSource::ParameterHandler, 4>(pHandler); break;
	case 5: p = parameter::inner<SnexSource::ParameterHandler, 5>(pHandler); break;
	case 6: p = parameter::inner<SnexSource::ParameterHandler, 6>(pHandler); break;
	case 7: p = parameter::inner<SnexSource::ParameterHandler, 7>(pHandler); break;
	case 8: p = parameter::inner<SnexSource::ParameterHandler, 8>(pHandler); break;
	}

	auto ndb = new parameter::dynamic_base(p);
	ndb->dataTree = dataTree;
	setCallbackNew(ndb);

	for (auto pTree : parent->getParameterTree())
	{
		if (pTree[PropertyIds::ID] == dataTree[PropertyIds::ID])
		{
			treeInNetwork = pTree;
			break;
		}
	}

	if (!treeInNetwork.isValid())
	{
		treeInNetwork = dataTree.createCopy();
		parent->getParameterTree().addChild(treeInNetwork, -1, parent->getUndoManager());
	}

	setTreeWithValue(treeInNetwork);

	auto ids = RangeHelpers::getRangeIds();
	ids.add(PropertyIds::ID);

	syncer.setPropertiesToSync(dataTree, treeInNetwork, ids, parent->getUndoManager());

	parentValueUpdater.setCallback(treeInNetwork, { PropertyIds::Value }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(SnexSource::SnexParameter::sendValueChangeToParentListeners));
}

void SnexSource::SnexParameter::sendValueChangeToParentListeners(Identifier id, var newValue)
{
	auto d = (double)newValue;

	for (auto l : snexSource->compileListeners)
	{
		if (l != nullptr)
			l->parameterChanged(pIndex, d);
	}
}

snex::jit::FunctionData SnexSource::HandlerBase::getFunctionAsObjectCallback(const String& id)
{
	if (auto wb = parent.getWorkbench())
	{
		if (auto obj = wb->getLastResult().mainClassPtr)
		{
			auto f = obj->getNonOverloadedFunction(Identifier(id));

			if (f.isResolved())
			{
				addObjectPtrToFunction(f);
				return f;
			}
		}
	}

	return {};
}

void SnexSource::HandlerBase::addObjectPtrToFunction(FunctionData& f)
{
	jassert(f.isResolved());
	f.object = obj.getObjectPtr();
}

juce::Result SnexSource::ParameterHandlerLight::recompiledOk(snex::jit::ComplexType::Ptr objectClass)
{
	using namespace snex::jit;

	snex::jit::FunctionClass::Ptr fc = objectClass->getFunctionClass();
	Array<FunctionData> matches;
	fc->addMatchingFunctions(matches, fc->getClassName().getChildId("setParameter"));

	SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

	for (int i = 0; i < matches.size(); i++)
	{
		auto m = matches[i];

		if (!m.templateParameters[0].constantDefined)
			matches.remove(i--);
	}

	for (int index = 0; index < matches.size(); index++)
	{
		pFunctions[index] = matches[index];

		if (pFunctions[index].templateParameters[0].constant != index)
			return Result::fail("Template index mismatch");

		addObjectPtrToFunction(pFunctions[index]);
		pFunctions[index].callVoid(lastValues[index]);
	}

	return Result::ok();
}

SnexComplexDataDisplay::SnexComplexDataDisplay(SnexSource* s) :
	source(s)
{
	setName("Complex Data Editor");
	source->addCompileListener(this);
	rebuildEditors();
}



SnexComplexDataDisplay::~SnexComplexDataDisplay()
{
	source->removeCompileListener(this);
}

void SnexComplexDataDisplay::rebuildEditors()
{
	auto updater = source->getParentNode()->getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
	auto undoManager = source->getParentNode()->getScriptProcessor()->getMainController_()->getControlUndoManager();

	auto& dataHandler = source->getComplexDataHandler();

	auto t = snex::ExternalData::DataType::Table;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::table_editor_without_mod(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::SliderPack;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		auto d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::sliderpack_editor_without_mod(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	t = snex::ExternalData::DataType::AudioFile;

	for (int i = 0; i < dataHandler.getNumDataObjects(t); i++)
	{
		data::pimpl::dynamic_base* d = dynamic_cast<data::pimpl::dynamic_base*>(dataHandler.getDynamicDataHolder(t, i));
		auto e = new data::ui::audiofile_editor(updater, d);
		addAndMakeVisible(e);
		editors.add(e);
	}

	setSize(512, editors.size() * 100);
}

SnexMenuBar::SnexMenuBar(SnexSource* s) :
	newButton("new", this, f),
	popupButton("popup", this, f),
	editButton("edit", this, f),
	addButton("add", this, f),
	deleteButton("delete", this, f),
	asmButton("asm", this, f),
	debugButton("debug", this, f),
	optimizeButton("optimize", this, f),
	source(s),
	cdp(s)
{
	s->addCompileListener(this);

	addAndMakeVisible(newButton);
	addAndMakeVisible(classSelector);
	addAndMakeVisible(popupButton);
	addAndMakeVisible(editButton);
	addAndMakeVisible(debugButton);
	addAndMakeVisible(optimizeButton);
	addAndMakeVisible(asmButton);
	classSelector.setLookAndFeel(&plaf);
	classSelector.addListener(this);

	addAndMakeVisible(addButton);
	addAndMakeVisible(deleteButton);
	addAndMakeVisible(cdp);

	editButton.setToggleModeWithColourChange(true);
	debugButton.setToggleModeWithColourChange(true);

	rebuildComboBoxItems();
	refreshButtonState();

	auto wb = static_cast<snex::ui::WorkbenchManager*>(source->getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());
	wb->addListener(this);
	workbenchChanged(wb->getCurrentWorkbench());

	GlobalHiseLookAndFeel::setDefaultColours(classSelector);
}

void SnexMenuBar::wasCompiled(bool ok)
{
	iconColour = ok ? Colours::green : Colours::red;
	iconColour = iconColour.withSaturation(0.2f).withAlpha(0.8f);
	repaint();
}

void SnexMenuBar::parameterChanged(int snexParameterId, double newValue)
{

}

SnexMenuBar::~SnexMenuBar()
{
	auto wb = static_cast<snex::ui::WorkbenchManager*>(source->getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());
	wb->removeListener(this);

	source->removeCompileListener(this);
}

void SnexMenuBar::workbenchChanged(snex::ui::WorkbenchData::Ptr newWb)
{
	editButton.setToggleStateAndUpdateIcon(source->getWorkbench() == newWb && newWb != nullptr, true);
	debugButton.setToggleStateAndUpdateIcon(newWb != nullptr && newWb->getGlobalScope().isDebugModeEnabled(), true);
}

void SnexMenuBar::complexDataAdded(snex::ExternalData::DataType t, int index)
{

}

void SnexMenuBar::rebuildComboBoxItems()
{
	classSelector.clear(dontSendNotification);
	classSelector.addItemList(source->getAvailableClassIds(), 1);

	if (auto w = source->getWorkbench())
		classSelector.setText(w->getInstanceId().toString(), dontSendNotification);
}

void SnexMenuBar::refreshButtonState()
{
	bool shouldBeEnabled = source->getWorkbench() != nullptr;

	addButton.setEnabled(shouldBeEnabled);
	editButton.setEnabled(shouldBeEnabled);
	deleteButton.setEnabled(shouldBeEnabled);
	popupButton.setEnabled(shouldBeEnabled);
	asmButton.setEnabled(shouldBeEnabled);
	debugButton.setEnabled(shouldBeEnabled);
	optimizeButton.setEnabled(shouldBeEnabled);
}

void SnexMenuBar::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	source->setClass(classSelector.getText());
	refreshButtonState();
}

void SnexMenuBar::buttonClicked(Button* b)
{
	if (b == &newButton)
	{
		auto name = PresetHandler::getCustomName(source->getId(), "Enter the name for the SNEX class file");

		if (name.isNotEmpty())
		{
			source->setClass(name);
			rebuildComboBoxItems();
			refreshButtonState();
		}
	}

	if (b == &popupButton)
	{
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Show popup editor", source->getWorkbench() != nullptr);
		m.addItem(2, "Show complex data", source->getComplexDataHandler().hasComplexData());

		if (auto r = m.show())
		{
			Component* c;

			if (r == 1)
			{
				c = new snex::jit::SnexPlayground(source->getWorkbench());
				c->setSize(900, 800);
			}
			else
			{
				c = new SnexComplexDataDisplay(source);
			}

			auto sp = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();
			auto area = sp->getLocalArea(this, getLocalBounds());
			sp->setCurrentModalWindow(c, area);

			//auto b = ft->getLocalArea(&popupButton, popupButton.getLocalBounds());

			//ft->showComponentInRootPopup(c, &popupButton, b.getCentre());
		}
	}

	if (b == &addButton)
	{
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Add Parameter");

		ExternalData::forEachType([&m](ExternalData::DataType t)
			{
				m.addItem(12 + (int)t, "Add " + ExternalData::getDataTypeName(t));
			});

		if (auto r = m.show())
		{
			if (r == 1)
			{
				auto n = PresetHandler::getCustomName("Parameter");

				if (n.isNotEmpty())
					source->getParameterHandler().addNewParameter(parameter::data(n, { 0.0, 1.0 }));
			}
			else
			{
				auto t = (ExternalData::DataType)(r - 12);
				source->getComplexDataHandler().addOrRemoveDataFromUI(t, true);
			}
		}
	}
	if (b == &deleteButton)
	{
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Remove last Parameter");

		ExternalData::forEachType([&m](ExternalData::DataType t)
			{
				m.addItem(12 + (int)t, "Remove " + ExternalData::getDataTypeName(t));
			});

		if (auto r = m.show())
		{
			if (r == 1)
				source->getParameterHandler().removeLastParameter();
			else
			{
				auto t = (ExternalData::DataType)(r - 12);
				source->getComplexDataHandler().addOrRemoveDataFromUI(t, false);
			}
		}
	}

	if (b == &debugButton)
	{
		source->getWorkbench()->getGlobalScope().setDebugMode(b->getToggleState());
	}

	if (b == &editButton)
	{
		auto& manager = dynamic_cast<BackendProcessor*>(findParentComponentOfClass<FloatingTile>()->getMainController())->workbenches;

		if (b->getToggleState())
			manager.setCurrentWorkbench(source->getWorkbench(), false);
		else
			manager.resetToRoot();
	}
}

void SnexMenuBar::paint(Graphics& g)
{
	g.setColour(Colours::black.withAlpha(0.2f));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 1.0f);
	g.setColour(iconColour);
	g.fillPath(snexIcon);
}

void SnexMenuBar::resized()
{
	auto b = getLocalBounds().reduced(1);
	auto h = getHeight();

	newButton.setBounds(b.removeFromLeft(h));
	classSelector.setBounds(b.removeFromLeft(128));
	popupButton.setBounds(b.removeFromLeft(h));
	editButton.setBounds(b.removeFromLeft(h));
	debugButton.setBounds(b.removeFromLeft(h));
	asmButton.setBounds(b.removeFromLeft(h));
	optimizeButton.setBounds(b.removeFromLeft(h));

	b.removeFromLeft(10);
	addButton.setBounds(b.removeFromLeft(h));
	deleteButton.setBounds(b.removeFromLeft(h));
	cdp.setBounds(b.removeFromLeft(90));

	snexIcon = f.createPath("snex");
	f.scalePath(snexIcon, getLocalBounds().removeFromRight(80).toFloat().reduced(2.0f));
}

SnexMenuBar::ComplexDataPopupButton::ComplexDataPopupButton(SnexSource* s) :
	Button("TSA"),
	source(s)
{
	t = s->getComplexDataHandler().getDataRoot();
	l.setTypesToWatch({ Identifier("Tables"), Identifier("SliderPacks"), Identifier("AudioFiles") });
	l.setCallback(t, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(ComplexDataPopupButton::update));

	setClickingTogglesState(true);

	onClick = [this]()
	{
		auto ft = findParentComponentOfClass<FloatingTile>();

		if (getToggleState())
		{
			ft->showComponentInRootPopup(nullptr, nullptr, {});
		}
		else
		{
			Component* c = new SnexComplexDataDisplay(source);

			auto nc = findParentComponentOfClass<NodeComponent>();
			auto lb = nc->getLocalBounds();
			auto fBounds = ft->getLocalBounds();

			auto la = ft->getLocalArea(nc, lb);

			auto showAbove = la.getY() > fBounds.getHeight() / 2;

			ft->showComponentInRootPopup(c, nc, { lb.getCentreX(), showAbove ? 0 : lb.getBottom() });
		}
	};
}

juce::Path SnexMenuBar::Factory::createPath(const String& url) const
{
	if (url == "snex")
	{
		snex::ui::SnexPathFactory f;
		return f.createPath(url);
	}

	Path p;

	LOAD_PATH_IF_URL("new", SampleMapIcons::newSampleMap);
	LOAD_PATH_IF_URL("edit", HiBinaryData::SpecialSymbols::scriptProcessor);
	LOAD_PATH_IF_URL("popup", HiBinaryData::ProcessorEditorHeaderIcons::popupShape);
	LOAD_PATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_PATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_PATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	LOAD_PATH_IF_URL("asm", SnexIcons::asmIcon);
	LOAD_PATH_IF_URL("debug", SnexIcons::bugIcon);
	//LOAD_PATH_IF_URL("optimize", SnexIcons::optimizeIcon);

	return p;
}

}

