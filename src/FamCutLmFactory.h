//
// Created by dh on 12.12.20.
//
// https://de.wikipedia.org/wiki/Max-Flow-Min-Cut-Theorem#Algorithmus_zum_Finden_minimaler_Schnitte

#ifndef SRC_FAMCUTLMFACTORY_H
#define SRC_FAMCUTLMFACTORY_H

#include "model.h"
#include "sasinvariants.h"
#include <vector>
#include <iostream>
#include "cassert"
#include "StaticS0Def.h"
#include "PINode.h"
#include "PIGraph.h"
#include "PIArc.h"
#include "LandmarkGraph.h"

using namespace std;

struct FAMmodifier {
    int action;
    int prec;
    int add;
    vector<int> staticPrecs;
};

class FamCutLmFactory {
    Domain domain;
    Problem problem;
    vector<FAMGroup> famGroups;
    set<int> invariant;

    vector<FAMmodifier *> *modifier;

    map<int, StaticS0Def *> staticS0;

    int getFAMMatch(PINode *n);

    void printFamGroup(int i);

    bool isCompatible(Task &t, PredicateWithArguments &arguments, FAMGroup &group);

    bool isNormalArc(int action, int relPrec, int relDel);

    // methods needed to integrate static preconditions

    StaticS0Def *getStaticS0Def(int predicateNo);

    void sortS0Def(PINode *partInstPrec, StaticS0Def *s0);

    void quicksort(StaticS0Def *A, int links, int rechts, vector<int> *sortBy);

    int teile(StaticS0Def *A, int links, int rechts, vector<int> *sortBy);

    bool smaller(StaticS0Def *A, int i, int *pivot, vector<int> *sortBy);

    bool greater(StaticS0Def *A, int i, int *pivot, vector<int> *sortBy);

public:
    FamCutLmFactory(Domain domain, Problem problem, vector<FAMGroup> famGroups);

    void generateLMs();

    bool nodeBasedLMs = true;
    bool cutBasedLMs = false;

    LandmarkGraph *generateLMs(PINode *node);

    void lmDispatcher(LandmarkGraph *lmg, int nodeID);

    LandmarkGraph *generateCutLMs(PIGraph &dtg, PINode *targetNode, int initialNodeID);

    LandmarkGraph *generateNodeLMs(PIGraph &dtg, PINode *targetNode, int initialNodeID);

    PINode *getInitNode(const FAMGroup &fam, const vector<int> &setFreeVars);

    LandmarkGraph *generatePrecNodes(Landmark *pLandmark);

    bool containedInS0(PINode *pNode);
};


#endif //SRC_FAMCUTLMFACTORY_H
