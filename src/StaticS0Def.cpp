//
// Created by dh on 12.11.21.
//

#include "StaticS0Def.h"
#include "cassert"

int StaticS0Def::get(int iEntry, int iArg) {
        assert((iEntry * sizeOfEntry + iArg) < (numEntries * sizeOfEntry));
        return entries[iEntry * sizeOfEntry + iArg];
}

void StaticS0Def::set(int iEntry, int iArg, int val) {
        assert((iEntry * sizeOfEntry + iArg) < (numEntries * sizeOfEntry));
        entries[iEntry * sizeOfEntry + iArg] = val;
}
