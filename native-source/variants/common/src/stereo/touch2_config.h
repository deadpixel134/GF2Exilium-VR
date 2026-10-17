#ifndef GF2VR_PRIVATE_TOUCH2_CONFIG_H
#define GF2VR_PRIVATE_TOUCH2_CONFIG_H

/* Private cold-load parser. Bounded strict JSON, no allocation, CRT, handles or
 * writes except the successful enabled result. Unknown members are accepted;
 * duplicate decoded member names are rejected in every object. Prior member
 * values are rescanned instead of imposing an arbitrary key-count/hash limit.
 */
typedef struct Touch2Json {
    const unsigned char *text;
    unsigned length, position;
} Touch2Json;

typedef struct Touch2ConfigValue {
    unsigned seen, enabled;
} Touch2ConfigValue;

static void touch2_json_ws(Touch2Json *j) {
    while (j->position < j->length) {
        unsigned c = j->text[j->position];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
        ++j->position;
    }
}

static int touch2_json_hex(unsigned c) {
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (int)(c - 'A' + 10);
    return -1;
}

static int touch2_json_u16(Touch2Json *j, unsigned *value) {
    if (j->length - j->position < 4) return 0;
    unsigned n = 0;
    for (unsigned i = 0; i < 4; ++i) {
        int digit = touch2_json_hex(j->text[j->position++]);
        if (digit < 0) return 0;
        n = (n << 4) | (unsigned)digit;
    }
    *value = n;
    return 1;
}

/* Cursor starts after opening quote. Return 1=one Unicode scalar, 0=end,
 * -1=invalid. Escape decoding makes literal UTF8 and \u surrogate pairs equal.
 */
static int touch2_json_scalar(Touch2Json *j, unsigned *value) {
    if (j->position >= j->length) return -1;
    unsigned c = j->text[j->position++];
    if (c == '"') return 0;
    if (c < 0x20) return -1;
    if (c == '\\') {
        if (j->position >= j->length) return -1;
        c = j->text[j->position++];
        switch (c) {
        case '"': case '\\': case '/': *value = c; return 1;
        case 'b': *value = 8; return 1;
        case 'f': *value = 12; return 1;
        case 'n': *value = 10; return 1;
        case 'r': *value = 13; return 1;
        case 't': *value = 9; return 1;
        case 'u': {
            if (!touch2_json_u16(j, &c)) return -1;
            if (c >= 0xd800 && c <= 0xdbff) {
                if (j->length - j->position < 6 ||
                    j->text[j->position++] != '\\' || j->text[j->position++] != 'u') return -1;
                unsigned low;
                if (!touch2_json_u16(j, &low) || low < 0xdc00 || low > 0xdfff) return -1;
                c = 0x10000 + ((c - 0xd800) << 10) + low - 0xdc00;
            } else if (c >= 0xdc00 && c <= 0xdfff) return -1;
            *value = c; return 1;
        }
        default: return -1;
        }
    }
    if (c < 0x80) { *value = c; return 1; }
    unsigned remaining, minimum, scalar;
    if (c >= 0xc2 && c <= 0xdf) { remaining = 1; minimum = 0x80; scalar = c & 31; }
    else if (c >= 0xe0 && c <= 0xef) { remaining = 2; minimum = 0x800; scalar = c & 15; }
    else if (c >= 0xf0 && c <= 0xf4) { remaining = 3; minimum = 0x10000; scalar = c & 7; }
    else return -1;
    if (j->length - j->position < remaining) return -1;
    for (unsigned i = 0; i < remaining; ++i) {
        unsigned next = j->text[j->position++];
        if ((next & 0xc0) != 0x80) return -1;
        scalar = (scalar << 6) | (next & 63);
    }
    if (scalar < minimum || scalar > 0x10ffff ||
        (scalar >= 0xd800 && scalar <= 0xdfff)) return -1;
    *value = scalar;
    return 1;
}

static int touch2_json_string(Touch2Json *j) {
    if (j->position >= j->length || j->text[j->position++] != '"') return 0;
    unsigned value;
    int status;
    do { status = touch2_json_scalar(j, &value); } while (status > 0);
    return status == 0;
}

static int touch2_json_keys_equal(const Touch2Json *j, unsigned a, unsigned b) {
    Touch2Json left = *j, right = *j;
    left.position = a; right.position = b;
    for (;;) {
        unsigned x = 0, y = 0;
        int sx = touch2_json_scalar(&left, &x), sy = touch2_json_scalar(&right, &y);
        if (sx < 0 || sy < 0 || sx != sy) return 0;
        if (sx == 0) return 1;
        if (x != y) return 0;
    }
}

static int touch2_json_key_is(const Touch2Json *j, unsigned start, const char *ascii) {
    Touch2Json key = *j; key.position = start;
    for (unsigned i = 0;; ++i) {
        unsigned value = 0;
        int status = touch2_json_scalar(&key, &value);
        if (!ascii[i]) return status == 0;
        if (status != 1 || value != (unsigned char)ascii[i]) return 0;
    }
}

static int touch2_json_literal(Touch2Json *j, const char *literal) {
    for (unsigned i = 0; literal[i]; ++i)
        if (j->position >= j->length || j->text[j->position++] != (unsigned char)literal[i]) return 0;
    return 1;
}

static int touch2_json_digit(unsigned c) { return c >= '0' && c <= '9'; }

static int touch2_json_number(Touch2Json *j) {
    if (j->position < j->length && j->text[j->position] == '-') ++j->position;
    if (j->position >= j->length) return 0;
    unsigned c = j->text[j->position++];
    if (c != '0') {
        if (c < '1' || c > '9') return 0;
        while (j->position < j->length && touch2_json_digit(j->text[j->position])) ++j->position;
    }
    if (j->position < j->length && j->text[j->position] == '.') {
        ++j->position;
        unsigned start = j->position;
        while (j->position < j->length && touch2_json_digit(j->text[j->position])) ++j->position;
        if (j->position == start) return 0;
    }
    if (j->position < j->length && (j->text[j->position] == 'e' || j->text[j->position] == 'E')) {
        ++j->position;
        if (j->position < j->length && (j->text[j->position] == '-' || j->text[j->position] == '+')) ++j->position;
        unsigned start = j->position;
        while (j->position < j->length && touch2_json_digit(j->text[j->position])) ++j->position;
        if (j->position == start) return 0;
    }
    return 1;
}

static int touch2_json_value(Touch2Json *, unsigned, unsigned);

static int touch2_json_object(Touch2Json *j, unsigned depth, unsigned duplicates,
    Touch2ConfigValue *config) {
    if (depth >= 16 || j->position >= j->length || j->text[j->position++] != '{') return 0;
    ++depth;
    unsigned first = j->position;
    touch2_json_ws(j);
    if (j->position < j->length && j->text[j->position] == '}') { ++j->position; return 1; }
    for (;;) {
        unsigned quote = j->position, key = quote + 1;
        if (!touch2_json_string(j)) return 0;
        if (duplicates) {
            Touch2Json prior = *j; prior.position = first; touch2_json_ws(&prior);
            while (prior.position < quote) {
                unsigned prior_key = prior.position + 1;
                if (!touch2_json_string(&prior)) return 0;
                if (touch2_json_keys_equal(j, prior_key, key)) return 0;
                touch2_json_ws(&prior);
                if (prior.position >= prior.length || prior.text[prior.position++] != ':') return 0;
                touch2_json_ws(&prior);
                if (!touch2_json_value(&prior, depth, 0)) return 0;
                touch2_json_ws(&prior);
                if (prior.position >= prior.length || prior.text[prior.position++] != ',') return 0;
                touch2_json_ws(&prior);
            }
            if (prior.position != quote) return 0;
        }
        touch2_json_ws(j);
        if (j->position >= j->length || j->text[j->position++] != ':') return 0;
        touch2_json_ws(j);
        if (config && touch2_json_key_is(j, key, "schemaVersion")) {
            unsigned start = j->position;
            if (!touch2_json_number(j) || j->position != start + 1 || j->text[start] != '1') return 0;
            config->seen |= 1;
        } else if (config && touch2_json_key_is(j, key, "enabled")) {
            if (j->position >= j->length) return 0;
            if (j->text[j->position] == 't') {
                if (!touch2_json_literal(j, "true")) return 0;
                config->enabled = 1;
            } else {
                if (!touch2_json_literal(j, "false")) return 0;
                config->enabled = 0;
            }
            config->seen |= 2;
        } else if (!touch2_json_value(j, depth, duplicates)) return 0;
        touch2_json_ws(j);
        if (j->position >= j->length) return 0;
        unsigned delimiter = j->text[j->position++];
        if (delimiter == '}') return 1;
        if (delimiter != ',') return 0;
        touch2_json_ws(j);
    }
}

static int touch2_json_value(Touch2Json *j, unsigned depth, unsigned duplicates) {
    touch2_json_ws(j);
    if (j->position >= j->length) return 0;
    unsigned c = j->text[j->position];
    if (c == '{') return touch2_json_object(j, depth, duplicates, 0);
    if (c == '[') {
        if (depth >= 16) return 0;
        ++depth; ++j->position;
        touch2_json_ws(j);
        if (j->position < j->length && j->text[j->position] == ']') { ++j->position; return 1; }
        for (;;) {
            if (!touch2_json_value(j, depth, duplicates)) return 0;
            touch2_json_ws(j);
            if (j->position >= j->length) return 0;
            unsigned delimiter = j->text[j->position++];
            if (delimiter == ']') return 1;
            if (delimiter != ',') return 0;
            touch2_json_ws(j);
        }
    }
    if (c == '"') return touch2_json_string(j);
    if (c == 't') return touch2_json_literal(j, "true");
    if (c == 'f') return touch2_json_literal(j, "false");
    if (c == 'n') return touch2_json_literal(j, "null");
    return touch2_json_number(j);
}

static int touch2_config_parse(const char *text, unsigned length, unsigned *enabled) {
    if (!text || !enabled || length == 0 || length > 4096) return 0;
    Touch2Json j = {(const unsigned char *)text, length, 0};
    if (length >= 3 && j.text[0] == 0xef && j.text[1] == 0xbb && j.text[2] == 0xbf) j.position = 3;
    touch2_json_ws(&j);
    Touch2ConfigValue config = {0, 0};
    if (!touch2_json_object(&j, 0, 1, &config) || config.seen != 3) return 0;
    touch2_json_ws(&j);
    if (j.position != length) return 0;
    *enabled = config.enabled;
    return 1;
}

#endif
