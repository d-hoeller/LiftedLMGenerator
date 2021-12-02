//
// Created by dh on 02.12.21.
//

#include "NecSubLmFactory.h"
#include "LandmarkGraph.h"

NecSubLmFactory::NecSubLmFactory(Domain &domain, Problem &problem) : LmFactory(domain, problem) {
}

LandmarkGraph *NecSubLmFactory::getLandmarks(PINode *n) {
    LandmarkGraph *allLMs = new LandmarkGraph();
    vector<Landmark*> newLMS;
//    if (model.Tinit >= 0) {
//        LandmarkNode initialTask = new LandmarkNode(LandmarkNode.nTask, new int[]{model.Tinit});
//        allLMs.add(initialTask);
//        newLMS.add(initialTask);
//    }

    for (auto g: problem.goal) {
        PINode *pNode = getPINode(g);
        Landmark *goalFact = new Landmark(pNode, FactAND);
        allLMs->addNode(goalFact);
        if (!containedInS0(pNode)) {
            newLMS.push_back(goalFact);
        }
    }
    cout << ";; Found " << allLMs->N.size() << " trivial LMs." << endl;

    while (!newLMS.empty()) {
        vector<Landmark*> thisRound;
        for (Landmark lm : newLMS) {
            //System.out.println("LM: " + lm);
            if (lm.isFactLM) {
                // predicate -> [{objX, objY, ...}, {...}, {...}]
                // - the predicate might be a LM, it occurs in the precs of every action that adds the actual LM
                // - the sets capture possible parameters of this LM
                // - however, each parameter is considered separately -> these are candidates, not LMs

                PINode* n = lm.getFirst();
                const int lmPred = n->schemaIndex;

                // no action adding it? -> continue
                if (achiever[lmPred].empty())
                    continue;

                // find action with least number of preconditions
                int precs = INTMAX_MAX;
                int iA0 = -1;

                for (int iAch = 0; iAch < achiever[lmPred].size(); iAch++) {
                    pair<int, int> ach = achiever[lmPred][iAch];
                    const int a = ach.first;
                    if (domain.tasks[a].preconditions.size() < precs) {
                        iA0 = iAch;
                    }
                }
                pair<int, int> pair = achiever[lmPred][iA0]; // pair of action and add effect
                int a = pair.first;
                int eff = pair.second;
                //System.out.println("Init LMs with:");
                //System.out.println(model.printTask(a));
                //System.out.println("a" + pair[0] + " - " + "e" + pair[1]);

                // which of the parameters are bound by the lm parameters?
                map<int, int> binding;
                for (int i = 0; i < domain.tasks[a].effectsAdd[eff].arguments.size(); i++) {
                    int paramIndex = domain.tasks[a].effectsAdd[eff].arguments[i];
                    int obj = n->consts[i]; // todo: was i - 1 (?)
//                    if (obj == -1) // unbound
//                        continue;
                    //System.out.println(paramIndex + " -> " + model.constNames[obj]);
                    binding[paramIndex] = obj;
                }

                // go over preconditions and collect candidates for new LMs
                // special cases:
                // - two effects of one action match lm -> can be treated as two actions
                // - two preconditions of same action match -> can choose the "right" one
                vector<PINode*> candidate;
                auto prec = domain.tasks[a].preconditions;
                for (int i = 0; i < prec.size(); i++) {
                    PINode* lmc = new PINode();
                    lmc->schemaIndex = prec[i].predicateNo;
                    int foundBindings = 0;
                    for (int k = 0; k < prec[i].arguments.size(); k++) {
                        lmc->consts[k] = -1; // unbound
                    }
                    for (int j = 1; j < prec[i].arguments.size(); j++) {
                        int param = prec[i].arguments[j];
                        if (binding[param] >= 0) {
                            lmc->consts[j] = binding[param];
                            foundBindings++;
                        }
                    }
                    if (foundBindings >= keepIfBound) {
                        candidate.push_back(lmc);
                    }
                }

//                candidate_loop:
                while (!candidate.empty()) {
                    PINode* lmc = candidate.back();
                    candidate.pop_back();

                    // need to find a precondition of each action that matches with lm candidates
                    bool continueCandidateLoop = false;
                    for (int iAc = 0; iAc < achiever[lmPred].size(); iAc++) {
                        if (iAc == iA0) continue; // this was used to generate candidates
                        pair = achiever[lmPred][iAc]; // pair of action and add effect
                        a = pair.first;
                        eff = pair.second;
                        //System.out.println(model.printTask(a));
                        //System.out.println("a" + pair[0] + " - " + "e" + pair[1]);

                        // which of the parameters are bound by the lm parameters?
                        binding.clear();
                        for (int j = 1; j < domain.tasks[a].effectsAdd[eff].arguments.size(); j++) {
                            int paramIndex = domain.tasks[a].effectsAdd[eff].arguments[j];
                            int obj = n->consts[j];
//                            if (obj == -1) continue; // unbound
                            binding[paramIndex] = obj;
                        }

                        //List<int[]> relaxedCandidates = new ArrayList<>();
                        bool foundsome = false;
//                        precLoop:
                        bool continuePrecLoop = true;
                        auto prec = domain.tasks[a].preconditions;
                        for (int i = 0; i < prec.size(); i++) {
                            int pred = prec[i].predicateNo;
                            if (lmc->schemaIndex != pred) continue;

                            for (int j = 1; j < prec[i].arguments.size(); j++) {
                                int param = prec[i].arguments[j];
                                int obj = -1;
                                if (binding[param] >= 0) {
                                    obj = binding[param];
                                }
                                if (lmc->consts[j] != obj) { // is this correct?
                                    continuePrecLoop = true;
                                    break;
//                                    continue precLoop;
                                }
                            }
                            if (continuePrecLoop) {
                                continuePrecLoop = false;
                                continue;
                            }
                            foundsome = true;
                            break;
                        }
                        if (!foundsome) {
//                            continue candidate_loop;
                            continueCandidateLoop = true;
                        }
                    }
                    if (continueCandidateLoop) {
                        continueCandidateLoop = false;
                        continue;
                    }
                    Landmark *lmNew = new Landmark(lmc, FactAND);
                    allLMs->addNode(lmNew);
                    allLMs->addArc(lm.nodeID, lmNew->nodeID, 0);
                    if (!containedInS0(lmc)) {
                        thisRound.push_back(lmNew);
                    }
                }
            }
        }
        newLMS = thisRound;
    }
    cout << ";; Found " << allLMs->N.size() << " LMs.";
    return allLMs;
}
