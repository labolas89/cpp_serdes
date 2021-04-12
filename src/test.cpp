
#include "serializer_deserializer.hpp"
#include <stdio.h>
#include <cstring>
#include <typeinfo>

//****************************************************************************************************
struct O1 {
	int m;
};

static_assert(SerDesLittle::is_serdesable_v<O1>, "");

//****************************************************************************************************
struct O2 {
	int m;
	char* u_str; // For pointers, only NULL-terminated strings are allowed.

	// Explicit trivially copyable (required std::is_trivially_copyable<Tp>::value)
	O2(O2 const&) = default; 
	O2() = default;
};

static_assert(SerDesLittle::is_serdesable_v<O2>, "");
//****************************************************************************************************

struct O3 {
	float re;
	float im;
	uint8_t c;

	O3() = default;
	O3(O3 const&) = default;
};

static_assert(SerDesLittle::is_serdesable_v<O3>, "");
//****************************************************************************************************

// pragma pack SERDES no problem
#pragma pack(push, 1)
struct O4 {
	O4() = default;
	O4(O4 const&) = default;
	char	s8;
	short	s16;
	float	f32;
	int		s32;
	double  f64;
};
#pragma pack(pop)

static_assert(SerDesLittle::is_serdesable_v<O4>, "");
//****************************************************************************************************

struct O5
{
	O5() = default;

	O5(int in0, int in1, int in2, int in3) // Constructor that acts like std::initializer_list<T>
		: distance((float)in0), speed((float)in1), psd((float)in2), thres((float)in3) {}
	O5(const O5&) = default;
	float distance;
	float speed;
	float psd;
	float thres;
	static O3 freq2dist_calib[2]; // Static members are excluded from SERDES
};

static_assert(SerDesLittle::is_serdesable_v<O5>, "");
//****************************************************************************************************

class O7 {
public:
	char	s8;
	short	s16;
	float	f32;
};

static_assert(SerDesLittle::is_serdesable_v<O7>, "");
//****************************************************************************************************

// Trivially copy is impossible due to std::vector<int>
// If you want dynamic allocation arguments below COMPLEX_O9 See example
struct X1 {
	int m;
	std::vector<int> vec; // std::is_trivially_copyable<Tp>::value == false 
	char c;
	X1() = default;
	X1(const X1&) = default; 
};

static_assert(!SerDesLittle::is_serdesable_v<X1>, "");
//****************************************************************************************************

struct O6
{
	uint16_t sc[2];
	static X1 freq2dist_calib[2]; // Static members are excluded from SERDES
};

static_assert(SerDesLittle::is_serdesable_v<O6>, "");
//****************************************************************************************************

// SERDES is not possible because the pointer cannot be copied simply
struct X2 {
	float f32;
	char* str; // std::is_trivially_copyable<Tp>::value == false 

	X2(const X2& other) : f32(other.f32), str(new char[16]) {} // Delete trivially copy constructor
};

static_assert(!SerDesLittle::is_serdesable_v<X2>, "");
//****************************************************************************************************

// SERDES is not possible because there is no member variable
struct X3 {
	virtual void foo();
};

static_assert(!SerDesLittle::is_serdesable_v<X3>, "");
//****************************************************************************************************

// SERDES can be performed on such a complex type
typedef std::vector<int>										COMPLEX_O1;
typedef std::vector<O2>											COMPLEX_O2;
typedef std::vector<O5>											COMPLEX_O3;

typedef std::array<float, 8>									COMPLEX_O4;
typedef std::array<COMPLEX_O3, 4>								COMPLEX_O5;

typedef std::tuple<std::string, COMPLEX_O2>						COMPLEX_O6;
typedef std::tuple<uint32_t, float, std::string, char>			COMPLEX_O7;
typedef std::tuple<uint32_t, std::string, COMPLEX_O5, O3, O4>	COMPLEX_O8;

typedef std::vector<COMPLEX_O8>									COMPLEX_O9;


static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O1>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O2>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O3>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O4>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O5>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O6>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O7>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O8>, "");
static_assert(SerDesLittle::is_serdesable_v<COMPLEX_O9>, "");

//****************************************************************************************************

// SERDES is impossible when disallowed types are mixed
typedef std::vector<std::tuple<COMPLEX_O2, std::string, X1>>	COMPLEX_X1;
typedef std::tuple<COMPLEX_O6, O6, X2>							COMPLEX_X2;

//static_assert(!SerDesLittle::is_serdesable_v<COMPLEX_X1>, "");
//static_assert(!SerDesLittle::is_serdesable_v<COMPLEX_X2>, "");

//****************************************************************************************************



int main() 
{
	//****************************************************************************************************
	{
		std::vector<uint8_t> buf[2];
		COMPLEX_O6 serial_src, deserial_dst;

		serial_src = std::make_tuple("COMPLEX_O6 test", std::vector<O2>(5));
		int i = 0;
		for (auto& elem_o2 : std::get<1>(serial_src))
		{
			elem_o2.m = i++;
			elem_o2.u_str = new char[32];
			sprintf(elem_o2.u_str, "SERDES:%d", elem_o2.m);
			printf("elem_o2.u_str size:%u\n", (uint32_t)SerDesLittle::payload_size(elem_o2.u_str));
		}

		// getting buffer size
		auto buf_size = SerDesLittle::payload_size(serial_src);

		// setting buffer
		buf[0].resize(buf_size);

		// run serialization src to buffer
		SerDesLittle::serialize(buf[0].data(), serial_src);

		// run deserialization buffer to complex structure
		SerDesLittle::deserialize(deserial_dst, buf[0].data());

		printf("<%s> : %s\n",
			typeid(deserial_dst).name(),
			SerDesLittle::to_string(deserial_dst).c_str() // Easy String Converter
			// In-class/structure parsing is supported by std:c++20, but not perfect
		);

		printf("<%s> : %s\n",
			typeid(serial_src).name(),
			SerDesLittle::to_string(serial_src).c_str() // Easy String Converter
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
			printf("%s / %s\n", std::get<1>(deserial_dst).at(0).u_str, std::get<1>(serial_src).at(0).u_str);
			printf("%s / %s\n", std::get<1>(deserial_dst).at(1).u_str, std::get<1>(serial_src).at(1).u_str);
			printf("%s / %s\n", std::get<1>(deserial_dst).at(2).u_str, std::get<1>(serial_src).at(2).u_str);
			printf("%s / %s\n", std::get<1>(deserial_dst).at(3).u_str, std::get<1>(serial_src).at(3).u_str);
			printf("%s / %s\n", std::get<1>(deserial_dst).at(4).u_str, std::get<1>(serial_src).at(4).u_str);
			//return EXIT_FAILURE;
		}
		else
			printf("buf[%u] : %d compare pass\n", (uint32_t)buf_size, compare);

	}

	//****************************************************************************************************
	{
		std::vector<uint8_t> buf[2];
		COMPLEX_O9 serial_src, deserial_dst;

		for (size_t i = 0; i < 3; i++)
		{
			serial_src.push_back(
				std::make_tuple( // initialize COMPLEX_O8
					(uint32_t)rand(),
					std::string("COMPLEX_O9 test" + std::to_string(i)),
					std::array<std::vector<O5>, 4>{ // COMPLEX_O5
						std::vector<O5>(1, { rand(), rand(), rand(), rand() }), // COMPLEX_O3
						std::vector<O5>(2, { rand(), rand(), rand(), rand() }), 
						std::vector<O5>(3, { rand(), rand(), rand(), rand() }), 
						std::vector<O5>(4, { rand(), rand(), rand(), rand() })},
					O3{ 1.f * i, 2.f * i, (uint8_t)i },
					O4{ (char)(8 * i), (short)(16 * i), 32.f * i, (int)(32 * i), 64.0 * i }
				)
			);
		}

		// getting buffer size
		auto buf_size = SerDesLittle::payload_size(serial_src);

		// setting buffer
		buf[0].resize(buf_size);

		// run serialization src to buffer
		SerDesLittle::serialize(buf[0].data(), serial_src);

		// run deserialization buffer to complex structure
		SerDesLittle::deserialize(deserial_dst, buf[0].data());

		printf("<%s> : %s\n",
			typeid(deserial_dst).name(),
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
			return EXIT_FAILURE;
		}
		else
			printf("buf[%u] : %d compare pass\n", (uint32_t)buf_size, compare);

	}
    
    return EXIT_FAILURE;
}