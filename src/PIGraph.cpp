//
// Created by dh on 12.11.21.
//

#include "PIGraph.h"
#include "model.h"
#include <cassert>
#include <fstream>
#include <bits/stdc++.h>

void PIGraph::addNode(PINode *pGnode) {
    pGnode->nodeID = this->nodeID++;
    N.insert(pGnode);
//    iToN[pGnode->id] = pGnode;
}

void PIGraph::addArc(int from, int to, PINode *partInstantiation) {
    assert(from >= 0);
    assert(to >= 0);
    PIArc *arc = new PIArc();
    arc->arcid = this->arcID++;
    arc->partInstantiation = partInstantiation;
    successors[from][to].insert(arc);
    predecessors[to][from].insert(arc);
}

typedef pair<int, int> pi;
bool PIGraph::reachable(set<int> &from, set<int> &to) {
    unordered_map<int, int> dist;
    unordered_map<int, PIArc*> via;
    priority_queue<pi, vector<pi>, greater<pi> > Q;

    for (int n : from) {
        dist[n] = 0;
        Q.push(make_pair(0, n));
    }
    while (!Q.empty()) {
        pair<int, int> top = Q.top();
        Q.pop();
        const int n = top.second;
        const int d = top.first;
        if (dist[n] < d) {
            continue;
        }
        for (auto arcs: successors[n]) {
            const int to = arcs.first;
            int minCosts = INT_MAX;
            PIArc* localvia;
            for (auto arc: arcs.second) {
                if (deactivatedArcs.find(arc->arcid) == deactivatedArcs.end()) {
                    if ((d + arc->arccosts) < minCosts) {
                        minCosts = d + arc->arccosts;
                        localvia = arc;
                    }
                }
            }
            if ((dist.find(to) == dist.end()) || (minCosts < dist[to])) {
                dist[to] = minCosts;
                via[to] = localvia;
                if (deactivatedNodes.find(to) == deactivatedNodes.end()) {
                    Q.push(make_pair(minCosts, to));
                }
            }
        }
    }
    for (int n: to) {
        auto iter = dist.find(n);
        if ((iter != dist.end()) && (dist[n] < INT_MAX)) {
            return true;
        }
    }
    return false;
}

void PIGraph::showDot(Domain domain) {
    ofstream dotfile;
    dotfile.open ("graph.dot");
    dotfile << "digraph {\n";

    for (PINode *n: this->N) {
        dotfile << "   n" << n->nodeID << " [label=\"(" << n->nodeID << ": " << domain.predicates[n->schemaIndex].name;
        for (int k = 0; k < n->consts.size(); k++) {
            int obj = n->consts[k];
            if (obj < 0) {
                dotfile << " ?";
            } else {
                dotfile << " " << domain.constants[obj];
            }
        }
        dotfile << ")\"]\n";
    }

    for (auto iter: this->successors) {
        int from = iter.first;
        for (auto iter2: iter.second) {
            int to = iter2.first;
            for (auto arc: iter2.second) {
                dotfile << "   n" << from << " -> n" << to << " [label=\"";
                dotfile << "(" << domain.tasks[arc->partInstantiation->schemaIndex].name;
                for (int i = 0; i < arc->partInstantiation->consts.size(); i++) {
                    int obj = arc->partInstantiation->consts[i];
                    if(obj == -1) {
                        dotfile << " ?";
                    } else {
                        dotfile << " " << domain.constants[obj];
                    }
                }
                dotfile << ")\"]\n";
            }
        }
    }
    dotfile << "}\n";
    dotfile.close();
    system("xdot graph.dot");
}
