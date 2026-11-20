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
*   Network connectivity detection is based on code from SquarePine Modules:
*   https://gitlab.com/squarepine/squarepinemodules
*   Used under the MIT licence.
*
*   ===========================================================================
*/

#pragma once

namespace hise {

using namespace juce;

/** Monitors network connectivity using native OS APIs.

    Polls every 2500ms and notifies listeners when the connection type changes.
    All methods must be called on the main thread.

    On Windows, add iphlpapi.lib and wlanapi.lib to your linker inputs.
    On Apple platforms, link SystemConfiguration.framework (declared in hi_tools.h).
*/
class NetworkConnectivityChecker : private Timer
{
public:
    enum class NetworkType
    {
        none,   // No active network connection
        wifi,   // IEEE 802.11 wireless
        wired,  // Ethernet
        mobile, // Cellular (3G/4G/5G)
        other   // Connected but type is unrecognised
    };

    struct Listener
    {
        virtual ~Listener() = default;

        /** Called on the main thread when the connection type changes. */
        virtual void networkStatusChanged (NetworkType newType) = 0;
    };

    NetworkConnectivityChecker();
    ~NetworkConnectivityChecker() override;

    /** Starts the polling timer. Safe to call more than once. */
    void start();

    /** Stops the polling timer. */
    void stop();

    /** Returns the last type detected by the timer -- no OS call, safe to call frequently. */
    NetworkType getLastKnownNetworkType() const noexcept { return lastKnownType; }

    /** Queries the OS immediately and updates the cached state. */
    NetworkType getCurrentNetworkType();

    /** Convenience wrapper around getLastKnownNetworkType(). */
    bool isConnectedToInternet() const noexcept { return lastKnownType != NetworkType::none; }

    /** Returns a normalised (0.0 to 1.0) RSSI value.
        Accuracy is platform-dependent; do not rely on the exact value. */
    double getRSSI() const noexcept { return lastKnownRSSI; }

    void addListener    (Listener* l) { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    void timerCallback() override;

    NetworkType lastKnownType { NetworkType::none };
    double lastKnownRSSI { 0.0 };
    ListenerList<Listener> listeners;

    static constexpr int TimerIntervalMs = 2500;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetworkConnectivityChecker)
};

} // namespace hise
