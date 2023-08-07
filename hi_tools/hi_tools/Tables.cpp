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

	Table::Listener::~Listener()
	{}

	void Table::Listener::indexChanged(float newIndex)
	{}

	void Table::Listener::graphHasChanged(int point)
	{}

	void Table::Listener::onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var n)
	{
		switch (t)
		{
		case ComplexDataUIUpdaterBase::EventType::DisplayIndex:
			indexChanged((float)n);
			break;
		case ComplexDataUIUpdaterBase::EventType::ContentChange:
			graphHasChanged((int)n);
			break;
		default:
			jassertfalse;
			break;
		}
	}

	Table::ScopedUpdateDelayer::ScopedUpdateDelayer(Table& t):
		table(t)
	{
		prevValue = t.delayUpdates;
	}

	Table::ScopedUpdateDelayer::~ScopedUpdateDelayer()
	{
		table.internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
		table.fillLookUpTable();
		table.delayUpdates = prevValue;
	}

	String Table::getDefaultTextValue(float input)
	{ return String(roundToInt(input * 100.0f)) + "%"; }

	Table::GraphPoint::GraphPoint(float x_, float y_, float curve_): x(x_), y(y_), curve(curve_)
	{ }

	Table::GraphPoint::GraphPoint(const GraphPoint& g): x(g.x), y(g.y), curve(g.curve)
	{ }

	Table::GraphPoint::GraphPoint()
	{ jassertfalse; }

	void Table::restoreData(const String& savedString)
	{
		if (savedString.isEmpty())
		{
			reset();
			return;
		}
			
		MemoryBlock b;
		
		b.fromBase64Encoding(savedString);

		if(b.getSize() == 0) return;

		{
			hise::SimpleReadWriteLock::ScopedWriteLock sl(graphPointLock);
            
			graphPoints.clear();
			graphPoints.insertArray(0, static_cast<const Table::GraphPoint*>(b.getData()), (int)(b.getSize() / sizeof(Table::GraphPoint)));
		}
		
		if (!delayUpdates)
		{
			fillLookUpTable();
			internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
		}
	}

	void Table::setTablePoint(int pointIndex, float x, float y, float curve)
	{
		const float sanitizedX = jlimit(0.0f, 1.0f, x);
		const float sanitizedY = jlimit(0.0f, 1.0f, y);
		const float sanitizedCurve = jlimit(0.0f, 1.0f, curve);

		{
			hise::SimpleReadWriteLock::ScopedReadLock sl(graphPointLock);
            
			if (pointIndex >= 0 && pointIndex < graphPoints.size())
			{
				if (pointIndex != 0 && pointIndex != graphPoints.size() - 1)
				{
					// Just change the x value of non-edge points...
					graphPoints.getRawDataPointer()[pointIndex].x = sanitizedX;
				}
                
				graphPoints.getRawDataPointer()[pointIndex].y = sanitizedY;
				graphPoints.getRawDataPointer()[pointIndex].curve = sanitizedCurve;
			}
		}

		if (!delayUpdates)
		{
			fillLookUpTable();
			internalUpdater.sendContentChangeMessage(sendNotificationSync, pointIndex);
		}
	}

	void Table::reset()
	{
		{
			hise::SimpleReadWriteLock::ScopedWriteLock sl(graphPointLock);
			graphPoints.clear();
			graphPoints.add(GraphPoint(0.0f, 0.0f, 0.5f));
			graphPoints.add(GraphPoint(1.0f, 1.0f, 0.5f));
		}

		if (!delayUpdates)
		{
			internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
			fillLookUpTable();
		}
	}

	void Table::addTablePoint(float x, float y, float curve)
	{
		{
			hise::SimpleReadWriteLock::ScopedWriteLock sl(graphPointLock);
			graphPoints.add(GraphPoint(x, y, curve));
		}

		if (!delayUpdates)
		{
			internalUpdater.sendContentChangeMessage(sendNotificationAsync, graphPoints.size() - 1);
			fillLookUpTable();
		}
	}

	int Table::getNumGraphPoints() const
	{ return graphPoints.size(); }

	Table::GraphPoint Table::getGraphPoint(int pointIndex) const
	{return graphPoints.getUnchecked(pointIndex); }

	void Table::setStartAndEndY(float newStartY, float newEndY)
	{
		startY = newStartY;
		endY = newEndY;
		fillLookUpTable();
	}

	String Table::getXValueText(float value)
	{
		if (xConverter)
			return xConverter(value);
		else
			return String(value, 2);
	}

	String Table::getYValueText(float value)
	{
		if (yConverter)
			return yConverter(value);
		else
			return String(value, 2);
	}

	void Table::setXTextConverter(const ValueTextConverter& converter)
	{
		xConverter = converter;
	}

	void Table::setYTextConverterRaw(const ValueTextConverter& converter)
	{
		yConverter = converter;
	}

	Table::ValueTextConverter Table::getYTextConverter()
	{
		return yConverter;
	}

	void Table::setNormalisedIndexSync(float value)
	{
		internalUpdater.sendDisplayChangeMessage(value, sendNotificationAsync);
	}

	void Table::sendGraphUpdateMessage()
	{
		internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
	}

	void Table::addRulerListener(Listener* l)
	{
		internalUpdater.addEventListener(l);
	}

	void Table::removeRulerListener(Listener* l)
	{
		internalUpdater.removeEventListener(l);
	}

	Table::GraphPointComparator::GraphPointComparator()
	{}

	int Table::GraphPointComparator::compareElements(GraphPoint dp1, GraphPoint dp2)
	{
		if(dp1.x < dp2.x) return -1;
		else if(dp1.x > dp2.x) return 1;
		else return 0;
	}

Table::Table():
	xConverter(getDefaultTextValue),
	yConverter(getDefaultTextValue)
{
    graphPoints.add(GraphPoint(0.0, 0.0, 0.5));
    graphPoints.add(GraphPoint(1.0, 1.0, 0.5));
}

Table::~Table()
{
}

void Table::setGraphPoints(const Array<GraphPoint> &newGraphPoints, int numPoints, bool refreshLookupTable)
{
    {
        hise::SimpleReadWriteLock::ScopedWriteLock sl(graphPointLock);
        graphPoints.clear();
        graphPoints.addArray(newGraphPoints, 0, numPoints);
    }

	if (refreshLookupTable)
		fillLookUpTable();

	internalUpdater.sendContentChangeMessage(sendNotificationSync, -1);
};

Array<Table::GraphPoint> Table::getCopyOfGraphPoints() const
{
    Array<GraphPoint> copy;
    copy.ensureStorageAllocated(graphPoints.size());
    
    hise::SimpleReadWriteLock::ScopedReadLock sl(graphPointLock);
    
    copy.addArray(graphPoints);
    return copy;
}

var Table::getTablePointsAsVarArray() const
{
    Array<var> a;

    hise::SimpleReadWriteLock::ScopedReadLock sl(graphPointLock);
    
    a.ensureStorageAllocated(graphPoints.size());
    
    for(const auto& gp: graphPoints)
    {
        Array<var> gpa;

        gpa.add(gp.x);
        gpa.add(gp.y);
        gpa.add(gp.curve);
        a.add(var(gpa));
    }

    return a;
}

String Table::exportData() const
{
    Array<GraphPoint> copy;
    
    {
        hise::SimpleReadWriteLock::ScopedReadLock sl(graphPointLock);
        copy.addArray(graphPoints);
    }

    if (copy.size() == 2)
    {
        auto first = copy.getFirst();
        auto second = copy.getLast();
        
        if (first.x == 0.0f && first.y == 0.0f)
        {
            if (second.x == 1.0f && second.y == 1.0f && second.curve == 0.5f)
                return "";
        }
    }
    
	MemoryBlock b(copy.getRawDataPointer(), sizeof(Table::GraphPoint) * copy.size());

	return b.toBase64Encoding();
}

String Table::dataVarToBase64(const var& data)
{
	Array<GraphPoint> gpd;

	if (auto d = data.getArray())
	{
		for (const auto& vgp : *d)
		{
			if (auto a = vgp.getArray())
			{
				gpd.add({ (float)(*a)[0], (float)(*a)[1], (float)(*a)[2] });
			}
		}
	}

	MemoryBlock b(gpd.getRawDataPointer(), sizeof(Table::GraphPoint) * gpd.size());
	return b.toBase64Encoding();
}

var Table::base64ToDataVar(const String& b64)
{
	MemoryBlock b;

	b.fromBase64Encoding(b64);

	if (b.getSize() == 0)
		return var();

	Array<GraphPoint> gpd;
	gpd.insertArray(0, static_cast<const Table::GraphPoint*>(b.getData()), (int)(b.getSize() / sizeof(Table::GraphPoint)));

    
    
	Array<var> data;

	for (auto& gp : gpd)
	{
		Array<var> vgp;
		vgp.add(gp.x);
		vgp.add(gp.y);
		vgp.add(gp.curve);
		data.add(var(vgp));
	}

	return data;
}

bool Table::fromBase64String(const String& b64)
{
	restoreData(b64);
	return true;
}

String Table::toBase64String() const
{
	return exportData();
}

void Table::createPath(Path &normalizedPath, bool fillPath, bool addStartEnd) const
{
	normalizedPath.clear();
	
	normalizedPath.startNewSubPath(0.0f, 0.0f);
	normalizedPath.startNewSubPath(1.0f, 1.0f);
	normalizedPath.startNewSubPath(0.0f, 1.0f);
	normalizedPath.startNewSubPath(1.0f, 0.0f);

    {
        hise::SimpleReadWriteLock::ScopedReadLock sl(graphPointLock);
        
        if (addStartEnd)
        {
            normalizedPath.startNewSubPath(0.0f, 1.0f - jmax(0.0f, startY));
            normalizedPath.lineTo(0.0f, 1.0f - graphPoints[0].y);
        }
        else
        {
            normalizedPath.startNewSubPath(0.0f, 1.0f - graphPoints[0].y);
        }

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
    }
    
	if(addStartEnd)
		normalizedPath.lineTo(1.0f, 1.0f - jmax(0.0f, endY));

	if (fillPath)
		normalizedPath.closeSubPath();
};

void Table::fillLookUpTable()
{
	HeapBlock<float> newValues;
	newValues.calloc(getTableSize());

    {
        hise::SimpleReadWriteLock::ScopedReadLock sl(graphPointLock);
        
        GraphPointComparator gpc;
        graphPoints.sort(gpc);
    }
    
	fillExternalLookupTable(newValues, getTableSize());
	
    FloatVectorOperations::copy(getWritePointer(), newValues, getTableSize());
};

void Table::fillExternalLookupTable(float* d, int numValues)
{
	Path renderPath;
	createPath(renderPath, false, false);
	
	// Allow a error of -60dB for the curve flattening
	PathFlatteningIterator it(renderPath, AffineTransform(), 0.001f);

	for (int i = 0; i < numValues; i++)
	{
		float xIndex = (float)i / (float)(numValues-1);

		Range<float> r(it.x1, it.x2);

		bool isLast = false;

		while (!r.contains(xIndex))
		{
			if (!it.next())
			{
				isLast = true;
				break;
			}

			r = Range<float>(it.x1, it.x2);
		}

		if (isLast)
			d[i] = 1.0f - it.y1;
		else
		{
			Line<float> section(it.x1, it.y1, it.x2, it.y2);

			Line<float> scanLine(xIndex, -0.1f, xIndex, 1.1f);

			auto v = section.getIntersection(scanLine).getY();
			d[i] = 1.0f - v;
		}
	};
}

float *SampleLookupTable::getWritePointer() {return data;};

} // namespace hise
