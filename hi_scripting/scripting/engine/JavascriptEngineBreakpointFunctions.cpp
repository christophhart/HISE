/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise {
using namespace juce;

HiseJavascriptEngine::RootObject::BreakpointState::ScopedSetter::ScopedSetter(const Scope& s, const Statement* statementAfterBreak) :
	r(*s.root)
{
	auto& bp = s.root->breakpointState;

	bp.clearBreakPosition();

	prevScope = bp.currentBreakpointScope;
	prevCreator = bp.currentLocalScopeCreator;
	prevStatement = bp.statementAfterBreak;
	bp.currentBreakpointScope = &s;
	bp.statementAfterBreak = statementAfterBreak;
	bp.currentLocalScopeCreator = s.root->threadAwareLocalScopeCreator.get();
}

HiseJavascriptEngine::RootObject::BreakpointState::ScopedSetter::~ScopedSetter()
{
	auto& bp = r.breakpointState;
	bp.currentBreakpointScope = prevScope;
	bp.statementAfterBreak = prevStatement;
	bp.currentLocalScopeCreator = prevCreator;
}

UriLocation HiseJavascriptEngine::moveInstructionPointer(const UriBreakpoint& newLocation)
{
	return root->moveInstructionPointer(newLocation);
}

std::pair<UriBreakpoint::Owner::AttachState, UriBreakpoint> HiseJavascriptEngine::attachBreakpoint(const UriBreakpoint& newBreakpoint)
{
	for (const auto& bp : root->breakpointState.resolvedBreakpoints)
	{
		if (bp.second.matches(newBreakpoint))
			return { UriBreakpoint::Owner::AttachState::Duplicate, newBreakpoint };
	}

	for (const auto& bp : root->breakpointState.unresolvedBreakpoints)
	{
		if (bp.matches(newBreakpoint))
			return { UriBreakpoint::Owner::AttachState::Duplicate, newBreakpoint };
	}

	if (auto r = root->addRuntimeBreakpoint(newBreakpoint))
	{
		if (root->breakpointState.hasBreakpoint(r))
			return { UriBreakpoint::Owner::AttachState::Duplicate, root->breakpointState.getBreakpoint(r) };

		auto realBreakpoint = newBreakpoint;
		r->location.fillLocation(realBreakpoint, true);
		root->breakpointState.resolvedBreakpoints[r] = realBreakpoint;

		return { UriBreakpoint::Owner::AttachState::OK, realBreakpoint };
	}

	return { UriBreakpoint::Owner::AttachState::Unresolved, newBreakpoint };
}

bool HiseJavascriptEngine::setRuntimeBreakpoint(const UriBreakpoint& bp, UriBreakpoint::Owner::UriBreakpointListener::ChangeType ct)
{
	using CT = UriBreakpoint::Owner::UriBreakpointListener::ChangeType;

	switch (ct)
	{
	case CT::Removed:
		return root->clearRuntimeBreakpoint(bp);
	case CT::Added:
		return root->addRuntimeBreakpoint(bp);
	case CT::Modified:
		return root->updateRuntimeBreakpoint(bp);
	case CT::AttachStateChange:

	default:
		return false;
	}
}

void HiseJavascriptEngine::setOnInitBreakpoints(const Array<UriBreakpoint>& breakpoints)
{
	jassert(root->breakpointState.resolvedBreakpoints.empty());
	jassert(root->breakpointState.unresolvedBreakpoints.isEmpty());

	root->breakpointState.unresolvedBreakpoints.addArray(breakpoints);
}

void HiseJavascriptEngine::clearRuntimeBreakpoints()
{
	root->breakpointState.resolvedBreakpoints.clear();
}

bool HiseJavascriptEngine::RootObject::clearRuntimeBreakpoint(const UriBreakpoint& bp)
{
	if (auto s = breakpointState.getStatementWithResolvedBreakpoint(bp))
	{
		breakpointState.resolvedBreakpoints.erase(s);
		return true;
	}

	return false;
}

const HiseJavascriptEngine::RootObject::Statement* HiseJavascriptEngine::RootObject::attachBreakpointInBlock(BlockStatement* bl, const UriBreakpoint& bp)
{
	const Statement* bestMatch = nullptr;

	UriLocation blockLocation;
	bl->location.fillLocation(blockLocation, true);

	CodeLocation cl = bl->location;
	cl.location = bl->closeLocation;

	Range<int> charRange(bl->location.getCharIndex(), cl.getCharIndex() + 1);

	if (blockLocation.sameUriAndProcessorId(bp) && charRange.contains(bp.charIndex))
	{
		auto rootTree = Statement::Tree::create(bl);

		bestMatch = rootTree->forEach<const Statement*>([&](const Statement* s)
			{
				const Statement* rv = nullptr;

				if (s->location.getCharIndex() >= bp.charIndex)
					rv = s;

				return rv;
			});

		if (bestMatch == nullptr)
			bestMatch = bl->statements.getLast();
		else if (auto bl = dynamic_cast<const BlockStatement*>(bestMatch))
		{
			if (auto firstChild = bl->statements.getFirst())
				bestMatch = firstChild;
		}
		else
		{
			auto parentBlock = rootTree->findParentWithType<BlockStatement>(bestMatch);
			auto indexInParent = rootTree->getIndexWithin(parentBlock, bestMatch);
			auto childOfBlock = rootTree->getChildOf(parentBlock, indexInParent);

			if (childOfBlock != nullptr)
				bestMatch = childOfBlock;
		}
	}

	return bestMatch;
}

const HiseJavascriptEngine::RootObject::Statement* HiseJavascriptEngine::RootObject::updateRuntimeBreakpoint(const UriBreakpoint& bp)
{
	for (auto& item : breakpointState.resolvedBreakpoints)
	{
		if (item.second.matches(bp))
		{
			item.second = bp;
			return item.first;
		}
	}

	return nullptr;
}

const HiseJavascriptEngine::RootObject::Statement* HiseJavascriptEngine::RootObject::addRuntimeBreakpoint(const UriBreakpoint& bp)
{
	auto resolvedStatement = forEachRuntimeBlockStatement([&](BlockStatement* bl)
		{
			return this->attachBreakpointInBlock(bl, bp);
		});

	return resolvedStatement;
}

const HiseJavascriptEngine::RootObject::Statement* HiseJavascriptEngine::RootObject::forEachRuntimeBlockStatement(const std::function<const Statement* (BlockStatement*)>& f)
{
	for (auto c : hiseSpecialData.callbackNEW)
	{
		if (auto bl = c->statements.get())
		{
			if (auto s = f(bl))
				return s;
		}
	}

	for (const auto& v : breakpointFunctions)
	{
		if (auto ifo = dynamic_cast<InlineFunction::Object*>(v.getObject()))
		{
			if (auto s = f(ifo->body.get()))
			{
				ifo->hasBreakpoints = true;
				return s;
			}
		}

		if (auto fo = dynamic_cast<FunctionObject*>(v.getObject()))
		{
			if (auto bl = dynamic_cast<BlockStatement*>(fo->body.get()))
			{
				if (auto s = f(bl))
				{
					fo->hasBreakpoints = true;
					return s;
				}
			}
		}
	}

	return nullptr;
}

UriLocation HiseJavascriptEngine::RootObject::moveInstructionPointer(const UriBreakpoint& newLocation)
{
	UriLocation loc;

	if (breakpointState.currentLocalScopeCreator.get() == nullptr)
	{
		jassertfalse;
		return {};
	}

	if (auto executedRootStatement = breakpointState.currentLocalScopeCreator.get()->getRootBlockStatement())
	{
		auto rootTree = Statement::Tree::create(executedRootStatement);

		auto resolved = rootTree->forEach<bool>([&](const Statement* s)
			{
				auto sIndex = s->location.getCharIndex();

				if (sIndex >= newLocation.charIndex)
				{
					if (rootTree->matches(s))
					{
						breakpointState.nextInstructionRoot = rootTree->getTypedStatement<BlockStatement>();
						breakpointState.nextInstructionIndex = 0;
						loc = s->location.createUriLocation();
					}
					else
					{
						std::pair<const BlockStatement*, int> rv;

						auto bp = rootTree->findParentWithType<BlockStatement>(s);
						breakpointState.nextInstructionRoot = bp;

						if (bp == s)
							breakpointState.nextInstructionIndex = 0;
						else
							breakpointState.nextInstructionIndex = rootTree->getIndexWithin(bp, s);

						auto movedStatement = bp->statements[breakpointState.nextInstructionIndex];

						auto parentTree = rootTree->findRecursive(movedStatement)->getParentTree();

						auto findJumpTarget = [movedStatement](const Statement* s)
							{
								const Statement* rv = nullptr;

								if (!s->isValidJumpTarget())
									rv = s;

								return rv;
							};

						while (auto newBoundary = parentTree->forEachParent<const Statement*>(findJumpTarget))
						{
							// the new location has a jump boundary, check if it is compatible to the old boundary
							auto containsOldStatement = rootTree->findRecursive(newBoundary)->isParentOf(breakpointState.statementAfterBreak);

							if (containsOldStatement)
								break;

							bp = rootTree->findParentWithType<BlockStatement>(newBoundary);
							breakpointState.nextInstructionRoot = bp;
							breakpointState.nextInstructionIndex = rootTree->getIndexWithin(bp, s);

							movedStatement = bp->statements[breakpointState.nextInstructionIndex];
							parentTree = rootTree->findRecursive(movedStatement)->getParentTree();
						}

						jassert(movedStatement != nullptr);

						if (movedStatement != nullptr)
						{
							breakpointState.resolvedBreakpoints.erase(movedStatement);
							loc = movedStatement->location.createUriLocation();
							return true;
						}

						return true;
					}

					return true;
				}

				return false;
			});

		return loc;
	}

	return loc;
}

}