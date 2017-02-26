# 3rd party SDKs

In order to build certain configurations, you'll need some 3rd party SDKs that can't be distributed 
along with the rest of the source code for licencing reasons. Instead you'll need to register at each
company and do everything they tell you to do in order to get their SDKs. 

Then download the SDK archives and extract them into this directory so that this directory structure is used:

```
tools/SDK/
tools/SDK/VSTSDK/
tools/SDK/AAX/

If you already have the SDKs, just copy them here. HISE is using the relative paths from the main 
HISE source code folder to save you setup hazzleness.

## ASIO SDK

This is required to enable ASIO low latency support for the standalone applications. Go to

https://www.steinberg.net/de/company/developer.html

and download the ASIO SDK v2.3. Extract them here so that you'll get this directory structure:

```
tools/SDK/ASIOSDK2.3/
tools/SDK/ASIOSDK2.3/asio
tools/SDK/ASIOSDK2.3/common
...
```

## VST SDK

HISE doesn't use VST3, but you'll need to get the VST3SDK because it includes the VST 2.4 SDK.

Extract them so you end up with this directory structure:

You need this directory structure:

```
tools/SDK/VST3 SDK/plugininterfaces
tools/SDK/VST3 SDK/public.sdk
```

