/*
  ==============================================================================

    JitFunctions.h
    Created: 4 Apr 2017 9:11:11pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JITFUNCTIONS_H_INCLUDED
#define JITFUNCTIONS_H_INCLUDED


#include <type_traits>

class BaseFunction
{
public:

	BaseFunction(const Identifier& id, void* func_) : functionName(id), func(func_) {}

	virtual ~BaseFunction() {};

	virtual int getNumParameters() const = 0;

	virtual TypeInfo getReturnType() const = 0;

	TypeInfo getTypeForParameter(int index)
	{
		if (index >= 0 && index < getNumParameters())
		{
			return parameterTypes[index];
		}
		else
		{
			return typeid(void);
		}
	}

	const Identifier functionName;
	void* func;

protected:

#pragma warning (push)
#pragma warning (disable: 4100)

	template <typename P, typename... Ps> void addTypeInfo(std::vector<TypeInfo>& info, P value, Ps... otherInfo)
	{
		info.push_back(typeid(value));
		addTypeInfo(info, otherInfo...);
	}

	template<typename P> void addTypeInfo(std::vector<TypeInfo>& info, P value) { info.push_back(typeid(value)); }

#pragma warning (pop)

	void addTypeInfo(std::vector<TypeInfo>& /*info*/) {}

	std::vector<TypeInfo> parameterTypes;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseFunction)
};


template <typename R, typename ... Parameters> class TypedFunction : public BaseFunction
{
public:

	TypedFunction(const Identifier& id, void* func, Parameters... types) :
		BaseFunction(id, func)
	{
		addTypeInfo(parameterTypes, types...);
	};

	int getNumParameters() const override { return sizeof...(Parameters); };

	virtual TypeInfo getReturnType() const override { return typeid(R); };


};


#endif  // JITFUNCTIONS_H_INCLUDED
