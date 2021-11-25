//
// Created by dh on 22.11.21.
//

#ifndef SRC_LANDMARKGRAPH_H
#define SRC_LANDMARKGRAPH_H


#include "Landmark.h"
#include <iostream>
#include <unordered_set>
#include <unordered_map>

struct LMHasher {
    PINodeHasher hasher;
    size_t operator()(const Landmark * p) const {
        size_t res = hash<int>()(p->isActionLM);
        res = (res << 1);
        res = res ^ hash<int>()(p->isAND);
        for (auto i: p->lm) {
            int x = hasher(i);
            res = (res << 1);
            res = res ^ hash<int>()(x);
        }
//        std::cout << "HASH: " << res << endl;
        return res;
    }
};

struct LMComparator {
    PINodeComparator comp;
    bool operator()(const Landmark* obj1, const Landmark* obj2) const {
        if (obj1->isAND != obj2->isAND)
            return false;
        if (obj1->isFactLM != obj2->isFactLM)
            return false;
        if (obj1->lm.size() != obj2->lm.size())
            return false;

        auto iter1 = obj1->lm.begin();
        auto iter2 = obj2->lm.begin();

        while (iter1 != obj1->lm.end()) {
            PINode* n1 = *iter1;
            PINode* n2 = *iter2;
            if (!comp(n1,n2)) {
               return false;
            }
            iter1++;
            iter2++;
        }
        return true;
    }
};

class LandmarkGraph {
    int nodeID = 0;
    int arcID = 0;
public:
    unordered_set<Landmark*, LMHasher, LMComparator> N;
    unordered_map<int, unordered_map<int, unordered_set<int>>> successors;
    unordered_map<int, unordered_map<int, unordered_set<int>>> predecessors;
    pair<int, int> addNode(Landmark *lm);

    void showDot(Domain domain);

    Landmark *getNode(int i);

    void addArc(int fromID, int toID, int type);

    vector<int>* merge(int nodeID, LandmarkGraph *subGraph, int OrderingType);
};


#endif //SRC_LANDMARKGRAPH_H
