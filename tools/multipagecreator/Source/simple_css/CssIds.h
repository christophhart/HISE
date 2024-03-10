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
namespace simple_css
{
using namespace juce;

/** TODO:
 *
 * - add font stuff
 * - add !important
 * - add box-shadow
 * - add selectors (input, button, select,  :hover, :active, :focus)
 * 
 * - cache stuff (after profiling!)
 * - add JSON converter
 * - add layout stuff (replace layoutdata?): width, height, display (none or inherit/init/default)
 * - write LAF for all stock elements
 * - add code editor language manager
 */

namespace PropertyIds
{
#define DECLARE_ID(x) static const Identifier x(#x);
	DECLARE_ID(margin);
	DECLARE_ID(padding);
#undef DECLARE_ID
}

enum class PropertyType
{
	Positioning, // margin or padding
	Layout, // height or width
	Transform,
	Border,
	BorderRadius,
	Colour,
	Transition,
	Shadow,
	Font,
	Undefined
};

enum class SelectorType
{
	None,
	Type,
	Class,
	ID,
	All
};

enum class ElementType
{
	Body,
	Button,
	TextInput,
	Selector,
	Panel
};

enum class PseudoClassType
{
	None = 0,
	Hover = 1,
	Active = 2,
	Focus = 4
};

enum class TransformTypes
{
	none, // none
	matrix, // matrix(n,n,n,n,n,n)
	translate, // translate(x,y)
	translateX, // translateX(x)
	translateY, // translateY(y)
	translateZ, // translateZ(z)
	scale, // scale(x,y)
	scaleX, // scaleX(x)
	scaleY, // scaleY(y)
	scaleZ, // scaleZ(z)
	rotate, // rotate(angle)
	rotateX, // rotateX(angle)
	rotateY, // rotateY(angle)
	rotateZ, // rotateZ(angle)
	skew, // skew(x-angle,y-angle)
	skewX, // skewX(angle)
	skewY, // skewY(angle)
	numTransformTypes
};

enum class ValueType
{
	Undefined,
	Colour,
	Gradient,
	Size,
	numValueTypes
};

	
} // namespace simple_css
} // namespace hise