namespace snex
{
namespace interpreter
{

struct Engine::Immediate : public Engine::Expression
{
	SNEX_INTERPRETER_NODE(Immediate);

	Immediate(StatementFactory& f, const ValueTree& v) :
		Expression(v),
		type(Helpers::getType(f, v)),
		value(type.getType(), v[AstIds::Value])
	{};

	VariableStorage getResult(Scope* s) override
	{
		return value;
	}

	TypeInfo getTypeInfo() const override
	{
		return type;
	}

	TypeInfo type;
	VariableStorage value;

};

struct Engine::VariableReference : public Engine::Expression
{
	SNEX_INTERPRETER_NODE(VariableReference);

	VariableReference(StatementFactory& f, const ValueTree& v) :
		Expression(v),
		symbol(Helpers::getSymbol(f, v))
	{
		switch (symbol.typeInfo.getType())
		{
		case Types::ID::Integer:
			r = readStatic<int>;
			break;
		case Types::ID::Float:
			r = readStatic<float>;
			break;
		case Types::ID::Double:
			r = readStatic<double>;
			break;
		}
	};

	typedef VariableStorage(*ReadFunction)(void*, size_t);

	template <typename T> static VariableStorage readStatic(void* data, size_t offset)
	{
		auto ptr = reinterpret_cast<uint8*>(data) + offset;
		return VariableStorage(*reinterpret_cast<T*>(ptr));
	}

	VariableStorage getResult(Scope* s) override
	{
		jassert(resolvedScope != nullptr);
		auto data = resolvedScope->getDataPointer();
		return r(data, resolvedOffset);
	}

	TypeInfo getTypeInfo() const override
	{
		return symbol.typeInfo;
	}

	ReadFunction r = nullptr;

	Scope::Ptr resolvedScope;
	size_t resolvedOffset = 0;
	jit::Symbol symbol;
};

} // namespace interpreter
} // namespace snex
