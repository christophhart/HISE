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

#include "JuceHeader.h"

namespace scriptnode {
using namespace juce;
using namespace hise;

#if 0

/** Howto convert hardcoded node classes to pimpl nodes:

1. Paste the entire class definition here.
2. Move implementation and alias definitions to source file (create impl namespace).
3. Subclass from hardcoded_pimpl instead
4. Add DEFINE_DSP_METHODS_PIMPL to class definition.
5. Add DSP_METHODS_PIMPL_IMPL(classname_) in source file (before createParameters)
6. Add `auto& obj = *pimpl;` to createParameters implementation.
*/

namespace meta
{
namespace width_bandpass_impl
{
// Template Alias Definition =======================================================

struct instance : public hardcoded_pimpl
{
	SN_NODE_ID("width_bandpass");
	SN_GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	String getSnippetText() const override;

	void createParameters(ParameterDataList& data);

	DEFINE_DSP_METHODS_PIMPL;
};

}

using width_bandpass = width_bandpass_impl::instance;

namespace filter_delay_impl
{

struct instance : public hardcoded_pimpl
{
	SN_NODE_ID("filter_delay");
	SN_GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	String getSnippetText() const override;

	void createParameters(ParameterDataList& data);

	DEFINE_DSP_METHODS_PIMPL;
};

}

using filter_delay = filter_delay_impl::instance;

namespace tremolo_impl
{

struct instance : public hardcoded_pimpl
{
	SN_NODE_ID("tremolo");
	SN_GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	String getSnippetText() const override;
	void createParameters(ParameterDataList& data);

	DEFINE_DSP_METHODS_PIMPL;
};

}

using tremolo = tremolo_impl::instance;

namespace transient_designer_impl
{

struct instance : public hardcoded_pimpl
{
	SN_NODE_ID("transient_designer");
	SN_GET_SELF_AS_OBJECT(instance);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	String getSnippetText() const override;

	void createParameters(ParameterDataList& data);

	DEFINE_DSP_METHODS_PIMPL;
};

}

using transient_designer = transient_designer_impl::instance;

}


#endif

}
