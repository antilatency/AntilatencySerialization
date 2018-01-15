#ifndef STREAM_SERIALIZATION_H
#define STREAM_SERIALIZATION_H




#include <stdint.h>
#include <stddef.h>

#include <string.h>

namespace Antilatency {
	namespace Serialization {
		
		class IStreamWriter {
		public:
			virtual ~IStreamWriter() = default;
			virtual bool write(const uint8_t* buffer, size_t size) = 0;
		};

		class IStreamReader {
		public:
			virtual ~IStreamReader() = default;
			virtual bool read(uint8_t* buffer, size_t size) = 0;
		};

		class MemoryStreamReader : public IStreamReader {
		public:
			MemoryStreamReader(const uint8_t* buffer, size_t capacity) :
				_buffer(buffer), 
				_capacity(capacity) 
			{
			}



		private:
			bool read(uint8_t* buffer, size_t size) override {
				if (_currentPosition + size <= _capacity) {
					memcpy(buffer, _buffer + _currentPosition, size);
					_currentPosition += size;
					return true;
				}
				return false;
			}

		private:
			const uint8_t* _buffer;
			size_t _capacity;
			size_t _currentPosition = 0;
		};

		class MemoryStreamWriter : public IStreamWriter {
		public:
			MemoryStreamWriter(uint8_t* buffer, size_t capacity) :
				_buffer(buffer),
				_capacity(capacity)
			{
			}

		private:
			bool write(const uint8_t* buffer, size_t size) override {
				if(_currentPosition + size <= _capacity) {
					memcpy(_buffer + _currentPosition, buffer, size);
					_currentPosition += size;
					return true;
				}

				return false;
			}

		private:
			uint8_t* _buffer;
			size_t _capacity;
			size_t _currentPosition = 0;
		};

		class MemorySizeCounterStream final : public IStreamWriter{

		private:
			bool write(const uint8_t* buffer, size_t size) override {
				buffer;
				_totalSize += size;
				return true;
			}
		public:
			size_t getActualSize() const {
				return _totalSize;
			}

		private:
			size_t _totalSize = 0;
		};

	}
}


#endif // STREAM_SERIALIZATION_H