#ifndef UserStream_H
#define UserStream_H

#include "StreamSerialization.h"
#include <functional>

namespace Antilatency {
	namespace Serialization {
		class UserStreamWriter : public IStreamWriter {
		public:
			using FunctionType = std::function<bool(const uint8_t*, size_t)>;

			explicit UserStreamWriter(FunctionType writeFunction) :
				_writeFunction(writeFunction)
			{
				
			}

			bool write(const uint8_t* buffer, size_t size) override {
				return _writeFunction(buffer, size);
			}
		private:
			FunctionType _writeFunction;
		};

		class UserStreamReader : public IStreamReader {
		public:
			using FunctionType = std::function<bool(uint8_t*, size_t)>;

			explicit UserStreamReader(FunctionType readFunction) :
				_readFunction(readFunction)
			{

			}

			bool read(uint8_t* buffer, size_t size) override {
				return _readFunction(buffer, size);
			}

		private:
			FunctionType _readFunction;
		};
	}
}


#endif // UserStream_H
