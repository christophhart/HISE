// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#pragma once

#include <cstdint>

#include <vector> // Embedded member.


namespace NativeJIT
{
    class CallSite;


    class Label
    {
    public:
        Label();
        explicit Label(size_t id);

        size_t GetId() const;

    private:
        size_t m_id;
    };


    // A JumpTable maintains collections of Labels and CallSites used in jump resolution.
    class JumpTable
    {
    public:
        JumpTable();

        void Clear();

        // Allocates a label that can be used as an argument to an X64 JCC/JMP/CALL instruction.
        Label AllocateLabel();

        // Binds a label to a memory address.
        void PlaceLabel(Label label, const uint8_t* address);

        // Records a position in memory containing call site (offset set portion of a JCC/JMP/CALL instruction)
        // that will need to be patch once all labels have been placed.
        void AddCallSite(Label label, uint8_t* site, unsigned size);

        // Patches each call site with the correct offset derived from its resolved label.
        void PatchCallSites();

        bool LabelIsDefined(Label label) const;
        const uint8_t* AddressOfLabel(Label label) const;

    private:
        // DESIGN NOTE: JumpTable is a part of CodeBuffer which is designed to
        // be allocated once and reused multiple times during the program lifetime.
        // Labels and call sites can thus use heap since the allocation (due to
        // reserve()) will happen only at startup and perhaps very rarely be
        // extended during the program lifetime.
        std::vector<uint8_t const *> m_labels;      // Storage for jump labels.
        std::vector<CallSite> m_callSites;          // Call sites to be patched.
    };


    // Call sites represent positions in memory which will contain jump offsets.
    // These locations need to be patched after all label offsets are resolved.
    class CallSite
    {
    public:
        CallSite();
        CallSite(Label label, unsigned size, uint8_t* site);

        Label GetLabel() const;

        size_t Size() const;
        uint8_t* Site() const;

    private:
        Label m_label;              // The jump label used to find the jump target address for this site.
        size_t m_size;            // The number of bytes to be patched at this site.
        uint8_t* m_site;    // The address to be patched.
    };
}
