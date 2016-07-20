/*
  ==============================================================================

    BaseFactory.h
    Created: 20 Jul 2016 7:14:04pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef BASEFACTORY_H_INCLUDED
#define BASEFACTORY_H_INCLUDED


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



#endif  // BASEFACTORY_H_INCLUDED
