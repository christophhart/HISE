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

#ifndef TABLES_H_INCLUDED
#define TABLES_H_INCLUDED

#include <float.h>

namespace hise { using namespace juce;

/** A table is a data structure that allows editing of a look up table with a TableEditor. It uses a list of graph points to create a path which is rendered to a float 
*	array of the desired size.
*
*	@ingroup ui_types
*/
class Table: public SafeChangeBroadcaster
{
public:

	static String getDefaultTextValue(float input) { return String(roundFloatToInt(input * 100.0f)) + "%"; };

	using ValueTextConverter = std::function<String(float)>;

	/** A Table can use two data types. */
	enum DataType
	{
		Midi = 0, ///< in this mode, the table contains 128 elements and can be used for everything midi-related.
		SampleLookupTable ///< in this mode, the table contains 2048 elements and can be used for holding everything sample-related (waveforms, envelopes, etc.)
	};

	/** Creates a new table of the specified type. */
	Table();

	virtual ~Table();;

	/** Overwrite this and return the table size. */
	virtual int getTableSize() const = 0;

	virtual const float *getReadPointer() const = 0;

	/** A representation of a graph point.
	*	This is used to save the graph
	*/
	struct GraphPoint
	{
		GraphPoint(float x_, float y_, float curve_): x(x_), y(y_), curve(curve_) { };
		
		GraphPoint(const GraphPoint &g): x(g.x), y(g.y), curve(g.curve) { };

		GraphPoint() { jassertfalse; };

		float x; ///< the x position of the point.
		float y; ///< the y position of the point.
		float curve; ///< the curve factor to the left 

	};

	/** Sets the GraphPoints. If you need to refresh the internal table, you also have to call fillLookUpTable(). */
	void setGraphPoints(const Array<GraphPoint> &newGraphPoints, int numPoints);

	/** Exports the data as base64 encoded String. This is not a ValueTree (so RestorableObject is no base class from Table),
	*	because it needs to be embedded in an XML attribute
	*
	*	@see restoreData()
	*/
	virtual String exportData() const
	{
		Array<GraphPoint> copy = Array<GraphPoint>(graphPoints);

		MemoryBlock b(copy.getRawDataPointer(), sizeof(Table::GraphPoint) * copy.size());

		return b.toBase64Encoding();
	};

	/** Restores the data from a base64 encoded String.
	*
	*	@see exportData()
	*/
	virtual void restoreData(const String &savedString)
	{
		MemoryBlock b;
		
		b.fromBase64Encoding(savedString);

		if(b.getSize() == 0) return;

		graphPoints.clear();
		graphPoints.insertArray(0, static_cast<const Table::GraphPoint*>(b.getData()), (int)(b.getSize() / sizeof(Table::GraphPoint)));
		
		fillLookUpTable();
	};

	void setTablePoint(int pointIndex, float x, float y, float curve)
	{
		const float sanitizedX = jlimit(0.0f, 1.0f, x);
		const float sanitizedY = jlimit(0.0f, 1.0f, y);
		const float sanitizedCurve = jlimit(0.0f, 1.0f, curve);

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

		fillLookUpTable();
	}

	void reset()
	{
		graphPoints.clear();
		graphPoints.add(GraphPoint(0.0f, 0.0f, 0.5f));
		graphPoints.add(GraphPoint(1.0f, 1.0f, 0.5f));

		fillLookUpTable();
	}

	void addTablePoint(float x, float y)
	{
		graphPoints.add(GraphPoint(x, y, 0.5f));

		fillLookUpTable();
	}

	/** Returns the number of graph points */
	int getNumGraphPoints() const { return graphPoints.size(); };

	/** Get a copy of the graph point at pointIndex. */
	GraphPoint getGraphPoint(int pointIndex) const {return graphPoints.getUnchecked(pointIndex); };

	/** This generates a normalized path from the GraphPoint array.
	*
	*	This is called by the editor to draw the path under the DragPoints.
	*/
	void createPath(Path &normalizedPath) const;

	/** Fills the look up table with the graph points generated from calculateGraphPoints()
	*
	*	Don't call this too often as it is quite heavy!
	*/
	virtual void fillLookUpTable();

	CriticalSection &getLock()
	{
		return lock;
	};

	/** Overwrite this and return a pointer to the data array. */
	virtual float *getWritePointer() = 0;

	/** This returns a String that can be used for displaying purposes.
	*
	*	You can supply a lambda for the conversion using setTextConverter().
	*/
	String getXValueText(float value)
	{
		if (xConverter)
			return xConverter(value);
		else
			return String(value, 2);
	}

	String getYValueText(float value)
	{
		if (yConverter)
			return yConverter(value);
		else
			return String(value, 2);
	}

	void setXTextConverter(const ValueTextConverter& converter)
	{
		xConverter = converter;
	}

	void setYTextConverterRaw(const ValueTextConverter& converter)
	{
		yConverter = converter;
	}

	ValueTextConverter getYTextConverter()
	{
		return yConverter;
	}

private:

	class GraphPointComparator
	{
	public:
		GraphPointComparator(){};

		static int compareElements(GraphPoint dp1, GraphPoint dp2) 
		{
			if(dp1.x < dp2.x) return -1;
			else if(dp1.x > dp2.x) return 1;
			else return 0;
		};
	};

	WeakReference<Table>::Master masterReference;
	friend class WeakReference<Table>;

	CriticalSection lock;

	Array<GraphPoint> graphPoints;

	ValueTextConverter xConverter;
	ValueTextConverter yConverter;
};


/** A Table subclass that can be used for any 7bit data.
*/
class MidiTable: public Table
{
public:

	MidiTable():
		Table()
	{
		fillLookUpTable();
	};
	

	const float *getReadPointer() const override {return data;};

	int getTableSize() const override {return 128;};

	/** Allows access to the lookup table*/
	inline float get(int index) const {return data[index]; };

	

protected:

	float *getWritePointer() override;

private:

	float data[128];

};

#define SAMPLE_LOOKUP_TABLE_SIZE 512

/** A Table subclass that contains sample data with the fixed size of 512. */
class SampleLookupTable: public Table
{
public:

	SampleLookupTable():
		coefficient(1.0),
        sampleLength(-1)
	{
		fillLookUpTable();
	};

	int getTableSize() const override {return SAMPLE_LOOKUP_TABLE_SIZE;};

	const float *getReadPointer() const override {return data;};

	/** Sets a sample amount which will be the sample length of the Table.
	*
	*	The internal table size will still be 512, but it allows the getValueForSample() method to accept sample data.
	*/
	void setLengthInSamples(double newSampleLength)
	{
		sampleLength = (int)newSampleLength;

		if (newSampleLength == 0.0)
		{
			coefficient = 0.0;
		}
		else
		{
			coefficient = ((double)SAMPLE_LOOKUP_TABLE_SIZE / (double)sampleLength);
		}
	};

	float getFirstValue() const
	{
		return data[0];
	};

	float getLastValue() const
	{
		return data[SAMPLE_LOOKUP_TABLE_SIZE - 1];
	};

	int getLengthInSamples() const
	{
		jassert(sampleLength != -1);
		return sampleLength;
	};

	/** Returns the interpolated value.
	*
	*	@param sampleIndex the sample index from 0 to SAMPLE_LOOKUP_TABLE_SIZE (default 512). Doesn't need to be an integer, of course.
	*	@returns the value of the table between 0.0 and 1.0
	*/
	float getInterpolatedValue(double sampleIndex) const
	{
		const double indexInTable = coefficient * sampleIndex;

		if(indexInTable >= (double)(SAMPLE_LOOKUP_TABLE_SIZE - 1)) return getLastValue();

		const int iLow = (int) indexInTable;
		const int iHigh = iLow + 1;

		jassert(iHigh < SAMPLE_LOOKUP_TABLE_SIZE);

		const float delta = (float)indexInTable - (float)iLow;

		const float value = Interpolator::interpolateLinear(data[iLow], data[iHigh], delta);

		return value;
		
	};

	
protected:

	float *getWritePointer() override;


private:

	double coefficient;

	float data[SAMPLE_LOOKUP_TABLE_SIZE];

	int sampleLength;



};



class DiscreteTable: public Table
{
public:

	DiscreteTable():
		Table()
	{
		for(int i = 0; i < 128; i++)
		{

			data[i] = 1.0f;
		}
	};
	
	/** Allows the setting of a value directly. */
	void setValue(int index, float newValue)
	{
		data[index] = newValue;
	}

	String exportData() const override
	{
		MemoryBlock b(data, sizeof(float) * 128);

		return b.toBase64Encoding();
	};

	virtual void restoreData(const String &savedString) override
	{
		MemoryBlock b;

		b.fromBase64Encoding(savedString);

		if (b.getSize() == 0) return;

		const float *savedData = static_cast<const float*>(b.getData());

		const int savedValues = (int)(b.getSize() / sizeof(float));
		jassert(savedValues == 128);

		for(int i = 0; i < savedValues; i++)
		{
			data[i] = savedData[i];
		}
	};

	const float *getReadPointer() const override {return data;};

	int getTableSize() const override {return 128;};

	/** Allows access to the lookup table*/
	inline float get(int index) const {return data[index]; };



	float *getWritePointer() override {return data;};

private:

	float data[128];

};

} // namespace hise

#endif  // TABLES_H_INCLUDED
