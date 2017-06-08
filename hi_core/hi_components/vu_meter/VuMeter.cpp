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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


VuMeter::VuMeter(float leftPeak/*=0.0f*/, float rightPeak/*=0.0f*/, Type t /*= MonoHorizontal*/) :
type(t),
l(leftPeak),
r(rightPeak),
previousValue(0.0f)
{
	setOpaque(true);

	colours[outlineColour] = Colours::transparentBlack;
	colours[backgroundColour] = Colours::black;
	colours[ledColour] = Colour(0x90bbb8);
}

void VuMeter::paint(Graphics &g)
{
	switch (type)
	{
	case MonoHorizontal:	drawMonoMeter(g); break;
	case MonoVertical:		drawMonoMeter(g); break;
	case StereoHorizontal:	drawStereoMeter(g); break;
	case StereoVertical:	drawStereoMeter(g); break;
    case MultiChannelVertical:
    case MultiChannelHorizontal:
	case numTypes:          break;
	}
}

void VuMeter::setType(Type newType)
{

	type = newType;
	if (type == StereoHorizontal || type == StereoVertical)
	{
		l = -100.0f;
		r = -100.0f;
	}
}

void VuMeter::setPeak(float left, float right/*=0.0f*/)
{
	if (type == StereoHorizontal || type == StereoVertical)
	{
		l -= 3.0f;
		r -= 3.0f;

		l = jmax(l, Decibels::gainToDecibels(left));
		r = jmax(r, Decibels::gainToDecibels(right));
		repaint();
	}
	else
	{


		if (left != l)
		{
			l = jmax(0.0f, left);

			repaint();
		}
	}
}

void VuMeter::drawMonoMeter(Graphics &g)
{
	const float w = (float)getWidth();
	const float h = (float)getHeight();

	float v = jlimit<float>(0.0f, 1.0f, l);



	g.setColour(colours[backgroundColour]);
	g.fillAll();

	g.setColour(colours[outlineColour]);
	g.drawRect(getLocalBounds());

	if (type == MonoHorizontal)
	{
		const float value = (w - 4.0f) * v;




		g.setGradientFill(ColourGradient(colours[ledColour].withMultipliedAlpha(.5f),
			0.0f, 0.0f,
			colours[ledColour].withMultipliedAlpha(0.2f),
			0.0f, h, false));

		g.fillRect(2.0f, 2.0f, value, h - 4.0f);

		previousValue = value;

	}

	if (type == MonoVertical)
	{
		g.setGradientFill(ColourGradient(colours[ledColour].withAlpha(0.2f),
			0.0f, 0.0f,
			colours[ledColour].withAlpha(0.05f),
			w, 0.0f, false));

		//g.fillRect(2.0f, 2.0f,		  w -4.0f, h - 4.0f);

		const float value = h * v;
		const float offset = h * (1.0f - v);

		g.setGradientFill(ColourGradient(colours[ledColour],
			0.0f, 0.0f,
			colours[ledColour].withMultipliedAlpha(0.5f),
			0.0f, h, false));


		Rectangle<int> a((int)2.0f, (int)offset, (int)w - 4, (int)value);

		if (w >= 16.0)
		{
			DropShadow d(Colours::white.withAlpha(0.2f), 5, Point<int>());

			d.drawForRectangle(g, a);
		}

		g.fillRect(a);

	}
}

void VuMeter::drawStereoMeter(Graphics &g)
{
	const float w = (float)getWidth();
	const float h = (float)getHeight();

	const float vL = jmin(1.0f, (l + 100.0f) / 100.0f);
	const float vR = jmin(1.0f, (r + 100.0f) / 100.0f);

	g.setColour(colours[backgroundColour]);
	g.fillAll();

	g.setColour(colours[outlineColour]);
	g.drawRect(getLocalBounds());

	if (type == StereoHorizontal)
	{
		g.setGradientFill(ColourGradient(colours[ledColour].withAlpha(0.2f),
			0, 0.0f,
			colours[ledColour].withAlpha(0.05f),
			0.0f, h, false));

		g.fillRect(2.0f, 2.0f, w - 4.0f, h / 2.0f - 3.0f);
		g.fillRect(2.0f, h / 2.0f + 1, w - 4.0f, h / 2.0f - 3.0f);

		const float valueL = (w)* vL;
		const float valueR = (w)* vR;

		const float offsetL = jmin(w, valueL);
		const float offsetR = jmin(w, valueR);

		g.setGradientFill(ColourGradient(colours[ledColour].withAlpha(1.0f).withMultipliedBrightness(1.4f),
			0.0f, 0.0f,
			colours[ledColour].withMultipliedBrightness(0.7f),
			0.0f, h, false));

		for (float i = 3.0f; i < offsetL; i += 3.0f)
		{
			g.drawLine(i, 2.0f, i, h / 2.0f - 1.0f, 1.0f);
		}

		for (float i = 3.0f; i < offsetR; i += 3.0f)
		{
			g.drawLine(i, h / 2.0f + 1.0f, i, h - 2.0f, 1.0f);
		}
	}
	else
	{
		g.setGradientFill(ColourGradient(colours[ledColour].withAlpha(0.2f),
			0.0f, 0.0f,
			colours[ledColour].withAlpha(0.05f),
			0.0f, h, false));



		g.fillRect(2.0f, 2.0f, w / 2.0f - 3.0f, h - 4.0f);
		g.fillRect(w / 2.0f + 1, 2.0f, w / 2.0f - 3.0f, h - 4.0f);

		const float valueL = (h)* vL;
		const float valueR = (h)* vR;

		const float offsetL = jmin(h, h - valueL);
		const float offsetR = jmin(h, h - valueR);

		g.setGradientFill(ColourGradient(colours[ledColour].withAlpha(1.0f).withMultipliedBrightness(1.4f),
			0.0f, 0.0f,
			colours[ledColour].withMultipliedBrightness(0.7f),
			0.0f, h, false));

		for (float i = h - 4; i > offsetL; i -= 3.0f)
		{

			g.drawLine(2.0, i, w / 2.0f - 1.0f, i, 1.0f);

		}

		for (float i = h - 4; i > offsetR; i -= 3.0f)
		{
			g.drawLine(w / 2.0f + 1.0f, i, w - 2.0f, i, 1.0f);
		}
	}
}

void WaveformComponent::paint(Graphics &g)
{
	Path path;

	switch (type)
	{
	case Sine:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 37, 67, 92, 46, 171, 67, 98, 0, 0, 37, 67, 92, 46, 171, 67, 0, 0, 42, 67, 92, 174, 163, 67, 0, 0, 47, 67, 92, 174, 163, 67, 98, 0, 0, 52, 67, 92, 174, 163, 67, 0, 0, 57, 67, 92, 46, 171, 67, 0, 0, 57, 67, 92, 46, 171, 67, 98, 0, 0, 57, 67, 92, 46, 171, 67, 0, 0, 62, 67, 92, 174, 178, 67, 0, 0, 67, 67, 92, 174, 178, 67,
			98, 0, 0, 72, 67, 92, 174, 178, 67, 0, 0, 77, 67, 92, 46, 171, 67, 0, 0, 77, 67, 92, 46, 171, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		break;
	}
	case Triangle:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 32, 67, 92, 46, 186, 67, 108, 0, 0, 42, 67, 92, 174, 178, 67, 108, 0, 0, 52, 67, 92, 46, 186, 67, 108, 0, 0, 62, 67, 92, 174, 193, 67, 108, 0, 0, 72, 67, 92, 46, 186, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		break;

	}
	case Saw:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 37, 67, 92, 174, 203, 67, 108, 0, 0, 37, 67, 92, 46, 196, 67, 108, 0, 0, 57, 67, 92, 174, 203, 67, 108, 0, 0, 77, 67, 92, 46, 211, 67, 108, 0, 0, 77, 67, 92, 174, 203, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));

		break;

	}
	case Square:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 37, 67, 92, 174, 223, 67, 108, 0, 0, 37, 67, 92, 46, 216, 67, 108, 0, 0, 57, 67, 92, 46, 216, 67, 108, 0, 0, 57, 67, 92, 174, 223, 67, 108, 0, 0, 57, 67, 92, 46, 231, 67, 108, 0, 0, 77, 67, 92, 46, 231, 67, 108, 0, 0, 77, 67, 92, 174, 223, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));
		break;

	}
	case Noise:
	{
		static const unsigned char pathData[] = { 110, 109, 0, 0, 92, 67, 92, 46, 211, 67, 108, 0, 0, 92, 67, 92, 174, 203, 67, 108, 0, 0, 97, 67, 92, 174, 203, 67, 108, 0, 0, 97, 67, 92, 46, 216, 67, 108, 0, 0, 102, 67, 92, 46, 216, 67, 108, 0, 0, 102, 67, 92, 174, 213, 67, 108, 0, 0, 107, 67, 92, 174, 213, 67, 108, 0, 0, 107, 67, 92, 46, 206, 67, 108, 0, 0, 112, 67, 92, 46, 206,
			67, 108, 0, 0, 112, 67, 92, 46, 216, 67, 108, 0, 0, 117, 67, 92, 46, 216, 67, 108, 0, 0, 117, 67, 92, 174, 208, 67, 108, 0, 0, 122, 67, 92, 174, 208, 67, 108, 0, 0, 122, 67, 92, 46, 206, 67, 108, 0, 0, 127, 67, 92, 46, 206, 67, 108, 0, 0, 127, 67, 92, 174, 218, 67, 108, 0, 0, 130, 67, 92, 174, 218, 67, 108, 0, 0, 130, 67, 92, 46,
			211, 67, 99, 101, 0, 0 };

		path.loadPathFromData(pathData, sizeof(pathData));
		break;

	}
	case Custom:
	case numWaveformTypes: break;

	}

	path.scaleToFit(2.0f, 2.0f, (float)getWidth() - 4.0f, (float)getHeight() - 4.0f, false);

	KnobLookAndFeel::fillPathHiStyle(g, path, getWidth(), getHeight());
}
