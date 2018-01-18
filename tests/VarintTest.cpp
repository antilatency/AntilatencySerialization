#include "stdafx.h"
#include "CppUnitTest.h"
#include <array>

#include <ctime>
#include <limits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "AntilatencySerialization/Varint.h"
#include "AntilatencySerialization/BinarySerialization.h"

using namespace Antilatency::Serialization;

namespace SerializationTest
{	
	TEST_CLASS(VarintTest)
	{
		TEST_CLASS_INITIALIZE(Init) {
			srand(static_cast<unsigned>(time(nullptr)));
		}

	public:
		template<typename VarintType>
		static void testValue(typename VarintType::Type value) {
			uint8_t buffer[32];
			MemoryStreamWriter writer(buffer, sizeof(buffer));
			BinarySerializer serializer(&writer);
			VarintType source{ value };
			Assert::IsTrue(serializer.serialize(source));
			VarintType dest;
			MemoryStreamReader reader(buffer, sizeof(buffer));
			BinaryDeserializer deserializer(&reader);
			deserializer.deserialize(dest);

			//AreEqual has't method for int64_t 
			if(source.getValue() != value) {
				Assert::Fail();
			}
			if (dest.getValue() != value) {
				Assert::Fail();
			}
		}

		TEST_METHOD(Zero) {
			testValue<Varint32>(0);
			testValue<Varint64>(0);
		}

		template<typename VarintType>
		static void testSize(size_t pow) {
			VarintType value{ static_cast<typename VarintType::Type>(128) << (pow * 7) };
			Assert::AreEqual(static_cast<size_t>(pow + 2), value.getActualSize());
			value = VarintType(value.getValue() - 1);
			Assert::AreEqual(static_cast<size_t>(pow + 1), value.getActualSize());
		}

		TEST_METHOD(Size) {
			for (size_t i = 0; i < sizeof(uint32_t); ++i) {
				testSize<Varint32>(i);
			}

			for (size_t i = 0; i < sizeof(uint64_t); ++i) {
				testSize<Varint64>(i);
			}
		}

		TEST_METHOD(RandomValue) {
			for(size_t i = 0; i < 10000; ++i) {
				testValue<Varint32>(std::rand());
				testValue<Varint64>((static_cast<uint64_t>(std::rand()) << 32) | std::rand());
			}
		}

		TEST_METHOD(IncValue) {
			for (size_t i = 0; i < 10000; ++i) {
				testValue<Varint32>(static_cast<Varint32::Type>(i));
				testValue<Varint64>((static_cast<uint64_t>(i) << 32) | i);
				testValue<Varint64>(i);
			}
		}

		TEST_METHOD(NegativeValue) {
			for (int32_t i = -10000; i < 10000; ++i) {
				testValue<Varint<int32_t>>(i);
			}
		}

		template<typename T>
		static void testMax() {
			testValue<Varint<T>>(std::numeric_limits<T>::max());
		}

		template<typename T>
		static void testMin() {
			testValue<Varint<T>>(std::numeric_limits<T>::min());
		}


		TEST_METHOD(MaxValues) {
			testMax<int32_t>();
			testMax<uint32_t>();
			testMax<int64_t>();
			testMax<uint64_t>();
		}

		TEST_METHOD(MinValues) {
			testMin<int32_t>();
			testMin<uint32_t>();
			testMin<int64_t>();
			testMin<uint64_t>();
		}

		TEST_METHOD(MaxValuesSize) {
			Varint32 val32{ std::numeric_limits<uint32_t>::max() };
			Varint64 val64{ std::numeric_limits<uint64_t>::max() };
			Assert::AreEqual(static_cast<size_t>(5), val32.getActualSize());
			Assert::AreEqual(static_cast<size_t>(10), val64.getActualSize());
		}

	};
}