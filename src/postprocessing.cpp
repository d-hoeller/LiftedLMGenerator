#include <unistd.h>
#include <iostream>
#include <cassert>


#include "util.h"
#include "postprocessing.h"
#include "debug.h"



void sortSubtasksOfMethodsTopologically(const Domain & domain,
		std::vector<bool> & prunedTasks,
		std::vector<bool> & prunedMethods,
		std::vector<GroundedMethod> & inputMethodsGroundedTdg){
	for (GroundedMethod & method : inputMethodsGroundedTdg){
		if (prunedMethods[method.groundedNo]) continue;
		
		// find a topological ordering of the subtasks
		std::vector<std::vector<int>> adj(method.groundedPreconditions.size());
		
		auto orderings = domain.decompositionMethods[method.methodNo].orderingConstraints;
		for (std::pair<int,int> ordering : orderings)
			adj[ordering.first].push_back(ordering.second);

		topsort(adj, method.preconditionOrdering);

	}
}


void applyEffectPriority(const Domain & domain,
		std::vector<bool> & prunedTasks,
		std::vector<GroundedTask> & inputTasksGroundedPg,
		std::vector<Fact> & inputFactsGroundedPg){

	for (GroundedTask & task : inputTasksGroundedPg){
		if (task.taskNo >= domain.nPrimitiveTasks || prunedTasks[task.groundedNo]) continue;

		std::set<int> addSet;
		for (int & add : task.groundedAddEffects) addSet.insert(add);

		// look for del effects that are also add effects
		std::set<int> addToRemove;
		std::set<int> delToRemove;
		for (int & del : task.groundedDelEffects)
			if (addSet.count(del)){
				// edge case, if this is a negated original predicate, then the del effect takes precedence
				Fact & fact = inputFactsGroundedPg[del];
				if (domain.predicates[fact.predicateNo].name[0] != '-')
					delToRemove.insert(del);
				else
					addToRemove.insert(del);
			}

		if (addToRemove.size()){
			std::vector<int> newAdd;
			for (int & add : task.groundedAddEffects)
				if (! addToRemove.count(add))
					newAdd.push_back(add);
			task.groundedAddEffects = newAdd;
		}

		if (delToRemove.size()){
			std::vector<int> newDel;
			for (int & del : task.groundedDelEffects)
				if (! delToRemove.count(del))
					newDel.push_back(del);
			task.groundedDelEffects = newDel;
		}

	}
}


void removeUnnecessaryFacts(const Domain & domain,
		const Problem & problem,
		std::vector<bool> & prunedTasks,
		std::vector<bool> & prunedFacts,
		std::vector<GroundedTask> & inputTasksGroundedPg,
		std::vector<Fact> & inputFactsGroundedPg){

	std::set<Fact> reachableFacts(inputFactsGroundedPg.begin(), inputFactsGroundedPg.end());
	//

	// find facts that are static
	// first determine their initial truth value
	std::vector<bool> initialTruth(prunedFacts.size());
	// all facts in the initial state are initially true
	for (const Fact & f : problem.init)
		initialTruth[reachableFacts.find(f)->groundedNo] = true;

	// check all non-pruned action's effects
	std::vector<bool> truthChanges (prunedFacts.size());
	for (GroundedTask & task : inputTasksGroundedPg){
		if (task.taskNo >= domain.nPrimitiveTasks || prunedTasks[task.groundedNo]) continue;
		// iterate over add and del effects
		for (int & add : task.groundedAddEffects)
			if (!initialTruth[add]) // if an actions adds this, but it is not initially true, it might be added
				truthChanges[add] = true;
			
		for (int & del : task.groundedDelEffects)
			if (initialTruth[del]) // opposite for a del
				truthChanges[del] = true;
	}

	// look out for facts whose truth never changes
	for (int f = 0; f < initialTruth.size(); f++)
		if (!truthChanges[f]){
			DEBUG(
			Fact & fact = inputFactsGroundedPg[f];
			std::cout << "static predicate " << domain.predicates[fact.predicateNo].name << "[";
			for (unsigned int i = 0; i < fact.arguments.size(); i++){
				if (i) std::cout << ",";
				std::cout << domain.constants[fact.arguments[i]];
			}
			std::cout << "]" << std::endl;
		
			for (const Fact & gf : problem.goal){
				auto it = reachableFacts.find(gf);
				if (it != reachableFacts.end()){
					if (f == it->groundedNo){
						std::cout << "Deleting goal fact, in init " << initialTruth[f] << std::endl;
					}
				}
			});

			// prune the fact that does not change its truth value
			prunedFacts[f] = true;
		}
	
	// look for facts that to not occur in preconditions. They can be removed as well
	std::vector<bool> occuringInPrecondition (prunedFacts.size());
	for (GroundedTask & task : inputTasksGroundedPg)
		for (int & pre : task.groundedPreconditions)
			occuringInPrecondition[pre] = true;
	// facts in the goal may also not be removed
	for (const Fact & f : problem.goal){
		auto it = reachableFacts.find(f);
		if (it == reachableFacts.end()){
			// TODO detect this earlier and do something intelligent
			std::cerr << "Goal is unreachable ..." << std::endl;
			_exit(0);
		}
		occuringInPrecondition[it->groundedNo] = true;
	}

	// remove facts that are not contained in preconditions
	for (int f = 0; f < occuringInPrecondition.size(); f++)
		if (!occuringInPrecondition[f]){
			DEBUG(
			Fact & fact = inputFactsGroundedPg[f];
			std::cout << "no precondition predicate " << domain.predicates[fact.predicateNo].name << "[";
			for (unsigned int i = 0; i < fact.arguments.size(); i++){
				if (i) std::cout << ",";
				std::cout << domain.constants[fact.arguments[i]];
			}
			std::cout << "]" << std::endl;

			for (const Fact & gf : problem.goal){
				auto it = reachableFacts.find(gf);
				if (it != reachableFacts.end()){
					if (f == it->groundedNo){
						std::cout << "Deleting goal fact, in init " << initialTruth[f] << std::endl;
					}
				}
			});

			// prune the fact that does not change its truth value
			prunedFacts[f] = true;
		}
}

void expandAbstractTasksWithSingleMethod(const Domain & domain,
		const Problem & problem,
		std::vector<bool> & prunedTasks,
		std::vector<bool> & prunedMethods,
		std::vector<GroundedTask> & inputTasksGroundedPg,
		std::vector<GroundedMethod> & inputMethodsGroundedTdg){

	std::vector<std::set<int>> taskToMethodsTheyAreContainedIn (inputTasksGroundedPg.size());
	for (auto & method : inputMethodsGroundedTdg){
		if (prunedMethods[method.groundedNo]) continue;
		for (size_t subTaskIdx = 0; subTaskIdx < method.groundedPreconditions.size(); subTaskIdx++)
			taskToMethodsTheyAreContainedIn[method.groundedPreconditions[subTaskIdx]].insert(method.groundedNo);
	}
	
	// may need to repeat due to empty methods hat might create new unit methods
	bool emptyExpanded = true;
	while (emptyExpanded) {
		emptyExpanded = false;
		for (auto & groundedTask : inputTasksGroundedPg){
			if (prunedTasks[groundedTask.groundedNo]) continue;
			if (groundedTask.taskNo < domain.nPrimitiveTasks) continue;
			// don't compactify the top method
			if (groundedTask.taskNo == problem.initialAbstractTask) continue;
			
			int applicableIndex = -1;
			for (auto & groundedMethodIdx : groundedTask.groundedDecompositionMethods)
			{
				if (prunedMethods[groundedMethodIdx])
					continue;
				if (applicableIndex != -1) {
					applicableIndex = -2;
					break;
				}
				applicableIndex = groundedMethodIdx;
			}
			// it can't be -1, else the TDG would have eliminated it
			if (applicableIndex == -2) continue;
			assert(applicableIndex != -1);

			// this method is now pruned ...
			prunedMethods[applicableIndex] = true;
			prunedTasks[groundedTask.groundedNo] = true;
	
			GroundedMethod & unitGroundedMethod = inputMethodsGroundedTdg[applicableIndex];
			DecompositionMethod unitLiftedMethod = domain.decompositionMethods[unitGroundedMethod.methodNo];

			std::string decomposedTaskName = domain.tasks[groundedTask.taskNo].name + "[";
			for (size_t i = 0; i < groundedTask.arguments.size(); i++){
				if (i) decomposedTaskName += ",";
				decomposedTaskName += domain.constants[groundedTask.arguments[i]];
			}
			decomposedTaskName += "]";
	
			// apply this method in all methods it is occurring
			DEBUG( std::cerr << "Abstract task " << groundedTask.groundedNo << " has only a single method" << std::endl);
			for (const int & method : taskToMethodsTheyAreContainedIn[groundedTask.groundedNo]){
				if (prunedMethods[method]) continue;
				DEBUG( std::cerr << "expanding in method " << method << std::endl);
				// copy the lifted method
				GroundedMethod & groundedMethod = inputMethodsGroundedTdg[method];
				DecompositionMethod liftedMethod = domain.decompositionMethods[groundedMethod.methodNo];
				
				while (true){
					bool found = false;
					for (size_t subTaskIdx = 0; subTaskIdx < liftedMethod.subtasks.size(); subTaskIdx++){
						DEBUG( std::cerr << "Checking  #" << subTaskIdx << ": " << groundedMethod.groundedPreconditions[subTaskIdx] << " == " << groundedTask.groundedNo << "?" << std::endl);
						if (groundedMethod.groundedPreconditions[subTaskIdx] == groundedTask.groundedNo){
							found = true;
							DEBUG( std::cerr << "Yes, so expand it" << std::endl);
	
							std::vector<std::pair<int,int>> orderPertainingToThisTask;
							std::vector<std::pair<int,int>> orderNotPertainingToThisTask;
							for(auto order : liftedMethod.orderingConstraints)
								if (order.first == subTaskIdx || order.second == subTaskIdx)
									orderPertainingToThisTask.push_back(order);
								else 
									orderNotPertainingToThisTask.push_back(order);
		
							
							std::vector<int> idmapping;
							int positionOfExpanded = -1;
	
							// if the method we have to apply is empty
							if (unitGroundedMethod.groundedPreconditions.size() == 0){
								DEBUG( std::cerr << "Applied method is empty." << std::endl);
								emptyExpanded = true;
								groundedMethod.groundedPreconditions.erase(groundedMethod.groundedPreconditions.begin() + subTaskIdx);
								liftedMethod.subtasks.erase(liftedMethod.subtasks.begin() + subTaskIdx);
								// adapt the ordering of the subtasks accordingly

								std::vector<int> newOrder;
								for (size_t i = 0; i < groundedMethod.preconditionOrdering.size(); i++){
									if (groundedMethod.preconditionOrdering[i] == subTaskIdx){
										positionOfExpanded = i;
									} else {
										bool afterDeleted = groundedMethod.preconditionOrdering[i] > subTaskIdx;
										newOrder.push_back(groundedMethod.preconditionOrdering[i] - (afterDeleted?1:0));
										idmapping.push_back(i);
									}
								}

								groundedMethod.preconditionOrdering = newOrder;

								// orderings that were transitively induced using the removed task
								for (auto a : orderPertainingToThisTask) {
									if (a.second != subTaskIdx) continue;
									for (auto b : orderPertainingToThisTask) {
										if (b.first != subTaskIdx) continue;
										orderNotPertainingToThisTask.push_back(std::make_pair(a.first,b.second));
									}
								}
								liftedMethod.orderingConstraints.clear();
								for (auto order : orderNotPertainingToThisTask){
									if (order.first > subTaskIdx) order.first--;
									if (order.second > subTaskIdx) order.second--;
									liftedMethod.orderingConstraints.push_back(order);
								}
								break; // we can't go on from here, we have to restart the loop. It is too dangerous
							} else {
								DEBUG( std::cerr << "Applied method is not empty." << std::endl);
								// set first subtask and add the rest
								groundedMethod.groundedPreconditions[subTaskIdx] = unitGroundedMethod.groundedPreconditions[0];
								int originalMethodSize = groundedMethod.groundedPreconditions.size();
								for (size_t i = 1; i < unitGroundedMethod.groundedPreconditions.size(); i++){
									for (auto order : orderPertainingToThisTask)
										if (order.first == subTaskIdx)
											liftedMethod.orderingConstraints.push_back(std::make_pair(groundedMethod.groundedPreconditions.size(), order.second));
										else 
											liftedMethod.orderingConstraints.push_back(std::make_pair(order.first, groundedMethod.groundedPreconditions.size())); 
									
									groundedMethod.groundedPreconditions.push_back(unitGroundedMethod.groundedPreconditions[i]);
									liftedMethod.subtasks.push_back(liftedMethod.subtasks[subTaskIdx]);
									// add to the name of the method what we have done
								}

								// update the ordering
								std::vector<int> newOrdering;
								for (size_t i = 0; i < groundedMethod.preconditionOrdering.size(); i++){
									if (groundedMethod.preconditionOrdering[i] == subTaskIdx){
										positionOfExpanded = i;
										// insert the ordering of the unit method here
									
										for (size_t j = 0; j < unitGroundedMethod.preconditionOrdering.size(); j++){
											int unitTaskID = unitGroundedMethod.preconditionOrdering[j];
											if (unitTaskID == 0){
												// the re-used task
												newOrdering.push_back(groundedMethod.preconditionOrdering[i]);
											} else {
												newOrdering.push_back(groundedMethod.preconditionOrdering.size() + unitTaskID - 1);
											}
											idmapping.push_back(-j-1);
										}

										//
									} else {
										newOrdering.push_back(groundedMethod.preconditionOrdering[i]);
										idmapping.push_back(i);
									}
								}
								groundedMethod.preconditionOrdering = newOrdering;


								for (auto order : unitLiftedMethod.orderingConstraints) {
									if (order.first == 0) // the replaced task
										liftedMethod.orderingConstraints.push_back(std::make_pair(subTaskIdx, order.second - 1 + originalMethodSize));
									else if (order.second == 0) // the replaced task
										liftedMethod.orderingConstraints.push_back(std::make_pair(order.first - 1 + originalMethodSize, subTaskIdx)); 
									else
										liftedMethod.orderingConstraints.push_back(std::make_pair(order.first - 1 + originalMethodSize, order.second - 1 + originalMethodSize)); 
								}
	
	
								// update table accordingly as new tasks are now contained in this method 
								for (size_t i = 0; i < unitGroundedMethod.groundedPreconditions.size(); i++){
									taskToMethodsTheyAreContainedIn[unitGroundedMethod.groundedPreconditions[i]].insert(groundedMethod.groundedNo);
								}
							}

							// create the correct name of the new method so that the plan extractor can extract the correct
							liftedMethod.name = "<" + liftedMethod.name + ";" + decomposedTaskName + ";" + unitLiftedMethod.name + ";";
							liftedMethod.name += std::to_string(positionOfExpanded) + ";";
							for (size_t i = 0; i < idmapping.size(); i++){
								if (i) liftedMethod.name += ",";
								liftedMethod.name += std::to_string(idmapping[i]);
							}
							liftedMethod.name += ">";
						}
					}
	
					if (!found)
						break;
				}
				// write back the new method, i.e. add the lifted version to the domain
				// the grounded one is a reference, so it does not need to be written back
				groundedMethod.methodNo = domain.decompositionMethods.size();
				const_cast<Domain &>(domain).decompositionMethods.push_back(liftedMethod);
			}	
		}
	}
}

void removeEmptyMethodPreconditions(const Domain & domain,
		std::vector<bool> & prunedFacts,
		std::vector<bool> & prunedTasks,
		std::vector<bool> & prunedMethods,
		std::vector<GroundedTask> & inputTasksGroundedPg,
		std::vector<GroundedMethod> & inputMethodsGroundedTdg){

	std::vector<std::set<int>> taskToMethodsTheyAreContainedIn (inputTasksGroundedPg.size());
	for (auto & method : inputMethodsGroundedTdg){
		if (prunedMethods[method.groundedNo]) continue;
		for (size_t subTaskIdx = 0; subTaskIdx < method.groundedPreconditions.size(); subTaskIdx++)
			taskToMethodsTheyAreContainedIn[method.groundedPreconditions[subTaskIdx]].insert(method.groundedNo);
	}


	// find method precondition actions that have no unpruned preconditions
	std::vector<bool> prunableMethodPrecondition(inputMethodsGroundedTdg.size());
	for (GroundedTask & task : inputTasksGroundedPg){
		if (task.taskNo >= domain.nPrimitiveTasks || prunedTasks[task.groundedNo]) continue;
		if (domain.tasks[task.taskNo].name.rfind("method_precondition_",0) != 0) continue;
		bool unprunedPrecondition = false;
		for (int & pre : task.groundedPreconditions)
			if (!prunedFacts[pre]) { unprunedPrecondition = true; break; }
		if (unprunedPrecondition) continue;
	
		// this task is now pruned
		prunedTasks[task.groundedNo] = true;

		// so remove it from all methods this task is contained in	
		for (const int & method : taskToMethodsTheyAreContainedIn[task.groundedNo]){
			if (prunedMethods[method]) continue;
			DEBUG( std::cerr << "removing task " << task.groundedNo << " from method " << method << std::endl);
			// copy the lifted method
			GroundedMethod & groundedMethod = inputMethodsGroundedTdg[method];
			DecompositionMethod liftedMethod = domain.decompositionMethods[groundedMethod.methodNo];
			
			for (size_t subTaskIdx = 0; subTaskIdx < liftedMethod.subtasks.size(); subTaskIdx++){
				if (groundedMethod.groundedPreconditions[subTaskIdx] != task.groundedNo) continue; 

				std::vector<std::pair<int,int>> orderPertainingToThisTask;
				std::vector<std::pair<int,int>> orderNotPertainingToThisTask;
				for(auto order : liftedMethod.orderingConstraints)
					if (order.first == subTaskIdx || order.second == subTaskIdx)
						orderPertainingToThisTask.push_back(order);
					else 
						orderNotPertainingToThisTask.push_back(order);

				// if the method we have to apply is empty
				groundedMethod.groundedPreconditions.erase(groundedMethod.groundedPreconditions.begin() + subTaskIdx);
				liftedMethod.subtasks.erase(liftedMethod.subtasks.begin() + subTaskIdx);
				for (auto a : orderPertainingToThisTask) {
					if (a.second != subTaskIdx) continue;
					for (auto b : orderPertainingToThisTask) {
						if (b.first != subTaskIdx) continue;
						orderNotPertainingToThisTask.push_back(std::make_pair(a.first,b.second));
					}
				}
				liftedMethod.orderingConstraints.clear();
				for (auto order : orderNotPertainingToThisTask){
					if (order.first > subTaskIdx) order.first--;
					if (order.second > subTaskIdx) order.second--;
					liftedMethod.orderingConstraints.push_back(order);
				}
			}

			// write back the new method, i.e. add the lifted version to the domain
			// the grounded one is a reference, so it does not need to be written back
			groundedMethod.methodNo = domain.decompositionMethods.size();
			const_cast<Domain &>(domain).decompositionMethods.push_back(liftedMethod);
		}
	}
}




void postprocess_grounding(const Domain & domain, const Problem & problem,
		std::vector<Fact> & reachableFacts,
		std::vector<GroundedTask> & reachableTasks,
		std::vector<GroundedMethod> & reachableMethods,
		std::vector<bool> & prunedFacts,
		std::vector<bool> & prunedTasks,
		std::vector<bool> & prunedMethods,
		bool removeUselessPredicates,
		bool expandChoicelessAbstractTasks,
		bool pruneEmptyMethodPreconditions,
		bool quietMode	
		){
	// sort the subtasks of each method topologically s.t. 
	sortSubtasksOfMethodsTopologically(domain, prunedTasks, prunedMethods, reachableMethods);
	applyEffectPriority(domain, prunedTasks, reachableTasks, reachableFacts);
		
	
	if (!quietMode) std::cerr << "Simplifying instance." << std::endl;
	if (removeUselessPredicates) {
		if (!quietMode) std::cerr << "Removing useless facts/literals" << std::endl;
		removeUnnecessaryFacts(domain, problem, prunedTasks, prunedFacts, reachableTasks, reachableFacts);
	}
	if (expandChoicelessAbstractTasks){
		if (!quietMode) std::cerr << "Expanding abstract tasks with only one method" << std::endl;
		expandAbstractTasksWithSingleMethod(domain, problem, prunedTasks, prunedMethods, reachableTasks, reachableMethods);
	}
	if (pruneEmptyMethodPreconditions){
		if (!quietMode) std::cerr << "Removing method precondition actions whose precondition is empty" << std::endl;
		removeEmptyMethodPreconditions(domain,prunedFacts,prunedTasks,prunedMethods,reachableTasks,reachableMethods);
	}
	
}