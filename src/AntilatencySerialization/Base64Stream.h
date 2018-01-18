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

        namespace Base64UrlConverterSymbols {
            static constexpr char ProhibitedSymbols[] = {'+', '/'};
            static constexpr char PermittedSymbols[] = {'-', '_'};
            static constexpr size_t SymbolsCount = sizeof(ProhibitedSymbols);
            static_assert(sizeof(ProhibitedSymbols) == sizeof(PermittedSymbols), "Wrong table in base64 url");
        }

		class Base64StreamReader final : public IStreamReader {
		public:
            explicit Base64StreamReader(IStreamReader* reader) : _reader(reader){

            }

		private:
            bool read(uint8_t* buffer, size_t size) override{
                for(size_t i = 0; i < size; ++i) {
                    uint8_t sym;
                    getSym(sym);
                    buffer[i] = sym;

                    if (_stopStream) {
                        return false;
                    }
                }
                return true;
            }
		
            void getSym(uint8_t& sym){
                uint8_t firstSym;
                uint8_t nextSym;

                switch (_currentPosition % 3) {
                case 0:
                    getRealByte(firstSym);
                    getRealByte(nextSym);
                    _savedValue = nextSym;
                    sym = (firstSym << 2) | ((nextSym >> 4) & 0x3);
                    break;
                case 1:
                    getRealByte(firstSym);
                    sym = (_savedValue << 4) | ((firstSym >> 2) & 0x0F);
                    _savedValue = firstSym;
                    break;
                case 2:
                    getRealByte(firstSym);
                    sym = (_savedValue << 6) | (firstSym);
                    _savedValue = 0;
                    break;
                default:
                    assert(false);
                }
                if (!_stopStream) {
                    ++_currentPosition;
                }
            }

            void getRealByte(uint8_t& byte){
                if (_reader->read(&byte, sizeof(uint8_t))) {
                    bool isOk;
                    byte = Base64::getPositionFromChar(byte, isOk);
                    if(!isOk) {
                        _stopStream = true;
                    }
                } else {
                    _stopStream = true;
                }
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
		
			bool flush() {
				if(_isSaved) {
					if(!processStage(0, true)) {
						return false;
					}
				}
				while (_currentPosition % 3 != 0) {
					if(!_writer->write(reinterpret_cast<const uint8_t*>(&Base64::PaddingSymbol), sizeof(char))) {
						return false;
					}
					++_currentPosition;
				}
				return true;
			}
			
		private:
			bool write(const uint8_t* buffer, size_t size) override {
				for (size_t i = 0; i < size; ++i) {
					if(!processStage(buffer[i])) {
						return false;
					}
				}
				return true;
			}

			bool processStage(uint8_t value, bool isFlush = false) {
				uint8_t index;
				switch (_currentPosition % 3) {
				case 0:
					index = (value & 0xfc) >> 2;
					if(!_writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t))) {
						return false;
					}
					_savedValue = value;
					_isSaved = true;
					break;
				case 1:
					index = ((_savedValue & 0x03) << 4) + ((value & 0xf0) >> 4);
					if(!_writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t))) {
						return false;
					}
					_savedValue = value;
					_isSaved = true;
					break;
				case 2:
					index = ((_savedValue & 0x0f) << 2) + ((value & 0xc0) >> 6);
					if(!_writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t))) {
						return false;
					}
					if (!isFlush) {
						index = value & 0x3f;
						if(!_writer->write(reinterpret_cast<const uint8_t*>(&Base64::LookupTableToChar[index]), sizeof(uint8_t))) {
							return false;
						}
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
				return true;
			}

		private:
			size_t _currentPosition = 0;
			uint8_t _savedValue = 0;
			bool _isSaved = 0;
			IStreamWriter* _writer;
		};

        class Base64UrlConverterReader final : public IStreamReader {
        public:
            explicit Base64UrlConverterReader(IStreamReader* reader) :
                    _reader(reader)
            {

            }
        private:
            bool read(uint8_t* buffer, size_t size) override {
                for(size_t i = 0; i < size; ++i) {

                    uint8_t val;
                    auto result = _reader->read(&val, sizeof(uint8_t));
                    if(!result){
                        return false;
                    }

                    auto convertStatus = false;
                    for(size_t j = 0; j < Base64UrlConverterSymbols::SymbolsCount; j++) {
                        if(val == static_cast<uint8_t>(Base64UrlConverterSymbols::PermittedSymbols[j])){
                            buffer[i] = static_cast<uint8_t>(Base64UrlConverterSymbols::ProhibitedSymbols[j]);
                            convertStatus = true;
                        }
                    }
                    if(!convertStatus) {
                        buffer[i] = val;
                    }
                }
                return true;
            }

            IStreamReader* _reader;
        };

        class Base64UrlConverterWriter final : public IStreamWriter {
        public:
            explicit Base64UrlConverterWriter(IStreamWriter* writer) :
                    _writer(writer)
            {

            }
        private:
            bool write(const uint8_t* buffer, size_t size) override {

                for (size_t i = 0; i < size; ++i) {
                    if(buffer[i] != '=') {
                        auto result = false;
                        auto convertStatus = false;
                        for(size_t j = 0; j < Base64UrlConverterSymbols::SymbolsCount; j++){
                            if(buffer[i] == static_cast<uint8_t>(Base64UrlConverterSymbols::ProhibitedSymbols[j])){
                                result = _writer->write(reinterpret_cast<const uint8_t*>(&Base64UrlConverterSymbols::PermittedSymbols[j]), sizeof(uint8_t));
                                convertStatus = true;
                            }
                        }
                        if(!convertStatus){
                            result = _writer->write(reinterpret_cast<const uint8_t*>(&buffer[i]), sizeof(uint8_t));
                        }
                        if(!result){
                            return result;
                        }
                    }
                }
                return true;
            }
            IStreamWriter* _writer;
        };
    }
}

#endif // Base64Stream_H


