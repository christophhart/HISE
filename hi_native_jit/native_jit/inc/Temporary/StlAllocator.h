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

#include <cstddef> // For ptrdiff_t and size_t.

#include "IAllocator.h"

namespace Allocators
{
    //*************************************************************************
    //
    // StlAllocator is a is a wrapper class for IAllocator. It is used to
    // create an allocator that is compatible with the STL container classes
    // like map and vector.
    //
    // This template declaration is a subset of the declaration in the C++98
    // spec (ISO/IEC 14882:1998), section 20.4.1.
    // See specification draft: http://www.open-std.org/jtc1/sc22/open/n2356/.
    // Also see http://www.codeguru.com/Cpp/Cpp/cpp_mfc/stl/article.php/c4079/.
    //
    //*************************************************************************
    template <typename T> class StlAllocator {
    public:
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;
        typedef T         value_type;

        template <typename U> struct rebind
        {
            typedef StlAllocator<U> other;
        };

        StlAllocator(IAllocator& allocator);

        // The following two methods are in the C++98 specification but were
        // not implemented. Keeping the declarations here to document
        // differences from the specification and to help with debugging.
        //StlAllocator() throw();
        //StlAllocator(const StlAllocator&) throw();

        template <typename U> StlAllocator(const StlAllocator<U>&) throw();

        // The following method is in the C++98 specification but was
        // not implemented. Keeping the declarations here to document
        // differences from the specification and to help with debugging.
        //~StlAllocator() throw();

        pointer address(reference x) const;
        const_pointer address(const_reference x) const;

        pointer allocate(size_type count, const void* hint = 0);
        void deallocate(pointer p, size_type n);

        size_type max_size() const throw();

        void construct(pointer p, const_reference val);
        void destroy(pointer p);

        bool operator==(const StlAllocator&);
        bool operator!=(const StlAllocator&);


    private:
        template <typename S>
        friend class StlAllocator;

        // StlAllocator does not implement an assignment operator because
        // it stores a reference to its IAllocator.
        StlAllocator& operator=(const StlAllocator&);

        IAllocator& m_allocator;
    };


    template <typename T>
    StlAllocator<T>::StlAllocator(IAllocator& allocator)
        : m_allocator(allocator)
    {
    }


    template <typename T>
    template <typename U>
    StlAllocator<T>::StlAllocator(const StlAllocator<U>& rhs) throw()
        : m_allocator(rhs.m_allocator)
    {
    }


    template <typename T>
    typename StlAllocator<T>::pointer StlAllocator<T>::address(typename StlAllocator<T>::reference x) const
    {
        return &x;
    }


    template <typename T>
    typename StlAllocator<T>::const_pointer StlAllocator<T>::address(typename StlAllocator<T>::const_reference x) const
    {
        return &x;
    }


    template <typename T>
    typename StlAllocator<T>::pointer StlAllocator<T>::allocate(typename StlAllocator<T>::size_type count, const void* /*hint*/)
    {
        return static_cast<StlAllocator<T>::pointer>(m_allocator.Allocate(sizeof(T) * count));
    }


    template <typename T>
    void StlAllocator<T>::deallocate(typename StlAllocator<T>::pointer p, typename StlAllocator<T>::size_type /*count*/)
    {
        m_allocator.Deallocate(p);
    }


    template <typename T>
    typename StlAllocator<T>::size_type StlAllocator<T>::max_size() const throw()
    {
        return m_allocator.MaxSize();
    }


    template <typename T>
    void StlAllocator<T>::construct(typename StlAllocator<T>::pointer ptr, typename StlAllocator<T>::const_reference val)
    {
        new (ptr) T(val);
    }


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif
    // Warning C4100 says that 'ptr' is unreferenced as a formal parameter.
    // This happens when T is a simple type like int. In this case it seems that
    // the compiler optimizes away the call to ptr->~T(), causing the ptr to
    // seem unreferenced. This seems like a compiler bug.
    template <typename T>
    void StlAllocator<T>::destroy(typename StlAllocator<T>::pointer ptr)
    {
        ptr->~T();
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif


    template <typename T>
    bool StlAllocator<T>::operator==(const StlAllocator&)
    {
        return true;
    }


    template <typename T>
    bool StlAllocator<T>::operator!=(const StlAllocator&)
    {
        return false;
    }
}
