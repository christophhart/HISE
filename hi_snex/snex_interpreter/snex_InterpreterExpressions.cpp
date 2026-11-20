namespace snex {
namespace interpreter {
namespace nodes {



struct FunctionCall : public Expression
{
	SNEX_INTERPRETER_NODE(FunctionCall, WantsNode::No);

	FunctionCall(Factory& f, const ValueTree& v) :
		Expression(f, v),
		fData(Helpers::getFunctionData(f, v)),
		callType(v[AstIds::CallType].toString())
	{
		int offset = 0;

		if (callType == Identifier("MemberFunction"))
		{
			objectType = Helpers::getType(f, v, AstIds::ObjectType).getComplexType();
			
			jassert(objectType != nullptr);
			args.add(VariableStorage());
			argExpressions.push_back(Helpers::getChildExpression(this, 0));
			offset = 1;
		}

		for (int i = 0; i < fData.args.size(); i++)
		{
			args.add(VariableStorage());
			argExpressions.push_back(Helpers::getChildExpression(this, i + offset));
		}

		std::map<String, ResolvedPtr> map;

		map["v"] = callStatic<void, 0>;
		map["i"] = callStatic<int, 0>;
		map["f"] = callStatic<float, 0>;
		map["d"] = callStatic<double, 0>;
		map["p"] = callStatic<void*, 0>;

		map["vi"] = callStatic<void,   1, int>;
		map["ii"] = callStatic<int,    1, int>;
		map["fi"] = callStatic<float,  1, int>;
		map["di"] = callStatic<double, 1, int>;
		map["pi"] = callStatic<void*,  1, int>;
		map["vf"] = callStatic<void,   1, float>;
		map["if"] = callStatic<int,    1, float>;
		map["ff"] = callStatic<float,  1, float>;
		map["df"] = callStatic<double, 1, float>;
		map["pf"] = callStatic<void*,  1, float>;
		map["vd"] = callStatic<void,   1, double>;
		map["id"] = callStatic<int,    1, double>;
		map["fd"] = callStatic<float,  1, double>;
		map["dd"] = callStatic<double, 1, double>;
		map["pd"] = callStatic<void*,  1, double>;
		map["vp"] = callStatic<void,   1, void*>;
		map["ip"] = callStatic<int,    1, void*>;
		map["fp"] = callStatic<float,  1, void*>;
		map["dp"] = callStatic<double, 1, void*>;
		map["pp"] = callStatic<void*,  1, void*>;

		auto sig = OpFunctions::getSigString(fData, objectType != nullptr);

		if (map.find(sig) != map.end())
			functionPointer = map.at(sig);

		jassert(args.size() == childStatements.size());
	};

	jit::TypeInfo getTypeInfo() const override { return fData.returnType; }

	
	void resolveDataPointers(Scope* s) override
	{
		if (callType == Identifier("ApiFunction"))
		{
			jit::GlobalScope* gs = s->engine->getGlobalScope();

			auto thisLabel = fData.toLabel();

			for (const auto& p : gs->getMap())
			{
				if (p.label == thisLabel)
				{
					fData.function = p.function;
					break;
				}
			}
		}

		if (callType == Identifier("MemberFunction"))
		{
			fData = objectType->getNonOverloadedFunction(fData.id.id);

			if (!fData.isResolved())
				fData = s->engine->getFunction(fData.id);	
		}

		if (callType == Identifier("RootFunction"))
		{
			fData = s->engine->getFunction(fData.id);
		}

		jassert(fData.isResolved());
	};

	template <typename RT, typename...Ts> static RT callStaticInternal(void* obj, Ts... args)
	{
		auto typed = static_cast<FunctionCall*>(obj);

		if (typed->fData.object)
			return typed->fData.callUncheckedWithObject<RT, Ts...>(args...);
		else
			return typed->fData.callUnchecked<RT, Ts...>(args...);
	}

	template <typename RT, int NumArgs, typename T1=void, typename T2=void> static VariableStorage callStatic(void* obj, VariableStorage* args)
	{
		static constexpr Types::ID T1Type = Types::Helpers::getTypeFromTypeId<T1>();
		static constexpr Types::ID T2Type = Types::Helpers::getTypeFromTypeId<T2>();

		if constexpr (std::is_same<RT, void>())
		{
			if constexpr (NumArgs == 0)
				callStaticInternal<void>(obj);
			if constexpr (NumArgs == 1)
				callStaticInternal<void>(obj, args[0].template toType<T1Type>());
			if constexpr (NumArgs == 2)
				callStaticInternal<void>(obj, args[0].template toType<T1Type>(), args[1].template toType<T2Type>());
		}
		else
		{
			if constexpr (NumArgs == 0)
				return VariableStorage(callStaticInternal<RT>(obj));
			if constexpr (NumArgs == 1)
				return VariableStorage(callStaticInternal<RT>(obj, args[0].template toType<T1Type>()));
			if constexpr (NumArgs == 2)
				return VariableStorage(callStaticInternal<RT>(obj, args[0].template toType<T1Type>(), args[1].template toType<T2Type>()));
		}

		return VariableStorage();
	}

	typedef VariableStorage(*ResolvedPtr)(void*, VariableStorage*);

	VariableStorage getResult(Scope* s) override
	{	
		for (int i = 0; i < args.size(); i++)
			args.set(i, argExpressions[i]->getResult(s));
		
		if (functionPointer != nullptr)
			return functionPointer(this, args.getRawDataPointer());

		if (fData.returnType.getType() == Types::ID::Void)
		{
			fData.callVoidDynamic(args.getRawDataPointer(), args.size());
			return VariableStorage();
		}
		else
		{
			return fData.callDynamic(args.getRawDataPointer(), args.size());
		}
	}

	ResolvedPtr functionPointer = nullptr;
	Identifier callType;

	ComplexType::Ptr objectType;

	FunctionData fData;

	Array<VariableStorage> args;

	ExprPtr thisObject;
	std::vector<ExprPtr> argExpressions;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
