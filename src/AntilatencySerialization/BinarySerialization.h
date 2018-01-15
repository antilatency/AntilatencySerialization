#ifndef BinarySerialization_H
#define BinarySerialization_H


#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "Varint.h"
#include "BaseTypes.h"

#include "StreamSerialization.h"

namespace Antilatency {
	namespace Serialization {

#define SERIALIZATION_SERIALIZE_BASE_TYPE(type)  template<> inline bool BinarySerializer::serialize<type>(const type& value) { return write(value); }
#define SERIALIZATION_DESERIALIZE_BASE_TYPE(type)  template<> inline bool BinaryDeserializer::deserialize<type>(type& value) { return read(value); }

#define SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(type) template<> inline bool BinarySerializer::serialize<type>(const BaseVectorType<type>& value) { return serializeNativeContainer<BaseVectorType<type>,type>(value, value.size()); }

#define SERIALIZATION_SERIALIZE_SIGNED_VARINT(type) template<> inline bool BinarySerializer::serialize<type>(const Varint<type>& value) { \
			auto temp = value.getValue();\
			if (temp < 0) {\
				temp = (-temp << 1) - 1;\
			}\
			else {\
				temp <<= 1;\
			}\
			return serialize(Varint<u##type>(temp));\
		}
#define SERIALIZATION_DESERIALIZE_SIGNED_VARINT(type) template<> inline bool BinaryDeserializer::deserialize<type>(Varint<type>& value) { \
			Varint<u##type> temp;\
			if(deserialize(temp)) {\
				u##type unsignedValue = temp.getValue();\
				if (unsignedValue & 1) {\
					value.setValue(-(static_cast<type>(((unsignedValue) >> 1) + 1))); \
				}\
				else {\
					value.setValue(unsignedValue >> 1);\
				}\
				return true;\
			}\
			return false;\
		}

		class BinarySerializer {
		public:

			BinarySerializer(IStreamWriter* writer) : _writer(writer) {
				assert(writer != nullptr);
			}

			~BinarySerializer() {
			}

			void setStreamWriter(IStreamWriter* writer) {
				assert(writer != nullptr);
				_writer = writer;
			}

			void beginStructure() {
				
			}

			void endStructure() {
				
			}

			template<typename T>
			bool serialize(const T& value) {
				return value.serialize(*this);
			}			

			template <typename T>
			bool serialize(const Varint<T>& value) {
				using VariantType = Varint<T>;
				T temp = value.getValue();

	#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
				temp = swapBytes(temp);
	#endif
				size_t i = 0;
				while (temp >= VariantType::base) {
					if(!serialize<uint8_t>((1 << VariantType::usedBits) | (temp & VariantType::mask))) {
						return false;
					}
					temp >>= VariantType::usedBits;
					++i;
				}
				return serialize<uint8_t>(temp & VariantType::mask);
			}

			template <typename T>
			bool serialize(const BaseVectorType<T>& value) {
				return serializeContainer(value, value.size());
			}

		private:
			template<typename T>
			bool write(const T& value) {
		#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
				T temp = swapBytes(value);
		#else
				T temp = value;
		#endif
				return _writer->write(reinterpret_cast<const uint8_t*>(&temp), sizeof(T));
			}

			template<typename T>
			bool serializeContainer(const T &value, size_t containerSize) {
				if(!serialize(Varint64(containerSize))) {
					return false;
				}
				for (size_t i = 0; i < containerSize; ++i) {
					if(!serialize(value[i])) {
						return false;
					}
				}
				return true;
			}


		#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_LITTLE_ENDIAN
			template<typename T, typename ItemType>
			bool serializeNativeContainer(const T &value, size_t containerSize) {
				if(!serialize(Varint64(containerSize))) {
					return false;
				}
				auto itemsSize = sizeof(ItemType) * containerSize;
				if (itemsSize) {
					if(!_writer->write(reinterpret_cast<const uint8_t*>(&value[0]), itemsSize)) {
						return false;
					}
				}
				return true;
			}
		#endif
		private:
			IStreamWriter* _writer;
		};

		SERIALIZATION_SERIALIZE_BASE_TYPE(char)
		SERIALIZATION_SERIALIZE_BASE_TYPE(uint8_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(int8_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(uint16_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(int16_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(uint32_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(int32_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(uint64_t)
		SERIALIZATION_SERIALIZE_BASE_TYPE(int64_t)

#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_LITTLE_ENDIAN
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(uint8_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(int8_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(uint16_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(int16_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(uint32_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(int32_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(uint64_t)
		SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(int64_t)
#endif

		template<>
		inline bool BinarySerializer::serialize<bool>(const bool& value) {
			return serialize(static_cast<uint8_t>(value));
		}

		template <>
		inline bool BinarySerializer::serialize<BaseStringType>(const BaseStringType& value) {
			return serializeContainer(value, value.length());
		}
		
		SERIALIZATION_SERIALIZE_SIGNED_VARINT(int16_t)
		SERIALIZATION_SERIALIZE_SIGNED_VARINT(int32_t)
		SERIALIZATION_SERIALIZE_SIGNED_VARINT(int64_t)
		


		class BinaryDeserializer {
		public:
			BinaryDeserializer(IStreamReader* reader) :
				_reader(reader)
			{
				assert(_reader != nullptr);
			}

			void setStreamReader(IStreamReader* reader) {
				assert(reader != nullptr);
				_reader = reader;
			}

			template<typename T>
			bool deserialize(T& value) {
				return value.deserialize(*this);
			}

			template <typename T>
			bool deserialize(Varint<T>& value) {
				using VariantType = Varint<T>;
				T temp = 0;
				size_t i = 0;
				while (true) {
					uint8_t sym;
					if(!deserialize<uint8_t>(sym)) {
						return false;
					}
					temp |= static_cast<T>(sym & VariantType::mask) << (VariantType::usedBits * i);
					if ((sym & (~VariantType::mask)) == 0) {
						break;
					}
					++i;
				}
	#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
				temp = swapBytes(temp);
	#endif
				value.setValue(temp);
				return true;
			}

			template <typename T>
			bool deserialize(BaseVectorType<T>& value) {
				return deserializeContainer(value);
			}		
			
		private:
			template<typename T>
			bool read(T& value) {
				T temp;
				if (_reader->read(reinterpret_cast<uint8_t *>(&temp), sizeof(T))) {
					#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
						value = swapBytes(temp);
					#else
						value = temp;
					#endif
					return true;
				}
				return false;
			}

			template<typename T>
			bool deserializeContainer(T& value) {
				Varint64 containerSize;
				if(!deserialize(containerSize)) {
					return false;
				}

				#if defined(ARDUINO) //Todo arduino string has resize only
					value.reserve(static_cast<size_t>(containerSize.getValue()));
				#else
					value.resize(static_cast<size_t>(containerSize.getValue()));
				#endif
				for (size_t i = 0; i < containerSize.getValue(); ++i) {
					if(!deserialize(value[i])) {
						return false;
					}
				}
				return true;
			}
		private:
			IStreamReader* _reader;
		};


		SERIALIZATION_DESERIALIZE_BASE_TYPE(char)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(uint8_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(int8_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(uint16_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(int16_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(uint32_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(int32_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(uint64_t)
		SERIALIZATION_DESERIALIZE_BASE_TYPE(int64_t)
		
		//Todo optimized version for deserializing container with primary types inside

		template<>
		inline bool BinaryDeserializer::deserialize<bool>(bool& value) {
			uint8_t temp;
			if(!deserialize(temp)) {
				return false;
			}
			value = static_cast<bool>(temp);
			return true;
		}
		
		template <>
		inline bool BinaryDeserializer::deserialize<BaseStringType>(BaseStringType& value) {
			return deserializeContainer(value);
		}

		SERIALIZATION_DESERIALIZE_SIGNED_VARINT(int16_t)
		SERIALIZATION_DESERIALIZE_SIGNED_VARINT(int32_t)
		SERIALIZATION_DESERIALIZE_SIGNED_VARINT(int64_t)
	}
}

#endif // BinarySerialization_H
