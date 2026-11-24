namespace snex {
namespace interpreter {
namespace nodes {

struct FunctionMap
{
	static void* getFromFunctionMap(const String& sig)
	{
		static const std::map<String, void*> functionMap = []
			{
				std::map<String, void*> functions;
				addReturnSignatures<void>(functions, "v");
				addReturnSignatures<int>(functions, "i");
				addReturnSignatures<float>(functions, "f");
				addReturnSignatures<double>(functions, "d");
				addReturnSignatures<void*>(functions, "p");
				return functions;
			}();

		auto match = functionMap.find(sig);
		return match != functionMap.end() ? match->second : nullptr;
	}

private:

	template <typename RV, typename... Ts> static RV call(void* obj, Ts... args)
	{
		auto data = static_cast<nodes::Function*>(obj);

		try
		{
			if (data == nullptr || data->active || data->parameters.size() != sizeof...(Ts))
			{
				if (data != nullptr)
				{
					if (data->active)
						data->throwRuntimeError("reentrant");

					if (data->parameters.size() != sizeof...(Ts))
						data->throwRuntimeError("parameter amount mismatch");
				}

				jassertfalse;
				if constexpr (std::is_void_v<RV>)
					return;
				else
					return RV();
			}

			ScopedValueSetter<bool> svs(data->active, true);

			data->statementScope->clear();
			writeArguments(*data, std::index_sequence_for<Ts...>{}, args...);

			VariableStorage rv;
			data->perform(data->statementScope.get(), &rv);

			if constexpr (!std::is_void_v<RV>)
			{
				if constexpr (std::is_same<RV, void*>())
				{
					if (data->convertToBlock)
					{
						auto ptr = reinterpret_cast<block*>(rv.getDataPointer());
						auto b1 = *ptr;
						return reinterpret_cast<void*>(ptr);
					}
				}

				static constexpr Types::ID RTypeId = Types::getTypeFromTypeId<RV>();
				return rv.toType<RTypeId>();

			}
		}
		catch (UriError& e)
		{
			Engine::CallError ne;
			ne.callstackId = data->fData.getSignature();
			ne.e = e;
			throw ne;
		}
	}

	template <typename RV, typename T> static void addTwoArgumentSignatures(
		std::map<String, void*>& functionMap, const String& firstArgumentSignature)
	{
		functionMap[firstArgumentSignature + "i"] = (void*)call<RV, T, int>;
		functionMap[firstArgumentSignature + "f"] = (void*)call<RV, T, float>;
		functionMap[firstArgumentSignature + "d"] = (void*)call<RV, T, double>;
		functionMap[firstArgumentSignature + "p"] = (void*)call<RV, T, void*>;
	}

	template <typename RV> static void addReturnSignatures(
		std::map<String, void*>& functionMap, const String& returnSignature)
	{
		functionMap[returnSignature] = (void*)call<RV>;
		functionMap[returnSignature + "i"] = (void*)call<RV, int>;
		functionMap[returnSignature + "f"] = (void*)call<RV, float>;
		functionMap[returnSignature + "d"] = (void*)call<RV, double>;
		functionMap[returnSignature + "p"] = (void*)call<RV, void*>;

		addTwoArgumentSignatures<RV, int>(functionMap, returnSignature + "i");
		addTwoArgumentSignatures<RV, float>(functionMap, returnSignature + "f");
		addTwoArgumentSignatures<RV, double>(functionMap, returnSignature + "d");
		addTwoArgumentSignatures<RV, void*>(functionMap, returnSignature + "p");
	}

	template <size_t... Indices, typename... Ts>
	static void writeArguments(nodes::Function& data, std::index_sequence<Indices...>, Ts... args)
	{
		(data.parameterBinders[Indices](data.parameters[Indices], VariableStorage(args),
			data.parameterByteCounts[Indices]), ...);
	}
};

struct Function : public Statement,
				  public DefinitionBase
{
	SNEX_INTERPRETER_NODE(Function, WantsNode::Yes);

	Function(Statement::Factory& f, const ValueTree& v) :
		Statement(f, v)
	{
		body = childStatements[0];
		fData = Helpers::getFunctionData(f, v);

		TypeParserBase parser(v[AstIds::Signature].toString(), true, f.typeLibrary);
		parser.getTypeInfo();
		lexicalScopeId = parser.parseNamespacedIdentifier();

		for (auto& a : fData.args)
			a.id = lexicalScopeId.getChildId(a.id.id);

		jassert(body != nullptr);
	};

	ResultCode perform(Scope* s, VariableStorage* returnValue) override
	{
		return body->perform(s, returnValue);
	};

	Scope::Ptr createScope(Scope* parent)
	{
		return new Scope(Scope::ScopeType::Function, parent, parent->engine, lexicalScopeId);
	}

	Result allocateInScope(Scope* scopeToUse) override
	{
		setAllocated();

		jassert(scopeToUse->scopeType == Scope::ScopeType::Function);
		jassert(scopeToUse->id == lexicalScopeId);

		auto addThisPointer = scopeToUse->parent != scopeToUse->getRootScope() && !fData.returnType.isStatic();

		auto thisId = addThisPointer ? lexicalScopeId.getChildId("this") : NamespacedIdentifier();

		if (thisId.isValid())
		{
			// Keep this first in the function scope: ThisPointer reads its object pointer from offset zero.
			jit::Symbol thisObj({ thisId, TypeInfo(Types::ID::Pointer) });
			scopeToUse->addSymbol(thisObj);
		}

		for (const auto& a : fData.args)
		{
			auto result = scopeToUse->addSymbol(a);
			jassert(result.wasOk());
		}

		return Result::ok();
	}

	void resolveDataPointers(Scope* scopeToUse)
	{
		jassert(scopeToUse->scopeType == Scope::ScopeType::Function);
		jassert(scopeToUse->id == lexicalScopeId);

		auto validParameters = true;
		auto addThisPointer = scopeToUse->parent != scopeToUse->getRootScope() && !fData.returnType.isStatic();
		auto thisId = addThisPointer ? lexicalScopeId.getChildId("this") : NamespacedIdentifier();

		if (addThisPointer)
		{
			parameters.push_back(scopeToUse->resolveDataPointer(thisId));
			parameterBinders.push_back(writeParameter);
			parameterByteCounts.push_back(0);
		}

		for (const auto& a : fData.args)
		{
			auto ptr = scopeToUse->resolveDataPointer(a.id);
			validParameters &= (bool)ptr;
			parameters.push_back(ptr);

			const bool copyComplex = a.typeInfo.isComplexType() && !a.typeInfo.isRef();
			parameterBinders.push_back(a.typeInfo.isRef() ? bindReference :
				copyComplex ? copyComplexParameter : writeParameter);
			parameterByteCounts.push_back(copyComplex ? a.typeInfo.getRequiredByteSize() : 0);
		}

		if (validParameters)
		{
			fData.object = this;
			fData.function = FunctionMap::getFromFunctionMap(OpFunctions::getSigString(fData, addThisPointer));

			if (fData.function == nullptr)
			{
				fData.function = (void*)callDynamicStatic;
			}
		}

		if (fData.returnType.toString() == "block&")
			convertToBlock = true;

		jassert(fData.function != nullptr);
	}

	void annotateUriLocationTree(UriLocationTree& t) override
	{
		for (auto a : fData.args)
		{
			auto argDef = new UriLocationTree(&t);
			argDef->fillLocation(t);
			argDef->category = "parameter";
			argDef->id = a.id.toString();
			Helpers::annotateType(*argDef, a.typeInfo);
		}
	}

	VariableStorage callDynamic(VariableStorage* args, int numArgs)
	{
		try
		{
			jassert(numArgs == parameters.size());

			ScopedValueSetter<bool> svs(active, true);

			statementScope->clear();

			for (int i = 0; i < numArgs; i++)
				parameterBinders[i](parameters[i], args[i], parameterByteCounts[i]);

			VariableStorage rv;
			perform(statementScope, &rv);

			return rv;
		}
		catch (UriError& e)
		{
			Engine::CallError ne;
			ne.callstackId = fData.getSignature();
			ne.e = e;
			throw ne;
		}
	}

	static VariableStorage callDynamicStatic(void* obj, VariableStorage* args, int numArgs)
	{
		auto data = static_cast<Function*>(obj);
		return data->callDynamic(args, numArgs);

	}

	static void bindReference(Scope::ResolvedReference& target, const VariableStorage& value, size_t)
	{
		jassert(value.getType() == Types::ID::Pointer);
		target.assign({ value.getDataPointer(), 0 });
	}

	static void copyComplexParameter(Scope::ResolvedReference& target, const VariableStorage& value, size_t numBytes)
	{
		memcpy(target.getDataPointer(), value.getDataPointer(), numBytes);
	}

	static void writeParameter(Scope::ResolvedReference& target, const VariableStorage& value, size_t)
	{
		target.write(value);
	}

	using ParameterBinder = void(*)(Scope::ResolvedReference&, const VariableStorage&, size_t);
	std::vector<Scope::ResolvedReference> parameters;
	std::vector<ParameterBinder> parameterBinders;
	std::vector<size_t> parameterByteCounts;

	jit::FunctionData fData;
	NamespacedIdentifier lexicalScopeId;
	Ptr body;

	bool active = false;
	bool convertToBlock = false;
};


} // namespace nodes
} // namespace interpreter
} // namespace snex
