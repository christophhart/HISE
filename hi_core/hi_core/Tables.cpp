/*
  ==============================================================================

    Tables.cpp
    Created: 22 Jun 2014 2:39:33pm
    Author:  Chrisboy

  ==============================================================================
*/


float Interpolator::interpolateLinear(const float lowValue, const float highValue, const float delta)
{
	jassert(isPositiveAndNotGreaterThan(delta, 1.0f));

	const float invDelta = 1.0f - delta;
	return invDelta * lowValue + delta * highValue;
};

Table::Table()
{
		graphPoints.add(GraphPoint(0.0, 0.0, 0.5));
		graphPoints.add(GraphPoint(1.0, 1.0, 0.5));
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
	ScopedPointer<GraphPointComparator> gpc = new GraphPointComparator();
	graphPoints.sort(*gpc);

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

	ScopedLock sl(lock);
	FloatVectorOperations::copy(getWritePointer(), newValues.getRawDataPointer(), getTableSize());
};

float *MidiTable::getWritePointer() {return data;};

float *SampleLookupTable::getWritePointer() {return data;};