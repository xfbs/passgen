#include "passgen/enum_mapping.h"
#include "passgen/data/token.h"
#include "tests.h"
#include <string.h>

test_result test_enum_mapping_exists(void) {
  assert(passgen_token_state_enum_count == 7);
  assert(passgen_token_state_enum_by_value[0].value == -3);
  assert(
      strcmp(
          passgen_token_state_enum_by_name[0].name,
          "PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD") == 0);

  return test_ok;
}

#define TEST_BY_NAME(enum_name, enum_value) \
  mapping = passgen_enum_by_name(           \
      enum_name##_enum_by_name,             \
      enum_name##_enum_count,               \
      #enum_value);                         \
  assert(mapping);                          \
  assert(mapping->value == enum_value);     \
  assert(strcmp(mapping->name, #enum_value) == 0)

test_result test_enum_mapping_by_name(void) {
  struct passgen_enum_mapping *mapping;

  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_INIT);
  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_ESCAPED);
  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_UNICODE);
  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_UNICODE_PAYLOAD);
  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_ERROR_UNICODE_START);
  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD);
  TEST_BY_NAME(passgen_token_state, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN);

  return test_ok;
}

#undef TEST_BY_NAME

#define TEST_BY_NAME_NONEXISTENT(enum_name, enum_value) \
  mapping = passgen_enum_by_name(                       \
      enum_name##_enum_by_name,                         \
      enum_name##_enum_count,                           \
      #enum_value);                                     \
  assert(mapping == NULL)

test_result test_enum_mapping_by_name_nonexistent(void) {
  struct passgen_enum_mapping *mapping;

  TEST_BY_NAME_NONEXISTENT(passgen_token_state, PASSGEN_TOKEN_WILD);
  TEST_BY_NAME_NONEXISTENT(passgen_token_state, AARDARK_PASSGEN_INVALID);
  TEST_BY_NAME_NONEXISTENT(passgen_token_state, ZZORAK_PASSGEN_INVALID);

  return test_ok;
}

#undef TEST_BY_NAME_NONEXISTENT

#define TEST_BY_VALUE(enum_name, enum_value) \
  mapping = passgen_enum_by_value(           \
      enum_name##_enum_by_value,             \
      enum_name##_enum_count,                \
      enum_value);                           \
  assert(mapping);                           \
  assert(mapping->value == enum_value);      \
  assert(strcmp(mapping->name, #enum_value) == 0)

test_result test_enum_mapping_by_value(void) {
  struct passgen_enum_mapping *mapping;

  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_INIT);
  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_ESCAPED);
  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_UNICODE);
  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_UNICODE_PAYLOAD);
  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_ERROR_UNICODE_START);
  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD);
  TEST_BY_VALUE(passgen_token_state, PASSGEN_TOKEN_ERROR_UNICODE_PAYLOAD_LEN);

  return test_ok;
}

#undef TEST_BY_VALUE

#define TEST_BY_VALUE_NONEXISTENT(enum_name, enum_value) \
  mapping = passgen_enum_by_value(                       \
      enum_name##_enum_by_value,                         \
      enum_name##_enum_count,                            \
      enum_value);                                       \
  assert(mapping == NULL)

test_result test_enum_mapping_by_value_nonexistent(void) {
  struct passgen_enum_mapping *mapping;

  TEST_BY_VALUE_NONEXISTENT(passgen_token_state, -10);
  TEST_BY_VALUE_NONEXISTENT(passgen_token_state, 9);
  TEST_BY_VALUE_NONEXISTENT(passgen_token_state, 123);

  return test_ok;
}

#undef TEST_BY_VALUE_NONEXISTENT
