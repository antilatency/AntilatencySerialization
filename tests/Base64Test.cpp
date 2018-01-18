#include "stdafx.h"
#include "CppUnitTest.h"
#include <array>

#include <ctime>
#include "AntilatencySerialization/BinarySerialization.h"
#include "AntilatencySerialization/Base64Stream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <type_traits>
#include <vector>

#include "AntilatencySerialization/Fields.h"



using namespace Antilatency::Serialization;

namespace SerializationTest
{


	TEST_CLASS(Base64Test)
	{
		TEST_CLASS_INITIALIZE(Init) {
			srand(static_cast<unsigned>(time(nullptr)));
		}

	public:
		static void testEncode(const uint8_t* base, size_t baseSize, const uint8_t* result, size_t resultSize) {
			uint8_t* buf = new uint8_t[resultSize];
			MemoryStreamWriter memoryWriter(buf, resultSize);
			Base64StreamWriter writer(&memoryWriter);
			IStreamWriter* iWriter = &writer;

			Assert::IsTrue(iWriter->write(base, baseSize));
			Assert::IsTrue(writer.flush());


			for(size_t i = 0; i < resultSize; ++i) {
				Assert::AreEqual(buf[i], result[i]);
			}

			delete[] buf;
		}

		static void testDecode(const uint8_t* realData, size_t realDataSize, const uint8_t* encodedData, size_t encodedSize) {
			uint8_t* buf = new uint8_t[realDataSize];
			MemoryStreamReader memoryReader(encodedData, encodedSize);
			Base64StreamReader reader(&memoryReader);
			IStreamReader* iReader = &reader;

			Assert::IsTrue(iReader->read(buf, realDataSize));

			for (size_t i = 0; i < realDataSize; ++i) {
				Assert::AreEqual(realData[i], buf[i]);
			}

			delete[] buf;
		}

		static void encodeDecode(const char decoded[], const char encoded[]) {
			testEncode(reinterpret_cast<const uint8_t*>(decoded), strlen(decoded), reinterpret_cast<const uint8_t*>(encoded), strlen(encoded));
			testDecode(reinterpret_cast<const uint8_t*>(decoded), strlen(decoded), reinterpret_cast<const uint8_t*>(encoded), strlen(encoded));
		}

		TEST_METHOD(EmptyString) {
			encodeDecode("", "");
		}

		TEST_METHOD(HelloString) {
			encodeDecode("Hello", "SGVsbG8=");
		}

		TEST_METHOD(Padding1) {
			encodeDecode("any carnal pleasure.", "YW55IGNhcm5hbCBwbGVhc3VyZS4=");			
		}

		TEST_METHOD(Padding2) {
			encodeDecode("any carnal pleasure", "YW55IGNhcm5hbCBwbGVhc3VyZQ==");			
		}

		TEST_METHOD(NoPadding) {
			encodeDecode("any carnal pleasur", "YW55IGNhcm5hbCBwbGVhc3Vy");			
		}

		TEST_METHOD(EncodeDecode) {
			for (auto i = 0; i < 100; ++i) {
				size_t testSize = rand() % 1024;

				std::vector<uint8_t> base;
				base.resize(testSize);

				for (auto i = 0; i < base.size(); ++i) {
					base[i] = static_cast<uint8_t>(rand());
				}

				MemorySizeCounterStream counter;
				 
				{
					Base64StreamWriter writer(&counter);
					IStreamWriter* iWriter = &writer;
					Assert::IsTrue(iWriter->write(reinterpret_cast<const uint8_t*>(base.data()), base.size()));
					Assert::IsTrue(writer.flush());
				}

				std::vector<uint8_t> buffer;
				buffer.resize(counter.getActualSize());
				{
					MemoryStreamWriter memoryWriter(buffer.data(), buffer.size());
					Base64StreamWriter writer(&memoryWriter);
					IStreamWriter* iWriter = &writer;
					Assert::IsTrue(iWriter->write(reinterpret_cast<const uint8_t*>(base.data()), base.size()));
					Assert::IsTrue(writer.flush());
				}
				

				MemoryStreamReader memoryReader(buffer.data(), buffer.size());
				Base64StreamReader reader(&memoryReader);
				IStreamReader* iReader = &reader;

				std::vector<uint8_t> result;
				result.resize(testSize);;
				Assert::IsTrue(iReader->read(result.data(), result.size()));

				for (size_t i = 0; i < testSize; ++i) {
					Assert::AreEqual(base[i], result[i]);
				}
			}
		}

		template<typename T>
		static void testBaseValue(const T& value) {
			uint8_t buffer[32];
			MemoryStreamWriter memoryWriter(buffer, sizeof(buffer));
			Base64StreamWriter writer(&memoryWriter);
			IStreamWriter* iWriter = &writer;
			Assert::IsTrue(iWriter->write(reinterpret_cast<const uint8_t*>(&value), sizeof(T)));
			Assert::IsTrue(writer.flush());

			MemoryStreamReader memoryReader(buffer, 32);
			Base64StreamReader reader(&memoryReader);
			IStreamReader* iReader = &reader;
			T result;
			Assert::IsTrue(iReader->read(reinterpret_cast<uint8_t*>(&result), sizeof(T)));
			Assert::AreEqual(value, result);
		}

		TEST_METHOD(EncodeDecodeUInt32) {
			for(size_t i = 0; i < 100; ++i) {
				testBaseValue(static_cast<uint32_t>(rand()));
			}
		}

		TEST_METHOD(EncodeDecodeInt32) {
			for (size_t i = 0; i < 100; ++i) {
				testBaseValue(static_cast<int32_t>(rand()));
			}
		}

		TEST_METHOD(EncodeDecodeUInt64) {
			for (size_t i = 0; i < 100; ++i) {
				testBaseValue(static_cast<uint64_t>(rand()));
			}
		}

		TEST_METHOD(EncodeDecodeDouble) {
			for (size_t i = 0; i < 100; ++i) {
				testBaseValue(static_cast<double>(rand()));
			}
		}

	};
}