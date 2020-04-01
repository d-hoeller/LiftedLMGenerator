#include <iostream>

#include "debug.h"
#include "duplicate.h"

void unify_duplicates(const Domain & domain, const Problem & problem,
		std::vector<Fact> & reachableFacts,
		std::vector<GroundedTask> & reachableTasks,
		std::vector<GroundedMethod> & reachableMethods,
		std::vector<bool> & prunedTasks,
		std::vector<bool> & prunedFacts,
		std::vector<bool> & prunedMethods,
		bool quietMode	
		){
	if (!quietMode) std::cout << "Starting duplicate elimination." << std::endl;
	// try do find tasks with identical preconditions and effects
	std::map<std::tuple<std::vector<int>, std::vector<int>, std::vector<int>>, std::vector<int>> duplicate;
	
	for (size_t tID = 0; tID < reachableTasks.size(); tID++){
		if (prunedTasks[tID]) continue;
		// don't do duplicate detection on abstract tasks
		if (reachableTasks[tID].taskNo >= domain.nPrimitiveTasks) continue;
		
		// check whether they are actually eligible for merging (only those that start with an underscore)
		if (domain.tasks[reachableTasks[tID].taskNo].name[0] != '_') continue;

		std::vector<int> pre;
		std::vector<int> add;
		std::vector<int> del;

		for (int x : reachableTasks[tID].groundedPreconditions) if (!prunedFacts[x]) pre.push_back(x);
		for (int x : reachableTasks[tID].groundedAddEffects) 	if (!prunedFacts[x]) add.push_back(x);
		for (int x : reachableTasks[tID].groundedDelEffects) 	if (!prunedFacts[x]) del.push_back(x);

		duplicate[std::make_tuple(pre,add,del)].push_back(tID);
	}
	if (!quietMode) std::cout << "Data structure build." << std::endl;


	std::map<int,int> taskReplacement;

	for (auto & entry : duplicate){
		if (entry.second.size() == 1) continue;

		DEBUG(std::cout << "Found action duplicates:";
				for (int t : entry.second) std::cout << " " << t;
				std::cout << std::endl);

		int representative = entry.second[0];

		// remove the others
		for (size_t i = 1; i < entry.second.size(); i++){
			taskReplacement[entry.second[i]] = representative;
			prunedTasks[entry.second[i]] = true;
		}
	}
	if (!quietMode) std::cout << taskReplacement.size() << " duplicates found." << std::endl;

	// perform the actual replacement (in methods)
	for (size_t mID = 0; mID < reachableMethods.size(); mID++){
		if (prunedMethods[mID]) continue;
		for (size_t sub = 0; sub < reachableMethods[mID].groundedPreconditions.size(); sub++){
			auto it = taskReplacement.find(reachableMethods[mID].groundedPreconditions[sub]);
			if (it == taskReplacement.end()) continue;

			reachableMethods[mID].groundedPreconditions[sub] = it->second;
		}
	}
	
	if (!quietMode) std::cout << "Duplicates replaced in methods." << std::endl;
}

