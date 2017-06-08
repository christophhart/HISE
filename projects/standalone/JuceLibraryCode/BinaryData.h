/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_28393625_INCLUDED
#define BINARYDATA_H_28393625_INCLUDED

namespace BinaryData
{
    extern const char*   logo_mini_png;
    const int            logo_mini_pngSize = 3429;

    extern const char*   infoError_png;
    const int            infoError_pngSize = 3915;

    extern const char*   infoInfo_png;
    const int            infoInfo_pngSize = 3180;

    extern const char*   infoQuestion_png;
    const int            infoQuestion_pngSize = 3790;

    extern const char*   infoWarning_png;
    const int            infoWarning_pngSize = 2843;

    extern const char*   FrontendKnob_Bipolar_png;
    const int            FrontendKnob_Bipolar_pngSize = 7253;

    extern const char*   FrontendKnob_Unipolar_png;
    const int            FrontendKnob_Unipolar_pngSize = 6717;

    extern const char*   balanceKnob_200_png;
    const int            balanceKnob_200_pngSize = 14976;

    extern const char*   knobEmpty_200_png;
    const int            knobEmpty_200_pngSize = 1071758;

    extern const char*   knobModulated_200_png;
    const int            knobModulated_200_pngSize = 289754;

    extern const char*   knobUnmodulated_200_png;
    const int            knobUnmodulated_200_pngSize = 289283;

    extern const char*   toggle_200_png;
    const int            toggle_200_pngSize = 2427;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 12;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
