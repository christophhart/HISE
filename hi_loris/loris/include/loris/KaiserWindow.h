#ifndef INCLUDE_KAISERWINDOW_H
#define INCLUDE_KAISERWINDOW_H
/*
 * This is the Loris C++ Class Library, implementing analysis, 
 * manipulation, and synthesis of digitized sounds using the Reassigned 
 * Bandwidth-Enhanced Additive Sound Model.
 *
 * Loris is Copyright (c) 1999-2010 by Kelly Fitz and Lippold Haken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * KaiserWindow.h
 *
 * Definition of class Loris::KaiserWindow.
 *
 * Kelly Fitz, 14 Dec 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#include <vector>

//  begin namespace
namespace Loris {

// ---------------------------------------------------------------------------
//  class KaiserWindow
//
//! Computes samples of a Kaiser window 
//! function (see Kaiser and Schafer, 1980) for windowing FFT data.
//
class KaiserWindow
{
//  -- public interface --
public:

    //  -- window building --
    
    //! Build a new Kaiser analysis window having the specified shaping
    //! parameter. See Oppenheim and Schafer:  "Digital Signal Processing" 
    //! (1975), p. 452 for further explanation of the Kaiser window. Also, 
    //! see Kaiser and Schafer, 1980.
    //!
    //! \param      win is the vector that will store the window
    //!             samples. The number of samples computed will be
    //!             equal to the length of this vector. Any previous
    //!             contents will be overwritten.
    //! \param      shape is the Kaiser shaping parameter, controlling
    //!             the sidelobe rejection level.
    static void buildWindow( std::vector< double > & win, double shape );
    
    //! Build a new time-derivative Kaiser analysis window having the
    //! specified shaping parameter, for computing frequency reassignment.
    //! The closed form for the time derivative can be obtained from the
    //! property of modified Bessel functions that the derivative of the
    //! zeroeth order function is equal to the first order function.
    //!
    //! \param      win is the vector that will store the window
    //!             samples. The number of samples computed will be
    //!             equal to the length of this vector. Any previous
    //!             contents will be overwritten.
    //! \param      shape is the Kaiser shaping parameter, controlling
    //!             the sidelobe rejection level.
    static void buildTimeDerivativeWindow( std::vector< double > & win, double shape );    
    
    
    //  -- window parameter estimation --
    
    //! Compute a shaping parameter that will achieve the specified
    //! level of sidelobe rejection.
    //!
    //! \param      atten is the desired sidelobe attenuation in
    //!             positive decibels (e.g. 65 dB)
    //! \returns    the Kaiser shaping paramater
    static double computeShape( double atten );

    //! Compute the necessary length in samples of a Kaiser window
    //! having the specified shaping parameter that has the
    //! desired main lobe width.
    //!
    //! \param      width is the desired main lobe width expressed
    //!             as a fraction of the sample rate.
    //! \param      alpha is the Kaiser shaping parameter (the
    //!             main lobe width is influenced primarily by the
    //!             window length,but also by the shape).
    //! \returns    the window length in samples
    static unsigned long computeLength( double width, double alpha );

//  construction is not allowed:
private:
    KaiserWindow( void );
    
};  // end of class KaiserWindow

}   //  end of namespace Loris

#endif /* ndef INCLUDE_KAISERWINDOW_H */
