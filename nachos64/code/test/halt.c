/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int
main()
{
     Halt();
     /*char* palabra = "";
     Halt();
     Create("aguacaton.txt");
     OpenFileId i = Open("aguacaton.txt");
     Write("aguacate el carro es rojo vino", 30, i);
     Close(i);
     Read(palabra, 8, i);
     Close(i);
     Create("palabra.txt");
     OpenFileId i2 = Open("palabra.txt");
     Write(palabra, 8, i2);
     Close(i2);
    /* not reached */
}
/*
int id;
void hijo(int dummy){
	Write( "hijo", 5, 1 );
	SemSignal(id);
}

void hijo(int);
int main(){
	id = SemCreate(0);
	Fork(hijo);

	SemWait(id);
	Write("padre", 5, 1);
	SemDestroy(id);
	Exit(0);
}
*/

