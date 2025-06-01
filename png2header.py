#!/usr/bin/python3
from pathlib import Path
import sys



files = [];

def munge_filename(filename):
    newname = filename.replace('/', '_');
    newname = newname.replace('.', '_');
    return newname

def file_to_bytes(filename):
    data = Path(filename).read_bytes()
    
    varname = munge_filename(filename)
    
    print("unsigned char ",varname,"[] = {")
    
    ind = 0
    while ind < len(data):
        if ind != 0:
            print(", ", end='')
        if ind % 32 == 0:
            print("")
        print(data[ind], end='')
        ind = ind + 1
    
    print("};");
    
    files.append([filename, varname, len(data)]);
    

def print_prelogue():
    print("#include <map>")
    print("#include <string>")
    print("")

def print_prologue():
    print("")
    print("std::map<std::string, unsigned char *> datamap;")
    print("std::map<std::string, unsigned int> sizemap;")
    print("")
    print("void fill_png_maps(void)");
    print("{");
    for f in files:
        (filename, varname, len) = f
        print("    datamap[\"" + filename + "\"] = "+varname+";")
        print("    sizemap[\"" + filename + "\"] =",len,";")
    print("}")
    
    
def main():
    print_prelogue()
    for filename in sys.argv[1:]:
        file_to_bytes(filename)
        
    print_prologue()
    


if __name__ == '__main__':
    main()
