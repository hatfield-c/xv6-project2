#include "syscall.h"
#include "types.h"
#include "user.h"

int main(int argc, char *argv[]){
	if(fork() == 0)
		printf(1, "Child 1");
	else
		printf(1, "Parent 1");
	
	if(fork() == 0)
		printf(1, "Child 2");
	else
		printf(1, "Parent 2");
	
	if(fork() == 0)
		printf(1, "Child 3");
	else
		printf(1, "Parent 3");
	//fork();
	//fork();
	//settickets(4);
	//fork();
	exit();
}
