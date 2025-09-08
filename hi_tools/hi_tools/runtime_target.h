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
    ExternalModulatorChain,
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

struct target_base
{
	virtual ~target_base() {};

    virtual void onData(const void* data, size_t numBytes) { ignoreUnused(data); ignoreUnused(numBytes); };
};

struct connection
{
    using ConnectFunction = bool(source_base*, target_base*);

    connection() = default;
    virtual ~connection() {};

    ConnectFunction* connectFunction = nullptr;
    ConnectFunction* disconnectFunction = nullptr;
    void* sendBackFunction = nullptr;
    void* sendBackDataFunction = nullptr;
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

template <typename MessageType> struct typed_target: public target_base
{
    ~typed_target() override {};
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
        sendBackDataFunction = other.sendBackDataFunction;
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

    void sendDataToSource(void* data, size_t numBytes)
    {
	    if(this->source != nullptr)
	    {
		    typedef void(*TypedFunction)(source_base*, void*, size_t);
            auto tf = (TypedFunction)this->sendBackDataFunction;
            tf(this->source, data, numBytes);
	    }
    }
    
    template <bool Add> bool connect(typed_target<MessageType>* obj)
    {
        auto ptr = Add ? this->connectFunction : this->disconnectFunction;
     
        // A connection without function pointers is supposed to always
        // connect
        if(this->source != nullptr && ptr == nullptr)
            return true;

        return ptr(this->source, obj);
    }
};


template <typename IndexSetter, RuntimeTarget TypeIndex, typename MessageType>
struct indexable_target:
public typed_target<MessageType>
{
    using TypedConnection = typed_connection<MessageType>;
    
    virtual ~indexable_target()
    {
        // this must be called before this constructor
        jassert(!isConnected());
    };

    /** Call this in the derived class destructor where you implement onData() etc... */
    void disconnect()
    {
	    if(currentConnection)
	    {
		    if(currentConnection.template connect<false>(this))
			    currentConnection.clear();
	    }
    }

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
        
        if(((th != ch) || !add) && match(th))
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

    bool sendDataToSource(void* data, size_t numBytes)
    {
	    currentConnection.sendDataToSource(data, numBytes);
        return isConnected();
    }
    
    IndexSetter& getIndex() { return index; }

    bool isConnected() const { return (bool)currentConnection; }

protected:
    
    TypedConnection currentConnection;
    IndexSetter index;
};

} // runtime_target
} // hise
