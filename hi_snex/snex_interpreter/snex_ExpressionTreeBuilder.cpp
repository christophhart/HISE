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
	registerType<nodes::Function>();
	registerType<nodes::ReturnStatement>();
	registerType<nodes::Immediate>();
	registerType<nodes::BinaryOp>();
	registerType<nodes::Cast>();
	registerType<nodes::VariableReference>();
	registerType<nodes::Assignment>();
}

ExpressionTreeBuilder::ExpressionTreeBuilder(const String& code, const String& preprocessed, const ValueTree& ast_, const ReferenceCountedArray<jit::ComplexType>& typeLibrary_):
  r(Result::ok()),
  astTree(ast_),
  typeLibrary(typeLibrary_),
  locationBuilder(new UriLocationBuilder(code, preprocessed))
{}

jit::FunctionCollectionBase* ExpressionTreeBuilder::createEngine(jit::GlobalScope & globalScope)
{
	Statement::Factory f;
	
	f.locationBuilder = locationBuilder;
	f.typeLibrary = typeLibrary;

	DBG(astTree.createXml()->createDocument(""));

	

	auto rootTree = f.create(astTree);

	auto locTree = createUriLocationTree(f, astTree, nullptr);

	f.createdNodes.root = rootTree;
	
	return new interpreter::Engine(globalScope, f.createdNodes);
}

UriLocationTree::Ptr ExpressionTreeBuilder::createUriLocationTree(const Statement::Factory& f, const ValueTree& v, UriLocationTree::Ptr parent)
{
	if (auto ptr = f.createdNodes.get(v))
	{
		if (ptr->shouldCreateNode() == Statement::WantsNode::Yes)
		{
			UriLocationTree::Ptr np = new UriLocationTree(parent.get());

			np->fillLocation(locationBuilder->createLocation(v));
			np->category = v.getType();
			ptr->annotateUriLocationTree(*np);

			for (auto c : v)
			{
				createUriLocationTree(f, c, np);
			}

			return np;
		}

		return nullptr;
	}
}

UriLocationBuilder::UriLocationBuilder(const String& code, const String& preprocessedCode)
{
	baseLocation.uri = "tbd";
	baseLocation.processorId = "tbd";

	codeDoc.setDisableUndo(true);
	codeDoc.replaceAllContent(code);

	if (code != preprocessedCode)
	{
		preprocessedDoc.setDisableUndo(true);
		preprocessedDoc.replaceAllContent(preprocessedCode);
		resolvePreprocessedCode = true;
	}
}

UriLocation UriLocationBuilder::createLocation(const ValueTree& v) const
{
	auto charIndex = (int)v[AstIds::charIndex];

	UriLocation l;
	l.fillLocation(baseLocation);

	if (resolvePreprocessedCode)
	{
		CodeDocument::Position prePos(preprocessedDoc, charIndex);

		auto line = prePos.getLineNumber();
		auto col = prePos.getIndexInLine();

		CodeDocument::Position realPos(codeDoc, line, col);
		l.fillLocation(realPos);
	}
	else
	{
		CodeDocument::Position charPos(codeDoc, charIndex);
		l.fillLocation(charPos);
	}

	return l;
}

} // namespace interpreter
} // namespace snex
