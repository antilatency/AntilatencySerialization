#include "stdafx.h"
#include "CppUnitTest.h"
#include <array>

#include "AntilatencySerialization/Fields.h"
#include "AntilatencySerialization/BinarySerialization.h"

#include <ctime>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Antilatency::Serialization;

namespace SerializationTest
{


	TEST_CLASS(VectorFieldTest)
	{
		TEST_CLASS_INITIALIZE(Init) {
			srand(static_cast<unsigned>(time(nullptr)));
		}

		class Name {};
	public:
		template<typename Type>
		static void testValue(const Type& t) {
			ContainerField<Type, Name> vectorField;
			vectorField.setValue(t);
			MemorySizeCounterStream counterStream;
			BinarySerializer serializer(&counterStream);
			serializer.serialize(vectorField);
			uint8_t* buffer = new uint8_t[counterStream.getActualSize()];
			MemoryStreamWriter writer(buffer, counterStream.getActualSize());
			serializer.setStreamWriter(&writer);
			auto writeSize = serializer.serialize(vectorField);
			MemoryStreamReader reader(buffer, writeSize);
			BinaryDeserializer deserializer(&reader);
			Type dest;
			deserializer.deserialize(dest);

			Assert::AreEqual(t.size(), dest.size());

			Assert::IsTrue(std::equal(t.begin(), t.end(), dest.begin()));
			delete[] buffer;
		}
	
		TEST_METHOD(Empty) {
			std::vector<int> var;
			testValue(var);
		}

		TEST_METHOD(Int) {
			std::vector<int> var = {1, 2, 3, 4, 5, 6, 7};
			testValue(var);
		}

		TEST_METHOD(String) {
			std::string var = "Test string ";
			testValue(var);
		}

		TEST_METHOD(Uin64_t) {
			std::vector<uint64_t> var = { 1, 2, 3, 4, 5, 6, 7 };
			testValue(var);
		}

		TEST_METHOD(Inc) {
			std::vector<int> var;
			for(int i = 0; i < rand() % 1000; ++i) {
				var.push_back(i);
			}
			testValue(var);
		}

		TEST_METHOD(Random) {
			std::vector<int> var;
			for (int i = 0; i < rand() % 1000; ++i) {
				var.push_back(rand());
			}
			testValue(var);
		}

		TEST_METHOD(Child) {
			std::vector<std::vector<std::vector<std::vector<int>>>> var = { {{{1,3}, {2, 4}, {5, 6}}} };
			MemorySizeCounterStream str;
			BinarySerializer serializer(&str);
			auto size = serializer.serialize(var);
			testValue(var);
		}
	};

}