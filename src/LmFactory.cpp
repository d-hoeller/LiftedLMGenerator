//
// Created by dh on 02.12.21.
//

#include "LmFactory.h"
#include "PINode.h"

LmFactory::LmFactory(Domain d, Problem p) {
    this->domain = d;
    this->problem = p;

    // generate achiever map
    achiever = new vector<pair<int, int>>[domain.predicates.size()];
    for (int iA = 0; iA < domain.tasks.size(); iA++) {
        auto task = domain.tasks[iA];
        for (int iAdd = 0; iAdd < task.effectsAdd.size(); iAdd++) {
            const int p = task.effectsAdd[iAdd].predicateNo;
            achiever[p].push_back(make_pair(iA, iAdd));
        }
    }
}

bool LmFactory::containedInS0(PINode *pNode) { // todo: this must be made more efficient!
    for (Fact f : problem.init) {
        if (f.predicateNo != pNode->schemaIndex) continue;
        if (f.arguments.size() != pNode->consts.size()) continue;
        bool compatible = true;
        for (int i = 0; i < pNode->consts.size(); i++) {
            int c = pNode->consts[i];
            if (c < 0) continue; // text next consts
            if (f.arguments[i] != c) {
                compatible = false;
                break;
            }
        }
        if (compatible) {
            return true;
        }
    }
    return false;
}

PINode *LmFactory::getPINode(Fact &f) {
    PINode* res = new PINode();
    res->schemaIndex = f.predicateNo;
    for (int obj: f.arguments) {
        res->consts.push_back(obj);
    }
    return res;
}
