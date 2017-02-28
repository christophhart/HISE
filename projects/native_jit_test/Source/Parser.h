/*
  ==============================================================================

    Parser.h
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED


#include "TokenIterator.h"

#include <map>
#include <tuple>
#include <type_traits>

typedef std::type_index TypeInfo;


struct NativeJITTypeHelpers
{
	/** Returns a pretty name for the given Type. */
	template <typename T> static String getTypeName();

	/** Returns a pretty name for the given String literal. */
	static String getTypeName(const String &t);

	/** Creates a error message if the types don't match. */
	template <typename ActualType, typename ExpectedType> static String getTypeMismatchErrorMessage();

	/** Returns the type ID for the given String literal. Currently supported: double, float & int. */
	static TypeInfo getTypeForLiteral(const String &t);;

	/** Checks if the given type ID matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const TypeInfo& actualType);

	/** Checks if the given string matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const String& t);;

	/** Compares two types. */
	template <typename R1, typename R2> static bool is();;
};

class BaseFunction
{
public:

	BaseFunction(const Identifier& id, void* func_): functionName(id), func(func_) {}

	virtual ~BaseFunction() {};

	virtual int getNumParameters() const = 0;

	virtual TypeInfo getReturnType() const = 0;

	const Identifier functionName;
	void* func;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseFunction)
};

struct GlobalBase
{
	GlobalBase(const Identifier& id_, TypeInfo type_) :
		id(id_),
		type(type_)
	{
		data = malloc(sizeof(double));
	};

	~GlobalBase()
	{
		free(data);
	}

	template<typename T> static T store(GlobalBase* b, T newValue)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		*reinterpret_cast<T*>(b->data) = newValue;

		return newValue;
	}

	template<typename T> static T get(GlobalBase* b)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		return *reinterpret_cast<T*>(b->data);
	}

	template <typename T> static GlobalBase* create(const Identifier& id)
	{
		return new GlobalBase(id, typeid(T));
	}

	TypeInfo type;
	Identifier id;
	void* data;
};



template <typename T> T storeInStack(T& target, T value)
{
	target = value;
	return value;
}

double getSampleRate()
{
	return 44100.0;
}

template <typename R, typename ... Parameters> class TypedFunction : public BaseFunction
{
public:

	TypedFunction(const Identifier& id, void* func) :
		BaseFunction(id, func)
	{};

	int getNumParameters() const override { return sizeof...(Parameters); };

	virtual TypeInfo getReturnType() const { return typeid(R); };

};

template <typename R, typename ... Parameters> class FunctionParser: private ParserHelpers::TokenIterator
{

	struct NamedNode
	{
		NamedNode(const Identifier& id_, NativeJIT::NodeBase* node_, bool isConst_, TypeInfo type_) : 
			id(id_), 
			node(node_), 
			isConst(isConst_), 
			type(type_)
		{};

		const Identifier id;
		NativeJIT::NodeBase* node;
		const bool isConst;
		TypeInfo type;
		
	};

public:

	NativeJIT::Function<R, Parameters...> expr;

	FunctionParser(const String &code, NativeJIT::Allocator& allocator, NativeJIT::FunctionBuffer& codeAllocator) :
		ParserHelpers::TokenIterator(code),
		expr(allocator, codeAllocator)
	{
		exposedFunctions.add(new TypedFunction<float, float>("sinf", sinf));
		exposedFunctions.add(new TypedFunction<double>("getSampleRate", getSampleRate));
		exposedFunctions.add(new TypedFunction<float, float, float>("powf", powf));

		globals.add(GlobalBase::create<float>("value"));

	}
	
	OwnedArray<BaseFunction> exposedFunctions;

	OwnedArray<NamedNode> stackVariables;

	OwnedArray<GlobalBase> globals;

	void parseFunctionBody()
	{
		lines.clear();

		
		while (currentType != NativeJitTokens::eof)
		{
			if (currentType == NativeJitTokens::identifier)
			{
				const Identifier id = parseIdentifier();

				auto g = getGlobal(id);

				if (g != nullptr)
				{
					if (NativeJITTypeHelpers::matchesType<float>(g->type))
					{
						match(NativeJitTokens::assign_);
						auto& exp = parseExpression<float>();
						match(NativeJitTokens::semicolon);

						auto& gl = expr.Immediate(g);

						auto& f1 = expr.Immediate(GlobalBase::store<float>);
						auto& f2 = expr.Call(f1, gl, exp);

						auto lastLine = getLine(lastParsedLine);

						if (lastLine != nullptr)
						{
							auto& wrapped = expr.Dependent(f2, *lastLine->node);
							lines.add(new NamedNode(id, &wrapped, false, typeid(float)));
						}
						else
						{
							lines.add(new NamedNode(id, &f2, false, typeid(float)));
						}

						lastParsedLine = id;
					}
				}
				else
				{
					location.throwError("Global variable not found");
				}
			}
			else if (matchIf(NativeJitTokens::const_))
			{
				if (matchIf(NativeJitTokens::float_))		parseLine<float>(true);
				else if (matchIf(NativeJitTokens::int_))	parseLine<int>(true);
				else if (matchIf(NativeJitTokens::double_))	parseLine<double>(true);
			}
			else
			{
				if (matchIf(NativeJitTokens::float_))		parseLine<float>(false);
				else if (matchIf(NativeJitTokens::int_))	parseLine<int>(false);
				else if (matchIf(NativeJitTokens::double_))	parseLine<double>(false);
				else if (matchIf(NativeJitTokens::return_)) parseReturn();
			}
			
		}
	}

	GlobalBase* getGlobal(const Identifier& id)
	{
		for (int i = 0; i < globals.size(); i++)
		{
			if (globals[i]->id == id) return globals[i];
		}

		return nullptr;
	}

	NamedNode* getStackVariable(const Identifier &id)
	{
		for (int i = 0; i < stackVariables.size(); i++)
		{
			if (stackVariables[i]->id == id) return stackVariables[i];
		}

		return nullptr;
	}

	NamedNode* getLine(const Identifier& id)
	{
		for (int i = 0; i < lines.size(); i++)
		{
			if (lines[i]->id == id) return lines[i];
		}

		return nullptr;
	}

	template <typename T> void parseLine(bool isConst)
	{
		Identifier id = parseIdentifier();

		auto existingLine = getLine(id);

		if(existingLine != nullptr)
		{
			location.throwError("Identifier " + id.toString() + " already defined");
		}

		match(NativeJitTokens::assign_);
		auto& r = parseExpression<T>();
		match(NativeJitTokens::semicolon);

		auto lastLine = getLine(lastParsedLine);

		if (isConst)
		{
			if (lastLine != nullptr)
			{
				auto& wrapped = expr.Dependent(r, *lastLine->node);
				lines.add(new NamedNode(id, &wrapped, isConst, typeid(T)));
			}
			else
			{
				lines.add(new NamedNode(id, &r, isConst, typeid(T)));
			}
		}
		else
		{
			auto& e1 = expr.StackVariable<T>();

			stackVariables.add(new NamedNode(id, &e1, false, typeid(T)));

			
			auto& e3 = expr.Immediate(storeInStack<T>);
			auto& e4 = expr.Call(e3, e1, r);

			if (lastLine != nullptr)
			{
				auto& wrapped = expr.Dependent(e4, *lastLine->node);
				lines.add(new NamedNode(id, &wrapped, isConst, typeid(T)));
			}
			else
			{	
				lines.add(new NamedNode(id, &e4, isConst, typeid(T)));
			}
		}
		
		lastParsedLine = id;
	}

		
	template <typename LineType> NativeJIT::Node<LineType>& parseExpression()
	{
		return parseSum<LineType>();
	}

	template <typename T> NativeJIT::Node<T>& parseSum()
	{
		auto& left = parseProduct<T>();

		if (matchIf(NativeJitTokens::plus))
		{
			auto& right = parseSum<T>();
			return expr.Add(left, right);
		}
		else if (matchIf(NativeJitTokens::minus))
		{
			auto& right = parseSum<T>();
			return expr.Sub(left, right);
		}
		else
		{
			return left;
		}
	}

	void parseReturn()
	{
		returnStatement = &parseSum<R>();
		match(NativeJitTokens::semicolon);
	}

	template <typename T> NativeJIT::Node<T>& parseProduct()
	{
		auto& left = parseTerm<T>();

		if (matchIf(NativeJitTokens::times))
		{
			auto& right = parseProduct<T>();
			return expr.Mul(left, right);
		}
		else if (matchIf(NativeJitTokens::divide))
		{
			auto& right = parseProduct<T>();
			auto& f1 = expr.Immediate(divideOp<T>);

			return expr.Call(f1, left, right);
		}
		else
		{
			return left;
		}
	}

	TypeInfo peekFirstType()
	{
		TokenIterator peeker(location.location);

		while (peeker.currentType == NativeJitTokens::openParen)
			peeker.skip();

		if (peeker.currentType == NativeJitTokens::identifier)
		{
			const Identifier currentId(currentValue);

			if (auto r = getLine(currentId))
			{
				if (dynamic_cast<NativeJIT::Node<float>*>(r->node)) return typeid(float);
				if (dynamic_cast<NativeJIT::Node<double>*>(r->node)) return typeid(double);
				if (dynamic_cast<NativeJIT::Node<int>*>(r->node)) return typeid(int);
			}

			if (auto f = getExposedFunction(currentId))
			{
				return f->getReturnType();
			}
		}

		if (peeker.currentType == NativeJitTokens::double_) return typeid(double);
		if (peeker.currentType == NativeJitTokens::int_) return typeid(int);
		if (peeker.currentType == NativeJitTokens::float_) return typeid(float);

		return NativeJITTypeHelpers::getTypeForLiteral(peeker.currentString);
	}

	template <typename TargetType, typename ExpectedType> NativeJIT::Node<TargetType>& parseCast()
	{
		if (!NativeJITTypeHelpers::is<TargetType, ExpectedType>())
		{
			location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType, TargetType>());
		}

		match(NativeJitTokens::closeParen);

		auto sourceType = peekFirstType();

		if (NativeJITTypeHelpers::matchesType<float>(sourceType))
		{
			auto &e1 = parseTerm<float>();
			return expr.Cast<TargetType>(e1);
		}
		else if (NativeJITTypeHelpers::matchesType<double>(sourceType))
		{
			auto &e1 = parseTerm<double>();
			return expr.Cast<TargetType>(e1);
		}
		else if (NativeJITTypeHelpers::matchesType<int>(sourceType))
		{
			auto &e1 = parseTerm<int>();
			return expr.Cast<TargetType>(e1);
		}
		else
		{
			location.throwError("Unsupported type");
		}
	}

	template <typename T> NativeJIT::Node<T>& parseTerm()
	{
		if (matchIf(NativeJitTokens::openParen))
		{
			if (matchIf(NativeJitTokens::float_))       return parseCast<T, float>();
			else if (matchIf(NativeJitTokens::int_))    return parseCast<T, int>();
			else if (matchIf(NativeJitTokens::double_)) return parseCast<T, double>();
			else
			{
				auto& result = parseSum<T>();
				match(NativeJitTokens::closeParen);
				return result;
			}
		}
		else if (matchIf(NativeJitTokens::identifier))	return parseSymbol<T>();
		else if (matchIf(NativeJitTokens::literal))		return parseLiteral<T>();
		else
		{
			location.throwError("Parsing error");
		}

	}

	template <typename T> NativeJIT::Node<T>& parseSymbol()
	{
		Identifier symbolId = Identifier(currentValue);

#if 0
		if (auto s = getStackVariable(symbolId))
		{
			if (NativeJIT::Node<T&>* n = dynamic_cast<NativeJIT::Node<T&>*>(s->node))
			{
				auto& e = expr.Deref(*n);

				return e;
			}
				
			else
				location.throwError("Type mismatch for Identifier " + currentValue.toString());
		}
#endif

		if (auto g = getGlobal(symbolId))
		{
			return parseGlobalReference<T>(symbolId);
		}
		if (auto r = getLine(symbolId))
		{
			if (NativeJIT::Node<T>* n = dynamic_cast<NativeJIT::Node<T>*>(r->node))
				return *n;
			else
				location.throwError("Type mismatch for Identifier " + currentValue.toString());
		}
		else if (BaseFunction* b = getExposedFunction(symbolId))
		{
			return parseFunctionCall<T>();
		}
		else
		{
			location.throwError("Unknown identifier " + symbolId.toString());
		}
	}

	template <typename T> NativeJIT::Node<T>& parseGlobalReference(const Identifier& id)
	{
		auto r = getGlobal(id);

		auto& e1 = expr.Immediate(GlobalBase::get<T>);

		auto& g = expr.Immediate(r);

		return expr.Call(e1, g);
	}

	template <typename T> NativeJIT::Node<T>& parseFunctionCall()
	{
		BaseFunction* b = getExposedFunction(Identifier(currentValue));

		jassert(b != nullptr);

		match(NativeJitTokens::openParen);

		Array<NativeJIT::NodeBase*> parameterNodes;
		std::vector<TypeInfo> parameterTypes;

		while (currentType != NativeJitTokens::closeParen)
		{
			auto t = peekFirstType();

			parameterTypes.push_back(t);

			if (NativeJITTypeHelpers::matchesType<float>(t)) parameterNodes.add(&parseSum<float>());
			else if (NativeJITTypeHelpers::matchesType<double>(t)) parameterNodes.add(&parseSum<double>());
			else if (NativeJITTypeHelpers::matchesType<int>(t)) parameterNodes.add(&parseSum<int>());
			else location.throwError("Type unknown");

			matchIf(NativeJitTokens::comma);
		}

		switch (parameterTypes.size())
		{
		case 0:
		{
			return parseFunctionParameterList<T>(b);
		}
		case 1:
		{
			if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[0])) return parseFunctionParameterList<T, float>(b, parameterNodes[0]);
			else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[0])) return parseFunctionParameterList<T, int>(b, parameterNodes[0]);
			else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[0])) return parseFunctionParameterList<T, double>(b, parameterNodes[0]);
		}
		case 2:
		{
			if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[0]))
			{
				if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, float, float>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, float, int>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, float, double>(b, parameterNodes[0], parameterNodes[1]);
			}
			else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[0]))
			{
				if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, int, float>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, int, int>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, int, double>(b, parameterNodes[0], parameterNodes[1]);
			}
			else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[0]))
			{
				if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, double, float>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, double, int>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, double, double>(b, parameterNodes[0], parameterNodes[1]);
			}
		}
			
		}

		location.throwError("Function parsing error");

		
	}

	template <typename R> NativeJIT::Node<R>& parseFunctionParameterList(BaseFunction* b)
	{
		R func();
		using f_p = decltype(func);

		auto function = (f_p*)(b->func);

		if (function != nullptr)
		{
			auto& e1 = expr.Immediate(function);
			auto& e3 = expr.Call(e1);
			match(NativeJitTokens::closeParen);
			return e3;
		}
		else
		{
			location.throwError("Function type mismatch");
		}
	}

	template <typename R, typename ParamType> NativeJIT::Node<R>& parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1)
	{
		R func(ParamType);
		using f_p = decltype(func);

		auto function = (f_p*)(b->func);

		if (function != nullptr)
		{
			auto& e1 = expr.Immediate(function);

			auto e2 = dynamic_cast<NativeJIT::Node<ParamType>*>(param1);

			if (e2 != nullptr)
			{
				auto& e3 = expr.Call(e1, *e2);
				match(NativeJitTokens::closeParen);
				return e3;
			}
			else
			{
				location.throwError("Parameter 1: Type mismatch. Expected: " + NativeJITTypeHelpers::getTypeName<ParamType>());
			}
		}
		else
		{
			location.throwError("Function type mismatch");
		}
	}

	template <typename R, typename ParamType1, typename ParamType2> NativeJIT::Node<R>& parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1, NativeJIT::NodeBase* param2)
	{
		R func(ParamType1, ParamType2);
		using f_p = decltype(func);

		auto function = (f_p*)(b->func);

		if (function != nullptr)
		{
			auto& e1 = expr.Immediate(function);

			auto e2 = dynamic_cast<NativeJIT::Node<ParamType1>*>(param1);

			if (e2 == nullptr)
			{
				location.throwError("Parameter 1: Type mismatch. Expected: " + NativeJITTypeHelpers::getTypeName<ParamType1>());
			}

			auto e3 = dynamic_cast<NativeJIT::Node<ParamType2>*>(param2);

			if (e3 != nullptr)
			{
				auto& e4 = expr.Call(e1, *e2, *e3);
				match(NativeJitTokens::closeParen);
				return e4;
			}
			else
			{
				location.throwError("Parameter 2: Type mismatch. Expected: " + NativeJITTypeHelpers::getTypeName<ParamType2>());
			}
		}
		else
		{
			location.throwError("Function type mismatch");
		}
	}

	BaseFunction* getExposedFunction(const Identifier& id)
	{
		for (int i = 0; i < exposedFunctions.size(); i++)
		{
			if (exposedFunctions[i]->functionName == id)
			{
				return exposedFunctions[i];
			}
		}

		return nullptr;
	}

	template <typename T> NativeJIT::Node<T>& parseLiteral()
	{
		T value = (T)currentValue;

		if (!NativeJITTypeHelpers::matchesType<T>(currentString))
		{
			location.throwError("Type mismatch: " + NativeJITTypeHelpers::getTypeName(currentString) + ", Expected: " + NativeJITTypeHelpers::getTypeName<T>());
		}

		return expr.Immediate(value);
	}

	typedef R (*FuncPointer)(float);

	FuncPointer compileFunction()
	{
		if (returnStatement == nullptr)
		{
			location.throwError("No return value specified");
		}

		try
		{
			if (lines.size() != 0)
			{
				auto& wrapped = expr.Dependent(*returnStatement, *lines.getLast()->node);

				return expr.Compile(wrapped);
			}
			else
			{
				return expr.Compile(*returnStatement);
			}
		}
		catch (std::runtime_error e)
		{

		}
	}


private:

	OwnedArray<NamedNode> lines;

	NativeJIT::Node<R>* returnStatement = nullptr;

	Identifier lastParsedLine;

};

#include "Parser.cpp"


#endif  // PARSER_H_INCLUDED
