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



struct PropertyHelpers
{
	static Colour getColour(ValueTree data)
	{
		while (data.getParent().isValid())
		{
			if (data.hasProperty(PropertyIds::NodeColour))
			{
				auto c = getColourFromVar(data[PropertyIds::NodeColour]);

				if (!c.isTransparent())
					return c;
			}

			data = data.getParent();
		}

		return Colour();
	}

	static Colour getColourFromVar(const var& value)
	{
		int64 colourValue = 0;

		if (value.isInt64() || value.isInt())
			colourValue = (int64)value;
		else if (value.isString())
		{
			auto string = value.toString();

			if (string.startsWith("0x"))
				colourValue = string.getHexValue64();
			else
				colourValue = string.getLargeIntValue();
		}

		return Colour((uint32)colourValue);
	};

	static PropertyComponent* createPropertyComponent(ProcessorWithScriptingContent* p, ValueTree& d, const Identifier& id, UndoManager* um);
};





#define DECLARE_SNEX_NODE(ClassType) SN_GET_SELF_AS_OBJECT() \
template <int P> static void setParameter(void* obj, double v) { static_cast<ClassType*>(obj)->setParameter<P>(v); } \
void initialise(scriptnode::NodeBase* n) {} \
snex::hmath Math;

namespace UIValues
{
static constexpr int HeaderHeight = 24;
static constexpr int ParameterHeight = 48 + 18 + 20;
static constexpr int NodeWidth = 128;
static constexpr int NodeHeight = 48;
static constexpr int NodeMargin = 10;
static constexpr int DuplicateSize = 128;
static constexpr int PinHeight = 24;
}

}
