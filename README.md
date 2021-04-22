# CPP_SERDES

This code serialize/deserialize a complex C++ type consisting of std::tuple, std::vector, std::array, etc.

## Requires

The minimum requirement is C++14.

## Simple import

1 header only library. you type just only #include "[serializer_deserializer.hpp](https://github.com/labolas89/cpp_serdes/blob/master/include/serializer_deserializer.hpp)"

## Usage

```c++
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
		SerDesLittle::to_string(dest) << std::endl;
}
```

## Test

There is a pre-written test code.
Please refer to the #include "[test.cpp](https://github.com/labolas89/cpp_serdes/blob/master/src/test.cpp)" file. 

This code is tested with several complex C++ type combinations, and additional explanatory comments are included.

## Test Build

Cmake build was tested on MSVC2017 and Linux (Ubuntu 16.04) G++ 5.4.

```bash
$ git clone git@github.com:labolas89/cpp_serdes.git
$ mkdir build && cd build
$ cmake ..
$ make
```

It will output something like this:

```c++
$ ./TEST_SERDES
deserialize[34] <std::tuple<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::vector<int, std::allocator<int> >, double>> :{"source", {1, 2, 3}, 4.000000}

<std::tuple<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::vector<triviallyCopyableStruct, std::allocator<triviallyCopyableStruct> > >> : {"COMPLEX_O6 test", {triviallyCopyableStruct, triviallyCopyableStruct, triviallyCopyableStruct, triviallyCopyableStruct, triviallyCopyableStruct}}
buf[343] : 0 compare pass

<std::vector<std::tuple<unsigned int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::array<std::vector<containCopyableStaticMemberStruct, std::allocator<containCopyableStaticMemberStruct> >, 4ul>, arithmeticStruct, bytePackStruct>, std::allocator<std::tuple<unsigned int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::array<std::vector<containCopyableStaticMemberStruct, std::allocator<containCopyableStaticMemberStruct> >, 4ul>, arithmeticStruct, bytePackStruct> > >> : {{861021530, "COMPLEX_O9 test0", {{containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}}, arithmeticStruct, bytePackStruct}, {1734575198, "COMPLEX_O9 test1", {{containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}}, arithmeticStruct, bytePackStruct}, {1632621729, "COMPLEX_O9 test2", {{containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}, {containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct, containCopyableStaticMemberStruct}}, arithmeticStruct, bytePackStruct}}
buf[736] : 0 compare pass

$
```

User-written structures or classes are not yet parsed by the to_string function.

When the compilers fully support C++20 features, will try to improve them.

