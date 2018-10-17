#include "syscall.h"
#include "types.h"
#include "user.h"
#include "pstat.h"
#include "param.h"

int main(int argc, char *argv[]){
	settickets(10);
	fork();
	if(fork() == 0)
		settickets(5);
	else
		settickets(7);

	struct pstat stats;
        getpinfo(&stats);

	int i;
	for(i = 0; i < NPROC; i++){
		if(stats.inuse[i] == 1)
			printf(1, "[PID: %d] [TICKETS: %d] [TICKS: %d]\n", stats.pid[i], stats.tickets[i], stats.ticks[i]);
	}
/*	
	if(fork() == 0)
		printf(1, "Child 2");
	else
		printf(1, "Parent 2");
	
	if(fork() == 0)
		printf(1, "Child 3");
	else
		printf(1, "Parent 3");
*/	//fork();
	//fork();
	//settickets(4);
	//fork();
	exit();
}
