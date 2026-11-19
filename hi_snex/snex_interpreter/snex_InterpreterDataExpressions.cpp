namespace snex {
namespace interpreter {
namespace nodes {

struct Immediate : public Expression
{
	SNEX_INTERPRETER_NODE(Immediate, WantsNode::No);

	Immediate(Factory& f, const ValueTree& v) :
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

struct VariableReference : public Expression
{
	SNEX_INTERPRETER_NODE(VariableReference, WantsNode::No);

	VariableReference(Factory& f, const ValueTree& v) :
		Expression(v),
		symbol(Helpers::getSymbol(f, v))
	{
		switch (symbol.typeInfo.getType())
		{
		case Types::ID::Integer:
			r = readStatic<int>;
			w = writeStatic<int>;
			break;
		case Types::ID::Float:
			r = readStatic<float>;
			w = writeStatic<float>;
			break;
		case Types::ID::Double:
			r = readStatic<double>;
			w = writeStatic<double>;
			break;
		}
	};

	typedef VariableStorage(*ReadFunction)(void*, size_t);
	typedef void(*WriteFunction)(void*, size_t, const VariableStorage&);

	template <typename T> static VariableStorage readStatic(void* data, size_t offset)
	{
		auto ptr = reinterpret_cast<uint8*>(data) + offset;
		return VariableStorage(*reinterpret_cast<T*>(ptr));
	}

	template <typename T> static void writeStatic(void* data, size_t offset, const VariableStorage& value)
	{
		static constexpr auto TID = Types::Helpers::getTypeFromTypeId<T>();

		auto ptr = reinterpret_cast<uint8*>(data) + offset;

		T v = value.template toType<TID>();
		*reinterpret_cast<T*>(ptr) = v;
	}

	void assign(Scope* s, const VariableStorage& value) override
	{
		if (resolvedScope != nullptr && r != nullptr)
			w(resolvedScope->getDataPointer(), resolvedOffset, value);
	}

	VariableStorage getResult(Scope* s) override
	{
		if (resolvedScope != nullptr && r != nullptr)
			return r(resolvedScope->getDataPointer(), resolvedOffset);

		jassertfalse;
		return {};
	}

	TypeInfo getTypeInfo() const override
	{
		return symbol.typeInfo;
	}

	ReadFunction r = nullptr;
	WriteFunction w = nullptr;

	Scope::Ptr resolvedScope;
	size_t resolvedOffset = 0;
	jit::Symbol symbol;
};

} // namespace nodes
} // namespace interpreter
} // namespace snex
