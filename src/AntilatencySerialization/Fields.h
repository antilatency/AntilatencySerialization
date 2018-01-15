#ifndef Fields_H
#define Fields_H

#include <stdint.h>
#include <assert.h>
#include "BaseTypes.h"

namespace Antilatency {
	namespace Serialization {
		
		template <typename FieldName>
		class Field {
		public:
			using Name = FieldName;
		};
		

		template <typename _Type, typename Name>
		class SingleField : public Field<Name> {	
		public:
			using Type = _Type;
		protected:
			Type _value {};
		public:
			const Type& getValue() const {
				return _value;
			}

			Type& getValue() {
				return _value;
			}

			void setValue(const Type& value) {
				_value = value;
			}

			template<typename Serializer>
			size_t serialize(Serializer& serializer) const {
				return serializer.serialize(_value);
			}		

			template<typename Deserializer>
			size_t deserialize(Deserializer& deserializer) {
				return deserializer.deserialize(_value);
			}
		};

	
		template <typename T, typename Name>
		class ContainerField : public Field<Name> {
		public:
			using Type = T;
		protected:
			Type _value {};
		public:

			const Type& getValue() const {
				return _value;
			}

			Type& getValue() {
				return _value;
			}

			void setValue(const Type& value) {
				_value = value;
			}


			template<typename Serializer>
			size_t serialize(Serializer& serializer) const {
				return serializer.serialize(_value);
			}

			template<typename Deserializer>
			size_t deserialize(Deserializer& deserializer) {
				return deserializer.deserialize(_value);
			}
		};

		template <typename Name>
		using Int32Field = SingleField<int32_t, Name>;

		template <typename T, typename Name>
		using VectorField = ContainerField<BaseVectorType<T>, Name>;

		template <typename Name>
		using StringField = ContainerField<BaseStringType, Name>;

		
		template <typename T>
		class OptioinalField : public T {
		public:
			
			void setValue(const typename T::Type& value) {
				T::setValue(value);
				_exists = true;
			}			

			const typename T::Type& getValue() const {
				assert(_exists);
				return T::getValue();
			}

			typename T::Type& getValue() {
				assert(_exists);
				return T::getValue();
			}

			bool isExists() const {
				return _exists;
			}

			template<typename Serializer>
			size_t serialize(Serializer& serializer) const {
				size_t serializedSize = serializer.serialize(_exists);

				if(_exists) {
					serializedSize += T::serialize(serializer);
				}

				return serializedSize;
			}

			template<typename Deserializer>
			size_t deserialize(Deserializer& deserializer) {
				size_t deserializedSize = deserializer.deserialize(_exists);
				if(_exists) {
					deserializedSize += T::deserialize(deserializer);
				}
				return deserializedSize;
			}
		private:
			bool _exists = false;
		};

	}
}

#endif // Fields_H
