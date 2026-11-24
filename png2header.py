#!/usr/bin/python3
from pathlib import Path
import re
import sys


files = []


def normalize_key(filename):
    posix = filename.replace('\\', '/')
    marker = 'assets/'
    idx = posix.find(marker)
    if idx != -1:
        return posix[idx:]
    return posix


def munge_filename(key):
    return 'png_' + re.sub(r'[^A-Za-z0-9_]', '_', key)


def file_to_bytes(filename):
    data = Path(filename).read_bytes()

    key = normalize_key(filename)
    varname = munge_filename(key)

    print("const unsigned char ", varname, "[] = {")

    ind = 0
    while ind < len(data):
        if ind != 0:
            print(", ", end='')
        if ind % 32 == 0:
            print("")
        print(data[ind], end='')
        ind = ind + 1

    print("};")

    files.append([key, varname, len(data)])
    

def print_prelogue():
    print("#include <map>")
    print("#include <string>")
    print("")

def print_prologue():
    print("")
    print("std::map<std::string, const unsigned char *> datamap;")
    print("std::map<std::string, unsigned int> sizemap;")
    print("static bool initialized = false;")
    print("")
    print("void fill_png_maps(void)");
    print("{");
    print("   if (initialized)");
    print("          return;");
    print("   initialized = true;");
    for f in files:
        (filename, varname, len) = f
        print("    datamap[\"" + filename + "\"] = "+varname+";")
        print("    sizemap[\"" + filename + "\"] =",len,";")
    print("}")
    print("")
    print("void clear_png_maps(void)")
    print("{")
    print("   datamap.clear();")
    print("   sizemap.clear();")
    print("}")
    
    
def main():
    print_prelogue()
    for filename in sys.argv[1:]:
        file_to_bytes(filename)
        
    print_prologue()
    


if __name__ == '__main__':
    main()
