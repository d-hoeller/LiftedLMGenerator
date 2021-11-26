//
// Created by dh on 12.11.21.
//

#ifndef SRC_PIHELPER_H
#define SRC_PIHELPER_H

#include "PINode.h"

struct PINodeHasher {
    size_t operator()(const PINode * p) const {
        size_t res = hash<int>()(p->schemaIndex);
        for (int i: p->consts) {
            res = (res << 1);
            if (i >= 0) {
                res = res ^ hash<int>()(i);
            } else {
                res = res ^ hash<int>()(-1);
            }
        }
        //cout << "HASH: " << res << endl;
        return res;
    }
};

struct PINodeComparator {
    bool operator()(const PINode* obj1, const PINode* obj2) const {
        if (obj1->schemaIndex != obj2->schemaIndex)
            return false;
        if (obj1->consts.size() != obj2->consts.size())
            return false;
        for (int i = 0; i < obj1->consts.size(); i++) {
            if ((obj1->consts[i] >= 0) || (obj2->consts[i] >= 0)) {
                if (obj1->consts[i] != obj2->consts[i]) {
                    return false;
                }
            }
        }
        return true;
    }
};

#endif //SRC_PIHELPER_H
