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

void LandmarkGraph::showDot(Domain domain, bool openXDot) {
    ofstream dotfile;
    dotfile.open ("graph.dot");
    dotfile << "digraph {\n";

    for (auto n: this->N) {
        dotfile << "   n" << n->nodeID << " [";
        if (n->isInS0) {
            dotfile << "color=blue ";
        }
        dotfile << "label=\"" << n->nodeID << ":";
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
    if (openXDot) {
        system("xdot graph.dot");
    }

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

void LandmarkGraph::prune(int pruneblanks, set<int> &invariants) {
    set<int> deletedNodes;
    vector<Landmark*> needToDelete;
    for (auto n: N) {
        if ((n->isFactLM) && (n->lm.size() == 1) && (invariants.find((*n->lm.begin())->schemaIndex) != invariants.end())) {
            deletedNodes.insert(n->nodeID);
            needToDelete.push_back(n);
        } else if (n->lm.size() == 1) {
            int count = 0;
            for (int arg: (*n->lm.begin())->consts) {
                if (arg >= 0) {
                    count++;
                }
            }
            if (count <= pruneblanks) {
                deletedNodes.insert(n->nodeID);
                needToDelete.push_back(n);
            }
        }
    }
    for(auto n: needToDelete) {
        N.erase(n);
    }

    for (auto iter: this->successors) {
        for (int d: deletedNodes) {
            iter.second.erase(d);
        }
    }
    for (int d: deletedNodes) {
        this->successors.erase(d);
    }

    for (auto iter: this->predecessors) {
        for (int d: deletedNodes) {
            iter.second.erase(d);
        }
    }
    for (int d: deletedNodes) {
        this->predecessors.erase(d);
    }
}

void LandmarkGraph::writeToFile(string filename, Domain domain) {
    ofstream dotfile;
    dotfile.open (filename);
    dotfile << ";; Format per line:\n";
    dotfile << ";; 1. int: 0 = fact lm\n";
    dotfile << ";;         1 = action lm\n";
    dotfile << ";; 2. int: 0 = connector \"and\"\n";
    dotfile << ";;         1 = connector \"or\"\n";
    dotfile << ";; 3. int: 1 = number of elements that are connected with connector given in 2\n";
    dotfile << ";; 4. int indicating whether the LM is contained in an ordering relation or not (0 if not contained, 1 if contained)\n";
    dotfile << "LMs\n";
    dotfile << this->N.size() << "\n";

    map<int, int> nodeID2line;
    int line = 0;

    for (auto n: this->N) {
        if (n->isDummy) continue;

        nodeID2line[n->nodeID] = line++;

        if (n->isFactLM) {
            dotfile << "0 ";
        } else {
            dotfile << "1 ";
        }
        if (n->isAND) {
            dotfile << "0 ";
        } else {
            dotfile << "1 ";
        }
        dotfile << n->lm.size() << " ";
        if ((successors.find(n->nodeID) != successors.end()) || (predecessors.find(n->nodeID) != predecessors.end())) {
            dotfile << "1 ";
        } else {
            dotfile << "0 ";
        }
        for (auto m: n->lm) {
            if (n->isFactLM) {
                dotfile << "(" << domain.predicates[m->schemaIndex].name;
                for (int k = 0; k < m->consts.size(); k++) {
                    int obj = m->consts[k];
                    if(obj < 0) {
                        dotfile << " ?";
                    } else {
                        dotfile << " " << domain.constants[obj];
                    }
                }
                dotfile << ")";
            } else {
                dotfile << "(" << domain.tasks[m->schemaIndex].name;
                for (int k = 0; k < m->consts.size(); k++) {
                    int obj = m->consts[k];
                    if(obj < 0) {
                        dotfile << " ?";
                    } else {
                        dotfile << " " << domain.constants[obj];
                    }
                }
                dotfile << ")";
            }
        }
        dotfile << "\n";
    }
    dotfile << ";; Orderings encoded as follows:\n";
    dotfile << ";; 1. int x: node in line x given above (0-based index)\n";
    dotfile << ";; 2. int y: node in line y given above (0-based index)\n";
    dotfile << ";; 3. int indicates type:\n";
    dotfile << ";;    - 0 if greedy necessary ordering\n";
    dotfile << ";;    - 1 if reasonable ordering\n";
    dotfile << "LM Orderings\n";
    for (auto iter: this->successors) {
        int from = iter.first;
        for (auto iter2: iter.second) {
            int to = iter2.first;
            for (auto ord: iter2.second) {
                dotfile << nodeID2line[from] << " " << nodeID2line[to] << " " << ord << "\n";
            }
        }
    }

    dotfile.close();
}
