//
// Created by dh on 12.11.21.
//

#include "DTGnode.h"
#include "model.h"
#include <cassert>
#include <iostream>

DTGnode::DTGnode(DTGnode *that) {
    this->schemaIndex =  that->schemaIndex;
    for (int i : that->consts) {
        this->consts.push_back(i);
    }
}

DTGnode::DTGnode() {

}

bool DTGnode::abstractionOf(DTGnode *that) {
    if (this->schemaIndex != that->schemaIndex) {
        return false;
    }
    assert(this->consts.size() == that->consts.size());
    for (int i = 0; i < this->consts.size(); i++) {
        if (this->consts[i] != -1) {
            if (that->consts[i] != this->consts[i]) {
                return false;
            }
        }
    }
    return true;
}

void DTGnode::printFact(Domain &domain) {
    cout << "(" << domain.predicates[this->schemaIndex].name;
    for (int k = 0; k < this->consts.size(); k++) {
        int obj = this->consts[k];
        if(obj < 0) {
            cout << " ?";
        } else {
            cout << " " << domain.constants[obj];
        }
    }
    cout << ")";
}
