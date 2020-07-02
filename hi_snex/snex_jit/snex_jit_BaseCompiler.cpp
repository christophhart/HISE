/*  ===========================================================================
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
namespace jit {
using namespace juce;
using namespace asmjit;

    void BaseCompiler::executeOptimization(ReferenceCountedObject* statement, BaseScope* scope)
    {
        if(currentOptimization == nullptr)
            return;
        
        Operations::Statement::Ptr ptr(dynamic_cast<Operations::Statement*>(statement));
        
		if (dynamic_cast<OptimizationPass*>(currentOptimization)->processStatementInternal(this, scope, ptr))
		{
			throw BaseCompiler::OptimisationSucess();
		}
    }

	void BaseCompiler::optimize(ReferenceCountedObject* statement, BaseScope* scope, bool useExistingPasses)
	{
		OwnedArray<OptimizationPassBase> constExprPasses;

		OwnedArray<OptimizationPassBase>* toUse = nullptr;

		if (useExistingPasses)
		{
			toUse = &passes;
		}
		else
		{
			OptimizationFactory f;

			Array<Identifier> optList = { OptimizationIds::BinaryOpOptimisation, OptimizationIds::ConstantFolding };

			for (const auto& id : optList)
				constExprPasses.add(f.createOptimization(id));

			toUse = &constExprPasses;
		}

		Operations::Statement::Ptr ptr(dynamic_cast<Operations::Statement*>(statement));

		bool noMoreOptimisationsPossible = false;

		while (!noMoreOptimisationsPossible)
		{
			try
			{
				for (auto o : *toUse)
				{
					currentOptimization = o;
					ptr->process(this, scope);
				}

				noMoreOptimisationsPossible = true;
			}
			catch (OptimisationSucess& s)
			{
				if(useExistingPasses)
					logMessage(MessageType::VerboseProcessMessage, "Repeat optimizations");
			}
		}
	}


	void BaseCompiler::executePass(Pass p, BaseScope* scope, ReferenceCountedObject* statement)
    {
		auto st = dynamic_cast<Operations::Statement*>(statement);

        if (isOptimizationPass(p) && passes.isEmpty())
            return;
        
        setCurrentPass(p);
        
		if (isOptimizationPass(p))
		{
			for (auto s : *st)
			{
				for (auto o : passes)
					o->reset();

				if (isOptimizationPass(p))
				{
					optimize(s, scope, true);
				}
			}
		}
		else
			st->process(this, scope);
    }
}
}
