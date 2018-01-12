#include <iostream>
#include <vector>

#include "AntilatencySerialization/Fields.h"
#include "AntilatencySerialization/Structures.h"

#include "AntilatencySerialization/BinarySerialization.h"
#include "AntilatencySerialization/Base64Stream.h"
#include "AntilatencySerialization/OstreamSerialization.h"

#include "AntilatencySerialization/UserStream.h"

namespace Bar {
	SERIALIZATION_MAKE_FIELD_NAME(PositionX);
	SERIALIZATION_MAKE_FIELD_NAME(PositionY);
	SERIALIZATION_MAKE_FIELD_NAME(Direction);

	using Bar = Antilatency::Serialization::Structure<
		Antilatency::Serialization::Int32Field<PositionX>,
		Antilatency::Serialization::Int32Field<PositionY>,
		Antilatency::Serialization::Int32Field<Direction>
	>;
}

namespace Environment0 {
	SERIALIZATION_MAKE_FIELD_NAME(Width);
	SERIALIZATION_MAKE_FIELD_NAME(Height);
	SERIALIZATION_MAKE_FIELD_NAME(Bars);
	SERIALIZATION_MAKE_FIELD_NAME(Name);

	template<typename T>
	using Base = Antilatency::Serialization::VersionedStructure<0,
		T,
		Antilatency::Serialization::OptioinalField<Antilatency::Serialization::StringField<Name>>,
		Antilatency::Serialization::Int32Field<Width>,
		Antilatency::Serialization::Int32Field<Height>,
		Antilatency::Serialization::VectorField<Bar::Bar, Bars>
	>;

	class Environment : public Base<Environment> {
	public:
		template<typename Deserializer>
		size_t convertFromPreviousVersion(VersionType version, Deserializer& deserializer) {
			return 0;
		}
	};
}

namespace Environment1 {
	SERIALIZATION_MAKE_FIELD_NAME(Type);
	SERIALIZATION_MAKE_FIELD_NAME(Width);
	SERIALIZATION_MAKE_FIELD_NAME(Height);
	SERIALIZATION_MAKE_FIELD_NAME(Bars);

	template<typename T>
	using Base = Antilatency::Serialization::VersionedStructure < 1,
		T,
		Antilatency::Serialization::Int32Field<Type>,
		Antilatency::Serialization::Int32Field<Width>,
		Antilatency::Serialization::Int32Field<Height>,
		Antilatency::Serialization::VectorField<Bar::Bar, Bars>
	>;


	class Environment : public Base<Environment> {
	public:
		template<typename Deserializer>
		size_t convertFromPreviousVersion(VersionType version, Deserializer& deserializer) {
			Environment0::Environment previousVersion;
			size_t size = previousVersion.deserialize(version, deserializer);

			get<Type>().setValue(0);
			get<Width>().setValue(previousVersion.get<Environment0::Width>().getValue());
			get<Height>().setValue(previousVersion.get<Environment0::Height>().getValue());
			get<Bars>().setValue(previousVersion.get<Environment0::Bars>().getValue());

			return size;
		}
	};
}

void testBinarySerialization(const Environment0::Environment& env0) {
	Antilatency::Serialization::OstreamSerializer ioSerializer(std::cout);
	Environment1::Environment env1;
	ioSerializer.serialize(env0);
	std::cout << std::endl;

	Antilatency::Serialization::MemorySizeCounterStream counterStream;
	Antilatency::Serialization::BinarySerializer binSerializer(&counterStream);

	size_t realSize = binSerializer.serialize(env0);
	std::vector<uint8_t> buffer;
	buffer.resize(realSize);
	Antilatency::Serialization::MemoryStreamWriter memoryStreamWriter(buffer.data(), buffer.size());
	binSerializer.setStreamWriter(&memoryStreamWriter);

	auto writeSize = binSerializer.serialize(env0);

	Antilatency::Serialization::MemoryStreamReader memoryStreamReader(buffer.data(), writeSize);
	Antilatency::Serialization::BinaryDeserializer binDeserializer(&memoryStreamReader);
	auto readSize = binDeserializer.deserialize(env1);

	ioSerializer.serialize(env1);
	std::cout << std::endl;

	std::cout << "Binary: Write size " << writeSize << "\t" << "Read size " << readSize << std::endl << std::endl;
}



void testBase64Serialization(const Environment0::Environment& env0) {
	Antilatency::Serialization::OstreamSerializer ioSerializer(std::cout);
	Environment1::Environment env1;

	ioSerializer.serialize(env0);
	std::cout << std::endl;

	Antilatency::Serialization::MemorySizeCounterStream counterStream;
	Antilatency::Serialization::Base64StreamWriter base64StreamCounter(&counterStream);
	Antilatency::Serialization::BinarySerializer binSerializer(&base64StreamCounter);

	size_t realSize = binSerializer.serialize(env0) + base64StreamCounter.flush();
	std::vector<uint8_t> buffer;
	buffer.resize(realSize);
	Antilatency::Serialization::MemoryStreamWriter memoryStreamWriter(buffer.data(), buffer.size());
	Antilatency::Serialization::Base64StreamWriter base64StreamWriter(&memoryStreamWriter);
	binSerializer.setStreamWriter(&base64StreamWriter);
	

	auto writeSize = binSerializer.serialize(env0) + base64StreamWriter.flush();


	Antilatency::Serialization::MemoryStreamReader memoryStreamReader(buffer.data(), writeSize);
	Antilatency::Serialization::Base64StreamReader base64StreamReader(&memoryStreamReader);
	Antilatency::Serialization::BinaryDeserializer binDeserializer(&base64StreamReader);
	auto readSize = binDeserializer.deserialize(env1);

	env1.serialize(ioSerializer);
	std::cout << std::endl;

	std::cout << "Base64: Write size " << writeSize << "\t" << "Read size " << readSize << std::endl << std::endl;
}

void testUserStream(const Environment0::Environment& env0) {
	Antilatency::Serialization::OstreamSerializer ioSerializer(std::cout);
	ioSerializer.serialize(env0);
	std::cout << std::endl;

	std::vector<uint8_t> binData;
	Antilatency::Serialization::UserStreamWriter vectorWriter([&binData](const uint8_t* buffer, size_t size) -> size_t {
		for (size_t i = 0; i < size; ++i) {
			binData.push_back(buffer[i]);
		}
		return size;
	});

	Antilatency::Serialization::BinarySerializer binSerializer(&vectorWriter);
	auto writeSize = binSerializer.serialize(env0);

	size_t readIndex = 0;
	Antilatency::Serialization::UserStreamReader vectorReader([&binData, &readIndex](uint8_t* buffer, size_t size) -> size_t {
		for (size_t i = 0; i < size; ++i) {
			buffer[i] = binData.at(readIndex);
			++readIndex;
		}
		return size;
	});
	Antilatency::Serialization::BinaryDeserializer binDeserializer(&vectorReader);
	Environment1::Environment env1;
	auto readSize = binDeserializer.deserialize(env1);

	ioSerializer.serialize(env1);
	std::cout << std::endl;

	std::cout << "User Stream: Write size " << writeSize << "\t" << "Read size " << readSize << std::endl << std::endl;
}


int main()
{
	Environment0::Environment environment0;
	environment0.get<Environment0::Name>().setValue("Enviroment0");
	environment0.get<Environment0::Width>().setValue(16);
	environment0.get<Environment0::Height>().setValue(12);
	auto& bars = environment0.get<Environment0::Bars>().getRef();
	bars.resize(5);

	for (size_t i = 0; i < environment0.get<Environment0::Bars>().getValue().size(); ++i) {
		bars[i].get<Bar::PositionX>().setValue(static_cast<int32_t>(i));
		bars[i].get<Bar::PositionY>().setValue(static_cast<int32_t>(i));
		bars[i].get<Bar::Direction>().setValue(static_cast<int32_t>(i));
	}

	testBinarySerialization(environment0);
	testBase64Serialization(environment0);
	testUserStream(environment0);

	std::cin.get();

	return 0;
}
