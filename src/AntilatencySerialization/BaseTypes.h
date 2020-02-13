#ifndef BaseTypes_H
#define BaseTypes_H


#define SERIALIZATION_LITTLE_ENDIAN 1
#define SERIALIZATION_BIG_ENDIAN 2

#if (1 & 0xFFFFFFFF) == 0x00000001
	#define SERIALIZATION_BYTE_ORDER SERIALIZATION_LITTLE_ENDIAN
#elif (1 & 0xFFFFFFFF) == 0x01000000
	#define SERIALIZATION_BYTE_ORDER SERIALIZATION_BIG_ENDIAN
#else
#error "Wrong endian"
#endif


#include <stdint.h>
#include <stddef.h>

#if defined(ARDUINO)
	#include "BasicVector.h"
#else
	#define ANTILATENCY_SERIALIZATION_STL_SUPPORT
	#include <vector>
	#include <string>
#endif

namespace Antilatency {
	namespace Serialization {

	template<typename T>
	T swapBytes(const T& value) {
		union {
			T convertedValue;
			uint8_t bytes[sizeof(T)];
		};
		convertedValue = value;

		for(size_t i = 0; i < sizeof(T); ++i) {
			//std::swap is not accesable
			uint8_t temp = bytes[i];
			bytes[i] = bytes[sizeof(T) - i - 1];
			bytes[sizeof(T) - i - 1] = temp;
		}
		return convertedValue;
	}

	//Todo Specialization for base types: uint16_t, int16_t, uint32_t ...

	#if defined(ARDUINO)
		template<typename T>
		using BaseVectorType = BasicVector<T>;
		using BaseStringType = String;
	#elif defined(ANTILATENCY_SERIALIZATION_STL_SUPPORT)
		template<typename T>
		using BaseVectorType = std::vector<T>;
		using BaseStringType = std::string;
	#endif

	}
}

#endif // BaseTypes_H
