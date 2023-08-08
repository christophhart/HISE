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

#include <math.h>

namespace hise { using namespace juce;

mdaLimiter::mdaLimiter()
{
	fParam1 = (float)0.60; //thresh 		
  fParam2 = (float)0.60; //trim
  fParam3 = (float)0.15; //attack
  fParam4 = (float)0.50; //release
  fParam5 = (float)0.40; //knee

  if(fParam5>0.5) //soft knee
  {
    thresh = (float)pow(10.0, 1.0 - (2.0 * fParam1));
  }
  else //hard knee
  {
    thresh = (float)pow(10.0, (2.0 * fParam1) - 2.0);
  }
  trim = (float)(pow(10.0, (2.0 * fParam2) - 1.0));
  att = (float)pow(10.0, -0.01 - 2.0 * fParam3);//wavelab overruns with zero???
  rel = (float)pow(10.0, -2.0 - (3.0 * fParam4));
  gain = 1.0;
}

mdaLimiter::~mdaLimiter()
{
	// nothing to do here
}

String mdaLimiter::getEffectName() const { return "Limiter"; }

void mdaLimiter::setParameter(int index, float value)
{
	switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam4 = value; break;
    case 3: fParam3 = value; break;
    case 4: fParam5 = value; break;
  }
  //calcs here
  if(fParam5>0.5) //soft knee
  {
    thresh = (float)pow(10.0, 1.0 - (2.0 * fParam1));
  }
  else //hard knee
  {
    thresh = (float)pow(10.0, (2.0 * fParam1) - 2.0);
  }
  trim = (float)(pow(10.0, (2.0 * fParam2) - 1.0));
  att = (float)pow(10.0, -2.0 * fParam3);
  rel = (float)pow(10.0, -2.0 - (3.0 * fParam4));
}

float mdaLimiter::getParameter(int index) const
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam4; break;
    case 3: v = fParam3; break;
    case 4: v = fParam5; break;
  }
  return v;
}

#pragma warning( push )
#pragma warning( disable : 4996)

void mdaLimiter::getParameterName(int index, char *label) const
{
	switch(index)
  {
    case 0: strcpy(label, "Thresh"); break;
    case 1: strcpy(label, "Output"); break;
    case 3: strcpy(label, "Attack"); break;
    case 2: strcpy(label, "Release"); break;
    case 4: strcpy(label, "Knee"); break;
  }
}

#include <stdio.h>
void int2strng(int value, char *string) { snprintf(string, 512, "%d", value); }

void mdaLimiter::getParameterDisplay(int index, char *text) const
{
	switch(index)
  {
    case 0: int2strng((int)(40.0*fParam1 - 40.0),text); break;
    case 1: int2strng((int)(40.0*fParam2 - 20.0),text); break;
    case 3: int2strng((int)(-301030.1 / (getSampleRate() * log10(1.0 - att))),text); break;
    case 2: int2strng((int)(-301.0301 / (getSampleRate() * log10(1.0 - rel))),text); break;
    case 4: if(fParam5<0.5) strcpy(text, "HARD");
            else strcpy(text, "SOFT"); break;
  }
	
}

void mdaLimiter::getParameterLabel(int index, char *label) const
{
	switch(index)
  {
    case 0: strcpy(label, "dB"); break;
    case 1: strcpy(label, "dB"); break;
    case 3: strcpy(label, "ms"); break; 
    case 2: strcpy(label, "ms"); break; 
    case 4: strcpy(label, ""); break; 
  }
}

#pragma warning( pop ) 

//--------------------------------------------------------------------------------
// process

void mdaLimiter::process(float **inputs, float **outputs, int sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
  float g, c, d, at, re, tr, th, lev, ol, or_;
  
  th = thresh;
  g = gain;
  at = att;
  re = rel;
  tr = trim;
  
  --in1;	
	--in2;	
	--out1;
	--out2;

  if(fParam5>0.5) //soft knee
  {
	  while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;
      c = out1[1];
      d = out2[1];

      lev = (float)(1.0 / (1.0 + th * fabs(ol + or_)));
      if(g>lev) { g=g-at*(g-lev); } else { g=g+re*(lev-g); }

      c += (ol * tr * g);
      d += (or_ * tr * g);

      *++out1 = c;	
		  *++out2 = d;
	  }
  }
  else
  {
    while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;
      c = out1[1];
      d = out2[1];
 		  
      lev = (float)(0.5 * g * fabs(ol + or_));
     
      if (lev > th)
      {
        g = g - (at * (lev - th));
      }
      else 
      {
        g = g + (float)(re * (1.0 - g));
      }

      c += (ol * tr * g);
      d += (or_ * tr * g);

      *++out1 = c;	
		  *++out2 = d;
	  }
  }
  gain = g;
}

void mdaLimiter::processReplacing(float **inputs, float **outputs, int sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float g, at, re, tr, th, lev, ol, or_;
  
  th = thresh;
  g = gain;
  at = att;
  re = rel;
  tr = trim;
  
	--in1;	
	--in2;	
	--out1;
	--out2;
  if(fParam5>0.5) //soft knee
  {
	  while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;

      lev = (float)(1.0 / (1.0 + th * fabs(ol + or_)));
      if(g>lev) { g=g-at*(g-lev); } else { g=g+re*(lev-g); }

      *++out1 = (ol * tr * g);	
		  *++out2 = (or_ * tr * g);	
	  }
  }
  else
  {
 	  while(--sampleFrames >= 0)
	  {
      ol = *++in1;
      or_ = *++in2;
		  
      lev = (float)(0.5 * g * fabs(ol + or_));
    
      if (lev > th)
      {
        g = g - (at * (lev - th));
      }
      else //below threshold
      {
        g = g + (float)(re * (1.0 - g));
      }

      *++out1 = (ol * tr * g);	
		  *++out2 = (or_ * tr * g);	
	  }
  }
  gain = g;
}

} // namespace hise
