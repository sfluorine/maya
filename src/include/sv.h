#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct StringView_t {
    const char* str;
    size_t len;
} StringView;

StringView sv_from_cstr(const char* str);
StringView sv_strip_by_delim(StringView sv, const char* delim);
StringView sv_chop_by_delim(StringView* sv, const char* delim);
StringView sv_chop_by_string_literal(StringView* sv);
bool sv_equals(StringView lhs, StringView rhs);
