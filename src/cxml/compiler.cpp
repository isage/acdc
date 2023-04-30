#include "compiler.h"
#include "../utils/sha1.h"
#include <zlib.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace cxml {

Compiler::Compiler(std::string schema, std::string xml) : _schema_file(schema), _xml_file(xml)
{
}

// table pushers

uint32_t Compiler::push_to_string_table(const char* value)
{
    if(string_table.count(value) == 0)
    {
        uint32_t offset = string_table_bin.size();
        string_table.emplace(value, offset);

        for(size_t i=0;i<strlen(value); i++)
            string_table_bin.push_back(value[i]);
        string_table_bin.push_back(0);

        return offset;
    }
    else
    {
        return string_table.at(value);
    }
}

uint32_t Compiler::push_to_string_table(std::string value)
{
    return push_to_string_table(value.c_str());
}

std::pair<uint32_t,uint32_t> Compiler::push_to_wstring_table(const char* value)
{
    if(wstring_table.count(value) == 0)
    {
        uint32_t offset = wstring_table_bin.size() / 2;

        uint32_t len = 0;
        for (int i = 0; value[i];) {
            uint8_t character[2];
            i += utf8_to_ucs2(&value[i], (uint16_t*)character);
            wstring_table_bin.push_back(character[0]);
            wstring_table_bin.push_back(character[1]);
            len+=2;
        }

        wstring_table_bin.push_back(0);
        wstring_table_bin.push_back(0);
        wstring_table.emplace(value, std::make_pair(offset, len));
        return {offset, len};
    }
    else
    {
        return wstring_table.at(value);
    }
}

std::pair<uint32_t,uint32_t> Compiler::push_to_intarray_table(const char* value)
{
    std::vector<uint8_t> temp_binary;
    std::vector<std::string> values = split(value,',');
    for (auto& v : values)
    {
        uint32_t intval = std::stoi(v);
        uint8_t* intval8 = reinterpret_cast<uint8_t*>(&intval);
        temp_binary.push_back(intval8[0]);
        temp_binary.push_back(intval8[1]);
        temp_binary.push_back(intval8[2]);
        temp_binary.push_back(intval8[3]);
    }

    auto start = intarray_table_bin.begin();
    int32_t offset = -1;
    while ( (start = std::search(start, intarray_table_bin.end(), temp_binary.begin(), temp_binary.end())) != intarray_table_bin.end()) {
        offset = std::distance(intarray_table_bin.begin(), start);
        if (offset % 4 == 0) break;
        start++;
    }
    if (offset > 0)
    {
        return {offset / 4, values.size()};
    }

    offset = intarray_table_size / 4;
    for (auto& v : temp_binary)
    {
        intarray_table_bin.push_back(v);
    }
    return {offset, values.size()};
}

std::pair<uint32_t,uint32_t> Compiler::push_to_floatarray_table(const char* value)
{
    std::vector<uint8_t> temp_binary;
    std::vector<std::string> values = split(value,',');
    for (auto& v : values)
    {
        float intval = std::stof(v);
        uint8_t* intval8 = reinterpret_cast<uint8_t*>(&intval);
        temp_binary.push_back(intval8[0]);
        temp_binary.push_back(intval8[1]);
        temp_binary.push_back(intval8[2]);
        temp_binary.push_back(intval8[3]);
    }

    auto start = floatarray_table_bin.begin();
    int32_t offset = -1;
    while ( (start = std::search(start, floatarray_table_bin.end(), temp_binary.begin(), temp_binary.end())) != floatarray_table_bin.end()) {
        offset = std::distance(floatarray_table_bin.begin(), start);
        if (offset % 4 == 0) break;
        start++;
    }
    if (offset > 0)
    {
        return {offset / 4, values.size()};
    }

    offset = floatarray_table_size / 4;
    for (auto& v : temp_binary)
    {
        floatarray_table_bin.push_back(v);
    }
    return {offset, values.size()};
}

uint32_t Compiler::push_to_id_table(const char* value, uint32_t entity_offset )
{
    if(id_table.count(value) == 0)
    {
        uint32_t offset = id_table_bin.size();
        id_table.emplace(value, offset);

        uint8_t* entity_offset8 = reinterpret_cast<uint8_t*>(&entity_offset);
        id_table_bin.push_back(entity_offset8[0]);
        id_table_bin.push_back(entity_offset8[1]);
        id_table_bin.push_back(entity_offset8[2]);
        id_table_bin.push_back(entity_offset8[3]);

        for(size_t i=0;i<strlen(value); i++)
            id_table_bin.push_back(value[i]);
        id_table_bin.push_back(0);

        // align to 4
        while (id_table_bin.size() % 4 != 0)
        {
            id_table_bin.push_back(0);
        }
        return offset;
    }
    else
    {
        // if table at offset == 0xFFFFFFFF - update that to offset, else error
        uint32_t offset = id_table.at(value);
        if (*(uint32_t*)((uint8_t*)id_table_bin.data() + offset) == 0xFFFFFFFF)
        {
            *(uint32_t*)((uint8_t*)id_table_bin.data() + offset) = entity_offset;
        }
        else
        {
            // error out
            exit(-1);
//            return -1;
        }
        return offset;
    }
}

uint32_t Compiler::push_to_idref_table(const char* value, uint32_t entity_offset )
{
    if(id_table.count(value) == 0)
    {
        uint32_t offset = id_table_bin.size();
        id_table.emplace(value, offset);

        id_table_bin.push_back(0xFF);
        id_table_bin.push_back(0xFF);
        id_table_bin.push_back(0xFF);
        id_table_bin.push_back(0xFF);

        for(size_t i=0;i<strlen(value); i++)
            id_table_bin.push_back(value[i]);
        id_table_bin.push_back(0);

        // align to 4?
        while (id_table_bin.size() % 4 != 0)
        {
            id_table_bin.push_back(0);
        }
        return offset;
    }
    else
    {
        return id_table.at(value);
    }
}


uint32_t Compiler::push_to_idhash_table(const char* value, uint32_t entity_offset, uint32_t* hash)
{
    // if it's hex value, treat as nid
    uint32_t nid;
    try {
        nid = std::stol(value, nullptr);
    }
    catch(std::invalid_argument& e)
    {
      SHA1_CTX ctx;
      SHA1Init(&ctx);

      for (size_t i = 0; i < strlen(value); i++)
        SHA1Update(&ctx, (const uint8_t*)value + i, 1);

      uint8_t sha1[20];
      SHA1Final(sha1, &ctx);

      nid = (sha1[0] << 24) | (sha1[1] << 16) | (sha1[2] << 8) | sha1[3];
    }

    *hash = nid;

    if(idhash_table.count(nid) == 0)
    {
        uint32_t offset = idhash_table_bin.size();

        idhash_table.emplace(nid, std::make_pair(offset, value));

        uint8_t* entity_offset8 = reinterpret_cast<uint8_t*>(&entity_offset);
        idhash_table_bin.push_back(entity_offset8[0]);
        idhash_table_bin.push_back(entity_offset8[1]);
        idhash_table_bin.push_back(entity_offset8[2]);
        idhash_table_bin.push_back(entity_offset8[3]);

        uint8_t* nid8 = reinterpret_cast<uint8_t*>(&nid);
        idhash_table_bin.push_back(nid8[0]);
        idhash_table_bin.push_back(nid8[1]);
        idhash_table_bin.push_back(nid8[2]);
        idhash_table_bin.push_back(nid8[3]);

        return offset;
    }
    else
    {
        std::pair<uint32_t, std::string> data = idhash_table.at(nid);
        uint32_t offset = data.first;
        if (*(uint32_t*)((uint8_t*)idhash_table_bin.data() + offset) == 0xFFFFFFFF)
        {
            *(uint32_t*)((uint8_t*)idhash_table_bin.data() + offset) = entity_offset;
            idhash_table.at(nid).second = value;
        }
        else
        {
            // error
            exit(-1);
        }
        return offset;
    }
}

uint32_t Compiler::push_to_idhashref_table(const char* value, uint32_t entity_offset)
{
    // if it's hex value, treat as nid
    uint32_t nid;
    try {
        nid = std::stol(value, nullptr);
    }
    catch(std::invalid_argument& e)
    {
      SHA1_CTX ctx;
      SHA1Init(&ctx);

      for (size_t i = 0; i < strlen(value); i++)
        SHA1Update(&ctx, (const uint8_t*)value + i, 1);

      uint8_t sha1[20];
      SHA1Final(sha1, &ctx);

      nid = (sha1[0] << 24) | (sha1[1] << 16) | (sha1[2] << 8) | sha1[3];
    }

    if(idhash_table.count(nid) == 0)
    {
        uint32_t offset = idhash_table_bin.size();

        idhash_table.emplace(nid, std::make_pair(offset, value));

        idhash_table_bin.push_back(0xFF);
        idhash_table_bin.push_back(0xFF);
        idhash_table_bin.push_back(0xFF);
        idhash_table_bin.push_back(0xFF);

        uint8_t* nid8 = reinterpret_cast<uint8_t*>(&nid);
        idhash_table_bin.push_back(nid8[0]);
        idhash_table_bin.push_back(nid8[1]);
        idhash_table_bin.push_back(nid8[2]);
        idhash_table_bin.push_back(nid8[3]);

        return offset;
    }
    else
    {
        std::pair<uint32_t, std::string> data = idhash_table.at(nid);
        return data.first;
    }
}

uint32_t Compiler::push_to_hash_table(const char* value, uint32_t* hash)
{
    uint32_t nid;
    try {
        nid = std::stol(value, nullptr);
    }
    catch(std::invalid_argument& e)
    {

      SHA1_CTX ctx;
      SHA1Init(&ctx);

      for (size_t i = 0; i < strlen(value); i++)
        SHA1Update(&ctx, (const uint8_t*)value + i, 1);

      uint8_t sha1[20];
      SHA1Final(sha1, &ctx);

      nid = (sha1[0] << 24) | (sha1[1] << 16) | (sha1[2] << 8) | sha1[3];
    }

    *hash = nid;

    if(hash_table.count(nid) == 0)
    {
        uint32_t offset = hash_table_bin.size() / 4;

        hash_table.emplace(nid, offset);

        uint8_t* nid8 = reinterpret_cast<uint8_t*>(&nid);
        hash_table_bin.push_back(nid8[0]);
        hash_table_bin.push_back(nid8[1]);
        hash_table_bin.push_back(nid8[2]);
        hash_table_bin.push_back(nid8[3]);

        return offset;
    }
    else
    {
        // warning
        if (!hash_table_origin.count(value))
            printf("Warning: %s has duplicate hash 0x%08x\n", value, nid);
        else
            hash_table_origin.emplace(value, true);
        return hash_table.at(nid);
    }
}

std::tuple<uint32_t,uint32_t,uint32_t> Compiler::push_to_file_table(const char* value, bool docompress)
{
    fs::path p = fs::absolute(_xml_file);
    p = p.remove_filename();
    p /= fs::path(value);
    p = fs::absolute(p);

    if (p.empty())
    {
        printf("File `%s` doesn't exist\n", value);
        exit(-1);
    }

    if(file_table.count(p) == 0)
    {
        uint32_t filesize = 0;
        uint32_t compressed_size = 0;
        uint32_t offset = file_table_bin.size();

        FILE* fp = fopen(p.c_str(), "rb");
        if (!fp)
        {
            printf("Can't open %s\n", p.c_str());
            exit(-1);
        }
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        compressed_size = filesize;

        fseek(fp, 0, SEEK_SET);

        uint8_t* buf = (uint8_t*)malloc(filesize);
        fread(buf, filesize, 1, fp);
        fclose(fp);

        if (docompress)
        {
            compressed_size = compressBound(filesize);
            uint8_t* zbuf = (uint8_t*)malloc(compressed_size);
            uint32_t fz = filesize;
            int res = compress(zbuf, (uLongf*)&compressed_size, buf, filesize);
            filesize = fz;

            if (res != Z_OK)
            {
                printf("Warning: can't compress %s. Adding as-is\n", p.c_str());
                for(size_t i = 0; i < filesize;i++)
                {
                    file_table_bin.push_back(buf[i]);
                }
                binarray_align(file_table_bin, 16);
            }
            else
            {
                for(size_t i = 0; i < compressed_size;i++)
                {
                    file_table_bin.push_back(zbuf[i]);
                }
                binarray_align(file_table_bin, 16);
            }
            free(zbuf);
        } else {
            for(size_t i = 0; i < filesize;i++)
            {
                file_table_bin.push_back(buf[i]);
            }
            binarray_align(file_table_bin, 16);
        }

        free(buf);
        file_table.emplace(p, std::make_tuple(offset, compressed_size, filesize));
        return {offset, compressed_size, filesize};
    }
    else
    {
        std::tuple<uint32_t,uint32_t,uint32_t> ret = file_table.at(p);
        return ret;
    }
}

cxml::Tag* Compiler::iterate_tree(tinyxml2::XMLElement* el, cxml::Tag* prevtag, cxml::Tag* parenttag)
{
    if (schema.count(el->Value()) == 0)
    {
        printf("Malformed cxml file, unexpected element `%s`\n", el->Value());
        return NULL;
    }

    cxml::SchemaElement elem_schema = schema.at(el->Value());
    tinyxml2::XMLNode* parent = el->Parent();

    // check tree validity
    if(!elem_schema.parents.size() && parent && parent->Value())
    {
        printf("Malformed cxml file, element `%s` shouldn't have parent `%s`\n", el->Value(), parent->Value());
        return NULL;
    }
    else if(elem_schema.parents.size())
    {
        bool found = false;
        for (auto& v: elem_schema.parents)
        {
            if (v == parent->Value())
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            printf("Malformed cxml file, element `%s` shouldn't have parent `%s`\n", el->Value(), parent->Value());
            return NULL;
        }
    }

    // create element
    cxml::Tag* tag = new cxml::Tag();

    tag->th.num_attributes = elem_schema.attributes.size();
    tag->th.name_handle = push_to_string_table(el->Value());

    if(parenttag)
        tag->th.parent_elm_offset = parenttag->offset;
    else
        tag->th.parent_elm_offset = 0xFFFFFFFF;

    tag->offset = tree_table_size;
    tag->th.next_elm_offset = 0xFFFFFFFF;
    tag->th.first_child_elm_offset = 0xFFFFFFFF;
    tag->th.last_child_elm_offset = 0xFFFFFFFF;

    if (prevtag)
    {
        tag->th.prev_elm_offset = prevtag->offset;
        prevtag->th.next_elm_offset = tag->offset;
    }
    else
        tag->th.prev_elm_offset = 0xFFFFFFFF;

    // increment tree table size on tree_header size;
    tree_table_size += sizeof(cxml::TreeHeader);

    bool is_file = false;
    uint32_t hash = 0;
    std::string key;
    std::string id;

    // check attr validity
    for (auto& v: elem_schema.attributes)
    {
        const tinyxml2::XMLAttribute *a = el->FindAttribute(v.name.c_str());
        if (!a)
        {
            tag->th.num_attributes--;
            continue;
        }
        else
        {
            // create tag kv, increment tree size on kv size
            cxml::TagAttr attr;
            attr.name_handle = push_to_string_table(v.name);
            attr.type = (uint32_t)v.type;

            // parse attr
            switch(v.type)
            {
                default: break;
                case cxml::Attr::None: break; // TODO: error
                case cxml::Attr::Int:
                {
                    attr.offset = el->IntAttribute(v.name.c_str());
                    attr.size = 0;
                    break;
                }
                case cxml::Attr::Float:
                {
                    attr.offset = (uint32_t)el->FloatAttribute(v.name.c_str());
                    attr.size = 0;
                    break;
                }
                case cxml::Attr::String:
                {
                    attr.offset = push_to_string_table(el->Attribute(v.name.c_str()));
                    attr.size = strlen(el->Attribute(v.name.c_str()));
                    break;
                }
                case cxml::Attr::WString:
                {
                    auto t = push_to_wstring_table(el->Attribute(v.name.c_str()));
                    attr.offset = t.first;
                    attr.size = t.second;
                    break;
                }
                case cxml::Attr::Hash:
                {
                    attr.offset = push_to_hash_table(el->Attribute(v.name.c_str()), &hash);
                    attr.size = 4;
                    break;
                }
                case cxml::Attr::IntArray:
                {
                    auto t = push_to_intarray_table(el->Attribute(v.name.c_str()));
                    attr.offset = t.first;
                    attr.size = t.second;
                    break;
                }
                case cxml::Attr::FloatArray:
                {
                    auto t = push_to_floatarray_table(el->Attribute(v.name.c_str()));
                    attr.offset = t.first;
                    attr.size = t.second;
                    break;
                }
                case cxml::Attr::File:
                {
                    is_file = true;
                    // if element has attr compress = on: add origsize field and compress
                    const tinyxml2::XMLAttribute *compress = el->FindAttribute("compress");
                    if(compress && strcmp(compress->Value(), "on") == 0)
                    {
                        // offset, size, origsize
                        std::tuple<uint32_t, uint32_t, uint32_t> oscs = push_to_file_table(el->Attribute(v.name.c_str()), true);
                        const tinyxml2::XMLAttribute *origsize_el = el->FindAttribute("origsize");
                        if(!origsize_el) // if we already have origsize attr - don't overwrite it
                        {
                            cxml::TagAttr orig_size_attr;
                            orig_size_attr.name_handle = push_to_string_table("origsize");
                            orig_size_attr.type = (uint32_t)cxml::Attr::Int;
                            orig_size_attr.offset = std::get<2>(oscs);
                            orig_size_attr.size = 0;
                            tree_table_size += sizeof(cxml::TagAttr);
                            tag->kv.push_back(orig_size_attr);
                            tag->th.num_attributes++;
                        }
                        attr.offset = std::get<0>(oscs);
                        attr.size = std::get<1>(oscs);
                    }
                    else
                    {
                        // offset, size, origsize
                        std::tuple<uint32_t, uint32_t, uint32_t> oscs = push_to_file_table(el->Attribute(v.name.c_str()), false);
                        attr.offset = std::get<0>(oscs);
                        attr.size = std::get<1>(oscs);
                    }
                    break;
                }
                case cxml::Attr::ID:
                {
                    attr.offset = push_to_id_table(el->Attribute(v.name.c_str()), tag->offset);
                    attr.size = 0;
                    id = el->Attribute(v.name.c_str());
                    key = el->Attribute(v.name.c_str());
                    break;
                }
                case cxml::Attr::IDRef:
                {
                    attr.offset = push_to_idref_table(el->Attribute(v.name.c_str()), tag->offset);
                    attr.size = 0;
                    break;
                }
                case cxml::Attr::IDHash:
                {
                    attr.offset = push_to_idhash_table(el->Attribute(v.name.c_str()), tag->offset, &hash);
                    attr.size = 0;
                    key = el->Attribute(v.name.c_str());
                    break;
                }
                case cxml::Attr::IDHashRef:
                {
                    attr.offset = push_to_idhashref_table(el->Attribute(v.name.c_str()), tag->offset);
                    attr.size = 0;
                    key = el->Attribute(v.name.c_str());
                    break;
                }
            }

            tree_table_size += sizeof(cxml::TagAttr);
            tag->kv.push_back(attr);
        }
    }

    if (hash != 0)
    {
        char h[16];
        sprintf(h, "%08x(0)", hash);
        if (is_file && el->Attribute("src") && el->Attribute("type"))
        {
            rcd_table.push_back(
                std::string("key:") + std::string(h)
                + std::string(" id:") + key
                + std::string(",src:") + std::string(el->Attribute("src"))
                + std::string(",type:") + std::string(el->Attribute("type"))
            );
        }
        else
        {
            rcd_table.push_back(std::string("key:") + std::string(h) + std::string(" id:")+key);
        }
    }
    else if (!id.empty())
    {
        if (is_file && el->Attribute("src") && el->Attribute("type"))
        {
            rcd_table.push_back(
                std::string("key:") + id
                + std::string(" id:") + key
                + std::string(",src:") + std::string(el->Attribute("src"))
                + std::string(",type:") + std::string(el->Attribute("type"))
            );
        }
        else
        {
            rcd_table.push_back(std::string("key:") + id + std::string(" id:")+key);
        }
    }

    // push tag to tree table
    tree_table.push_back(tag);

    cxml::Tag* prev = NULL;
    for (tinyxml2::XMLElement* child = el->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        if(!prev)
        {
            prev = iterate_tree(child, prev, tag);
            // save it's offset in first_child
            tag->th.first_child_elm_offset = prev->offset;
        }
        else
            prev = iterate_tree(child, prev, tag);

        if (!prev) return NULL; // break early
        else tag->th.last_child_elm_offset = prev->offset;
    }
    return tag;
}

bool Compiler::build_and_validate_schema()
{
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLError err;

  std::map<std::string, cxml::Attr> attr_types = {
    {"none", cxml::Attr::None},
    {"int", cxml::Attr::Int},
    {"float", cxml::Attr::Float},
    {"string", cxml::Attr::String},
    {"wstring", cxml::Attr::WString},
    {"hash", cxml::Attr::Hash},
    {"intarray", cxml::Attr::IntArray},
    {"floatarray", cxml::Attr::FloatArray},
    {"file", cxml::Attr::File},
    {"id", cxml::Attr::ID},
    {"idref", cxml::Attr::IDRef},
    {"idhash", cxml::Attr::IDHash},
    {"idhashref", cxml::Attr::IDHashRef}
  };

  err = doc.LoadFile(_schema_file.c_str());

  if (err == tinyxml2::XML_SUCCESS)
  {
    tinyxml2::XMLElement* root = doc.FirstChildElement();
    if (strcmp(root->Value(), "cxml") != 0)
    {
      printf("Malformed cxml definition, expected cxml as root element\n");
      return false;
    }
    _version = root->IntAttribute("version", 0);
    _magic = root->Attribute("magic");

    // iterate over elements, parse into list
    tinyxml2::XMLElement* child = root->FirstChildElement();
    while(child)
    {
        if (strcmp(child->Value(), "element") != 0)
        {
            printf("Malformed cxml definition, expected `element`, got `%s`\n", child->Value());
            return false;
        }

        const char* name = child->Attribute("name");
        if (!name)
        {
            printf("Malformed cxml definition, expected `element`, got `%s`\n", child->Value());
            return false;
        }

        cxml::SchemaElement el;
        el.name = name;

        const char* parents = child->Attribute("parents");
        if (parents)
        {
            //split parents into vector
            el.parents = split(parents, ',');
        }

        const char* attributes = child->Attribute("attributes");
        if (attributes)
        {
            //split attributes into vector
            std::vector<std::string> attrs;
            attrs = split(attributes, ',');
            for (auto& a : attrs)
            {
                std::vector<std::string> v = split(a, ':');
                cxml::SchemaAttr attr;
                attr.name = v[0];
                attr.type = cxml::Attr::None;
                if (attr_types.count(v[1]))
                {
                    attr.type = attr_types[v[1]];
                }
                else
                {
                    printf("Malformed cxml definition, unknown attr type `%s`\n", v[1].c_str());
                    return false;
                }
                el.attributes.emplace_back(attr);
            }
        }

        schema.emplace(name, el);

        child = child->NextSiblingElement();
    }
  } else {
      printf("Malformed cxml definition: %s\n", doc.ErrorStr());
      return false;
  }
  return true;
}

bool Compiler::compile(std::string cxml)
{
  tinyxml2::XMLDocument xml;
  tinyxml2::XMLError err;

  err = xml.LoadFile(_xml_file.c_str());

  if (err == tinyxml2::XML_SUCCESS)
  {
    if(!build_and_validate_schema())
    {
        printf("Abort!\n");
        return false;
    }

    cxml::Tag* tag = iterate_tree(xml.FirstChildElement());
    if (!tag)
    {
        printf("Abort!\n");
        return false;
    }
  } else {
      printf("Malformed xml definition: %s\n", xml.ErrorStr());
      return false;
  }

  for (auto& t: tree_table)
  {
    uint8_t* header8 = reinterpret_cast<uint8_t*>(&t->th);
    for(size_t i = 0; i < sizeof(cxml::TreeHeader); i++)
        tree_table_bin.push_back(header8[i]);

    for (auto& a: t->kv)
    {
        uint8_t* kv8 = reinterpret_cast<uint8_t*>(&a);
        for(size_t i = 0; i < sizeof(cxml::TagAttr); i++)
            tree_table_bin.push_back(kv8[i]);
    }
  }

  // align all bins to 16
  // remember old sizes
  uint32_t tree_table_bin_size = tree_table_bin.size();
  binarray_align(tree_table_bin, 16);

  uint32_t id_table_bin_size = id_table_bin.size();
  binarray_align(id_table_bin, 16);

  uint32_t idhash_table_bin_size = idhash_table_bin.size();
  binarray_align(idhash_table_bin, 16);

  uint32_t string_table_bin_size = string_table_bin.size();
  binarray_align(string_table_bin, 16);

  uint32_t wstring_table_bin_size = wstring_table_bin.size();
  binarray_align(wstring_table_bin, 16);

  uint32_t hash_table_bin_size = hash_table_bin.size();
  binarray_align(hash_table_bin, 16);

  uint32_t intarray_table_bin_size = intarray_table_bin.size();
  binarray_align(intarray_table_bin, 16);

  uint32_t floatarray_table_bin_size = floatarray_table_bin.size();
  binarray_align(floatarray_table_bin, 16);

  cxml::Header head;
  head.magic[0] = _magic[0];
  head.magic[1] = _magic[1];
  head.magic[2] = _magic[2];
  head.magic[3] = _magic[3];
  head.version = _version;

  head.tree_offset = sizeof(cxml::Header);
  head.tree_size = tree_table_bin_size;

  head.idtable_offset = head.tree_offset + tree_table_bin.size();
  head.idtable_size = id_table_bin_size;

  head.idhashtable_offset = head.idtable_offset + id_table_bin.size();
  head.idhashtable_size = idhash_table_bin_size;

  head.stringtable_offset = head.idhashtable_offset + idhash_table_bin.size();
  head.stringtable_size = string_table_bin_size;

  head.wstringtable_offset = head.stringtable_offset + string_table_bin.size();
  head.wstringtable_size = wstring_table_bin_size;

  head.hashtable_offset = head.wstringtable_offset + wstring_table_bin.size();
  head.hashtable_size = hash_table_bin_size;

  head.intarraytable_offset = head.hashtable_offset + hash_table_bin.size();
  head.intarraytable_size = intarray_table_bin_size;

  head.floatarraytable_offset = head.intarraytable_offset + intarray_table_bin.size();
  head.floatarraytable_size = floatarray_table_bin_size;

  head.filetable_offset = head.floatarraytable_offset + floatarray_table_bin.size();
  head.filetable_size = file_table_bin.size(); // filetable isn't aligned

  FILE* fp = fopen(cxml.c_str(), "wb");
  fwrite(&head, sizeof(Header), 1, fp);
  fwrite(tree_table_bin.data(), tree_table_bin.size(), 1, fp);
  fwrite(id_table_bin.data(), id_table_bin.size(), 1, fp);
  fwrite(idhash_table_bin.data(), idhash_table_bin.size(), 1, fp);
  fwrite(string_table_bin.data(), string_table_bin.size(), 1, fp);
  fwrite(wstring_table_bin.data(), wstring_table_bin.size(), 1, fp);
  fwrite(hash_table_bin.data(), hash_table_bin.size(), 1, fp);
  fwrite(intarray_table_bin.data(), intarray_table_bin.size(), 1, fp);
  fwrite(floatarray_table_bin.data(), floatarray_table_bin.size(), 1, fp);
  fwrite(file_table_bin.data(), file_table_bin.size(), 1, fp);

  fclose(fp);

  return true;
}

void Compiler::generateRcd(std::string cxml, std::string rcd)
{
  // rcd generation
  FILE*fp = fopen(rcd.c_str(), "wb");
  fprintf(fp, "# generated by acdc from %s\n\n", cxml.c_str());

  for (auto&v : rcd_table)
  {
    fprintf(fp, "%s\n", v.c_str());
  }

  fclose(fp);
}

void Compiler::generateCHeader(std::string cxml, std::string header)
{
  // header generation

  fs::path p = header;
  std::string stem = p.filename();
  std::transform(stem.begin(), stem.end(), stem.begin(), ::toupper);
  std::replace( stem.begin(), stem.end(), '.', '_');

  FILE* fp = fopen(header.c_str(), "wb");

  fprintf(fp, "#ifndef __%s__\n", stem.c_str());
  fprintf(fp, "#define __%s__\n", stem.c_str());
  fprintf(fp, "\n//generated by acdc from %s\n\n", cxml.c_str());
  for (auto&v : idhash_table)
  {
    fprintf(fp, "#define %s (0x%08x)\n", v.second.second.c_str(), v.first);
  }
  fprintf(fp, "\n#endif // __%s__\n", stem.c_str());

  fclose(fp);

}


}
