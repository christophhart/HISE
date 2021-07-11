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

#if JUCE_WINDOWS
 #undef _WIN32_WINNT
 #define _WIN32_WINNT 0x500
 #undef STRICT
 #define STRICT 1
 #include <windows.h>
 #include <float.h>
 #if JUCE_MSVC
  #pragma warning (disable : 4312 4355)
 #endif
 #ifdef __INTEL_COMPILER
  #pragma warning (disable : 1899)
 #endif
#elif JUCE_LINUX
 #include <float.h>
 #include <sys/time.h>
 #include <arpa/inet.h>
#elif JUCE_MAC || JUCE_IOS
 #if ! (defined (JUCE_SUPPORT_CARBON) || defined (__LP64__))
  #define JUCE_SUPPORT_CARBON 1
 #endif

 #ifdef __OBJC__
  #if JUCE_MAC
   #include <Cocoa/Cocoa.h>
  #elif JUCE_IOS
   #include <UIKit/UIKit.h>
  #else
   #error
  #endif
 #endif

 #if JUCE_SUPPORT_CARBON && (! JUCE_IOS)
  #include <Carbon/Carbon.h>
 #endif

 #include <objc/runtime.h>
 #include <objc/objc.h>
 #include <objc/message.h>
#endif
