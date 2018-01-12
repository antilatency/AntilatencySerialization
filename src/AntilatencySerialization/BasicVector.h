#ifndef BasicVector_H
#define BasicVector_H

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

namespace Antilatency {
	namespace Serialization {

		template<typename T>
		class BasicVector {
		public:
			~BasicVector() {
				delete[] _data;
			}

			void resize(size_t size) {
				T * newData = new T[size];
				auto copySize = size > _size ? _size : size;
				for (size_t i = 0; i < copySize; ++i) {
					newData[i] = _data[i];
				}
				_size = size;
				_capacity = size;
				delete[] _data;
				_data = newData;
			}

			void reserve(size_t size) {
				_size = 0;
				resize(size);
			}

			size_t size() const {
				return _size;
			}

			T& operator[](size_t index) {
				assert(index < _size);
				return _data[index];
			}

			const T& operator[](size_t index) const {
				assert(index < _size);
				return _data[index];
			}

			BasicVector<T>& operator= (const BasicVector<T>& other) {
				if (this != &other) {
					delete[] _data;
					_size = other._size;
					_capacity = other._capacity;
					_data = new T[_capacity];
					for (size_t i = 0; i < _size; ++i) {
						_data[i] = other._data[i];
					}
				}
				return *this;
			}

			void push_back(const T& value) {
				if(_capacity == 0) {
					resize(minCapacity);
				}
				if (_size >= _capacity) {
					resize(_capacity + _capacity / 2);//*1.5
				}
				_data[_size] = value;
				++_size;
			}

			T& back() {
				return _data[_size - 1];
			}

			T* data() {
				return _data;
			}

			const T* data() const {
				return _data;
			}

			void pop_back()	{
				--_size;
			}
			
			void clear() {
				delete[] _data;
				_size = 0;
				_capacity = 0;
			}

		private:
			size_t _capacity = 0;
			size_t _size = 0;
			T* _data = nullptr;
			static constexpr size_t minCapacity = 4;
		};
		
	}
}

#endif // BasicVector_H
