//
// Created by dh on 22.11.21.
//

#ifndef SRC_LANDMARK_H
#define SRC_LANDMARK_H


#include "PINode.h"
#include "PIHelper.h"

enum LMType { FactAND, FactOR, ActionAND, ActionOR };

struct mycompare {
    bool operator() (const PINode *x, const PINode *y) const {
        if (x->schemaIndex != y->schemaIndex) {
            return x->schemaIndex < y->schemaIndex;
        }
        if(x->consts.size() != y->consts.size()) {
            return x->consts.size() < y->consts.size();
        }
        for (int i = 0; i < x->consts.size(); i++) {
            if ((x->consts[i] >= 0) || (y->consts[i] >= 0)) {
                if (x->consts[i] != y->consts[i]) {
                    return x->consts[i] < y->consts[i];
                }
            }
        }
        return false;
    }
};

class Landmark {
public:
    bool isInS0 = false; // just for displaying
    string name = "";
    bool isDummy = false;
    Landmark(LMType type);

    bool isFactLM = false;
    bool isActionLM = false;
    bool isAND = false;
    bool isOR = false;
    set<PINode*, mycompare> lm;
    Landmark(PINode *pNode, LMType t);
    Landmark(Landmark *that);
    Landmark();

    int nodeID;

    void makeDummy(string name);

    PINode *getFirst();
};


#endif //SRC_LANDMARK_H
