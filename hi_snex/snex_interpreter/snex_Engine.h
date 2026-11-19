#pragma once

namespace snex
{
namespace interpreter
{

/** Implementation plan for the opt-in SNEX interpreter.
 *
 *  Goal
 *  ----
 *  Execute the same validated SNEX programs as the MIR backend without emitting
 *  machine code. Preserve the JitObject / FunctionData entry points where their
 *  native signatures can be bridged safely. Build an inspectable execution state
 *  for source-level debugging. The interpreter owns SNEX memory operations, but
 *  calls into arbitrary native functions and host-owned memory are not crash-proof.
 *
 *  1. Compilation boundary and ownership
 *  ------------------------------------
 *  Keep preprocessing, parsing, type checking, and AST generation in the existing
 *  compiler. After a successful common compilation pass, select the interpreter
 *  instead of MIR and give ExpressionTreeBuilder the AST ValueTree, the complex
 *  type layouts, and any required GlobalScope services. Do not make the builder
 *  depend on MIR instructions or generated MIR text. Backend selection must be
 *  opt-in; enabling interpreter support must not select it for every compile.
 *  Return a JitObject holding an Engine via FunctionCollectionBase. Propagate
 *  builder failures as compile errors, without silently falling through to MIR.
 *  Keep the executable nodes, globals, type metadata, and native entry bindings
 *  alive in the Engine for at least as long as any returned FunctionData is used.
 *  Review the common compiler passes for work that still runs native code during
 *  compilation (for example, root construction) before claiming full safety.
 *
 *  2. Lower the AST once
 *  ---------------------
 *  ExpressionTreeBuilder validates node kinds and required properties, resolves
 *  types, symbols, functions, member offsets, and scope slot indices, and creates
 *  immutable executable Statement / Expression nodes. Reject unsupported nodes
 *  with an attributed error. Do not repeatedly parse ValueTree property strings
 *  in the execution loop. Retain source positions and the resolved TypeInfo on
 *  executable nodes. The serialized AST currently has Line and charIndex; use
 *  charIndex as the cheap source offset and defer line / column resolution.
 *  Remember that the AST offset is into the preprocessed program: provide a
 *  source mapping when editor text and preprocessed text differ. Resolve types
 *  during lowering rather than assuming that every AST node has a Type property.
 *
 *  3. Runtime nodes, values, and control flow
 *  ------------------------------------------
 *  Model execution after HiseJavascriptEngine: statements perform an action and
 *  propagate ok / return / break / continue; expressions evaluate to a
 *  VariableStorage. Provide a default Expression::perform() that discards its
 *  result. The node graph is shared and should not store mutable call positions
 *  or local values: recursion and nested calls need independent frames. Blocks
 *  own ordered statements; frames own the current block position and call stack.
 *  Evaluate lvalues separately from rvalues so assignments can write through a
 *  typed address rather than modifying a temporary VariableStorage copy.
 *
 *  4. Scopes and memory
 *  --------------------
 *  Use typed, indexed scope slots rather than a DynamicObject. The builder
 *  assigns slot indices and the runtime links function parameters, local block
 *  scopes, and the engine/global scope through Scope::parent. DataSource selects
 *  the intended storage category; specify nearest-scope/shadowing behavior for
 *  nested scopes of the same category. A matching scope with an invalid index
 *  must report an error, not accidentally resolve an outer slot. Use runtime
 *  frame sizes from compiled function metadata unless fixed sizes genuinely
 *  simplify a particular callback. Frames own their storage while suspended.
 *  Scope::resolve() returns the writable address of the SNEX value. Native
 *  VariableStorage slots expose their payload address; complex members use
 *  their containing object's address plus a validated data-layout offset.
 *  Distinguish the address of a pointer slot from the address it points to:
 *  VariableStorage::getDataPointer() returns the pointee for Pointer and is
 *  not a universal address-of-slot operation. Define block/dyn storage layouts
 *  explicitly rather than treating their tagged VariableStorage union as the
 *  native block struct. Keep TypeInfo, size, alignment, constness, and storage
 *  lifetime associated with each resolved address. Check bounds and alignment
 *  before dereferencing interpreter-owned storage; do not assume that a valid
 *  SNEX type proves a host-provided pointer is safe.
 *  For native scalars, copy values into slots. For a non-reference complex
 *  parameter, allocate correctly sized/aligned frame storage and copy its
 *  object representation, then use the address of that copy internally. For a
 *  reference, retain an address to the original object. Copying a block/dyn
 *  view copies its pointer and length, not its underlying sample buffer. Apply
 *  the same value-versus-reference distinction to assignments and returns.
 *  Define initialization and destruction for owned complex storage separately.
 *
 *  5. Functions and host entry points
 *  ----------------------------------
 *  Engine implements FunctionCollectionBase: expose function IDs and metadata,
 *  global variables and pointers, main-object size, layout snapshots, and a
 *  dump of live data. Keep callable FunctionData metadata/bindings separate
 *  from the executable function-body list. Populate FunctionData.function with
 *  a typed native trampoline and FunctionData.object with a stable per-function
 *  binding containing Engine and function index. One trampoline per supported
 *  native signature is enough; avoid one template instantiation per function
 *  index. Select the trampoline once when creating the FunctionData. Its exact
 *  C++ signature must match the caller ABI (including float/double, pointers,
 *  block-by-value callbacks, and returns). Inside it, construct a frame from
 *  the declared SNEX TypeInfo, execute the body, and convert the return value.
 *  Cover CallbackCollection signatures first, then ScriptnodeCallbacks
 *  (prepare, reset, process, frame, event, external data, and optional calls)
 *  as needed. Check all callers using callUnchecked/callVoidUnchecked: those
 *  methods do not prepend FunctionData.object. Route interpreter calls through
 *  an object-aware path; never put a context-requiring trampoline behind an
 *  unchecked call that omits its context. Keep bindings valid for copied
 *  FunctionData values and decide how errors cross native callback boundaries.
 *  Internal SNEX calls may invoke executable nodes directly without a native
 *  trampoline. External native calls need explicit safe adapters or a clearly
 *  documented escape from interpreter crash protection.
 *
 *  6. Scope tree and debugger
 *  -------------------------
 *  Project functions, definitions, parameters, and nested scopes from the AST
 *  into a UriLocationTree with source ranges and categories. Attach typeInfo
 *  to definition nodes: strings for native types and a var holding a ref-counted
 *  jit::ComplexType for complex types. Some HiseScript consumers only recognize
 *  strings or LightweightDiagnostics, so provide a SNEX-aware display adapter.
 *  Use the shared UriBreakpoint::Owner interface for attachment, resume, and
 *  step actions. Keep SNEX-specific breakpoint state and per-call frames rather
 *  than depending on JavaScript's DynamicObject scopes. Resolve stops to stable
 *  executable statement positions. Implement step-in/over/out against frame
 *  depth and block positions. Restrict instruction-pointer moves to validated
 *  targets in the current function: entering a block with uninitialized locals
 *  or crossing an incompatible control-flow boundary must be rejected.
 *  At a confirmed pause, snapshot live VariableStorage values into var scalars,
 *  DynamicObjects for structs, and Array<var> for span/dyn values. Use TypeInfo
 *  and layouts to interpret memory. Do not dereference untrusted pointers merely
 *  to populate the UI. Materialize snapshots only while execution is halted;
 *  the interpreter itself never stores its values as var. Provide an appropriate
 *  non-audio-thread suspension path rather than reusing JavascriptProcessor's
 *  thread-specific sleeper directly.
 *
 *  7. Verification milestones
 *  --------------------------
 *  Adapt JitFileTestCase to call either backend through the same metadata and
 *  expected-output checks. The test_files directory has scalar, control-flow,
 *  struct, span/dyn, template, event, polyphonic, and node/process-data cases.
 *  Keep expected compile errors on the common front end; assembly loop-count
 *  and optimization-specific assertions remain MIR-only. Extend the embedded
 *  C++ tests to exercise the typed entry bindings and inspect scoped locals.
 *  Implement in stages: scalar statements and calls; addressable scopes and
 *  complex values; host callbacks and buffers; debugger stepping and snapshots;
 *  finally validated instruction-pointer moves. Report unsupported constructs
 *  explicitly and track behavioral parity by test category. The maintainer
 *  builds and runs tests; do not build or run HISE from this repository.
 */
struct Engine: public snex::jit::FunctionCollectionBase
{
	// Base interfaces ==================================================================================

	struct Scope: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Scope>;
		using List = ReferenceCountedArray<Scope>;
		using WeakPtr = WeakReference<Scope>;

		Scope(Scope* parent_, Engine* e, const NamespacedIdentifier& id_):
		  parent(parent_),
		  engine(e),
		  id(id_)
		{
			if(parent != nullptr)
				parent->children.add(this);
		}

		void* getDataPointer() const
		{
			return reinterpret_cast<void*>(scopeData.get());
		}

		size_t getOffset(const Identifier& id) const
		{
			if (offsets.find(id) != offsets.end())
				return offsets.at(id);

			return 0;
		}

		Scope* getScope(const jit::NamespacedIdentifier& scopeId)
		{
			if (id == scopeId)
				return this;

			for (auto c : children)
			{
				if (auto match = c->getScope(scopeId))
					return match;
			}

			return nullptr;
		}

		List children;
		WeakPtr parent = nullptr;
		Engine* engine = nullptr;
		jit::NamespacedIdentifier id;

		HeapBlock<uint8> scopeData;
		std::map<Identifier, size_t> offsets;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Scope);
	};

	struct Statement: public ReferenceCountedObject
	{
		Statement(const ValueTree& astNode_) :
		  astNode(astNode_)
		{};

		using Ptr = ReferenceCountedObjectPtr<Statement>;
		using List = ReferenceCountedArray<Statement>;
		enum ResultCode { ok = 0, returnWasHit, breakWasHit, continueWasHit };

		virtual ~Statement() = default;
		virtual ResultCode perform(Scope* s, VariableStorage* returnValue) { return ResultCode::ok; };

		ValueTree astNode;
	};

	struct Expression : public Statement
	{
		using ExprPtr = ReferenceCountedObjectPtr<Expression>;
		using ExprList = ReferenceCountedArray<Expression>;

		Expression(const ValueTree& astNode) :
		  Statement(astNode)
		{};

		virtual jit::TypeInfo getTypeInfo() const = 0;
		virtual VariableStorage getResult(Scope* s) = 0;
		
		virtual void assign(Scope* s, const VariableStorage& v) {}
	};

	struct StatementCollection
	{
		template <typename StatementType = Statement> bool callRecursive(const ValueTree& v, const std::function<bool(StatementType*)>& f) const
		{
			if (auto s = get(v))
			{
				if (auto typed = dynamic_cast<StatementType*>(s.get()))
				{
					if (f(typed))
						return true;
				}

				for (auto c : v)
				{
					if (callRecursive(c, f))
						return true;
				}
			}

			return false;
		};

		Statement::Ptr get(const ValueTree& v) const
		{
			for (const auto& item : flatList)
			{
				if (item.first == v)
					return item.second;
			}

			return nullptr;
		}

		Statement::Ptr root;
		std::vector<std::pair<ValueTree, Statement::Ptr>> flatList;
	};

	// Forward declarations ===============================================================================

	Engine(jit::GlobalScope& scope, const StatementCollection& rootTree);
	~Engine();

	// Function methods

	Array<jit::NamespacedIdentifier> getFunctionIds() const override 
	{ 
		Array<jit::NamespacedIdentifier> ids;

		for (const auto& f : dispatchData)
		{
			ids.add(f.functionData.id);
		}
		
		return ids;
	}

	jit::FunctionData getFunction(const jit::NamespacedIdentifier& functionId) override;

	// Data methods

	Array<jit::Symbol> getAllVariables() const override;
	VariableStorage getVariable(const Identifier& id) const override;

	size_t getMainObjectSize() const override;
	void* getVariablePtr(const Identifier& id) const override;
	juce::String dumpTable() override;
	ValueTree getDataLayout(int dataIndex) const override;

private:

	friend class snex::interpreter::ExpressionTreeBuilder;

	struct StatementFactory
	{
		using CreateFunction = std::function<Statement::Ptr(StatementFactory& f, const ValueTree& v)>;

		StatementFactory();;

		template <typename T> void registerType()
		{
			auto id = T::getStatementType();
			createFunctions[id] = T::create;
		}

		Statement::Ptr create(const ValueTree& v);
		std::map<Identifier, CreateFunction> createFunctions;
		StatementCollection createdNodes;
		ReferenceCountedArray<jit::ComplexType> typeLibrary;
	};

	struct Helpers;

	struct SyntaxTree;
	struct Function;
	struct ReturnStatement;
	struct Immediate;
	struct BinaryOp;
	struct Cast;
	struct VariableReference;
	struct Assignment;

	struct EngineScope;

	template <int NumArgs> struct FunctionScope: public Scope
	{
		FunctionScope()
		{
			sourceType = DataSource::FunctionParameters;
		}

		void* resolve(int index) const override { return data[index].getDataPointer(); }

		VariableStorage data[NumArgs];
	};

	template <int FunctionIndex, typename RT, typename T1> static RT dispatch1(void* obj, T1 value)
	{
		auto typed = static_cast<Engine*>(obj);

		if (isPositiveAndBelow(FunctionIndex, typed->dispatchData.size()))
		{
			auto& d = typed->dispatchData[FunctionIndex];

			VariableStorage rv;
			ComplexType::writeNativeMemberType(d.functionScope->getDataPointer(), d.parameterOffsets[0], VariableStorage(value));
			typed->dispatchData[FunctionIndex].functionStatement->perform(d.functionScope.get(), &rv);

			static constexpr Types::ID RTypeId = Types::getTypeFromTypeId<RT>();
			return rv.toType<RTypeId>();
		}

		return RT();
	}

	struct DispatchData
	{
		jit::FunctionData functionData;
		Scope::Ptr functionScope;
		Statement::Ptr functionStatement;
		std::vector<size_t> parameterOffsets;
	};

	std::vector<DispatchData> dispatchData;

	Statement::Ptr rootTree;
	Scope::Ptr rootScope;

	jit::GlobalScope& globalScope;
	std::unique_ptr<Scope> engineScope;
};

} // namespace interpreter
} // namespace snex
