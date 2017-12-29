#ifndef StreamSerialization_H
#define StreamSerialization_H

#include "BaseTypes.h"

#if defined(ANTILATENCY_SERIALIZATION_IOSTREAM_SUPPORT)


#include <type_traits>
#include <iostream>

#include "Varint.h"

namespace Antilatency {
	namespace Serialization {

		class IostreamSerializer {
		public:

			void beginStructure() {
				std::cout << "{";
			}

			void endStructure() {
				std::cout << "}";
			}

			template<typename T>
			std::enable_if_t<!std::is_class<T>::value, size_t> serialize(const T& value) {
				std::cout << value;
				return 0;
			}

			template<typename T>
			std::enable_if_t<std::is_class<T>::value, size_t> serialize(const T& value) {
				value.serialize(*this);
				return 0;
			}

			template <typename T>
			size_t serialize(const Varint<T>& value) {
				std::cout << value.getValue();
				return 0;
			}

			template <typename T>
			size_t serialize(const BaseVectorType<T>& value) {
				serialize('[');
				for (size_t i = 0; i < value.size(); ++i) {
					serialize(value[i]);
					if (i != value.size() - 1) {
						serialize(',');
					}
				}
				serialize(']');
				return 0;
			}
		};

		template<>
		inline size_t IostreamSerializer::serialize(const bool& value) {
			std::cout << std::boolalpha << value;
			return 0;
		}

		template <>
		inline size_t IostreamSerializer::serialize<BaseStringType>(const BaseStringType& value) {
			serialize('\"');
			std::cout << value;
			serialize('\"');
			return 0;
		}
	}
}

#endif

#endif // StreamSerialization_H
