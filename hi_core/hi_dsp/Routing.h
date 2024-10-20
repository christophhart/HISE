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

#ifndef ROUTING_H_INCLUDED
#define ROUTING_H_INCLUDED

namespace hise { using namespace juce;

#define RESTORE_MATRIX() {getMatrix().restoreFromValueTree(v.getChildWithName("RoutingMatrix"));}

/** A Processor with flexible audio routing
*	@ingroup processor_interfaces
*
*	A processor that uses multiple audio channels can be subclassed from this class to allow handling of the routing matrix. */
class RoutableProcessor
{
public:

	enum class Presets
	{
		AllChannels = 10000,
		FirstStereo,
		SecondStereo,
		ThirdStereo,
		AllChannelsToStereo
	};

    static constexpr float SilenceThreshold = -90.f;
    
	RoutableProcessor();

	virtual ~RoutableProcessor();;

	class MatrixData : public RestorableObject,
					   public SafeChangeBroadcaster
	{
	public:

        void setDecayCoefficients(float newUpDecayFactor, float newDownDecayFactor);

        MatrixData(RoutableProcessor *p);

		~MatrixData();

		void clearAllConnections();
		bool resizingIsAllowed() const noexcept;;
		void setAllowResizing(bool shouldAllowResizing) noexcept;
        bool isUsed(int sourceChannel) const noexcept;;

        float getGainValue(int channelIndex, bool getSource) const;
        
		bool toggleEnabling(int sourceChannel) noexcept;
		bool toggleSendEnabling(int sourceChannel) noexcept;
		bool toggleConnection(int sourceChannel, int destinationChannel) noexcept;;
		bool addConnection(int sourceChannel, int destinationChannel) noexcept;
		bool removeConnection(int sourceChannel, int destinationChannel) noexcept;

		bool toggleSendConnection(int sourceChannel, int destinationChannel) noexcept;;
		bool addSendConnection(int sourceChannel, int destinationChannel) noexcept;
		bool removeSendConnection(int sourceChannel, int destinationChannel) noexcept;

		bool onlyEnablingAllowed() const noexcept;

        int getNumAllowedConnections() const;

        int getConnectionForSourceChannel(int sourceChannel) const noexcept;
		int getSendForSourceChannel(int sourceChannel) const noexcept;

		void resetToDefault();

		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;

		void setNumSourceChannels(int newNumChannels, NotificationType notifyProcessors=sendNotification);
		void setNumDestinationChannels(int newNumChannels, NotificationType notifyProcessors = sendNotification);

		void handleDisplayValues(const AudioSampleBuffer& input, const AudioSampleBuffer& output, bool useOutput);

		SimpleReadWriteLock& getLock() const;

		int getNumSourceChannels() const;
        int getNumDestinationChannels() const;;

		void setOnlyEnablingAllowed(bool noRouting);
        void setNumAllowedConnections(int newNum) noexcept;;

		bool isEditorShown(int channelIndex) const noexcept;

        bool anyChannelActive() const noexcept;

        void setEditorShown(Array<int> channelIndexes, bool isShown) noexcept;

        void setTargetProcessor(Processor *p);
		String getTargetName() const;

        String getSourceName() const;

        void setGainValues(float *numMaxChannelValues, bool isSourceValue);
		

		void init(Processor* pTouse=nullptr);

        bool isProcessorMatrix() const;

        void loadPreset(Presets newPreset);

	protected:

		int numAllowedConnections;
		int numSourceChannels;
		int numDestinationChannels;

	private:

		bool anyActive = false;

        float upDecayFactor = 1.0f;
        float downDecayFactor = 0.97f;
        
		void refreshSourceUseStates();

		friend class RoutableProcessor;
		mutable SimpleReadWriteLock lock;

		WeakReference<Processor> targetProcessor;
		RoutableProcessor *owningProcessor;
		Processor *thisAsProcessor;

		bool resizeAllowed;
		bool allowEnablingOnly;

        int numEditors[NUM_MAX_CHANNELS];

		float sourceGainValues[NUM_MAX_CHANNELS];
		float targetGainValues[NUM_MAX_CHANNELS];
        int channelConnections[NUM_MAX_CHANNELS];
		int sendConnections[NUM_MAX_CHANNELS];

		JUCE_DECLARE_WEAK_REFERENCEABLE(MatrixData);
	};

	virtual void numSourceChannelsChanged() = 0;

	virtual void numDestinationChannelsChanged() = 0;

	virtual void connectionChanged();;

	/** Opens a routing editor in the quasi modal popup. Pass in any component that is a child component of the backend window. */
	void editRouting(Component *childComponent);

	const MatrixData &getMatrix() const;;

	MatrixData &getMatrix();;

	/** Quick way to get the left channel for processing. */
	int getLeftSourceChannel() const;;

	/** Quick way to get the right channel for processing. */
	int getRightSourceChannel() const;;

	/** Quick way to get the left target channel. */
	int getLeftDestinationChannel() const;;

	/** Quick way to get the right target channel. */
	int getRightDestinationChannel() const;;

	MatrixData data;

protected:

private:

	int leftSourceChannel;
	int rightSourceChannel;
	int leftTargetChannel;
	int rightTargetChannel;

	JUCE_DECLARE_WEAK_REFERENCEABLE(RoutableProcessor);
};


} // namespace hise

#endif  // ROUTING_H_INCLUDED
