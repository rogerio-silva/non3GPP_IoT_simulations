Trace Sources
=============

Various trace sources can be used to keep track of events throughout the
simulation, mainly regarding the lifetime of a packet. At the PHY layer, the
following trace sources are exposed:

- In ``LoraPhy`` (both ``EndDeviceLoraPhy`` and ``GatewayLoraPhy``):

    - ``StartSending``, fired when a PHY layer begins transmitting a packet;
    - ``PhyRxBegin``, fired when a PHY layer becomes locked on a packet;
    - ``PhyRxEnd``, fired when a PHY's reception of a packet ends;
    - ``ReceivedPacket``, fired when a packet is correctly received;
    - ``LostPacketBecauseInterference``, fired when a packet is lost because of
      interference from other transmissions;
    - ``LostPacketBecauseUnderSensitivity``, fired when a PHY cannot lock on a
      packet because it's being received with a power below the device sensitivity;

- In ``EndDeviceLoraPhy``:

    - ``LoraPacketBecauseWrongFrequency`` is fired when an incoming packet is
      using a frequency that is different from that on which the PHY is listening;
    - ``LoraPacketBecauseWrongSpreadingFactor`` is fired when an incoming packet
      is using a SF that is different from that for which the PHY is listening;
    - ``EndDeviceState`` is used to keep track of the state of the device's PHY
      layer.

- In ``GatewayLoraPhy``:

    - ``LostPacketBecauseNoMoreReceivers`` is fired when a packet is lost because
      no more receive paths are available to lock onto the incoming packet;
    - ``OccupiedReceptionPaths`` is used to keep track of the number of occupied
      reception paths out of the 8 that are available at the gateway;

- In ``LorawanMac`` (both ``EndDeviceLorawanMac`` and ``GatewayLorawanMac``):

    - ``CannotSendBecauseDutyCycle`` is used to keep track of the number of when a
      packet coming from the application layer cannot be sent on any of the
      available channels because of duty cycle limitations;

- In ``EndDeviceLorawanMac``:

    - ``DataRate`` keeps track of the data rate that is employed by the device;
    - ``LastKnownLinkMargin`` keeps track of the last link margin of this device's
      uplink transmissions; This information is gathered through the ``LinkCheck``
      MAC commands;
    - ``LastKnownGatewayCount`` keeps track of the last known number of gateways
      that this device is able to reach; This information is gathered through the
      ``LinkCheck`` MAC commands;
    - ``AggregatedDutyCycle`` keeps track of the currently set aggregated duty
      cycle limitations;

- ``PacketSent`` in ``LoraChannel`` is fired when a packet is sent on the channel;
