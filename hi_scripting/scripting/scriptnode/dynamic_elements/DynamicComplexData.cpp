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

namespace data
{
namespace pimpl
{

using namespace snex;

dynamic_base::dynamic_base(data::base& b, ExternalData::DataType t, int index_) :
	dt(t),
	index(index_),
	dataNode(&b)
{

}

dynamic_base::~dynamic_base()
{

}

void dynamic_base::initialise(NodeBase* p)
{
	parentNode = p;

	auto dataTree = parentNode->getValueTree().getOrCreateChildWithName(PropertyIds::ComplexData, parentNode->getUndoManager());
	auto dataName = ExternalData::getDataTypeName(dt);
	auto typeTree = dataTree.getOrCreateChildWithName(Identifier(dataName + "s"), parentNode->getUndoManager());

	if (typeTree.getNumChildren() <= index)
	{
		for (int i = 0; i <= index; i++)
		{
			ValueTree newChild(dataName);
			newChild.setProperty(PropertyIds::Index, -1, nullptr);
			newChild.setProperty(PropertyIds::EmbeddedData, -1, nullptr);
			typeTree.addChild(newChild, -1, parentNode->getUndoManager());
		}
	}

	cTree = typeTree.getChild(index);
	jassert(cTree.isValid());

	dataUpdater.setCallback(cTree, { PropertyIds::Index, PropertyIds::EmbeddedData }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(dynamic_base::updateData));

	getInternalData()->setGlobalUIUpdater(parentNode->getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	getInternalData()->getUpdater().addEventListener(this);

	setIndex(getIndex(), true);
}

void dynamic_base::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType d, var data)
{
	if (d == ComplexDataUIUpdaterBase::EventType::ContentChange ||
		d == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
	{
		if (currentlyUsedData == getInternalData())
		{
			auto s = getInternalData()->toBase64String();
			cTree.setProperty(PropertyIds::EmbeddedData, s, parentNode->getUndoManager());
		}

		if(d == ComplexDataUIUpdaterBase::EventType::ContentRedirected)
			updateExternalData();
	}
}

void dynamic_base::updateExternalData()
{
	if (currentlyUsedData != nullptr)
	{
		auto updater = parentNode != nullptr ? parentNode->getScriptProcessor()->getMainController_()->getGlobalUIUpdater() : nullptr;
		auto um = parentNode != nullptr ? parentNode->getUndoManager() : nullptr;

		ExternalData ed(currentlyUsedData, 0);

		{
			SimpleReadWriteLock::ScopedWriteLock sl(currentlyUsedData->getDataLock());
			setExternalData(*dataNode, ed, 0);
		}
		
		sourceWatcher.setNewSource(currentlyUsedData);
	}
}

void dynamic_base::updateData(Identifier id, var newValue)
{
	if (id == PropertyIds::Index)
	{
		setIndex((int)newValue, false);
	}
	if (id == PropertyIds::EmbeddedData)
	{
		auto b64 = newValue.toString();

		if (getIndex() == -1 && b64.isNotEmpty())
		{
			auto thisString = getInternalData()->toBase64String();

			if (thisString.compare(b64) != 0)
				getInternalData()->fromBase64String(b64);
		}
	}
	
}

int dynamic_base::getNumDataObjects(ExternalData::DataType t) const
{
	return (int)(t == dt);
}

void dynamic_base::setExternalData(data::base& n, const ExternalData& b, int index)
{
	n.setExternalData(b, index);
}

void dynamic_base::setIndex(int index, bool forceUpdate)
{
	ComplexDataUIBase* newData = nullptr;

	if (index != -1 && parentNode != nullptr)
	{
		if (auto h = parentNode->getRootNetwork()->getExternalDataHolder())
			newData = h->getComplexBaseType(dt, index);
	}
		
	if (newData == nullptr)
		newData = getInternalData();
	
	if (currentlyUsedData != newData || forceUpdate)
	{
		if (currentlyUsedData != nullptr)
			currentlyUsedData->getUpdater().removeEventListener(this);

		currentlyUsedData = newData;

		if (currentlyUsedData != nullptr)
			currentlyUsedData->getUpdater().addEventListener(this);

		updateExternalData();
	}
}

}

void dynamic::sliderpack::initialise(NodeBase* p)
{
	dynamicT<hise::SliderPackData>::initialise(p);

	numParameterSyncer.setCallback(getValueTree(), { PropertyIds::NumParameters }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(sliderpack::updateNumParameters));
}

void dynamic::sliderpack::updateNumParameters(Identifier id, var newValue)
{
	if (auto sp = dynamic_cast<SliderPackData*>(currentlyUsedData))
	{
		sp->setNumSliders((int)newValue);
	}
}

namespace ui
{
namespace pimpl
{

	editor_base::editor_base(ObjectType* b, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<ObjectType>(b, updater)
	{
		b->sourceWatcher.addSourceListener(this);

		// nothing to do...
		stop();
	}

	editor_base::~editor_base()
	{
		if (getObject() != nullptr)
			getObject()->sourceWatcher.removeSourceListener(this);
	}

	void editor_base::timerCallback()
	{
		jassertfalse;
	}

}

}
}

}

