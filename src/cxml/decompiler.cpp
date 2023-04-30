#include "decompiler.h"
#include <zlib.h>
#include <filesystem>

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
      fseek(fp, header.hashtable_offset + handle, SEEK_SET);
      std::string attr_name;

      uint32_t data;
      fread(&data, 4, 1, fp);
      char cdata[16];
      sprintf(cdata, "0x%08x", data);
      attr_name = cdata;
      return attr_name;
  }

  std::string Decompiler::get_from_idhashtable(int32_t handle)
  {
      fseek(fp, header.idhashtable_offset + handle + 4, SEEK_SET);
      std::string attr_name;

      uint32_t data;
      fread(&data, 4, 1, fp);

      // todo: if we have .rcd - search for corresponding id

      char cdata[16];
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
      fseek(fp, header.filetable_offset + handle, SEEK_SET);
      uint8_t* file = (uint8_t*)malloc(size);
      fread(file, size, 1, fp);

      if (compressed)
      {
        uint32_t zout_len = orig_size;
        uint8_t* zout = (uint8_t*)malloc(zout_len);
        int ret = uncompress(zout, (uLongf*)&zout_len, file, size);
        if (ret != Z_OK)
        {
            printf("Error: failed to decompress file %s!", name.c_str());
            exit(-1);
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

        bool is_file = false;
        uint32_t file_size = 0;
        uint32_t file_orig_size = 0;
        uint32_t file_offset = 0;
        std::string file_attr_name;
        bool file_compress = false;
        for(int i = 0; i < theader->num_attributes; i++)
        {
            cxml::TagAttr* attr = (cxml::TagAttr*)(tree + tree_offset);
            std::string attr_name = get_from_stringtable(attr->name_handle);

            std::string val;
            std::string type;
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
                        file_compress = true;
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
           std::string val = std::to_string(file_offset) + std::string(".bin");
           // todo: if we have filename from rcd - update it
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


  bool Decompiler::decompile(std::string out)
  {
    fs::path p = fs::absolute(out);
    _outdir = p.remove_filename();
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

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    parse_tree(0, nullptr, doc);
    doc.SaveFile(out.c_str());

    return true;
  }
}

