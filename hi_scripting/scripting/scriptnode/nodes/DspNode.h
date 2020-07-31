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



class DspNode : public NodeBase,
	public AssignableObject
{
public:

	DspNode(DspNetwork* root, DspFactory* f_, ValueTree data);

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
        
        RETURN_IF_NO_THROW({});
	}

	void reset() final override {};

	virtual int getCachedIndex(const var &indexExpression) const override
	{
		return (int)indexExpression;
	}

	void prepare(PrepareSpecs ps) override
	{
		if (obj != nullptr)
			obj->prepareToPlay(ps.sampleRate, ps.blockSize);
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

			int numRows = (int)std::ceil((float)numParameters / 4.0f);

			auto b = Rectangle<int>(0, 0, jmin(400, numParameters * 100), numRows * (48+18) + UIValues::HeaderHeight);

			return b.expanded(UIValues::NodeMargin).withPosition(topLeft);
		}
        
        return {};
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
