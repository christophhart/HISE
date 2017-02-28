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


#include <cstddef>
#include <limits>
#include <stdexcept>

#include "../../inc/Temporary/Assert.h"
#include "../../inc/NativeJIT/CodeGen/JumpTable.h"


namespace NativeJIT
{
    const unsigned c_labelsReserveCount = 512;
    const unsigned c_callSitesReserveCount = 512;

    JumpTable::JumpTable()
    {
        m_labels.reserve(c_labelsReserveCount);
        m_callSites.reserve(c_callSitesReserveCount);
    }


    void JumpTable::Clear()
    {
        m_labels.clear();
        m_callSites.clear();
    }


    Label JumpTable::AllocateLabel()
    {
        Label label = Label(m_labels.size());
        m_labels.push_back(NULL);

        return label;
    }


    void JumpTable::PlaceLabel(Label label, const uint8_t* address)
    {
        if (LabelIsDefined(label))
        {
            throw std::runtime_error("CodeBuffer: attempting to place label more than once.");
        }

        m_labels[label.GetId()] = address;
    }


    void JumpTable::AddCallSite(Label label, uint8_t* site, unsigned size)
    {
        m_callSites.push_back(CallSite(label, size, site));
    }


    bool JumpTable::LabelIsDefined(Label label) const
    {
        return m_labels.at(label.GetId()) != NULL;
    }


    const uint8_t* JumpTable::AddressOfLabel(Label label) const
    {
        if (!LabelIsDefined(label))
        {
            throw std::runtime_error("CodeBuffer: attempting to use a label that hasn't been placed.");
        }

        return m_labels[label.GetId()];
    }


    // WARNING: Non portable. Assumes little endian machine architecture.
    // WARNING: Non portable. Assumes that fixup value is labelAddress - siteAddress - size.Size().
    void JumpTable::PatchCallSites()
    {
        for (size_t i=0; i < m_callSites.size(); ++i)
        {
            const CallSite& site = m_callSites[i];
            const uint8_t* labelAddress = AddressOfLabel(site.GetLabel());
            uint8_t* siteAddress = site.Site();
            ptrdiff_t delta = labelAddress - siteAddress - site.Size();

            // TODO: Evaluate whether special cases for size == 2 and size == 4 actually improve performance.
            size_t size = site.Size();
            if (size == 2)
            {

				
#if 0
                LogThrowAssert(delta <= std::numeric_limits<int16_t>::max() &&
                               delta >= std::numeric_limits<int16_t>::min(),
                               "Overflow/underflow in cast to int16_t.");
#endif

                *(reinterpret_cast<int16_t*>(siteAddress)) = static_cast<int16_t>(delta);
                siteAddress += size;
            }
            else if (size == 4)
            {
#if 0
                LogThrowAssert(delta <= std::numeric_limits<int32_t>::max() &&
                               delta >= std::numeric_limits<int32_t>::min(),
                               "Overflow/underflow in cast to int32_t.");
#endif
                *(reinterpret_cast<int32_t*>(siteAddress)) = static_cast<int32_t>(delta);
                siteAddress += size;
            }
            else
            {
                while (size > 0)
                {
                    *siteAddress++ = static_cast<uint8_t>(delta);
                    delta = delta >> 8;
                    size--;
                }
            }
        }
    }

    //*************************************************************************
    //
    // CallSite
    //
    //*************************************************************************
    CallSite::CallSite()
        : m_label(0)
    {
        m_site = NULL;
    }

    CallSite::CallSite(Label label, unsigned size, uint8_t* site)
        : m_label(label)
    {
        m_size = size;
        m_site = site;
    }

    Label CallSite::GetLabel() const
    {
        return m_label;
    }

    size_t CallSite::Size() const
    {
        return m_size;
    }

    uint8_t* CallSite::Site() const
    {
        return m_site;
    }
}
