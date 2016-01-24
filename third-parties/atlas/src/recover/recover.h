#ifndef _RECOVER_H
#define _RECOVER_H

#include <map>

using namespace std;

// There is a one-to-one mapping between a release and an acquire log
// entry, unless the associated lock is a rwlock
typedef multimap<LogEntry*, pair<LogEntry*,int /* thread id */> > MapR2A;
typedef MapR2A::const_iterator R2AIter;
typedef map<int /* tid */, LogEntry* /* last log replayed */> Tid2Log;
typedef map<LogEntry*, bool> MapLog2Bool;
typedef map<uint32_t, bool> MapInt2Bool;
typedef map<LogEntry*, LogEntry*> MapLog2Log;

void R_Initialize(const char *name);
void R_Finalize(const char *name);
LogStructure *GetLogStructureHeader();
void CreateRelToAcqMappings(LogStructure*);
void AddToMap(LogEntry*,int);
void Recover();
void Recover(int);
void MarkReplayed(LogEntry*);
bool isAlreadyReplayed(LogEntry*);

#endif
