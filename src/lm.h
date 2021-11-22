//
// Created by dh on 22.11.21.
//

#ifndef SRC_LM_H
#define SRC_LM_H


#include "PINode.h"
#include "PIHelper.h"

class lm {
    bool isFactLM = false;
    bool isActionLM = false;
    bool isAND = false;
    bool isOR = false;
    unordered_set<PINode*, PINodeHasher, PINodeComparator> lm;
};


#endif //SRC_LM_H
