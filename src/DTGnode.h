//
// Created by dh on 12.11.21.
//

#ifndef SRC_DTGNODE_H
#define SRC_DTGNODE_H

#include <vector>
//#include "DTGraph.h"
//#include "DTGHelper.h"
#include <unordered_set>
#include "model.h"

using namespace std;

class DTGnode {
public:
    DTGnode();

    DTGnode(DTGnode *pGnode);

    int schemaIndex; // might be action or predicate schema
    vector<int> consts;
    int id = -1;
    bool deactivated = false;
//    unordered_set<DTGnode*, DTGnodeHasher, DTGnodeComparator> N;
    bool abstractionOf(DTGnode *pGnode);

    void printFact(Domain &domain);
};


#endif //SRC_DTGNODE_H
