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

namespace hise { using namespace juce;;

Table::Table():
	xConverter(getDefaultTextValue),
	yConverter(getDefaultTextValue)
{
	
    graphPoints.add(GraphPoint(0.0, 0.0, 0.5));
    graphPoints.add(GraphPoint(1.0, 1.0, 0.5));
}

Table::~Table()
{
	

	masterReference.clear();
}

void Table::setGraphPoints(const Array<GraphPoint> &newGraphPoints, int numPoints)
{
	graphPoints.clear();
	graphPoints.addArray(newGraphPoints, 0, numPoints);
};

void Table::createPath(Path &normalizedPath) const
{
	normalizedPath.clear();
	normalizedPath.startNewSubPath(-0.00000001f, 1.0000001f);
	normalizedPath.lineTo(0.0f, 1.0f - graphPoints[0].y);

	for(int i = 1; i < graphPoints.size(); ++i)
	{
		const float curve = graphPoints[i].curve;
		
		const float end_x = graphPoints[i].x;
		const float end_y = 1.0f - graphPoints[i].y;

		if(curve == 0.5)
		{
			normalizedPath.lineTo(end_x, end_y);
		}
		else
		{
			const float prev_x = graphPoints[i-1].x;
			const float prev_y = 1.0f - graphPoints[i-1].y;

			const float control_x = curve * prev_x + (1-curve) * end_x;
			const float control_y = (1-curve) * prev_y + curve * end_y;

			normalizedPath.quadraticTo(control_x, control_y, end_x, end_y);
		}
		
	}

	normalizedPath.lineTo(1.0000001f, 0.0000001f); // fix wrong scaling if greatest value is < 1
	normalizedPath.lineTo(1.0000001f, 1.0000001f);
	normalizedPath.closeSubPath();
};

void Table::fillLookUpTable()
{
	GraphPointComparator gpc;
	graphPoints.sort(gpc);

	Path renderPath;

	createPath(renderPath);

	renderPath.applyTransform(AffineTransform::scale((float)getTableSize(), 1.0f));

	Line<float> l;
	Line<float> clipped;

	Array<float> newValues;

	for(int i = 0; i < getTableSize(); i++)
	{
		l = Line<float>((float)i, 0.0f, (float)i, 1.0f);
		clipped = renderPath.getClippedLine(l, false);
		const float value = 1.0f - (clipped.getStartY());
		jassert(i < getTableSize());
		newValues.add(value);
	};

	ScopedLock sl(getLock());
	FloatVectorOperations::copy(getWritePointer(), newValues.getRawDataPointer(), getTableSize());
};

float *MidiTable::getWritePointer() {return data;};

float *SampleLookupTable::getWritePointer() {return data;};

} // namespace hise