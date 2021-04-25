
#include "serializer_deserializer.hpp"
#include <iostream>
#include <cstring>
#include <typeinfo>
#include <list>


//----------------------------------------------------------------------------------------------------
struct arithmeticStruct {
	short	s16;
	float	f32;
	int		s32;
	double  f64;
};

static_assert(SerDesLittle::is_serdesable_v<arithmeticStruct>, "");

//----------------------------------------------------------------------------------------------------
struct triviallyCopyableStruct {
	int s32;
	char s8;
	arithmeticStruct ar_st;
	// If std::is_trivially_copyable<Tp>::value is TRUE, 
	// SERDES is possible even if it is included as a member variable.
	std::array<char, 32> s8_arr; 


	// Explicit default constructor
	triviallyCopyableStruct(triviallyCopyableStruct const&) = default; 
	triviallyCopyableStruct() = default;
};


static_assert(SerDesLittle::is_serdesable_v<triviallyCopyableStruct>, "");

//----------------------------------------------------------------------------------------------------
// Trivially copy is impossible due to std::vector<int>
// If you want dynamic allocation arguments below COMPLEX_O9 See example
struct triviallyUncopyableStruct {
	int s32;
	std::vector<int> vec; // std::is_trivially_copyable<Tp>::value == false 
	char u8;
	triviallyUncopyableStruct() = default;
	triviallyUncopyableStruct(const triviallyUncopyableStruct&) = default;
};

static_assert(!SerDesLittle::is_serdesable_v<triviallyUncopyableStruct>, "");
//----------------------------------------------------------------------------------------------------

// pragma pack SERDES no problem
#pragma pack(push, 1)
struct bytePackStruct {
	char					s8;
	unsigned char			u8;
	short					s16;
	float					f32;
	int						s32;
	double					f64;
};
#pragma pack(pop)

static_assert(SerDesLittle::is_serdesable_v<bytePackStruct>, "");
//----------------------------------------------------------------------------------------------------

struct containCopyableStaticMemberStruct
{
	containCopyableStaticMemberStruct() = default;

	// Constructor that acts like std::initializer_list<T>
	containCopyableStaticMemberStruct(int in0, int in1, int in2, int in3) 
		: s32_0(in0), s32_1(in1), s32_2(in2), s32_3(in3) {}
	containCopyableStaticMemberStruct(const containCopyableStaticMemberStruct&) = default;
	int s32_0;
	int s32_1;
	int s32_2;
	int s32_3;
	static arithmeticStruct copy_able[2]; // Static members are excluded from SERDES
};

static_assert(SerDesLittle::is_serdesable_v<containCopyableStaticMemberStruct>, "");
//----------------------------------------------------------------------------------------------------

struct containUncopyableStaticMemberStruct
{
	containUncopyableStaticMemberStruct() = default;

	containUncopyableStaticMemberStruct(std::initializer_list<int> ilist) 
	{
		auto it = ilist.begin(); s32_0 = *it++; s32_1 = *it++; s32_2 = *it++; s32_3 = *it;
	}
	containUncopyableStaticMemberStruct(const containUncopyableStaticMemberStruct&) = default;
	int s32_0;
	int s32_1;
	int s32_2;
	int s32_3;
	static triviallyUncopyableStruct copy_unable[2]; // Static members are excluded from SERDES
};

static_assert(SerDesLittle::is_serdesable_v<containUncopyableStaticMemberStruct>, "");
//----------------------------------------------------------------------------------------------------

class defaultConstructorClass {
public:
	char	s8;
	short	s16;
protected:
	double	f64;
private:
	float	f32;
};

static_assert(SerDesLittle::is_serdesable_v<defaultConstructorClass>, "");
//----------------------------------------------------------------------------------------------------

class defaultConstructorDeleteClass {
public:
	defaultConstructorDeleteClass() : s8(8), s16(16), f64(64.0), f32(32.f) {};
	defaultConstructorDeleteClass(const defaultConstructorDeleteClass&) {};
	char	s8;
	short	s16;
protected:
	double	f64;
private:
	float	f32;
};

static_assert(!SerDesLittle::is_serdesable_v<defaultConstructorDeleteClass>, "");
//----------------------------------------------------------------------------------------------------

// SERDES is not possible because there is no member variable
struct NoMembersStruct {
	virtual void foo();
};

static_assert(!SerDesLittle::is_serdesable_v<NoMembersStruct>, "");
//----------------------------------------------------------------------------------------------------

// Member pointers inside structures are difficult to distinguish in the current C++ standard.
// If you need dynamic allocation, use a combination of std::vector and std::tuple
struct containPointerMemberStruct {
	int		s32;
	char*	str; // WARNING!! This is only the address value copied
};

static_assert(SerDesLittle::is_serdesable_v<containPointerMemberStruct>, "");
//----------------------------------------------------------------------------------------------------

// SERDES can be performed on such a complex type
typedef std::vector<int>																COMPLEX_O1;
typedef std::vector<triviallyCopyableStruct>											COMPLEX_O2;
typedef std::vector<containCopyableStaticMemberStruct>									COMPLEX_O3;

typedef std::array<float, 8>															COMPLEX_O4;
typedef std::array<COMPLEX_O3, 4>														COMPLEX_O5;

typedef std::tuple<std::string, COMPLEX_O2>												COMPLEX_O6;
typedef std::tuple<uint32_t, float, std::string, char>									COMPLEX_O7;
typedef std::tuple<uint32_t, std::string, COMPLEX_O5, arithmeticStruct, bytePackStruct>	COMPLEX_O8;

typedef std::vector<COMPLEX_O8>															COMPLEX_O9;


static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O1>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O2>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O3>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O4>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O5>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O6>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O7>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O8>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O9>, "");

//----------------------------------------------------------------------------------------------------

// SERDES is impossible when disallowed types are mixed
typedef std::vector<std::tuple<COMPLEX_O2, std::string, triviallyUncopyableStruct>>	COMPLEX_X1;
typedef std::tuple<COMPLEX_O6, arithmeticStruct, defaultConstructorDeleteClass>		COMPLEX_X2;

static_assert(!SerDesLittle::is_serdesable_v<COMPLEX_X1>, "");
static_assert(!SerDesLittle::is_serdesable_v<COMPLEX_X2>, "");

//----------------------------------------------------------------------------------------------------

int main() 
{
	// Decide what type to serialize/deserialize.
	using target_type = typename std::tuple<std::string, std::vector<int>, double>;

	// Determine serialization/deserialization is possible at compile time. 
	// Applicable in type trait.
	static_assert(SerDesLittle::is_serdesable_v<target_type>, "cannot convert");

	// Define a buffer to store serialized data.
	// It means data to be transmitted through the outside of the function or communication interface
	std::vector<uint8_t> serial_buffer;
	{
		// Insert data to be transmitted in a predefined data type variable.
		target_type src = std::make_tuple("source", std::vector<int>{1, 2, 3}, 4.0);

		// Calculate the payload size of the data inserted above.
		// Next, the serialization buffer is dynamically allocated.
		serial_buffer.resize(SerDesLittle::payload_size(src));

		// Perform serialization.
		size_t serialize_size = SerDesLittle::serialize(serial_buffer.data(), src);

		// Compare buffer size and serialized size. 
		// This step can be omitted.
		assert(serial_buffer.size() == serialize_size && "converting size error");

		std::cout << "serialize_size : " << SerDesLittle::to_string(serialize_size) << " byte" << std::endl;
	}
	{
		// Define a variable to receive deserialization data.
		target_type dest;

		// Perform deserialization from serialized data.
		// Only dynamically allocated (vector, etc.) buffer size-related information 
		// is separately serialized and included.
		size_t deserialize_size = SerDesLittle::deserialize(dest, serial_buffer.data());

		// Compare buffer size and deserialized size.
		// This step can be omitted.
		assert(serial_buffer.size() == deserialize_size && "converting size error");

		// Output data to the console to check the result.
		// Easily printable by using the following function.
		std::cout << "deserialize[" << SerDesLittle::to_string(deserialize_size) << "] <" <<
			SerDesLittle::type_name<decltype(dest)>() << "> :" <<
			SerDesLittle::to_string(dest) << std::endl << std::endl;
	}
	
	int ret = EXIT_SUCCESS;

	//----------------------------------------------------------------------------------------------------
	{
		
		COMPLEX_O6 serial_src, deserial_dst;

		serial_src = std::make_tuple("COMPLEX_O6 test", std::vector<triviallyCopyableStruct>(5));
		int i = 0;
		for (auto& elem_o2 : std::get<1>(serial_src))
		{
			elem_o2.s8 = i++;
			elem_o2.s32 = elem_o2.s8 * 32;

			elem_o2.ar_st.f32 = elem_o2.s32;
			elem_o2.ar_st.f64 = elem_o2.s32 * 2;
			elem_o2.ar_st.s16 = elem_o2.s32 / 2;
			elem_o2.ar_st.s32 = rand();

			sprintf(elem_o2.s8_arr.data(), "SERDES:%d", elem_o2.s32);
		}

		
		// getting buffer size
		auto buf_size = SerDesLittle::payload_size(serial_src);

		// initialize zero & buf_size
		// for byte-compare(memcmp) with none bytepack struct
		std::vector<uint8_t> buf[2] = { std::vector<uint8_t>(0, buf_size), std::vector<uint8_t>(0, buf_size) };

		// setting buffer
		buf[0].resize(buf_size);

		// run serialization src to buffer
		SerDesLittle::serialize(buf[0].data(), serial_src);

		// run deserialization buffer to complex structure
		SerDesLittle::deserialize(deserial_dst, buf[0].data());

		printf("<%s> : %s\n",
			SerDesLittle::type_name<decltype(deserial_dst)>().c_str(),
			SerDesLittle::to_string(deserial_dst).c_str() // Easy String Converter
			// In-class/structure parsing is supported by std:c++20, but not perfect
		);

		// Preparing a buffer for byte comparison
		buf[1].resize(buf_size);

		// run serialization to another buffer
		SerDesLittle::serialize(buf[1].data(), deserial_dst);

		// check byte error
		int compare = memcmp(buf[0].data(), buf[1].data(), buf_size);

		if (compare != 0) {
			printf("buf[%u] : %d compare fail\n\n", (uint32_t)buf_size, compare);
			for (size_t idx = 0; idx < buf_size; idx++) {
				if (buf[0].data()[idx] != buf[1].data()[idx])
					printf("[%ull]{%hu:%hu} ", idx, buf[0].data()[idx], buf[1].data()[idx]);
			}
			printf("\n");
			ret = EXIT_FAILURE;
		}
		else
			printf("buf[%u] : %d compare pass\n\n", (uint32_t)buf_size, compare);

	}

	//----------------------------------------------------------------------------------------------------
	{
		
		COMPLEX_O9 serial_src, deserial_dst;

		for (size_t i = 0; i < 3; i++)
		{
			serial_src.push_back(
				std::make_tuple( // initialize COMPLEX_O8
					(uint32_t)rand(),
					std::string("COMPLEX_O9 test" + std::to_string(i)),
					std::array<std::vector<containCopyableStaticMemberStruct>, 4>{ // COMPLEX_O5
						std::vector<containCopyableStaticMemberStruct>(1, { rand(), rand(), rand(), rand() }), // COMPLEX_O3
						std::vector<containCopyableStaticMemberStruct>(2, { rand(), rand(), rand(), rand() }), 
						std::vector<containCopyableStaticMemberStruct>(3, { rand(), rand(), rand(), rand() }), 
						std::vector<containCopyableStaticMemberStruct>(4, { rand(), rand(), rand(), rand() })},
					arithmeticStruct{ (short)rand(), (float)rand(), (int)rand(), (double)rand()},
					bytePackStruct{ 
						(char)(8 * i), 
						(unsigned char)(8U * i), 
						(short)(16 * i), 
						32.f * i, 
						(int)(32 * i), 
						64.0 * i }
				)
			);
		}

		// getting buffer size
		auto buf_size = SerDesLittle::payload_size(serial_src);

		// initialize zero & buf_size
		// for byte-compare(memcmp) with none bytepack struct
		std::vector<uint8_t> buf[2] = { std::vector<uint8_t>(0, buf_size), std::vector<uint8_t>(0, buf_size) };

		// setting buffer
		buf[0].resize(buf_size);

		// run serialization src to buffer
		SerDesLittle::serialize(buf[0].data(), serial_src);

		// run deserialization buffer to complex structure
		SerDesLittle::deserialize(deserial_dst, buf[0].data());

		printf("<%s> : %s\n",
			SerDesLittle::type_name<decltype(deserial_dst)>().c_str(),
			SerDesLittle::to_string(deserial_dst).c_str() // Easy String Converter
			// In-class/structure parsing is supported by std:c++20, but not perfect
		);

		// Preparing a buffer for byte comparison
		buf[1].resize(buf_size);

		// run serialization to another buffer
		SerDesLittle::serialize(buf[1].data(), deserial_dst);

		// check byte error
		int compare = memcmp(buf[0].data(), buf[1].data(), buf_size);

		if (compare != 0) {
			printf("buf[%u] : %d compare fail\n\n", (uint32_t)buf_size, compare);
			for (size_t idx = 0; idx < buf_size; idx++) {
				if (buf[0].data()[idx] != buf[1].data()[idx])
					printf("[%ull]{%hu:%hu} ", idx, buf[0].data()[idx], buf[1].data()[idx]);
			}
			printf("\n");
			ret = EXIT_FAILURE;
		}
		else
			printf("buf[%u] : %d compare pass\n\n", (uint32_t)buf_size, compare);

	}
    
    return ret;
}