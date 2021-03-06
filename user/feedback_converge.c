#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#define check(exp, msg) if(exp) {} else {\
   printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
   exit();}
#define lowtickets 100
#define hightickets 1000
#define starttickets 100000000

void spin(int dur)
{
	int i = 0;
  int j = 0;
  int k = 0;
	for(i = 0; i < 50; ++i)
	{
		for(j = 0; j < dur; ++j)
		{
      k = j % 10;
      k = k + 1;
    }
	}
}

void sendParentReadySignal(int* chan, char* buffer, int numchd)
{
  // Write 1 byte for each child
  int i;
  for (i = 0; i < numchd; i++){
    buffer[i] = 1;
  }
  write(chan[1],buffer,numchd*sizeof(char));
}

int
main(int argc, char *argv[])
{
   int numprocs = 6; // Don't change
   int pid_par = getpid();
   check(settickets(starttickets) == 0, "settickets");
   int pid_chds[numprocs];
   char buffer[16]; // max(16,numprocs+1)
   int pipe_pend[2]; // Parent writes to signal all children to move to ready pipe
   int pipe_ready[2]; // Parent writes to signal all children to start spinning
   int pipe_reap[2]; // Parent writes to collect all children when they finish spinning
   int pipe_par[2]; // Parent writes the new ticket value to one child
   int pipe_chd[2]; // Child writes to notify parent is has set its new ticket value
   
   close(0);
   close(2);
   pipe(pipe_pend);
   pipe(pipe_ready);
   pipe(pipe_reap);
   pipe(pipe_par);
   pipe(pipe_chd);
   int i;
   for (i = 0; i < numprocs; i++){
     pid_chds[i] = fork();
     if (pid_chds[i] == 0){
       // Child
       // Set up pipes
       close(pipe_pend[1]);
       close(pipe_ready[1]);
       close(pipe_reap[1]);
       close(pipe_par[1]);
       close(pipe_chd[0]);
       
       int tickets_chd;
       if (i < numprocs/3 || i == 3) // Even out flaws in PRG
         tickets_chd = lowtickets-i*4;
       else
         tickets_chd = hightickets+i*4;
       for(;;){
         
         if (settickets(tickets_chd) != 0){
           printf(1,"check failed: settickets\n");
           // Send error signal
           write(pipe_chd[1],"b",sizeof(char));
           exit();
         }
         // Send ready signal
         write(pipe_chd[1],"a",sizeof(char));
         // Wait for ready signal to clear pending pipe
         read(pipe_pend[0],buffer,sizeof(char));
         // Send ready signal
         write(pipe_chd[1],"a",sizeof(char));
         // Wait for ready signal to begin spinning
         read(pipe_ready[0],buffer,sizeof(char));
         int l;
         for (l = 0; l < 10; l++){
           spin(21000);
           sleep(1);
         }
         
         // Acquire private channel with parent
         read(pipe_reap[0],buffer,sizeof(char));
         // Send child number to parent
         printf(pipe_chd[1],"%d\n",i+1);
         // Read new ticket value from parent
         read(pipe_par[0],buffer,16*sizeof(char));
         int readbuf = atoi(buffer);
         if (readbuf > 0){
           int newtickets = (int)((double)readbuf*tickets_chd/256);
           if (newtickets < 1)
             newtickets = 1;
           else if (newtickets > 10000000)
             newtickets = 10000000;
           tickets_chd = newtickets;
         }
       }
       
     }
	   else if (pid_chds[i] < 0){
       printf(1,"Fork failed\n");
	     goto Cleanup;
     }
   }
   // Parent
   // Set up pipes
   close(pipe_pend[0]);
   close(pipe_ready[0]);
   close(pipe_reap[0]);
   close(pipe_par[0]);
   close(pipe_chd[1]);
   // Prime the reap pipe
   write(pipe_reap[1],"a",sizeof(char));
   
   // Prepare children
   struct pstat before, after;
   for (i = 0; i < numprocs; i++){
     read(pipe_chd[0],buffer,sizeof(char));
     if (*buffer == 'b')
       goto Cleanup;
   }
   int goal = -1;
   
   // Begin monitoring children
   for(;;){
     int i;
     // All children have updated their tickets and are ready for the round. Move them to the ready pipe.
     sendParentReadySignal(pipe_pend,buffer,numprocs);
     // Wait for all children to receive ready signal to clear the pending pipe.
     for (i = 0; i < numprocs; i++){
       read(pipe_chd[0],buffer,sizeof(char));
     }
     // All children are on the ready pipe, one byte sits on the reap pipe
     if (getpinfo(&before) != 0){
       printf(1,"check failed: getpinfo\n");
       goto Cleanup;
     }
     sendParentReadySignal(pipe_ready,buffer,numprocs);
     
     int sum = 0, count = 0, var = -1;
     int avg;
     int ticks_chds[numprocs];
     int chdnum;
     for (chdnum = 0; chdnum < numprocs; chdnum++){ // Handle updating tickets for each child
  	   // Wait for child to finish and obtain its pid
       read(pipe_chd[0],buffer,16*sizeof(char));
       int id_chd = atoi(buffer);
       id_chd--;
       
       if (chdnum == 0){
         // First child to finish, run getpinfo
         if (getpinfo(&after) != 0){
           printf(1,"check failed: getpinfo\n");
           goto Cleanup;
         }
         // Obtain info on child processes
         int j;
         int k;
         for (i = 0; i < numprocs; i++){
           for (j = 0; j < NPROC; j++){
             if (before.inuse[j] != 1)
               continue;
             if (before.pid[j] == pid_chds[i]){
               for (k = 0; k < NPROC; k++){
                 if (after.inuse[k] != 1)
                   continue;
                 if (after.pid[k] == pid_chds[i]){
                   ticks_chds[i] = after.ticks[k] - before.ticks[j];
                   sum += ticks_chds[i];
                   count++;
                   break;
                 }
               }
               break;
             }
           }
         }
         if (count < numprocs){
           printf(1,"check failed: Lost child process\n");
           goto Cleanup;
         }
         if (sum != 0){
           // Print info
           // Compute variance
           avg = sum/count;
           for (i = 0; i < numprocs; i++)
             var += (avg-ticks_chds[i])*(avg-ticks_chds[i]);
           var /= count;
           
           if (goal == -1){
             if (var < 25){
               printf(1,"check failed: Starting variance %d too low\n",var);
               goto Cleanup;
             }
             goal = var/5;
             if (goal < 15)
               goal = 15;
             printf(1,"Goal child ticks variance: <%d\n\n",goal);
           }
           
           printf(1,"Child ticks variance: %d\n",var);
           if (var < goal)
             goto Success;
           
         }
         else{
           printf(1,"check failed: Ticks not properly implemented\n");
           goto Cleanup;
         }
       } // Chdnum = 0
       
       // Send updated tickets to child
       if (id_chd < 0)
         write(pipe_par[1],"256\n",4*sizeof(char));
       else{
         if (ticks_chds[id_chd] < 1)
           ticks_chds[id_chd] = 1;
         printf(pipe_par[1],"%d\n",(int)((double)avg/ticks_chds[id_chd]*256));
       }
       // Wait for child to update its tickets
       read(pipe_chd[0],buffer,16*sizeof(char));
       if (*buffer == 'b'){
         printf(1,"check failed: settickets\n");
         goto Cleanup;
       }
       // Signal next child
       write(pipe_reap[1],"a",sizeof(char));
     } // Handle updating tickets for each child
   }
Success:
   printf(1, "Should print 1 then 2");
Cleanup:
   for (i = 0; i < numprocs; i++){
     if (pid_chds[i] > pid_par)
       kill(pid_chds[i]);
     else
       break;
   }
   close(pipe_pend[1]);
   close(pipe_ready[1]);
   close(pipe_reap[1]);
   close(pipe_par[1]);
   close(pipe_chd[0]);
   while (wait() > 0);
   
   exit();
}
