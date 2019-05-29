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
						if(t->parameter != nullptr)
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

	String createCppClass(bool isOuterClass) override
	{
		auto s = NodeBase::createCppClass(isOuterClass);

		if (getModulationTargetTree().getNumChildren() > 0)
			return CppGen::Emitter::wrapIntoTemplate(s, "wrap::mod");
		else
			return s;
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


struct ModulationSourceBaseComponent : public Component,
	public PooledUIUpdater::SimpleTimer
{
	ModulationSourceBaseComponent(PooledUIUpdater* updater) :
		SimpleTimer(updater)
	{};

	ModulationSourceNode* getSourceNodeFromParent() const;

	void timerCallback() override {};

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);
		g.setColour(Colours::white.withAlpha(0.1f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Drag to modulation target", getLocalBounds().toFloat(), Justification::centred);
	}

	Image createDragImage()
	{
		Image img(Image::ARGB, 128, 48, true);
		Graphics g(img);

		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.4f));
		g.fillAll();
		g.setColour(Colours::black);
		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText(getSourceNodeFromParent()->getId(), 0, 0, 128, 48, Justification::centred);

		return img;
	}

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& event) override
	{
		if (getSourceNodeFromParent() == nullptr)
			return;

		if (auto container = DragAndDropContainer::findParentDragContainerFor(this))
		{
			// We need to be able to drag it anywhere...
			while (auto pc = DragAndDropContainer::findParentDragContainerFor(dynamic_cast<Component*>(container)))
				container = pc;

			var d;

			DynamicObject::Ptr details = new DynamicObject();

			details->setProperty(PropertyIds::ID, sourceNode->getId());
			details->setProperty(PropertyIds::ModulationTarget, true);

			container->startDragging(var(details), this, createDragImage());
		}
	}

protected:

	mutable WeakReference<ModulationSourceNode> sourceNode;
};

struct ScriptFunctionManager: public hise::GlobalScriptCompileListener
{
	~ScriptFunctionManager()
	{
		if (mc != nullptr)
			mc->removeScriptListener(this);
	}

	bool init(NodeBase* n, HiseDspBase* parent);

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	void updateFunction(Identifier, var newValue);

	double callWithDouble(double inputValue);

	Identifier getCallbackId() const
	{
		return callbackId;
	}

	valuetree::PropertyListener functionListener;
	WeakReference<JavascriptProcessor> jp;
	Result lastResult = Result::ok();

	Identifier callbackId;

	var functionName;
	var function;
	var input[8];
	bool ok = false;

	MainController* mc;

	NodeBase* pendingNode = nullptr;
	HiseDspBase* pendingParent = nullptr;
};

class MidiSourceNode : public HiseDspBase
{
public:

	enum class Mode
	{
		Gate = 0,
		Velocity,
		NoteNumber,
		Frequency
	};

	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_ID("midi");
	GET_SELF_AS_OBJECT(MidiSourceNode);
	SET_HISE_NODE_EXTRA_HEIGHT(40);
	SET_HISE_NODE_EXTRA_WIDTH(256);

	void initialise(NodeBase* n)
	{
		scriptFunction.init(n, this);
	}

	struct Display : public HiseDspBase::ExtraComponent<MidiSourceNode>
	{
		Display(MidiSourceNode* t, PooledUIUpdater* updater):
			ExtraComponent<MidiSourceNode>(t, updater),
			dragger(updater)
		{
			meter.setColour(VuMeter::backgroundColour, Colour(0xFF333333));
			meter.setColour(VuMeter::outlineColour, Colour(0x45ffffff));
			meter.setType(VuMeter::MonoHorizontal);
			meter.setColour(VuMeter::ledColour, Colours::grey);

			addAndMakeVisible(meter);
			addAndMakeVisible(dragger);

			setSize(256, 40);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			meter.setBounds(b.removeFromTop(24));
			dragger.setBounds(b);
		}

		void timerCallback() override
		{
			meter.setPeak(getObject()->modValue);
		}

		ModulationSourceBaseComponent dragger;
		VuMeter meter;
	};

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return new Display(this, updater);
	}

	void prepare(int numChannels, double sampleRate, int blockSize) {};

	void reset() {};

	void processSingle(float* frameData, int numChannels) {};

	void process(ProcessData& d)
	{
		if (d.eventBuffer != nullptr)
		{
			HiseEventBuffer::Iterator it(*d.eventBuffer);

			while (auto e = it.getNextEventPointer(true, false))
			{
				switch (currentMode)
				{
				case Mode::Gate:
				{
					if (e->isNoteOnOrOff())
					{
						modValue = e->isNoteOn() ? 1.0 : 0.0;
						changed = true;
					}

					break;
				}
				case Mode::Velocity:
				{
					if (e->isNoteOn())
					{
						modValue = e->getVelocity() / 127.0;
						changed = true;
					}

					break;
				}
				case Mode::NoteNumber:
				{
					if (e->isNoteOn())
					{
						modValue = (double)e->getNoteNumber() / 127.0;
						changed = true;
					}

					break;
				}
				case Mode::Frequency:
				{
					if (e->isNoteOn())
					{
						modValue = (e->getFrequency() - 20.0) / 19980.0;
						changed = true;
					}

					break;
				}
				}

				if(changed)
					modValue = scriptFunction.callWithDouble(modValue);
			}
		}
	}

	bool handleModulation(double& value)
	{
		if (changed)
		{
			value = modValue;
			changed = false;
			return true;
		}

		return false;
	}

	void createParameters(Array<ParameterData>& data) override
	{
		ParameterData d("Mode");

		d.setParameterValueNames({ "Gate", "Velocity", "NoteNumber", "Frequency"});
		d.defaultValue = 0.0;
		d.db = BIND_MEMBER_FUNCTION_1(MidiSourceNode::setMode);

		data.add(std::move(d));

		scriptFunction.init(nullptr, nullptr);
	}

	void setMode(double newMode)
	{
		currentMode = (Mode)(int)newMode;
	}

	Mode currentMode;

	ScriptFunctionManager scriptFunction;

	double modValue = 0.0;
	bool changed = false;
};

class TimerNode : public HiseDspBase
{
public:

	SET_HISE_NODE_IS_MODULATION_SOURCE(true);
	SET_HISE_NODE_ID("timer");
	GET_SELF_AS_OBJECT(TimerNode);
	SET_HISE_NODE_EXTRA_HEIGHT(40);
	SET_HISE_NODE_EXTRA_WIDTH(256);

	void initialise(NodeBase* n)
	{
		scriptFunction.init(n, this);
	}

	struct Display : public HiseDspBase::ExtraComponent<TimerNode>
	{
		Display(TimerNode* t, PooledUIUpdater* updater) :
			ExtraComponent<TimerNode>(t, updater),
			dragger(updater)
		{
			addAndMakeVisible(dragger);

			setSize(256, 40);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromTop(24);
			dragger.setBounds(b);
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().removeFromTop(24);

			g.setColour(Colours::white.withAlpha(alpha));
			g.fillEllipse(b.withSizeKeepingCentre(20, 20).toFloat());
		}

		void timerCallback() override
		{
			float lastAlpha = alpha;

			if (getObject()->ui_led)
			{
				alpha = 1.0f;
				getObject()->ui_led = false;
			}
			else
				alpha = jmax(0.0f, alpha - 0.1f);

			if (lastAlpha != alpha)
				repaint();
		}

		float alpha = 0.0f;
		ModulationSourceBaseComponent dragger;
	};

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return new Display(this, updater);
	}

	void prepare(int numChannels, double sampleRate, int blockSize) 
	{
		sr = sampleRate;
	};

	void reset()
	{
		samplesLeft = samplesBetweenCallbacks;
	};

	void processSingle(float* frameData, int numChannels)
	{
		if (!active)
			return;

		if (--samplesLeft <= 0)
		{
			modValue = scriptFunction.callWithDouble(0.0);
			changed = true;
			ui_led = true;
			samplesLeft = samplesBetweenCallbacks;
		}

		FloatVectorOperations::fill(frameData, modValue, numChannels);
	};

	void process(ProcessData& d)
	{
		if (!active)
		{
			return;
		}

		if (d.size < samplesLeft)
		{
			samplesLeft -= d.size;

			for (auto ch : d)
				FloatVectorOperations::fill(ch, (float)modValue, d.size);
		}

		else
		{
			const int numRemaining = d.size - samplesLeft;

			for (auto ch : d)
				FloatVectorOperations::fill(ch, (float)modValue, numRemaining);

			modValue = scriptFunction.callWithDouble(0.0);
			changed = true;
			ui_led = true;

			const int numAfter = d.size - numRemaining;

			for (auto ch : d)
				FloatVectorOperations::fill(ch + numRemaining, modValue, numAfter);

			samplesLeft = samplesBetweenCallbacks + numRemaining;
		}
	}

	bool handleModulation(double& value)
	{
		if (changed)
		{
			value = modValue;
			changed = false;
			return true;
		}

		return false;
	}

	void createParameters(Array<ParameterData>& data) override
	{
		{
			ParameterData d("Interval");

			d.range = { 0.0, 2000.0, 0.1 };
			d.defaultValue = 500.0;
			d.db = BIND_MEMBER_FUNCTION_1(TimerNode::setInterval);

			data.add(std::move(d));
		}
		
		{
			ParameterData d("Active");

			d.range = { 0.0, 1.0, 1.0 };
			d.defaultValue = 500.0;
			d.db = BIND_MEMBER_FUNCTION_1(TimerNode::setActive);

			data.add(std::move(d));
		}

		scriptFunction.init(nullptr, nullptr);
	}

	void setActive(double value)
	{
		bool thisActive = value > 0.5;

		if (active != thisActive)
		{
			active = thisActive;
			reset();
		}
	}

	void setInterval(double timeMs)
	{
		samplesBetweenCallbacks = roundDoubleToInt(timeMs * 0.001 * sr);
	}

	bool active = false;
	double sr = 44100.0;
	int samplesBetweenCallbacks = 22050;
	int samplesLeft = 22050;

	ScriptFunctionManager scriptFunction;

	double modValue = 0.0;
	bool changed = false;
	bool ui_led = false;
};

namespace core
{
using midi = MidiSourceNode;
using timer = TimerNode;
}

}
