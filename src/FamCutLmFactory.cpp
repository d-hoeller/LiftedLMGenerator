//
// Created by dh on 12.12.20.
//

#include "FamCutLmFactory.h"
#include "LandmarkGraph.h"
#include "Landmark.h"
#include "NecSubLmFactory.h"
#include <iostream>
#include <map>
#include <cassert>

bool printIntermediateLMSets = false;
bool printDTGs = false;

FamCutLmFactory::FamCutLmFactory(Domain d, Problem p, vector<FAMGroup> fg) : LmFactory(d, p) {
    this->famGroups = fg;

    for (int j = 0; j < famGroups.size(); j++) {
        printFamGroup(j);
    }
    //
    // create inverse mappings
    //
//    for (int i = 0; i < domain.tasks.size(); i++) {
//        for (int j = 0; j < domain.tasks[i].effectsAdd.size(); j++) {
//            int p = domain.tasks[i].effectsAdd[j].predicateNo;
//            AddEff* ae = new AddEff;
//            ae->action = i;
//            ae->add = j;
//            this->addEff2Action[p].insert(ae);
//        }
//    }

    getFamModifiers(fg);
    printModifiers(fg);
//    exit(0);

/*
    ofstream dotfile;
    dotfile.open ("cg.dot");
    dotfile << "digraph {\n";
    for (int i = 0 ; i < famGroups.size(); i++) {
        dotfile << "   n" << i << " [label=\"FAM" << i << "\"]\n";;
    }

    set<int> effs[domain.tasks.size()];
    for (int iFAM = 0; iFAM < famGroups.size(); iFAM++) {
        for (auto m: modifier[iFAM]) {
            int a = m->action;
            effs[a].insert(iFAM);
        }
    }
    for (int iTask = 0; iTask < d.tasks.size(); iTask++) {
        auto task = d.tasks[iTask];
        for (int iPrec = 0; iPrec < task.preconditions.size(); iPrec++) {
            auto prec = task.preconditions[iPrec];
            for (int precFAM = 0; precFAM < famGroups.size(); precFAM++) {
                if (isCompatible(domain, iTask,prec,famGroups[precFAM])) {
                    for (int effFAM : effs[iTask]) {
                        dotfile << "   n" << precFAM << " -> n" << effFAM << " [label=\"";
                        dotfile << domain.tasks[iTask].name << "\"]\n";
                    }
                }
            }
        }
        for (int eff1FAM : effs[iTask]) {
            for (int eff2FAM : effs[iTask]) {
                if (eff1FAM != eff2FAM) {
                    dotfile << "   n" << eff1FAM << " -> n" << eff2FAM << " [label=\"";
                    dotfile << domain.tasks[iTask].name << "\"]\n";
                }
            }
        }

    }
    dotfile << "}\n";
    dotfile.close();
    //system("xdot cg.dot");
*/
    //
    // calculate static predicates
    //
    for (int i = 0; i < domain.predicates.size(); i++) { invariant.insert(i); }
    for (const auto &task: domain.tasks) {
        for (const auto &eAdd: task.effectsAdd) {
            invariant.erase(eAdd.predicateNo);
        }
        for (const auto &eDel: task.effectsDel) {
            invariant.erase(eDel.predicateNo);
        }
    }
    if (!invariant.empty()) {
        cout << "The following predicates are static:" << endl;
        for (int pred: invariant) {
            cout << "- " << domain.predicates[pred].name << endl;
        }

        // add indices of static precs to relevant actions, these will be checked when constructing the graph
        for (int i = 0; i < fg.size(); i++) {
            for (int j = 0; j < modifier[i].size(); j++) {
                int a = modifier[i][j]->action;
                for (int k = 0; k < domain.tasks[a].preconditions.size(); k++) {
                    int p = domain.tasks[a].preconditions[k].predicateNo;
                    if (invariant.find(p) != invariant.end()) {
                        modifier[i][j]->staticPrecs.push_back(k);
                    }
                }
            }
        }
    } else {
        cout << "No static predicates detected." << endl;
    }
}

void FamCutLmFactory::getFamModifiers(vector<FAMGroup> &fg) {
    this->modifier = new vector<FAMmodifier *>[fg.size()];
    for (int iFAM = 0; iFAM < fg.size(); iFAM++) {
        cout << "- FAM " << iFAM << endl;
        for (int iTask = 0; iTask < domain.tasks.size(); iTask++) {
            auto task = domain.tasks[iTask];
            cout << "  - Action " << iTask << " " << task.name << endl;
            for (int iPrec = 0; iPrec < task.preconditions.size(); iPrec++) {
                cout << "    - Prec " << iPrec << " " << domain.predicates[task.preconditions[iPrec].predicateNo].name << endl;
                for (int famLit = 0; famLit < fg[iFAM].literals.size(); famLit++) {
                    if (isCompatible(domain, iTask, task.preconditions[iPrec], fg[iFAM], famLit)) {
                        cout << "      - compatible with lit " << famLit << endl;
                        FAMmodifier *mod = new FAMmodifier();
                        auto prec = task.preconditions[iPrec];
                        for (int i = 0; i < prec.arguments.size(); i++) {
                            if (!famGroups[iFAM].literals[famLit].isConstant[i]) {
                                const int iFamVar = famGroups[iFAM].literals[famLit].args[i];
                                if (!famGroups[iFAM].vars[iFamVar].isCounted) { // this is a free variable
                                    mod->freeActionVars.insert(prec.arguments[i]);
                                }
                            } // todo: do I have to do something here?
                        }

                        int delEffect = -1;
                        for (int i = 0; i < task.effectsDel.size(); i++) {
                            auto del = task.effectsDel[i];
                            if (del.predicateNo != prec.predicateNo) continue;
                            bool identical = true;
                            for (int ip = 0; ip < del.arguments.size(); ip++) {
                                if (prec.arguments[ip] != del.arguments[ip]) {
                                    identical = false;
                                    break;
                                }
                            }
                            if (identical) {
                                delEffect = i;
                                break;
                            }
                        }
                        if (delEffect < 0) {
                            cout << "      - no del found" << endl;
                            continue;
                        }

                        vector<pair<int,int>> adds;
                        for (int i = 0; i < task.effectsAdd.size(); i++) {
                            auto add = task.effectsAdd[i];
                            for (int addLit = 0; addLit < fg[iFAM].literals.size(); addLit++) {
                                if (!isCompatible(domain, iTask, add, fg[iFAM], addLit)) {
                                    continue;
                                }
                                cout << "      - add " << i << " compatible." << endl;
                                // check for free variables
                                bool isIncompatible = false;
                                for (int j = 0; j < fg[iFAM].literals[addLit].args.size(); j++) {
                                    if (fg[iFAM].literals[addLit].isConstant[j]) {
                                        continue;
                                    }
                                    const int famVarIndex = fg[iFAM].literals[addLit].args[j];
                                    auto famVar = fg[iFAM].vars[famVarIndex];
                                    const bool actionVarIsCounted = (mod->freeActionVars.find(add.arguments[j]) == mod->freeActionVars.end());
                                    if (famVar.isCounted != actionVarIsCounted) {
                                        cout << "      - add " << i << " discarded due to counted vars." << endl;
                                        cout << "        - var " << j;
                                        if (famVar.isCounted) {
                                            cout << " is counted";
                                        } else {
                                            cout << " is not counted";
                                        }
                                        cout << endl;
                                        isIncompatible = true;
                                        break;
                                    }
                                }
                                if (!isIncompatible) {
                                   adds.push_back(make_pair(i, addLit));
                                }
                            }
                        }
                        cout << "      - adds found: " << adds.size() << endl;
                        if (adds.size() == 1) {
                            mod->action = iTask;
                            mod->prec = iPrec;
                            mod->precFamLit = famLit;
                            mod->addFamLit = adds[0].second;
                            mod->add = adds[0].first;
                            modifier[iFAM].push_back(mod);
                        } else if (adds.size() > 1) {
                            cout << "ERROR: Relevant sizes do not match." << endl;
                            exit(-1);
                        }
                    }
                }
            }
        }
    }
}


void FamCutLmFactory::printModifiers(vector<FAMGroup> &fg) {
    for (int iFAM = 0; iFAM < fg.size(); iFAM++) {
        printFamGroup(iFAM);
        cout << "Modifiers:" << endl;
        for (auto m : modifier[iFAM]) {
            auto task = domain.tasks[m->action];
            cout << "- " << task.name << endl;
            cout << "  action's free vars:";
            for (int var : m->freeActionVars) {
                cout << " v" << var;
            }
            cout << endl;
            cout << "  FAM literals " << m->precFamLit << " -> " << m->addFamLit << endl;
            auto prec = task.preconditions[m->prec];
            cout << "  from " << m->prec << ":(" << domain.predicates[prec.predicateNo].name;
            for (int k = 0; k < prec.arguments.size(); k++) {
                cout << " v" << prec.arguments[k];
            }
            cout << ")" << endl;

            auto add = task.effectsAdd[m->add];
            cout << "    to " << m->prec << ":(" << domain.predicates[add.predicateNo].name;
            for (int k = 0; k < add.arguments.size(); k++) {
                cout << " v" << add.arguments[k];
            }
            cout << ")" << endl;
        }
    }
}

bool FamCutLmFactory::isCompatible(Domain &d, int iT, PredicateWithArguments &lit, FAMGroup &fg) {
    for (int iLit = 0; iLit < fg.literals.size(); iLit++) {
        if (isCompatible(d, iT, lit, fg, iLit)) return true;
    }
    return false;
}

// a package might be "AT a position" or "IN a truck" -> these are the literals
bool FamCutLmFactory::isCompatible(Domain &d, int iT, PredicateWithArguments &actionLit, FAMGroup &fg, int iLit) {
    Task t = domain.tasks[iT];
    int p = actionLit.predicateNo;
    if (fg.literals[iLit].predicateNo == p) {
        bool match = true;
        for (int iArg = 0; iArg < fg.literals[iLit].args.size(); iArg++) { // loop params
            int iVar = actionLit.arguments[iArg];
            int sPrec = t.variableSorts[iVar];
            if (!fg.literals[iLit].isConstant[iArg]) {
                int sFG = fg.vars[fg.literals[iLit].args[iArg]].sort;
                // check variable types
                if (sFG == sPrec) {
                    continue; // check next variable
                } else {
                    bool intersects = false;
                    for (int c: domain.sorts[sPrec].members) {
                        if (domain.sorts[sFG].members.find(c) != domain.sorts[sFG].members.end()) {
                            intersects = true;
                            break;
                        }
                    }
                    if (intersects) {
                        continue; // check next variable
                    }
                }
                match = false;
            } else { // argument from FAM group is const
                int c = fg.literals[iLit].args[iArg];
                if (domain.sorts[sPrec].members.find(c) != domain.sorts[sPrec].members.end()) {
                    continue;
                }
                match = false;
            }
        }
        if (match) { return true; }
    }
    return false;
}

void FamCutLmFactory::generateLMs() {
    // initialize graph with goal nodes and start generation
    LandmarkGraph* lmg = new LandmarkGraph();
    Landmark *dummyGoal = new Landmark();
    lmg->addNode(dummyGoal);
    dummyGoal->makeDummy("g");
    vector<int> todo;
    for (auto g: problem.goal) {
        Landmark* lmn = new Landmark(FactAND);
        PINode* gNode = new PINode();
        gNode->schemaIndex = g.predicateNo;
        for (int i = 0; i < g.arguments.size(); i++) {
            gNode->consts.push_back(g.arguments[i]);
        }
        lmn->lm.insert(gNode);
        lmg->addNode(lmn);
        lmg->addArc(lmn->nodeID, dummyGoal->nodeID, 0); // todo: ordering type must be directly before
        todo.push_back(lmn->nodeID);
    }
    for (int id: todo) {
        lmDispatcher(lmg, id);
    }
//    lmg->showDot(domain, true);
    cout << "- [numLMs=" << (lmg->N.size() - 1) << "]\n";
    lmg->prune(0, invariant);
    lmg->showDot(domain, false);
    cout << "- [numPrunedLMs=" << (lmg->N.size() - 1) << "]\n";
    lmg->writeToFile("LMs.txt", domain);
}

int FamCutLmFactory::getFAMMatch(PINode* node) {
    const int p = node->schemaIndex;
    for (int ifg = 0; ifg < famGroups.size(); ifg++) {
        FAMGroup fg = famGroups[ifg];
        // a package might be "AT a position" or "IN a truck" -> these are the literals
        for (int iLit = 0; iLit < fg.literals.size(); iLit++) {
            if (fg.literals[iLit].predicateNo == p) {
                bool match = true;
                for (int iArg = 0; iArg < fg.literals[iLit].args.size(); iArg++) { // loop params
                    if (fg.literals[iLit].isConstant[iArg]) {
                        // check for non-matching constants
                        if (fg.literals[iLit].args[iArg] != node->consts[iArg]) {
                            match = false;
                            break;
                        }
                    } else {
                        // check variable types
                        int s = fg.vars[fg.literals[iLit].args[iArg]].sort;
                        int c = node->consts[iArg];
                        if ((c >= 0) && (domain.sorts[s].members.find(c) == domain.sorts[s].members.end())) {
                            match = false;
                            break;
                        }
                    }
                }
                if (match) {
                    return ifg;
                }
            }
        }
    }
    return -1;
}

void FamCutLmFactory::lmDispatcher(LandmarkGraph* lmg, int nodeID) {
    if (printIntermediateLMSets) {
        lmg->showDot(domain, true);
    }

    auto node = lmg->getNode(nodeID);
    if ((node->isAND) || (node->lm.size() == 1)) {
        if (node->isFactLM) {
            PINode* n = *node->lm.begin();
            if (containedInS0(n)) {
                node->isInS0 = true;
                return;
            }
            for (auto n: node->lm) {
                const int iFamGroup = getFAMMatch(n);
                LandmarkGraph *subgraph;
                if (iFamGroup >= 0) {
                    subgraph = generateLMs(n, iFamGroup);
                } else { // this is a LM not contained in a FAM-Group
                    //subgraph = generateAchieverNodes(n);
                    cout << "- LM not contained in a FAM-Group, generating action nodes" << endl;
                    subgraph = generateActionNodes(n);
                }
//                lmg->showDot(domain, true);
                vector<int> *newIDs = lmg->merge(node->nodeID, subgraph, 0); // todo: which type?
//                lmg->showDot(domain, true);
//                exit(-1);
                for (int n: *newIDs) {
                    lmDispatcher(lmg, n);
                }
//                delete newIDs;
            }
        } else { // this is an action landmark
            LandmarkGraph *subgraph = generatePrecNodes(*node->lm.begin());
            //subgraph->showDot(domain, true);
//            lmg->showDot(domain, true);
            vector<int> *newIDs = lmg->merge(node->nodeID, subgraph, 0); // todo: which type?
//            lmg->showDot(domain, true);
            for (int n : *newIDs) {
                lmDispatcher(lmg, n);
            }
//            cout << "What to do?" << endl;
//            exit(13);
        }
    } else { // is an OR landmark
//        lmg->showDot(domain);
        if (node->isFactLM) {
           cerr << "WARNING: Unimplemented feature \"fact or landmark\": " << endl;
           for (auto n: node->lm) {
               n->printFact(domain, cerr);
               cerr << endl;
           }
            cerr << "Generation is continued, but no further LMs are generated based on this one." << endl;
        } else {
            LandmarkGraph *subgraph = generatePrecIntersection(node);
            vector<int> *newIDs = lmg->merge(node->nodeID, subgraph, 0); // todo: which type?
            for (int n : *newIDs) {
                lmDispatcher(lmg, n);
            }
        }
//        exit(12);
    }
}

LandmarkGraph *FamCutLmFactory::generatePrecNodes(PINode *actionNode) {
    cout << "- generating precondition nodes for action LM \"";
    actionNode->printAction(domain);
    cout << "\"" << endl;

    LandmarkGraph *res = new LandmarkGraph();
    PINode* piAction = actionNode;
    auto action = domain.tasks[piAction->schemaIndex];
    for (auto prec: action.preconditions) {
        PINode* precNode = new PINode();
//        cout << domain.predicates[prec.predicateNo].name << endl;
        precNode->schemaIndex = prec.predicateNo;
        for (int i = 0; i < prec.arguments.size(); i++) {
            const int var = prec.arguments[i];
            precNode->consts.push_back(piAction->consts[var]);
        }
        res->addNode(new Landmark(precNode, FactAND));
    }
//    cout << "Prec-Size: " << action.preconditions.size() << " : " << res->N.size() << endl;
//    res->showDot(domain);
    return res;
}


LandmarkGraph *FamCutLmFactory::generateLMs(PINode* node, int iFamGroup) {
    cout << "- generating DTG for LM \"";
    node->printFact(domain, cout);
    cout << "\"" << endl;

    FAMGroup fam = famGroups[iFamGroup];

    // need to store the free variables set by the goal fact
    vector<int> setFreeVars;
    for (int i = 0; i < fam.vars.size(); i++) setFreeVars.push_back(-1); // todo: replace -1
    for (int iLit = 0; iLit < fam.literals.size(); iLit++) {
        if (fam.literals[iLit].predicateNo == node->schemaIndex) {
            for (int iLitArg = 0; iLitArg < fam.literals[iLit].args.size(); iLitArg++) {
                int iFamArg = fam.literals[iLit].args[iLitArg];
                if (!fam.vars[iFamArg].isCounted) {
                    setFreeVars[iFamArg] = node->consts[iLitArg];
                }
            }
        }
    }
//    cout << "Have to check:";
//    for(int i = 0; i <  setFreeVars.size(); i++) {
//        int x = setFreeVars[i];
//        if(x >= 0)
//        cout << " " << i << "=" << domain.constants[x];
//    }
//    cout << endl;


    // do two things:
    // - convert initial state to a data structure supporting a contains test
    // - get starting point of that particular "free variable"

    PINode *s0Node = getInitNode(fam, setFreeVars);
    if (s0Node == nullptr) {
        return new LandmarkGraph();
    }

//    unordered_set<PINode *, PINodeHasher, PINodeComparator> N;
    PIGraph graph;
    auto *N_last = new vector<PINode *>;
    auto *N_this = new vector<PINode *>;

    graph.addNode(s0Node);
    N_last->push_back(s0Node);

    bool groundFAMVars = false;
    int varID = -1;
    while (!N_last->empty()) {
        for (auto n: *N_last) {
            cout << "  - generating successors for \"";
            n->printFact(domain, cout);
            cout << "\"" << endl;

            // need actions where
            // - precondition is n
            // - static preconditions hold
            for (auto arc: modifier[iFamGroup]) {
                int a = arc->action;
                auto prec = domain.tasks[a].preconditions[arc->prec];
                if ((prec.predicateNo == n->schemaIndex) && (isCompatible(domain, a, prec, fam, arc->precFamLit))) {
                    int numVars = domain.tasks[a].variableSorts.size();
                    PINode *partInstAction = new PINode;
                    for (int i = 0; i < numVars; i++) {
                        partInstAction->consts.push_back(varID--); // todo: here is a -1
                    }
                    partInstAction->schemaIndex = a;
                    // determine bindings by precondition belonging to FAM group
                    for (int iPrec = 0; iPrec < prec.arguments.size(); iPrec++) {
                        const int var = prec.arguments[iPrec];
                        const int val = n->consts[iPrec];
                        partInstAction->consts[var] = val;
                    }
                    cout << "    - arc introduced by action \"";
                    partInstAction->printAction(domain);
                    cout << "\"" << endl;
                    // determine bindings by static precondition
                    bool incompatible = false;
                    for (int inv: arc->staticPrecs) {
                        PINode *partInstPrec = new PINode;
                        auto precSchema = domain.tasks[a].preconditions[inv];
                        partInstPrec->schemaIndex = precSchema.predicateNo;
                        for (int i = 0; i < precSchema.arguments.size(); i++) {
                            int var = precSchema.arguments[i];
                            partInstPrec->consts.push_back(partInstAction->consts[var]);
                        }
                        cout << "      - analyzing static prec \"";
                        partInstPrec->printFact(domain, cout);
                        cout << "\": ";

                        vector<Fact>* s0d = gets0Def(partInstPrec);
                        if (s0d->empty()) {
                            cout << "unfulfilled -> no new arc." << endl;
                            incompatible = true;
                            break;
                        } else if (s0d->size() == 1) {
                            for (int l = 0; l < s0d->size(); l++) {
                                const int obj = s0d->at(0).arguments[l];
                                const int var = precSchema.arguments[l];
                                if (partInstAction->consts[var] < 0) {
                                    cout << "setting param " << var << " to \"" << domain.constants[obj] << "\" -> \"";
                                    partInstAction->consts[var] = obj;
                                    partInstAction->printAction(domain);
                                    cout << "\"." << endl;
                                } else if (partInstAction->consts[var] != obj) {
                                    cout << "unfulfilled -> no new arc" << endl;
                                    incompatible = true;
                                    break;
                                } else {
                                    cout << "fine." << endl;
                                }
                            }
                        } else {
                            cout << "found " << s0d->size() << " compatible atoms in s0." << endl;
                        }
//                        StaticS0Def* s0Def = getStaticS0Def(precSchema.predicateNo);
//                        sortS0Def(partInstPrec, s0Def);
//                        for ()
                    }
                    if (incompatible) {
                        continue;
                    }

                    // now we have a partially instantiated action, we need to get the effect

                    vector<PINode *> *actionsToGround = new vector<PINode *>;
                    actionsToGround->push_back(partInstAction);

                    if (groundFAMVars) {
                        set<int> containedInEffect;
                        auto eff = domain.tasks[a].effectsAdd[arc->add];
                        for (int l = 0; l < eff.arguments.size(); l++) {
                            containedInEffect.insert(eff.arguments[l]);
                        }
                        vector<int> varsToGround;
                        for (int i = 0; i < partInstAction->consts.size(); i++) {
                            if ((partInstAction->consts[i] < 0) &&
                                (containedInEffect.find(i) != containedInEffect.end())) {
                                varsToGround.push_back(i);
                            }
                        }

                        vector<PINode *> *actionsToGround2 = new vector<PINode *>;
                        for (int var: varsToGround) {
                            for (PINode *action: *actionsToGround) {
                                int sort = domain.tasks[a].variableSorts[var];
                                for (int obj: domain.sorts[sort].members) {
                                    PINode *copy = new PINode(action);
                                    copy->consts[var] = obj;
                                    actionsToGround2->push_back(copy);
                                }
                                //delete action;
                            }
                            actionsToGround->clear();
                            vector<PINode *> *temp = actionsToGround2;
                            actionsToGround2 = actionsToGround;
                            actionsToGround = temp;
                        }
                    }
                    for (PINode *action: *actionsToGround) {
                        PINode *partInstEffect = new PINode;
                        auto eff = domain.tasks[a].effectsAdd[arc->add];
                        partInstEffect->schemaIndex = eff.predicateNo;
                        for (int l = 0; l < eff.arguments.size(); l++) {
                            const int var = eff.arguments[l];
                            partInstEffect->consts.push_back(action->consts[var]);
                        }
                        auto iter = graph.N.find(partInstEffect);
                        int to;
                        if (iter == graph.N.end()) {
                            graph.addNode(partInstEffect);
                            to = partInstEffect->nodeID;
                            N_this->push_back(partInstEffect);

                            graph.addArc(n->nodeID, to, action);
                        } else {
                            to = (*iter)->nodeID;
                            graph.addArc(n->nodeID, to, action);

                            for (int i = 0; i < partInstEffect->consts.size(); i++) {
                                int from = partInstEffect->consts[i];
                                int to = (*iter)->consts[i];
                                if ((from < 0) && (from != to)) {
                                    cout << "      - need to adapt bindings" << endl << "        ";
                                    partInstEffect->printFact(domain, cout);
                                    cout << endl << "         and "<< endl << "        ";
                                    (*iter)->printFact(domain, cout);
                                    cout << endl;
                                    graph.replaceWildcard(from, to);
                                }
                            }
                        }

                        cout << "    - target node is \"";
                        partInstEffect->printFact(domain, cout);
                        cout << "\"" << endl;
                    }
                }
            }
        }

        // switch sets
        N_last->clear();
        auto *temp = N_last;
        N_last = N_this;
        N_this = temp;
    }
    if (printDTGs) {
        graph.showDot(domain);
    }
//    LandmarkGraph *gNew = generateNodeLMs(graph, node, s0Node->nodeID);
    LandmarkGraph *gNew = generateCutLMs(graph, node, s0Node->nodeID);
//    gNew->showDot(domain, true);
    return gNew;

//    if (this->nodeBasedLMs) {
//        generateNodeLMs(graph, node, s0Node->nodeID);
//    }
//    if (this->cutBasedLMs) {
//        generateCutLMs(graph, node, s0Node->nodeID);
//    }
}

LandmarkGraph* FamCutLmFactory::generateNodeLMs(PIGraph &dtg, PINode *targetNode, int initialNodeID) {
    cout << "- generating FAM node cut LMs" << endl;

    LandmarkGraph* result = new LandmarkGraph();
    set<int> goalNodes;
    for (PINode *n: dtg.N) {
        if (n->abstractionOf(targetNode)) {
            goalNodes.insert(n->nodeID);
            cout << "goal id " << n->nodeID << endl;
        }
    }
    cout << "Goal nodes: " << goalNodes.size() << endl;

    set<int> from;
    from.insert(initialNodeID);
    cout << "init id: " << initialNodeID << endl;
    dtg.deactivatedNodes.clear();
    if (dtg.reachable(from, goalNodes)) {
        cout << "reachable" << endl;
        for (auto n: dtg.N) {
            if ((from.find(n->nodeID) != from.end()) || (goalNodes.find(n->nodeID) != goalNodes.end())) {
                continue;
            }
            dtg.deactivatedNodes.insert(n->nodeID);
            if (!dtg.reachable(from, goalNodes)) {
                cout << "- ";
                n->printFact(domain, cout);
                cout << " is a LM" << endl;
                Landmark* newLMNode = new Landmark(n, FactAND); // need to copy, otherwise the id will be overwritten
                result->addNode(newLMNode);
            }
            dtg.deactivatedNodes.clear();
        }
    }
    return result;
}

LandmarkGraph* FamCutLmFactory::generateCutLMs(PIGraph &dtg, PINode *targetNode, int initialNodeID) {
    cout << "- generating FAM action cut LMs" << endl;

    LandmarkGraph* result = new LandmarkGraph();
    set<int> from;
    from.insert(initialNodeID);
    set<int> goalZone;
    set<int> newGoalZone;
    map<int, int> binding;
    cout << "  - searching for ";
    targetNode->printFact(domain, cout);
    cout << endl;
    for (PINode *n: dtg.N) {
        if (n->abstractionOf(targetNode)) {
            goalZone.insert(n->nodeID);
            cout << "    - found ";
            n->printFact(domain, cout);
            cout << endl;
            for (int i = 0; i < targetNode->consts.size(); i++) { // target node might be more specific
                if (targetNode->consts[i] != n->consts[i]) {
                    binding[n->consts[i]] = targetNode->consts[i];
                    if (targetNode->consts[i] >= 0) {
                        cout << "      - mapping ?" << (-1 * n->consts[i]) << " to " << domain.constants[targetNode->consts[i]] << endl;
                    } else {
                        cout << "      - mapping ?" << (-1 * n->consts[i]) << " to ?" << (-1 * targetNode->consts[i]) << endl;
                    }
                }
            }
        }
    }

    if (goalZone.size() == 0) {
        cerr << "Target node: ";
        targetNode->printFact(domain, cerr);
        cerr << endl;
//        dtg.showDot(domain);
        cerr << "ERROR: goal zone is empty" << endl;
        exit(-1);
    }
    bool goalReached = false;
    int lastCut = -1;
    while (!goalReached) {
        cout << "    - new cut, nodes in goal zone" << endl;
        Landmark* cut = new Landmark(ActionOR);
        //unordered_set<PINode*, PINodeHasher, PINodeComparator> cut;
        newGoalZone.insert(goalZone.begin(), goalZone.end());
        //cout << "- nodes in goal zone: " << goalZone.size() << endl;
        for (auto n: goalZone) {
            cout << "      - ";
            dtg.getNode(n)->printFact(domain, cout);
            auto temp = dtg.predecessors.find(n);
            cout << " incoming arcs: " << temp->second.size() << endl;
            if (temp != dtg.predecessors.end()) {
                unordered_map<int, unordered_set<PIArc*>> arcs = temp->second;
                for (auto arc : arcs) {
                    int predNode = arc.first;
                    if (goalZone.find(predNode) == goalZone.end()) {
                        newGoalZone.insert(predNode);
                        for (auto a : arc.second) {
                            auto temp = new PINode(a->ArcLabel);
                            for (int i = 0; i < temp->consts.size(); i++) {
                                if (binding.find(temp->consts[i]) != binding.end()) {
//                                    cout << " blub " << temp->consts[i] << " -> " << binding[temp->consts[i]] << endl;
                                    temp->consts[i] = binding[temp->consts[i]];
                                }
                            }
                            cut->lm.insert(temp);
                        }
                        if (from.find(predNode) != from.end()) {
                            goalReached = true;
                        }
                    }
                }
            }
        }
        assert(cut->lm.size() > 0);

        cout << "    - actions in cut" << endl;
        for (auto a: cut->lm) {
            cout << "      - ";
            a->printAction(domain);
            cout << endl;
        }

        if (cut->lm.size() > 1) {
            vector<PINode *> needToDelete;
            for (auto p1: cut->lm) {
                for (auto p2: cut->lm) {
                    if (p1->abstractionNotEqOf(p2)) {
                        needToDelete.push_back(p1);
                    }
                }
            }
            cout << "  - cut: ";
            for (PINode* n : cut->lm) {
                n->printAction(domain);
                cout << " ";
            }
            cout << endl;
            if (!needToDelete.empty()) {
                cout << "  - pruning cut, deleting ";
                for (PINode *n: needToDelete) {
                    cout << "\"";
                    n->printAction(domain);
                    cout << "\"";
                    cut->lm.erase(n);
                }
                cout << endl;
            }
        }

        result->addNode(cut);
        if (lastCut >= 0) {
            result->addArc(cut->nodeID, lastCut, 0); // todo: which type?
        }
        lastCut = cut->nodeID;
        auto temp = goalZone;
        goalZone = newGoalZone;
        newGoalZone = temp;
        newGoalZone.clear();
    }
//    result->showDot(domain, true);
    return result;
}

PINode * FamCutLmFactory::getInitNode(FAMGroup &fam, vector<int> &setFreeVars) {
    PIGraph *s0 = new PIGraph();
    vector<PINode *> sameFAMNodes;
    for (int i = 0; i < problem.init.size(); i++) {
        Fact f = problem.init[i];
        PINode *n = new PINode;
        n->schemaIndex = f.predicateNo;
        for (int j = 0; j < f.arguments.size(); j++) {
            n->consts.push_back(f.arguments[j]);
        }
        s0->N.insert(n);

        for (int iLit = 0; iLit < fam.literals.size(); iLit++) {
            if (fam.literals[iLit].predicateNo == n->schemaIndex) {
                bool sameFAM = true;
                for (int iLitArg = 0; iLitArg < fam.literals[iLit].args.size(); iLitArg++) {
                    int iFamArg = fam.literals[iLit].args[iLitArg];
                    if (!fam.vars[iFamArg].isCounted) {
                        // check equality of free vars
                        if (setFreeVars[iFamArg] != n->consts[iLitArg]) {
                            sameFAM = false;
                        }
                    }
                }
                if (sameFAM) {
                    cout << "  - starting value is \"";
                    n->printFact(domain, cout);
                    cout << "\"" << endl;
                    sameFAMNodes.push_back(n);
                }
            }
        }
    }
    if (sameFAMNodes.size() != 1) {
        cerr << "WARNING: No starting value found for FAM group:" << endl;
        cerr << "         Need to store type of wildcards to do better." << endl;
        printFamGroup(fam, cerr);
        cerr << "Generation is continued, but no further LMs are generated based on this one." << endl;
        return nullptr;
    }
    return sameFAMNodes[0];
}

void FamCutLmFactory::printFamGroup(int i) {
    cout << std::endl << "FAM group " << i << " { ";
    printFamGroup(famGroups[i], cout);
}

void FamCutLmFactory::printFamGroup(FAMGroup &fg, ostream& stream) {
    for (int j = 0; j < fg.literals.size(); j++) {
        FAMGroupLiteral famLits = fg.literals[j];
        stream << "(" << domain.predicates[famLits.predicateNo].name;
        for (int k = 0; k < famLits.args.size(); k++) {
            int index = famLits.args[k];
            stream << " ";
            if (famLits.isConstant[k]) {
                stream << domain.constants[index];
            } else {
                stream << "v" << index;
            }
        }
        cout << ") ";
    }
    stream << "}" << std::endl;
    stream << "counted vars: {";
    for (int j = 0; j < fg.vars.size(); j++) {
        FAMVariable v = fg.vars[j];
        if (v.isCounted)
            stream << " [v" << j << " - " << domain.sorts[v.sort].name << "]";
    }
    stream << " }" << std::endl;

    stream << "free vars:    {";
    for (int j = 0; j < fg.vars.size(); j++) {
        FAMVariable v = fg.vars[j];
        if (!v.isCounted) {
            stream << " [v" << j << " - " << domain.sorts[v.sort].name << "]";
        }
    }
    stream << " }" << std::endl;
}


bool FamCutLmFactory::isNormalArc(int action, int relPrec, int relDel) {
    myassert((action >= 0) && (action < domain.tasks.size()));
    myassert((relPrec >= 0) && (relPrec < domain.tasks[action].preconditions.size()));
    myassert((relDel >= 0) && (relDel < domain.tasks[action].effectsDel.size()));

    auto prec = domain.tasks[action].preconditions[relPrec];
    auto del = domain.tasks[action].effectsDel[relDel];
    if (prec.predicateNo != del.predicateNo) {
        return false;
    } else {
        for (int l = 0; l < prec.arguments.size(); l++) {
            if (prec.arguments[l] != del.arguments[l]) {
                return false;
            }
        }
    }
    return true;
}

StaticS0Def *FamCutLmFactory::getStaticS0Def(int predicateNo) {
    if (this->staticS0.find(predicateNo) == this->staticS0.end()) {
        StaticS0Def *def = new StaticS0Def;
        def->predicateNo = predicateNo;
        vector<int> s0Entries;
        for (int i = 0; i < problem.init.size(); i++) {
            if (problem.init[i].predicateNo == predicateNo) {
                s0Entries.push_back(i);
            }
        }
        def->numEntries = s0Entries.size();
        if (def->numEntries > 0) {
            def->sizeOfEntry = problem.init[0].arguments.size();
            def->entries = new int[def->numEntries * def->sizeOfEntry];
            int iTarget = 0;
            for (int iS0 : s0Entries) {
                for (int iArg = 0; iArg < def->sizeOfEntry; iArg++) {
                    def->set(iTarget, iArg, problem.init[iS0].arguments[iArg]);
                }
                iTarget++;
            }
        } else {
            def->sizeOfEntry = 0;
            def->entries = nullptr;
        }
        this->staticS0[predicateNo] = def;
    }
    return this->staticS0[predicateNo];
}

void FamCutLmFactory::sortS0Def(PINode *partInstPrec, StaticS0Def *s0) {
    vector<int>* sortBy = new vector<int>;
    for (int i = 0; i < partInstPrec->consts.size(); i++) {
        sortBy->push_back(i);
    }
    quicksort(s0, 0, s0->numEntries - 1, sortBy);
}

void FamCutLmFactory::quicksort(StaticS0Def *A, int lo, int hi, vector<int> *sortBy) {
    if ((lo >= 0) && (hi >= 0) && (lo < hi)) {
        int p = teile(A, lo, hi, sortBy);
        quicksort(A, lo, p, sortBy);
        quicksort(A, p + 1, hi, sortBy);
    }
}

int FamCutLmFactory::teile(StaticS0Def *A, int lo, int hi, vector<int> *sortBy) {
    int iPivot = (lo + hi) / 2;
    int* pivot = new int[A->sizeOfEntry];
    for (int i = 0; i < A->sizeOfEntry; i++) {
        pivot[i] = A->get(iPivot, i);
    }
    int i = lo - 1;
    int j = hi + 1;
    while (true) {
        do { i++; } while ((i <= hi) && (smaller(A, i, pivot, sortBy)));
        do { j--; } while ((j >= lo) && (greater(A, j, pivot, sortBy)));
        if (i >= j) {
            delete[] pivot;
            return j;
        }
        int temp;
        for (int k = 0; k < A->sizeOfEntry; k++) {
            temp = A->get(i, k);
            A->set(i, k, A->get(j, k));
            A->set(j, k, temp);
        }
    }
}

bool FamCutLmFactory::smaller(StaticS0Def *A, int i, int* pivot, vector<int> *sortBy) {
    for (int k : *sortBy) {
        int elem1 = A->get(i, k);
        int elem2 = pivot[k];
        if (elem1 != elem2) {
            return elem1 < elem2;
        }
    }
    return false; // they are equal
}

bool FamCutLmFactory::greater(StaticS0Def *A, int i, int* pivot, vector<int> *sortBy) {
    for (int k : *sortBy) {
        int elem1 = A->get(i, k);
        int elem2 = pivot[k];
        if (elem1 != elem2) {
           return elem1 > elem2;
        }
    }
    return false; // they are equal
}

void FamCutLmFactory::myassert(bool b) {
    if(!b) {
        cerr << "assertion failed" << endl;
        exit(-1);
    }
}

vector<Fact> *FamCutLmFactory::gets0Def(PINode *pNode) {
    vector<Fact> *res = new vector<Fact>();
    for (Fact f: problem.init) {
        if (pNode->schemaIndex != f.predicateNo) {
            continue;
        }
        bool compatible = true;
        for (int i = 0; i < f.arguments.size(); i++) {
            if ((pNode->consts[i] >= 0) && (pNode->consts[i] != f.arguments[i])) {
                compatible = false;
                break;
            }
        }
        if (compatible) {
            res->push_back(f);
        }
    }
    return res;
}

LandmarkGraph *FamCutLmFactory::generateAchieverNodes(PINode *n) {
    NecSubLmFactory nsl(domain, problem);
    LandmarkGraph *g = nsl.getLandmarks(n);
    g->showDot(domain, true);
    exit(0);
    LandmarkGraph *res = new LandmarkGraph();
    n->printFact(domain, cout);
    const int p = n->schemaIndex;
    for (pair<int, int> ach : achiever[p]) {
        cout << domain.tasks[ach.first].name << " ";
        cout << domain.predicates[domain.tasks[ach.first].effectsAdd[ach.second].predicateNo].name << endl;
    }
    exit(-1);
    return res;
}

LandmarkGraph *FamCutLmFactory::generateActionNodes(PINode *n) {
    LandmarkGraph *g = new LandmarkGraph();
    const int p = n->schemaIndex;
    Landmark *achieverLM = new Landmark(ActionOR);
    for (pair<int, int> ach: achiever[p]) {
        const int iA = ach.first;
        const int iEff = ach.second;
        PINode* action = new PINode();
        action->schemaIndex = iA;
        for (int i = 0; i < domain.tasks[iA].variableSorts.size(); i++) {
            action->consts.push_back(-1);
        }
        auto eff = domain.tasks[iA].effectsAdd[iEff];
        for (int i = 0; i < eff.arguments.size(); i++) {
            const int var = eff.arguments[i];
            const int obj = n->consts[i];
            action->consts[var] = obj;
        }
        achieverLM->lm.insert(action);
    }
    g->addNode(achieverLM);
    return g;
}

LandmarkGraph *FamCutLmFactory::generatePrecIntersection(Landmark *node) {
    bool empty = true;
    set<PINode*, mycompare> *interSection = new set<PINode*, mycompare>();
    set<PINode*, mycompare> *tempInterSection= new set<PINode*, mycompare>();

    for (auto actionNode: node->lm) {
        LandmarkGraph *subgraph = generatePrecNodes(actionNode);
        auto precs = subgraph->N;
//        subgraph->showDot(domain,true);
        if (empty) {
            empty = false;
            for (auto lm: precs) {
                interSection->insert(lm->getFirst());
            }
        } else {
            tempInterSection->clear();
            for (auto lm: precs) {
                auto fact = lm->getFirst();
                if (interSection->find(fact) != interSection->end()) {
                    tempInterSection->insert(fact);
                } else {
                    for (auto fact2: *interSection){
                        if (fact->abstractionOf(fact2)) {
                            tempInterSection->insert(fact);
                        } else if (fact2->abstractionOf(fact)) {
                            tempInterSection->insert(fact2);
                        }
                    }
                }
            }
            auto temp = tempInterSection;
            tempInterSection = interSection;
            interSection = temp;
        }
        if (interSection->empty()) {
            break;
        }
    }
    auto res = new LandmarkGraph();
    for (auto n: *interSection) {
        Landmark *lm = new Landmark(FactAND);
        lm->lm.insert(n);
        res->addNode(lm);
    }
    //res->showDot(domain, true);

    return res;
}

