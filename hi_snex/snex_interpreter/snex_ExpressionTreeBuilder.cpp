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

namespace snex
{
namespace interpreter
{

Engine::StatementFactory::StatementFactory()
{
	registerType<Engine::SyntaxTree>();
	registerType<Engine::Function>();
	registerType<Engine::ReturnStatement>();
	registerType<Engine::Immediate>();
	registerType<Engine::BinaryOp>();
	registerType<Engine::Cast>();
	registerType<Engine::VariableReference>();
	registerType<Engine::Assignment>();
}

ExpressionTreeBuilder::ExpressionTreeBuilder(const ValueTree& ast_, const ReferenceCountedArray<jit::ComplexType>& typeLibrary_):
  r(Result::ok()),
  astTree(ast_),
  typeLibrary(typeLibrary_)
{}

jit::FunctionCollectionBase* ExpressionTreeBuilder::createEngine(jit::GlobalScope & globalScope)
{
	Engine::StatementFactory f;
	f.typeLibrary = typeLibrary;

	auto rootTree = f.create(astTree);
	f.createdNodes.root = rootTree;
	return new interpreter::Engine(globalScope, f.createdNodes);
}

} // namespace interpreter
} // namespace snex
