/** ===========================================================================
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

namespace snex {
namespace interpreter {

Statement::Factory::Factory()
{
	registerType<nodes::SyntaxTree>();
	registerType<nodes::InternalProperty>();
	registerType<nodes::Function>();
	registerType<nodes::ReturnStatement>();
	registerType<nodes::Immediate>();
	registerType<nodes::BinaryOp>();
	registerType<nodes::Increment>();
	registerType<nodes::Comparison>();
	registerType<nodes::Cast>();
	registerType<nodes::VariableReference>();
	registerType<nodes::Assignment>();
	registerType<nodes::ComplexTypeDefinition>();
	registerType<nodes::Subscript>();
	registerType<nodes::FunctionCall>();
	registerType<nodes::TemplatedFunction>();
	registerType<nodes::TemplateDefinition>();
	registerType<nodes::ClassStatement>();
	registerType<nodes::Dot>();
	registerType<nodes::MemoryReference>();
	registerType<nodes::ThisPointer>();
	registerType<nodes::PointerAccess>();
	registerType<nodes::StatementBlock>();
	registerType<nodes::TernaryOp>();
	registerType<nodes::IfStatement>();
	registerType<nodes::ControlFlowStatement>();
	registerType<nodes::Loop>();
	registerType<nodes::WhileLoop>();

	registerType<nodes::InlinedFunction>();
	registerType<nodes::InlinedArgument>();
	registerType<nodes::InlinedReturnValue>();
	registerType<nodes::InlinedParameter>();

	registerType<nodes::AnonymousBlock>();
}

ExpressionTreeBuilder::ExpressionTreeBuilder(UriLocationBuilder::Ptr locations, const ValueTree& ast_, const ReferenceCountedArray<jit::ComplexType>& typeLibrary_):
  r(Result::ok()),
  astTree(ast_),
  typeLibrary(typeLibrary_),
  locationBuilder(locations)
{}

jit::FunctionCollectionBase* ExpressionTreeBuilder::createEngine(jit::GlobalScope & globalScope)
{
	Statement::Factory f;
	
	f.locationBuilder = locationBuilder;
	f.typeLibrary = typeLibrary;

#if !TEST_RUN
	DBG(astTree.createXml()->createDocument(""));
#endif

	auto rootTree = f.create(astTree);
	auto locTree = createUriLocationTree(f, rootTree, nullptr);

	
	return new interpreter::Engine(globalScope, rootTree);
}

UriLocationTree::Ptr ExpressionTreeBuilder::createUriLocationTree(const Statement::Factory& f, Statement::Ptr ptr, UriLocationTree* parent)
{
	if (ptr->shouldCreateNode() == Statement::WantsNode::Yes)
	{
		UriLocationTree::Ptr np = new UriLocationTree(parent);

		np->fillLocation(locationBuilder->createLocation((int)ptr->astNode[AstIds::charIndex]));
		np->category = ptr->astNode.getType();
		ptr->annotateUriLocationTree(*np);

		UriLocationTree::List children;

		for (auto c : ptr->childStatements)
		{
			children.add(createUriLocationTree(f, c, np.get()));
		}

		return np;
	}

	return nullptr;
}

} // namespace interpreter
} // namespace snex
