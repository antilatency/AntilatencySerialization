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
		bool convertFromPreviousVersion(VersionType version, Deserializer& deserializer) {
            static_cast<void>(version);
			static_cast<void>(deserializer);
			return false;
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
		bool convertFromPreviousVersion(VersionType version, Deserializer& deserializer) {
			Environment0::Environment previousVersion;
			if(previousVersion.deserialize(version, deserializer)) {
				get<Type>().setValue(0);
				get<Width>().setValue(previousVersion.get<Environment0::Width>().getValue());
				get<Height>().setValue(previousVersion.get<Environment0::Height>().getValue());
				get<Bars>().setValue(previousVersion.get<Environment0::Bars>().getValue());

				return true;
			}	

			return false;
		}
	};
}

size_t testBinarySerialization(const Environment0::Environment& env0) {
	Antilatency::Serialization::OstreamSerializer ioSerializer(std::cout);
	Environment1::Environment env1;
	ioSerializer.serialize(env0);
	std::cout << std::endl;

	Antilatency::Serialization::MemorySizeCounterStream counterStream;
	Antilatency::Serialization::BinarySerializer binSerializer(&counterStream);
	if (binSerializer.serialize(env0)) {

		size_t realSize = counterStream.getActualSize();

		std::vector<uint8_t> buffer;
		buffer.resize(realSize);
		Antilatency::Serialization::MemoryStreamWriter memoryStreamWriter(buffer.data(), buffer.size());
		binSerializer.setStreamWriter(&memoryStreamWriter);

		if (binSerializer.serialize(env0)) {
			Antilatency::Serialization::MemoryStreamReader memoryStreamReader(buffer.data(), buffer.size());
			Antilatency::Serialization::BinaryDeserializer binDeserializer(&memoryStreamReader);
			if (binDeserializer.deserialize(env1)) {

				ioSerializer.serialize(env1);
				std::cout << std::endl;
				return buffer.size();
			}
		}
	}
	
	return 0;
}


size_t testBase64Serialization(const Environment0::Environment& env0) {
	Antilatency::Serialization::OstreamSerializer ioSerializer(std::cout);
	Environment1::Environment env1;

	ioSerializer.serialize(env0);
	std::cout << std::endl;

	Antilatency::Serialization::MemorySizeCounterStream counterStream;
	Antilatency::Serialization::Base64StreamWriter base64StreamCounter(&counterStream);
	Antilatency::Serialization::BinarySerializer binSerializer(&base64StreamCounter);
	if(binSerializer.serialize(env0)) {	
		base64StreamCounter.flush();

		size_t realSize = counterStream.getActualSize();

		std::vector<uint8_t> buffer;
		buffer.resize(realSize);
		Antilatency::Serialization::MemoryStreamWriter memoryStreamWriter(buffer.data(), buffer.size());
		Antilatency::Serialization::Base64StreamWriter base64StreamWriter(&memoryStreamWriter);
		binSerializer.setStreamWriter(&base64StreamWriter);
	
		if (binSerializer.serialize(env0)) {
			base64StreamWriter.flush();

			Antilatency::Serialization::MemoryStreamReader memoryStreamReader(buffer.data(), buffer.size());
			Antilatency::Serialization::Base64StreamReader base64StreamReader(&memoryStreamReader);
			Antilatency::Serialization::BinaryDeserializer binDeserializer(&base64StreamReader);
			if (binDeserializer.deserialize(env1)) {
				env1.serialize(ioSerializer);
				std::cout << std::endl;
				return buffer.size();
			}
		} 
	}
	return 0;
}

size_t testUserStream(const Environment0::Environment& env0) {
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
	if (binSerializer.serialize(env0)) {

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
		if (binDeserializer.deserialize(env1)) {
			ioSerializer.serialize(env1);
			std::cout << std::endl;
			return binData.size();
		}
	}
	return 0;
}

void testConst(const Environment0::Environment environment) {
	using namespace Environment0;
	auto& name = environment.get<Name>().getValue();
	auto& width = environment.get<Width>().getValue();
	auto& height = environment.get<Height>().getValue();
	auto& bars = environment.get<Bars>().getValue();
}

int main()
{
	using namespace Environment0;
	Environment environment;	
	environment.get<Name>().setValue("Enviroment0");
	environment.get<Width>().setValue(16);
	environment.get<Height>().setValue(12);
	auto& bars = environment.get<Bars>().getValue();
	bars.resize(5);

	for (size_t i = 0; i < environment.get<Bars>().getValue().size(); ++i) {
		bars[i].get<Bar::PositionX>().setValue(static_cast<int32_t>(i));
		bars[i].get<Bar::PositionY>().setValue(static_cast<int32_t>(i));
		bars[i].get<Bar::Direction>().setValue(static_cast<int32_t>(i));
	}
	
	std::cout << "Binary size = " << testBinarySerialization(environment) << std::endl << std::endl;
	std::cout << "Base64 size = " << testBase64Serialization(environment) << std::endl << std::endl;
	std::cout << "User stream size = " << testUserStream(environment) << std::endl << std::endl;

	testConst(environment);

    std::cin.get();

	return 0;
}
