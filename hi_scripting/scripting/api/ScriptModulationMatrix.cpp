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

#include "hi_core/hi_dsp/modules/ModulationMatrixTools.h"

namespace hise { using namespace juce;


ScriptingApi::Content::ScriptSlider::MatrixCableConnection::QueryFunction::QueryFunction(
	MatrixCableConnection* c):
	connection(c)
{}

ModulationDisplayValue ScriptingApi::Content::ScriptSlider::MatrixCableConnection::QueryFunction::getDisplayValue(
	Processor* p, double nv, NormalisableRange<double> nr) const
{
	if(connection != nullptr)
		return connection->getDisplayValue(nv, nr);

	return {};
}

ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::AuxTarget::AuxTarget(Target& p,
	const String& sourceId):
	parentTarget(&p)
{
	auxIntensity = (double)p.connection[MatrixIds::AuxIntensity];
	FloatSanitizers::sanitizeDoubleNumber(auxIntensity);

	auto rm = routing::GlobalRoutingManager::Helpers::getOrCreate(p.parent.getMainController());
				
	cable = dynamic_cast<CableType*>(rm->getSlotBase(sourceId, C).get());

	if(cable != nullptr)
		cable->addTarget(this);
}

ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::AuxTarget::~AuxTarget()
{
	if(cable != nullptr)
	{
		cable->removeTarget(this);
		cable = nullptr;
	}
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::AuxTarget::sendValue(double v)
{
	lastAuxValue = jlimit(0.0, 1.0, v);

	if(parentTarget != nullptr && parentTarget->isVoiceStart)
	{
		parentTarget->parent.calculateNewModValue();
	}
}

double ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::AuxTarget::getAuxValue() const
{
	auto a = 1.0 - auxIntensity;
	return a + auxIntensity * lastAuxValue;
}

ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::Target(MatrixCableConnection& parent_,
	const ValueTree& connection_):
	SimpleTimer(parent_.getMainController()->getGlobalUIUpdater(), false),
	connection(connection_),
	parent(parent_),
	rb(new SimpleRingBuffer()),
	sourceIndex(connection[MatrixIds::SourceIndex])
{
			
	rb->setGlobalUIUpdater(parent.getMainController()->getGlobalUIUpdater());
	rb->setProperty("BufferLength", 44110 / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR);
	jassert(connection.getType() == MatrixIds::Connection);

	propertyListener.setCallback(connection, 
	                             MatrixIds::Helpers::getWatchableIds(), 
	                             valuetree::AsyncMode::Synchronously,
	                             BIND_MEMBER_FUNCTION_2(Target::onPropertyUpdate));
}

ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::~Target()
{
	if(sourceCable != nullptr)
		sourceCable->removeTarget(this);

	sourceCable = nullptr;
	auxTarget = nullptr;
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::sendValue(double v)
{
	lastModValue = jlimit(0.0, 1.0, v);

	if(inverted)
		lastModValue = 1.0f - lastModValue;

	parent.calculateNewModValue();
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::timerCallback()
{
	jassert(isVoiceStart);

	auto now = Time::getMillisecondCounter();
	auto sr = parent.getMainController()->getMainSynthChain()->getSampleRate();
	auto deltaSeconds = jmin((double)(now - lastMs) * 0.001, 0.03);

	lastMs = now;

	auto numSamplesToPush = (sr * deltaSeconds) / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	if(rb != nullptr)
	{
		rb->write(voiceStartValue, numSamplesToPush);
	}
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::Target::onPropertyUpdate(const Identifier& id,
	const var& newValue)
{
	if(id == MatrixIds::Intensity)
		intensity = jlimit(-1.0, 1.0, (double)newValue);
	if(id == MatrixIds::Mode)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(parent.listLock);
		tm = (modulation::TargetMode)(int)newValue;
		parent.rebuildTargets();
	}
	if(id == MatrixIds::Inverted)
		inverted = (bool)newValue;

	if(id == MatrixIds::AuxIndex)
	{
		auto idx = (int)newValue;

		if(idx == -1)
		{
			auxTarget = nullptr;
		}
		else
		{
			auxTarget = new AuxTarget(*this, parent.sourceNames[idx]);
		}
	}
	if(id == MatrixIds::SourceIndex)
	{
		if(sourceCable != nullptr)
		{
			sourceCable->removeTarget(this);
			sourceCable = nullptr;
		}

		sourceIndex = (int)newValue;

		auto sourceId = parent.sourceNames[sourceIndex];

		auto sourceMod = ProcessorHelpers::getFirstProcessorWithName(parent.getMainController()->getMainSynthChain(), sourceId);
		isVoiceStart = dynamic_cast<VoiceStartModulator*>(sourceMod) != nullptr;

		if(isVoiceStart)
			start();
		else
			stop();

		auto rm = routing::GlobalRoutingManager::Helpers::getOrCreate(parent.getMainController());
		sourceCable = dynamic_cast<CableType*>(rm->getSlotBase(sourceId, C).get());

		if(sourceCable != nullptr)
			sourceCable->addTarget(this);
	}
	if(id == MatrixIds::AuxIntensity)
	{
		if(auxTarget != nullptr)
		{
			auxTarget->auxIntensity = (double)newValue;
			FloatSanitizers::sanitizeDoubleNumber(auxTarget->auxIntensity);
		}
	}
			
	parent.calculateNewModValue();
}

ScriptingApi::Content::ScriptSlider::MatrixCableConnection::MatrixCableConnection(ScriptSlider& slider,
                                                                                  const ValueTree& matrixData, const String& targetId_):
	MatrixConnectionBase(slider, matrixData, targetId_)
{
	setTargetSlider(&slider);

	if(gc != nullptr)
	{
		auto gainChain = gc->getChildProcessor(ModulatorSynth::GainModulation);
		auto rm = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(getMainController());

		for(int i = 0; i < gainChain->getNumChildProcessors(); i++)
		{
			auto mod = dynamic_cast<Modulator*>(gainChain->getChildProcessor(i));
			auto id = mod->getId();
			auto c = rm->getSlotBase(id, routing::GlobalRoutingManager::SlotBase::SlotType::Cable);
			gc->connectToGlobalCable(mod, var(c.get()), true);
			globalModCables.add(var(c.get()));
		}
	}

	MatrixIds::Helpers::fillModSourceList(getMainController(), sourceNames);

	auto componentIndex = slider.getScriptProcessor()->getScriptingContent()->getComponentIndex(&slider);
	slider.getScriptProcessor()->setModulationDisplayQueryFunction(componentIndex, dynamic_cast<Processor*>(parent->getScriptProcessor()), new QueryFunction(this));

	connectionListener.setCallback(matrixData, 
	                               valuetree::AsyncMode::Synchronously, 
	                               BIND_MEMBER_FUNCTION_2(MatrixCableConnection::onUpdate));

	sourceTargetListener.setCallback(matrixData, 
	                                 { MatrixIds::TargetId }, 
	                                 valuetree::AsyncMode::Asynchronously,
	                                 BIND_MEMBER_FUNCTION_2(MatrixCableConnection::onSourceTargetChange));
}

ScriptingApi::Content::ScriptSlider::MatrixCableConnection::~MatrixCableConnection()
{
	SimpleReadWriteLock::ScopedWriteLock sl(listLock);

	scaleTargets.clear();
	addTargets.clear();
	allTargets.clear();
	globalModCables.clear();
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::setTargetSlider(ScriptSlider* newParent)
{
	jassert(newParent != nullptr);
	parent = newParent;
	sliderRange = (RangeHelpers::getDoubleRange(parent->getPropertyValueTree(), RangeHelpers::IdSet::ScriptComponents));
}

MatrixIds::Helpers::IntensityTextConverter::ConstructData ScriptingApi::Content::ScriptSlider::MatrixCableConnection::
createIntensityConverter(int sourceIndex)
{
	MatrixIds::Helpers::IntensityTextConverter::ConstructData cd;
	cd.parameterIndex = parent->getScriptProcessor()->getScriptingContent()->getComponentIndex(parent);
	cd.p = dynamic_cast<Processor*>(parent->getScriptProcessor());
	auto rng = RangeHelpers::getDoubleRange(parent->getPropertyValueTree(), RangeHelpers::IdSet::ScriptComponents);
	cd.inputRange = rng.rng;
	cd.prettifierMode = parent->getScriptObjectProperty(ScriptSlider::Mode);

	for(auto t: allTargets)
	{
		if(t->sourceIndex == sourceIndex)
		{
			cd.tm = t->tm;
			return cd;
		}
	}

	return cd;
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::rebuildTargets()
{
	Target::List newScale, newAdd;

	for(auto t: allTargets)
	{
		if(t->tm == modulation::TargetMode::Gain)
			newScale.add(t);
		else
			newAdd.add(t);
	}

	{
		scaleTargets.swapWith(newScale);
		addTargets.swapWith(newAdd);
	}
}

SimpleRingBuffer::Ptr ScriptingApi::Content::ScriptSlider::MatrixCableConnection::getDisplayBuffer(int sourceIndex)
{
	for(auto t: allTargets)
	{
		if(t->sourceIndex == sourceIndex)
			return t->rb;
	}

	return nullptr;
}

ModulationDisplayValue ScriptingApi::Content::ScriptSlider::MatrixCableConnection::getDisplayValue(double nv,
	NormalisableRange<double> nr)
{
	ModulationDisplayValue mv;

	auto sv = nr.convertTo0to1(nv);

	auto normValue = sv;
		
	auto lastValue = mv.getNormalisedModulationValue();

	double min = normValue;
	double max = normValue;

	mv.modulationActive = !scaleTargets.isEmpty() || !addTargets.isEmpty();
	mv.normalisedValue = normValue;
		
	for(auto s: scaleTargets)
	{
		auto i = s->intensity;
		auto a = 1.0f - i;
		normValue *= a + i * s->lastModValue;
		min = jmin(min, a * sv);
	}
		
	mv.scaledValue = normValue;

	for(auto a: addTargets)
	{
		auto modValue = a->lastModValue;
		max += a->intensity;

		if(a->tm == modulation::TargetMode::Bipolar)
		{
			modValue *= 2.0;
			modValue -= 1.0;
			min -= a->intensity;
		}

		modValue *= a->intensity;
		mv.addValue += modValue;
	}

	if(min > max)
		std::swap(min, max);

	mv.modulationRange = { min, max };
	mv.lastModValue = lastValue;
	mv.clipTo0To1();
	return mv;
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::onSourceTargetChange(const ValueTree& v,
	const Identifier& id)
{
	auto isTarget = MatrixIds::Helpers::matchesTarget(v, targetId);

	if(isTarget)
		addConnection(v);
	else
		removeConnection(v);
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::addConnection(const ValueTree& v)
{
	auto idx = (int)v[MatrixIds::SourceIndex];

	if(isPositiveAndBelow(idx, sourceNames.size()))
	{
		auto n = new Target(*this, v);
		allTargets.add(n);

		if((int)v[MatrixIds::Mode] == 0)
			scaleTargets.add(n);
		else
			addTargets.add(n);
	}
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::removeConnection(const ValueTree& v)
{
	ReferenceCountedObjectPtr<Target> toBeRemoved;

	for(auto t: scaleTargets)
	{
		if(t->propertyListener.isRegisteredTo(v))
		{
			toBeRemoved = t;
			scaleTargets.removeObject(t);
			allTargets.removeObject(t);
			break;
		}
	}

	for(auto t: addTargets)
	{
		if(t->propertyListener.isRegisteredTo(v))
		{
			toBeRemoved = t;
			addTargets.removeObject(t);
			allTargets.removeObject(t);
			break;
		}
	}

	toBeRemoved = nullptr;
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::onUpdate(const ValueTree& v, bool wasAdded)
{
	if(MatrixIds::Helpers::matchesTarget(v, targetId))
	{
		if(!wasAdded)
			removeConnection(v);
		else
			addConnection(v);
	}
}

void ScriptingApi::Content::ScriptSlider::MatrixCableConnection::calculateNewModValue()
{
	auto normValue = sliderRange.convertTo0to1((double)parent->value, false);
	auto numThisTime = 0;

	if(gc != nullptr && getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::TargetThread::AudioThread)
		numThisTime = gc->getCurrentNumSamples() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(listLock))
	{
		for(auto s: scaleTargets)
		{
			jassert(s->tm == modulation::TargetMode::Gain);
			auto i = s->intensity;

			if(s->auxTarget != nullptr)
				i *= s->auxTarget->getAuxValue();

			auto a = 1.0f - i;
			auto v = a + i * s->lastModValue;

			if(s->isVoiceStart)
				s->voiceStartValue = v;
			else if (numThisTime > 0)
				s->rb->write(v, numThisTime);

			normValue *= v;
		}
			
		for(auto a: addTargets)
		{
			auto modValue = a->lastModValue;

			if(a->tm == modulation::TargetMode::Bipolar)
			{
				modValue *= 2.0;
				modValue -= 1.0;
			}

			auto i = a->intensity;

			if(a->auxTarget != nullptr)
				i *= a->auxTarget->getAuxValue();

			modValue *= i;

			if(a->isVoiceStart)
				a->voiceStartValue = modValue;
			if(numThisTime > 0)
				a->rb->write(modValue, numThisTime);

			normValue += modValue;
		}

		normValue = jlimit(0.0, 1.0, normValue);
		auto outputValue = sliderRange.convertFrom0to1(normValue, false);

		auto pr = parent.get()->getConnectedProcessor();

		Processor::ScopedAttributeNotificationSuspender sp(pr);

		parent->getScriptProcessor()->controlCallback(parent.get(), outputValue);
	}
}


namespace ScriptingObjects
{


struct ScriptingObjects::ScriptModulationMatrix::Wrapper
{
	API_METHOD_WRAPPER_3(ScriptModulationMatrix, connect);
	API_METHOD_WRAPPER_1(ScriptModulationMatrix, getTargetId);
	API_METHOD_WRAPPER_1(ScriptModulationMatrix, getComponent);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, toBase64);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, fromBase64);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setConnectionCallback);
	API_VOID_METHOD_WRAPPER_2(ScriptModulationMatrix, setEditCallback);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, getSourceList);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, getTargetList);
	API_METHOD_WRAPPER_2(ScriptModulationMatrix, canConnect);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, clearAllConnections);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setCurrentlySelectedSource);
	API_VOID_METHOD_WRAPPER_4(ScriptModulationMatrix, setConnectionProperty);
	API_METHOD_WRAPPER_3(ScriptModulationMatrix, getConnectionProperty);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setSourceSelectionCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setMatrixModulationProperties);
	API_METHOD_WRAPPER_0(ScriptModulationMatrix, getMatrixModulationProperties);
	API_VOID_METHOD_WRAPPER_1(ScriptModulationMatrix, setDragCallback);
};

ScriptModulationMatrix::ScriptModulationMatrix(ProcessorWithScriptingContent* p, const String& cid) :
	ConstScriptingObject(p, 0),
	ControlledObject(p->getMainController_()),
	connectionCallback(p, nullptr, var(), 3),
	editCallback(p, nullptr, var(), 1),
	um(getMainController()->getControlUndoManager()),
	sourceSelectionCallback(getScriptProcessor(), this, var(), 1),
	dragCallback(getScriptProcessor(), this, {}, 3) 
{
	container = dynamic_cast<GlobalModulatorContainer*>(ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), cid));

	if (container == nullptr)
		reportScriptError(cid + " is not a global modulation container");

	ADD_API_METHOD_3(connect);
	ADD_API_METHOD_1(fromBase64);
	ADD_API_METHOD_0(toBase64);
	ADD_API_METHOD_1(setConnectionCallback);
	ADD_API_METHOD_2(setEditCallback);
	ADD_API_METHOD_0(getSourceList);
	ADD_API_METHOD_0(getTargetList);
	ADD_API_METHOD_1(getTargetId);
	ADD_API_METHOD_1(getComponent);
	ADD_API_METHOD_2(canConnect);
	ADD_API_METHOD_1(clearAllConnections);

	ADD_API_METHOD_1(setCurrentlySelectedSource);
	ADD_API_METHOD_1(setSourceSelectionCallback);
	ADD_API_METHOD_1(setDragCallback);

	ADD_API_METHOD_1(setMatrixModulationProperties);
	ADD_API_METHOD_0(getMatrixModulationProperties);

	ADD_API_METHOD_3(getConnectionProperty);
	ADD_API_METHOD_4(setConnectionProperty);

	getScriptProcessor()->getMainController_()->getUserPresetHandler().addStateManager(this);

	connectionListener.setCallback(container->getMatrixModulatorData(), valuetree::AsyncMode::Synchronously, [this](const ValueTree& v, bool wasAdded)
	{
		if(connectionCallback)
		{
			var args[3];

			args[0] = sourceList[(int)v[MatrixIds::SourceIndex]];
			args[1] = v[MatrixIds::TargetId];
			args[2] = wasAdded;

			connectionCallback.call(args, 3);
		}
	});

	MatrixIds::Helpers::fillModSourceList(getMainController(), sourceList);
	MatrixIds::Helpers::fillModTargetList(getMainController(), allTargets, MatrixIds::Helpers::TargetType::All);
	MatrixIds::Helpers::fillModTargetList(getMainController(), parameterTargets, MatrixIds::Helpers::TargetType::Parameters);
	MatrixIds::Helpers::fillModTargetList(getMainController(), modulatorTargets, MatrixIds::Helpers::TargetType::Modulators);
}

ScriptModulationMatrix::~ScriptModulationMatrix()
{
	if(auto gc = container.get())
	{
		gc->editCallbackHandler.removeListener(*this);
	}

	getScriptProcessor()->getMainController_()->getUserPresetHandler().removeStateManager(this);
}

ValueTree ScriptModulationMatrix::exportAsValueTree() const
{
	return container->getMatrixModulatorData().createCopy();
}

void ScriptModulationMatrix::restoreFromValueTree(const ValueTree& v)
{
	jassert(v.getType() == MatrixIds::MatrixData);

	auto tree = container->getMatrixModulatorData();

	tree.removeAllChildren(um);

	for(auto c: v)
		tree.addChild(c.createCopy(), -1, um);
}

bool ScriptModulationMatrix::connect(String sourceId, String targetId, bool addConnection)
{
	auto idx = sourceList.indexOf(sourceId);

	if(idx != -1)
	{
		callSuspended([idx, targetId, addConnection](ScriptModulationMatrix& m)
		{
			auto md = m.container->getMatrixModulatorData();

			if(addConnection)
				return MatrixIds::Helpers::addConnection(md, m.getMainController(), targetId, idx).isValid();
			else
				return MatrixIds::Helpers::removeConnection(md, m.um, targetId, idx);
		});
	}

	return false;
}



String ScriptModulationMatrix::getTargetId(var componentOrId)
{
	ScriptComponent* sc = dynamic_cast<ScriptComponent*>(componentOrId.getObject());

	if(sc == nullptr)
		sc = getScriptProcessor()->getScriptingContent()->getComponentWithName(componentOrId.toString());

	if(sc != nullptr)
	{

		if(auto mod = dynamic_cast<MatrixModulator*>(sc->getConnectedProcessor()))
		{
			if(sc->getConnectedParameterIndex() == MatrixModulator::SpecialParameters::Value)
			{
				return mod->getId();
			}
		}

		return sc->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::matrixTargetId).toString();
	}

	return {};
}

var ScriptModulationMatrix::getComponent(String targetId)
{
	auto c = getScriptProcessor()->getScriptingContent();

	for(int i = 0; i < c->getNumComponents(); i++)
	{
		auto sc = c->getComponent(i);

		if(auto mod = dynamic_cast<MatrixModulator*>(sc->getConnectedProcessor()))
		{
			if(sc->getConnectedParameterIndex() == MatrixModulator::SpecialParameters::Value)
			{
				if(mod->getId() == targetId)
					return var(sc);
			}
		}

		if(auto s = dynamic_cast<ScriptingApi::Content::ScriptSlider*>(sc))
		{
			auto tid = s->getScriptObjectProperty(ScriptingApi::Content::ScriptSlider::matrixTargetId).toString();

			if(tid == targetId)
				return var(sc);
		}
	}

	return var();
}

bool ScriptModulationMatrix::canConnect(String source, String target)
{
	auto idx = sourceList.indexOf(source);

	if(idx != -1)
	{
		auto con = MatrixIds::Helpers::getConnection(container->getMatrixModulatorData(), idx, target);
		return !con.isValid();
	}

	return false;

}

String ScriptModulationMatrix::toBase64()
{
	zstd::ZDefaultCompressor comp;

	auto md = container->getMatrixModulatorData();
	MemoryBlock mb;
	comp.compress(md, mb);
	return mb.toBase64Encoding();
}

void ScriptModulationMatrix::fromBase64(String b64)
{
	MemoryBlock mb;

	if(mb.fromBase64Encoding(b64))
	{
		zstd::ZDefaultCompressor comp;
		ValueTree v;
		if(comp.expand(mb, v))
		{
			callSuspended([v](ScriptModulationMatrix& m)
			{
				m.restoreFromValueTree(v);
			});
		}
	}
}

void ScriptModulationMatrix::clearAllConnections(String targetId)
{
	callSuspended([targetId](ScriptModulationMatrix& m)
	{
		auto md = m.container->getMatrixModulatorData();
		MatrixIds::Helpers::removeAllConnections(md, m.um, targetId);
	});
}

void ScriptModulationMatrix::setConnectionCallback(var updateFunction)
{
	if(HiseJavascriptEngine::isJavascriptFunction(updateFunction))
	{
		connectionCallback = WeakCallbackHolder(getScriptProcessor(), this, updateFunction, 3);
		connectionCallback.incRefCount();
		connectionCallback.setThisObject(this);
	}
}

void ScriptModulationMatrix::setEditCallback(var menuItems, var editFunction)
{
	StringArray items;

	if(menuItems.isArray())
	{
		for(const auto& a: *menuItems.getArray())
			items.add(a.toString());
	}
	else if (menuItems.isString())
	{
		items.add(menuItems.toString());
	}

	items.removeEmptyStrings(true);
	items.removeDuplicates(false);

	if(HiseJavascriptEngine::isJavascriptFunction(editFunction) && !items.isEmpty())
	{
		editCallback = WeakCallbackHolder(getScriptProcessor(), this, editFunction, 1);
		editCallback.incRefCount();
		editCallback.setThisObject(this);

		container->customEditCallbacks = items;

		container->editCallbackHandler.addListener(*this, [](ScriptModulationMatrix& m, int idx, const String& targetId)
		{
			if(targetId.isNotEmpty() && m.editCallback)
			{
				var args[2];
				args[0] = var(idx);
				args[1] = targetId;

				m.editCallback.call(args, 2);
			}
		}, false);
	}
	else
	{
		container->customEditCallbacks = {};
		container->editCallbackHandler.removeListener(*this);
	}
}

var ScriptModulationMatrix::getSourceList() const
{
	Array<var> l;

	for(auto s: sourceList)
		l.add(s);

	return var(l);
}

var ScriptModulationMatrix::getTargetList() const
{
	Array<var> l;

	for(auto t: allTargets)
		l.add(t);

	return var(l);
}

void ScriptModulationMatrix::setCurrentlySelectedSource(String sourceId)
{
	auto idx = sourceList.indexOf(sourceId);

	if(idx != -1 && container != nullptr)
	{
		if(!container->matrixProperties.selectableSources)
			reportScriptError("Selectable sources are disabled");

		container->currentMatrixSourceBroadcaster.sendMessage(sendNotificationSync, idx);
	}
}

void ScriptModulationMatrix::setSourceSelectionCallback(var newCallback)
{
	container->currentMatrixSourceBroadcaster.removeListener(*this);

	if(HiseJavascriptEngine::isJavascriptFunction(newCallback))
	{
		if(!container->matrixProperties.selectableSources)
			reportScriptError("Selectable sources are disabled");

		sourceSelectionCallback = WeakCallbackHolder(getScriptProcessor(), this, newCallback, 1);
		sourceSelectionCallback.incRefCount();
		sourceSelectionCallback.setThisObject(this);

		container->currentMatrixSourceBroadcaster.addListener(*this, [](ScriptModulationMatrix& m, int idx)
		{
			if(isPositiveAndBelow(idx, m.sourceList.size()))
			{
				auto sourceName = m.sourceList[idx];
				m.sourceSelectionCallback.call1(sourceName);
			}
		});
	}
	
}

void ScriptModulationMatrix::setDragCallback(var newDragCallback)
{
	container->dragBroadcaster.removeListener(*this);

	if(HiseJavascriptEngine::isJavascriptFunction(newDragCallback))
	{
		dragCallback = WeakCallbackHolder(getScriptProcessor(), this, newDragCallback, 3);
		dragCallback.incRefCount();

		container->dragBroadcaster.addListener(*this, [](ScriptModulationMatrix& m, int si, const String& t, GlobalModulatorContainer::DragAction a)
		{
			if(m.dragCallback)
			{
				std::array<var, (int)GlobalModulatorContainer::DragAction::numDragActions> s({
					"DragEnd",
					"DragStart",
					"Drop",
					"Hover",
					"DisabledHover"
				});

				var args[3];
				args[0] = si == -1 ? String() : m.sourceList[si];
				args[1] = t;
				args[2] = s[(int)a];

				m.dragCallback.call(args, 3);
			}
		}, false);
	}
}

void ScriptModulationMatrix::setMatrixModulationProperties(var newProperties)
{
	container->matrixProperties.fromJSON(newProperties);

	for(const auto& iv: container->matrixProperties.initValues)
	{
		if(iv.second.intensity != 0.0 && !iv.second)
		{
			reportScriptError(iv.first + " init value has no Mode property defined");
		}
	}
}

var ScriptModulationMatrix::getConnectionProperty(String sourceId, String targetId, String propertyId)
{
	auto sourceIndex = sourceList.indexOf(sourceId);

	if(sourceIndex != -1)
	{
		auto c = MatrixIds::Helpers::getConnection(container->getMatrixModulatorData(), sourceIndex, targetId);

		if(c.isValid())
		{
			Identifier id(propertyId);

			if(MatrixIds::Helpers::getWatchableIds().contains(id))
			{
				return c[id];
			}
		}
	}

	return {};
}

bool ScriptModulationMatrix::setConnectionProperty(String sourceId, String targetId, String propertyId, var value)
{
	auto sourceIndex = sourceList.indexOf(sourceId);

	if(sourceIndex != -1)
	{
		auto c = MatrixIds::Helpers::getConnection(container->getMatrixModulatorData(), sourceIndex, targetId);

		if(c.isValid())
		{
			Identifier id(propertyId);

			if(MatrixIds::Helpers::getWatchableIds().contains(id))
			{
				c.setProperty(id, value, getScriptProcessor()->getMainController_()->getControlUndoManager());
				return true;
			}
		}
	}

	return false;
}

var ScriptModulationMatrix::getMatrixModulationProperties() const
{
	return container->getMatrixModulationProperties();
}

} // namespace ScriptingObjects
} // namespace hise
