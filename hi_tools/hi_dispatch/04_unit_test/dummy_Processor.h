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

#pragma once
#include "dummy_Actions.h"

namespace hise {
namespace dispatch {
namespace dummy {
using namespace juce;

struct Processor: public ControlledObject,
				  public SourceOwner
{
    struct AddAction;
    struct ActionBase;
    struct RemoveAction;
    struct SetAttributeAction;
    struct SetBypassed;
    struct AddListener;
    struct RemoveListener;

    struct Builder: public Action::Builder
    {
        Builder(MainController* mc):
          Action::Builder(mc)
        {};

	    Action::List createActions(const var& jsonData) override;
        Identifier getId() const override { return ActionIds::processor; }
        Action::Ptr addAction;

        void expectOrThrow(bool ok, const String& errorMessage)
        {
            if(!ok)
                throw String(errorMessage);
        }
    };
    
    void setAttribute(int parameterIndex, float newValue, DispatchType n);
    void setBypassed(bool shouldBeBypassed, DispatchType n);

    Processor(MainController* mc, HashedCharPtr id);;
    ~Processor() override;

    library::Processor dispatcher;
};


} // dummy
} // dispatch
} // hise