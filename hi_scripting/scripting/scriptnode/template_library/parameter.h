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
#define _post_tuple_(x) std::index_sequence<Ns...>) { _pre_for_each_ std::get<Ns>(elements).x _post_for_each_ }
#define _post_tuple_op_(x) std::index_sequence<Ns...>) { _pre_for_each_ x(std::get<Ns>(elements)) _post_for_each_ }
         

#define tuple_iterator0(name)							_pre_tuple_(name) _post_tuple_(name())
#define tuple_iterator1(name, t1, a1)					_pre_tuple_(name) t1 a1, _post_tuple_(name(a1))
#define tuple_iterator2(name, t1, a1, t2, a2)			_pre_tuple_(name) t1 a1, t2 a2, _post_tuple_(name(a1, a2))
#define tuple_iterator3(name, t1, a1, t2, a2, t3, a3)	_pre_tuple_(name) t1 a1, t2 a2, t3 a3, _post_tuple_(name(a1, a2, a3))

#define tuple_iterator_op(name, opType) _pre_tuple_(name) opType& data, _post_tuple_op_(data)
#define tuple_iterator_opT(name, ta, opType) _pre_tupleT_(name, ta) opType & data, _post_tuple_op_(data)


#define call_tuple_iterator0(name)             name##_each(getIndexSequence());
#define call_tuple_iterator1(name, a1)         name##_each(a1, getIndexSequence());
#define call_tuple_iterator2(name, a1, a2)	   name##_each(a1, a2, getIndexSequence());
#define call_tuple_iterator3(name, a1, a2, a3) name##_each(a1, a2, a3, getIndexSequence());

#define tuple_iteratorT0(name, ta)							_pre_tupleT_(name, ta) _post_tuple_(name())
#define tuple_iteratorT1(name, ta, t1, a1)					_pre_tupleT_(name, ta) t1 a1, _post_tuple_(name(a1))
#define tuple_iteratorT2(name, ta, t1, a1, t2, a2)			_pre_tupleT_(name, ta) t1 a1, t2 a2, _post_tuple_(name(a1, a2))
#define tuple_iteratorT3(name, ta, t1, a1, t2, a2, t3, a3)	_pre_tupleT_(name, ta) t1 a1, t2 a2, t3 a3, _post_tuple_(name(a1, a2, a3))

namespace parameter
{


#define PARAMETER_SPECS(parameterType, parameterAmount) static constexpr int size = parameterAmount; static constexpr ParameterType type = parameterType; static constexpr bool isRange() { return false; };

enum class ParameterType
{
	Single,
	Chain,
	List
};

struct dynamic
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	using Function = void(*)(void*, double);

	static constexpr int MaxSize = 32;

	dynamic() :
		f(nullptr),
		obj(nullptr)
	{}

	template <typename T> dynamic& operator=(T&& other)
	{
		obj = other.getObjectPtr();
		f = typename T::callStatic;

		return *this;
	}

	void call(double v) const
	{
		if (f != nullptr && obj != nullptr)
			f(getObjectPtr(), v);
	}

	void referTo(void* p, Function f_)
	{
		f = f_;
		obj = p;
	}

	void operator()(double v) const
	{
		call(v);
	}

	void* getObjectPtr() const
	{
		return obj;
	}

	Function getFunction() { return f; }

private:

	void* obj = nullptr;
	Function f = nullptr;
};

}

struct ParameterDataImpl
{
	ParameterDataImpl(const String& id_);;
	ParameterDataImpl(const String& id_, NormalisableRange<double> r);
	ParameterDataImpl withRange(NormalisableRange<double> r);

	ValueTree createValueTree() const;

	void setDefaultValue(double newDefaultValue)
	{
		defaultValue = newDefaultValue;
	}

	operator bool() const
	{
		return id.isNotEmpty();
	}

	void setParameterValueNames(const StringArray& valueNames);
	void init();

	String id;
	NormalisableRange<double> range;
	double defaultValue = 0.0;

	std::function<void(double)> db;
	parameter::dynamic dbNew;

	StringArray parameterNames;

	double lastValue = 0.0;
	bool isUsingRange = true;
};


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
	PARAMETER_SPECS(ParameterType::Single, 1);

	template <int Index, class OtherType> void connect(OtherType& element)
	{
		static_assert(Index == 0, "Index must be zero");
		static_assert(std::is_same<OtherType, T>(), "target type mismatch");

		obj = reinterpret_cast<void*>(&element);
	}

	void* getObjectPtr() { return obj; }

protected:

	void* obj;
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

	static void callStatic(void*, double) {};

	void addToList(Array<ParameterDataImpl>& data)
	{
		
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

template <typename T> struct bypass : public single_base<T, 999>
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	void call(double v)
	{
		callStatic(obj, v);
	}

	void addToList(Array<ParameterDataImpl>& data)
	{

	}

	static void callStatic(void*  obj, double v)
	{
		T::setBypassed(obj, v);
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


template <typename T, int P> struct inner
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	inner(T& obj_) :
		obj(&obj_)
	{}

	void call(double v)
	{
		callStatic(obj, v);
	}

	static void callStatic(void* obj_, double v)
	{
		//auto target = &static_cast<inner*>(obj_)->obj;
		T::setParameter<P>(obj_, v);
	}

	void* getObjectPtr() { return obj; }

	void* obj;
};


struct dynamic_base
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	dynamic_base(parameter::dynamic& obj_) :
		obj(obj_.getObjectPtr()),
		f(obj_.getFunction())
	{};

	dynamic_base():
		obj(nullptr),
		f(nullptr)
	{}

	virtual void call(double value)
	{
		lastValue = value;
		f(obj, lastValue);
	}

	virtual void updateUI()
	{
		if (displayValuePointer != nullptr)
			*displayValuePointer = lastValue;

		if (dataTree.isValid())
			dataTree.setProperty(PropertyIds::Value, lastValue, nullptr);
	};

	void setDataTree(ValueTree d) { dataTree = d; }

	parameter::dynamic::Function f;
	void* obj;

	ValueTree dataTree;
	double lastValue = 0.0;
	double* displayValuePointer = nullptr;
};


struct dynamic_base_holder
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	void call(double v)
	{
		lastValue = v;

		if (base != nullptr)
			base->call(v);

		if (buffer != nullptr)
			buffer->write(lastValue, numSamples);
	}

	void setRingBuffer(SimpleRingBuffer* r)
	{
		buffer = r;
	}

	void setSamplesToWrite(int numSamplesThisTime)
	{
		numSamples = numSamplesThisTime;
	}

	void updateUI()
	{
		if (base != nullptr)
			base->updateUI();
	}

	void setParameter(dynamic_base* b)
	{
		base = b;
	}

	ScopedPointer<dynamic_base> base;
	
	SimpleRingBuffer* buffer = nullptr;
	int numSamples = 1;
	double lastValue = 0.0f;
};



struct dynamic_from0to1 : public dynamic_base
{
	dynamic_from0to1(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{}

	void call(double v) final override
	{
		lastValue = range.convertFrom0to1(v);
		f(obj, lastValue);
	}

	const NormalisableRange<double> range;
	
};

struct dynamic_to0to1 : public dynamic_base
{
	dynamic_to0to1(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{}

	void call(double v) final override
	{
		lastValue = range.convertTo0to1(v);
		f(obj, lastValue);
	}

	const NormalisableRange<double> range;
};

struct dynamic_expression : public dynamic_base
{
	dynamic_expression(parameter::dynamic& obj, snex::JitExpression* p_) :
		dynamic_base(obj),
		p(p_)
	{
		if (!p->isValid())
			p = nullptr;
	}

	void call(double v) final override
	{
		if (p != nullptr)
		{
			lastValue = p->getValueUnchecked(v);
			f(obj, lastValue);
		}
		else
		{
			lastValue = v;
			f(obj, lastValue);
		}
	}

	snex::JitExpression::Ptr p;
};

struct dynamic_chain: public dynamic_base
{
	dynamic_chain() :
		dynamic_base()
	{};

	bool isEmpty() const { return targets.isEmpty(); }

	void addParameter(dynamic_base* p)
	{
		targets.add(p);
	}

	dynamic_base* getFirstIfSingle()
	{
		if (targets.size() == 1)
			return targets.removeAndReturn(0);
		
		return nullptr;
	}

	void call(double v)
	{
		for (auto& t : targets)
			t->call(v);
	}

	void updateUI() override
	{
		for (auto& t : targets)
			t->updateUI();
	}

	OwnedArray<dynamic_base> targets;
};



/** The most simple parameter type without any range conversion. */
template <class T, int P> struct plain : public single_base<T, P>
{
	void call(double v)
	{
		callStatic(obj, v);
	}

	static void callStatic(void* o, double v)
	{
		T::setParameter<P>(o, v);
	}

	void addToList(Array<ParameterDataImpl>& data)
	{
		ParameterDataImpl p("plainUnNamed");
		p.dbNew.referTo(this, callStatic);
		p.range = NormalisableRange<double>(0.0, 1.0);
		data.add(p);
	}

	template <int P> auto& getParameter()
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
		jassert(obj != nullptr);
		jassert(connected);

		f(obj, Expression()(v));
	}

	void operator()(double v)
	{
		call(v);
	}

	void addToList(Array<ParameterDataImpl>& data)
	{
		ParameterDataImpl p("exprUnNamed");
		p.dbNew.referTo(this, callStatic);
		p.range = NormalisableRange<double>();
		data.add(p);
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(connected);
		jassert(obj != nullptr);
	}
};

/** A parameter that converts the input from 0...1 to the given range.

	The RangeType argument must be a class created by one of the

	DECLARE_PARAMETER_RANGE_XXX

	macros that define a helper class with the required function signature.

	This class is usually used in connections from a macro parameter in order to
	convert the range to the respective limits for each connection. */
template <class T, int P, class RangeType> struct from0to1 : public single_base<T, P>
{
	void call(double v)
	{
		callStatic(obj, v);
	}

	static void callStatic(void* obj_, double v)
	{
		auto converted = RangeType::from0To1(v);
		T::setParameter<P>(obj_, converted);
	}

	void operator()(double v)
	{
		call(v);
	}

	void addToList(Array<ParameterDataImpl>& data)
	{
		ParameterDataImpl p("plainUnNamed");
		p.dbNew.referTo(this, callStatic);

		// use the default range here...
		p.range = NormalisableRange<double>(0.0, 1.0);
		data.add(p);
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(connected);
		jassert(obj != nullptr);
	}
};

/** A parameter that converts the input to 0...1 from the given range.

	The RangeType argument must be a class created by one of the

	DECLARE_PARAMETER_RANGE_XXX

	macros that define a helper class with the required function signature.

	This class is usually used in macro parameters to convert the "public" knob
	range into the normalised range that is sent to each connection.
*/
template <class T, int P, class RangeType> struct to0to1 : public single_base<T, P>
{
	void call(double v)
	{
		jassert(obj != nullptr);
		jassert(connected);

		f(obj, RangeType::to0to1(v));
	}

	void addToList(Array<ParameterDataImpl>& data)
	{
		ParameterDataImpl p("plainUnNamed");
		p.dbNew.referTo(this, callStatic);
		p.range = RangeType::createNormalisableRange();
		data.add(p);
	}

	template <int P> auto& getParameter()
	{
		return *this;
	}

	void validate()
	{
		jassert(connected);
		jassert(obj != nullptr);
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
	PARAMETER_SPECS(ParameterType::Chain, 1);

	template <int P> auto& getParameter()
	{
		return *this;
	}

	static void callStatic(void* obj, double value)
	{
		static_cast<chain*>(obj)->call(value);
	}

	void* getObjectPtr() { return this; }

	tuple_iterator1(call, double, v);

	void call(double v)
	{
		v = InputRange::to0To1(v);

		call_tuple_iterator1(call, v);
	}

	void addToList(Array<ParameterDataImpl>& data)
	{
		ParameterDataImpl p("plainUnNamed");
		p.dbNew.referTo(this, callStatic);
		p.range = InputRange::createNormalisableRange();
		data.add(p);
	}

	template <int Index, class Target> void connect(Target& t)
	{
		get<Index>().connect<0>(t);
	}
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
	PARAMETER_SPECS(ParameterType::List, sizeof...(Parameters));

	template <int P> auto& getParameter()
	{
		return get<P>();
	}

	tuple_iterator1(addToList, Array<ParameterDataImpl>&, d);

	void addToList(Array<ParameterDataImpl>& data)
	{
		call_tuple_iterator1(addToList, data);
	}

	template <int Index, class Target> void connect(Target& t)
	{
		static_assert(false, "Can't connect to list");
	}
};


}


}
