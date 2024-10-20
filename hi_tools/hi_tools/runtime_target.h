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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace hise
{
using namespace juce;

namespace runtime_target
{

/** There are multiple target types that can connect to a dynamic source in HISE. */
enum class RuntimeTarget
{
    Undefined,
    Macro,
    GlobalCable,
    NeuralNetwork,
    GlobalModulator,
    numRuntimeTargets
};

namespace indexers
{

/** Used by a connected node that can't change the connection. */
template <int HashIndex> struct fix_hash
{
    static constexpr int getIndex() { return HashIndex; }
};

/** Used by unconnected nodes. */
struct none
{
    static constexpr int getIndex() { return -1; }
};

/** Used by a connected node that can change the connection. */
struct dynamic
{
    int getIndex() const { return currentHash; }
    int currentHash = -1;
};

}



struct source_base;

struct connection
{
    connection() = default;
    virtual ~connection() {};

    void* connectFunction = nullptr;
    void* disconnectFunction = nullptr;
    void* sendBackFunction = nullptr;
    source_base* source = nullptr;
    
    void clear();
    RuntimeTarget getType() const;
    int getHash() const;
    operator bool() const { return source != nullptr; }
};

struct source_base
{
    virtual ~source_base() {};
    
    virtual int getRuntimeHash() const = 0;
    virtual RuntimeTarget getType() const = 0;
    
    virtual connection createConnection() const;
};

template <typename MessageType> struct target_base
{
    virtual ~target_base() {};
    virtual void onValue(MessageType value) = 0;
};

/** A simple POD with a type index and a hash as well
 as two function pointers that will connect / disconnect
 any object
 */
template <typename MessageType> struct typed_connection: public connection
{
    typed_connection& operator=(const connection& other)
    {
        source = other.source;
        connectFunction = other.connectFunction;
        disconnectFunction = other.disconnectFunction;
        sendBackFunction = other.sendBackFunction;
        return *this;
    }
    
    void sendValueToSource(MessageType t)
    {
        if(this->source != nullptr)
        {
            typedef void(*TypedFunction)(source_base*, MessageType);
            auto tf = (TypedFunction)this->sendBackFunction;
            tf(this->source, t);
        }
    }
    
    template <bool Add> bool connect(target_base<MessageType>* obj)
    {
        auto ptr = Add ? this->connectFunction : this->disconnectFunction;
     
        // A connection without function pointers is supposed to always
        // connect
        if(this->source != nullptr && ptr == nullptr)
            return true;
        
        typedef bool(*TypedFunction)(source_base*,  target_base<MessageType>*);
        
        auto typed = (TypedFunction)ptr;
        return typed(this->source, obj);
    }
};


template <typename IndexSetter, RuntimeTarget TypeIndex, typename MessageType>
struct indexable_target:
public target_base<MessageType>
{
    using TypedConnection = typed_connection<MessageType>;
    
    virtual ~indexable_target()
    {
        if(currentConnection)
            currentConnection.template connect<false>(this);
    };
    
    bool match(int hash) const
    {
        return index.getIndex() == hash;
    }
    
    void connectToRuntimeTarget(bool add, const connection& c)
    {
        if(c.getType() != TypeIndex)
            return;
        
        TypedConnection tc;
        tc = c;
        
        auto th = c.getHash();
        auto ch = currentConnection.getHash();
        
        if(th != ch && match(th))
        {
            if (add)
            {
                if(currentConnection)
                    currentConnection.template connect<false>(this);
                
                if(tc.template connect<true>(this))
                    currentConnection = c;
            }
            else
            {
                if(tc.template connect<false>(this))
                    currentConnection.clear();
            }
            
            onConnectionChange();
        }
    }
    
    virtual void onConnectionChange() {};
    
    void sendValueToSource(MessageType v)
    {
        currentConnection.sendValueToSource(v);
    }
    
    IndexSetter& getIndex() { return index; }
    
protected:
    
    TypedConnection currentConnection;
    IndexSetter index;
};

} // runtime_target
} // hise
