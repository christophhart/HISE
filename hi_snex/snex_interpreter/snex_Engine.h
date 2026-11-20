/* ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace interpreter {

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
 *  1. Compilation boundary and validation
 *  --------------------------------------
 *  The existing compiler produces the AST and instantiated ComplexType registry;
 *  ExpressionTreeBuilder creates an Engine returned through JitObject. The
 *  SnexSession selects the backend per compilation; validate the opt-in for
 *  live nodes before production use. Reject unsupported AST
 *  nodes, missing children, unresolved types and symbols, and unsupported native
 *  signatures with attributed builder errors before running initializers.
 *  Review compiler passes that execute native root constructors before claiming
 *  complete crash isolation.
 *
 *  2. Complete executable AST coverage
 *  -----------------------------------
 *  The prototype already lowers scalar literals, references, arithmetic, casts,
 *  assignments, returns, and function bodies. Add the remaining ValueTree types:
 *  locals and nested scopes, control flow, calls, member/index expressions,
 *  complex definitions, and compound operators. Resolve member and slot offsets
 *  once while building nodes; never parse AST strings during execution. Retain
 *  Line and charIndex; map preprocessed offsets back to editor source locations.
 *
 *  3. Runtime nodes, values, and control flow
 *  ------------------------------------------
 *  Add an Expression::perform() for discarded values and complete return, break,
 *  and continue propagation. Keep executable nodes immutable: use the current
 *  reusable per-function buffer with an active-call guard to reject reentry.
 *  If recursion becomes necessary, move call state into separate frames. Keep
 *  lvalue addresses distinct from temporary VariableStorage results.
 *  Audio-thread hot-path performance is critical even though interpretation is
 *  slower than JIT code. Resolve IDs, types, and slot offsets once during
 *  lowering; per-sample execution must not repeat symbol lookups, parse strings,
 *  allocate memory, or perform other avoidable work in DSP loops. Select typed
 *  operation function pointers while constructing each node (as BinaryOp does)
 *  so the per-sample path does not branch on operation kinds or value types.
 *
 *  4. Complex storage and runtime errors
 *  -------------------------------------
 *  Scope already owns aligned buffers and resolved scalar offsets. Add nested
 *  scopes, shadowing, and complex member/index addresses with checked bounds.
 *  Represent complex references as addresses of caller-owned objects, not
 *  by-value object slots; keep pointer-slot and pointee addresses distinct.
 *  Handle block/dyn views, complex initialization and destruction, and constness
 *  without treating VariableStorage as the native object layout. Host-owned
 *  pointers require separate validation and cannot be assumed crash-proof.
 *  Report runtime failures with AST locations through UriErrorLogger; defer UI
 *  notifications off the audio thread without allocation in the hot path.
 *
 *  5. Calls and host integration
 *  -----------------------------
 *  FunctionData already uses stable per-function bindings and setup-time typed
 *  trampolines for zero to two scalar/pointer arguments. Add internal SNEX calls
 *  without a native trampoline. Audit callUnchecked/callVoidUnchecked callers:
 *  they omit FunctionData.object and cannot call a bound trampoline directly.
 *  Support the required CallbackCollection and ScriptnodeCallbacks signatures,
 *  including block views and complex references. Wrap PrepareSpecs copies before
 *  passing them as references. Finish main-object access, layout snapshots, and
 *  dumpTable; define how runtime errors cross native callback boundaries.
 *
 *  6. Scope tree and debugger
 *  -------------------------
 *  Project functions, definitions, parameters, and nested scopes from the AST
 *  into a UriLocationTree with source ranges and categories. Attach typeInfo
 *  to definition nodes: strings for native types and a var holding a ref-counted
 *  jit::ComplexType for complex types. Some HiseScript consumers only recognize
 *  strings or LightweightDiagnostics, so provide a SNEX-aware display adapter.
 *  Use the shared UriBreakpoint::Owner interface for attachment, resume, and
 *  step actions. Study HiseJavascriptEngine::BlockStatement::BreakpointHelpers
 *  for stack unwinding and instruction-pointer movement, but validate SNEX
 *  scope lifetimes before adapting its control-flow behavior. Keep SNEX-specific
 *  breakpoint state and active function scopes instead of JavaScript's
 *  DynamicObject scopes. Resolve stops to stable
 *  executable statement positions. Implement step-in/over/out against frame
 *  depth and block positions. Restrict instruction-pointer moves to validated
 *  targets in the current function: entering a block with uninitialized locals
 *  or crossing an incompatible control-flow boundary must be rejected.
 *  At a confirmed pause, snapshot live VariableStorage values into var scalars,
 *  DynamicObjects for structs, and Array<var> for span/dyn values. Use TypeInfo
 *  and layouts to interpret memory. Do not dereference untrusted pointers merely
 *  to populate the UI. Materialize snapshots only while execution is halted;
 *  the interpreter itself never stores its values as var. Provide a paused-only
 *  REPL and value readout over typed scopes, starting with read-only inspection.
 *  Provide an appropriate non-audio-thread suspension path rather than reusing
 *  JavascriptProcessor's
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
 *  Track behavioral parity by test category: scalar control flow, complex
 *  storage, host callbacks, debugger snapshots, and validated pointer moves.
 *  The maintainer
 *  builds and runs tests; do not build or run HISE from this repository.
 */

struct Engine: public snex::jit::FunctionCollectionBase
{
	struct CallError
	{
		UriError e;
		String callstackId;
	};

	// Base interfaces ==================================================================================

	// Forward declarations ===============================================================================

	Engine(jit::GlobalScope& scope, Statement::Ptr rootTree);
	~Engine();

	// Function methods

	Array<jit::NamespacedIdentifier> getFunctionIds() const override;

	jit::FunctionData getFunction(const jit::NamespacedIdentifier& functionId) override;

	// Data methods

	Array<jit::Symbol> getAllVariables() const override;
	VariableStorage getVariable(const Identifier& id) const override;

	size_t getMainObjectSize() const override;
	void* getVariablePtr(const Identifier& id) const override;
	juce::String dumpTable() override;
	ValueTree getDataLayout(int dataIndex) const override;

	jit::GlobalScope* getGlobalScope() { return &globalScope; }

private:

	friend class snex::interpreter::ExpressionTreeBuilder;

	struct Helpers;
	struct EngineScope;

	struct InterpretedFunction
	{
		String getSigString() const;

		static void* getFromFunctionMap(const String& sig);

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
		static void writeArguments(InterpretedFunction& data, std::index_sequence<Indices...>, Ts... args)
		{
			(ComplexType::writeNativeMemberType(data.parameterOffsets[Indices], 0, VariableStorage(args)), ...);
		}

		static VariableStorage callDynamic(void* obj, VariableStorage* args, int numArgs);;

		template <typename RV, typename... Ts> static RV call(void* obj, Ts... args)
		{
			auto data = static_cast<InterpretedFunction*>(obj);

			try
			{
				if (data == nullptr || data->active || data->parameterOffsets.size() != sizeof...(Ts))
				{
					jassertfalse;
					if constexpr (std::is_void_v<RV>)
						return;
					else
						return RV();
				}

				ScopedValueSetter<bool> svs(data->active, true);

				data->functionScope->clear();
				writeArguments(*data, std::index_sequence_for<Ts...>{}, args...);

				VariableStorage rv;
				data->functionStatement->perform(data->functionScope.get(), &rv);

				if constexpr (!std::is_void_v<RV>)
				{
					static constexpr Types::ID RTypeId = Types::getTypeFromTypeId<RV>();
					return rv.toType<RTypeId>();
				}
			}
			catch (UriError& e)
			{
				CallError ne;
				ne.callstackId = data->functionData.getSignature();
				ne.e = e;
				throw ne;
			}
		}

		jit::FunctionData functionData;
		Scope::Ptr functionScope;
		Statement::Ptr functionStatement;
		std::vector<void*> parameterOffsets;
		bool addThisPointer = false;
		bool active = false;
	};

	struct InterpretedClass
	{
		Scope::Ptr classScope;
		jit::ComplexType::Ptr classType;

		jit::FunctionData getFunction(const NamespacedIdentifier& id)
		{
			for (auto m : members)
			{
				if (m->functionData.id == id)
					return m->functionData;
			}

			for (auto c : subclasses)
			{
				auto fd = c->getFunction(id);

				if (fd.id != NamespacedIdentifier())
					return fd;
			}

			return {};
		}

		Statement::List initialisers;
		OwnedArray<InterpretedFunction> members;
		OwnedArray<InterpretedClass> subclasses;
	};

	InterpretedFunction* createInterpretedFunction(Statement::Ptr f, Scope* s, bool addThisPointer);
	InterpretedClass* createInterpretedClass(Statement::Ptr c, Scope* s);

	DefinitionBase* createScopeInitialiser(Statement::Ptr c, Scope* s);

	OwnedArray<InterpretedFunction> rootFunctions;
	OwnedArray<InterpretedClass> classes;

	Statement::Ptr rootTree;
	Scope::Ptr rootScope;

	jit::GlobalScope& globalScope;
	std::unique_ptr<Scope> engineScope;
};

} // namespace interpreter
} // namespace snex
