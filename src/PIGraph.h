//
// Created by dh on 12.11.21.
//

#ifndef SRC_PIGRAPH_H
#define SRC_PIGRAPH_H

#include "PINode.h"
#include "PIArc.h"
#include "PIHelper.h"
#include <unordered_set>
#include <unordered_map>
#include "model.h"
#include "model.h"
#include "LandmarkGraph.h"
#include <cassert>
#include <fstream>
#include <bits/stdc++.h>

// partial instantiation graph
class PIGraph {
    int nodeID = 0;
    int arcID = 0;
public:
    unordered_set<PINode*, PINodeHasher, PINodeComparator> N;
    unordered_map<int, unordered_map<int, unordered_set<PIArc*>>> successors;
    unordered_map<int, unordered_map<int, unordered_set<PIArc*>>> predecessors;

    set<int> deactivatedNodes;
    set<int> deactivatedArcs;

    void addArc(int from, int to, PINode* ArcLabel);

    void addNode(PINode *pGnode);

    bool reachable(set<int> &from, set<int> &to);

    void showDot(Domain domain);

    PINode *getNode(int nodeID);
};



#endif //SRC_PIGRAPH_H
