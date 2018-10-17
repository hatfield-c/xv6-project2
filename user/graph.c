#include "syscall.h"
#include "types.h"
#include "user.h"
#include "pstat.h"
#include "param.h"

void spin(int);
void stress(int);

struct lstats {
	int tickets;
	int pid;
	int ticks;
	int tickShare;
	int ticketShare;
};

int main(int argc, char *argv[]){
	settickets(100000);
	//struct pstat stats;
	//printf(1, "\n\n < PARENT ID: %d >\n", getpid());
	int graphPID = getpid();
	int tickets[] = {100, 200, 300};
	int pip1[2], pip2[2], pip3[2];
	int* pipes[3] = { pip1, pip2, pip3};

	struct pstat stats;
	struct lstats runstats[3];
	graphPID = graphPID + 0;
	//struct lstats* buf1, buf2, buf3;

	pipe(pip1);
	pipe(pip2);
	pipe(pip3);

	int i;
	for(i = 0; i < 3; i++){
		if(fork() == 0){
			//printf(1, "- CHILD ID: %d -\n", getpid());
			settickets(10);
			settickets(tickets[i]);
			stress(90000000);

			if(getpinfo(&stats) < 0)
				exit();

			int k, spinPID = getpid();
			for(k = 0; k < NPROC; k++){
				//if(stats.pid[k] !=  0)
				//printf(1, "[%d - %d]\n", stats.pid[k], stats.ticks[k]);
				if(spinPID == stats.pid[k]){
					runstats[i].pid = stats.pid[k];
					runstats[i].tickets = stats.tickets[k];
					runstats[i].ticks = stats.ticks[k];
					//printf(1, "[CHI - %d]", runstats[i].tickets);

					int pipfd[] = { *pipes[i], *(pipes[i] + 1) };
					close(pipfd[0]);
					if(write(pipfd[1], (void*)&runstats[i], sizeof(runstats[i])) < 0)
						exit();
				}
			}
			
			exit();
		}
	}
	settickets(1);
	wait();
	wait();
	wait();
	//exit();
	for(i = 0; i < 3; i++){
		int pipfd[] = { *pipes[i], *(pipes[i] + 1) };
		close(pipfd[1]);

		if(read(pipfd[0], (void*)&runstats[i], sizeof(runstats[i])) < 0)
			exit();
	}
	
	int totalTicks = runstats[0].ticks + runstats[1].ticks + runstats[2].ticks;

	for(i = 0; i < 3; i++){
		runstats[i].tickShare = (36 * runstats[i].ticks) / totalTicks;
		runstats[i].tickShare = runstats[i].tickShare < 1 ? 1 : runstats[i].tickShare;
	}
	
	int allShare = runstats[0].tickShare + runstats[1].tickShare + runstats[2].tickShare;
	printf(1, "\n>======================== Lottery Schedule Graph ========================<\n");
	for(i = 0; i < 3; i++){
		//printf(1, "[PAR: PID - %d, TICKETS - %d]", runstats[i].pid, runstats[i].tickets);
		char barGraph[37];

		int k;
		for(k = 0; k < runstats[i].tickShare; k++){
			barGraph[k] = '*';
		}

		for(k = runstats[i].tickShare; k < 36; k++){
			barGraph[k] = '-';
		}

		barGraph[36] = '\0';

		printf(1, "[PID]:        %d\n", runstats[i].pid);
		printf(1, "[Ticks]:      %d           [%s]\n", runstats[i].ticks, barGraph);
		printf(1, "[Tickets]:    %d\n", runstats[i].tickets);
		printf(1, "[Tick Share]: %d/%d\n\n", runstats[i].tickShare, allShare);

	}

	
	char totGraph[allShare + 1];

	for(i = 0; i < runstats[0].tickShare; i++){
		totGraph[i] = '&';
	}

	for(i = runstats[0].tickShare; i < runstats[0].tickShare + runstats[1].tickShare; i++){
		totGraph[i] = '#';
	}

	for(i = runstats[0].tickShare + runstats[1].tickShare; i < allShare; i++){
		totGraph[i] = '$';
	}

	totGraph[allShare] = '\0';
	printf(1, "          < TOTAL >     [%s]\n", totGraph);
	printf(1, "           [100: &]\n           [200: #]\n           [300: $]\n\n");
	
	exit();
}

void spin(int cycles){
	int i;
	volatile int sink = 0;
	for(i = 0; i < cycles; i++){
		sink = sink + 1;
		sink = sink + 1;
		sink = sink + 1;
	}
}


void stress(int cycles){
	spin(cycles);
}
