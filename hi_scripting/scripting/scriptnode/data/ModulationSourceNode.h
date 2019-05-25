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

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;


class ModulationSourceNode: public NodeBase
{
public:

	static constexpr int ModulationBarHeight = 60;

	static constexpr int RingBufferSize = 65536;

	ValueTree getModulationTargetTree()
	{
		auto vt = getValueTree().getChildWithName(PropertyIds::ModulationTargets);

		if (!vt.isValid())
		{
			vt = ValueTree(PropertyIds::ModulationTargets);
			getValueTree().addChild(vt, -1, getUndoManager());
		}

		return vt;
	};

	struct ModulationTarget: public ConstScriptingObject
	{
		static void nothing(double unused)
		{

		}

		ModulationTarget(ModulationSourceNode* parent_, ValueTree data_):
			ConstScriptingObject(parent_->getScriptProcessor(), 0),
			parent(parent_),
			data(data_)
		{
			rangeUpdater.setCallback(data, RangeHelpers::getRangeIds(), 
				valuetree::AsyncMode::Coallescated,
				[this](Identifier, var)
			{
				inverted = RangeHelpers::checkInversion(data, &rangeUpdater, parent->getUndoManager());
				targetRange = RangeHelpers::getDoubleRange(data);
			});

			callback = nothing;
		}

		Identifier getObjectName() const override { return PropertyIds::ModulationTarget; }

		bool findTarget()
		{
			String nodeId = data[PropertyIds::NodeId];
			String parameterId = data[PropertyIds::ParameterId];

			auto list = parent->getRootNetwork()->getListOfNodesWithType<NodeBase>(false);

			for (auto n : list)
			{
				if (n->getId() == nodeId)
				{
					for (int i = 0; i < n->getNumParameters(); i++)
					{
						if (n->getParameter(i)->getId() == parameterId)
						{
							parameter = n->getParameter(i);
							callback = parameter->getCallback();

							removeWatcher.setCallback(parameter->data, 
								valuetree::AsyncMode::Synchronously,
							[this](ValueTree& c)
							{
								data.getParent().removeChild(data.getParent(), parent->getUndoManager());
							});

							return true;
						}
					}
				}
			}

			return false;
		}

		valuetree::PropertyListener rangeUpdater;
		valuetree::RemoveListener removeWatcher;
		ReferenceCountedObjectPtr<NodeBase::Parameter> parameter;
		WeakReference<ModulationSourceNode> parent;
		ValueTree data;
		NormalisableRange<double> targetRange;
		bool inverted = false;
		DspHelpers::ParameterCallback callback;
	};

	ModulationSourceNode(DspNetwork* n, ValueTree d) :
		NodeBase(n, d, 0)
	{
		targetListener.setCallback(getModulationTargetTree(), 
			valuetree::AsyncMode::Synchronously,
			[this](ValueTree& c, bool wasAdded)
		{
			if (wasAdded)
				targets.add(new ModulationTarget(this, c));
			else
			{
				for (auto t : targets)
				{
					if (t->data == c)
					{
						t->parameter->data.removeProperty(PropertyIds::ModulationTarget, getUndoManager());
						targets.removeObject(t);
						break;
					}
				}
			}

			ok = true;

			for (auto t : targets)
				ok &= t->findTarget();
		});
	};

	void addModulationTarget(NodeBase::Parameter* n)
	{
		ValueTree m(PropertyIds::ModulationTarget);

		m.setProperty(PropertyIds::NodeId, n->parent->getId(), nullptr);
		m.setProperty(PropertyIds::ParameterId, n->getId(), nullptr);
		
		n->data.setProperty(PropertyIds::ModulationTarget, true, getUndoManager());

		auto range = RangeHelpers::getDoubleRange(n->data);

		RangeHelpers::storeDoubleRange(m, false, range, nullptr);

		getModulationTargetTree().addChild(m, -1, getUndoManager());
	}
	
	void prepare(double sampleRate, int blockSize)
	{
		ringBuffer = new SimpleRingBuffer();
		
		if(sampleRate > 0.0)
			sampleRateFactor = 32768.0 / sampleRate;

		ok = true;

		for (auto t : targets)
			ok &= t->findTarget();
	}

	void sendValueToTargets(double value, int numSamplesForAnalysis)
	{
		if (ringBuffer != nullptr)
		{
			ringBuffer->write(value, (int)(jmax(1.0, sampleRateFactor * (double)numSamplesForAnalysis)));
		}

		if (!ok)
			return;

		if (scaleModulationValue)
		{
			value = jlimit(0.0, 1.0, value);

			for (auto t : targets)
			{
				auto thisValue = value;

				if (t->inverted)
					thisValue = 1.0 - thisValue;

				auto normalised = t->targetRange.convertFrom0to1(thisValue);

				t->parameter->setValueAndStoreAsync(normalised);
			}
		}
		else
		{
			for (auto t : targets)
				t->parameter->setValueAndStoreAsync(value);
		}
	};

	int fillAnalysisBuffer(AudioSampleBuffer& b)
	{
		if (ringBuffer != nullptr)
			return ringBuffer->read(b);
	}

	void setScaleModulationValue(bool shouldScaleValue)
	{
		scaleModulationValue = shouldScaleValue;
	}

private:

	double sampleRateFactor = 1.0;

	bool scaleModulationValue = true;

	int ringBufferSize = 0;
	int blockSize = 0;

	struct SimpleRingBuffer
	{
		SimpleRingBuffer()
		{
			clear();
		}

		void clear()
		{
			memset(buffer, 0, sizeof(float) * RingBufferSize);
		}

		int read(AudioSampleBuffer& b)
		{
			jassert(b.getNumSamples() == RingBufferSize && b.getNumChannels() == 1);

			auto start = Time::getMillisecondCounter();

			while(isBeingWritten) // busy wait, but OK
			{
				auto end = Time::getMillisecondCounter();
				if (end - start > 10)
					break;
			}

			auto dst = b.getWritePointer(0);
			int numBeforeIndex = writeIndex;
			int offsetBeforeIndex = RingBufferSize - numBeforeIndex;
			
			FloatVectorOperations::copy(dst + offsetBeforeIndex, buffer, numBeforeIndex);
			FloatVectorOperations::copy(dst, buffer + writeIndex, offsetBeforeIndex);

			int numToReturn = numAvailable;
			numAvailable = 0;
			return numToReturn;
		}

		void write(double value, int numSamples)
		{
			ScopedValueSetter<bool> svs(isBeingWritten, true);

			if (numSamples == 1)
			{
				buffer[writeIndex++] = (float)value;
				
				if (writeIndex >= RingBufferSize)
					writeIndex = 0;

				numAvailable++;
				return;
			}
			else
			{
				int numBeforeWrap = jmin(numSamples, RingBufferSize - writeIndex);
				int numAfterWrap = numSamples - numBeforeWrap;

				if (numBeforeWrap > 0)
					FloatVectorOperations::fill(buffer + writeIndex, (float)value, numBeforeWrap);

				writeIndex += numBeforeWrap;

				if (numAfterWrap > 0)
				{
					FloatVectorOperations::fill(buffer, (float)value, numAfterWrap);
					writeIndex = numAfterWrap;
				}
					
				numAvailable += numSamples;
			}
		}

		bool isBeingWritten = false;

		int numAvailable = 0;
		int writeIndex = 0;
		float buffer[RingBufferSize];
	};

	ScopedPointer<SimpleRingBuffer> ringBuffer;

	valuetree::ChildListener targetListener;

	
	bool ok = false;

	ReferenceCountedArray<ModulationTarget> targets;
	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulationSourceNode);
};



}
