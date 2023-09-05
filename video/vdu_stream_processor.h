#ifndef VDU_STREAM_PROCESSOR_H
#define VDU_STREAM_PROCESSOR_H

#include <memory>
#include <Stream.h>

#include "agon.h"

class VDUStreamProcessor {
	public:
		VDUStreamProcessor(Stream * stream) : stream(stream) {}
		inline bool byteAvailable() {
			return stream->available() > 0;
		}
		inline uint8_t readByte() {
			return stream->read();
		}
		inline void writeByte(uint8_t b) {
			stream->write(b);
		}
		void send_packet(uint8_t code, uint16_t len, uint8_t data[]);

		void processAllAvailable();
		void processNext();

		void vdu(uint8_t c);

		void wait_eZ80();
		void sendModeInformation();
	private:
		Stream * stream;

		int16_t readByte_t(uint16_t timeout);
		int32_t readWord_t(uint16_t timeout);
		int32_t read24_t(uint16_t timeout);
		uint8_t readByte_b();
		uint32_t readLong_b();
		void discardBytes(uint32_t length);

		void vdu_colour();
		void vdu_gcol();
		void vdu_palette();
		void vdu_mode();
		void vdu_graphicsViewport();
		void vdu_plot();
		void vdu_resetViewports();
		void vdu_textViewport();
		void vdu_origin();
		void vdu_cursorTab();

		void vdu_sys();
		void vdu_sys_video();
		void sendGeneralPoll();
		void vdu_sys_video_kblayout();
		void sendCursorPosition();
		void sendScreenChar(uint16_t x, uint16_t y);
		void sendScreenPixel(uint16_t x, uint16_t y);
		void sendTime();
		void vdu_sys_video_time();
		void sendKeyboardState();
		void vdu_sys_keystate();
		void vdu_sys_scroll();
		void vdu_sys_cursorBehaviour();
		void vdu_sys_udg(char c);

		void vdu_sys_audio();
		void sendAudioStatus(uint8_t channel, uint8_t status);
		uint8_t loadSample(uint8_t sampleIndex, uint32_t length);
		void setVolumeEnvelope(uint8_t channel, uint8_t type);
		void setFrequencyEnvelope(uint8_t channel, uint8_t type);

		void vdu_sys_sprites(void);
		void receiveBitmap(uint8_t cmd, uint16_t width, uint16_t height);

		void vdu_sys_buffered();
		void bufferWrite(uint16_t bufferId);
		void bufferCall(uint16_t bufferId);
		void bufferClear(uint16_t bufferId);
		void bufferAdjust(uint16_t bufferId);
};

// Read an unsigned byte from the serial port, with a timeout
// Returns:
// - Byte value (0 to 255) if value read, otherwise -1
//
int16_t VDUStreamProcessor::readByte_t(uint16_t timeout = COMMS_TIMEOUT) {
	auto t = millis();

	while (millis() - t <= timeout) {
		if (byteAvailable()) {
			return readByte();
		}
	}
	return -1;
}

// Read an unsigned word from the serial port, with a timeout
// Returns:
// - Word value (0 to 65535) if 2 bytes read, otherwise -1
//
int32_t VDUStreamProcessor::readWord_t(uint16_t timeout = COMMS_TIMEOUT) {
	auto l = readByte_t(timeout);
	if (l >= 0) {
		auto h = readByte_t(timeout);
		if (h >= 0) {
			return (h << 8) | l;
		}
	}
	return -1;
}

// Read an unsigned 24-bit value from the serial port, with a timeout
// Returns:
// - Value (0 to 16777215) if 3 bytes read, otherwise -1
//
int32_t VDUStreamProcessor::read24_t(uint16_t timeout = COMMS_TIMEOUT) {
	auto l = readByte_t(timeout);
	if (l >= 0) {
		auto m = readByte_t(timeout);
		if (m >= 0) {
			auto h = readByte_t(timeout);
			if (h >= 0) {
				return (h << 16) | (m << 8) | l;
			}
		}
	}
	return -1;
}

// Read an unsigned byte from the serial port (blocking)
//
uint8_t VDUStreamProcessor::readByte_b() {
	while (stream->available() == 0);
	return readByte();
}

// Read an unsigned word from the serial port (blocking)
//
uint32_t VDUStreamProcessor::readLong_b() {
  uint32_t temp;
  temp  =  readByte_b();		// LSB;
  temp |= (readByte_b() << 8);
  temp |= (readByte_b() << 16);
  temp |= (readByte_b() << 24);
  return temp;
}

// Discard a given number of bytes from input stream
//
void VDUStreamProcessor::discardBytes(uint32_t length) {
	while (length > 0) {
		readByte_t(0);
		length--;
	}
}

// Send a packet of data to the MOS
//
void VDUStreamProcessor::send_packet(uint8_t code, uint16_t len, uint8_t data[]) {
	writeByte(code + 0x80);
	writeByte(len);
	for (int i = 0; i < len; i++) {
		writeByte(data[i]);
	}
}

// Process all available commands from the stream
//
void VDUStreamProcessor::processAllAvailable() {
	while (byteAvailable()) {
		vdu(readByte());
	}
}

// Process next command from the stream
//
void VDUStreamProcessor::processNext() {
	if (byteAvailable()) {
		vdu(readByte());
	}
}

#include "vdu.h"

#endif // VDU_STREAM_PROCESSOR_H
