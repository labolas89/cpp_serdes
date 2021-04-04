#pragma once
#ifndef __SERIALIZER_DESERIALIZER_HPP__
#define __SERIALIZER_DESERIALIZER_HPP__

#include <cassert>
#include <vector>
#include <array>
#include <tuple>
#include <type_traits>
#include <string>
#include <typeinfo>

namespace serdes {

#define UNUSED(x) (void)(x)

// ---------------------------
// Type traits
// ---------------------------

// Dynamic container
// http://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector
template<typename T, typename _ = void>
struct is_container : std::false_type {};

template<typename... Ts>
struct is_container_helper {};

template<typename T>
struct is_container<
	T,
	std::conditional_t<
	false,
	is_container_helper<
	typename T::value_type,
	typename T::size_type,
	typename T::allocator_type,
	typename T::iterator,
	typename T::const_iterator,
	decltype(std::declval<T>().size()),
	decltype(std::declval<T>().data()),
	decltype(std::declval<T>().begin()),
	decltype(std::declval<T>().end()),
	decltype(std::declval<T>().cbegin()),
	decltype(std::declval<T>().cend())
	>,
	void
	>
> : public std::true_type{};

template<typename T>
static constexpr bool is_container_v = is_container<T>::value;

static_assert(is_container_v<std::vector<float>>, "");
static_assert(is_container_v<std::string>, "");
static_assert(!is_container_v<std::array<float, 4>>, "");
static_assert(!is_container_v<float>, "");

// Scalar
template<typename T>
static constexpr bool is_scalar_v = std::is_scalar<std::remove_reference_t<T>>::value &&
!std::is_pointer<std::remove_reference_t<T>>::value;

static_assert(is_scalar_v<float>, "");
static_assert(!is_scalar_v<uint32_t*>, "");
static_assert(!is_scalar_v<std::vector<float>>, "");

// Tuple
template <typename T>
struct is_std_tuple : std::false_type {};
template <typename... Args>
struct is_std_tuple<std::tuple<Args...>> : std::true_type {};

template<typename T>
static constexpr bool is_std_tuple_v = is_std_tuple<T>::value;

static_assert(is_std_tuple_v<std::tuple<uint32_t, float>>, "");
static_assert(!is_std_tuple_v<uint32_t>, "");

// Array
template <typename T>
struct is_std_array : std::false_type {};
template <typename V, size_t N>
struct is_std_array<std::array<V, N>> : std::true_type {};

template <typename T>
static constexpr bool is_std_array_v = is_std_array<T>::value;

static_assert(is_std_array_v<std::array<uint32_t, 10>>, "");
static_assert(!is_std_array_v<std::vector<uint32_t>>, "");

// C string
// http://stackoverflow.com/questions/8097534/type-trait-for-strings
template <typename T>
struct is_c_string : public
	std::integral_constant<bool,
	std::is_same<char*, typename std::remove_reference<T>::type>::value ||
	std::is_same<const char*, typename std::remove_reference<T>::type>::value
	> {};

template<typename T>
static constexpr bool is_c_string_v = is_c_string<T>::value;

static_assert(is_c_string_v<char*>, "");
static_assert(is_c_string_v<const char*>, "");
static_assert(!is_c_string_v<std::string>, "");

} // namespace serdes

//--------------------------------------------------------------------------------------------------
// New Serializer/Deserializer
//--------------------------------------------------------------------------------------------------

template<typename buf_t = uint8_t, bool big_endian = false>
class SerDes {
private:
	template<typename Tp>
	static constexpr bool is_serdes_special = (
		serdes::is_container_v<Tp> ||
		serdes::is_std_array_v<Tp> ||
		serdes::is_std_tuple_v<Tp> ||
		serdes::is_c_string_v<Tp>);

public:

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_container_v<std::decay_t<Tp>>,
		bool> is_serdesable() {
		return is_serdesable<std::decay_t<Tp>::value_type>();
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_array_v<std::decay_t<Tp>>,
		bool> is_serdesable() {
		return is_serdesable<std::decay_t<Tp>::value_type>();
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_tuple_v<std::decay_t<Tp>>,
		bool> is_serdesable() {
		return is_tuple_serdesable<Tp, 0>();
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_c_string_v<std::decay_t<Tp>>,
		bool> is_serdesable() {
		return true;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!is_serdes_special<Tp>,
		bool> is_serdesable() {
		return std::is_trivially_copyable<Tp>::value;
	}

	template<class Tup, size_t idx>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		!(idx < std::tuple_size<Tup>::value),
		bool> is_tuple_serdesable() {
		// do notting
		return std::tuple_size<Tup>::value > 0;
	}

	template<class Tup, size_t idx>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		(idx < std::tuple_size<Tup>::value),
		bool> is_tuple_serdesable() {
		bool ok = std::tuple_size<Tup>::value ? is_serdesable<std::tuple_element<idx, Tup>::type>() : false;
		return ok && is_tuple_serdesable<Tup, idx + 1>();
	}

	template<typename Tp>
	static constexpr bool is_serdesable_v = is_serdesable<Tp>();

public:
	typedef const buf_t* const __restrict deser_src;

	template<typename Tp>
	static inline constexpr std::enable_if_t<(big_endian && !std::is_floating_point<Tp>::value),
		Tp> extract(deser_src ptr) {
		union extract_un
		{
			buf_t dst_mem[sizeof(Tp)];
			Tp ret_tp;
		} dst = {0, };
		for (uint32_t i = 0; i < sizeof(Tp); i++)
			dst.dst_mem[i] = ptr[sizeof(Tp) - 1 - i];
		return dst.ret_tp;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!(big_endian && !std::is_floating_point<Tp>::value),
		Tp> extract(deser_src ptr) {
		union extract_un
		{
			buf_t dst_mem[sizeof(Tp)];
			Tp ret_tp;
		} dst = {0, };
		for (uint32_t i = 0; i < sizeof(Tp); i++)
			dst.dst_mem[i] = ptr[i];
		return dst.ret_tp;
	}
	
	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_container_v<std::decay_t<Tp>>,
		size_t>	deserialize(Tp& vec, deser_src ptr) {
		auto elem_nums = extract<uint32_t>(ptr);
		size_t cursor = sizeof(uint32_t);
		vec.resize(elem_nums);
		for (auto& elem : vec)
			cursor += deserialize(elem, ptr + cursor);
		return cursor;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_array_v<std::decay_t<Tp>>,
		size_t>	deserialize(Tp& arr, deser_src ptr) {
		size_t cursor = 0;
		for (auto& elem : arr)
			cursor += deserialize(elem, ptr + cursor);
		return cursor;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_tuple_v<std::decay_t<Tp>>,
		size_t> deserialize(Tp& tup, deser_src ptr) {
		return dump_buffer_to_tuple<Tp, 0>(tup, ptr);
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_c_string_v<std::decay_t<Tp>>,
		size_t> deserialize(Tp& c_str, deser_src ptr) {
		auto elem_nums = extract<uint32_t>(ptr);
		constexpr size_t cursor = sizeof(uint32_t);
		if (c_str) delete c_str;
		c_str = new Tp[elem_nums + 1];
		memcpy(c_str, ptr + cursor, elem_nums);
		c_str[elem_nums] = '\0';
		return cursor + elem_nums;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!is_serdes_special<Tp>,
		size_t> deserialize(Tp& dst, deser_src ptr) {
		dst = extract<Tp>(ptr);
		return sizeof(Tp);
	}

	template<class Tup, size_t idx>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		!(idx < std::tuple_size<Tup>::value),
		size_t> dump_buffer_to_tuple(Tup&, deser_src) {
		// do notting
		return (size_t)0;
	}

	template<class Tup, size_t idx = 0>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		(idx < std::tuple_size<Tup>::value),
		size_t> dump_buffer_to_tuple(Tup& tup, deser_src ptr) {
		size_t cursor_move = deserialize(std::get<idx>(tup), ptr);
		return cursor_move + dump_buffer_to_tuple<Tup, idx + 1>(tup, ptr + cursor_move);
	}

	static constexpr size_t to_string_repeat_limit = 4;

	template<typename Tp>
	static inline constexpr std::enable_if_t<
		serdes::is_container_v<std::decay_t<Tp>> ||
		serdes::is_std_array_v<std::decay_t<Tp>>,
		std::string> to_string(const Tp& vec) {
		std::string ret = "{";
		size_t repeat = 0;
		for (auto& elem : vec) {
			if (repeat++ < to_string_repeat_limit)
				ret += to_string(elem) + (repeat != vec.size() ? ", " : "");
			else {
				ret += "...";
				break;
			}
		}
		ret += "}";
		return ret;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_c_string_v<std::decay_t<Tp>>,
		std::string> to_string(const Tp& c_str) {
		return std::string(c_str);
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_tuple_v<std::decay_t<Tp>>,
		std::string> to_string(const Tp& tup) {
		return "{" + tuple_to_string(tup) + "}";
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!is_serdes_special<Tp> && std::is_arithmetic<Tp>::value,
		std::string> to_string(const Tp& src) {
		return std::to_string(src);
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!is_serdes_special<Tp> && !std::is_arithmetic<Tp>::value,
		std::string> to_string(const Tp& src) {

		// not working
#if 0 //__cplusplus > 201402L && (defined(__GNUC__) ? __GNUC__ > 6 : true)
		auto&& tup = to_tuple(src);
		if constexpr (std::tuple_size<std::decay_t<decltype(tup)>>::value > 0) {
			return to_string(tup);
		}
		else // cannot convert structure to tuple
			return typeid(Tp).name();
#else
		UNUSED(src);
		return typeid(Tp).name();
#endif
	}


	// not working
#if 0 // __cplusplus > 201402L && (defined(__GNUC__) ? __GNUC__ > 6 : true)

public:
	//private:
	struct any {
		template<class T>
		constexpr operator T(); // non explicit
	};

	template <class T, class... TArgs>
	static inline decltype(void(T{ std::declval<TArgs>()... }), std::true_type{}) test_is_braces_constructible(int);

	template <class, class...>
	static inline std::false_type test_is_braces_constructible(...);

	template <class T, class... TArgs>
	using is_braces_constructible = decltype(test_is_braces_constructible<T, TArgs...>(0));

public:

	// non-standard : working in clang compilier
	// https://stackoverflow.com/questions/49340829/how-to-write-a-concept-for-structured-bindings

	// working not good
	// https://stackoverflow.com/questions/39768517/structured-bindings-width

	// forum
	// https://www.reddit.com/r/cpp/comments/dz1sgk/find_the_number_of_structured_bindings_for_any/?utm_source=share&utm_medium=web2x

	template <class T, class... TArgs>
	static inline constexpr bool is_braces_constructible_v = is_braces_constructible<T, TArgs...>::value && std::is_constructible_v<T, TArgs...>;

	// only structures with up to 16 elements are supported 
	template<class T>
	static inline constexpr auto to_tuple(T&& object) noexcept {
		using type = std::decay_t<T>;
		if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC, pD, pE, pF] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC, pD, pE, pF);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC, pD, pE] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC, pD, pE);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC, pD] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC, pD);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB, pC);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA, pB);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, pA);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8, p9] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7, p8] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7, p8);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6, p7] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6, p7);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5, p6] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5, p6);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4, p5] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4, p5);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any, any>) {
			auto&&[p0, p1, p2, p3, p4] = object;
			return std::forward_as_tuple(p0, p1, p2, p3, p4);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any, any>) {
			auto&&[p0, p1, p2, p3] = object;
			return std::forward_as_tuple(p0, p1, p2, p3);
		}
		else if constexpr (is_braces_constructible_v<type, any, any, any>) {
			auto&&[p0, p1, p2] = object;
			return std::forward_as_tuple(p0, p1, p2);
		}
		else if constexpr (is_braces_constructible_v<type, any, any>) {
			auto&&[p0, p1] = object;
			return std::forward_as_tuple(p0, p1);
		}
		else {
			return std::forward_as_tuple();
		}
	}

#endif

	template<class Tup, size_t idx>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		!(idx < std::tuple_size<Tup>::value),
		std::string> tuple_to_string(const Tup&) {
		// do notting
		return std::string();
	}

	template<class Tup, size_t idx = 0>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		(idx < std::tuple_size<Tup>::value),
		std::string> tuple_to_string(const Tup& tup) {
		constexpr const char* delimiter = idx == 0 ? "" : ", ";
		return delimiter + to_string(std::get<idx>(tup)) + tuple_to_string<Tup, idx + 1>(tup);
	}


public:
	typedef buf_t* const __restrict ser_dst;

	template<typename Tp>
	static inline constexpr std::enable_if_t<big_endian && !std::is_floating_point<Tp>::value,
		void> inject(ser_dst dst, const Tp& src) {
#ifdef __GNUC__ 
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-align"
#endif
		const buf_t* src_ptr = (const buf_t*)(&src);
		for (uint32_t i = 0; i < sizeof(Tp); i++)
			dst[i] = src_ptr[sizeof(Tp) - 1 - i];
#ifdef __GNUC__ 
	#pragma GCC diagnostic pop
#endif
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!(big_endian && !std::is_floating_point<Tp>::value),
		void> inject(ser_dst dst, const Tp& src) {
		static_assert(std::is_trivially_copyable<Tp>::value, "this type is not trivially copyable");
#ifdef __GNUC__ 
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-align"
#endif
		const buf_t* src_ptr = (const buf_t*)(&src);
		for (uint32_t i = 0; i < sizeof(Tp); i++)
			dst[i] = src_ptr[i];
		//*((Tp* const __restrict)dst) = src;
#ifdef __GNUC__ 
	#pragma GCC diagnostic pop
#endif
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_container_v<std::decay_t<Tp>>,
		size_t>	serialize(ser_dst ptr, const Tp& vec) {
		inject<uint32_t>(ptr, (uint32_t)vec.size());
		size_t cursor = sizeof(uint32_t);
		for (auto& elem : vec)
			cursor += serialize(ptr + cursor, elem);
		return cursor;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_array_v<std::decay_t<Tp>>,
		size_t>	serialize(ser_dst ptr, const Tp& arr) {
		size_t cursor = 0;
		for (auto& elem : arr)
			cursor += serialize(ptr + cursor, elem);
		return cursor;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_tuple_v<std::decay_t<Tp>>,
		size_t> serialize(ser_dst ptr, const Tp& tup) {
		return dump_tuple_to_buffer<Tp, 0>(ptr, tup);
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_c_string_v<std::decay_t<Tp>>,
		size_t> serialize(ser_dst ptr, const Tp& c_str) {
		return serialize(ptr, std::string(c_str));
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!is_serdes_special<Tp>,
		size_t> serialize(ser_dst ptr, const Tp& src) {
		inject(ptr, src);
		return sizeof(Tp);
	}

	template<class Tup, size_t idx>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		!(idx < std::tuple_size<Tup>::value),
		size_t> dump_tuple_to_buffer(ser_dst, const Tup&) {
		// do notting
		return (size_t)0;
	}

	template<class Tup, size_t idx = 0>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		(idx < std::tuple_size<Tup>::value),
		size_t> dump_tuple_to_buffer(ser_dst ptr, const Tup& tup) {
		size_t cursor_move = serialize(ptr, std::get<idx>(tup));
		return cursor_move + dump_tuple_to_buffer<Tup, idx + 1>(ptr + cursor_move, tup);
	}



	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_container_v<std::decay_t<Tp>>,
		size_t>	payload_size(const Tp& vec) {
		size_t cursor = sizeof(uint32_t);
		for (auto& elem : vec)
			cursor += payload_size(elem);
		return cursor;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_array_v<std::decay_t<Tp>>,
		size_t>	payload_size(const Tp& arr) {
		size_t cursor = 0;
		for (auto& elem : arr)
			cursor += payload_size(elem);
		return cursor;
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_std_tuple_v<std::decay_t<Tp>>,
		size_t> payload_size(const Tp& tup) {
		return tuple_payload_size<Tp, 0>(tup);
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<serdes::is_c_string_v<std::decay_t<Tp>>,
		size_t> payload_size(const Tp& c_str) {
		return payload_size(std::string(c_str).size());
	}

	template<typename Tp>
	static inline constexpr std::enable_if_t<!is_serdes_special<Tp>,
		size_t> payload_size(const Tp&) {
		static_assert(std::is_trivially_copyable<Tp>::value, "this type is not trivially copyable");
		return sizeof(Tp);
	}

	template<class Tup, size_t idx>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		!(idx < std::tuple_size<Tup>::value),
		size_t> tuple_payload_size(const Tup&) {
		// do notting
		return (size_t)0;
	}

	template<class Tup, size_t idx = 0>
	static inline constexpr std::enable_if_t<
		serdes::is_std_tuple_v<Tup> &&
		(idx < std::tuple_size<Tup>::value),
		size_t> tuple_payload_size(const Tup& tup) {
		size_t cursor_move = payload_size(std::get<idx>(tup));
		return cursor_move + tuple_payload_size<Tup, idx + 1>(tup);
	}

};

typedef SerDes<uint8_t, false> SerDesLittle;
typedef SerDes<uint8_t, true> SerDesBig;

#pragma pack(push, 1) 
typedef struct length_header {
	static constexpr int sof_bits = 8;
	static constexpr int length_bits = 18;
	static constexpr int checksum_bits = 6;
	static constexpr uint32_t checksum_mask = (1 << checksum_bits) - 1;
	static constexpr uint32_t max_packet_size = (1 << length_bits) - 1;
	static_assert((sof_bits + length_bits + checksum_bits) == (sizeof(uint32_t) * 8), "not matching bit width");

	length_header() = default;

	length_header(size_t size)
		: sof(0x77)
		, length(size)
		, checksum(cal_checksum(size)) {}


#ifdef __GNUC__ 
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wcast-align"
#endif
	length_header(uint8_t* data)
		: sof(*((uint32_t*)data))
		, length(*((uint32_t*)(data + (sof_bits / 8))))
		, checksum(cal_checksum()) {}
#ifdef __GNUC__ 
	#pragma GCC diagnostic pop
#endif

	static_assert(!(sof_bits % 8), "cannot align length_header::sof");

	inline uint32_t cal_checksum() {
		return (sof + (length >> (6 * 0)) + (length >> (6 * 1)) + (length >> (6 * 2))) & checksum_mask;
	}

	static inline uint32_t cal_checksum(uint32_t size) {
		assert(size <= max_packet_size && "cannot calculate checksum");
		return (0x77 + (size >> (6 * 0)) + (size >> (6 * 1)) + (size >> (6 * 2))) & checksum_mask;
	}

	bool check() {
		return checksum == cal_checksum(length);
	}

	uint32_t sof : sof_bits;
	uint32_t length : length_bits;
	uint32_t checksum : checksum_bits;
} length_header_t;
#pragma pack(pop)

typedef std::tuple<length_header_t, uint16_t, uint16_t> header_type;

//--------------------------------------------------------------------------------------------------
// Commands serializer
//--------------------------------------------------------------------------------------------------

template<typename buf_t = uint8_t, bool big_endian = false>
class DynamicSerDes {
private:

	template<uint16_t class_id, uint16_t func_id,
		std::size_t... I, typename... Args>
		inline size_t call_command_serializer(std::vector<uint8_t>& buffer,
			std::index_sequence<I...>,
			const std::tuple<Args...>& tup_args) {
		return build_command<class_id, func_id>(buffer, std::get<I>(tup_args)...);
	}

public:
	template<uint16_t class_id, uint16_t func_id, typename Tp0, typename... Args>
	inline typename std::enable_if_t<0 <= sizeof...(Args) && !serdes::is_std_tuple_v<typename std::remove_reference<Tp0>::type>,
	size_t> build_command(std::vector<buf_t>& buffer, Tp0&& arg0, Args&&... args) {
		auto all_arg = std::tuple_cat(std::forward_as_tuple(arg0), std::forward_as_tuple(args)...);
		const size_t all_arg_size = SerDes<buf_t, big_endian>::payload_size(all_arg);
		buffer.resize(sizeof(header_type) + all_arg_size);
		return SerDes<buf_t, big_endian>::serialize(buffer.data(), 
			std::tuple_cat(std::make_tuple(length_header_t(all_arg_size), class_id, func_id), all_arg));
	}

	template<uint16_t class_id, uint16_t func_id, typename... Args>
	inline typename std::enable_if_t< 0 == sizeof...(Args),
	size_t> build_command(std::vector<buf_t>& buffer, Args&&... args) {
		header_type header(length_header_t((size_t)0), class_id, func_id);
		buffer.resize(sizeof(header_type));
		return SerDes<buf_t, big_endian>::serialize(buffer.data(), header);
	}

	template<uint16_t class_id, uint16_t func_id, typename... Args>
	inline size_t build_command(std::vector<uint8_t>& buffer,
		const std::tuple<Args...>& tup_args) {
		return call_command_serializer<class_id, func_id>(buffer,
			std::index_sequence_for<Args...>{}, tup_args);
	}
};


#endif // !__SERIALIZER_DESERIALIZER_HPP__
