# acdc
Another CXML (De)Compiler

## Features
* Supports custom schema definitions with magic and version. Definitions for RCS and RCO  are provided
* Supports zlib-compressing files in `file` attr if `compression="on"` attribute exists. Auto-adds `origsize` attr.
* Supports producing debug files (rcd)
* Supports producing c header files
* Supports already hashed idhash fields (e.g. from other decompilators, `id="0xDEADBEEF"` will be put into idhash table as is)

## Credits
* [PoS](https://github.com/Princess-of-Sleeping) for rco decompiler
* [Graphene](https://github.com/GrapheneCt) for rcs/rco schema
