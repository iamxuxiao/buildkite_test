// Formatting library for C++ - formatting library implementation tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#define FMT_NOEXCEPT
#undef FMT_SHARED
#include "test-assert.h"

// Include format.cc instead of format.h to test implementation.
#include <algorithm>
#include <cstring>

#include "../src/format.cc"
#include "fmt/printf.h"
#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

#ifdef _WIN32
#include <windows.h>
#undef max
#endif

using fmt::detail::bigint;
using fmt::detail::fp;
using fmt::detail::max_value;

static_assert(!std::is_copy_constructible<bigint>::value, "");
static_assert(!std::is_copy_assignable<bigint>::value, "");

TEST(BigIntTest, Construct) {
  EXPECT_EQ("", fmt::format("{}", bigint()));
  EXPECT_EQ("42", fmt::format("{}", bigint(0x42)));
  EXPECT_EQ("123456789abcedf0", fmt::format("{}", bigint(0x123456789abcedf0)));
}

TEST(BigIntTest, Compare) {
  bigint n1(42);
  bigint n2(42);
  EXPECT_EQ(compare(n1, n2), 0);
  n2 <<= 32;
  EXPECT_LT(compare(n1, n2), 0);
  bigint n3(43);
  EXPECT_LT(compare(n1, n3), 0);
  EXPECT_GT(compare(n3, n1), 0);
  bigint n4(42 * 0x100000001);
  EXPECT_LT(compare(n2, n4), 0);
  EXPECT_GT(compare(n4, n2), 0);
}

TEST(BigIntTest, AddCompare) {
  EXPECT_LT(add_compare(bigint(0xffffffff), bigint(0xffffffff), bigint(1) <<= 64), 0);
  EXPECT_LT(add_compare(bigint(1) <<= 32, bigint(1), bigint(1) <<= 96), 0);
  EXPECT_GT(add_compare(bigint(1) <<= 32, bigint(0), bigint(0xffffffff)), 0);
  EXPECT_GT(add_compare(bigint(0), bigint(1) <<= 32, bigint(0xffffffff)), 0);
  EXPECT_GT(add_compare(bigint(42), bigint(1), bigint(42)), 0);
  EXPECT_GT(add_compare(bigint(0xffffffff), bigint(1), bigint(0xffffffff)), 0);
  EXPECT_LT(add_compare(bigint(10), bigint(10), bigint(22)), 0);
  EXPECT_LT(add_compare(bigint(0x100000010), bigint(0x100000010), bigint(0x300000010)), 0);
  EXPECT_GT(add_compare(bigint(0x1ffffffff), bigint(0x100000002), bigint(0x300000000)), 0);
  EXPECT_EQ(add_compare(bigint(0x1ffffffff), bigint(0x100000002), bigint(0x300000001)), 0);
  EXPECT_LT(add_compare(bigint(0x1ffffffff), bigint(0x100000002), bigint(0x300000002)), 0);
  EXPECT_LT(add_compare(bigint(0x1ffffffff), bigint(0x100000002), bigint(0x300000003)), 0);
}

TEST(BigIntTest, ShiftLeft) {
  bigint n(0x42);
  n <<= 0;
  EXPECT_EQ("42", fmt::format("{}", n));
  n <<= 1;
  EXPECT_EQ("84", fmt::format("{}", n));
  n <<= 25;
  EXPECT_EQ("108000000", fmt::format("{}", n));
}

TEST(BigIntTest, Multiply) {
  bigint n(0x42);
  EXPECT_THROW(n *= 0, assertion_failure);
  n *= 1;
  EXPECT_EQ("42", fmt::format("{}", n));
  n *= 2;
  EXPECT_EQ("84", fmt::format("{}", n));
  n *= 0x12345678;
  EXPECT_EQ("962fc95e0", fmt::format("{}", n));
  bigint bigmax(max_value<uint32_t>());
  bigmax *= max_value<uint32_t>();
  EXPECT_EQ("fffffffe00000001", fmt::format("{}", bigmax));
  bigmax.assign(max_value<uint64_t>());
  bigmax *= max_value<uint64_t>();
  EXPECT_EQ("fffffffffffffffe0000000000000001", fmt::format("{}", bigmax));
}

TEST(BigIntTest, Accumulator) {
  fmt::detail::accumulator acc;
  EXPECT_EQ(acc.lower, 0);
  EXPECT_EQ(acc.upper, 0);
  acc.upper = 12;
  acc.lower = 34;
  EXPECT_EQ(static_cast<uint32_t>(acc), 34);
  acc += 56;
  EXPECT_EQ(acc.lower, 90);
  acc += fmt::detail::max_value<uint64_t>();
  EXPECT_EQ(acc.upper, 13);
  EXPECT_EQ(acc.lower, 89);
  acc >>= 32;
  EXPECT_EQ(acc.upper, 0);
  EXPECT_EQ(acc.lower, 13 * 0x100000000);
}

TEST(BigIntTest, Square) {
  bigint n0(0);
  n0.square();
  EXPECT_EQ("0", fmt::format("{}", n0));
  bigint n1(0x100);
  n1.square();
  EXPECT_EQ("10000", fmt::format("{}", n1));
  bigint n2(0xfffffffff);
  n2.square();
  EXPECT_EQ("ffffffffe000000001", fmt::format("{}", n2));
  bigint n3(max_value<uint64_t>());
  n3.square();
  EXPECT_EQ("fffffffffffffffe0000000000000001", fmt::format("{}", n3));
  bigint n4;
  n4.assign_pow10(10);
  EXPECT_EQ("2540be400", fmt::format("{}", n4));
}

TEST(BigIntTest, DivModAssignZeroDivisor) {
  bigint zero(0);
  EXPECT_THROW(bigint(0).divmod_assign(zero), assertion_failure);
  EXPECT_THROW(bigint(42).divmod_assign(zero), assertion_failure);
}

TEST(BigIntTest, DivModAssignSelf) {
  bigint n(100);
  EXPECT_THROW(n.divmod_assign(n), assertion_failure);
}

TEST(BigIntTest, DivModAssignUnaligned) {
  // (42 << 340) / pow(10, 100):
  bigint n1(42);
  n1 <<= 340;
  bigint n2;
  n2.assign_pow10(100);
  int result = n1.divmod_assign(n2);
  EXPECT_EQ(result, 9406);
  EXPECT_EQ("10f8353019583bfc29ffc8f564e1b9f9d819dbb4cf783e4507eca1539220p96", fmt::format("{}", n1));
}

TEST(BigIntTest, DivModAssign) {
  // 100 / 10:
  bigint n1(100);
  int result = n1.divmod_assign(bigint(10));
  EXPECT_EQ(result, 10);
  EXPECT_EQ("0", fmt::format("{}", n1));
  // pow(10, 100) / (42 << 320):
  n1.assign_pow10(100);
  result = n1.divmod_assign(bigint(42) <<= 320);
  EXPECT_EQ(result, 111);
  EXPECT_EQ("13ad2594c37ceb0b2784c4ce0bf38ace408e211a7caab24308a82e8f10p96", fmt::format("{}", n1));
  // 42 / 100:
  bigint n2(42);
  n1.assign_pow10(2);
  result = n2.divmod_assign(n1);
  EXPECT_EQ(result, 0);
  EXPECT_EQ("2a", fmt::format("{}", n2));
}

template <bool is_iec559>
void run_double_tests() {
  fmt::print("warning: double is not IEC559, skipping FP tests\n");
}

template <>
void run_double_tests<true>() {
  // Construct from double.
  EXPECT_EQ(fp(1.23), fp(0x13ae147ae147aeu, -52));
}

TEST(FPTest, DoubleTests) { run_double_tests<std::numeric_limits<double>::is_iec559>(); }

TEST(FPTest, Normalize) {
  const auto v = fp(0xbeef, 42);
  auto normalized = normalize(v);
  EXPECT_EQ(0xbeef000000000000, normalized.f);
  EXPECT_EQ(-6, normalized.e);
}

TEST(FPTest, Multiply) {
  auto v = fp(123ULL << 32, 4) * fp(56ULL << 32, 7);
  EXPECT_EQ(v.f, 123u * 56u);
  EXPECT_EQ(v.e, 4 + 7 + 64);
  v = fp(123ULL << 32, 4) * fp(567ULL << 31, 8);
  EXPECT_EQ(v.f, (123 * 567 + 1u) / 2);
  EXPECT_EQ(v.e, 4 + 8 + 64);
}

TEST(FPTest, GetCachedPower) {
  using limits = std::numeric_limits<double>;
  for (auto exp = limits::min_exponent; exp <= limits::max_exponent; ++exp) {
    int dec_exp = 0;
    auto fp = fmt::detail::get_cached_power(exp, dec_exp);
    bigint exact, cache(fp.f);
    if (dec_exp >= 0) {
      exact.assign_pow10(dec_exp);
      if (fp.e <= 0)
        exact <<= -fp.e;
      else
        cache <<= fp.e;
      exact.align(cache);
      cache.align(exact);
      auto exact_str = fmt::format("{}", exact);
      auto cache_str = fmt::format("{}", cache);
      EXPECT_EQ(exact_str.size(), cache_str.size());
      EXPECT_EQ(exact_str.substr(0, 15), cache_str.substr(0, 15));
      int diff = cache_str[15] - exact_str[15];
      if (diff == 1)
        EXPECT_GT(exact_str[16], '8');
      else
        EXPECT_EQ(diff, 0);
    } else {
      cache.assign_pow10(-dec_exp);
      cache *= fp.f + 1;  // Inexact check.
      exact.assign(1);
      exact <<= -fp.e;
      exact.align(cache);
      auto exact_str = fmt::format("{}", exact);
      auto cache_str = fmt::format("{}", cache);
      EXPECT_EQ(exact_str.size(), cache_str.size());
      EXPECT_EQ(exact_str.substr(0, 16), cache_str.substr(0, 16));
    }
  }
}

TEST(FPTest, DragonboxMaxK) {
  using fmt::detail::dragonbox::floor_log10_pow2;
  using float_info = fmt::detail::dragonbox::float_info<float>;
  EXPECT_EQ(fmt::detail::const_check(float_info::max_k),
            float_info::kappa - floor_log10_pow2(float_info::min_exponent - float_info::significand_bits));
  using double_info = fmt::detail::dragonbox::float_info<double>;
  EXPECT_EQ(fmt::detail::const_check(double_info::max_k),
            double_info::kappa - floor_log10_pow2(double_info::min_exponent - double_info::significand_bits));
}

TEST(FPTest, GetRoundDirection) {
  using fmt::detail::get_round_direction;
  using fmt::detail::round_direction;
  EXPECT_EQ(round_direction::down, get_round_direction(100, 50, 0));
  EXPECT_EQ(round_direction::up, get_round_direction(100, 51, 0));
  EXPECT_EQ(round_direction::down, get_round_direction(100, 40, 10));
  EXPECT_EQ(round_direction::up, get_round_direction(100, 60, 10));
  for (size_t i = 41; i < 60; ++i) EXPECT_EQ(round_direction::unknown, get_round_direction(100, i, 10));
  uint64_t max = max_value<uint64_t>();
  EXPECT_THROW(get_round_direction(100, 100, 0), assertion_failure);
  EXPECT_THROW(get_round_direction(100, 0, 100), assertion_failure);
  EXPECT_THROW(get_round_direction(100, 0, 50), assertion_failure);
  // Check that remainder + error doesn't overflow.
  EXPECT_EQ(round_direction::up, get_round_direction(max, max - 1, 2));
  // Check that 2 * (remainder + error) doesn't overflow.
  EXPECT_EQ(round_direction::unknown, get_round_direction(max, max / 2 + 1, max / 2));
  // Check that remainder - error doesn't overflow.
  EXPECT_EQ(round_direction::unknown, get_round_direction(100, 40, 41));
  // Check that 2 * (remainder - error) doesn't overflow.
  EXPECT_EQ(round_direction::up, get_round_direction(max, max - 1, 1));
}

TEST(FPTest, FixedHandler) {
  struct handler : fmt::detail::fixed_handler {
    char buffer[10];
    handler(int prec = 0) : fmt::detail::fixed_handler() {
      buf = buffer;
      precision = prec;
    }
  };
  int exp = 0;
  handler().on_digit('0', 100, 99, 0, exp, false);
  EXPECT_THROW(handler().on_digit('0', 100, 100, 0, exp, false), assertion_failure);
  namespace digits = fmt::detail::digits;
  EXPECT_EQ(handler(1).on_digit('0', 100, 10, 10, exp, false), digits::error);
  // Check that divisor - error doesn't overflow.
  EXPECT_EQ(handler(1).on_digit('0', 100, 10, 101, exp, false), digits::error);
  // Check that 2 * error doesn't overflow.
  uint64_t max = max_value<uint64_t>();
  EXPECT_EQ(handler(1).on_digit('0', max, 10, max - 1, exp, false), digits::error);
}

TEST(FPTest, GrisuFormatCompilesWithNonIEEEDouble) {
  fmt::memory_buffer buf;
  format_float(0.42, -1, fmt::detail::float_specs(), buf);
}

template <typename T>
struct value_extractor {
  T operator()(T value) { return value; }

  template <typename U>
  FMT_NORETURN T operator()(U) {
    throw std::runtime_error(fmt::format("invalid type {}", typeid(U).name()));
  }

#if FMT_USE_INT128
  // Apple Clang does not define typeid for __int128_t and __uint128_t.
  FMT_NORETURN T operator()(fmt::detail::int128_t) { throw std::runtime_error("invalid type __int128_t"); }

  FMT_NORETURN T operator()(fmt::detail::uint128_t) { throw std::runtime_error("invalid type __uint128_t"); }
#endif
};

TEST(FormatTest, ArgConverter) {
  long long value = max_value<long long>();
  auto arg = fmt::detail::make_arg<fmt::format_context>(value);
  fmt::visit_format_arg(fmt::detail::arg_converter<long long, fmt::format_context>(arg, 'd'), arg);
  EXPECT_EQ(value, fmt::visit_format_arg(value_extractor<long long>(), arg));
}

TEST(FormatTest, StrError) {
  char *message = nullptr;
  char buffer[BUFFER_SIZE];
  EXPECT_ASSERT(fmt::detail::safe_strerror(EDOM, message = nullptr, 0), "invalid buffer");
  EXPECT_ASSERT(fmt::detail::safe_strerror(EDOM, message = buffer, 0), "invalid buffer");
  buffer[0] = 'x';
#if defined(_GNU_SOURCE) && !defined(__COVERITY__)
  // Use invalid error code to make sure that safe_strerror returns an error
  // message in the buffer rather than a pointer to a static string.
  int error_code = -1;
#else
  int error_code = EDOM;
#endif

  int result = fmt::detail::safe_strerror(error_code, message = buffer, BUFFER_SIZE);
  EXPECT_EQ(result, 0);
  size_t message_size = std::strlen(message);
  EXPECT_GE(BUFFER_SIZE - 1u, message_size);
  EXPECT_EQ(get_system_error(error_code), message);

// safe_strerror never uses buffer on MinGW.
#if !defined(__MINGW32__) && !defined(__sun)
  result = fmt::detail::safe_strerror(error_code, message = buffer, message_size);
  EXPECT_EQ(ERANGE, result);
  result = fmt::detail::safe_strerror(error_code, message = buffer, 1);
  EXPECT_EQ(buffer, message);  // Message should point to buffer.
  EXPECT_EQ(ERANGE, result);
  EXPECT_STREQ("", message);
#endif
}

TEST(FormatTest, FormatErrorCode) {
  std::string msg = "error 42", sep = ": ";
  {
    fmt::memory_buffer buffer;
    format_to(buffer, "garbage");
    fmt::detail::format_error_code(buffer, 42, "test");
    EXPECT_EQ("test: " + msg, to_string(buffer));
  }
  {
    fmt::memory_buffer buffer;
    std::string prefix(fmt::inline_buffer_size - msg.size() - sep.size() + 1, 'x');
    fmt::detail::format_error_code(buffer, 42, prefix);
    EXPECT_EQ(msg, to_string(buffer));
  }
  int codes[] = {42, -1};
  for (size_t i = 0, n = sizeof(codes) / sizeof(*codes); i < n; ++i) {
    // Test maximum buffer size.
    msg = fmt::format("error {}", codes[i]);
    fmt::memory_buffer buffer;
    std::string prefix(fmt::inline_buffer_size - msg.size() - sep.size(), 'x');
    fmt::detail::format_error_code(buffer, codes[i], prefix);
    EXPECT_EQ(prefix + sep + msg, to_string(buffer));
    size_t size = fmt::inline_buffer_size;
    EXPECT_EQ(size, buffer.size());
    buffer.resize(0);
    // Test with a message that doesn't fit into the buffer.
    prefix += 'x';
    fmt::detail::format_error_code(buffer, codes[i], prefix);
    EXPECT_EQ(msg, to_string(buffer));
  }
}

TEST(FormatTest, ComputeWidth) {
  EXPECT_EQ(4, fmt::detail::compute_width(fmt::basic_string_view<fmt::detail::char8_type>(
                   reinterpret_cast<const fmt::detail::char8_type *>("ёжик"))));
}

// Tests fmt::detail::count_digits for integer type Int.
template <typename Int>
void test_count_digits() {
  for (Int i = 0; i < 10; ++i) EXPECT_EQ(1u, fmt::detail::count_digits(i));
  for (Int i = 1, n = 1, end = max_value<Int>() / 10; n <= end; ++i) {
    n *= 10;
    EXPECT_EQ(i, fmt::detail::count_digits(n - 1));
    EXPECT_EQ(i + 1, fmt::detail::count_digits(n));
  }
}

TEST(UtilTest, CountDigits) {
  test_count_digits<uint32_t>();
  test_count_digits<uint64_t>();
}

TEST(UtilTest, WriteFallbackUIntPtr) {
  std::string s;
  fmt::detail::write_ptr<char>(std::back_inserter(s), fmt::detail::fallback_uintptr(reinterpret_cast<void *>(0xface)),
                               nullptr);
  EXPECT_EQ(s, "0xface");
}

#ifdef _WIN32
TEST(UtilTest, WriteConsoleSignature) {
  decltype(WriteConsoleW) *p = fmt::detail::WriteConsoleW;
  (void)p;
}
#endif
