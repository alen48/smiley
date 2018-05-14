#ifndef MULTIPROC_H_
#define MULTIPROC_H_

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <thread>
#include <mutex>
#include <algorithm>
#include <vector>
#include <queue>
#include <map>
#include <string>

using namespace std;

class Task {
	public:
		int taskNo;
		double arrivalT;
		int period;
		int taskCriticality;
		std::vector <double> execT;
		double deadline;
		double taskUtil;
		int processor;
};


class Job {
	public:	
		int taskNo;
		int jobNo;
		double arrivalT;
		double remainingT;
		int jobCriticality;
		double deadline;
		double jobUtil;
		int currJob; // 1 if this job is currently being executed in a processor
};

class greater_than_key {
	public:
		inline bool operator()(const Job& J1, const Job& J2)
		{
			return (J1.deadline > J2.deadline);
		}
};

extern int maxNoOfProcessors;
extern int procReqd;
extern int noOfTasks;
extern int sysCriticality;
extern int hyperPeriod;
extern int maxCriticality;
extern int loCriticality;
extern std::vector<int> processorTasks;
extern std::multimap<pair<double, double>, Task> orderedHiTasks;
extern std::vector<Task> LoTasks;
extern Task *LoTaskset;
extern std::priority_queue<Job> LoJobQ;
extern double globalNextDecTime;
extern int procExeCount;
extern int procExeCountNext;
extern mutex procMutex;
extern mutex decMutex;
extern mutex loQMutex;
extern mutex loQAddMutex;
extern mutex slackMutex;
extern mutex insertMutex;
extern mutex slackFinishMutex;
extern mutex minSlackMutex1;
extern mutex minSlackMutex2;
extern mutex printMutex;
extern int loopCount1;
extern int loopCount2;
extern std::vector<int> decProc;
extern std::vector<int> decProcNext;
extern int decProcCount;
extern int decProcCountNext;
extern double minSlack;
extern int procInserted;
extern int slackFinished;
extern int minSlackCount1;
extern int minSlackCount2;


extern bool operator
<(const Job &, const Job &);

extern int
lcm(int, int);

extern int
calcHyperPeriod(Task, int);

extern void
readTasks(char *);

extern int
checkProcessor(void);

extern void
allocateTasks(void);

extern void
executeEdf(int);

extern void
populateLoTasks(void);

extern void
removeExpJobs(double);

extern void
generateLoJobs(void); 

extern double
calcSlack(Task *, int, double, Job, std::priority_queue<Job>, std::priority_queue<Job>, int);

#endif
