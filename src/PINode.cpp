//
// Created by dh on 12.11.21.
//

#include "PINode.h"
#include "model.h"
#include <cassert>
#include <iostream>

PINode::PINode(PINode *that) {
    this->schemaIndex =  that->schemaIndex;
    for (int i : that->consts) {
        this->consts.push_back(i);
    }
}

PINode::PINode() {

}

bool PINode::abstractionNotEqOf(PINode *that) {
    bool equal = true;
    for (int i = 0; i < this->consts.size(); i++) {
        if (that->consts[i] != this->consts[i]) {
            equal = false;
            break;
        }
    }
    if (equal) {
        return false; // no generalization
    } else {
        return abstractionOf(that);
    }
}

bool PINode::abstractionOf(PINode *that) {
    if (this->schemaIndex != that->schemaIndex) {
        return false;
    }
    assert(this->consts.size() == that->consts.size());
    for (int i = 0; i < this->consts.size(); i++) {
        if (this->consts[i] >= 0) {
            if (that->consts[i] != this->consts[i]) {
                return false;
            }
        }
    }
    return true;
}

void PINode::printFact(Domain &domain, ostream& stream) {
    stream << "(" << domain.predicates[this->schemaIndex].name;
    for (int k = 0; k < this->consts.size(); k++) {
        int obj = this->consts[k];
        if(obj < 0) {
            stream << " ?" << (-1 * obj);
        } else {
            stream << " " << domain.constants[obj];
        }
    }
    stream << ")";
}

void PINode::printAction(Domain &domain) {
    cout << "(" << domain.tasks[this->schemaIndex].name;
    for (int k = 0; k < this->consts.size(); k++) {
        int obj = this->consts[k];
        if(obj < 0) {
            cout << " ?" << (-1 * obj);
        } else {
            cout << " " << domain.constants[obj];
        }
    }
    cout << ")";
}
