/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================

/**
    Contains methods for finding out about the current hardware and OS configuration.

    @tags{Core}
*/
class JUCE_API  SystemStats  final
{
public:
    //==============================================================================

    /** Returns the current version of JUCE.
        See also the JUCE_VERSION, JUCE_MAJOR_VERSION and JUCE_MINOR_VERSION macros.
    */
    static String getJUCEVersion();

    //==============================================================================

    /** The set of possible results of the getOperatingSystemType() method. */
    enum OperatingSystemType
    {
        UnknownOS       = 0,

        MacOSX          = 0x0100,  
        Windows         = 0x0200,  
        Linux           = 0x0400,
        Android         = 0x0800,
        iOS             = 0x1000,
        WASM            = 0x2000,

        MacOSX_10_7     = MacOSX | 7,
        MacOSX_10_8     = MacOSX | 8,
        MacOSX_10_9     = MacOSX | 9,
        MacOSX_10_10    = MacOSX | 10,
        MacOSX_10_11    = MacOSX | 11,
        MacOSX_10_12    = MacOSX | 12,
        MacOSX_10_13    = MacOSX | 13,
        MacOSX_10_14    = MacOSX | 14,
        MacOSX_10_15    = MacOSX | 15,
        MacOS_11        = MacOSX | 16,
        MacOS_12        = MacOSX | 17,
        MacOS_13        = MacOSX | 18,
        MacOS_14        = MacOSX | 19,
        MacOS_15        = MacOSX | 20,

        Win2000         = Windows | 1,
        WinXP           = Windows | 2,
        WinVista        = Windows | 3,
        Windows7        = Windows | 4,
        Windows8_0      = Windows | 5,
        Windows8_1      = Windows | 6,
        Windows10       = Windows | 7,
        Windows11       = Windows | 8  
    };

    /** Returns the type of operating system we're running on. */
    static OperatingSystemType getOperatingSystemType();

    /** Returns the name of the type of operating system we're running on. */
    static String getOperatingSystemName();

    /** Returns true if the OS is 64-bit, or false for a 32-bit OS. */
    static bool isOperatingSystem64Bit();

    /** Returns an environment variable. */
    static String getEnvironmentVariable (const String& name, const String& defaultValue);

    //==============================================================================

    /** Returns the current user's name, if available. */
    static String getLogonName();

    /** Returns the current user's full name, if available. */
    static String getFullUserName();

    /** Returns the host-name of the computer. */
    static String getComputerName();

    /** Returns the language of the user's locale. */
    static String getUserLanguage();

    /** Returns the region of the user's locale. */
    static String getUserRegion();

    /** Returns the user's display language. */
    static String getDisplayLanguage();

    /** This will attempt to return some kind of string describing the device. */
    static String getDeviceDescription();

    /** This will attempt to return the manufacturer of the device. */
    static String getDeviceManufacturer();

    //==============================================================================

    /** This method calculates some IDs to uniquely identify the device.

        The first choice for an ID is a filesystem ID for the user's home folder or
        Windows directory. If that fails, this function returns the MAC addresses.
    */
    [[deprecated ("The identifiers produced by this function are not reliable. Use getUniqueDeviceID() instead.")]]
    static StringArray getDeviceIdentifiers();

    /** This method returns a machine unique ID unaffected by storage or peripheral
        changes.

        - On **non-mobile** platforms: ID changes when motherboard/CPU is replaced.  
        - On **iOS**: ID is stable for all vendor apps but resets when all are uninstalled.  
        - On **Android**: ID resets after a system restore.

        This function **may return an empty string** if the device has not been unlocked since a restart.
    */
    static String getUniqueDeviceID();

    /** Kinds of identifier that are passed to `getMachineIdentifiers()`. */
    enum class MachineIdFlags
    {
        macAddresses    = 1 << 0,  ///< All Mac addresses of the machine.
        fileSystemId    = 1 << 1,  ///< The filesystem ID of the home directory (or system directory on Windows).
        legacyUniqueId  = 1 << 2,  ///< Windows only. A hash of the full SMBIOS table, may be unstable.
        uniqueId        = 1 << 3   ///< The most stable machine identifier. A good default to use.
    };

    /** Returns a list of strings that can be used to uniquely identify a machine.

        - To get multiple kinds of identifier at once, use bitwise OR: `uniqueId | legacyUniqueId`.  
        - If an identifier is unavailable, it is omitted from the result.  
        - You can enable **all flags** and check against stored identifiers.
    */
    static StringArray getMachineIdentifiers (MachineIdFlags flags);

    //==============================================================================

    /** Returns the number of logical CPU cores. */
    static int getNumCpus() noexcept;

    /** Returns the number of physical CPU cores. */
    static int getNumPhysicalCpus() noexcept;

    /** Returns the approximate CPU speed (in MHz). */
    static int getCpuSpeedInMegahertz();

    /** Returns the CPU vendor. */
    static String getCpuVendor();

    /** Attempts to return a string describing the CPU model. */
    static String getCpuModel();

    static bool hasMMX() noexcept;  
    static bool has3DNow() noexcept;
    static bool hasFMA3() noexcept;
    static bool hasFMA4() noexcept;
    static bool hasSSE() noexcept;
    static bool hasSSE2() noexcept;
    static bool hasSSE3() noexcept;
    static bool hasSSSE3() noexcept;
    static bool hasSSE41() noexcept;
    static bool hasSSE42() noexcept;
    static bool hasAVX() noexcept;
    static bool hasAVX2() noexcept;
    static bool hasAVX512F() noexcept;
    static bool hasAVX512BW() noexcept;
    static bool hasAVX512CD() noexcept;
    static bool hasAVX512DQ() noexcept;
    static bool hasAVX512ER() noexcept;
    static bool hasAVX512IFMA() noexcept;
    static bool hasAVX512PF() noexcept;
    static bool hasAVX512VBMI() noexcept;
    static bool hasAVX512VL() noexcept;
    static bool hasAVX512VPOPCNTDQ() noexcept;
    static bool hasNeon() noexcept;

    //==============================================================================

    /** Returns the total system RAM (in MB). */
    static int getMemorySizeInMegabytes();

    /** Returns the system page-size. */
    static int getPageSize();

    //==============================================================================

    /** Returns a backtrace of the current call-stack. */
    static String getStackBacktrace();

    /** A function type for use in setApplicationCrashHandler(). */
    using CrashHandlerFunction = void (*)(void*);

    /** Sets a global crash handler callback function. */
    static void setApplicationCrashHandler (CrashHandlerFunction);

    /** Returns true if running inside an **app extension sandbox**.
        - **Always returns false on Windows, Linux, and Android.**
    */
    static bool isRunningInAppExtensionSandbox() noexcept;

   #if JUCE_MAC
    /** Returns true if the macOS **App Sandbox** is enabled. */
    static bool isAppSandboxEnabled();
   #endif

private:
    SystemStats() = delete; // uses only static methods
    JUCE_DECLARE_NON_COPYABLE(SystemStats)
};

//==============================================================================
// **Manual Bitwise Operators for JUCE 6 Compatibility**
//==============================================================================

inline SystemStats::MachineIdFlags operator| (SystemStats::MachineIdFlags lhs, SystemStats::MachineIdFlags rhs)
{
    return static_cast<SystemStats::MachineIdFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline SystemStats::MachineIdFlags operator& (SystemStats::MachineIdFlags lhs, SystemStats::MachineIdFlags rhs)
{
    return static_cast<SystemStats::MachineIdFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline SystemStats::MachineIdFlags operator~ (SystemStats::MachineIdFlags lhs)
{
    return static_cast<SystemStats::MachineIdFlags>(~static_cast<int>(lhs));
}

inline SystemStats::MachineIdFlags& operator|= (SystemStats::MachineIdFlags& lhs, SystemStats::MachineIdFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline SystemStats::MachineIdFlags& operator&= (SystemStats::MachineIdFlags& lhs, SystemStats::MachineIdFlags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

} // namespace juce