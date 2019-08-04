/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   balanceKnob_200_png;
    const int            balanceKnob_200_pngSize = 14976;

    extern const char*   FrontendKnob_Bipolar_png;
    const int            FrontendKnob_Bipolar_pngSize = 7253;

    extern const char*   FrontendKnob_Unipolar_png;
    const int            FrontendKnob_Unipolar_pngSize = 6717;

    extern const char*   infoError_png;
    const int            infoError_pngSize = 3915;

    extern const char*   infoInfo_png;
    const int            infoInfo_pngSize = 3180;

    extern const char*   infoQuestion_png;
    const int            infoQuestion_pngSize = 3790;

    extern const char*   infoWarning_png;
    const int            infoWarning_pngSize = 2843;

    extern const char*   knobEmpty_200_png;
    const int            knobEmpty_200_pngSize = 27940;

    extern const char*   knobModulated_200_png;
    const int            knobModulated_200_pngSize = 9941;

    extern const char*   knobUnmodulated_200_png;
    const int            knobUnmodulated_200_pngSize = 16137;

    extern const char*   logo_mini_png;
    const int            logo_mini_pngSize = 3429;

    extern const char*   logo_new_png;
    const int            logo_new_pngSize = 29122;

    extern const char*   oxygen_bold_ttf;
    const int            oxygen_bold_ttfSize = 48812;

    extern const char*   oxygen_mono_regular_otf;
    const int            oxygen_mono_regular_otfSize = 25860;

    extern const char*   oxygen_regular_ttf;
    const int            oxygen_regular_ttfSize = 48092;

    extern const char*   toggle_200_png;
    const int            toggle_200_pngSize = 2427;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 16;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
