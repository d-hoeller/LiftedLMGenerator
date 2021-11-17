//
// Created by dh on 12.11.21.
//

#ifndef SRC_PIARC_H
#define SRC_PIARC_H

#include "PINode.h"

class PIArc {
public:
    int arcid = -1;
    const int arccosts = 1;
//    int from = -1;
//    int to = -1;
    PINode *partInstantiation;
};


#endif //SRC_PIARC_H
