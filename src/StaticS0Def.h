//
// Created by dh on 12.11.21.
//

#ifndef SRC_STATICS0DEF_H
#define SRC_STATICS0DEF_H


class StaticS0Def {
public:
    int predicateNo;
    int numEntries;
    int sizeOfEntry;
    int* entries;
    int* sortedBy;
    int get(int iEntry, int iArg);
    void set(int iEntry, int iArg, int val);

};


#endif //SRC_STATICS0DEF_H
