#ifndef __CXML_H__
#define __CXML_H__

#include <stdint.h>
#include <vector>
#include <string>

namespace cxml {

enum class Attr
{
    None = 0,          ///< No attribute
    Int = 1,           ///< 32-bit integer
    Float = 2,         ///< 32-bit float
    String = 3,        ///< utf-8 string
    WString = 4,       ///< utf-16 string
    Hash = 5,          ///< Hash
    IntArray = 6,      ///< Array of integers
    FloatArray = 7,    ///< Array of floats
    File = 8,          ///< File
    ID = 9,            ///< ID
    IDRef = 10,        ///< Reference to ID
    IDHash = 11,       ///< ID Hash
    IDHashRef = 12,    ///< Reference to ID hash
};

typedef struct Header
{
    char magic[4]; //CXML
    int32_t version;  //0x110
    int32_t tree_offset;
    int32_t tree_size;
    int32_t idtable_offset;
    int32_t idtable_size;
    int32_t idhashtable_offset;
    int32_t idhashtable_size;
    int32_t stringtable_offset;
    int32_t stringtable_size;
    int32_t wstringtable_offset;
    int32_t wstringtable_size;
    int32_t hashtable_offset;
    int32_t hashtable_size;
    int32_t intarraytable_offset;
    int32_t intarraytable_size;
    int32_t floatarraytable_offset;
    int32_t floatarraytable_size;
    int32_t filetable_offset;
    int32_t filetable_size;
} Header;

typedef struct TreeHeader {
    int32_t name_handle;
    int32_t num_attributes;
    int32_t parent_elm_offset;
    int32_t prev_elm_offset;
    int32_t next_elm_offset;
    int32_t first_child_elm_offset;
    int32_t last_child_elm_offset;
} TreeHeader;

typedef struct TagAttr {
    uint32_t name_handle;
    uint32_t type;
    uint32_t offset; // of value
    uint32_t size;
} TagAttr;

typedef struct Tag {
        std::vector<TagAttr> kv;
        uint32_t offset;
        TreeHeader th;
} Tag;


typedef struct {
    std::string name;
    cxml::Attr type;
} SchemaAttr;

typedef struct {
    std::string name;
    std::vector<std::string> parents;
    std::vector<SchemaAttr> attributes;
} SchemaElement;


}

#endif
