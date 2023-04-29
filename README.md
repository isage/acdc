# acdc
Another CXML (De)Compiler

## Features
* Supports custom schema definitions with magix and version. Definitions for RCS and RCO  are provided
* Supports producing debug files (rcd)
* Supports producing c header files
* Supports already hashed idhash fields (e.g. from other decompilators, `id="0xDEADBEEF"` will be put into idhash table as is)

## Todo
cxml decompilation with rcd support

## Credits
* [PoS](https://github.com/Princess-of-Sleeping) for rco decompiler
* [Graphene](https://github.com/GrapheneCt) for rcs/rco schema
