/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{
namespace dsp
{

/**
    Acts as a polymorphic base class for processors.
    This exposes the same set of methods that a processor must implement as virtual
    methods, so that you can use the ProcessorWrapper class to wrap an instance of
    a subclass, and then pass that around using ProcessorBase as a base class.
    @see ProcessorWrapper

    @tags{DSP}
*/
struct ProcessorBase
{
    ProcessorBase() = default;
    virtual ~ProcessorBase() = default;

    virtual void prepare (const ProcessSpec&)  = 0;
    virtual void process (const ProcessContextReplacing<float>&) = 0;
    virtual void reset() = 0;
};


//==============================================================================
/**
    Wraps an instance of a given processor class, and exposes it through the
    ProcessorBase interface.
    @see ProcessorBase

    @tags{DSP}
*/
template <typename ProcessorType>
struct ProcessorWrapper  : public ProcessorBase
{
    void prepare (const ProcessSpec& spec) override
    {
        processor.prepare (spec);
    }

    void process (const ProcessContextReplacing<float>& context) override
    {
        processor.process (context);
    }

    void reset() override
    {
        processor.reset();
    }

    ProcessorType processor;
};

} // namespace dsp
} // namespace juce
