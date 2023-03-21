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



#define OLD_TABLE_LISTENER 0


/** A table is a data structure that allows editing of a look up table with a TableEditor. It uses a list of graph points to create a path which is rendered to a float 
*	array of the desired size.
*
*	@ingroup ui_types
*/
class Table: public ComplexDataUIBase
{
public:

	struct Listener : private ComplexDataUIUpdaterBase::EventListener
	{
		virtual ~Listener() {};

		virtual void indexChanged(float newIndex) {};

		virtual void graphHasChanged(int point) {};

	private:

		friend class Table;

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var n) override
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
	};

	/** Delays the notification until this object goes out of scope. 
		Use this if you're creating a lot of calls to the add point method
		to avoid unnecessary updates between those calls. */
	struct ScopedUpdateDelayer
	{
		ScopedUpdateDelayer(Table& t) :
			table(t)
		{
			prevValue = t.delayUpdates;
		}

		~ScopedUpdateDelayer()
		{
			table.internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
			table.fillLookUpTable();
			table.delayUpdates = prevValue;
		}

		Table& table;
		bool prevValue = false;
	};

	static String getDefaultTextValue(float input) { return String(roundToInt(input * 100.0f)) + "%"; };

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
	void setGraphPoints(const Array<GraphPoint> &newGraphPoints, int numPoints, bool refreshLookupTable);

	/** Exports the data as base64 encoded String. This is not a ValueTree (so RestorableObject is no base class from Table),
	*	because it needs to be embedded in an XML attribute
	*
	*	@see restoreData()
	*/
	virtual String exportData() const;;

	static String dataVarToBase64(const var& data);
	static var base64ToDataVar(const String& b64);

	bool fromBase64String(const String& b64) override;

	String toBase64String() const override;

	

	/** Restores the data from a base64 encoded String.
	*
	*	@see exportData()
	*/
	virtual void restoreData(const String &savedString)
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
	};

	void setTablePoint(int pointIndex, float x, float y, float curve)
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

	void reset()
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

	void addTablePoint(float x, float y, float curve=0.5f)
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

	/** Returns the number of graph points */
	int getNumGraphPoints() const { return graphPoints.size(); };

    var getTablePointsAsVarArray() const;
    
	/** Get a copy of the graph point at pointIndex. */
	GraphPoint getGraphPoint(int pointIndex) const {return graphPoints.getUnchecked(pointIndex); };

	/** This generates a normalized path from the GraphPoint array.
	*
	*	This is called by the editor to draw the path under the DragPoints.
	*/
	void createPath(Path &normalizedPath, bool fillPath, bool addStartEnd=true) const;

	void setStartAndEndY(float newStartY, float newEndY)
	{
		startY = newStartY;
		endY = newEndY;
		fillLookUpTable();
	}

	/** Fills the look up table with the graph points generated from calculateGraphPoints()
	*
	*	Don't call this too often as it is quite heavy!
	*/
	virtual void fillLookUpTable();

	void fillExternalLookupTable(float* d, int numValues);

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

	void setNormalisedIndexSync(float value)
	{
		internalUpdater.sendDisplayChangeMessage(value, sendNotificationAsync);
	}

	void sendGraphUpdateMessage()
	{
		internalUpdater.sendContentChangeMessage(sendNotificationAsync, -1);
	}

	void addRulerListener(Listener* l)
	{
		internalUpdater.addEventListener(l);
	}

	void removeRulerListener(Listener* l)
	{
		internalUpdater.removeEventListener(l);
	}

    Array<GraphPoint> getCopyOfGraphPoints() const;
    
private:

	bool delayUpdates = false;

	float startY = -1.0f;
	float endY = -1.0f;

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

	Array<GraphPoint> graphPoints;
    mutable hise::SimpleReadWriteLock graphPointLock;

	ValueTextConverter xConverter;
	ValueTextConverter yConverter;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Table);
};


#if 0
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
	float get(int index, NotificationType notifyEditors) const 
	{ 
		if (notifyEditors != dontSendNotification)
			internalUpdater.sendDisplayChangeMessage((float)index / 127.0, notifyEditors);

		return data[index]; 
	};

protected:

	float *getWritePointer() override;

private:

	float data[128];

};
#endif

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
	float getInterpolatedValue(double sampleIndex, NotificationType notifyEditor) const
	{
        if (notifyEditor != dontSendNotification)
			internalUpdater.sendDisplayChangeMessage(sampleIndex, notifyEditor);

        
        sampleIndex *= (double)SAMPLE_LOOKUP_TABLE_SIZE;
        
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

	JUCE_DECLARE_WEAK_REFERENCEABLE(SampleLookupTable);
};


} // namespace hise

#endif  // TABLES_H_INCLUDED
