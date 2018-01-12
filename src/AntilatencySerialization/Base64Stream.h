#ifndef Base64Stream_H
#define Base64Stream_H

#include <stdint.h>
#include <stddef.h>

#include "StreamSerialization.h"

namespace Antilatency {
	namespace Serialization {

		namespace Base64 {
			static constexpr char LookupTableToChar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			static constexpr size_t UsedBits = 6;
			static constexpr size_t LeftBits = 8 - UsedBits;
			static constexpr uint8_t Mask = (1 << UsedBits) - 1;

			static constexpr char PaddingSymbol = '=';

			static constexpr uint8_t getPositionFromChar(char sym, bool& isOk) {
				isOk = true;
				if (sym >= 'A' && sym <= 'Z') {
					return sym - 'A' + 0;
				}
				if (sym >= 'a' && sym <= 'z') {
					return sym - 'a' + 26;
				}
				if (sym >= '0' && sym <= '9') {
					return sym - '0' + 52;
				}
				if (sym == '+') {
					return 62;
				}
				if (sym == '/') {
					return 63;
				}
				isOk = false;
				return 0;
			}

			
		}

		class Base64StreamReader final : public IStreamReader {
		public:
			explicit Base64StreamReader(IStreamReader* reader) :
				_reader(reader)
			{

			}

			size_t read(uint8_t* buffer, size_t size) override {
				size_t read = 0;
				for(size_t i = 0; i < size; ++i) {
					uint8_t sym;
					auto readBytes = getSym(sym);
					read += readBytes;
					buffer[i] = sym;
					
					if (_stopStream) {
						return read;
					}
				}
				return read;
			}
		private:
			size_t getSym(uint8_t& sym) {
				uint8_t firstSym;
				uint8_t nextSym;
				size_t read = 0;
				switch (_currentPosition % 3) {
				case 0:
					read += getRealByte(firstSym);
					read += getRealByte(nextSym);
					_savedValue = nextSym;
					sym = (firstSym << 2) | ((nextSym >> 4) & 0x3);
					break;
				case 1:
					read += getRealByte(firstSym);
					sym = (_savedValue << 4) | ((firstSym >> 2) & 0x0F);
					_savedValue = firstSym;
					break;
				case 2:
					read += getRealByte(firstSym);
					sym = (_savedValue << 6) | (firstSym);
					_savedValue = 0;
					break;
				default:
					assert(false);
				}
				if (!_stopStream) {
					++_currentPosition;
				}
				return read;
			}

			size_t getRealByte(uint8_t& byte) {
				auto size = _reader->read(&byte, sizeof(uint8_t));
				if ( size == sizeof(uint8_t)) {
					bool isOk;
					byte = Base64::getPositionFromChar(byte, isOk);
					if(!isOk) {
						_stopStream = true;
					}
				}
				return size;
			}

		private:
			uint8_t _savedValue = 0;
			size_t _currentPosition = 0;
			bool _stopStream = false;
			IStreamReader* _reader;
		};


		class Base64StreamWriter final : public IStreamWriter {
		public:
			explicit Base64StreamWriter(IStreamWriter* writer) :
				_writer(writer)
			{

			}
		
			size_t flush() {
				size_t flushedSize = 0;
				if(_isSaved) {
					flushedSize += processStage(0, true);
				}
				while (_currentPosition % 3 != 0) {
					flushedSize += _writer->write(reinterpret_cast<const uint8_t*>(&Base64::PaddingSymbol), sizeof(char));
					++_currentPosition;
				}
				return flushedSize;
			}
			size_t write(const uint8_t* buffer, size_t size) override {
				size_t writen = 0;
				for(size_t i = 0; i < size; ++i) {
					writen += processStage(buffer[i]);
				}
				return writen;
			}

		private:
			size_t processStage(uint8_t value, bool isFlush = false) {
				size_t writen = 0;
				uint8_t index;
				switch (_currentPosition % 3) {
				case 0:
					index = (value & 0xfc) >> 2;
					writen += _writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t));
					_savedValue = value;
					_isSaved = true;
					break;
				case 1:
					index = ((_savedValue & 0x03) << 4) + ((value & 0xf0) >> 4);
					writen += _writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t));
					_savedValue = value;
					_isSaved = true;
					break;
				case 2:
					index = ((_savedValue & 0x0f) << 2) + ((value & 0xc0) >> 6);
					writen += _writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t));
					if (!isFlush) {
						index = value & 0x3f;
						writen += _writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t));
					}
					_savedValue = 0;
					_isSaved = false;
					break;
				default:
					assert(false);
				}
				if (!isFlush) {
					++_currentPosition;
				}
				return writen;
			}

		private:
			size_t _currentPosition = 0;
			uint8_t _savedValue = 0;
			bool _isSaved = 0;
			IStreamWriter* _writer;
		};
	}
}

#endif // Base64Stream_H


