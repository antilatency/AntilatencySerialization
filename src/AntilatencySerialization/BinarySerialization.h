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

#define SERIALIZATION_SERIALIZE_BASE_TYPE(type)  template<> inline size_t BinarySerializer::serialize<type>(const type& value) { return write(value); }
#define SERIALIZATION_DESERIALIZE_BASE_TYPE(type)  template<> inline size_t BinaryDeserializer::deserialize<type>(type& value) { return read(value); }

#define SERIALIZATION_SERIALIZE_CONTAINER_BASE_TYPE(type) template<> inline size_t BinarySerializer::serialize<type>(const BaseVectorType<type>& value) { return serializeNativeContainer<BaseVectorType<type>,type>(value, value.size()); }

#define SERIALIZATION_SERIALIZE_SIGNED_VARINT(type) template<> inline size_t BinarySerializer::serialize<type>(const Varint<type>& value) { \
			auto temp = value.getValue();\
			if (temp < 0) {\
				temp = (-temp << 1) - 1;\
			}\
			else {\
				temp <<= 1;\
			}\
			return serialize(Varint<u##type>(temp));\
		}
#define SERIALIZATION_DESERIALIZE_SIGNED_VARINT(type) template<> inline size_t BinaryDeserializer::deserialize<type>(Varint<type>& value) { \
			Varint<u##type> temp;\
			size_t size = deserialize(temp);\
			u##type unsignedValue = temp.getValue();\
			if (unsignedValue & 1) {\
				value = -(static_cast<type>(((unsignedValue) >> 1) + 1)); \
			}\
			else {\
				value = unsignedValue >> 1;\
			}\
			return size;\
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
			size_t serialize(const T& value) {
				auto serializedSize = value.serialize(*this);
				return serializedSize;
			}			

			template <typename T>
			size_t serialize(const Varint<T>& value) {
				using VariantType = Varint<T>;
				T temp = value.getValue();

	#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
				temp = swapBytes(temp);
	#endif
				size_t writtenSize = 0;
				size_t i = 0;
				while (temp >= VariantType::base) {
					writtenSize += serialize<uint8_t>((1 << VariantType::usedBits) | (temp & VariantType::mask));
					temp >>= VariantType::usedBits;
					++i;
				}
				writtenSize += serialize<uint8_t>(temp & VariantType::mask);
				return writtenSize;
			}

			template <typename T>
			size_t serialize(const BaseVectorType<T>& value) {
				return serializeContainer(value, value.size());
			}

		private:
			template<typename T>
			size_t write(const T& value) {
		#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
				T temp = swapBytes(value);
		#else
				T temp = value;
		#endif
                                return _writer->write(reinterpret_cast<const uint8_t*>(&temp), sizeof(T));
			}

			template<typename T>
			size_t serializeContainer(const T &value, size_t containerSize) {
				auto written = serialize(Varint64(containerSize));
				for (size_t i = 0; i < containerSize; ++i) {
					written += serialize(value[i]);
				}
				return written;
			}


		#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_LITTLE_ENDIAN
			template<typename T, typename ItemType>
			size_t serializeNativeContainer(const T &value, size_t containerSize) {
				auto written = serialize(Varint64(containerSize));
				auto itemsSize = sizeof(ItemType) * containerSize;
				if (itemsSize) {
					written += _writer->write(reinterpret_cast<const uint8_t*>(&value[0]), itemsSize);
				}
				return written;
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
		inline size_t BinarySerializer::serialize<bool>(const bool& value) {
			return serialize(static_cast<uint8_t>(value));
		}

		template <>
		inline size_t BinarySerializer::serialize<BaseStringType>(const BaseStringType& value) {
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
			size_t deserialize(T& value) {
				auto deserializedSize = value.deserialize(*this);
				return deserializedSize;
			}

			template <typename T>
			size_t deserialize(Varint<T>& value) {
				using VariantType = Varint<T>;
				T temp = 0;
				size_t i = 0;
				size_t readSize = 0;
				while (true) {
					uint8_t sym;
					readSize += deserialize<uint8_t>(sym);
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
				return readSize;
			}

			template <typename T>
			size_t deserialize(BaseVectorType<T>& value) {
				return deserializeContainer(value);
			}		
			
		private:
			template<typename T>
			size_t read(T& value) {
				T temp;
				auto size = _reader->read(reinterpret_cast<uint8_t *>(&temp), sizeof(T));
	#if SERIALIZATION_BYTE_ORDER == SERIALIZATION_BIG_ENDIAN
				value = swapBytes(temp);
	#else
				value = temp;
	#endif
				return size;
			}

			template<typename T>
			size_t deserializeContainer(T &value) {
				Varint64 containerSize;
				auto deserializedSize = deserialize(containerSize);
				#if defined(ARDUINO) //Todo arduino string has resize only
					value.reserve(static_cast<size_t>(containerSize.getValue()));
				#else
					value.resize(static_cast<size_t>(containerSize.getValue()));
				#endif
				for (size_t i = 0; i < containerSize; ++i) {
					deserializedSize += deserialize(value[i]);
				}
				return deserializedSize;
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
		inline size_t BinaryDeserializer::deserialize<bool>(bool& value) {
			uint8_t temp;
			size_t size = deserialize<uint8_t>(temp);
			value = static_cast<bool>(temp);
			return size;
		}

		template <>
		inline size_t BinaryDeserializer::deserialize<BaseStringType>(BaseStringType& value) {
			return deserializeContainer(value);
		}

		SERIALIZATION_DESERIALIZE_SIGNED_VARINT(int16_t)
		SERIALIZATION_DESERIALIZE_SIGNED_VARINT(int32_t)
		SERIALIZATION_DESERIALIZE_SIGNED_VARINT(int64_t)
	}
}

#endif // BinarySerialization_H
