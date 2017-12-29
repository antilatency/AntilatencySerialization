#ifndef Structures_H
#define Structures_H

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
			};

			template< typename T >
			struct Single<T, typename T::Name> {
				static T& get(T& value) {
					return value;
				}
			};

			template< typename FirstFieldType, typename RestFieldsType, typename Name>
			struct Multi {
				static auto get(FirstFieldType& firstField, RestFieldsType& restFields) -> decltype(restFields.template get<Name>()) {
					return restFields.template get<Name>();
				}
			};

			template<typename FirstFieldType, typename RestFieldsType>
			struct Multi<FirstFieldType, RestFieldsType, typename FirstFieldType::Name> {
				static FirstFieldType& get(FirstFieldType& firstField, RestFieldsType& restFields) {
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
			
			template<typename Serializer>
			size_t serialize(Serializer& serializer) const {
				serializer.beginStructure();
				auto result = firstField.serialize(serializer) + restFields.serialize(serializer);
				serializer.endStructure();
				return result;
			}

			template<typename Deserializer>
			size_t deserialize(Deserializer& deserializer) {
				return firstField.deserialize(deserializer) + restFields.deserialize(deserializer);
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


			template<typename Serializer>
			size_t serialize(Serializer& serializer) const {
				serializer.beginStructure();
				auto result = firstField.serialize(serializer);
				serializer.endStructure();
				return result;
			}

			template<typename Deserializer>
			size_t deserialize(Deserializer& deserializer) {
				return firstField.deserialize(deserializer);
			}
		};

		template <uint32_t Version_, typename ChildType, typename ... Fields>
		class VersionedStructure : public Structure<Fields...> {
		public:
			virtual ~VersionedStructure() = default;

			using VersionType = Varint32;

			static constexpr VersionType Version = Version_;

			template<typename Serializer>
			size_t serialize(Serializer& serializer) const {	
				serializer.beginStructure();
				auto result = serializer.serialize(Version) + Structure<Fields...>::serialize(serializer);
				serializer.endStructure();
				return result;
			}

			template<typename Deserializer>
			size_t deserialize(Deserializer& deserializer) {
				VersionType version;
				size_t desirializedSize = deserializer.deserialize(version);
				return desirializedSize + deserialize(version, deserializer);
			}

			template<typename Deserializer>
			size_t deserialize(VersionType version, Deserializer& deserializer) {
				if (version == Version) {
					return Structure<Fields...>::deserialize(deserializer);
				}
				else {
					return static_cast<ChildType*>(this)->convert(version, deserializer);
				}
			}
		};
	}
}

#endif // Structures_H
