#include <string.h>

#include "sv.h"

StringView sv_from_cstr(const char* str) {
    size_t len = 0;
    while (str[len] != 0)
        len++;

    return (StringView) {
        .str = str,
        .len = len,
    };
}

StringView sv_strip_by_delim(StringView sv, const char* delim) {
    const char* ptr = sv.str;
    while (sv.len != 0 && strchr(delim, *ptr) != NULL) {
        ptr++;
        sv.len--;
    }

    return (StringView) {
        .str = ptr,
        .len = sv.len,
    };
}

StringView sv_chop_by_delim(StringView* sv, const char* delim) {
    *sv = sv_strip_by_delim(*sv, delim);

    const char* start = sv->str;
    size_t len = 0;
    while (sv->len != 0 && strchr(delim, *sv->str) == NULL) {
        len++;
        sv->str++;
        sv->len--;
    }

    return (StringView) {
        .str = start,
        .len = len,
    };
}

StringView sv_chop_by_string_literal(StringView* sv) {
    // skip '"'
    sv->str++;

    const char* start = sv->str;
    size_t len = 0;
    while (sv->len != 1 && sv->str[0] != '"') {
        len++;
        sv->str++;
        sv->len--;
    }

    // skip '"'
    sv->str++;

    return (StringView) {
        .str = start,
        .len = len,
    };
}

bool sv_equals(StringView lhs, StringView rhs) {
    if (lhs.len != rhs.len)
        return false;

    // i feel like if i only check one of the arguments length it's one sided so i decided to make a neutral variable lmao :^)
    size_t len = (lhs.len + rhs.len) / 2;
    for (size_t i = 0; i < len; i++) {
        if (lhs.str[i] != rhs.str[i])
            return false;
    }

    return true;
}
