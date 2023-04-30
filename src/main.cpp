#include "cxml/compiler.h"
#include "cxml/decompiler.h"
#include "utils/cxxopts.hpp"

int main(int argc, char* argv[])
{
  cxxopts::Options options("acdc", "Another CXML (De)Compiler");

  options
    .allow_unrecognised_options()
    .add_options()
      ("c,compile", "Compile")
      ("d,decompile", "Decompile")
      ("h,help", "Print usage")
  ;

  options.add_options("Compile")
      ("i,input", "Input xml file", cxxopts::value<std::string>())
      ("s,schema", "Cxml schema file", cxxopts::value<std::string>())
      ("o,output", "Output cxml file", cxxopts::value<std::string>())
      ("r,output_rcd", "Output rcd file (optional)", cxxopts::value<std::string>())
      ("x,output_cheader", "Output C header file (optional)", cxxopts::value<std::string>())
  ;

  options.add_options("Decompile")
      ("cxml", "Input xml file", cxxopts::value<std::string>())
      ("xml", "Output xml file", cxxopts::value<std::string>())
      ("rcd", "Input rcd file (optional)", cxxopts::value<std::string>())
  ;

  auto result = options.parse(argc, argv);

  if (result.count("help") || (!result.count("compile") && !result.count("decompile")))
  {
    std::cout << options.help({"","Compile","Decompile"}) << std::endl;
    return 0;
  }

  if (result.count("compile"))
  {
      if (!result.count("input") || !result.count("output") || !result.count("schema"))
      {
        std::cout << "Compile (-c) requires input (-i) and output (-o) files and schema (-s)" << std::endl;
        return -1;
      }
      std::string in = result["input"].as<std::string>();
      std::string out = result["output"].as<std::string>();
      std::string schema = result["schema"].as<std::string>();

      cxml::Compiler compiler(schema, in);
      if(!compiler.compile(out))
      {
        return -1;
      }

      if (result.count("output_rcd"))
      {
          compiler.generateRcd(in, result["output_rcd"].as<std::string>());
      }

      if (result.count("output_header"))
      {
          compiler.generateCHeader(in, result["output_header"].as<std::string>());
      }
  }
  else if (result.count("decompile"))
  {
      if (!result.count("cxml") || !result.count("xml"))
      {
        std::cout << "Decompile (-d) requires input (--cxml) and output (--xml)" << std::endl;
        return -1;
      }

      std::string in = result["cxml"].as<std::string>();
      std::string out = result["xml"].as<std::string>();
      std::string rcd;
      if (result.count("rcd"))
      {
          rcd = result["rcd"].as<std::string>();
      }

      cxml::Decompiler decompiler(in, rcd);
      if(!decompiler.decompile(out))
      {
        return -1;
      }

  }

  return 0;
}

