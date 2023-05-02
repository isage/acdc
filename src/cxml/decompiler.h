#ifndef __DECOMPILER_H__
#define __DECOMPILER_H__

#include "../utils/tinyxml2.h"
#include "../utils/utils.h"
#include "cxml.h"

#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>

namespace cxml {

typedef struct {
    std::set<std::string> parents;
    std::map<std::string, cxml::Attr> attributes;
} defElement;

class Decompiler {
  public:
    Decompiler(std::string cxml, std::string rcd);
    bool decompile(std::string out);
    void generate_cxmldef(std::string filename);

  private:
    FILE* fp;
    void parse_tree(int32_t tree_offset, tinyxml2::XMLElement* parent, tinyxml2::XMLDocument& doc);
    std::string get_from_stringtable(int32_t handle);
    std::string get_from_wstringtable(int32_t handle, int32_t size);
    std::string get_from_hashtable(int32_t handle);
    std::string get_from_idhashtable(int32_t handle);
    std::string get_from_intarraytable(int32_t handle, int32_t size);
    std::string get_from_floatarraytable(int32_t handle, int32_t size);
    std::string get_from_idtable(int32_t handle);
    void extract_from_filetable(std::string filename, int32_t handle, int32_t size, bool compressed = false, uint32_t file_orig_size = 0);

    void parse_rcd();


    std::map<
        std::string,
        std::map<std::string,std::string>
    > _rcd_table;

    std::string _cxml;
    std::string _rcd;
    std::string _outdir;
    cxml::Header header;
    uint8_t* tree;
    uint8_t* idtable;
    uint8_t* idhashtable;
    uint8_t* stringtable;
    uint8_t* wstringtable;
    uint8_t* hashtable;
    uint8_t* intarraytable;
    uint8_t* floatarraytable;
    uint8_t* filetable;

    std::map<std::string, defElement*> _definition;
};

}

#endif