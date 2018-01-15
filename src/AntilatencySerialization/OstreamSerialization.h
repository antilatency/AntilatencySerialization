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
			std::enable_if_t<!std::is_class<T>::value, bool> serialize(const T& value) {
				_stream << value;
				return true;
			}

			template<typename T>
			std::enable_if_t<std::is_class<T>::value, bool> serialize(const T& value) {
				value.serialize(*this);
				return true;
			}

			template <typename T>
			bool serialize(const Varint<T>& value) {
				_stream << value.getValue();
				return true;
			}

			template <typename T>
			bool serialize(const BaseVectorType<T>& value) {
				serialize('[');
				for (size_t i = 0; i < value.size(); ++i) {
					serialize(value[i]);
					if (i != value.size() - 1) {
						serialize(',');
					}
				}
				serialize(']');
				return true;
			}
		private:
			std::ostream& _stream;
		};

		template<>
		inline bool OstreamSerializer::serialize(const bool& value) {
			_stream << std::boolalpha << value;
			return true;
		}

		template <>
		inline bool OstreamSerializer::serialize<BaseStringType>(const BaseStringType& value) {
			serialize('\"');
			_stream << value;
			serialize('\"');
			return true;
		}
	}
}

#endif // OstreamSerialization_H

