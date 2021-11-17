//
// Created by dh on 12.11.21.
//

#ifndef SRC_DTARC_H
#define SRC_DTARC_H

#include "DTGnode.h"

class DTarc {
public:
    const int arccosts = 1;
    int from = -1;
    int to = -1;
    DTGnode *partInstAction;

    bool deactivated = false;
};


#endif //SRC_DTARC_H
