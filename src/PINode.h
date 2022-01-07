//
// Created by dh on 12.11.21.
//

#ifndef SRC_PINODE_H
#define SRC_PINODE_H

#include <vector>
//#include "PIGraph.h"
//#include "DTGHelper.h"
#include <unordered_set>
#include "model.h"

using namespace std;

class PINode {
public:
    PINode();

    PINode(PINode *pGnode);

    int schemaIndex; // might be action or predicate schema
    vector<int> consts;
    int nodeID = -1;
    bool deactivated = false;
//    unordered_set<PINode*, PINodeHasher, PINodeComparator> N;
    bool abstractionOf(PINode *pGnode);
    bool abstractionNotEqOf(PINode *pGnode);

    void printFact(Domain &domain, ostream& stream);
    void printAction(Domain &domain);
};


#endif //SRC_PINODE_H
