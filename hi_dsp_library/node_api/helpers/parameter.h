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

/** This is a wrapper around std::tuple<T...> with a few additional functions:

	- a get<Index>() method
	- a getIndexSequenceMethod()

	This class becomes particularly useful in conjunction with the macros defined
	below that allow a fairly clean syntax for compile-time tuple iteration:

	If you want to iterate over the tuple and call a certain function:

	1. add a function with any amount and types of arguments (the macros must match
	   this amount and the return type must be void).
	2. Use one of the tuple_iteratorX macros to define an internal method to the class
	   that wants to iterate the tuple
	3. In the function body, use the call_tuple_iterator with the function name and
	   arguments to call the method for each element in the tuple.
	
	This is an example (taken from the parameter::chain class template):

		tuple_iterator1(call, double, v)

		void call(double v)
		{
			// Do something with v as you please...
			v = v * 12.0;

			call_tuple_iterator1(call, v);
		}

	If you want to iterate over a templated method, use the tuple_iteratorTX
	macros.
*/
template <class... Elements> struct advanced_tuple
{
	static constexpr auto getIndexSequence()
	{
		return std::index_sequence_for<Elements...>();
	}

	template <int Index> constexpr auto& get() noexcept
	{
		return std::get<Index>(elements);
	}

	template <int Index> constexpr const auto& get() const noexcept
	{
		return std::get<Index>(elements);
	}

protected:

	std::tuple<Elements...> elements;
};

/**@internal. */
#define _pre_for_each_ using swallow = int[]; (void)swallow { 1, (
/**@internal. */
#define _post_for_each_ , void(), int{})... };
/**@internal. */
#define _pre_tuple_(name) template <std::size_t ...Ns> void name##_each(
/**@internal. */
#define _pre_tupleT_(name, ta) template <typename ta, std::size_t ...Ns> void name##_each(
/**@internal. */
#define _post_tuple_(x) std::index_sequence<Ns...>) { _pre_for_each_ std::get<Ns>(this->elements).x _post_for_each_ }
#define _post_tuple_op_(x) std::index_sequence<Ns...>) { _pre_for_each_ x(std::get<Ns>(this->elements)) _post_for_each_ }
         

#define tuple_iterator0(name)							_pre_tuple_(name) _post_tuple_(name())
#define tuple_iterator1(name, t1, a1)					_pre_tuple_(name) t1 a1, _post_tuple_(name(a1))
#define tuple_iterator2(name, t1, a1, t2, a2)			_pre_tuple_(name) t1 a1, t2 a2, _post_tuple_(name(a1, a2))
#define tuple_iterator3(name, t1, a1, t2, a2, t3, a3)	_pre_tuple_(name) t1 a1, t2 a2, t3 a3, _post_tuple_(name(a1, a2, a3))

#define tuple_iterator_op(name, opType) _pre_tuple_(name) opType& data, _post_tuple_op_(data)
#define tuple_iterator_opT(name, ta, opType) _pre_tupleT_(name, ta) opType & data, _post_tuple_op_(data)


#define call_tuple_iterator0(name)             this->name##_each(Type::getIndexSequence());
#define call_tuple_iterator1(name, a1)         this->name##_each(a1, Type::getIndexSequence());
#define call_tuple_iterator2(name, a1, a2)	   this->name##_each(a1, a2, Type::getIndexSequence());
#define call_tuple_iterator3(name, a1, a2, a3) this->name##_each(a1, a2, a3, Type::getIndexSequence());

#define tuple_iteratorT0(name, ta)							_pre_tupleT_(name, ta) _post_tuple_(name())
#define tuple_iteratorT1(name, ta, t1, a1)					_pre_tupleT_(name, ta) t1 a1, _post_tuple_(name(a1))
#define tuple_iteratorT2(name, ta, t1, a1, t2, a2)			_pre_tupleT_(name, ta) t1 a1, t2 a2, _post_tuple_(name(a1, a2))
#define tuple_iteratorT3(name, ta, t1, a1, t2, a2, t3, a3)	_pre_tupleT_(name, ta) t1 a1, t2 a2, t3 a3, _post_tuple_(name(a1, a2, a3))



/** This namespace contains different parameter templates that can be used to create compile time callbacks with the same
	behaviour as the connections in scriptnode.
*/
namespace parameter
{


/** The base class for all parameters that represent a single connection between two nodes.

	This class has two template arguments:

	- T: the object that this parameter operates on
	- I: the parameter index

	Because these things are compile-time constants, the compiler should be able to resolve and
	inline the function call to the parameter callback.
*/
template <class T, int P> struct single_base
{
	using TargetType = T;

	PARAMETER_SPECS(ParameterType::Single, 1);

	template <int Index, class OtherType> void connect(OtherType& element)
	{
        static_assert(P >= 0, "parameter index must be positive");
        
		static_assert(Index == 0, "Index must be zero");
        static_assert(std::is_same<typename OtherType::ObjectType, typename T::ObjectType>(), "target type mismatch");

		obj = reinterpret_cast<void*>(&element.getObject());
	}

	void* getObjectPtr() { return obj; }

	bool isConnected() const noexcept
	{
		return obj != nullptr;
	}

	void setObjPtr(void* o)
	{
		obj = o;
	}

protected:

	void* obj = nullptr;
};

/** A dummy class that can be used when the container does not have any macro parameters.

	Since every container template requires a parameter class, using this class will not generate
	any code if the container does not need to have macro parameters. */
struct empty
{
	PARAMETER_SPECS(ParameterType::Single, 0);

	void call(double v)
	{
		
	}

	void* getObjectPtr() { return reinterpret_cast<void*>(this); }

	static void callStatic(void*, double) {};

	bool isConnected() const { return true; }

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this, callStatic);
		d.add(p);
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	template <int P> NormalisableRange<double> createParameterRange()
	{
		static_assert(P == 0, "not zero");
		return NormalisableRange<double>();
	}
};

template <typename T, typename RangeType=typename ranges::Identity> struct bypass : public single_base<T, 9000>
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	void call(double v)
	{
		callStatic(this->obj, v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this->obj, callStatic);
		d.add(p);
	}

	static void callStatic(void*  obj, double v)
	{
		if constexpr (!std::is_same<RangeType, ranges::Identity>())
		{
			auto s = RangeType::getSimpleRange();
			auto shouldBeOn = v >= s[0] && v <= s[1];
			v = (double)!shouldBeOn;
		}
		else
			v = v < 0.5;

		T::template setParameter<9000>(obj, v);
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	template <int P> NormalisableRange<double> createParameterRange()
	{
		return NormalisableRange<double>(0.0, 1.0);
	}
};






/** The most simple parameter type without any range conversion. */
template <class T, int P> struct plain : public single_base<T, P>
{
    void call(double v)
	{
		jassert(this->isConnected());
		callStatic(this->obj, v);
	}

	static void callStatic(void* o, double v)
	{
        using ObjectType = typename T::ObjectType;
		jassert(o != nullptr);

		ObjectType::template setParameterStatic<P>(o, v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this->obj, callStatic);
		d.add(p);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}
};


/** Inverts the normalised value. */
template <class T, int P> struct inverted: public single_base<T, P>
{
	void call(double v)
	{
		jassert(this->isConnected());
		callStatic(this->obj, v);
	}

	static void callStatic(void* o, double v)
	{
		using ObjectType = typename T::ObjectType;
		jassert(o != nullptr);

		ObjectType::template setParameterStatic<P>(o, 1.0 - v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this->obj, callStatic);
		d.add(p);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}
};



/** A parameter that takes a expression class for converting the value that is passed to the parameter callback.

	the Expression argument should be a class derived by SnexExpressionBase
	(and best created using the DECLARE_PARAMETER_EXPRESSION macro).
*/
template <class T, int P, class Expression> struct expression : public single_base<T, P>
{
    void call(double v)
	{
		jassert(this->isConnected());

        using ObjectType = typename T::ObjectType;

		Expression e;
		v = e.op(v);

		ObjectType:: template setParameterStatic<P>(this->obj, v);
	}

	void operator()(double v)
	{
		call(v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("exprUnNamed");
		p.callback.referTo(this, single_base<T, P>::callStatic);
        p.setRange({});
		d.add(p);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(this->connected);
		jassert(this->obj != nullptr);
	}
};

/** A parameter that converts the input from 0...1 to the given range.

	The RangeType argument must be a class created by one of the

	DECLARE_PARAMETER_RANGE_XXX

	macros that define a helper class with the required function signature.

	This class is usually used in connections from a macro parameter in order to
	convert the range to the respective limits for each connection. */
template <class T, int P, class RangeType> struct from0To1 : public single_base<T, P>
{
	void call(double v)
	{
		jassert(this->isConnected());
		callStatic(this->obj, v);
	}

	static void callStatic(void* obj_, double v)
	{
        auto converted = RangeType::from0To1(v);

        using ObjectType = typename T::ObjectType;

		ObjectType::template setParameterStatic<P>(obj_, converted);
	}

	void operator()(double v)
	{
		call(v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this->obj, callStatic);

		// use the default range here...
		p.setRange({ 0.0, 1.0 });
		d.add(p);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(this->connected);
		jassert(this->obj != nullptr);
	}
};

template <class T, int P, class RangeType> struct from0To1_inv : public single_base<T, P>
{
	void call(double v)
	{
		jassert(this->isConnected());
		callStatic(this->obj, v);
	}

	static void callStatic(void* obj_, double v)
	{
		auto converted = RangeType::from0To1(v);
		using ObjectType = typename T::ObjectType;
		ObjectType::template setParameterStatic<P>(obj_, converted);
	}

	void operator()(double v)
	{
		call(v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this->obj, callStatic);

		// use the default range here...
		p.setRange({ 0.0, 1.0 });
		d.add(p);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(this->connected);
		jassert(this->obj != nullptr);
	}
};

/** A parameter that converts the input to 0...1 from the given range.

	The RangeType argument must be a class created by one of the

	DECLARE_PARAMETER_RANGE_XXX

	macros that define a helper class with the required function signature.

	This class is usually used in macro parameters to convert the "public" knob
	range into the normalised range that is sent to each connection.
*/
template <class T, int P, class RangeType> struct to0To1 : public single_base<T, P>
{
	void call(double v)
	{
		jassert(this->obj != nullptr);
		jassert(this->connected);

		f(this->obj, RangeType::to0To1(v));
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
        p.callback.referTo(this, single_base<T, P>::callStatic);
		p.setRange(RangeType::createNormalisableRange());
		d.add(p);
	}

	template <int Unused> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(this->connected);
		jassert(this->obj != nullptr);
	}
};





/** A parameter chain is a list of parameter callbacks that are processed serially.

	The InputRange template argument must be a range class and will be used to convert
	the input to 0...1 before it is processed with each parameter in the list.

	The Others... variadic template argument is a list of all parameters and can be used
	to control multiple parameters at once.
*/
template <class InputRange, class... Others> struct chain: public advanced_tuple<Others...>
{
    using Type = advanced_tuple<Others...>;
    
	PARAMETER_SPECS(ParameterType::Chain, 1);

	chain()
	{
		for(auto& c: connectBits)
			c = false;
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	static void callStatic(void* obj, double value)
	{
		static_cast<chain*>(obj)->call(value);
	}

	bool isConnected() const noexcept
	{
		return connected;
	}

	void* getObjectPtr() { return this; }

	tuple_iterator1(call, double, v);

	void call(double v)
	{
		v = InputRange::to0To1(v);

		call_tuple_iterator1(call, v);
	}

	void addToList(ParameterDataList& d)
	{
		data p("plainUnNamed");
		p.callback.referTo(this, callStatic);
		p.setRange(InputRange::createNormalisableRange());
		d.add(p);
	}

	template <int Index, class Target> void connect(Target& t)
	{
		connectBits[Index] = true;
		this->template get<Index>().template connect<0>(t);

		connected = true;

		for(const auto& x: connectBits)
			connected &= x;
	}

	std::array<bool, sizeof...(Others)> connectBits;
	bool connected = false;
};


/** This class is just a placeholder for a node that expects multiple outputs without connections.
*/
struct empty_list
{
    PARAMETER_SPECS(ParameterType::List, 0);
    
    template <int P> auto& getParameter()
    {
        return *this;
    }
    
    static constexpr int getNumParameters() { return 0; }

    template <int Index, class Target> void connect(Target& t)
    {
        jassertfalse;
    }
    
    template <int P> void call(double) {};

    static constexpr bool isStaticList() { return true; }
};

/** The parameter list is a collection of multiple parameters that can be called individually.

	It can be used to add multiple macro parameters to a chain and supports nested calls.

	The Parameters... argument just contains a list of parameters that will be called by their
	index - so the first parameter can be called using list::call<0>(value).

	If this class is supplied to a container node template, it will forward the calls to

	T::setParameter<I>() to the call<I>() method, so you can connect it like a hardcoded node.
*/
template <class... Parameters> struct list: public advanced_tuple<Parameters...>
{
    using Type = advanced_tuple<Parameters...>;
    
	PARAMETER_SPECS(ParameterType::List, sizeof...(Parameters));

	template <int P> auto& getParameter()
	{
		return this->template get<P>();
	}

	tuple_iterator1(addToList, ParameterDataList&, d);

	void addToList(ParameterDataList& d)
	{
		call_tuple_iterator1(addToList, d);
	}

	template <int Index, class Target> void connect(Target& t)
	{
        jassertfalse;
	}

	// the dynamic list needs to be initialised with the value tree
	SN_EMPTY_INITIALISE;

	template <int P> void call(double v)
	{
		if constexpr (P <= getNumParameters())
		{
			getParameter<P>().call(v);
		}
	}

    static constexpr int getNumParameters() { return sizeof...(Parameters); }



	static constexpr bool isStaticList() { return true; }
};


}



}
