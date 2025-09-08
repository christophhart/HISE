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

#pragma once

namespace hise {
namespace dyncomp {
namespace dcid {
using namespace hise;
using namespace juce;

#define DECLARE_ID(x) static const Identifier x(#x);

DECLARE_ID(ContentProperties);
DECLARE_ID(Root);
DECLARE_ID(Component);

DECLARE_ID(id);
DECLARE_ID(type);
DECLARE_ID(text);
DECLARE_ID(enabled);
DECLARE_ID(visible);
DECLARE_ID(tooltip);
DECLARE_ID(defaultValue);
DECLARE_ID(useUndoManager);

static const Identifier class_("class");
DECLARE_ID(elementStyle);

DECLARE_ID(parentComponent);
DECLARE_ID(bounds);
DECLARE_ID(x);
DECLARE_ID(y);
DECLARE_ID(width);
DECLARE_ID(height);

DECLARE_ID(isMomentary);
DECLARE_ID(radioGroupId);
DECLARE_ID(setValueOnClick);
DECLARE_ID(popupMenuAlign);

DECLARE_ID(useCustomPopup);
DECLARE_ID(items);

DECLARE_ID(editable);
DECLARE_ID(multiline);
DECLARE_ID(updateEachKey);

DECLARE_ID(min);
DECLARE_ID(max);
DECLARE_ID(middlePosition);
DECLARE_ID(stepSize);
DECLARE_ID(mode);
DECLARE_ID(suffix);
DECLARE_ID(style);
DECLARE_ID(showValuePopup);

DECLARE_ID(processorId);
DECLARE_ID(parameterId);

DECLARE_ID(filmstripImage);
DECLARE_ID(numStrips);
DECLARE_ID(isVertical);
DECLARE_ID(scaleFactor);

DECLARE_ID(animationSpeed);
DECLARE_ID(dragMargin);
DECLARE_ID(polyFX);

DECLARE_ID(FloatingTileData);

DECLARE_ID(bgColour);
DECLARE_ID(itemColour);
DECLARE_ID(itemColour2);
DECLARE_ID(textColour);

DECLARE_ID(index);

#undef DECLARE_ID

struct Helpers
{
	static const Array<Identifier>& getProperties();
	static const Array<Identifier>& getBasicProperties();
	static const Array<Identifier>& getCSSProperties();

	static var get(const ValueTree& v, const Identifier& p)
	{
		return v.getProperty(p, getDefaultValue(p));
	}
	static var getDefaultValue(const Identifier& p);
	static bool isValidProperty(const Identifier& p);
	static var convertColour(const Identifier& id, const var& v);
	static var getDragContainerIndexValues(const std::vector<int>& indexList, const ValueTree& parentData);
	static int getMaxPositionOfChildComponents(const ValueTree& parentData);
	static ComplexDataUIBase* getComplexDataBase(MainController* mc, const ValueTree& v, ExternalData::DataType dt_);
	static std::vector<int> getChildComponentOrderFromPosition(const ValueTree& parentData);
};
	
} // namespace dcid
} // namespace dyncomp
} // namespace hise
