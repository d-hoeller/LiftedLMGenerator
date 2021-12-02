//
// Created by dh on 22.11.21.
//

#include "Landmark.h"

Landmark::Landmark(PINode *pNode, LMType t) {
    this->isAND = ((t == ActionAND) || (t == FactAND));
    this->isOR = ((t == ActionOR) || (t == FactOR));
    this->isFactLM = ((t == FactAND) || (t == FactOR));
    this->isActionLM = ((t == ActionAND) || (t == ActionOR));
    this->lm.insert(pNode);
}

Landmark::Landmark(LMType t) {
    this->isAND = ((t == ActionAND) || (t == FactAND));
    this->isOR = ((t == ActionOR) || (t == FactOR));
    this->isFactLM = ((t == FactAND) || (t == FactOR));
    this->isActionLM = ((t == ActionAND) || (t == ActionOR));
}

Landmark::Landmark(Landmark *that) {
    this->isFactLM = that->isFactLM;
    this->isActionLM = that->isActionLM;
    this->isAND = that->isAND;
    this->isOR = that->isOR;
    for (PINode* n : that->lm) {
        this->lm.insert(new PINode(n));
    }
}

Landmark::Landmark() {

}

void Landmark::makeDummy(string name) {
    this->name = name;
    this->isDummy = true;
}

PINode *Landmark::getFirst() {
    return (*this->lm.begin());
}
