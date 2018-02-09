#include "multiProc.h"

	int
main(int argc, char *argv[])
{
	readTasks(argv[1]);
	if (!checkProcessor()) {
		cout << "More number of processors/cores are required to execute the task-set.\n";
		exit(0);
	}
	cout << "Processors required: " << procReqd << "\n";
	allocateTasks();
	for (int i = 0; i < procReqd; i++) {
		decProc.insert(decProc.begin() + i, 1);
		decProcNext.insert(decProcNext.begin() + i, 1);
	}
	thread procThread[procReqd + 1];
	for (int i = 0; i < procReqd; i++) {
		procThread[i] = thread(executeEdf, i);
	}
	procThread[procReqd] = thread(generateLoJobs);

	for (int i = 0; i <= procReqd; i++) {
		procThread[i].join();
	}
	return 0;
}
