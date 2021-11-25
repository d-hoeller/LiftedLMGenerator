//
// Created by dh on 22.11.21.
//

#include "LandmarkGraph.h"
#include "model.h"
#include <fstream>
#include <bits/stdc++.h>

Landmark *LandmarkGraph::getNode(int i) {
    for (auto n: N) {
        if (n->nodeID == i) {
            return n;
        }
    }
    return nullptr;
}

void LandmarkGraph::addArc(int from, int to, int type) {
    assert(from >= 0);
    assert(to >= 0);
//    cout << "insert " << from << " " << to << endl;
    successors[from][to].insert(type);
    predecessors[to][from].insert(type);
}

pair<int, int> LandmarkGraph::addNode(Landmark *pLandmark) {
    auto iter = this->N.find(pLandmark);
    if (iter == this->N.end()) {
        pLandmark->nodeID = this->nodeID++;
        this->N.insert(pLandmark);
        return make_pair(pLandmark->nodeID, 1);
    } else {
        return make_pair((*iter)->nodeID, 0);
    }
}

void LandmarkGraph::showDot(Domain domain) {
    ofstream dotfile;
    dotfile.open ("graph.dot");
    dotfile << "digraph {\n";

    for (auto n: this->N) {
        dotfile << "   n" << n->nodeID << " [label=\"" << n->nodeID << ":";
        if (!n->isDummy) {
            if (n->lm.size() > 1) {
                if (n->isOR) {
                    dotfile << "OR{";
                } else {
                    dotfile << "AND{";
                }
            }
            bool first = true;
            int linebreak = round(sqrt(n->lm.size()));
            int i = 0;
            for (auto m: n->lm) {
                if ((n->lm.size() > 1) && (((i++) % linebreak) == 0)) {
                    dotfile << "\\n";
                }
                if (first) {
                    first = false;
                } else {
                    dotfile << " ";
                }
                dotfile << "(";
                if (n->isFactLM) {
                    dotfile << domain.predicates[m->schemaIndex].name;
                } else {
                    dotfile << domain.tasks[m->schemaIndex].name;
                }
                for (int k = 0; k < m->consts.size(); k++) {
                    int obj = m->consts[k];
                    if (obj < 0) {
                        dotfile << " ?";
                    } else {
                        dotfile << " " << domain.constants[obj];
                    }
                }
                dotfile << ")";
            }
            if (n->lm.size() > 1) {
                dotfile << "}";
            }
        } else {
            dotfile << n->name;
        }
        dotfile << "\"]\n";
    }

    for (auto iter: this->successors) {
        int from = iter.first;
        for (auto iter2: iter.second) {
            int to = iter2.first;
            for (auto ord: iter2.second) {
                dotfile << "   n" << from << " -> n" << to << " [label=\"";
                dotfile << ord;
//                for (int i = 0; i < arc->ArcLabel->consts.size(); i++) {
//                    int obj = arc->ArcLabel->consts[i];
//                    if(obj == -1) {
//                        dotfile << " ?";
//                    } else {
//                        dotfile << " " << domain.constants[obj];
//                    }
//                }
                dotfile << "\"]\n";
            }
        }
    }
    dotfile << "}\n";
    dotfile.close();
    system("xdot graph.dot");

}

vector<int>* LandmarkGraph::merge(int nodeID, LandmarkGraph *subGraph, int OrderingType) {
    vector<int>* newIDs = new vector<int>();
    map<int, int> idMapping;
    set<int> lastNodes;
    for (auto n: subGraph->N) {
        Landmark* newLM = new Landmark(n);
        pair<int, int> res = this->addNode(newLM);
        int id = res.first;
        bool isNew = (res.second != 0);
        idMapping[n->nodeID] = id;
        if (isNew) {
            newIDs->push_back(id);
        }
        lastNodes.insert(n->nodeID);
    }
    for (auto iter: subGraph->successors) {
        int from = iter.first;
        for (auto iter2: iter.second) {
            int to = iter2.first;
            for (auto ord: iter2.second) {
                lastNodes.erase(from);
                this->addArc(idMapping[from], idMapping[to], ord);
            }
        }
    }
    for (int i : lastNodes) {
        this->addArc(idMapping[i], nodeID, OrderingType);
    }
    return newIDs;
}
