#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "../utils/tinyxml2.h"
#include "../utils/utils.h"
#include "cxml.h"

#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <map>

namespace cxml {

class Compiler {
  public:
    Compiler(std::string schema, std::string xml);
    bool compile(std::string cxml);
    void generateRcd(std::string cxml, std::string rcd);
    void generateCHeader(std::string cxml, std::string header);

  private:
    std::string _schema_file;
    std::string _xml_file;
    uint32_t _version;
    std::string _magic;

    std::map<std::string, cxml::SchemaElement> schema;

    std::map<std::string, uint32_t> string_table;
    std::vector<uint8_t> string_table_bin = {};

    std::map<std::string, std::pair<uint32_t, uint32_t>> wstring_table;
    std::vector<uint8_t> wstring_table_bin = {};

    uint32_t intarray_table_size = 0;
    std::vector<uint8_t> intarray_table_bin = {};

    uint32_t floatarray_table_size = 0;
    std::vector<uint8_t> floatarray_table_bin = {};

    std::map<std::string, uint32_t> id_table;
    std::vector<uint8_t> id_table_bin = {};

    std::map<uint32_t, std::pair<uint32_t, std::string>> idhash_table;
    std::vector<uint8_t> idhash_table_bin = {};

    std::map<uint32_t, uint32_t> hash_table;
    std::vector<uint8_t> hash_table_bin = {};
    std::map<std::string, bool> hash_table_origin;

    std::map<std::string, std::tuple<uint32_t, uint32_t, uint32_t>> file_table;
    std::vector<uint8_t> file_table_bin = {};

    std::vector<cxml::Tag*> tree_table;
    uint32_t tree_table_size  = 0;
    std::vector<uint8_t> tree_table_bin = {};

    std::vector<std::string> rcd_table = {};

    uint32_t push_to_string_table(const char* value);
    uint32_t push_to_string_table(std::string value); // todo: remove?
    std::pair<uint32_t,uint32_t> push_to_wstring_table(const char* value);
    uint32_t push_to_id_table(const char* value, uint32_t entity_offset);
    uint32_t push_to_idref_table(const char* value, uint32_t entity_offset);
    uint32_t push_to_idhash_table(const char* value, uint32_t entity_offset, uint32_t* hash);
    uint32_t push_to_idhashref_table(const char* value, uint32_t entity_offset);
    uint32_t push_to_hash_table(const char* value, uint32_t* hash);
    std::pair<uint32_t, uint32_t> push_to_intarray_table(const char* value);
    std::pair<uint32_t, uint32_t> push_to_floatarray_table(const char* value);
    std::tuple<uint32_t, uint32_t, uint32_t> push_to_file_table(const char* value, bool docompress);

    bool build_and_validate_schema();
    cxml::Tag* iterate_tree(tinyxml2::XMLElement* el, cxml::Tag* prevtag = NULL, cxml::Tag* parenttag = NULL);
};

}

#endif