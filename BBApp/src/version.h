#ifndef VERSION_H
#define VERSION_H

#define VER_FILEVERSION             3,0,5
#define VER_FILEVERSION_STR         "3.0.5"

#define VER_PRODUCTVERSION          3,0,5
#define VER_PRODUCTVERSION_STR      "3.0.5"

#define VER_COMPANYNAME_STR         "Signal Hound"
#define VER_FILEDESCRIPTION_STR     "Spectrum Analyzer Software"
#define VER_INTERNALNAME_STR        "Spike"
#define VER_LEGALCOPYRIGHT_STR      "Copyright 2015 Signal Hound"
#define VER_LEGALTRADEMARKS1_STR    "All Rights Reserved"
#define VER_LEGALTRADEMARKS2_STR    VER_LEGALTRADEMARKS1_STR
#define VER_ORIGINALFILENAME_STR    "Spike.exe"
#define VER_PRODUCTNAME_STR         "Spike"

#if _WIN64
#define IS_32BIT 0
#else
#define IS_32BIT 1
#endif

#endif // VERSION_H
