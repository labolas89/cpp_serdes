
#include "serializer_deserializer.hpp"
#include <stdio.h>
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
		std::vector<uint8_t> buf[2] = { std::vector<uint8_t>(0, buf_size), };

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
			printf("buf[%u] : %d compare fail\n", (uint32_t)buf_size, compare);
			for (size_t idx = 0; idx < buf_size; idx++)
				printf("{%hu:%hu} ", buf[0].data()[idx], buf[1].data()[idx]);
			printf("\n");
			//return EXIT_FAILURE;
		}
		else
			printf("buf[%u] : %d compare pass\n", (uint32_t)buf_size, compare);

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
		std::vector<uint8_t> buf[2] = { std::vector<uint8_t>(0, buf_size), };

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
			printf("buf[%u] : %d compare fail\n", (uint32_t)buf_size, compare);
			for (size_t idx = 0; idx < buf_size; idx++)
				printf("{%hu:%hu} ", buf[0].data()[idx], buf[1].data()[idx]);
			printf("\n");
			return EXIT_FAILURE;
		}
		else
			printf("buf[%u] : %d compare pass\n", (uint32_t)buf_size, compare);

	}
    
    return EXIT_FAILURE;
}