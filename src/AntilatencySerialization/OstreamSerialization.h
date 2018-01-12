#ifndef OstreamSerialization_H
#define OstreamSerialization_H

#include "BaseTypes.h"


#include <type_traits>
#include <ostream>

#include "Varint.h"

namespace Antilatency {
	namespace Serialization {

		class OstreamSerializer {
		public:
			explicit OstreamSerializer(std::ostream& stream) :
				_stream(stream)
			{
				
			}
			void beginStructure() {
				_stream << "{";
			}

			void endStructure() {
				_stream << "}";
			}

			template<typename T>
			std::enable_if_t<!std::is_class<T>::value, size_t> serialize(const T& value) {
				_stream << value;
				return 0;
			}

			template<typename T>
			std::enable_if_t<std::is_class<T>::value, size_t> serialize(const T& value) {
				value.serialize(*this);
				return 0;
			}

			template <typename T>
			size_t serialize(const Varint<T>& value) {
				_stream << value.getValue();
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
		private:
			std::ostream& _stream;
		};

		template<>
		inline size_t OstreamSerializer::serialize(const bool& value) {
			std::cout << std::boolalpha << value;
			return 0;
		}

		template <>
		inline size_t OstreamSerializer::serialize<BaseStringType>(const BaseStringType& value) {
			serialize('\"');
			std::cout << value;
			serialize('\"');
			return 0;
		}
	}
}

#endif // OstreamSerialization_H

