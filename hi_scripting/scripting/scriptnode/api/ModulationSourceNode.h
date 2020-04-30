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



class WrapperNode : public NodeBase
{
protected:

	WrapperNode(DspNetwork* parent, ValueTree d);;

	NodeComponent* createComponent() override;

	virtual Component* createExtraComponent() = 0;

	Rectangle<int> getExtraComponentBounds() const;

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override;
	Rectangle<int> createRectangleForParameterSliders(int numColumns) const;

	void initParameterData(Array<ParameterDataImpl>& pData);

private:

	mutable int cachedExtraWidth = -1;
	mutable int cachedExtraHeight = -1;
};
    
class ModulationSourceNode: public WrapperNode,
							public SnexDebugHandler,
							public PooledUIUpdater::SimpleTimer
{
public:

	static constexpr int ModulationBarHeight = 60;
	

	struct ModulationTarget: public ConnectionBase
	{
		static void nothing(double) {}

		ModulationTarget(ModulationSourceNode* parent_, ValueTree data_);

		~ModulationTarget();

		bool findTarget();

#if OM
		void setExpression(const String& exprCode)
		{
#if HISE_INCLUDE_SNEX
			snex::JitExpression::Ptr newExpr;

			if (exprCode.isEmpty())
			{
				SpinLock::ScopedLockType lock(expressionLock);
				std::swap(expr, newExpr);
			}
			else
			{
				newExpr = new snex::JitExpression(exprCode, parent);

				if (newExpr->isValid())
				{
					SpinLock::ScopedLockType lock(expressionLock);
					std::swap(newExpr, expr);
				}
			}
#endif
		}

		void applyValue(double value)
		{
			if (!active)
				return;

			switch (opType)
			{
			case SetValue: targetParameter->setValueAndStoreAsync(value); break;
			case Multiply: targetParameter->multiplyModulationValue(value); break;
			case Add:	   targetParameter->addModulationValue(value); break;
			default:   break;
			}
		}
#endif

		bool isModulationConnection() const override { return true; }

		valuetree::PropertyListener expressionUpdater;
		valuetree::PropertyListener rangeUpdater;
		valuetree::RemoveListener removeWatcher;
		WeakReference<ModulationSourceNode> parent;

#if OM
		DspHelpers::ParameterCallback callback;
#endif

		CachedValue<bool> active;
		
		

#if OM
		SnexExpressionPtr expr;
		SpinLock expressionLock;
#endif
	};

	ValueTree getModulationTargetTree();;

	ModulationSourceNode(DspNetwork* n, ValueTree d);;

	var addModulationTarget(NodeBase::Parameter* n);

	String createCppClass(bool isOuterClass) override;
	
	NodeBase* getTargetNode(const ValueTree& m) const;

	ParameterDataImpl getParameterData(const ValueTree& m) const;

	virtual bool isUsingNormalisedRange() const = 0;
	virtual parameter::dynamic_base_holder* getParameterHolder() { return nullptr; }

	parameter::dynamic_base* createDynamicParameterData(ValueTree& m);

	void rebuildModulationConnections();

	void prepare(PrepareSpecs ps) override;

#if OM
	void sendValueToTargets(double value, int numSamplesForAnalysis);
	{
		if (ringBuffer != nullptr &&
			numSamplesForAnalysis > 0 &&
			getRootNetwork()->isRenderingFirstVoice())
		{
			ringBuffer->write(value, (int)(jmax(1.0, sampleRateFactor * (double)numSamplesForAnalysis)));
		}

		if (scaleModulationValue)
		{
			value = jlimit(0.0, 1.0, value);

			for (auto t : targets)
			{
#if HISE_INCLUDE_SNEX
				SpinLock::ScopedLockType l(t->expressionLock);

				if (t->expr != nullptr)
				{
					auto thisValue = t->expr->getValue(value);
					t->applyValue(thisValue);
					continue;
				}
#endif

				auto thisValue = value;

				if (t->inverted)
					thisValue = 1.0 - thisValue;

				auto normalised = t->connectionRange.convertFrom0to1(thisValue);

				t->applyValue(normalised);
			}
		}
		else
		{
			for (auto t : targets)
			{
#if HISE_INCLUDE_SNEX
				SpinLock::ScopedLockType l(t->expressionLock);

				if (t->expr != nullptr)
				{
					auto thisValue = t->expr->getValue(value);
					t->applyValue(thisValue);
					continue;
				}
#endif

				t->applyValue(value);
			}
		}

		lastModValue = value;
	}
#endif

	void logMessage(int level, const String& s) override;

	int fillAnalysisBuffer(AudioSampleBuffer& b);

#if OM
	void setScaleModulationValue(bool shouldScaleValue)
	{
		scaleModulationValue = shouldScaleValue;
	}

	bool shouldScaleModulationValue() const noexcept { return scaleModulationValue; }

	virtual bool isUsingModulation() const { return false; }
#endif

	void checkTargets();

	double sampleRateFactor = 1.0;
	bool prepareWasCalled = false;

	double lastModValue = 0.0;

	bool scaleModulationValue = true;

	int ringBufferSize = 0;

	ScopedPointer<SimpleRingBuffer> ringBuffer;
	valuetree::ChildListener targetListener;

	bool ok = false;

	ReferenceCountedArray<ModulationTarget> targets;
	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulationSourceNode);
};


struct ModulationSourceBaseComponent : public Component,
	public PooledUIUpdater::SimpleTimer
{
	ModulationSourceBaseComponent(PooledUIUpdater* updater);;
	ModulationSourceNode* getSourceNodeFromParent() const;

	using ObjectType = HiseDspBase;

	static Component* createExtraComponent(ObjectType* obj, PooledUIUpdater* updater)
	{
		return new ModulationSourceBaseComponent(updater);
	}

	void timerCallback() override {};
	void paint(Graphics& g) override;
	Image createDragImage();

	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent&) override;

protected:

	mutable WeakReference<ModulationSourceNode> sourceNode;
};


struct ModulationSourcePlotter : ModulationSourceBaseComponent
{
	using ObjectType = HiseDspBase;

	static Component* createExtraComponent(ObjectType* obj, PooledUIUpdater* updater)
	{
		return new ModulationSourcePlotter(updater);
	}

	ModulationSourcePlotter(PooledUIUpdater* updater);

	void timerCallback() override;
	void rebuildPath();
	void paint(Graphics& g) override;

	int getSamplesPerPixel() const;

	float pixelCounter = 0.0f;
	bool skip = false;

	RectangleList<float> rectangles;
	AudioSampleBuffer buffer;
};

}
