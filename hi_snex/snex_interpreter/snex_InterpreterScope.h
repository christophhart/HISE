#pragma once

namespace snex {
namespace interpreter {

struct Scope : public ReferenceCountedObject
{
	enum class ScopeType { Unknown, Root, Class, Function, Block, InlinedFunction };

	using Ptr = ReferenceCountedObjectPtr<Scope>;
	using List = ReferenceCountedArray<Scope>;
	using WeakPtr = WeakReference<Scope>;

	struct LValue
	{
		void* first = nullptr;
		size_t second = 0;

		VariableStorage readAsPointer() const
		{
			jassert(first != nullptr);
			auto ptr = reinterpret_cast<uint8*>(first) + second;
			return VariableStorage(reinterpret_cast<void*>(ptr));
		}
	};

	typedef VariableStorage(*ReadFunction)(LValue);
	typedef void(*WriteFunction)(LValue, const VariableStorage&);

	// internal structure for owned and allowcated memory
	struct Entry
	{
		jit::Symbol symbol;
		size_t offset = 0;
		size_t size = 0;
	};

	struct ScopeError
	{
		jit::Symbol location;
		LValue address;
		String errorMessage;

		String toString() const;
	};

	// a table of all references, both external and owned
	struct Reference
	{
		bool isExternal;
		Identifier id;
		jit::TypeInfo type;
		LValue address;
	};

	// A resolved reference that returns the current LValue address
	struct ResolvedReference
	{
		ResolvedReference() = default;
		
		operator bool() const 
		{ 
			return immediateLocation.first != nullptr ||
				   s != nullptr && referenceIndex != -1; 
		}

		LValue& getLValue();
		LValue getLValueCopy() const;
		void* getDataPointer() const;
		VariableStorage read() const;
		void write(const VariableStorage& v);

		void assign(const LValue& lValue);

		static bool isBlockType(const jit::TypeInfo& t);

		void throwNullptrAccess() const;

		// Convert a pointer to a block type
		static VariableStorage toBlock(const VariableStorage& pointer);

		Scope::Ptr s;
		int referenceIndex = -1;
		jit::TypeInfo type;
		LValue immediateLocation = { nullptr, 0 };
	};

	Scope(ScopeType st, Scope* parent_, Engine* e, const NamespacedIdentifier& id_);

	Result addSymbol(const jit::Symbol& symbol);

	ResolvedReference resolveDataPointer(const NamespacedIdentifier& id);

	Result finalise();

	void clear();
	void* getDataPointer() const { return dataStart; }
	size_t getDataSize() const { return totalSize; }

	const Entry* find(const Identifier& symbolId) const;
	
	//void* getAddress(const Identifier& symbolId, size_t bytesNeeded) const;
	Array<jit::Symbol> getSymbols() const;
	Scope* getChildScope(const jit::NamespacedIdentifier& scopeId);
	
	Scope* getParentScopeWithId(const jit::NamespacedIdentifier& scopeId);

	Scope* getParentScopeWithType(ScopeType sp)
	{
		if (scopeType == sp)
			return this;

		if(parent != nullptr)
			return parent->getParentScopeWithType(sp);

		return nullptr;
	}

	Scope* getRootScope()
	{
		if (parent == nullptr)
			return this;

		return parent->getRootScope();
	}

	List children;
	WeakPtr parent = nullptr;
	Engine* engine = nullptr;
	jit::NamespacedIdentifier id;

	HeapBlock<uint8> scopeData;
	void* thisPtr = nullptr;

	const ScopeType scopeType;

	std::map<Identifier, Entry> entries;
	uint8* dataStart = nullptr;
	size_t totalSize = 0;
	size_t maxAlignment = 1;

	std::vector<Reference> references;
	JUCE_DECLARE_WEAK_REFERENCEABLE(Scope);
};

} // namespace interpreter
} // namespace snex
