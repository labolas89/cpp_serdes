
#include "serializer_deserializer.hpp"
#include <stdio.h>
#include <cstring>
#include <typeinfo>

struct O1 {
	int m;
};

struct O2 {
	int m;

	O2(O2 const&) = default; // -> trivially copyable
	O2(int x) : m(x + 1) {}
};

struct O3 {
	//O3() : re(2.f), im(6.f), c('c') {}
	//O3(float RE, float IM, uint8_t C) : re(RE), im(IM), c(C) {}
	float re;
	float im;
	uint8_t c;

	O3() = default;
	O3(O3 const&) = default;
	//O3() : re(1.0), im(0.0) {};

	//F(const F& other) : re(other.re), im(other.im) {}
};

#pragma pack(push, 1)
struct O4 {
	//O4() : s8(8), s16(-16), f32(32), s32(-32), f64(-64) {}
	O4() = default;
	O4(O4 const&) = default;
	char	s8;
	short	s16;
	float	f32;
	int		s32;
	double  f64;
};
#pragma pack(pop)

struct O5
{
	O5() = default;
	//O5() : distance(1.234f), speed(-2.345f), psd(4.12), thres(3.14) {}

	O5(int in0, int in1, int in2, int in3)
		: distance((float)in0), speed((float)in1), psd((float)in2), thres((float)in3) {}
	O5(const O5&) = default;
	float distance;
	float speed;
	float psd;
	float thres;
	static O3 freq2dist_calib[2];
};

class O7 {
public:
	char	s8;
	short	s16;
	float	f32;
};


int main() 
{

    std::vector<uint8_t> buf[2];
    std::vector<std::tuple<uint32_t, std::string, std::array<std::vector<O5>, 4>, O3, O4>> serial_src, deserial_dst;

    for (size_t i = 0; i < 3; i++)
    {
        serial_src.push_back(
            std::make_tuple( 
                (uint32_t)rand(), 
                std::string("test" + std::to_string(i)),
                std::array<std::vector<O5>, 4>{
                    std::vector<O5>(1, {rand(), rand(), rand(), rand()}), 
                    std::vector<O5>(2, {rand(), rand(), rand(), rand()}), 
                    std::vector<O5>(3, {rand(), rand(), rand(), rand()}), 
                    std::vector<O5>(4, {rand(), rand(), rand(), rand()})},
                O3{1.f * i, 2.f * i, (uint8_t)i},
                O4{(char)(8 * i), (short)(16 * i), 32.f * i, (int)(32 * i), 64.0 * i}
            )
        );
    }
    
    auto buf_size = SerDesLittle::payload_size(serial_src);

	buf[0].resize(buf_size);
	SerDesLittle::serialize(buf[0].data(), serial_src);

	SerDesLittle::deserialize(deserial_dst, buf[0].data());
	
    //printf("%s\n", SerDesLittle::to_string(deserial_dst).c_str());
    printf("<%s> : %s\n", typeid(deserial_dst).name(), SerDesLittle::to_string(deserial_dst).c_str());

    buf[1].resize(buf_size);
    SerDesLittle::serialize(buf[1].data(), deserial_dst);

    int compare = memcmp(buf[0].data(), buf[1].data(), buf_size);

    if (compare == 0) {
        printf("buf[%u] : %d compare pass\n", (uint32_t)buf_size, compare);
        return EXIT_SUCCESS;
    }

    printf("buf[%u] : %d compare fail\n", (uint32_t)buf_size, compare);
    return EXIT_FAILURE;
}