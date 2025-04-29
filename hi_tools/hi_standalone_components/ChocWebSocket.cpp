namespace hise
{
using namespace juce;


struct WebViewData::TCPServer::Helpers
{
	static std::string btoa(void* data, size_t numBytes)
	{
		static const std::string b64Characters =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";

		auto bytesToEncode = reinterpret_cast<const uint8*>(data);

		std::string ret;
		ret.reserve(numBytes);
		int i = 0;
		int j = 0;
		uint8 d1[3];
		uint8 d2[4];

		while (numBytes--) 
		{
			d1[i++] = *(bytesToEncode++);
			if (i == 3) 
			{
				d2[0] = (d1[0] & 0xfc) >> 2;
				d2[1] = ((d1[0] & 0x03) << 4) + ((d1[1] & 0xf0) >> 4);
				d2[2] = ((d1[1] & 0x0f) << 2) + ((d1[2] & 0xc0) >> 6);
				d2[3] = d1[2] & 0x3f;

				for(i = 0; (i <4) ; i++)
					ret += b64Characters[d2[i]];

				i = 0;
			}
		}

		if (i)
		{
			for(j = i; j < 3; j++)
				d1[j] = '\0';

			d2[0] = (d1[0] & 0xfc) >> 2;
			d2[1] = ((d1[0] & 0x03) << 4) + ((d1[1] & 0xf0) >> 4);
			d2[2] = ((d1[1] & 0x0f) << 2) + ((d1[2] & 0xc0) >> 6);
			d2[3] = d1[2] & 0x3f;

			for (j = 0; (j < i + 1); j++)
				ret += b64Characters[d2[j]];

			while((i++ < 3))
				ret += '=';
		}

		return ret;
	}

	static bool performHandshake(juce::StreamingSocket* socket)
	{
	    char buffer[4096] = {0};

		auto ok = socket->waitUntilReady(true, 3000);

	    int bytesRead = socket->read(buffer, sizeof(buffer), false);

	    if (!ok || bytesRead <= 0) 
			return false;

	    juce::String request(buffer);
		
	    DBG("Received WebSocket handshake request:\n" + request);

	    // Extract Sec-WebSocket-Key
	    juce::String key;
	    for (const auto& line : juce::StringArray::fromLines(request))
	    {
	        if (line.startsWith("Sec-WebSocket-Key: "))
	        {
	            key = line.fromFirstOccurrenceOf("Sec-WebSocket-Key: ", false, true).trim();
	            break;
	        }
	    }
	    if (key.isEmpty()) return false;

		juce::String keyWithGUID = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	    // 2. Compute SHA-1 hash
	    sha1::SHA1 sha1;
	    sha1.processBytes(keyWithGUID.toRawUTF8(), keyWithGUID.getNumBytesAsUTF8());

	    sha1::SHA1::digest8_t digest;
	    sha1.getDigestBytes(digest);  // Raw SHA-1 hash (20 bytes)
		

	    auto x = btoa(digest, sizeof(digest));

	    // Send handshake response
	    juce::String response =
	        "HTTP/1.1 101 Switching Protocols\r\n"
	        "Upgrade: websocket\r\n"
	        "Connection: Upgrade\r\n"
	        "Sec-WebSocket-Accept: " + String(x) + "\r\n\r\n";
	    return socket->write(response.toRawUTF8(), response.length()) == response.length();
	}
};

WebViewData::TCPServer::TCPServer(WebViewData& parent_, int port):
	parent(parent_),
	serverPort(port != -1 ? port : createRandomPort()),
	connectionThread(*this),
	communicationThread(*this)
{}

WebViewData::TCPServer::~TCPServer()
{
	stop(1000);
}

void WebViewData::TCPServer::start()
{
	if (!serverSocket.createListener(serverPort, "127.0.0.1"))
	{
		parent.errorLogger("Failed to create server on port " + juce::String(serverPort));
		return;
	}

	communicationThread.startThread(5);
	connectionThread.startThread(5);
}

void WebViewData::TCPServer::stop(int timeout)
{
	serverSocket.close();

	for(auto oc: openConnections)
		oc->close();

	openConnections.clear();

	connectionThread.stopThread(timeout);
	communicationThread.stopThread(timeout);
}

void WebViewData::TCPServer::sendData(const Identifier& id, const void* data, size_t numBytes)
{
	sendInternal(new Data(id, data, numBytes));
	
}

void WebViewData::TCPServer::sendString(const Identifier& id, const String& message)
{
	sendInternal(new Data(id, message));
}

void WebViewData::TCPServer::addBuffer(uint8 bufferIndex, VariantBuffer::Ptr buffer)
{
	for(auto b: buffers)
	{
		if(bufferIndex == b->index)
		{
			b->buffer = buffer;
			b->initialSize = jmin(b->initialSize, buffer->size);
			b->dirty = true;
			return;
		}
	}

	ScopedLock sl(connectionLock);
	buffers.add(new BufferSlot(bufferIndex, buffer));
}

void WebViewData::TCPServer::updateBuffer(uint8 bufferIndex)
{
	for(auto b: buffers)
	{
		if(b->index == bufferIndex)
		{
			b->dirty = true;
			connectionThread.notify();
		}
	}
}

void WebViewData::TCPServer::sendInternal(Data::Ptr data)
{
	ScopedLock sl(dataLock);

	int i = 0;
	for(auto& ed: queue)
	{
		if(ed->getId() == data->getId())
		{
			queue.set(i, data);
			return;
		}

		i++;
	}

	queue.add(data);
	communicationThread.notify();
}

WebViewData::TCPServer::ConnectionThread::ConnectionThread(TCPServer& parent_):
	Thread("TCP Connection Thread"),
	parent(parent_)
{}

void WebViewData::TCPServer::ConnectionThread::run()
{
	while (!threadShouldExit())
	{
		ScopedPointer<StreamingSocket> newConnection = parent.serverSocket.waitForNextConnection();

		if(newConnection == nullptr)
			continue;

		parent.parent.errorLogger("Client connected!");

		if (Helpers::performHandshake(newConnection))
		{
			parent.parent.errorLogger("WebSocket handshake successful.");

			ScopedLock (parent.connectionLock);
			parent.openConnections.add(newConnection.release());
		}
	}
}

WebViewData::TCPServer::CommunicationThread::CommunicationThread(TCPServer& parent_):
	Thread("TCP Message Thread"),
	parent(parent_)
{}

class WebSocketParser {
public:

	WebSocketParser():
		r(Result::ok())
	{};

    // Function to parse WebSocket frame and handle data based on opcode
    size_t parseFrame(const uint8* frame, size_t numBytes) {

		size_t numParsed = 0;

		
        size_t offset = 0;

        // Extract the first byte (FIN, RSV, Opcode)
        uint8_t firstByte = frame[offset++];
        uint8_t opcode = firstByte & 0x0F;  // Opcode (e.g., 0x1 for text, 0x2 for binary)

        // Extract the second byte (Mask bit, Payload length)
        uint8_t secondByte = frame[offset++];
        bool mask = secondByte & 0x80;  // Mask bit
        size_t payloadLength = secondByte & 0x7F;  // Payload length

        // If the payload length is 126 or 127, read the extended length
        if (payloadLength == 126)
        {
            payloadLength = (frame[offset++] << 8);
            payloadLength |= frame[offset++];
        } else if (payloadLength == 127) {
            payloadLength = 0; // 64-bit length (optional for normal use)
        }

        // Extract the masking key if Mask bit is set
        uint32_t maskingKey = 0;
        if (mask) {
            maskingKey = (frame[offset++] << 24);
            maskingKey |= (frame[offset++] << 16);
            maskingKey |= (frame[offset++] << 8);
            maskingKey |= frame[offset++];
        }

		numParsed = offset + payloadLength;

        // Extract the payload data
        std::vector<uint8_t> payload(frame + offset, frame + offset + payloadLength);
        
        // Unmask the payload if the mask bit was set
        if (mask) {
            for (size_t i = 0; i < payload.size(); ++i) {
                payload[i] ^= (maskingKey >> ((3 - (i % 4)) * 8)) & 0xFF;
            }
        }

        // Handle the payload based on the opcode
        if (opcode == 0x1) {
            // Text message (String)
            message = std::string(payload.begin(), payload.end());
            
        } else if (opcode == 0x2) {

			int numValues = (int)payload.size() / 4;

			buffer = new VariantBuffer(numValues);

            // Binary message (Assumed to be Float32Array)
            std::vector<float> floatData;
            for (size_t i = 0; i < payload.size(); i += 4) {
                float value;
				std::memcpy(&value, &payload[i], sizeof(float));
				buffer->setSample((int)i / 4, value);
            }
			
        } else {
            r = Result::fail("Unsupported opcode: " + String(opcode));
        }

		return numParsed;
    }

	Result getError() const { return r; }

	var getData() const
    {
	    if(r.wasOk())
	    {
			if(buffer != nullptr)
				return var(buffer.get());
			else
			{
				if(message[0] == '{')
				{
					return JSON::parse(String(message));
				}
				else
				{
					return var(String(message));
				}
			}
	    }

		return var();
    }

private:

	Result r;
	std::string message;
	
	VariantBuffer::Ptr buffer;
};

void WebViewData::TCPServer::CommunicationThread::run()
{
	// Continuously send data
	while (!threadShouldExit())
	{
		if(parent.openConnections.isEmpty())
		{
			Thread::sleep(25);
			continue;
		}

		for(int i = 0; i < parent.openConnections.size(); i++)
		{
			uint8 buffer[1024];
			parent.openConnections[i]->waitUntilReady(true, 5);
			int bytesRead = parent.openConnections[i]->read(buffer, sizeof(buffer), false);

			MemoryOutputStream mos;

			while(bytesRead > 0)
			{
				if(buffer[0] == 0x88) 
				{
					parent.parent.errorLogger("Connection closed by client");
					ScopedLock (parent.connectionLock);
					parent.openConnections.remove(i--);
					bytesRead = 0;
					break;
				}
				else
				{
					mos.write(buffer, bytesRead);
				}

				parent.openConnections[i]->waitUntilReady(true, 5);
				bytesRead = parent.openConnections[i]->read(buffer, sizeof(buffer), false);
			}

			if(mos.getDataSize() > 0 && parent.dataCallback)
			{
				WebSocketParser p;

				auto numParsed = 0;

				Array<var> values;

				while(numParsed < mos.getDataSize())
				{
					if(threadShouldExit())
						break;

					numParsed += (int)p.parseFrame((uint8*)mos.getData() + numParsed, mos.getDataSize() - numParsed);

					auto ok = p.getError();

					if(!ok.wasOk())
					{
						if(parent.parent.errorLogger)
							parent.parent.errorLogger(ok.getErrorMessage());

						break;
					}
					else
					{
						values.add(p.getData());
					}
				}

				for(const auto& v: values)
					parent.dataCallback(v);
			}
		}

		Data::List thisQueue;

		{
			ScopedLock sl2(parent.dataLock);

			if(!parent.queue.isEmpty())
				thisQueue.swapWith(parent.queue);
		}

		{
			ScopedLock sl(parent.connectionLock);

			for(auto c: parent.openConnections)
			{
				for(auto b: parent.buffers)
					b->update(c);
			}
		}

		if(!thisQueue.isEmpty())
		{
			ScopedLock sl(parent.connectionLock);

			for(int i = 0; i < parent.openConnections.size(); i++)
			{
				for(auto d: thisQueue)
					d->send(parent.openConnections[i]);
			}
		}

		Thread::sleep(5);
	}
}

WebViewData::TCPServer::Data::Data(const Identifier& id_, const String& message):
  id(id_)
{
	append(true, message.toRawUTF8(), message.length());
}

WebViewData::TCPServer::Data::Data(const Identifier& id_, const void* data, size_t size):
  id(id_)
{
	append(false, data, size);
}

void WebViewData::TCPServer::Data::append(bool isString, const void* data, size_t size)
{
	// Payload format:
	// 1 byte: `0x01` if it's a string, `0x02` if it's a Float32Array
	// 2 bytes: length of the ID + 1
	// X bytes: the ID as UTF8 string
	// 4 bytes: length of the data as uint32
	// X bytes: the data

	size_t headerLength = 0;

	// always send as binary stream
	uint8 binaryHeader = 0x82;
	mos.write(&binaryHeader, 1);
	headerLength++;

	auto idLength = (uint16)id.toString().length() + 1;

	size_t totalLength = sizeof(uint8)     // opcode
					   + sizeof(uint16)    // length of ID
					   + (size_t)idLength  // ID
					   + sizeof(uint32)    // length of data
					   + size;             // the data

	

	if (totalLength <= 125)
	{
		uint8_t payloadLen = static_cast<uint8_t>(totalLength);
		mos.write(&payloadLen, 1);
		headerLength++;
	}
	else if (totalLength <= 65535)
	{
		uint8_t payloadLen = 126; // Extended length 16-bit
		mos.write(&payloadLen, 1);
		headerLength++;
		uint16_t len = static_cast<uint16_t>(totalLength);
		mos.write(&len, sizeof(uint16_t));
		headerLength += sizeof(uint16_t);
	}
	else
	{
		uint8_t payloadLen = 127; // Extended length 64-bit
		mos.write(&payloadLen, 1);
		headerLength++;
		uint64_t len = static_cast<uint64>(totalLength);
		mos.write(&len, sizeof(uint64_t));
		headerLength += sizeof(sizeof(uint64_t));
	}

	uint8_t dataOpcode = isString ? 0x01 : 0x02;
	mos.writeByte(dataOpcode);

	mos.write(&idLength, 2);
	mos.writeString(id.toString());

	auto s = (uint32)size;

	mos.write(&s, sizeof(uint32));

	dataOffset = mos.getPosition();
	mos.write(data, size);
	mos.flush();

	jassert(mos.getDataSize() == (totalLength + headerLength));
}

void WebViewData::TCPServer::BufferSlot::update(StreamingSocket* socket)
{
	if(dirty)
	{
		auto dst = reinterpret_cast<float*>
			(data->getDataStart());

		FloatVectorOperations::copy(dst, buffer->buffer.getReadPointer(0), initialSize);
		data->send(socket);
		dirty = false;
	}
}

bool WebViewData::TCPServer::Data::send(StreamingSocket* socket)
{
	return socket->write(mos.getData(), (int)mos.getDataSize()) == (int)mos.getDataSize();
}

}
