#ifndef ROUTING_H_INCLUDED
#define ROUTING_H_INCLUDED


/** A processor that uses multiple audio channels can be subclassed from this class to allow handling of the routing matrix. */
class RoutableProcessor
{
public:

	RoutableProcessor();

	virtual ~RoutableProcessor() {};

	class MatrixData : public RestorableObject,
					   public SafeChangeBroadcaster
	{
	public:

		MatrixData(RoutableProcessor *p);

		void clearAllConnections();

		bool resizingIsAllowed() const noexcept{ return resizeAllowed; };

		void setAllowResizing(bool shouldAllowResizing) noexcept{ resizeAllowed = shouldAllowResizing; }

		bool isUsed(int sourceChannel) const noexcept;;

		bool toggleEnabling(int sourceChannel) noexcept;
		bool toggleSendEnabling(int sourceChannel) noexcept;


		bool toggleConnection(int sourceChannel, int destinationChannel) noexcept;;
		bool addConnection(int sourceChannel, int destinationChannel) noexcept;
		bool removeConnection(int sourceChannel, int destinationChannel) noexcept;

		bool toggleSendConnection(int sourceChannel, int destinationChannel) noexcept;;
		bool addSendConnection(int sourceChannel, int destinationChannel) noexcept;
		bool removeSendConnection(int sourceChannel, int destinationChannel) noexcept;


		bool onlyEnablingAllowed() const noexcept { return allowEnablingOnly; }

		int getConnectionForSourceChannel(int sourceChannel) const noexcept;
		int getSendForSourceChannel(int sourceChannel) const noexcept;

		void resetToDefault()
		{
			for (int i = 0; i < NUM_MAX_CHANNELS; i++)
			{
				channelConnections[i] = -1;
				sendConnections[i] = -1;
			}

			channelConnections[0] = 0;
			channelConnections[1] = 1;
		}

		ValueTree exportAsValueTree() const override;
		void restoreFromValueTree(const ValueTree &v) override;

		void setNumSourceChannels(int newNumChannels, NotificationType notifyProcessors=sendNotification);
		void setNumDestinationChannels(int newNumChannels, NotificationType notifyProcessors = sendNotification);

		CriticalSection &getLock();

		int getNumSourceChannels() const { return numSourceChannels; }
		int getNumDestinationChannels() const { return numDestinationChannels; };

		void setOnlyEnablingAllowed(bool noRouting) { allowEnablingOnly = noRouting; }
		void setNumAllowedConnections(int newNum) noexcept { numAllowedConnections = newNum; };

		bool isEditorShown() const noexcept { return editorShown; }
		void setEditorShown(bool isShown) noexcept { editorShown = isShown; }

		void setTargetProcessor(Processor *p);
		String getTargetName() const { return targetProcessor.get() != nullptr ? targetProcessor.get()->getId() : "HISE Output"; }
		String getSourceName() const { return dynamic_cast<Processor*>(owningProcessor)->getId(); }

		void setGainValues(float *numMaxChannelValues, bool isSourceValue);
		float getGainValue(int channelIndex, bool getSourceValue) const noexcept { return getSourceValue ? sourceGainValues[channelIndex] : targetGainValues[channelIndex]; };

	protected:

		int numAllowedConnections;
		int numSourceChannels;
		int numDestinationChannels;

	private:

		void refreshSourceUseStates();

		friend class RoutableProcessor;

		WeakReference<Processor> targetProcessor;
		RoutableProcessor *owningProcessor;

		bool resizeAllowed;
		bool allowEnablingOnly;
		bool editorShown;

		float sourceGainValues[NUM_MAX_CHANNELS];
		float targetGainValues[NUM_MAX_CHANNELS];

		int channelConnections[NUM_MAX_CHANNELS];

		int sendConnections[NUM_MAX_CHANNELS];
	};

	virtual void numSourceChannelsChanged() = 0;

	virtual void numDestinationChannelsChanged() = 0;

	/** Opens a routing editor in the quasi modal popup. Pass in any component that is a child component of the backend window. */
	void editRouting(Component *childComponent);

	

	const MatrixData &getMatrix() const { return data; };

	MatrixData &getMatrix() { return data; };

	/** Quick way to get the left channel for processing. */
	int getLeftSourceChannel() const { return leftSourceChannel; };

	/** Quick way to get the right channel for processing. */
	int getRightSourceChannel() const  { return rightSourceChannel; };

	/** Quick way to get the left target channel. */
	int getLeftDestinationChannel() const  { return leftTargetChannel; };

	/** Quick way to get the right target channel. */
	int getRightDestinationChannel() const  { return leftTargetChannel; };

	

	MatrixData data;

protected:



private:

	int leftSourceChannel;
	int rightSourceChannel;
	int leftTargetChannel;
	int rightTargetChannel;
};




#endif  // ROUTING_H_INCLUDED
