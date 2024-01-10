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

namespace hise
{
using namespace juce;

#define DEFAULT_PROPERTIES(x) SN_NODE_ID(#x); DefaultProperties getDefaultProperties() const override { return getStaticDefaultProperties(); } \
    static DefaultProperties getStaticDefaultProperties()

namespace multipage {

#define DECLARE_ID(x) static const Identifier x(#x);

namespace mpid
{
    // Root properties
	DECLARE_ID(StyleData);
    DECLARE_ID(LayoutData);
    DECLARE_ID(Properties);
    DECLARE_ID(GlobalState);
    DECLARE_ID(Children);

    // Global Properties
    DECLARE_ID(Header);
    DECLARE_ID(Subtitle);

    // Child properties
    DECLARE_ID(Type);
    DECLARE_ID(ID);
    DECLARE_ID(CallType);
    DECLARE_ID(Text);
    DECLARE_ID(Items);
    DECLARE_ID(Help);
    DECLARE_ID(Foldable);
    DECLARE_ID(Folded);
    DECLARE_ID(Required);
    DECLARE_ID(Multiline);
    DECLARE_ID(Value);
    DECLARE_ID(Padding);
    DECLARE_ID(Width);
    DECLARE_ID(Wildcard);
    DECLARE_ID(SaveFile);
    DECLARE_ID(Directory);
    DECLARE_ID(Visible);

}

#undef DECLARE_ID

class DefaultProperties
{
public:
    DefaultProperties(std::initializer_list<std::pair<Identifier, var>> ilist)
    : ilist_(ilist) {}

    const std::pair<Identifier, var>* begin() const noexcept
    {
        return ilist_.data();
    }

    const std::pair<Identifier, var>* end() const noexcept
    {
        return ilist_.data() + size();
    }

    std::size_t size() const noexcept
    {
        return ilist_.size();
    }

private:
    
    const std::vector<std::pair<Identifier, var>> ilist_;
};

}

}