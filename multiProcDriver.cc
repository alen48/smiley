#include "multiProc.h"

int maxNoOfProcessors;
int procReqd;
int noOfTasks;
int sysCriticality;
int maxCriticality;
int loCriticality = 1;
int hyperPeriod;
std::vector<int> processorTasks;
std::vector<int> periods;
int noOfLoTasks;
std::multimap<pair<double, double>, Task> orderedHiTasks;
std::vector<Task> LoTasks;
Task *LoTaskset;
std::priority_queue<Job> LoJobQ;
double globalNextDecTime;
int procExeCount = 0;
int procInserted = 0;
std::mutex procMutex;
std::mutex decMutex;
std::mutex loQMutex;
std::mutex insertMutex;
std::mutex loQAddMutex;
std::mutex slackMutex;
std::mutex slackFinishMutex;
std::mutex minSlackMutex1;
std::mutex minSlackMutex2;
std::mutex printMutex;
int loopCount1 = 0;
int loopCount2 = 0;
std::vector<int> decProc;
std::vector<int> decProcNext;
int decProcCount = 0;
int decProcCountNext = 0;
int procSlacked = 0;
int slackFinished = 0;
int minSlackCount1 = 0;
int minSlackCount2 = 0;
double minSlack = 0.0;
int minSlackProc;


		bool operator 
<(const Job &a, const Job &b)
{
		if (a.deadline != b.deadline)
				return (a.deadline > b.deadline);
		return (a.arrivalT > b.arrivalT);
}

		int
lcm(int a, int b)
{
		int temp = a;
		while (1)
		{
				if (temp % b == 0 && temp % a == 0)
						break;
				temp++;
		}
		return temp;
}

		void
calcHyperPeriod()
{
		int i = 1;
		hyperPeriod = periods[0];
		while (i < noOfTasks) {
				hyperPeriod = lcm(periods[i++], hyperPeriod);
		}
		return;
}

		void
readTasks(char *inFile)
{
		//char *dir = "test/";
		//strcat(dir, inFile);
		fstream fileOp(inFile, ios_base::in);
		fileOp >> maxNoOfProcessors >> maxCriticality >> noOfTasks;
		if (maxNoOfProcessors <= 0 || maxCriticality <= 0 || noOfTasks <= 0) {
				std::cout << "Invalid inputs.\n";
				exit(0);
		}

		sysCriticality = maxCriticality;
		int i = 0;
		noOfLoTasks = 0;
		while (i < noOfTasks) {
				Task taskset;
				taskset.taskNo = i;
				fileOp >> taskset.arrivalT >> taskset.period >> taskset.taskCriticality;
				if (taskset.arrivalT < 0 || taskset.period < 0 || taskset.taskCriticality < 0) {
						std::cout << "Invalid inputs.\n";
						exit(0);
				}
				periods.push_back(taskset.period);
				for (int j = 0; j < maxCriticality; j++) {
						double temp;
						fileOp >> temp;
						if (temp < 0) {
								std::cout << "Invalid inputs.\n";
								exit(0);
						}
						(taskset.execT).push_back(temp);
				}	
				fileOp >> taskset.deadline;
				taskset.processor = 0;
				taskset.taskUtil = taskset.execT[maxCriticality - 1] / taskset.period;
				if (taskset.taskCriticality >= sysCriticality) {
						orderedHiTasks.insert(make_pair(make_pair(taskset.period, taskset.arrivalT), taskset));
				} else {
						LoTasks.push_back(taskset);
						noOfLoTasks++;
				}
				i++;
		}
		fileOp.close();
		calcHyperPeriod();	
		return;
}

		int
checkProcessor(void)
{
		double U[maxNoOfProcessors];
		int K[maxNoOfProcessors];
		U[0] = K[0] = 0;
		int q = 0;
		int j;
		for (std::multimap<pair<double, double>, Task>::iterator it = orderedHiTasks.begin(); it != orderedHiTasks.end() ; it++) {
				double tempUtil = (it->second).taskUtil;
				for (j = 0; j <= q; j++) {
						if (U[j] + tempUtil <= 1) {
								U[j] += tempUtil;
								K[j] += 1;
								(it->second).processor = j;
								break;
						}
				}
				if (j > q) {
						if (j + 1 > maxNoOfProcessors) {
								return 0;
						}
						q++;
						U[q] = tempUtil;
						K[q] = 1;
						(it->second).processor = q;
				}
		}
		for (int i = 0; i <= q; i++) {
				processorTasks.push_back(K[i]);
		}
		return procReqd = q + 1;
}

		void
allocateTasks(void)
{
		int i = 0;
		while (i < procReqd) {
				ofstream outFile;
				std::string fileName("inputP");
				std::string buffer = to_string(i);
				fileName = fileName + buffer;

				outFile.open(fileName.c_str(), ios_base::trunc);
				outFile << maxCriticality << " " << processorTasks[i];
				for (std::multimap<pair<double, double>, Task>::iterator it = orderedHiTasks.begin(); it != orderedHiTasks.end() ; it++) {
						if ((it->second).processor == i) {
								outFile << "\n" << (it->second).arrivalT << "\t" << (it->second).period << "\t" << (it->second).taskCriticality << "\t";
								for (int j = 0; j < maxCriticality; j++) {
										outFile << (it->second).execT[j] << "\t";
								}
								outFile << (it->second).deadline;
						}
				}
				outFile.close();
				i++;
		}
		ofstream outFile;
		outFile.open("inputG", ios_base::trunc);
		outFile << maxCriticality << " " <<  noOfLoTasks;
		for (std::vector<Task>::iterator it = LoTasks.begin(); it != LoTasks.end() ; it++) {
				outFile << "\n" << it->arrivalT << "\t" << it->period << "\t" << it->taskCriticality << "\t";
				for (int j = 0; j < maxCriticality; j++) {
						outFile << it->execT[j] << "\t";
				}
				outFile << it->deadline;
		}
		outFile.close();

		return;
}

		void
removeExpJobs(double currTime)
{
		while (!LoJobQ.empty()) {
				if (currTime >= LoJobQ.top().deadline) {
						LoJobQ.pop();
				} else {
						break;
				}
		}
}

		void 
populateLoTasks(void)
{
		int noOfLoTasksTemp, maxCriticalityTemp;
		int i = 0;
		ifstream fileOp;
		fileOp.open("inputG", ios_base::in);
		fileOp >> maxCriticalityTemp >> noOfLoTasksTemp;

		LoTaskset = new Task[noOfLoTasks];

		while (i < noOfLoTasks)
		{
				LoTaskset[i].taskNo = i;
				fileOp >> LoTaskset[i].arrivalT >> LoTaskset[i].period >> LoTaskset[i].taskCriticality;
				for (int j = 0; j < maxCriticality; j++) {
						double temp;
						fileOp >> temp;
						(LoTaskset[i].execT).push_back(temp);
				}
				fileOp >> LoTaskset[i].deadline;
				LoTaskset[i].taskUtil = LoTaskset[i].execT[maxCriticality - 1] / LoTaskset[i].period;
				LoTaskset[i].processor = -1;
				i++;
		}
		fileOp.close();
		return;
}

		void 
generateLoJobs(void)
{
		double currTime = 0.0;
		int i = 1;
		populateLoTasks();
		double nextArrival, nextDecTime;
		while (currTime < hyperPeriod) {
				procMutex.lock();
				loopCount1++;
				procMutex.unlock();
				while (loopCount1 < (procReqd + 1) * i);
				removeExpJobs(currTime);
				procMutex.lock();
				loopCount2++;
				globalNextDecTime = hyperPeriod;
				procMutex.unlock();
				while (loopCount2 < (procReqd + 1) * i);

				nextArrival = hyperPeriod;
				for (int j = 0; j < noOfLoTasks; j++) {
						if ((int)(currTime - LoTaskset[j].arrivalT) % LoTaskset[j].period == 0 && LoTaskset[j].arrivalT <= currTime) {
								Job newJob;
								newJob.taskNo = LoTaskset[j].taskNo;
								newJob.jobNo = (currTime - LoTaskset[j].arrivalT) / LoTaskset[j].period;
								newJob.arrivalT = currTime;
								newJob.remainingT = LoTaskset[j].execT[maxCriticality - 1];
								newJob.deadline = currTime + LoTaskset[j].deadline;	
								newJob.jobCriticality = LoTaskset[j].taskCriticality;		
								newJob.jobUtil = LoTaskset[j].taskUtil;
								LoJobQ.push(newJob);
						}
						double calc = LoTaskset[j].period * (floor((currTime - LoTaskset[j].arrivalT) / LoTaskset[j].period) + 1);
						if (calc < nextArrival) {
								nextArrival = calc;
						}
				}
				nextDecTime = nextArrival;
				insertMutex.lock();
				procInserted++;
				insertMutex.unlock();
				while (procInserted < (procReqd + 1) * i);

				decMutex.lock();
				globalNextDecTime = min(globalNextDecTime, nextDecTime);
				procExeCount++;
				decMutex.unlock();
				while (procExeCount < (procReqd + 1) * i); 
				i++;
				currTime = globalNextDecTime;
		}
}

		double
calcSlack(Task *taskset, int noOfEdfTasks, double currTime, Job loQTop, std::priority_queue<Job> readyQ, int procNo) 
{
		double slack = 0.0;
		double maxDeadline = 0.0;
		double utilisation = 0.0;
		for (int i = 0; i < noOfEdfTasks; i++) {
				utilisation += taskset[i].taskUtil;
		}   

		std::vector<Job> type1;
		// printMutex.lock();
		// std::cout << "\nRQ: ";
		while (!readyQ.empty()) {	
				type1.push_back(readyQ.top());
				maxDeadline = max((readyQ.top()).deadline, maxDeadline);
				// if (readyQ.top().jobCriticality < sysCriticality)
				// 	std::cout << "LO_";
				// std::cout << readyQ.top().taskNo << readyQ.top().jobNo << " ";
				readyQ.pop();
		}
		//	std::cout << "\n";
		for(int i = 0; i < noOfEdfTasks; i++) {
				if(taskset[i].taskCriticality >= sysCriticality){
						double nextArrival = taskset[i].period * (floor((currTime - taskset[i].arrivalT) / taskset[i].period) + 1);

						while(nextArrival <= loQTop.deadline) {
								double currDeadline = nextArrival + taskset[i].deadline;
								Job newJob;
								newJob.taskNo = taskset[i].taskNo;
								newJob.jobNo = (nextArrival - taskset[i].arrivalT) / taskset[i].period;
								newJob.arrivalT = nextArrival;
								newJob.remainingT = taskset[i].execT[maxCriticality - 1];
								newJob.deadline = currDeadline;	
								newJob.jobCriticality = taskset[i].taskCriticality;		
								newJob.jobUtil = taskset[i].taskUtil;

								if(newJob.deadline > maxDeadline && nextArrival < loQTop.deadline) {
										maxDeadline = newJob.deadline;
								}

								type1.push_back(newJob);  
								nextArrival = nextArrival + taskset[i].period;

						}
				}
		}


		for (int i = 0; i < noOfEdfTasks; i++) {
				if (taskset[i].taskCriticality >= sysCriticality) {
						double nextArrival = (floor(loQTop.deadline / taskset[i].period) + 1) * taskset[i].period;
						while (nextArrival < maxDeadline) {
								double currDeadline = nextArrival + taskset[i].deadline;
								Job newJob;
								newJob.taskNo = taskset[i].taskNo;
								newJob.jobNo = (nextArrival - taskset[i].arrivalT) / taskset[i].period;
								newJob.arrivalT = nextArrival;
								newJob.remainingT = taskset[i].execT[maxCriticality - 1];
								newJob.deadline = currDeadline;	
								newJob.jobCriticality = taskset[i].taskCriticality;		
								newJob.jobUtil = taskset[i].taskUtil;

								type1.push_back(newJob);

								nextArrival = nextArrival + taskset[i].period;			
						}
				}
		}

		// printMutex.lock();
		// std::cout << "current time: " << currTime << "; loDeadline: " << loQTop.deadline << "; maxD: " << maxDeadline << "; incoming task : J_LO_" << loQTop.taskNo << loQTop.jobNo << "\n";
		// for (std::vector<Job>::iterator i = type1.begin(); i != type1.end(); i++) {
		// 	if (i->jobCriticality < sysCriticality)
		// 		std::cout << "LO_";
		// 	std::cout << i->taskNo << i->jobNo << "\n";
		// }
		// printMutex.unlock();

		sort(type1.begin(), type1.end(),  greater_than_key());

		double dynamicT = maxDeadline;

		// std::cout << "\nincomplete: ";
		for(int i = 0; i < (int) type1.size(); i++) {

				Job temp = type1[i];
				if (temp.deadline > dynamicT) {
						if (temp.deadline > maxDeadline) {
								dynamicT = dynamicT - temp.remainingT * (maxDeadline - temp.arrivalT) * utilisation / (temp.deadline - temp.arrivalT);
								// std::cout << temp.taskNo << temp.jobNo << " " << temp.remainingT * (maxDeadline - temp.arrivalT) * utilisation / (temp.deadline - temp.arrivalT) << "; ";
						} else{
								dynamicT = dynamicT - temp.remainingT;
						}
				} else {
						if (dynamicT < loQTop.deadline) {
								slack = slack + dynamicT - temp.deadline;
						}
						dynamicT = temp.deadline - temp.remainingT;	
				}
				// std::cout << "\ninter slack: " << slack << " " << "dynamicT: " << dynamicT;
		}
		if (dynamicT > loQTop.deadline) {
				dynamicT = loQTop.deadline;
				// std::cout << "\ndynamicT: " << dynamicT;
		}
		slack = slack + dynamicT - currTime;
		printMutex.lock();
		std::cout << "\nslack is : " << slack << "\n";
		std::cout << "-----------------\n";
		printMutex.unlock();
		return slack;
}

		void
executeEdf(int procNo)
{	
		int noOfEdfTasks, maxCriticalityTemp;
		int i = 0;
		double currTime = 0.0;
		double totUtil = 0.0;

		std::string fileName("inputP");
		std::string outputFile("outputP");	
		std::string buffer = to_string(procNo);	
		fileName = fileName + buffer;
		outputFile = outputFile + buffer;

		ifstream fileOp;
		fileOp.open(fileName.c_str(), ios_base::in);	
		fileOp >> maxCriticalityTemp >> noOfEdfTasks;

		Task *taskset = new Task[noOfEdfTasks];

		while (i < noOfEdfTasks) {
				taskset[i].taskNo = i;
				fileOp >> taskset[i].arrivalT >> taskset[i].period >> taskset[i].taskCriticality;
				for (int j = 0; j < maxCriticality; j++) {
						double temp;
						fileOp >> temp;
						(taskset[i].execT).push_back(temp);
				}
				fileOp >> taskset[i].deadline;
				taskset[i].taskUtil = taskset[i].execT[maxCriticality - 1] / taskset[i].period;
				taskset[i].processor = procNo;
				i++;
		}
		fileOp.close();

		ofstream fileOut;
		fileOut.open(outputFile.c_str(), ios_base::trunc);	

		i = 0;
		while (i < noOfEdfTasks) {
				totUtil += taskset[i++].taskUtil;
		}
		if (totUtil > 1) {
				fileOut << "Unfeasible schedule as total utilization is greater than 1 on processor " << procNo <<".\n";
				exit(0);
		}

		fileOut << "Processor " << procNo << "\n-----------\n\n";
		fileOut << "Utilization : " << totUtil << "\nHyperperiod : " << hyperPeriod << "\n";
		fileOut << "\nThe EDF schedule till hyperperiod is as follows:\n\n";

		i = 1;
		double nextArrival, completionTime = 0.0, nextDecTime;
		std::priority_queue<Job> readyQ;

		while (currTime < hyperPeriod) {

				procMutex.lock();
				loopCount1++;
				procMutex.unlock();
				while (loopCount1 < (procReqd + 1) * i);
				procMutex.lock();
				loopCount2++;
				globalNextDecTime = hyperPeriod;
				procMutex.unlock();
				while (loopCount2 < (procReqd + 1) * i);

				nextArrival = hyperPeriod;
				for (int j = 0; j < noOfEdfTasks; j++) {
						if ((int)(currTime - taskset[j].arrivalT) % taskset[j].period == 0 && taskset[j].arrivalT <= currTime) {
								Job newJob;
								newJob.taskNo = taskset[j].taskNo;
								newJob.jobNo = (currTime - taskset[j].arrivalT) / taskset[j].period;
								newJob.arrivalT = currTime;
								newJob.remainingT = taskset[j].execT[maxCriticality - 1];
								newJob.deadline = currTime + taskset[j].deadline;	
								newJob.jobCriticality = maxCriticality;		
								newJob.jobUtil = taskset[j].taskUtil;
								readyQ.push(newJob);
						}
						double calc = taskset[j].period * (floor ((currTime - taskset[j].arrivalT) / taskset[j].period) + 1);
						if (calc < nextArrival) {
								nextArrival = calc;
						}
				}


				insertMutex.lock();
				procInserted++;
				minSlack = hyperPeriod;
				insertMutex.unlock();
				while (procInserted < (procReqd + 1) * i);

				while (!LoJobQ.empty()) {
						double localSlack = 0.0;
						printMutex.lock();
						printMutex.unlock();
						localSlack = calcSlack(taskset, noOfEdfTasks, currTime, LoJobQ.top(), readyQ, procNo);
						slackMutex.lock();
						if ((LoJobQ.top()).remainingT <= localSlack && localSlack < minSlack) {
								minSlack = min(minSlack, localSlack);
								minSlackProc = procNo;
						}
						procSlacked++;
						slackMutex.unlock();
						while (procSlacked % procReqd != 0);
						if (minSlack == hyperPeriod && procNo == 0) {
								LoJobQ.pop();
						} else if (minSlack >= (LoJobQ.top()).remainingT && minSlackProc == procNo) {
								readyQ.push(LoJobQ.top());
								LoJobQ.pop();
						}

						minSlackMutex1.lock();
						minSlackCount1++;
						minSlackMutex1.unlock();
						while (minSlackCount1 % procReqd != 0);

						minSlackMutex2.lock();
						if (!LoJobQ.empty()) {
								minSlack = hyperPeriod;
						}
						minSlackCount2++;
						minSlackMutex2.unlock();

						while (minSlackCount2 % procReqd != 0);
				}
				slackFinishMutex.lock();
				slackFinished++;
				slackFinishMutex.unlock();
				while (slackFinished < procReqd * i);

				if (!readyQ.empty()) {
						completionTime = currTime + (readyQ.top()).remainingT;
						nextDecTime = min(nextArrival, completionTime);
				} else {
						nextDecTime = nextArrival;
				}

				decMutex.lock();
				globalNextDecTime = min(globalNextDecTime, nextDecTime);
				procExeCount++;
				decMutex.unlock();

				while (procExeCount < (procReqd + 1) * i);

				if (!readyQ.empty()) {
						Job temp;
						temp = readyQ.top();
						readyQ.pop();
						if (temp.jobCriticality < sysCriticality) {
								if ((temp.remainingT = currTime + temp.remainingT - globalNextDecTime) <= 0) {  
										fileOut << "\nJ_LO_" << temp.taskNo << "," << temp.jobNo << "\t" << currTime << " to " << globalNextDecTime;
								} else {
										fileOut << "\nJ*_LO_" << temp.taskNo << "," << temp.jobNo << "\t" << currTime << " to " << globalNextDecTime;
										readyQ.push(temp);
								}
						} else {
								if ((temp.remainingT = currTime + temp.remainingT - globalNextDecTime) == 0) {  
										fileOut << "\nJ_" << temp.taskNo << "," << temp.jobNo << "\t\t" << currTime << " to " << globalNextDecTime;
								} else {
										fileOut << "\nJ*_" << temp.taskNo << "," << temp.jobNo << "\t\t" << currTime << " to " << globalNextDecTime;
										readyQ.push(temp);
								}
						}
				} else {
						fileOut << "\nIdle" << "\t\t" << currTime << " to " << globalNextDecTime;
				}

				i++;
				currTime = globalNextDecTime;
		}
		fileOut << "\n";
		fileOut.close();
		return;
}
