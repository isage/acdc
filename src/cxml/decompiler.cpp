#include "decompiler.h"
#include <zlib.h>
#include <filesystem>
#include <fstream>
#include <sstream>


namespace fs = std::filesystem;

namespace cxml {
  Decompiler::Decompiler(std::string cxml, std::string rcd) : _cxml(cxml), _rcd(rcd)
  {
  }

  std::string Decompiler::get_from_stringtable(int32_t handle)
  {
      // search in stringtable
      fseek(fp, header.stringtable_offset + handle, SEEK_SET);
      std::string attr_name;
      // read null-terminated string
      uint8_t ch = 0;
      fread(&ch, 1, 1, fp);
      while(ch != 0)
      {
          attr_name.push_back(ch);
          fread(&ch, 1, 1, fp);
      }
      return attr_name;
  }

  std::string Decompiler::get_from_wstringtable(int32_t handle, int32_t size)
  {
      // search in stringtable
      fseek(fp, header.wstringtable_offset + handle*2, SEEK_SET);
      std::string attr_name;

      uint8_t* data = (uint8_t*)malloc(size+2);
      fread(data, size+2, 1, fp);
      uint8_t* data_out = (uint8_t*)malloc(size*4);
      utf16_to_utf8((uint16_t*)data, data_out);
      free(data);
      attr_name = std::string((char*)data_out);
      free(data_out);
      return attr_name;
  }

  std::string Decompiler::get_from_hashtable(int32_t handle)
  {
      fseek(fp, header.hashtable_offset + handle*4, SEEK_SET);
      std::string attr_name;

      uint32_t data;
      fread(&data, 4, 1, fp);
      char cdata[16];

      sprintf(cdata, "%08x", data);
      if (_rcd_table.count(cdata))
      {
        attr_name = _rcd_table.at(cdata).at("id");
        return attr_name;
      }

      sprintf(cdata, "0x%08x", data);
      attr_name = cdata;
      return attr_name;
  }

  std::string Decompiler::get_from_idhashtable(int32_t handle, bool demangle)
  {
      fseek(fp, header.idhashtable_offset + handle + 4, SEEK_SET);
      std::string attr_name;

      uint32_t data;
      fread(&data, 4, 1, fp);

      char cdata[16];

      //if we have .rcd - search for corresponding id
      sprintf(cdata, "%08x", data);
      if (_rcd_table.count(cdata) && demangle)
      {
        attr_name = _rcd_table.at(cdata).at("id");
        return attr_name;
      }
      if (!demangle)
          sprintf(cdata, "%08x", data);
      else
          sprintf(cdata, "0x%08x", data);
      attr_name = cdata;
      return attr_name;
  }

  std::string Decompiler::get_from_intarraytable(int32_t handle, int32_t count)
  {
      fseek(fp, header.intarraytable_offset + handle*4, SEEK_SET);
      std::string attr_name;
      std::vector<std::string> strv;
      for(int i = 0; i < count; i++)
      {
          int32_t data;
          fread(&data, 4, 1, fp);
          strv.push_back(std::to_string(data));
      }

      attr_name = join(strv, ", ");

      return attr_name;
  }

  std::string Decompiler::get_from_floatarraytable(int32_t handle, int32_t count)
  {
      fseek(fp, header.floatarraytable_offset + handle*4, SEEK_SET);
      std::string attr_name;
      std::vector<std::string> strv;
      for(int i = 0; i < count; i++)
      {
          uint32_t data;
          fread(&data, 4, 1, fp);
          float* f = (float*)&data;
          strv.push_back(float_to_string(*f));
      }

      attr_name = join(strv, ", ");

      return attr_name;
  }

  std::string Decompiler::get_from_idtable(int32_t handle)
  {
      fseek(fp, header.idtable_offset + handle + 4, SEEK_SET);
      std::string attr_name;
      // read null-terminated string
      uint8_t ch = 0;
      fread(&ch, 1, 1, fp);
      while(ch != 0)
      {
          attr_name.push_back(ch);
          fread(&ch, 1, 1, fp);
      }
      return attr_name;
  }

  void Decompiler::extract_from_filetable(std::string name, int32_t handle, int32_t size, bool compressed, uint32_t orig_size)
  {
      fs::path p(name);
      fs::create_directories(fs::absolute(p.remove_filename()));

      fseek(fp, header.filetable_offset + handle, SEEK_SET);
      uint8_t* file = (uint8_t*)malloc(size);
      fread(file, size, 1, fp);

      if (compressed && orig_size == 0)
      {
            printf("Warning: failed to decompress file (origsize=0) %s!\n", name.c_str());
            FILE* out = fopen(name.c_str(), "wb");
            fwrite(file, size, 1, out);
            fclose(out);
            free(file);
            return;
      }

      if (compressed)
      {
        uint32_t zout_len = orig_size;
        uint8_t* zout = (uint8_t*)malloc(zout_len);
        int ret = uncompress(zout, (uLongf*)&zout_len, file, size);
        if (ret != Z_OK)
        {
            printf("Warning: failed to decompress file %s!\n", name.c_str());
            FILE* out = fopen(name.c_str(), "wb");
            fwrite(file, size, 1, out);
            fclose(out);
            free(file);
            return;
        }
        FILE* out = fopen(name.c_str(), "wb");
        fwrite(zout, zout_len, 1, out);
        fclose(out);
        free(zout);
        free(file);
        return;
      }

      FILE* out = fopen(name.c_str(), "wb");
      fwrite(file, size, 1, out);
      fclose(out);
      free(file);
  }

  void Decompiler::parse_tree(int32_t tree_offset, tinyxml2::XMLElement* parent, tinyxml2::XMLDocument& doc)
  {
    while(1)
    {
        cxml::TreeHeader* theader = (cxml::TreeHeader*)(tree + tree_offset);
        tree_offset += sizeof(cxml::TreeHeader);


        std::string node_name = get_from_stringtable(theader->name_handle);

        tinyxml2::XMLElement* el;
        if (!parent)
        {
            el = doc.NewElement(node_name.c_str());
        }
        else
            el = parent->InsertNewChildElement(node_name.c_str());

        defElement* e;
        if (_definition.count(node_name))
        {
            e = _definition.at(node_name);
        }
        else
        {
            e = new defElement();
            _definition.emplace(node_name, e);
        }

        if(parent)
        {
            e->parents.insert(parent->Name());
        }

        bool is_file = false;
        uint32_t file_size = 0;
        uint32_t file_orig_size = 0;
        uint32_t file_offset = 0;
        std::string file_attr_name;
        std::string file_attr_hash;
        bool file_compress = false;

        for(int i = 0; i < theader->num_attributes; i++)
        {
            cxml::TagAttr* attr = (cxml::TagAttr*)(tree + tree_offset);
            std::string attr_name = get_from_stringtable(attr->name_handle);

            std::string val;
            std::string type;

            if (!e->attributes.count(attr_name))
            {
                e->attributes.emplace(attr_name, (cxml::Attr)attr->type);
            }

            switch((cxml::Attr)attr->type)
            {
                default: break;
                case cxml::Attr::None: break; // TODO: error
                case cxml::Attr::Int:
                {
                    val = std::to_string(attr->offset);
                    if (attr_name.compare("origsize") == 0)
                    {
                        file_orig_size = attr->offset;
                    }
                    break;
                }
                case cxml::Attr::Float:
                {
                    float* f = (float*)&attr->offset;
                    val = float_to_string(*f);
                    break;
                }
                case cxml::Attr::String:
                {
                    val = get_from_stringtable(attr->offset);
                    if (attr_name.compare("compress") == 0 && val.compare("on") == 0)
                    {
                        file_compress = true;
                    }
                    break;
                }
                case cxml::Attr::WString:
                {
                    val = get_from_wstringtable(attr->offset, attr->size);
                    break;
                }
                case cxml::Attr::Hash:
                {
                    val = get_from_hashtable(attr->offset);
                    break;
                }
                case cxml::Attr::IntArray:
                {
                    val = get_from_intarraytable(attr->offset, attr->size);
                    break;
                }
                case cxml::Attr::FloatArray:
                {
                    val = get_from_floatarraytable(attr->offset, attr->size);
                    break;
                }
                case cxml::Attr::File:
                {
                    val = std::to_string(attr->offset) + std::string(".bin");
                    is_file = true;
                    file_offset = attr->offset;
                    file_size = attr->size;
                    file_attr_name = attr_name;
                    break;
                }
                case cxml::Attr::ID:
                {
                    val = get_from_idtable(attr->offset);
                    break;
                }
                case cxml::Attr::IDRef:
                {
                    val = get_from_idtable(attr->offset);
                    break;
                }
                case cxml::Attr::IDHash:
                {
                    val = get_from_idhashtable(attr->offset);
                    file_attr_hash = get_from_idhashtable(attr->offset, false);
                    break;
                }
                case cxml::Attr::IDHashRef:
                {
                    val = get_from_idhashtable(attr->offset);
                    break;
                }
            }

            el->SetAttribute(attr_name.c_str(), val.c_str());
            tree_offset += sizeof(cxml::TagAttr);
        }

        // check if we have file and unpack if needed
        if(is_file)
        {
           std::string val  = std::string(el->Attribute("id")) + std::string(".bin");
           // if we have filename from rcd - update it
           if (!file_attr_hash.empty() && _rcd_table.count(file_attr_hash))
           {
             if (_rcd_table.at(file_attr_hash).count("src"))
             {
                val = _rcd_table.at(file_attr_hash).at("src");
             }
           }

           extract_from_filetable(_outdir + val, file_offset, file_size, file_compress, file_orig_size);
           // update attr
           el->SetAttribute(file_attr_name.c_str(), val.c_str());
           // remove origsize
           el->DeleteAttribute("origsize");
        }

        if (theader->first_child_elm_offset != -1)
        {
            parse_tree(theader->first_child_elm_offset, el, doc);
        }

        if (!parent)
        {
            doc.InsertEndChild(el);
        }
        if (theader->next_elm_offset == -1) break;
        tree_offset = theader->next_elm_offset;
    }
  }

  void Decompiler::parse_rcd()
  {
    if (!_rcd.empty())
    {
        std::ifstream rcdf(_rcd);
        if (!rcdf.is_open()) return;
        std::string line;
        while (std::getline(rcdf, line))
        {
            trim(line);
            if(!line.empty() && line[0] != '#')
            {
                std::vector<std::string> tokens = split(line, ' ');
                std::vector<std::string> keyhash = split(tokens[0], ':');
                std::vector<std::string> attrs = split(tokens[1], ',');
                std::string key = keyhash[1].substr(0, keyhash[1].find("("));

                std::map<std::string, std::string> attrmap;

                for (auto& a: attrs)
                {
                    std::vector<std::string> attrval = split(a,':');
                    attrmap.emplace(attrval[0], attrval[1]);
                }
                _rcd_table.emplace(key, attrmap);
            }
        }
    }
  }

  void Decompiler::generate_cxmldef(std::string filename)
  {
    std::map<cxml::Attr, std::string> attr_types = {
      {cxml::Attr::None, "none"},
      {cxml::Attr::Int, "int"},
      {cxml::Attr::Float, "float"},
      {cxml::Attr::String, "string"},
      {cxml::Attr::WString, "wstring"},
      {cxml::Attr::Hash, "hash"},
      {cxml::Attr::IntArray, "intarray"},
      {cxml::Attr::FloatArray, "floatarray"},
      {cxml::Attr::File, "file"},
      {cxml::Attr::ID, "id"},
      {cxml::Attr::IDRef, "idref"},
      {cxml::Attr::IDHash, "idhash"},
      {cxml::Attr::IDHashRef, "idhashref" }
    };

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());


    tinyxml2::XMLElement* wrapper = doc.NewElement("cxml");
    doc.InsertEndChild(wrapper);
    char magic[5] = {0};
    memcpy(magic, header.magic, 4);

    char version[8] = {0};
    snprintf(version, 7, "0x%04X", header.version);
    memcpy(magic, header.magic, 4);
    wrapper->SetAttribute("magic", magic);
    wrapper->SetAttribute("version", version);

    for(auto& child: _definition)
    {
        tinyxml2::XMLElement* child_element = wrapper->InsertNewChildElement("element");
        child_element->SetAttribute("name", child.first.c_str());
        if(child.second->parents.size() > 0)
            child_element->SetAttribute("parents", join(child.second->parents, ", ").c_str());
        std::vector<std::string> attributes;
        for(auto& a: child.second->attributes)
        {
            std::string attr = a.first + std::string(":") + attr_types.at(a.second);
            attributes.push_back(attr);
        }
        if(attributes.size() > 0)
            child_element->SetAttribute("attributes", join(attributes, ", ").c_str());
    }

    doc.SaveFile(filename.c_str());

  }


  bool Decompiler::decompile(std::string out)
  {
    fs::path p = fs::absolute(out);
    _outdir = p.remove_filename().string();
    fs::create_directories(p.remove_filename());

    fp = fopen(_cxml.c_str(), "rb");
    if(!fp)
    {
        std::cout << "Can't open cxml file " << _cxml << std::endl;
        return false;
    }

    size_t result = fread(&header, sizeof(cxml::Header), 1, fp);
    if (result < 1)
    {
        std::cout << "Malformed cxml file" << std::endl;
        return false;
    }

    tree = (uint8_t*)malloc(header.tree_size);

    result = fread(tree, header.tree_size, 1, fp);
    if (result < 1)
    {
        std::cout << "Malformed cxml file" << std::endl;
        return false;
    }

    // parse rcd if we have one
    parse_rcd();

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    parse_tree(0, nullptr, doc);
    doc.SaveFile(out.c_str());


    return true;
  }
}

