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



namespace hise {
namespace dyncomp {
namespace dcid {

const Array<Identifier>& Helpers::getProperties()
{
	static const Array<Identifier> all({
		id,
		index,
		type,
		text,
		enabled,
		visible,
		tooltip,
		class_,
		defaultValue,
		useUndoManager,
		elementStyle,
		parentComponent,
		x,
		y,
		width,
		height,
		isMomentary,
		radioGroupId,
		setValueOnClick,
		useCustomPopup,
		items,
		editable,
		multiline,
		updateEachKey,
		popupMenuAlign,
		min,
		max,
		middlePosition,
		stepSize,
		mode,
		suffix,
		style,
		showValuePopup,
		processorId,
		parameterId,
		filmstripImage,
		numStrips,
		isVertical,
		scaleFactor,
		animationSpeed,
		dragMargin,
		bgColour,
		itemColour,
		itemColour2,
		textColour
	});

	return all;
}

const Array<Identifier>& Helpers::getBasicProperties()
{
	static const Array<Identifier> basicProperties({ 
		dcid::enabled,
		dcid::visible,
		dcid::class_,
		dcid::elementStyle,
		dcid::bgColour,
		dcid::itemColour,
		dcid::itemColour2,
		dcid::textColour
	});

	return basicProperties;
}

const Array<Identifier>& Helpers::getCSSProperties()
{
	static const Array<Identifier> cssProperties({
		dcid::id,
		dcid::type,
		dcid::text,
		dcid::bgColour,
		dcid::itemColour,
		dcid::itemColour2,
		dcid::textColour,
		dcid::x,
		dcid::y,
		dcid::width,
		dcid::height
	});
				
	return cssProperties;
}

var Helpers::getDefaultValue(const Identifier& p)
{
	jassert(isValidProperty(p));

	static NamedValueSet defaultValues;

	defaultValues.set(id, var(""));
	defaultValues.set(type, var(""));
	defaultValues.set(text, var(""));
	defaultValues.set(enabled, var(true));
	defaultValues.set(visible, var(true));
	defaultValues.set(tooltip, var(""));
	defaultValues.set(class_, var(""));
	defaultValues.set(defaultValue, var(0.0));
	defaultValues.set(useUndoManager, var(false));
	defaultValues.set(elementStyle, var(""));
	defaultValues.set(parentComponent, var(""));
	defaultValues.set(x, var(0));
	defaultValues.set(y, var(0));
	defaultValues.set(width, var(128));
	defaultValues.set(height, var(50));
	defaultValues.set(isMomentary, var(false));
	defaultValues.set(radioGroupId, var(0));
	defaultValues.set(setValueOnClick, var(false));
	defaultValues.set(useCustomPopup, var(false));
	defaultValues.set(items, var(""));
	defaultValues.set(editable, var(true));
	defaultValues.set(multiline, var(false));
	defaultValues.set(updateEachKey, var(false));
	defaultValues.set(min, var(0.0));
	defaultValues.set(max, var(1.0));
	defaultValues.set(middlePosition, var(-10));
	defaultValues.set(stepSize, var(0.01));
	defaultValues.set(mode, var(""));
	defaultValues.set(suffix, var(""));
	defaultValues.set(style, var("Knob"));
	defaultValues.set(showValuePopup, var(false));
	defaultValues.set(processorId, var(""));
	defaultValues.set(parameterId, var(""));
	defaultValues.set(filmstripImage, var(""));
	defaultValues.set(numStrips, var(64));
	defaultValues.set(isVertical, var(true));
	defaultValues.set(scaleFactor, var(1.0));
	defaultValues.set(animationSpeed, var());
	defaultValues.set(popupMenuAlign, var(false));
	defaultValues.set(dragMargin, var());
	defaultValues.set(index, var(0));
	defaultValues.set(bgColour, Colours::black.withAlpha(0.5f).getARGB());
	defaultValues.set(itemColour, Colours::white.withAlpha(0.2f).getARGB());
	defaultValues.set(itemColour2, Colours::white.withAlpha(0.2f).getARGB());
	defaultValues.set(textColour, Colours::white.withAlpha(0.8f).getARGB());

	return defaultValues[p];
}

bool Helpers::isValidProperty(const Identifier& p)
{
	return getProperties().contains(p);
}

var Helpers::convertColour(const Identifier& pid, const var& v)
{
	if(pid == bgColour || pid == itemColour || pid == itemColour2 || pid == textColour)
	{
		return ApiHelpers::convertStyleSheetProperty(v, "color");
	}

	return v;
}

var Helpers::getDragContainerIndexValues(const std::vector<int>& indexList, const ValueTree& parentData)
	{
		auto offset = (int)parentData[min];

		Array<var> values;

		for(auto& i: indexList)
		{
			auto idx = (int)parentData.getChild(i)[index];
			values.add(var(offset + idx));
		}

		return var(values);
	}

int Helpers::getMaxPositionOfChildComponents(const ValueTree& parentData)
{
	auto vertical = parentData.getProperty(isVertical, true);
	auto posId = vertical ? y : x;
	auto sizeId = vertical ? height : width;

	auto maxPos = 0;

	for(auto c: parentData)
	{
		maxPos = jmax(maxPos, (int)c[posId] + (int)c[sizeId]);
	}

	return maxPos;
}

ComplexDataUIBase* Helpers::getComplexDataBase(MainController* mc, const ValueTree& v, ExternalData::DataType dt_)
{
	auto pid = v[dcid::processorId].toString();
	auto pindex = (int)v[dcid::index];

	auto chain = mc->getMainSynthChain();

	if(auto p = ProcessorHelpers::getFirstProcessorWithName(chain, pid))
	{
		if(auto typed = dynamic_cast<ExternalDataHolder*>(p))
		{
			auto numObjects = typed->getNumDataObjects(dt_);

			if(isPositiveAndBelow(pindex, numObjects))
			{
				return typed->getComplexBaseType(dt_, pindex);
			}
		}
	}

	return nullptr;
}

std::vector<int> Helpers::getChildComponentOrderFromPosition(const ValueTree& parentData)
{
	Array<ValueTree> children;

	for(const auto& c: parentData)
		children.add(c);

	auto vertical = parentData.getProperty(isVertical, true);

	struct Sorter
	{
		Sorter(bool vertical_):
			vertical(vertical_)
		{}

		int compareElements(const ValueTree& v1, const ValueTree& v2) const
		{
			auto id1 = v1[id].toString();
			auto id2 = v2[id].toString();

			int p1 = (int)v1[vertical ? y : x];
			int p2 = (int)v2[vertical ? y : x];

			if(p1 < p2)
				return -1;
			if(p1 > p2)
				return 1;

			// this shouldn't happen
			jassertfalse;
			return 0;
		}

		const bool vertical;
	};

	Sorter s(vertical);
	children.sort(s);

	std::vector<int> indexes;

	for(const auto& v: children)
		indexes.push_back(parentData.indexOf(v));

	return indexes;

}
} // namespace dcid
} // namespace dyncomp
} // namespace hise
