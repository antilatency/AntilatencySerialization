#ifndef Varint_H
#define Varint_H

#include <stdint.h>

namespace Antilatency {
	namespace Serialization {

		template<typename BaseType>
		struct Varint {
			using Type = BaseType;
		
			constexpr Varint() = default;

			constexpr Varint(const Type& value) : 
				_value(value) 
			{			
			}

			Varint& operator=(const Varint& other) = default;

			const Type& getValue() const {
				return _value;
			}
		
			Type& getRef() {
				return _value;
			}
		
			void setValue(const Type& value) {
				_value = value;
			}

			size_t getActualSize() const {
				Type value = _value;
				size_t size = 1;
				while (value >= base) {
					++size;
					value >>= usedBits;
				}
				return size;
			}
					
			bool operator==(const Varint<Type>& rhs) const {
				return rhs._value == _value;
			}

			bool operator!=(const Varint<Type>& rhs) const {
				return !operator==(rhs);
			}

			bool operator==(Type value) const {
				return _value == value;
			}

			bool operator!=(Type value) const {
				return !operator==(value);
			}

			operator Type() const {
				return _value;
			}

			static constexpr size_t usedBits = 7;
			static constexpr size_t base = 1 << usedBits;
			static constexpr size_t mask = base - 1;


		private:
			Type _value = 0;
		};

		using Varint32 = Varint<uint32_t>;
		using Varint64 = Varint<uint64_t>;
	}
}

#endif // Varint_H
