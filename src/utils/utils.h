#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <iostream>
#include <sstream>
#include <cctype>
#include <locale>
#include <algorithm>
#include <vector>
#include <set>
#include <cstdint>
#include <iterator>

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

static inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        trim(item);
        result.push_back (item);
    }

    return result;
}

static inline std::string join(const std::vector<std::string> &vec, const char* delim) {
    switch (vec.size())
    {
        case 0:
            return "";
        case 1:
            return vec[0];
        default:
            std::ostringstream os;
            std::copy(vec.begin(), vec.end() - 1, std::ostream_iterator<std::string>(os, delim));
            os << *vec.rbegin();
            return os.str();
    }
}

static inline std::string join(const std::set<std::string> &vec, const char* delim) {
    switch (vec.size())
    {
        case 0:
            return "";
        case 1:
            return *vec.begin();
        default:
            std::ostringstream os;
            std::copy(vec.begin(), std::prev(vec.end()), std::ostream_iterator<std::string>(os, delim));
            os << *vec.rbegin();
            return os.str();
    }
}





static inline void binarray_align(std::vector<uint8_t>& v, uint32_t align)
{
    while(v.size() % 16 != 0)
    {
        v.push_back(0);
    }
}

static inline uint16_t utf8_to_ucs2(const char *utf8, uint16_t *character)
{
    if (((utf8[0] & 0xF0) == 0xE0) && ((utf8[1] & 0xC0) == 0x80) && ((utf8[2] & 0xC0) == 0x80)) {
        *character = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
        return 3;
    } else if (((utf8[0] & 0xE0) == 0xC0) && ((utf8[1] & 0xC0) == 0x80)) {
        *character = ((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F);
        return 2;
    } else {
        *character = utf8[0];
        return 1;
    }
}

static inline void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
  int i;
  for (i = 0; src[i]; i++) {
    if ((src[i] & 0xFF80) == 0) {
      *(dst++) = src[i] & 0xFF;
    } else if((src[i] & 0xF800) == 0) {
      *(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
      *(dst++) = (src[i] & 0x3F) | 0x80;
    } else if((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
      *(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
      *(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
      *(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
      *(dst++) = (src[i + 1] & 0x3F) | 0x80;
      i += 1;
    } else {
      *(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
      *(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
      *(dst++) = (src[i] & 0x3F) | 0x80;
    }
  }

  *dst = '\0';
}

static inline std::string float_to_string(float val)
{
    size_t s = snprintf(NULL, 0, "%g", val);
    char* data = (char*)malloc(s+1);
    snprintf(data, s+1, "%g", val);
    std::string str(data);
    free(data);
    return str;
}

#endif