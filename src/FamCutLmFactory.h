//
// Created by dh on 12.12.20.
//
// https://de.wikipedia.org/wiki/Max-Flow-Min-Cut-Theorem#Algorithmus_zum_Finden_minimaler_Schnitte

#ifndef SRC_FAMCUTLMFACTORY_H
#define SRC_FAMCUTLMFACTORY_H

#include "model.h"
#include "sasinvariants.h"
//#include "LDTG.h"
#include <vector>
#include <iostream>
#include "cassert"
#include "StaticS0Def.h"
#include "PINode.h"

using namespace std;

struct GoalMatch {
    int iFamGroup;
    int iLiteral;
    int iGoal;
};

struct FAMmodifier {
    int action;
    int prec;
    int add;
    vector<int> staticPrecs;
};



class FamCutLmFactory {
    Domain domain;
    Problem problem;
    vector <FAMGroup> famGroups;
    vector<GoalMatch*> matches;
    set<int> invariant;

    vector<FAMmodifier*>* modifier;

    map<int, StaticS0Def*> staticS0;
public:
    FamCutLmFactory(Domain domain, Problem problem, vector<FAMGroup> famGroups);
    void findGoalMatches();
    void findGoalMatch(int ig, int ifg);
    void addGoalMatch(int iGroup, int iLiteral, int iGoal);

    void generateLMs();
    void generateLMs(int ig);

    void printFamGroup(int i);
    void printMatch(int i);

//    vector<LDTG *> *groundNodes(LDTG *node, int iFG, int iLit, vector<int> cVarNeedToGround);

//    void printNode(LDTG *pLdtg);

    bool isCompatible(Task &t, PredicateWithArguments &arguments, FAMGroup &group);

    bool isNormalArc(int action, int relPrec, int relDel);

    StaticS0Def *getStaticS0Def(int predicateNo);

    void sortS0Def(PINode *partInstPrec, StaticS0Def *s0);

    void quicksort(StaticS0Def *A, int links, int rechts, vector<int> *sortBy);

    int teile(StaticS0Def *A, int links, int rechts, vector<int> *sortBy);

    bool smaller(StaticS0Def *A, int i, int* pivot, vector<int> *sortBy);

    bool greater(StaticS0Def *A, int i, int* pivot, vector<int> *sortBy);
};


#endif //SRC_FAMCUTLMFACTORY_H
