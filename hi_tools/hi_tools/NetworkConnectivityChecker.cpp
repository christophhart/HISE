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

namespace hise {

// Forward declarations -- each platform file provides these two free functions.
NetworkConnectivityChecker::NetworkType getCurrentSystemNetworkType();
double getCurrentSystemRSSI();

NetworkConnectivityChecker::NetworkConnectivityChecker()
{
    lastKnownType = getCurrentSystemNetworkType();
    lastKnownRSSI = getCurrentSystemRSSI();
}

NetworkConnectivityChecker::~NetworkConnectivityChecker()
{
    stop();
}

void NetworkConnectivityChecker::start()
{
    startTimer (TimerIntervalMs);
}

void NetworkConnectivityChecker::stop()
{
    stopTimer();
}

NetworkConnectivityChecker::NetworkType NetworkConnectivityChecker::getCurrentNetworkType()
{
    lastKnownType = getCurrentSystemNetworkType();
    return lastKnownType;
}

void NetworkConnectivityChecker::timerCallback()
{
    // Only query network type here -- getRSSI() triggers an active WiFi scan on
    // Windows (WlanScan) which is expensive and should only run on explicit request.
    const auto newType = getCurrentSystemNetworkType();

    if (newType != lastKnownType)
    {
        lastKnownType = newType;
        listeners.call ([newType] (Listener& l) { l.networkStatusChanged (newType); });
    }
}

} // namespace hise
