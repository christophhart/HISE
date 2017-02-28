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


#include <algorithm>    // For std::max

#include "../../inc/NativeJIT/ExpressionTree.h"
#include "../../inc/NativeJIT/Nodes/Node.h"
#include "../../inc/Temporary/Assert.h"


namespace NativeJIT
{
    //*************************************************************************
    //
    // NodeBase
    //
    //*************************************************************************
    NodeBase::NodeBase(ExpressionTree& tree)
        : m_id(tree.AddNode(*this)),
          m_parentCount(0),
          m_isReferenced(false),
          m_hasBeenEvaluated(false)
    {
    }


    unsigned NodeBase::GetId() const
    {
        return m_id;
    }


    void NodeBase::IncrementParentCount()
    {
        LogThrowAssert(!HasBeenEvaluated(), "Cannot change the parent count after the node was evaluated");

        ++m_parentCount;
        MarkReferenced();
    }


    void NodeBase::DecrementParentCount()
    {
        LogThrowAssert(!HasBeenEvaluated(), "Cannot change the parent count after the node was evaluated");
        LogThrowAssert(m_parentCount > 0,
                       "Cannot decrement parent count of node %u with zero parents",
                       GetId());

        --m_parentCount;
        // Note: m_isReferenced is not affected by this, decrementing the parent
        // count is optimization-related call which doesn't change the
        // fact that a node is referenced at least conceptually. This is because
        // currently having a non-referenced node is considered to be an error,
        // and not a chance for removing such nodes.
    }


    bool NodeBase::HasBeenEvaluated() const
    {
        return m_hasBeenEvaluated;
    }


    void NodeBase::MarkEvaluated()
    {
        m_hasBeenEvaluated = true;
    }


    bool NodeBase::IsReferenced() const
    {
        return m_isReferenced;
    }


    void NodeBase::MarkReferenced()
    {
        m_isReferenced = true;
    }


    unsigned NodeBase::GetParentCount() const
    {
        return m_parentCount;
    }


    void NodeBase::CompileAsRoot(ExpressionTree& /*tree*/)
    {
        LogThrowAbort("Root of ExpressionTree must be a ReturnNode node.");
    }


    bool NodeBase::CanBeOptimizedAway() const
    {
        return GetParentCount() == 0;
    }


    void NodeBase::ReleaseReferencesToChildren()
    {
        LogThrowAbort("Don't know how to remove optimized away references for node with ID %u (orphan node?)",
                      GetId());
    }


    bool NodeBase::GetBaseAndOffset(NodeBase*& /* base */, int32_t& /* offset */) const
    {
        return false;
    }
}
