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

namespace feedback
{
	class SourceNode : public NodeBase
	{
	public:

		SourceNode(DspNetwork* rootNetwork, ValueTree data) :
			NodeBase(rootNetwork, data, 0)
		{}

		SCRIPTNODE_FACTORY(SourceNode, "output");
		
		Identifier getObjectName() const override { return getStaticId(); }

		void prepare(double sampleRate, int blockSize) override
		{
			if (buffer.getNumChannels() != getNumChannelsToProcess() ||
				buffer.getNumSamples() < blockSize)
			{
				buffer.setSize(getNumChannelsToProcess(), blockSize);
			}

			reset();
		}

		void reset() final override
		{
			memset(singleFrameData, 0, sizeof(float) * NUM_MAX_CHANNELS);
		}

		void process(ProcessData& data) final override
		{
			if (connectedOK)
			{
				for (int i = 0; i < data.numChannels; i++)
					FloatVectorOperations::copy(data.data[i], buffer.getReadPointer(i), data.size);
			}
		}

		void processSingle(float* frameData, int numChannels) final override
		{
			FloatVectorOperations::copy(frameData, singleFrameData, numChannels);
		}

		float singleFrameData[NUM_MAX_CHANNELS];

		AudioSampleBuffer buffer;
		bool connectedOK = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SourceNode);
	};

	class TargetNode : public NodeBase
	{
	public:


		TargetNode(DspNetwork* parent, ValueTree data) :
			NodeBase(parent, data, 0)
		{
			connectionListener.setCallback(data, { PropertyIds::Connection },
				valuetree::AsyncMode::Synchronously,
				BIND_MEMBER_FUNCTION_2(TargetNode::updateConnection));
		};

		SCRIPTNODE_FACTORY(TargetNode, "input");

		Identifier getObjectName() const override { return getStaticId(); }

		void reset() final override {}

		void updateConnection(Identifier, var newValue)
		{
			if (auto n = dynamic_cast<NodeBase*>(getRootNetwork()->get(newValue).getObject()))
			{
				if (connectedSource != nullptr)
					connectedSource->connectedOK = false;

				if (connectedSource = dynamic_cast<SourceNode*>(n))
					connectedSource->connectedOK = true;
			}
		}

		~TargetNode()
		{
			if (connectedSource != nullptr)
				connectedSource->connectedOK = false;
		}

		void prepare(double sampleRate, int blockSize) override
		{
			
		}

		NodeComponent* createComponent();

		Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
		{
			return Rectangle<int>(0, 0, 128, 32 + UIValues::HeaderHeight).withPosition(topLeft);
		}

		void process(ProcessData& data) override
		{
			if (connectedSource != nullptr)
			{
				int numChannelsToSend = jmin(connectedSource->buffer.getNumChannels(), data.numChannels);
				int numSamplesToSend = jmin(connectedSource->buffer.getNumSamples(), data.size);

				for (int i = 0; i < numChannelsToSend; i++)
					connectedSource->buffer.copyFrom(i, 0, data.data[i], numSamplesToSend);
			}
		}

		void processSingle(float* frameData, int numChannels) final override
		{
			if (connectedSource != nullptr)
			{
				FloatVectorOperations::copy(connectedSource->singleFrameData, frameData, numChannels);
			}
		}

		valuetree::PropertyListener connectionListener;

		WeakReference<SourceNode> connectedSource;

		JUCE_DECLARE_WEAK_REFERENCEABLE(TargetNode);
	};

	using input = SourceNode;
	using output = TargetNode;

	class FeedbackFactory : public NodeFactory
	{
	public:
		FeedbackFactory(DspNetwork* n) :
			NodeFactory(n)
		{
			registerNode<input>();
			registerNode<output>();
		}

		Identifier getId() const override { return "feedback"; }
	};
};





class DspNode : public NodeBase,
	public AssignableObject
{
public:

	DspNode(DspNetwork* root, DspFactory* f_, ValueTree data);

	Identifier getObjectName() const override { return "DspNode"; }

	virtual void assign(const int index, var newValue) override
	{
		if (auto p = getParameter(index))
		{
			auto floatValue = (float)newValue;
			FloatSanitizers::sanitizeFloatNumber(floatValue);

			p->setValueAndStoreAsync(floatValue);
		}
		else
			reportScriptError("Cant' find parameter for index " + String(index));
	}

	/** Return the value for the specified index. The parameter passed in must relate to the index created with getCachedIndex. */
	var getAssignedValue(int index) const override
	{
		if (auto p = getParameter(index))
		{
			return p->getValue();
		}
		else
			reportScriptError("Cant' find parameter for index " + String(index));
	}

	void reset() final override {};

	virtual int getCachedIndex(const var &indexExpression) const override
	{
		return (int)indexExpression;
	}

	void prepare(double sampleRate, int blockSize) override
	{
		if (obj != nullptr)
			obj->prepareToPlay(sampleRate, blockSize);
	}

	void process(ProcessData& data) final override
	{
		if (obj != nullptr)
			obj->processBlock(data.data, data.numChannels, data.size);
	}

	~DspNode()
	{
		f->destroyDspBaseObject(obj);
	}

	Rectangle<int> getPositionInCanvas(Point<int> topLeft) const override
	{
		if (obj != nullptr)
		{
			int numParameters = obj->getNumParameters();

			int numRows = std::ceil((float)numParameters / 4.0f);

			auto b = Rectangle<int>(0, 0, jmin(400, numParameters * 100), numRows * (48+18) + UIValues::HeaderHeight);

			return b.expanded(UIValues::NodeMargin).withPosition(topLeft);
		}
	}

	NodeComponent* createComponent() override;

private:

	friend class DefaultParameterNodeComponent;

	String moduleName;

	void initialise();

	DspFactory::Ptr f;
	DspBaseObject* obj = nullptr;

};



}
