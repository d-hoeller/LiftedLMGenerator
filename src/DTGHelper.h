//
// Created by dh on 12.11.21.
//

#ifndef SRC_DTGHELPER_H
#define SRC_DTGHELPER_H

#include "DTGnode.h"

struct DTGnodeHasher {
    size_t operator()(const DTGnode * p) const {
        size_t res = hash<int>()(p->schemaIndex);
        for (int i: p->consts) {
            res = (res << 1);
            res = res ^ hash<int>()(i);
        }
        //cout << "HASH: " << res << endl;
        return res;
    }
};

struct DTGnodeComparator {
    bool operator()(const DTGnode* obj1, const DTGnode* obj2) const {
        if (obj1->schemaIndex != obj2->schemaIndex)
            return false;
        if (obj1->consts.size() != obj2->consts.size())
            return false;
        for (int i = 0; i < obj1->consts.size(); i++) {
            if (obj1->consts[i] != obj2->consts[i])
                return false;
        }
        return true;
    }
};

#endif //SRC_DTGHELPER_H
