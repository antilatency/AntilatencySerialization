#include "stdafx.h"
#include "CppUnitTest.h"
#include <array>

#include <ctime>
#include "AntilatencySerialization/BinarySerialization.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <type_traits>

#include "AntilatencySerialization/Fields.h"



using namespace Antilatency::Serialization;

namespace SerializationTest
{


	TEST_CLASS(SingleFieldTest)
	{
		TEST_CLASS_INITIALIZE(Init) {
			srand(static_cast<unsigned>(time(nullptr)));
		}

	public:
		class FieldNameA {};
		class FieldNameB {};
		using FieldA = SingleField<int32_t, FieldNameA>;
		using FieldB = SingleField<uint64_t, FieldNameB>;
		

		TEST_METHOD(EqualId) {
			if constexpr(!std::is_same<FieldA::Name, FieldA::Name>()) {
				Assert::Fail();
			}

			if constexpr(!std::is_same<FieldB::Name, FieldB::Name>()) {
				Assert::Fail();
			}
		}

		TEST_METHOD(NotEqualId) {
			if constexpr(std::is_same<FieldA::Name, FieldB::Name>()) {
				Assert::Fail();
			}
		}
		template<typename TypeField, typename TypeValue>
		static void testPackUnpack(TypeValue value) {
			TypeField src;
			src.setValue(value);
			Assert::AreEqual(src.getValue(), value);
			uint8_t buffer[32];
			MemoryStreamWriter writer(buffer, sizeof(buffer));
			BinarySerializer binarySerializer(&writer);
			auto write = binarySerializer.serialize(src);
			TypeField dst;
			MemoryStreamReader reader(buffer, write);
			BinaryDeserializer deserializer(&reader);
			auto read = deserializer.deserialize(dst);
			Assert::AreEqual(dst.getValue(), value);
			Assert::AreEqual(write, read);
			Assert::AreEqual(sizeof(TypeValue), read);
		}

		TEST_METHOD(IncValue) {
			for (int32_t i = -10000; i < 10000; ++i) {
				testPackUnpack<FieldA>(i);
			}
			for (uint64_t i = 0; i < 10000; ++i) {
				testPackUnpack<FieldB>(i);
			}
		}

		TEST_METHOD(RandomValue) {
			for (size_t i = 0; i < 10000; ++i) {
				testPackUnpack<FieldA>(std::rand());
				testPackUnpack<FieldB>((static_cast<FieldB::Type>(std::rand()) << 32) | std::rand());
			}
		}

	};
}