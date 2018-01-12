#ifndef STREAM_SERIALIZATION_H
#define STREAM_SERIALIZATION_H




#include <stdint.h>
#include <stddef.h>

#include <assert.h>
#include <string.h>

namespace Antilatency {
	namespace Serialization {
		
		class IStreamWriter {
		public:
			virtual ~IStreamWriter() = default;
			virtual size_t write(const uint8_t* buffer, size_t size) = 0;
		};

		class IStreamReader {
		public:
			virtual ~IStreamReader() = default;
			virtual size_t read(uint8_t* buffer, size_t size) = 0;
		};

		class MemoryStreamReader : public IStreamReader {
		public:
			MemoryStreamReader(const uint8_t* buffer, size_t capacity) :
				_buffer(buffer), 
				_capacity(capacity) 
			{
			}

			void setMemory(const uint8_t* buffer, size_t capacity) {
				_buffer = buffer;
				_capacity = capacity;
				_currentPosition = 0;
			}

			size_t read(uint8_t* buffer, size_t size) override {
				assert(_currentPosition + size <= _capacity);
				memcpy(buffer, _buffer + _currentPosition, size);
				_currentPosition += size;
				return size;
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

			size_t write(const uint8_t* buffer, size_t size) override {
				assert(_currentPosition + size <= _capacity);
				memcpy(_buffer + _currentPosition, buffer, size);
				_currentPosition += size;
				return size;
			}

		private:
			uint8_t* _buffer;
			size_t _capacity;
			size_t _currentPosition = 0;
		};

		class MemorySizeCounterStream final : public IStreamWriter{
		public:
			size_t write(const uint8_t* buffer, size_t size) override {
				buffer;
				_totalSize += size;
				return size;
			}

			size_t getActualSize() const {
				return _totalSize;
			}

		private:
			size_t _totalSize = 0;
		};

	}
}


#endif // STREAM_SERIALIZATION_H