//
// Created by dh on 02.12.21.
//

#include "NecSubLmFactory.h"
#include "LandmarkGraph.h"

NecSubLmFactory::NecSubLmFactory(Domain &domain, Problem &problem) : LmFactory(domain, problem) {
}

LandmarkGraph *NecSubLmFactory::getLandmarks(PINode *n) {
    Assignment.model = this.model;

    LandmarkGraph allLMs = new LandmarkGraph();
    List<LandmarkNode> newLMS = new ArrayList<>();
    if (model.Tinit >= 0) {
        LandmarkNode initialTask = new LandmarkNode(LandmarkNode.nTask, new int[]{model.Tinit});
        allLMs.add(initialTask);
        newLMS.add(initialTask);
    }

    for (int i = 0; i < model.numGoalFacts; i++) {
        LandmarkNode goalFact = new LandmarkNode(LandmarkNode.nFact, model.goal[i]);
        allLMs.add(goalFact);
        if (!goalFact.includedIn(model.s0)) {
            newLMS.add(goalFact);
        }
    }
    System.out.println(";; Found " + allLMs.size() + " trivial LMs.");
    //for(Assignment a : allLMs)
    //    System.out.println(a);

    while (!newLMS.isEmpty()) {
        List<LandmarkNode> thisRound = new ArrayList<>();
        for (LandmarkNode lm : newLMS) {
            //System.out.println("LM: " + lm);
            if (lm.isFact()) {
                // predicate -> [{objX, objY, ...}, {...}, {...}]
                // - the predicate might be a LM, it occurs in the precs of every action that adds the actual LM
                // - the sets capture possible parameters of this LM
                // - however, each parameter is considered separately -> these are candidates, not LMs

                int lmPred = lm.getHeadDef();

                // no action adding it? -> continue
                if (model.predicateToActionsAddingIt[lmPred].length == 0)
                    continue;

                // find action with least number of preconditions
                int precs = Integer.MAX_VALUE;
                int iA0 = -1;
                for (int iAc = 0; iAc < model.predicateToActionsAddingIt[lmPred].length; iAc++) {
                    int a = model.predicateToActionsAddingIt[lmPred][iAc][0];
                    if (model.numPrecond[a] < precs) {
                        iA0 = iAc;
                    }
                }
                int[] pair = model.predicateToActionsAddingIt[lmPred][iA0]; // pair of action and add effect
                int a = pair[0];
                int eff = pair[1];
                //System.out.println("Init LMs with:");
                //System.out.println(model.printTask(a));
                //System.out.println("a" + pair[0] + " - " + "e" + pair[1]);

                // which of the parameters are bound by the lm parameters?
                Map<Integer, Integer> binding = new HashMap<>();
                for (int i = 1; i < model.add[a][eff].length; i++) {
                    int paramIndex = model.add[a][eff][i];
                    int obj = lm.getParamsNo(i - 1);
                    if (obj == -1) // unbound
                        continue;
                    //System.out.println(paramIndex + " -> " + model.constNames[obj]);
                    binding.put(paramIndex, obj);
                }

                // go over preconditions and collect candidates for new LMs
                // special cases:
                // - two effects of one action match lm -> can be treated as two actions
                // - two preconditions of same action match -> can choose the "right" one
                List<int[]> candidate = new ArrayList<>();
                for (int i = 0; i < model.numPrecond[a]; i++) {
                    int[] lmc = new int[model.precond[a][i].length];
                    lmc[0] = model.precond[a][i][0];
                    int foundBindings = 0;
                    for (int k = 1; k < model.precond[a][i].length; k++) {
                        lmc[k] = -1; // unbound
                    }
                    for (int j = 1; j < model.precond[a][i].length; j++) {
                        int param = model.precond[a][i][j];
                        if (binding.containsKey(param)) {
                            lmc[j] = binding.get(param);
                            foundBindings++;
                        }
                    }
                    if (foundBindings >= keepIfBound) {
                        candidate.add(lmc);
                    }
                }

                candidate_loop:
                while (!candidate.isEmpty()) {
                    int[] lmc = candidate.remove(0);
                    // need to find a precondition of each action that matches with lm candidates
                    for (int iAc = 0; iAc < model.predicateToActionsAddingIt[lmPred].length; iAc++) {
                        if (iAc == iA0) continue; // this was used to generate candidates
                        pair = model.predicateToActionsAddingIt[lmPred][iAc]; // pair of action and add effect
                        a = pair[0];
                        eff = pair[1];
                        //System.out.println(model.printTask(a));
                        //System.out.println("a" + pair[0] + " - " + "e" + pair[1]);

                        // which of the parameters are bound by the lm parameters?
                        binding = new HashMap<>();
                        for (int j = 1; j < model.add[a][eff].length; j++) {
                            int paramIndex = model.add[a][eff][j];
                            int obj = lm.getParamsNo(j - 1);
                            if (obj == -1) continue; // unbound
                            binding.put(paramIndex, obj);
                        }

                        //List<int[]> relaxedCandidates = new ArrayList<>();
                        boolean foundsome = false;
                        precLoop:
                        for (int i = 0; i < model.numPrecond[a]; i++) {
                            int pred = model.precond[a][i][0];
                            if (lmc[0] != pred) continue;

                            for (int j = 1; j < model.precond[a][i].length; j++) {
                                int param = model.precond[a][i][j];
                                int obj = -1;
                                if (binding.containsKey(param)) {
                                    obj = binding.get(param);
                                }
                                if (lmc[j] != obj) {
                                    continue precLoop;
                                }
                            }
                            foundsome = true;
                            break;
                        }
                        if (!foundsome) {
                            continue candidate_loop;
                        }
                    }
                    LandmarkNode lmNew = new LandmarkNode(Assignment.nFact, lmc);
                    allLMs.add(lmNew, lm);
                    if (!lmNew.includedIn(model.s0)) {
                        thisRound.add(lmNew);
                    }
                }
            }
        }
        newLMS = thisRound;
    }
    System.out.println(";; Found " + allLMs.size() + " LMs.");
    return allLMs;
}
