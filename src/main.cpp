#include "cxml/compiler.h"
#include "utils/cxxopts.hpp"

int main(int argc, char* argv[])
{
  // todo: getopt
  cxxopts::Options options("acdc", "Another CXML (De)Compiler");

  options
    .allow_unrecognised_options()
    .add_options()
      ("c,compile", "Compile")
      ("h,help", "Print usage")
  ;

  options.add_options("Compile")
      ("i,input", "Input xml file", cxxopts::value<std::string>())
      ("s,schema", "Cxml schema file", cxxopts::value<std::string>())
      ("o,output", "Output cxml file", cxxopts::value<std::string>())
      ("r,output_rcd", "Output rcd file (optional)", cxxopts::value<std::string>())
      ("d,output_header", "Output C header file (optional)", cxxopts::value<std::string>())
  ;

  auto result = options.parse(argc, argv);

  if (result.count("help") || !result.count("compile"))
  {
    std::cout << options.help({"","Compile"}) << std::endl;
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


  return 0;
}

