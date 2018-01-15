#ifndef Structures_H
#define Structures_H

#include <stdint.h>
#include <stddef.h>
#include "Varint.h"


namespace Antilatency {
	namespace Serialization {

#define SERIALIZATION_MAKE_FIELD_NAME(name) class name {public: static constexpr auto FieldName = #name;} 
		namespace detail {
			template< typename T, typename Name>
			struct Single {
				static T& get(T& value) {
					static_assert(sizeof(Name) != sizeof(Name), "Field is not supported");
					return value;
				}

				static const T& get(const T& value) {
					static_assert(sizeof(Name) != sizeof(Name), "Field is not supported");
					return value;
				}
			};

			template< typename T >
			struct Single<T, typename T::Name> {
				static T& get(T& value) {
					return value;
				}

				static const T& get(const T& value) {
					return value;
				}
			};

			template< typename FirstFieldType, typename RestFieldsType, typename Name>
			struct Multi {
				static auto get(FirstFieldType& firstField, RestFieldsType& restFields) -> decltype(restFields.template get<Name>()) {
					firstField;
					return restFields.template get<Name>();
				}

				static auto get(const FirstFieldType& firstField, const RestFieldsType& restFields) -> decltype(restFields.template get<Name>()) {
					firstField;
					return restFields.template get<Name>();
				}
			};

			template<typename FirstFieldType, typename RestFieldsType>
			struct Multi<FirstFieldType, RestFieldsType, typename FirstFieldType::Name> {

				static FirstFieldType& get(FirstFieldType& firstField, RestFieldsType& restFields) {
					restFields;
					return firstField;
				}

				static const FirstFieldType& get(const FirstFieldType& firstField, const RestFieldsType& restFields) {
					restFields;
					return firstField;
				}
			};

		}

		template <typename FirstField, typename ... Fields>
		class Structure {
			FirstField firstField;
			Structure<Fields...> restFields;
		public:
			template<typename Name>
			auto get() -> decltype(detail::Multi<FirstField, Structure<Fields...>, Name>::get(firstField, restFields)) {
				return detail::Multi<FirstField, Structure<Fields...>, Name>::get(firstField, restFields);
			}

			template<typename Name>
			auto get() const -> decltype(detail::Multi<FirstField, Structure<Fields...>, Name>::get(firstField, restFields)) {
				return detail::Multi<FirstField, Structure<Fields...>, Name>::get(firstField, restFields);
			}
		
			template<typename Serializer>
			bool serialize(Serializer& serializer) const {
				serializer.beginStructure();
				if(firstField.serialize(serializer)) {
					if(restFields.serialize(serializer)) {
						serializer.endStructure();
						return true;
					}
				}		
				return false;
			}

			template<typename Deserializer>
			bool deserialize(Deserializer& deserializer) {
				if(firstField.deserialize(deserializer)) {
					return restFields.deserialize(deserializer);
				}
				return false;
			}
		};		

		template <typename FirstField>
		class Structure<FirstField> {
			FirstField firstField;
		public:
			template<typename Name>
			FirstField& get() {
				return detail::Single<FirstField, Name>::get(firstField);
			}

			template<typename Name>
			const FirstField& get() const {
				return detail::Single<FirstField, Name>::get(firstField);
			}


			template<typename Serializer>
			bool serialize(Serializer& serializer) const {
				serializer.beginStructure();
				if (firstField.serialize(serializer)) {
					serializer.endStructure();
					return true;
				}
				return false;
			}

			template<typename Deserializer>
			bool deserialize(Deserializer& deserializer) {
				return firstField.deserialize(deserializer);
			}
		};

		template <uint64_t Version_, typename ChildType, typename ... Fields>
		class VersionedStructure : public Structure<Fields...> {
		public:

			using VersionType = Varint64;

			static constexpr VersionType Version { Version_ };

			template<typename Serializer>
			bool serialize(Serializer& serializer) const {	
				serializer.beginStructure();
				if(serializer.serialize(Version)) {
					if(Structure<Fields...>::serialize(serializer)) {
						serializer.endStructure();
						return true;
					}
				}		
				return false;
			}

			template<typename Deserializer>
			bool deserialize(Deserializer& deserializer) {
				VersionType version;
				if(deserializer.deserialize(version)) {
					return deserialize(version, deserializer);
				}
				return false;
			}

			template<typename Deserializer>
			bool deserialize(VersionType version, Deserializer& deserializer) {
				if (version == Version) {
					return Structure<Fields...>::deserialize(deserializer);
				}
				else {
					return reinterpret_cast<ChildType*>(this)->convertFromPreviousVersion(version, deserializer);
				}
			}
		};
	}
}

#endif // Structures_H
