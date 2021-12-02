//
// Created by dh on 02.12.21.
//

#ifndef SRC_NECSUBLMFACTORY_H
#define SRC_NECSUBLMFACTORY_H


#include "model.h"
#include "Landmark.h"
#include "LandmarkGraph.h"
#include "LmFactory.h"

class NecSubLmFactory: public LmFactory{
    int keepIfBound = 1;
public:
    NecSubLmFactory(Domain &domain, Problem &problem);

    LandmarkGraph *getLandmarks(PINode *pNode);
};


#endif //SRC_NECSUBLMFACTORY_H
