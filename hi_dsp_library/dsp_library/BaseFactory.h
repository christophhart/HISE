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
 *   information about commercial licencing:
 *
 *   http://www.hartinstruments.net/hise/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#ifndef BASEFACTORY_H_INCLUDED
#define BASEFACTORY_H_INCLUDED

namespace hise {using namespace juce;


#pragma warning (push)
#pragma warning (disable: 4127)

/** A generic template factory class.
*
*	Create a instance with the base class as template and add subclasses via registerType<SubClass>().
*	The subclasses must have a static method
*
*		static Identifier getName() { RETURN_STATIC_IDENTIFIER(name) }
*
*	so that the factory can decide which subtype to create.
*/
template <typename BaseClass>
class Factory
{
public:

	/** Register a subclass to this factory. The subclass must have a static method 'Identifier getName()'. */
	template <typename DerivedClass> void registerType()
	{
		if (std::is_base_of<BaseClass, DerivedClass>::value)
		{
			ids.add(DerivedClass::getName());
			functions.add(&createFunc<DerivedClass>);
		}
	}

	/** Creates a subclass instance with the registered Identifier and returns a base class pointer to this. You need to take care of the ownership of course. */
	BaseClass* createFromId(const Identifier &id) const
	{
		const int index = ids.indexOf(id);

		if (index != -1) return functions[index]();
		else			 return nullptr;
	}

    /** Returns the list of all registered items. */
	const Array<Identifier> &getIdList() const { return ids; }

private:

	/** @internal */
	template <typename DerivedClass> static BaseClass* createFunc() { return new DerivedClass(); }

	/** @internal */
	typedef BaseClass* (*PCreateFunc)();

	Array<Identifier> ids;
	Array <PCreateFunc> functions;;
};


#pragma warning (pop)

} // namespace hise

#endif  // BASEFACTORY_H_INCLUDED
