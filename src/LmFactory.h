//
// Created by dh on 02.12.21.
//

#ifndef SRC_LMFACTORY_H
#define SRC_LMFACTORY_H


#include "model.h"
#include "PINode.h"
#include "LandmarkGraph.h"

class LmFactory {
public:
    LmFactory(Domain domain, Problem problem);

    Domain domain;
    Problem problem;

    vector<pair<int, int>>* achiever;

    bool containedInS0(PINode *pNode);
    PINode* getPINode(Fact &f);
    void addHeuristicOrderings(LandmarkGraph* g);
};


#endif //SRC_LMFACTORY_H
