//
// Created by dh on 12.12.20.
//

#include "FamCutLmFactory.h"
//#include "LDTG.h"
#include <iostream>
#include <fstream>
#include <map>
#include <cassert>
#include "PINode.h"
#include "PIGraph.h"
#include "PIArc.h"

FamCutLmFactory::FamCutLmFactory(Domain d, Problem p, vector<FAMGroup> fg) {
    this->domain = d;
    this->problem = p;
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

    modifier = new vector<FAMmodifier *>[fg.size()];
    for (int iFAM = 0; iFAM < fg.size(); iFAM++) {
        for (int i = 0; i < domain.tasks.size(); i++) {
            vector<int> relevantPrecs;
            vector<int> relevantAdds;
            vector<int> relevantDels;
            for (int k = 0; k < domain.tasks[i].preconditions.size(); k++) {
                if (isCompatible(domain.tasks[i], domain.tasks[i].preconditions[k], fg[iFAM])) {
                    relevantPrecs.push_back(k);
                }
            }
            for (int k = 0; k < domain.tasks[i].effectsAdd.size(); k++) {
                if (isCompatible(domain.tasks[i], domain.tasks[i].effectsAdd[k], fg[iFAM])) {
                    relevantAdds.push_back(k);
                }
            }
            for (int k = 0; k < domain.tasks[i].effectsDel.size(); k++) {
                if (isCompatible(domain.tasks[i], domain.tasks[i].effectsDel[k], fg[iFAM])) {
                    relevantDels.push_back(k);
                }
            }
//            cout << "(" << domain.tasks[i].name;
//            for (int j = 0; j < domain.tasks[i].variableSorts.size(); j++) {
//                int s = domain.tasks[i].variableSorts[j];
//                cout << " v" << j << " - " << domain.sorts[s].name;
//            }
//            cout << ")" << endl << ":preconditions" << endl;
//            for (int j: relevantPrecs) {
//                int p = domain.tasks[i].preconditions[j].predicateNo;
//                cout << "   (" << domain.predicates[p].name;
//                for (int k = 0; k < domain.tasks[i].preconditions[j].arguments.size(); k++) {
//                    cout << " v" << domain.tasks[i].preconditions[j].arguments[k];
//                }
//                cout << ")" << endl;
//            }
//            cout << ":effects" << endl;
//            for (int j: relevantAdds) {
//                int p = domain.tasks[i].effectsAdd[j].predicateNo;
//                cout << "   (" << domain.predicates[p].name;
//                for (int k = 0; k < domain.tasks[i].effectsAdd[j].arguments.size(); k++) {
//                    cout << " v" << domain.tasks[i].effectsAdd[j].arguments[k];
//                }
//                cout << ")" << endl;
//            }
//            for (int j: relevantDels) {
//                int p = domain.tasks[i].effectsDel[j].predicateNo;
//                cout << "   (not (" << domain.predicates[p].name;
//                for (int k = 0; k < domain.tasks[i].effectsDel[j].arguments.size(); k++) {
//                    cout << " v" << domain.tasks[i].effectsDel[j].arguments[k];
//                }
//                cout << "))" << endl;
//            }

            if (relevantAdds.empty() && relevantDels.empty()) {
                // maybe a precondition, no effect: not interesting, not harmful
            } else if ((relevantPrecs.size() == 1 && relevantAdds.size() == 1 && relevantDels.size() == 1)
                       && isNormalArc(i, relevantPrecs[0], relevantDels[0])) {
                FAMmodifier *mod = new FAMmodifier;
                mod->action = i;
                mod->prec = relevantPrecs[0];
                mod->add = relevantAdds[0];
                this->modifier[iFAM].push_back(mod);
                //cout << "MOD: " << iFAM << " " << mod->action << " " <<domain.tasks[mod->action].name << endl;
            } else {
                cout << "AAAAHH" << endl;
            }
        }
    }

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

void FamCutLmFactory::findGoalMatches() {
    cout << "Matching goals to FAM groups" << endl;
    for (int i = 0; i < problem.goal.size(); i++) {
        for (int j = 0; j < famGroups.size(); j++) { // todo: does it make sense for a goal to be in two groups?
            findGoalMatch(i, j);
        }
    }
}

void FamCutLmFactory::findGoalMatch(int ig, int ifg) {
    int p = problem.goal[ig].predicateNo;
    FAMGroup fg = famGroups[ifg];
    // a package might be "AT a position" or "IN a truck" -> these are the literals
    for (int iLit = 0; iLit < fg.literals.size(); iLit++) {
        if (fg.literals[iLit].predicateNo == p) {
            bool match = true;
            for (int iArg = 0; iArg < fg.literals[iLit].args.size(); iArg++) { // loop params
                if (fg.literals[iLit].isConstant[iArg]) {
                    // check for non-matching constants
                    if (fg.literals[iLit].args[iArg] != problem.goal[ig].arguments[iArg]) {
                        match = false;
                        break;
                    }
                } else {
                    // check variable types
                    int s = fg.vars[fg.literals[iLit].args[iArg]].sort;
                    int c = problem.goal[ig].arguments[iArg];
                    if (domain.sorts[s].members.find(c) == domain.sorts[s].members.end()) {
                        match = false;
                        break;
                    }
                }
            }
            if (match) {
                addGoalMatch(ifg, iLit, ig);
            }
        }
    }
}

void FamCutLmFactory::addGoalMatch(int iGroup, int iLiteral, int iGoal) {
    GoalMatch *gm = new GoalMatch();
    gm->iFamGroup = iGroup;
    gm->iLiteral = iLiteral;
    gm->iGoal = iGoal;
    this->matches.push_back(gm);
    //printMatch(matches.size() - 1);
}

void FamCutLmFactory::generateLMs() {
    //for (int i = 0; i < matches.size(); i++) {
    //    generateLMs(i);
    //}
    generateLMs(0);
}

void FamCutLmFactory::generateLMs(int ig) {
    GoalMatch *gm = matches[ig];
    FAMGroup fam = famGroups[gm->iFamGroup];
    Fact fgoal = problem.goal[gm->iGoal];

    cout << "Generating DTG containing LM (";
    cout << domain.predicates[fgoal.predicateNo].name;
    for (int k = 0; k < fgoal.arguments.size(); k++)
        cout << " " << domain.constants[fgoal.arguments[k]];
    cout << ")" << endl;

    // need to store the free variables set by the goal fact
    vector<int> setFreeVars;
    for (int i = 0; i < fam.vars.size(); i++) setFreeVars.push_back(-1);
    for (int iLit = 0; iLit < fam.literals.size(); iLit++) {
        if (fam.literals[iLit].predicateNo == fgoal.predicateNo) {
            for (int iLitArg = 0; iLitArg < fam.literals[iLit].args.size(); iLitArg++) {
                int iFamArg = fam.literals[iLit].args[iLitArg];
                if (!fam.vars[iFamArg].isCounted) {
                    setFreeVars[iFamArg] = fgoal.arguments[iLitArg];
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
    PIGraph *s0 = new PIGraph;
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
                    cout << "Starting value of DTG is     (";
                    cout << domain.predicates[n->schemaIndex].name;
                    for (int k = 0; k < n->consts.size(); k++)
                        cout << " " << domain.constants[n->consts[k]];
                    cout << ")" << endl;
                    sameFAMNodes.push_back(n);
                }
            }
        }
    }
    if (sameFAMNodes.size() != 1) {
        exit(-17);
    }

//    unordered_set<PINode *, PINodeHasher, PINodeComparator> N;
    PIGraph graph;
    auto *N_last = new vector<PINode *>;
    auto *N_this = new vector<PINode *>;

    graph.addNode(sameFAMNodes[0]);
    N_last->push_back(sameFAMNodes[0]);

    bool groundFAMVars = false;
    while (!N_last->empty()) {
        for (auto n: *N_last) {
            cout << "successors for (";
            cout << domain.predicates[n->schemaIndex].name;
            for (int k = 0; k < n->consts.size(); k++) {
                int obj = n->consts[k];
                if(obj < 0) {
                    cout << " ?";
                } else {
                    cout << " " << domain.constants[obj];
                }
            }
            cout << ")" << endl;

            // need actions where
            // - precondition is n
            // - static preconditions hold
            cout << "arcs: " << modifier[gm->iFamGroup].size() << endl;
            for (auto arc: modifier[gm->iFamGroup]) {
                cout << "- action: " << domain.tasks[arc->action].name << endl;
                int a = arc->action;
                int numVars = domain.tasks[a].variableSorts.size();
                PINode *partInstAction = new PINode;
                for (int i = 0; i < numVars; i++) {
                    partInstAction->consts.push_back(-1);
                }
                auto prec = domain.tasks[a].preconditions[arc->prec];
                partInstAction->schemaIndex = a;
                if ((prec.predicateNo == n->schemaIndex) && (isCompatible(domain.tasks[a], prec, fam))) {
                    // determine bindings by precondition belonging to FAM group
                    for (int iPrec = 0; iPrec < prec.arguments.size(); iPrec++) {
                        int var = prec.arguments[iPrec];
                        int val = n->consts[iPrec];
                        partInstAction->consts[var] = val;
                    }
                    cout << "  - (" << domain.tasks[prec.predicateNo].name;
                    for (int i = 0; i < numVars; i++) {
                        int obj = partInstAction->consts[i];
                        if (obj == -1) {
                            cout << " ?";
                        } else {
                            cout << " " << domain.constants[obj];
                        }
                    }
                    cout << ")" << endl;
                    // determine bindings by static precondition
//                    for (int inv: arc->staticPrecs) {
//                        PINode *partInstPrec = new PINode;
//                        auto precSchema = domain.tasks[a].preconditions[inv];
//                        for (int i = 0; i < precSchema.arguments.size(); i++) {
//                            int var = precSchema.arguments[i];
//                            partInstPrec->consts.push_back(partInstantiation->consts[var]);
//                        }
//                        StaticS0Def* s0Def = getStaticS0Def(precSchema.predicateNo);
//                        sortS0Def(partInstPrec, s0Def);
//                    }
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
                            if ((partInstAction->consts[i] == -1) &&
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
                            int var = eff.arguments[l];
                            partInstEffect->consts.push_back(action->consts[var]);
                        }
                        auto iter = graph.N.find(partInstEffect);
                        int to;
                        if (iter == graph.N.end()) {
                            graph.addNode(partInstEffect);
                            to = partInstEffect->nodeID;
                            N_this->push_back(partInstEffect);
                        } else {
                            to = (*iter)->nodeID;
                        }

                        // add arcs
                        graph.addArc(n->nodeID, to, action);
                        cout << "  -> (" << domain.predicates[partInstEffect->schemaIndex].name;
                        for (int i = 0; i < partInstEffect->consts.size(); i++) {
                            int obj = partInstEffect->consts[i];
                            if (obj == -1) {
                                cout << " ?";
                            } else {
                                cout << " " << domain.constants[obj];
                            }
                        }
                        cout << ")" << endl;
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


//    cout << "Graph: " << graph.N.size() << endl;
//
//    for (auto s: domain.sorts) {
//        cout << s.name << " " << s.members.size() << endl;
//    }

    graph.showDot(domain);

    PINode *goalNode = new PINode();
    goalNode->schemaIndex = fgoal.predicateNo;
    for (int k = 0; k < fgoal.arguments.size(); k++) {
        goalNode->consts.push_back(fgoal.arguments[k]);
    }

    set<int> goalNodes;
    for (PINode *n: graph.N) {
        if (n->abstractionOf(goalNode)) {
            goalNodes.insert(n->nodeID);
            cout << "goal id " << n->nodeID << endl;
        }
    }
    cout << "Goal nodes: " << goalNodes.size() << endl;

    // find fact landmarks
    set<int> from;
    from.insert(sameFAMNodes[0]->nodeID);
    cout << "init id: " << sameFAMNodes[0]->nodeID << endl;
    graph.deactivatedNodes.clear();
    if (graph.reachable(from, goalNodes)) {
        cout << "reachable" << endl;
        for (auto n: graph.N) {

            if ((from.find(n->nodeID) != from.end()) || (goalNodes.find(n->nodeID) != goalNodes.end())) {
                continue;
            }
            graph.deactivatedNodes.insert(n->nodeID);
            if (!graph.reachable(from, goalNodes)) {
                cout << "- ";
                n->printFact(domain);
                cout << " is a LM" << endl;
            }
            graph.deactivatedNodes.clear();
        }
    }
}


/*
void FA::showDOT(string *pString) {
    system("rm fa.dot");
    //system("rm fa.dot.pdf");

    ofstream myfile;
    myfile.open ("fa.dot");

    myfile << endl << "digraph D {" << endl << endl;

    for (int i : sInit) {
        myfile << "   n" << i << " [shape=diamond]" << endl;
    }
    for (int g : sGoal) {
        myfile << "   n" << g << " [shape=box]" << endl;
    }

    delta->fullIterInit();
    tStateID from, to;
    tLabelID label;
    while (delta->fullIterNext(&from, &label, &to)) {
        if (label == epsilon) {
            myfile << "   n" << from << " -> n" << to << " [label=epsilon]" << endl;
        } else {
            myfile << "   n" << from << " -> n" << to << " [label=\"" << pString[label] << "\"]" << endl;
        }
    }

    myfile << endl << "}" << endl;
    myfile.close();
    system("xdot fa.dot");
    //system("dot -Tpdf -O fa.dot");
    //system("okular fa.dot.pdf &");
}

 */

//void FamCutLmFactory::generateLMs(int ig) {
//    GoalMatch *gm = matches[ig];
//    FAMGroup fam = famGroups[gm->iFamGroup];
//
//    // need to determine fix binding decisions:
//    // - some free variables are assigned by goal definition
//    // - some cVarNeedToGround might also be set by goal, these are the target to reach in the graph
//    // - some variables from the fam might be set to consts in the fam definition
//    vector<int> cVarNeedToGround;
//    set<int> countedVars;
//    map<int,int> freeButSet; // free variables that are set by goal def
//
//    LDTG* node = new LDTG();
//    node->predicate = fam.literals[gm->iLiteral].predicateNo;
//    node->numConsts = fam.literals[gm->iLiteral].args.size();
//    node->consts = new int[node->numConsts];
//    for(int i = 0; i < node->numConsts; i++) {node->consts[i] = -1;}
//
//    int iLit = gm->iLiteral;
//    for (int iParam = 0; iParam < fam.literals[iLit].args.size(); iParam++) {
//        if (fam.literals[iLit].isConstant[iParam]) {
//            node->consts[iParam] = fam.literals[iLit].args[iParam];
//        } else {
//            int famVar = fam.literals[iLit].args[iParam];
//            if(!fam.vars[famVar].isCounted) {
//                int gConst = problem.goal[gm->iGoal].arguments[iParam];
//                node->consts[iParam] = gConst;
//                freeButSet[famVar] = gConst; // need to set other literals
//            } else {
//                cVarNeedToGround.push_back(iParam);
//                countedVars.insert(famVar);
//            }
//        }
//    }
//
//    vector<LDTG*>* nodes1 = groundNodes(node, gm->iFamGroup, iLit, cVarNeedToGround);
//
//    // need to generate the other literals
//
//    cVarNeedToGround.clear();
//    for(int i = 0; i < fam.literals.size(); i++) {
//        if (i == iLit) continue; // already done
//        FAMGroupLiteral &famLit = fam.literals[iLit];
//        LDTG *n = new LDTG();
//        n->predicate = famLit.predicateNo;
//        n->numConsts = famLit.args.size();
//        n->consts = new int [n->numConsts];
//        for(int j = 0; j < n->numConsts; j++) n->consts[j] = -1; // initialize
//
//        for (int iParam = 0; iParam < famLit.args.size(); iParam++){
//            if (famLit.isConstant[iParam]) {
//               n->consts[iParam] = famLit.args[iParam];
//            } else {
//                int famVar = famLit.args[iParam];
//                if(fam.vars[famVar].isCounted) {
//                    cVarNeedToGround.push_back(iParam);
//                } else {
//                    if (freeButSet.find(famVar) != freeButSet.end()) {
//                        n->consts[iParam] = freeButSet[famVar];
//                    } else {
//                        // might have counted -> need to ground
//                        // might have free -> set or unset?
//                        // - todo what if this is free? -> just set a whildcard?
//                        assert(false);
//                    }
//                }
//            }
//        }
//
//        vector<LDTG*>* nodes5 = groundNodes(n, gm->iFamGroup, iLit,cVarNeedToGround);
//        for(LDTG* node : *nodes5) {
//            nodes1->push_back(node);
//        }
//    }
//
//    cout << endl << "nodes:" << endl;
//    for(LDTG* n : *nodes1) {
//        printNode(n);
//    }
//    exit(0);
//}

void FamCutLmFactory::printFamGroup(int i) {
    cout << std::endl << "FAM group " << i << " { ";
    FAMGroup fg = famGroups[i];

    for (int j = 0; j < fg.literals.size(); j++) {
        FAMGroupLiteral famLits = fg.literals[j];
        std::cout << "(" << domain.predicates[famLits.predicateNo].name;
        for (int k = 0; k < famLits.args.size(); k++) {
            int index = famLits.args[k];
            std::cout << " ";
            if (famLits.isConstant[k]) {
                std::cout << domain.constants[index];
            } else {
                std::cout << "v" << index;
            }
        }
        cout << ") ";
    }
    std::cout << "}" << std::endl;
    std::cout << "counted vars: {";
    for (int j = 0; j < fg.vars.size(); j++) {
        FAMVariable v = fg.vars[j];
        if (v.isCounted)
            std::cout << " [v" << j << " - " << domain.sorts[v.sort].name << "]";
    }
    std::cout << " }" << std::endl;

    std::cout << "free vars:    {";
    for (int j = 0; j < fg.vars.size(); j++) {
        FAMVariable v = fg.vars[j];
        if (!v.isCounted) {
            std::cout << " [v" << j << " - " << domain.sorts[v.sort].name << "]";
        }
    }
    std::cout << " }" << std::endl;
}

void FamCutLmFactory::printMatch(int i) {
    GoalMatch *gm = matches[i];
    int p = problem.goal[gm->iGoal].predicateNo;
    std::cout << "- (" << domain.predicates[p].name;
    for (int j = 0; j < problem.goal[gm->iGoal].arguments.size(); j++) {
        std::cout << " " << domain.constants[problem.goal[gm->iGoal].arguments[j]];
    }
    std::cout << ") ->";

    FAMGroup fg = famGroups[gm->iFamGroup];
    for (int k = 0; k < fg.literals[gm->iLiteral].args.size(); k++) {
        if (!fg.literals[gm->iLiteral].isConstant[k]) {
            int famVar = fg.literals[gm->iLiteral].args[k];
            cout << " [v" << famVar << " = " << domain.constants[problem.goal[gm->iGoal].arguments[k]] << "]";
        }
    }
    std::cout << std::endl;
}

//vector<LDTG *> *FamCutLmFactory::groundNodes(LDTG *node, int iFG, int iLit, vector<int> cVarNeedToGround) {
//    vector<LDTG *> *nodes1 = new vector<LDTG *>;
//    FAMGroup fam = famGroups[iFG];
//    nodes1->push_back(node);
//    vector<LDTG *> *nodes2 = new vector<LDTG *>;
//    for (int k = 0; k < cVarNeedToGround.size(); k++) {
//        int iLitParam = cVarNeedToGround[k]; // this is a parameter index of the node
//        int iFamVar = fam.literals[iLit].args[iLitParam]; // this is the index of the fam parameter at this position
//        FAMVariable cvar = fam.vars[iFamVar];
//        int ctype = cvar.sort;
//        nodes2->clear();
//        for (int obj: domain.sorts[ctype].members) {
//            for (int ni = 0; ni < nodes1->size(); ni++) {
//                LDTG *node2 = nodes1->at(ni)->clone();
//                node2->consts[iLitParam] = obj;
//                nodes2->push_back(node2);
//            }
//        }
//        vector<LDTG *> *nodes3 = nodes1;
//        nodes1 = nodes2;
//        nodes2 = nodes3;
//    }
//    return nodes1;
//}

//void FamCutLmFactory::printNode(LDTG *n) {
//    cout << "- (" << domain.predicates[n->predicate].name;
//    for (int i = 0; i < n->numConsts; i++) {
//        cout << " " << domain.constants[n->consts[i]];
//    }
//    cout << ")" << endl;
//}

bool FamCutLmFactory::isCompatible(Task &t, PredicateWithArguments &lit, FAMGroup &fg) {
    int p = lit.predicateNo;
    for (int iLit = 0; iLit <
                       fg.literals.size(); iLit++) { // a package might be "AT a position" or "IN a truck" -> these are the literals
        if (fg.literals[iLit].predicateNo == p) {
            bool match = true;
            for (int iArg = 0; iArg < fg.literals[iLit].args.size(); iArg++) { // loop params
                if (!fg.literals[iLit].isConstant[iArg]) { // todo: what if it is?
                    // check variable types
                    int sFG = fg.vars[fg.literals[iLit].args[iArg]].sort;
                    int iVar = lit.arguments[iArg];
                    int sPrec = t.variableSorts[iVar];

                    if (sFG == sPrec) {
                        continue; // check next variable
                    } else {
                        bool intersects = false;
                        for (int c: domain.sorts[sPrec].members) {
                            if (domain.sorts[sFG].members.find(c) != domain.sorts[sFG].members.end()) {
//                        cout << endl << "FAM-Lit: (" << domain.sorts[sFG].name << ") ";
//                        for (int obj : domain.sorts[sFG].members) {
//                            cout << domain.constants[obj] << " ";
//                        }
//                        cout << endl << "Prec: (" << domain.sorts[sPrec].name << ") ";
//                        for (int obj : domain.sorts[sPrec].members) {
//                            cout << domain.constants[obj] << " ";
//                        }
//                        cout << endl;
//                                cout << domain.constants[c] << " ";
                                intersects = true;
                                break;
                            }
                        }
                        if (intersects) {
                            continue; // check next variable
                        }
                    }
                    match = false;
                }
            }
            if (match) { return true; }
        }
    }
    return false;
}

bool FamCutLmFactory::isNormalArc(int action, int relPrec, int relDel) {
    assert((action >= 0) && (action < domain.tasks.size()));
    assert((relPrec >= 0) && (relPrec < domain.tasks[action].preconditions.size()));
    assert((relDel >= 0) && (relDel < domain.tasks[action].effectsDel.size()));

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
