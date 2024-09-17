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
 * - cache stuff (after profiling!)
 * - add JSON converter
 * - add layout stuff (replace layoutdata?): width, height, display (none or inherit/init/default)
 * - write LAF for all stock elements
 */

/** A high level, internal property categorization that helps the parser figure out some stuff. */
enum class PropertyType
{
	Positioning, 
	Layout, 
	Transform,
	Border,
	BorderRadius,
	Colour,
	Transition,
	Shadow,
	Font,
	Variable,
	Undefined
};

/** A high level, internal value categorization that helps the parser figure out some stuff. */
enum class ValueType
{
	Undefined,
	Colour,
	Gradient,
	Size,
	Number,
	Time,
	Variable,
	numValueTypes
};

/** the type of selector used for the next CSS block. */
enum class SelectorType
{
	None,
	Type,
	Class,
	ID,
	Element,
	AtRule,
	All,
	ParentDelimiter,
	numSelectorTypes
};

/** The supported element types which correlates to some native JUCE UI components. */
enum class ElementType
{
	Body,
	Button,
	TextInput,
	Paragraph,
	Selector,
	Panel,
	Ruler,
	Image,
	Table,
	TableHeader,
	TableRow,
	TableCell,
	Label,
	Headline1,
	Headline2,
	Headline3,
	Headline4,
	Progress,
	Scrollbar
};

/** The positioning mode values. Currently there is only a distinction between absolute & fixed and the others. */
enum class PositionType
{
	initial,
	relative,
	absolute,
	fixed,
	numPositionTypes
};

/** A list of supported pseudo class states that can be used to customize the appearance based on the component state. */
enum class PseudoClassType
{
	None = 0,
	First = 1,
	Last = 2,
	Root = 4,
	Hover = 8,
	Active = 16,
	Focus = 32,
	Disabled = 64,
	Hidden = 128,
	Checked = 256,
	All = 511
};

/** A list of pseudo elements. Currently there is only support for before & after (because most text-based pseudo elements like
 *  first-letter do not really apply to a JUCE based layout system). */
enum class PseudoElementType
{
	None = 0,
	Before,
	After,
	All
};

/** The number of supported expression functions when using CSS expressions. */
enum class ExpressionType
{
	none,
	literal,
	calc,
	min,
	max,
	clamp,
	numExpressionTypes
};


/** The supported transform types. Note that this is a complete list of all CSS translations, but some
 *  are unsupported */
enum class TransformTypes
{
	none, // none
	matrix, // matrix(n,n,n,n,n,n) - unsupported
	translate, // translate(x,y)
	translateX, // translateX(x)
	translateY, // translateY(y)
	translateZ, // translateZ(z) - unsupported
	scale, // scale(x,y)
	scaleX, // scaleX(x)
	scaleY, // scaleY(y)
	scaleZ, // scaleZ(z) - unsupported
	rotate, // rotate(angle)
	rotateX, // rotateX(angle)
	rotateY, // rotateY(angle)
	rotateZ, // rotateZ(angle) - unsupported
	skew, // skew(x-angle,y-angle)
	skewX, // skewX(angle)
	skewY, // skewY(angle)
	numTransformTypes
};




	
	
} // namespace simple_css
} // namespace hise
