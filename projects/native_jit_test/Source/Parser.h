/*
  ==============================================================================

    Parser.h
    Created: 23 Feb 2017 1:20:45pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED


#define INCLUDE_COMPLEX_TEMPLATES 1


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

	static String getTypeName(const TypeInfo& info);

	/** Creates a error message if the types don't match. */
	template <typename ActualType, typename ExpectedType> static String getTypeMismatchErrorMessage();

	template <typename ExpectedType> static String getTypeMismatchErrorMessage(const TypeInfo& actualType);

	/** Returns the type ID for the given String literal. Currently supported: double, float & int. */
	static TypeInfo getTypeForLiteral(const String &t);;

	/** Checks if the given type ID matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const TypeInfo& actualType);

	/** Checks if the given string matches the expected type. */
	template <typename ExpectedType> static bool matchesType(const String& t);;

	/** Checks if the given token matches the type. */
	template <typename ExpectedType> static bool matchesToken(const char* token)
	{
		return matchesType<ExpectedType>(getTypeForToken(token));
	}

	template <typename ExpectedType> static bool matchesToken(const Identifier& tokenName)
	{
		return matchesType<ExpectedType>(getTypeForToken(tokenName.toString().getCharPointer()));
	}

	static TypeInfo getTypeForToken(const char* token)
	{
		if (String(token) == String(NativeJitTokens::double_)) return typeid(double);
		else if (String(token) == String(NativeJitTokens::int_))  return typeid(int);
		else if (String(token) == String(NativeJitTokens::float_))  return typeid(float);
		else return typeid(void);
	}

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

	std::vector<TypeInfo> parameterTypes;

private:

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseFunction)
};

struct GlobalBase
{
	GlobalBase(const Identifier& id_, TypeInfo type_) :
		id(id_),
		type(type_)
	{
		
	};

	

	template<typename T> static T store(GlobalBase* b, T newValue)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		*reinterpret_cast<T*>(&b->value) = newValue;

		return (T)0;
	}

	template<typename T> static T get(GlobalBase* b)
	{
		jassert(NativeJITTypeHelpers::matchesType<T>(b->type));

		return *reinterpret_cast<T*>(&b->value);
	}

	TypeInfo getType() const noexcept { return type; }
	
	template <typename T> static GlobalBase* create(const Identifier& id)
	{
		return new GlobalBase(id, typeid(T));
	}

	

	TypeInfo type;
	Identifier id;
	
	double value = 0.0;
	
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

	TypedFunction(const Identifier& id, void* func, Parameters... types) :
		BaseFunction(id, func)
	{
		addTypeInfo(parameterTypes, types...);
	};

	int getNumParameters() const override { return sizeof...(Parameters); };

	virtual TypeInfo getReturnType() const override { return typeid(R); };

private:

	template <typename P, typename... Ps> void addTypeInfo(std::vector<TypeInfo>& info, P value, Ps... otherInfo)
	{
		info.push_back(typeid(value));
		addTypeInfo(info, otherInfo...);
	}

	template<typename P> void addTypeInfo(std::vector<TypeInfo>& info, P value) { info.push_back(typeid(value)); }

	void addTypeInfo(std::vector<TypeInfo>& info) {}

};

#define NATIVE_JIT_ADD_C_FUNCTION_0(return_type, name) exposedFunctions.add(new TypedFunction<return_type>(Identifier(#name), (void*)name)); 
#define NATIVE_JIT_ADD_C_FUNCTION_1(return_type, name, param1_type) exposedFunctions.add(new TypedFunction<return_type, param1_type>(Identifier(#name), (void*)name, param1_type())); 
#define NATIVE_JIT_ADD_C_FUNCTION_2(return_type, name, param1_type, param2_type) exposedFunctions.add(new TypedFunction<return_type, param1_type, param2_type>(Identifier(#name), (void*)name, param1_type(), param2_type())); 
#define ADD_C_FUNCTION_3(return_type, name, param1_type, param2_type, param3_type) exposedFunctions.add(new TypedFunction<return_type, param1_type, param2_type, param3_type>(Identifier(#name), (void*)name, param1_type(), param2_type(), param3_type())); 

#define ADD_GLOBAL(type, name) globals.add(GlobalBase::create<type>(name));

class GlobalScope : public DynamicObject
{
public:

	GlobalScope() :
		codeAllocator(32768),
		allocator(32768)
	{
		addExposedFunctions();
	}

	void addExposedFunctions()
	{
		NATIVE_JIT_ADD_C_FUNCTION_1(float, sinf, float);
		NATIVE_JIT_ADD_C_FUNCTION_0(double, getSampleRate);
		NATIVE_JIT_ADD_C_FUNCTION_2(float, powf, float, float);
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

	GlobalBase* getGlobal(const Identifier& id)
	{
		for (int i = 0; i < globals.size(); i++)
		{
			if (globals[i]->id == id) return globals[i];
		}

		return nullptr;
	}

	BaseFunction* getCompiledBaseFunction(const Identifier& id)
	{
		BaseFunction* b = nullptr;

		for (int i = 0; i < compiledFunctions.size(); i++)
		{
			if (compiledFunctions[i]->functionName == id)
			{
				return compiledFunctions[i];
			}
		}

		return nullptr;
	}

	template <typename ExpectedType> void checkTypeMatch(BaseFunction* b, int parameterIndex)
	{
		if (parameterIndex == -1)
		{
			if (!NativeJITTypeHelpers::matchesType<ExpectedType>(b->getReturnType()))
			{
				throw String("Return type of function \"" + b->functionName + "\" does not match. ") + NativeJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType>(b->getReturnType());
			}
		}
		else
		{
			if (!NativeJITTypeHelpers::matchesType<ExpectedType>(b->getTypeForParameter(parameterIndex)))
			{
				throw String("Parameter " + String(parameterIndex+1) + " type of function \"" + b->functionName + "\" does not match. ") + NativeJITTypeHelpers::getTypeMismatchErrorMessage<ExpectedType>(b->getReturnType());
			}
		}
	}

	template <typename ReturnType> ReturnType(*getCompiledFunction0(const Identifier& id))()
	{
		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			
			return (ReturnType(*)())b->func;
		}

		return nullptr;
	}

	template <typename ReturnType, typename ParamType1> ReturnType (*getCompiledFunction1(const Identifier& id))(ParamType1)
	{
		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			checkTypeMatch<ParamType1>(b, 0);

			return (ReturnType(*)(ParamType1))b->func;
		}

		return nullptr;
	}

	template <typename ReturnType, typename ParamType1, typename ParamType2> ReturnType(*getCompiledFunction2(const Identifier& id))(ParamType1, ParamType2)
	{
		auto b = getCompiledBaseFunction(id);

		if (b != nullptr)
		{
			checkTypeMatch<ReturnType>(b, -1);
			checkTypeMatch<ParamType1>(b, 0);
			checkTypeMatch<ParamType2>(b, 1);

			return (ReturnType(*)(ParamType1, ParamType2))b->func;
		}

		return nullptr;
	}

	NativeJIT::FunctionBuffer* createFunctionBuffer()
	{
		functionBuffers.add(new NativeJIT::FunctionBuffer(codeAllocator, 8192));

		return functionBuffers.getLast();
	}

	OwnedArray<GlobalBase> globals;

	OwnedArray<BaseFunction> compiledFunctions;

	OwnedArray<BaseFunction> exposedFunctions;

	Array<NativeJIT::FunctionBuffer*> functionBuffers;

	typedef ReferenceCountedObjectPtr<GlobalScope> Ptr;

	NativeJIT::ExecutionBuffer codeAllocator;
	NativeJIT::Allocator allocator;
};

#if INCLUDE_COMPLEX_TEMPLATES
template <typename R, typename...Parameters> class GlobalNode
{
public:

	GlobalNode(GlobalBase* b_) : b(b_) {};

	bool matchesName(const Identifier& id)
	{
		return b->id == id;
	}

	using TypedFunction = NativeJIT::Function<R, Parameters...>;

	template <typename T> void checkType(NativeJIT::Node<T>& n)
	{
		if (!NativeJITTypeHelpers::matchesType<T>(b->type))
		{
			throw String("Global Reference parsing error");
		}
	}

	template <typename T> void addNode(TypedFunction& expr, NativeJIT::Node<T>& newNode)
	{
		checkType(newNode);

		if (nodes.size() == 0)
		{
			nodes.add(&newNode);
		}
		else
		{
			auto& lastNode = getLastNode<T>();
			auto& zero = expr.template Immediate<T>(0);
			auto& zeroed = expr.Mul(lastNode, zero);
			auto& added = expr.template Add<T>(zeroed, newNode);

			nodes.add(&added);
		}
	}

	template <typename T> NativeJIT::Node<T>& getLastNode() const
	{
		auto n = dynamic_cast<NativeJIT::Node<T>*>(nodes.getLast());

		if (n == nullptr)
		{
			throw "Global Type mismatch";
		}

		return *n;
	}

	template <typename T> NativeJIT::Node<R>& callStoreFunction(TypedFunction& expr)
	{
		auto& t = getLastNode<T>();
		auto& f1 = expr.Immediate(GlobalBase::store<T>);
		auto& gb = expr.Immediate(b);
		auto& f2 = expr.Call(f1, gb, t); 

		if (typeid(R) == typeid(T))
		{
			return *dynamic_cast<NativeJIT::Node<R>*>(&f2);
		}
		else
		{
			auto& cast = expr.template Cast<R, T>(f2);

			return cast;
		}
	}



	NativeJIT::Node<R>& getAssignmentNodeForReturnStatement(TypedFunction& expr)
	{
		if (NativeJITTypeHelpers::matchesType<float>(b->type)) return callStoreFunction<float>(expr);
		if (NativeJITTypeHelpers::matchesType<double>(b->type)) return callStoreFunction<double>(expr);
		if (NativeJITTypeHelpers::matchesType<int>(b->type)) return callStoreFunction<int>(expr);

		return expr.Immediate(R());
	}

private: 

	

	GlobalBase* b;

	Array<NativeJIT::NodeBase*> nodes;
};
#endif


struct FunctionInfo
{
	Array<Identifier> parameterTypes;
	Array<Identifier> parameterNames;

	String code;
	
	String program;

	int offset = 0;

	int parameterAmount = 0;
};

struct HelperFunctions
{
    template <typename R, typename A, typename B> static R assertEqual(A a, B b)
    {
        if(a != static_cast<A>(b))
        {
            Logger::writeToLog(String("Assertion failure: " + String(a) + ", Expected: " + String(b)));
            
            return static_cast<R>(1);
        }
        
        return static_cast<R>(0);
    };
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

	
	using TypedNativeJITFunction = NativeJIT::Function<R, Parameters...>;

#if INCLUDE_COMPLEX_TEMPLATES

	using TypedGlobalNode = GlobalNode<R, Parameters...>;

	OwnedArray<TypedGlobalNode> globalNodes;

	template <typename R2, typename...Parameters2> struct MissingOperatorFunctions
	{
		template <typename T> static T divideOp(T a, T b)
		{
			return a / b;
		}

		static int moduloOp(int a, int b)
		{
			return a % b;
		}

		using ModuloFunction = int(*)(int, int);

		template <typename T> using DivideFunction = T(*)(T, T);

		template <typename T> using FunctionNode = NativeJIT::Node<T>;

		template <typename T> FunctionNode<DivideFunction<T>>& getDivideFunction(TypedNativeJITFunction& expr)
		{
			if (NativeJITTypeHelpers::is<T, int>())
			{
				if (divideIntFunction == nullptr) 
					divideIntFunction = &expr.Immediate(divideOp<int>);

				return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideIntFunction);
			}
			else if (NativeJITTypeHelpers::is<T, float>())
			{
				if (divideFloatFunction == nullptr) 
					divideFloatFunction = &expr.Immediate(divideOp<float>);

				return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideFloatFunction);
			}
			else if (NativeJITTypeHelpers::is<T, double>())
			{
				if (divideDoubleFunction == nullptr) 
					divideDoubleFunction = &expr.Immediate(divideOp<double>);

				return *dynamic_cast<FunctionNode<DivideFunction<T>>*>(divideDoubleFunction);
			}

			return expr.template Immediate<DivideFunction<T>>(0);
		}

		NativeJIT::Node<ModuloFunction>& getModuloFunction(TypedNativeJITFunction& expr)
		{
			if (moduloIntFunction == nullptr)
			{
				moduloIntFunction = &expr.Immediate(moduloOp);
			}

			return *moduloIntFunction;
		};

	private:

		FunctionNode<ModuloFunction>* moduloIntFunction = nullptr;

		FunctionNode<DivideFunction<int>>* divideIntFunction = nullptr;
		FunctionNode<DivideFunction<float>>* divideFloatFunction = nullptr;
		FunctionNode<DivideFunction<double>>* divideDoubleFunction = nullptr;
	};


	MissingOperatorFunctions<R, Parameters...> missingOperatorFunctions;

#endif

	GlobalScope::Ptr scope;

	const FunctionInfo& info;

	TypedNativeJITFunction expr;
    
	FunctionParser(GlobalScope* scope_, const FunctionInfo& info_) :
		ParserHelpers::TokenIterator(info_.code),
		scope(scope_),
		info(info_),
		expr(scope->allocator, *scope->createFunctionBuffer())
	{}
	
	void parseFunctionBody()
	{
		lines.clear();

		while (currentType != NativeJitTokens::eof && currentType != NativeJitTokens::closeBrace)
		{
			
			if (currentType == NativeJitTokens::identifier)
			{
				const Identifier id = parseIdentifier();

				auto g = scope->getGlobal(id);

				if (g != nullptr)
				{
#if INCLUDE_COMPLEX_TEMPLATES
					if (NativeJITTypeHelpers::matchesType<float>(g->type)) parseGlobalAssignment<float>(g);
					if (NativeJITTypeHelpers::matchesType<double>(g->type)) parseGlobalAssignment<double>(g);
					if (NativeJITTypeHelpers::matchesType<int>(g->type)) parseGlobalAssignment<int>(g);
#endif
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
			else if (matchIf(NativeJitTokens::float_))		parseLine<float>(false);
			else if (matchIf(NativeJitTokens::int_))	parseLine<int>(false);
			else if (matchIf(NativeJitTokens::double_))	parseLine<double>(false);
			else if (matchIf(NativeJitTokens::return_)) parseReturn();
			
			else match(NativeJitTokens::eof);
			
		}
	}

#if INCLUDE_COMPLEX_TEMPLATES
	template <typename T> void parseGlobalAssignment(GlobalBase* g)
	{
		enum AssignType
		{
			Assign = 0,
			Add,
			Sub,
			Mul,
			Div,
			Mod,
			numAssignTypes
		};

		AssignType assignType;

		if (matchIf(NativeJitTokens::minusEquals)) assignType = Sub;
		else if (matchIf(NativeJitTokens::divideEquals)) assignType = Div;
		else if (matchIf(NativeJitTokens::timesEquals)) assignType = Mul;
		else if (matchIf(NativeJitTokens::plusEquals)) assignType = Add;
		else if (matchIf(NativeJitTokens::moduloEquals)) assignType = Mod;
		else
		{
			match(NativeJitTokens::assign_);
			assignType = Assign;
		}

		auto& exp = parseExpression<T>();

		NativeJIT::Node<T>* newNode = &exp;
		
		NativeJIT::Node<T>* old = nullptr;

		auto existingNode = getGlobalNode(g->id);
		
		if (existingNode == nullptr) old = &getGlobalReference<T>(g->id);
		else						 old = &existingNode->template getLastNode<T>();

		switch (assignType)
		{
		
		case Add:	newNode = &expr.Add(*old, *newNode); break;
		case Sub:	newNode = &expr.Sub(*old, *newNode); break;
		case Mul:	newNode = &expr.Add(*old, *newNode); break;
		case Div:	newNode = &expr.Call(missingOperatorFunctions.template getDivideFunction<T>(expr), *old, *newNode); break;
		case Mod:	newNode = dynamic_cast<NativeJIT::Node<T>*>(&expr.Call(missingOperatorFunctions.getModuloFunction(expr), *dynamic_cast<NativeJIT::Node<int>*>(old), *dynamic_cast<NativeJIT::Node<int>*>(newNode))); break;
		case numAssignTypes:
		case Assign:
		default:
			break;
		}

		match(NativeJitTokens::semicolon);

		if (existingNode != nullptr)
		{
			existingNode->template addNode<T>(expr, *newNode);
		}
		else
		{
			ScopedPointer<TypedGlobalNode> newGlobalNode = new TypedGlobalNode(g);

			newGlobalNode->template addNode<T>(expr, *newNode);

			globalNodes.add(newGlobalNode.release());
		}
	}


	TypedGlobalNode* getGlobalNode(const Identifier& id)
	{
		for (int i = 0; i < globalNodes.size(); i++)
		{
			if (globalNodes[i]->matchesName(id))
				return globalNodes[i];
		}

		return nullptr;
	}
#endif

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
#if 0
			auto& e1 = expr.template StackVariable<T>();

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
#endif
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

	void addVoidReturnStatement()
	{
		//jassert(NativeJITTypeHelpers::is<R, int>());

		returnStatement = &expr.template Immediate<R>(0);

	}

	void finalizeReturnStatement()
	{
        returnStatement = &expr.Add(*returnStatement, expr.template Immediate<R>(0));
        
		for (int i = 0; i < lines.size(); i++)
		{
			if (!lines[i]->node->IsReferenced())
			{
				location.throwError("Unused variable: " + lines[i]->id.toString());
			}
		}

#if INCLUDE_COMPLEX_TEMPLATES
		for (int i = 0; i < globalNodes.size(); i++)
		{
			auto& zero = globalNodes[i]->getAssignmentNodeForReturnStatement(expr);

			returnStatement = &expr.Add(*returnStatement, zero);
		}
#endif

		
	}


	void parseReturn()
	{
		returnStatement = &parseSum<R>();
		match(NativeJitTokens::semicolon);
	}

	

	template <typename T> NativeJIT::Node<T>& parseProduct()
	{
		auto& left = parseCondition<T>();

		if (matchIf(NativeJitTokens::times))
		{
			auto& right = parseProduct<T>();
			return expr.Mul(left, right);
		}
#if INCLUDE_COMPLEX_TEMPLATES
		else if (matchIf(NativeJitTokens::divide))
		{

			auto& right = parseProduct<T>();
			auto& f1 = missingOperatorFunctions.template getDivideFunction<T>(expr);

			return expr.Call(f1, left, right);
		}
		else if (matchIf(NativeJitTokens::modulo))
		{
			if (NativeJITTypeHelpers::is<T, int>())
			{
				auto& right = parseProduct<T>();
				
				auto& f1 = missingOperatorFunctions.getModuloFunction(expr);

				auto castedLeft = dynamic_cast<NativeJIT::Node<int>*>(&left);
				auto castedRight = dynamic_cast<NativeJIT::Node<int>*>(&right);

				auto& result = expr.Call(f1, *castedLeft, *castedRight);

				return *dynamic_cast<NativeJIT::Node<T>*>(&result);
			}
			else
			{
				throw String("Modulo operation on " + NativeJITTypeHelpers::getTypeName<T>() + " type");
			}

		}
#endif
		else
		{
			return left;
		}
	}

	template <typename T> NativeJIT::Node<T>& parseCondition()
	{
		if (NativeJITTypeHelpers::matchesType<float>(peekFirstType()))		 return parseTernaryOperator<T, float>();
		else if (NativeJITTypeHelpers::matchesType<double>(peekFirstType())) return parseTernaryOperator<T, double>();
		else if (NativeJITTypeHelpers::matchesType<int>(peekFirstType()))	 return parseTernaryOperator<T, int>();
		else location.throwError("Parsing error");

		return getEmptyNode<T>();
	}

    template <typename T, typename ConditionType, NativeJIT::JccType compareFlag> NativeJIT::Node<T>& parseBranches(NativeJIT::Node<ConditionType>& left)
	{
		auto& right = parseConditional<ConditionType>();
		auto& a = expr.template Compare<compareFlag>(left, right);

		match(NativeJitTokens::question);

		auto& true_b = parseTerm<T>();

		match(NativeJitTokens::colon);

		auto& false_b = parseTerm<T>();

		return expr.Conditional(a, true_b, false_b);
	}

	template <typename T> NativeJIT::Node<T>& parseConditional()
	{
		auto& left = parseTerm<T>();

		if (matchIf(NativeJitTokens::logicalAnd))
		{
			auto& right = parseConditional<T>();

            auto& a = expr.template Compare<NativeJIT::JccType::JE>(left, right);

			return expr.Conditional(a, expr.Immediate((T)1), expr.Immediate((T)0));
		}
		else
		{
			return left;
		}
	}

	template <typename T, typename ConditionType> NativeJIT::Node<T>& parseSmallCond()
	{
		auto& left = parseConditional<ConditionType>();

		return left;
	}

	template <typename T, typename ConditionType> NativeJIT::Node<T>& parseTernaryOperator()
	{
		auto& left = parseConditional<ConditionType>();

		if (matchIf(NativeJitTokens::greaterThan))
		{
			return parseBranches<T, ConditionType, NativeJIT::JccType::JG>(left);
		}
		else if (matchIf(NativeJitTokens::greaterThanOrEqual))
		{
			return parseBranches<T, ConditionType, NativeJIT::JccType::JGE>(left);
		}
		else if (matchIf(NativeJitTokens::lessThan))
		{
			return parseBranches<T, ConditionType, NativeJIT::JccType::JL>(left);
		}
		else if (matchIf(NativeJitTokens::lessThanOrEqual))
		{
			return parseBranches<T, ConditionType, NativeJIT::JccType::JLE>(left);
		}
		else if (matchIf(NativeJitTokens::equals))
		{
			return parseBranches<T, ConditionType, NativeJIT::JccType::JE>(left);
		}
		else if (matchIf(NativeJitTokens::notEquals))
		{
			return parseBranches<T, ConditionType, NativeJIT::JccType::JNE>(left);
		}
		else
			return *dynamic_cast<NativeJIT::Node<T>*>(&left);
	}

	TypeInfo peekFirstType()
	{
        const String trimmedCode(location.location);
        
		TokenIterator peeker(trimmedCode);

		while (peeker.currentType == NativeJitTokens::openParen)
			peeker.skip();

		if (peeker.currentType == NativeJitTokens::identifier)
		{
			const Identifier currentId(currentValue);

			int pIndex = info.parameterNames.indexOf(currentId);

			if (pIndex != -1)
			{
				Identifier typeId = info.parameterTypes[pIndex];

				if (typeId.toString() == NativeJitTokens::float_) return typeid(float);
				if (typeId.toString() == NativeJitTokens::double_) return typeid(double);
				if (typeId.toString() == NativeJitTokens::int_) return typeid(int);
			}
			if (auto r = getLine(currentId))
			{
				if (dynamic_cast<NativeJIT::Node<float>*>(r->node)) return typeid(float);
				if (dynamic_cast<NativeJIT::Node<double>*>(r->node)) return typeid(double);
				if (dynamic_cast<NativeJIT::Node<int>*>(r->node)) return typeid(int);
			}

			if (auto f = scope->getExposedFunction(currentId))
			{
				return f->getReturnType();
			}

            if (auto f = scope->getCompiledBaseFunction(currentId))
            {
                return f->getReturnType();
            }
            
			if (auto g = scope->getGlobal(currentId))
			{
				return g->getType();
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
			return expr.template Cast<TargetType>(e1);
		}
		else if (NativeJITTypeHelpers::matchesType<double>(sourceType))
		{
			auto &e1 = parseTerm<double>();
			return expr.template Cast<TargetType>(e1);
		}
		else if (NativeJITTypeHelpers::matchesType<int>(sourceType))
		{
			auto &e1 = parseTerm<int>();
			return expr.template Cast<TargetType>(e1);
		}
		else
		{
			location.throwError("Unsupported type");
		}

		return getEmptyNode<TargetType>();
	}

	template <typename T> NativeJIT::Node<T>& getEmptyNode()
	{
		return expr.Immediate(T());
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
		else if (currentType == NativeJitTokens::identifier || currentType == NativeJitTokens::literal)
			return parseFactor<T>();
		else
		{
			location.throwError("Parsing error");
		}

		return getEmptyNode<T>();
	}

	template <typename T> NativeJIT::Node<T>& parseFactor()
	{
		TypeInfo t = peekFirstType();

		if (NativeJITTypeHelpers::matchesType<T>(t))
		{
            auto& e = parseSymbolOrLiteral<T>();
            
            return e;
		}
		else
		{
            location.throwError("Parsing error");
		}
        
        return getEmptyNode<T>();
	}

	template <typename T> NativeJIT::Node<T>& parseSymbolOrLiteral()
	{
		

		if (matchIf(NativeJitTokens::identifier))	return parseSymbol<T>();
		else if (matchIf(NativeJitTokens::literal))	return parseLiteral<T>();
	}

	template <typename T> NativeJIT::Node<T>& getNodeForLine(NamedNode* r)
	{
		if (NativeJIT::Node<T>* n = dynamic_cast<NativeJIT::Node<T>*>(r->node))
			return *n;
		else
			location.throwError("Type mismatch for Identifier " + currentValue.toString());

		return getEmptyNode<T>();
	}

	template <typename T> NativeJIT::Node<T>& parseSymbol()
	{
		Identifier symbolId = Identifier(currentValue);

		
        if(symbolId == Identifier("assertEqual"))               return parseAssertion<T>();
		else if (info.parameterNames.indexOf(symbolId) != -1)	return parseParameterReference<T>(symbolId);
		else if (auto r = getLine(symbolId))					return getNodeForLine<T>(r);
        
#if INCLUDE_COMPLEX_TEMPLATES
		else if (auto gn = getGlobalNode(symbolId))				return gn->template getLastNode<T>();
		else if (auto g = scope->getGlobal(symbolId))			return getGlobalReference<T>(symbolId);
#endif
		else if (auto b = scope->getExposedFunction(symbolId))	return parseFunctionCall<T>(b);
		else if (auto cf = scope->getCompiledBaseFunction(symbolId)) return parseFunctionCall<T>(cf);
		else
		{
			location.throwError("Unknown identifier " + symbolId.toString());
		}

		return getEmptyNode<T>();
	}

    template <typename T> NativeJIT::Node<T>& parseAssertion()
    {
        match(NativeJitTokens::openParen);
        
        TypeInfo leftType = peekFirstType();
        
        if(NativeJITTypeHelpers::matchesType<int>(leftType))
        {
            auto& l = parseSum<int>();
            match(NativeJitTokens::comma);
            
            TypeInfo rightType = peekFirstType();
            
            if(NativeJITTypeHelpers::matchesType<int>(rightType))
            {
                auto& r = parseSum<int>();
                match(NativeJitTokens::closeParen);
                
                auto& a1 = expr.Immediate(HelperFunctions::assertEqual<T, int, int>);
                
                auto& a2 = expr.Call(a1, l, r);
                
                return a2;
            }
        }
        
        return getEmptyNode<T>();
    }

	template <typename T> NativeJIT::Node<T>& parseParameterReference(const Identifier& id)
	{
		//static_assert(sizeof ...(Parameters) > 2, "Incorrect parameter amount");

		const int pIndex = info.parameterNames.indexOf(id);

		NativeJIT::Node<T>* r = nullptr;

		switch (pIndex)
		{
		case 0:
		{
			auto& e = expr.GetP1();

			r = dynamic_cast<NativeJIT::Node<T>*>(&e); break;
		}
		case 1:
		{
			auto& e = expr.GetP2();

			r = dynamic_cast<NativeJIT::Node<T>*>(&e); break;
		}
		
		}

		if (r != nullptr)
		{
			return *r;
		}
		else
		{
			location.throwError("Parameter reference type mismatch");
			
		}

		return getEmptyNode<T>();
	}

#if INCLUDE_COMPLEX_TEMPLATES
	template <typename T> NativeJIT::Node<T>& getGlobalReference(const Identifier& id)
	{
		auto existingNode = getGlobalNode(id);

		const bool globalWasChanged = existingNode != nullptr;

		if (globalWasChanged)
		{
			return existingNode->template getLastNode<T>();
		}
		else
		{
			auto r = scope->getGlobal(id);

			if (!NativeJITTypeHelpers::matchesType<T>(r->type))
			{
				location.throwError(NativeJITTypeHelpers::getTypeMismatchErrorMessage<T>(r->type));
			}

			auto& e1 = expr.Immediate(GlobalBase::get<T>);
			auto& g = expr.Immediate(r);

			return expr.Call(e1, g);
		}
		
	}
#endif

	template <typename T> NativeJIT::Node<T>& parseFunctionCall(BaseFunction* b)
	{
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
			else
			{
				location.throwError("No type deducted");
			}
		}
		case 2:
		{
			if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[0]))
			{
				if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, float, float>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, float, int>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, float, double>(b, parameterNodes[0], parameterNodes[1]);
				else
				{
					location.throwError("No type deducted");
				}
			}
			else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[0]))
			{
				if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, int, float>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, int, int>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, int, double>(b, parameterNodes[0], parameterNodes[1]);
				else
				{
					location.throwError("No type deducted");
				}
			}
			else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[0]))
			{
				if (NativeJITTypeHelpers::matchesType<float>(parameterTypes[1])) return parseFunctionParameterList<T, double, float>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<int>(parameterTypes[1])) return parseFunctionParameterList<T, double, int>(b, parameterNodes[0], parameterNodes[1]);
				else if (NativeJITTypeHelpers::matchesType<double>(parameterTypes[1])) return parseFunctionParameterList<T, double, double>(b, parameterNodes[0], parameterNodes[1]);
				else
				{
					location.throwError("No type deducted");
				}
			}
		}
		default:
		{
			location.throwError("Function parsing error");
		}	
		}

		return getEmptyNode<T>();
	}

	template <typename R2> NativeJIT::Node<R2>& parseFunctionParameterList(BaseFunction* b)
	{
        typedef R2(*f_p)();
		
		auto function = (f_p)(b->func);

		if (function != nullptr)
		{
			auto& e2 = expr.Immediate(function);
			auto& e3 = expr.Call(e2);
			match(NativeJitTokens::closeParen);
			return e3;
		}
		else
		{
			location.throwError("Function type mismatch");
		}


		return getEmptyNode<R2>();
	}

	template <typename R2, typename ParamType> NativeJIT::Node<R2>& parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1)
	{
		checkParameterType<ParamType>(b, 0);

		typedef R2 (*f_p)(ParamType);
		auto function = (f_p)(b->func);

		if (function != nullptr)
		{
			auto& e1 = expr.Immediate(function);

			auto e2 = dynamic_cast<NativeJIT::Node<ParamType>*>(param1);

			if (e2 != nullptr)
			{
				auto& e3 = expr.template Call<R2, ParamType>(e1, *e2);
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


		return getEmptyNode<R2>();
	}

	template <typename ParamType> void checkParameterType(BaseFunction* b, int parameterIndex)
	{
		if (b->getTypeForParameter(parameterIndex) != typeid(ParamType))
		{
			location.throwError("Parameter " + String(parameterIndex+1) + " type mismatch: " + NativeJITTypeHelpers::getTypeName<ParamType>() + ", Expected: " + b->getTypeForParameter(parameterIndex).name());
		}
	}

	template <typename R2, typename ParamType1, typename ParamType2> NativeJIT::Node<R2>& parseFunctionParameterList(BaseFunction* b, NativeJIT::NodeBase* param1, NativeJIT::NodeBase* param2)
	{
		checkParameterType<ParamType1>(b, 0);
		checkParameterType<ParamType2>(b, 1);

		typedef R2(*f_p)(ParamType1, ParamType2);
        
		auto function = (f_p)(b->func);

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


		return getEmptyNode<R2>();
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

	typedef R (*FuncPointer)(Parameters...);

	FuncPointer compileFunction()
	{
		if (returnStatement == nullptr)
		{
			location.throwError("No return value specified");
			return nullptr;
		}

		finalizeReturnStatement();

		
		
		return expr.Compile(*returnStatement);
	}


private:

	OwnedArray<NamedNode> lines;

	NativeJIT::Node<R>* returnStatement = nullptr;

	Identifier lastParsedLine;

};



#include "GlobalParser.h"

#include "Parser.cpp"


#endif  // PARSER_H_INCLUDED
