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
#include <float.h>

namespace hise { using namespace juce;

mdaDegrade::mdaDegrade()
{
	

	//inits here!
	fParam1 = (float)0.8; //clip
	fParam2 = (float)0.50; //bits
	fParam3 = (float)0.5; //rate
	fParam4 = (float)0.9; //postfilt
	fParam5 = (float)0.58; //non-lin
	fParam6 = (float)0.5; //level

	buf0 = buf1 = buf2 = buf3 = buf4 = buf5 = buf6 = buf7 = buf8 = buf9 = 0.0f;

	buf0R = buf1R = buf2R = buf3R = buf4R = buf5R = buf6R = buf7R = buf8R = buf9R = 0.0f;
	setParameter(5, 0.5f);
}

void mdaDegrade::setParameter(int index, float value)
{
	float f;
  
  switch(index)
  {
    case 0: fParam1 = value; break;
    case 1: fParam2 = value; break;
    case 2: fParam3 = value; break;
    case 3: fParam4 = value; break;
    case 4: fParam5 = value; break; 
    case 5: fParam6 = value; break;
  }
  //calcs here
  if(fParam3>0.5) { f = fParam3 - 0.5f;  mode = 1.0f; }
             else { f = 0.5f - fParam3;  mode = 0.0f; }
    
  tn = (int)exp(18.0f * f);
    
    //tn = (int)(18.0 * fParam3 - 8.0); mode=1.f; }
    //         else { tn = (int)(10.0 - 18.0 * fParam3); mode=0.f; }
  tcount = 1;
  clp = (float)(pow(10.0,(-1.5 + 1.5 * fParam1)) );
  fo2 = filterFreq((float)pow(10.0f, 2.30104f + 2.f*fParam4));
  fi2 = (1.f-fo2); fi2=fi2*fi2; fi2=fi2*fi2;
  float _g1 = (float)(pow(2.0,2.0 + int(fParam2*12.0))); 
  g2 = (float)(1.0/(2.0 * _g1));
  if(fParam3>0.5) g1 = -_g1/(float)tn; else g1= -_g1; 
  g3 = (float)(pow(10.0,2.0*fParam6 - 1.0));
  if(fParam5>0.5) { lin = (float)(pow(10.0,0.3 * (0.5 - fParam5))); lin2=lin; }
  else { lin = (float)pow(10.0,0.3 * (fParam5 - 0.5)); lin2=1.0; }
}

mdaDegrade::~mdaDegrade()
{
}

void mdaDegrade::suspend()
{
}

float mdaDegrade::filterFreq(float hz)
{
  float j, k, r=0.999f;
  
  j = r * r - 1;
  k = (float)(2.f - 2.f * r * r * cos(0.647f * hz / getSampleRate() ));
  return (float)((sqrt(k*k - 4.f*j*j) - k) / (2.f*j));
}

float mdaDegrade::getParameter(int index) const
{
	float v=0;

  switch(index)
  {
    case 0: v = fParam1; break;
    case 1: v = fParam2; break;
    case 2: v = fParam3; break;
    case 3: v = fParam4; break;
    case 4: v = fParam5; break;
    case 5: v = fParam6; break;
  }
  return v;
}

#pragma warning( push )
#pragma warning( disable : 4996)

void mdaDegrade::getParameterName(int index, char *label) const
{
	switch(index)
  {
    case 0: strcpy(label, "Headroom"); break;
    case 1: strcpy(label, "Quant"); break;
    case 2: strcpy(label, "Rate"); break;
    case 3: strcpy(label, "PostFilt"); break;
    case 4: strcpy(label, "Non-Lin"); break;
    case 5: strcpy(label, "Output"); break;
  }
}

void mdaDegrade::getParameterDisplay(int index, char *text) const
{
	switch(index)
  {
    case 0: int2strng((int)(-30.0 * (1.0 - fParam1)), text); break;
    case 1: int2strng((int)(4.0 + 12.0 * fParam2), text); break;
    case 2: int2strng((int)(getSampleRate()/tn), text); break;
    case 3: int2strng((int)(pow(10.0f, 2.30104f + 2.f*fParam4)), text); break;
    case 4: int2strng((int)(200.0 * fabs(fParam5 - 0.5)), text); break;
    case 5: int2strng((int)(40.0 * fParam6 - 20.0), text); break;
  }
}

void mdaDegrade::getParameterLabel(int index, char *label) const
{
	switch(index)
  {
    case 0: strcpy(label, "dB"); break;
    case 1: strcpy(label, "bits"); break;
    case 2: strcpy(label, "Hz"); break;
    case 3: strcpy(label, "Hz"); break;
    case 4: strcpy(label, "Odd<>Eve"); break;
  }
}

#pragma warning( pop ) 

//--------------------------------------------------------------------------------
// process


void mdaDegrade::processReplacing(float **inputs, float **outputs, int sampleFrames)
{
	float *in1 = inputs[0];
	float *in2 = inputs[1];
	float *out1 = outputs[0];
	float *out2 = outputs[1];
	float l=lin, l2=lin2;
	float cl=clp, i2=fi2, o2=fo2;
	
	float gi=g1, go=g2, ga=g3, m=mode;  
	int n=tn, t=tcount;   
  
	float b0=buf0;
	float b1=buf1;
	float b2=buf2;
	float b3=buf3;
	float b4=buf4;
	float b5=buf5;
	float b6=buf6;
	float b7=buf7;
	float b8=buf8;
	float b9=buf9;

	float b0R = buf0R;
	float b1R = buf1R;
	float b2R = buf2R;
	float b3R = buf3R;
	float b4R = buf4R;
	float b5R = buf5R;
	float b6R = buf6R;
	float b7R = buf7R;
	float b8R = buf8R;
	float b9R = buf9R;
	
	--in1;	
	--in2;	
	--out1;
	--out2;

	while(--sampleFrames >= 0)
	{
		b0 = (*++in1) + m * b0;
		b0R = (*++in2) + m * b0R;
		

		if(t==n)
		{
			t=0;
			b5 = (float)(go * int(b0 * gi));
			b5R = (float)(go * int(b0R * gi));
			
			if(b5>0)
			{
				b5=(float)pow(b5,l2); 
				if(b5>cl) b5=cl;
			}
			else
			{ 
				b5=-(float)pow(-b5,l); 
				if(b5<-cl) b5=-cl; 
			}

			if(b5R>0)
			{
				b5R=(float)pow(b5R,l2); 
				if(b5R>cl) b5R=cl;
			}
			else
			{ 
				b5R=-(float)pow(-b5R,l); 
				if(b5R<-cl) b5R=-cl; 
			}


			b0=0;
			b0R=0;
		} 
		t=t+1;

		b1 = i2 * (b5 * ga) + o2 * b1;
		b2 =      b1 + o2 * b2;
		b3 =      b2 + o2 * b3;
		b4 =      b3 + o2 * b4;
		b6 = i2 * b4 + o2 * b6;
		b7 =      b6 + o2 * b7;
		b8 =      b7 + o2 * b8;
		b9 =      b8 + o2 * b9;

		b1R = i2 * (b5R * ga) + o2 * b1R;
		b2R =      b1R + o2 * b2R;
		b3R =      b2R + o2 * b3R;
		b4R =      b3R + o2 * b4R;
		b6R = i2 * b4R + o2 * b6R;
		b7R =      b6R + o2 * b7R;
		b8R =      b7R + o2 * b8R;
		b9R =      b8R + o2 * b9R;

		*++out1 = b9;
		*++out2 = b9R;
	}
	if(fabs(b1)<1.0e-10) 
	{ 
		buf0=0.f; 
		buf1=0.f; 
		buf2=0.f; 
		buf3=0.f; 
		buf4=0.f;
		buf5=0.f; 
		buf6=0.f; 
		buf7=0.f;
		buf8=0.f; 
		buf9=0.f; 
	}
	else 
	{ 
		buf0=b0; 
		buf1=b1; 
		buf2=b2; 
		buf3=b3; 
		buf4=b4;
		buf5=b5;
		buf6=b6; 
		buf7=b7; 
        buf8=b8;
		buf9=b9;  
		tcount=t; 
	}
	if(fabs(b1R)<1.0e-10) 
	{ 
		buf0R=0.f; 
		buf1R=0.f; 
		buf2R=0.f; 
		buf3R=0.f; 
		buf4R=0.f;
		buf5R=0.f; 
		buf6R=0.f; 
		buf7R=0.f;
		buf8R=0.f; 
		buf9R=0.f; 
	}
	else 
	{ 
		buf0R=b0R; 
		buf1R=b1R; 
		buf2R=b2R; 
		buf3R=b3R; 
		buf4R=b4R;
		buf5R=b5R;
		buf6R=b6R; 
		buf7R=b7R; 
        buf8R=b8R;
		buf9R=b9R;  
		tcount=t; }

}
} // namespace hise
