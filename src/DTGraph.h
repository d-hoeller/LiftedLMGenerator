//
// Created by dh on 12.11.21.
//

#ifndef SRC_DTGRAPH_H
#define SRC_DTGRAPH_H

#include "DTGnode.h"
#include "DTarc.h"
#include "DTGHelper.h"
#include <unordered_set>
#include <unordered_map>
#include "model.h"

class DTGraph {
    int id = 0;
public:
    unordered_set<DTGnode*, DTGnodeHasher, DTGnodeComparator> N;
    unordered_map<int, DTGnode*> iToN;
    unordered_map<int, unordered_map<int, unordered_set<DTarc*>>> successors;
    unordered_map<int, unordered_map<int, unordered_set<DTarc*>>> predecessors;

    set<int> deactivatedNodes;

//    void addArc(int from, int to);
    void addArc(int from, int to, DTGnode *partInstAction);

    bool reachable(set<int> &from, set<int> &to);

    void showDot(Domain domain);

    void insertNode(DTGnode *pGnode);
};


#endif //SRC_DTGRAPH_H
