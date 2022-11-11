# FAM-based LMs for Lifted  Classical Planning
This is the code to generate FAM-based landmarks based on a lifted classical planning model.

The code is based on the execution stack of pandaPI. The LM generation code in this directory is based on the pandaPI's grounder (though we do not ground).



## Building
To compile the code, you need to perform the following steps:

 - check out
 - git submodule init
 - git submodule update
 - cd cppdl
 - make boruvka opts bliss
	- if you have lpsolve installed, you may also need to ```make lpsolve```
 - make
 - cd ../src
 - make

Further, you need the pandaPI parser.

## Command line options

If you start with a PDDL domain file and a PDDL problem file, you first need to call the parser, e.g.:

```
pandaPIparser domain.pddl problem.pddl temp.parsed
```

Then, call the given code with the following arguments:

```
lmgen -q -2iDE temp.parsed
```

The generator will write a file called ```LMs.txt``` containing the landmarks.
