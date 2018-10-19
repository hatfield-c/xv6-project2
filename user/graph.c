#include "syscall.h"
#include "types.h"
#include "user.h"
#include "pstat.h"
#include "param.h"

void spin(int);
void stress(int);

// Local struct to hold process statistics
struct lstats {
	int tickets;
	int pid;
	int ticks;
	int tickShare;
	int ticketShare;
};

int main(int argc, char *argv[]){
        // Set the parent to a high number of tickets, so that it can finish creating its children
	// before it waits
	settickets(100000);
	
	// Ticket values, and pipes for communicating with the parent
	int tickets[] = {100, 200, 300};
	int pip1[2], pip2[2], pip3[2];
	int* pipes[3] = { pip1, pip2, pip3};

        // Process statistics structures
	struct pstat stats;
	struct lstats runstats[3];

        // Parent sets up its pipes
	pipe(pip1);
	pipe(pip2);
	pipe(pip3);

	// Parent generates children to gather stats on
	int i;
	for(i = 0; i < 3; i++){
		// If not a child, then continue generating children
		if(fork() == 0){
			// If a child, then set your tickets appropriately
			settickets(tickets[i]);

			// Spin/stress for a large amount of cycles
			stress(90000000);

			// Each child process gets its own stats
			if(getpinfo(&stats) < 0)
				exit();

			// Each child build a local stats structure, and sends only its own stats to the parent
			int k, spinPID = getpid();
			for(k = 0; k < NPROC; k++){
			        // If the child has found its stats to send
				if(spinPID == stats.pid[k]){
					// Build the stats structure
					runstats[i].pid = stats.pid[k];
					runstats[i].tickets = stats.tickets[k];
					runstats[i].ticks = stats.ticks[k];

					// Each child sets up its associated pipes
					int pipfd[] = { *pipes[i], *(pipes[i] + 1) };
					close(pipfd[0]);

					// The child tries to write to their pipes, if an error, exit
					if(write(pipfd[1], (void*)&runstats[i], sizeof(runstats[i])) < 0)
						exit();
				}
			}
			// The child is done - it can exit now.
			exit();
		}
	}

	//The parent sets its tickets to 1, so as not to draw consumption from children
	settickets(1);

	// Wait for all three children to finish
	wait();
	wait();
	wait();
	
	// The children are done - the parent reads the data they've sent through the pipe,
	// storing it into a complete stats structure
	for(i = 0; i < 3; i++){
		int pipfd[] = { *pipes[i], *(pipes[i] + 1) };
		close(pipfd[1]);

		if(read(pipfd[0], (void*)&runstats[i], sizeof(runstats[i])) < 0)
			exit();
	}
	
	// Find the total number of ticks
	int totalTicks = runstats[0].ticks + runstats[1].ticks + runstats[2].ticks;

	// Calculate the "share" of ticks each child received, scaled to a factor of about 36 to make
	// output easier
	for(i = 0; i < 3; i++){
		runstats[i].tickShare = (36 * runstats[i].ticks) / totalTicks;
		runstats[i].tickShare = runstats[i].tickShare < 1 ? 1 : runstats[i].tickShare;
	}
	
	// Calculate the total scaled share of ticks, and output the results to the console
	int allShare = runstats[0].tickShare + runstats[1].tickShare + runstats[2].tickShare;
	printf(1, "\n>======================== Lottery Schedule Graph ========================<\n");
	for(i = 0; i < 3; i++){
		// The c-string to represent the individual bar graphs of each child
		char barGraph[37];

		// Build the child bar graphs, with each character representing a single "tick" share
		// it held
		int k;
		for(k = 0; k < runstats[i].tickShare; k++){
			barGraph[k] = '*';
		}

		// Build the child bar graphs, with each character representing a single "tick" share
		// it did not hold
		for(k = runstats[i].tickShare; k < 36; k++){
			barGraph[k] = '-';
		}

		// Null terminate
		barGraph[36] = '\0';

		// Output the data of each graph to the console
		printf(1, "[PID]:        %d\n", runstats[i].pid);
		printf(1, "[Ticks]:      %d           [%s]\n", runstats[i].ticks, barGraph);
		printf(1, "[Tickets]:    %d\n", runstats[i].tickets);
		printf(1, "[Tick Share]: %d/%d\n\n", runstats[i].tickShare, allShare);

	}

	// The c-string to hold the total graph of all the child processes
	char totGraph[allShare + 1];

	// Build the total graph, with each char representing a tick share of process 0 (100 tickets)
	for(i = 0; i < runstats[0].tickShare; i++){
		totGraph[i] = '&';
	}

	// Build the total graph, with each char representing a tick share of process 1 (200 tickets)
	for(i = runstats[0].tickShare; i < runstats[0].tickShare + runstats[1].tickShare; i++){
		totGraph[i] = '#';
	}

	// Build the total graph, with each char representing a tick share of process 2 (300 tickets)
	for(i = runstats[0].tickShare + runstats[1].tickShare; i < allShare; i++){
		totGraph[i] = '$';
	}

	// Null terminate
	totGraph[allShare] = '\0';

	// Output the graph to the screen, with a key for the tickets and their characters
	printf(1, "          < TOTAL >     [%s]\n", totGraph);
	printf(1, "           [100: &]\n           [200: #]\n           [300: $]\n\n");
	
	exit();
}

// Method to waste time on the cpu. Uses a volatile integer as the sink to increase time spent per
// operation
void spin(int cycles){
	int i;
	volatile int sink = 0;
	for(i = 0; i < cycles; i++){
		sink = sink + 1;
		sink = sink + 1;
		sink = sink + 1;
	}
}


// Shell method for spin, created in case of the need for additional logic
void stress(int cycles){
	spin(cycles);
}
