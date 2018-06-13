#include "syscall.h"

//Globals
int sH;
int sO;
int cH;
int cO;

void O()
{
    cO++;
	if(cH > 1)
	{
		cH=-2;
		cO--;
		Write("Haciendo agua!!\n",17,ConsoleOutput);
		SemSignal(sH); 
		SemSignal(sH);
	}
	else
	{
		SemWait(sO);
	}
}

void H()
{
	cH++;
	if(cH > 1 && cO > 0)
	{
		cH=-2;
		cO--;
		Write("Haciendo agua!!\n",17,ConsoleOutput);
		SemSignal(sH);
		SemSignal(sO);
	}
	else
	{
		SemWait(sH);
	}
}

int main()
{
    int i;

    sH = SemCreate(0);
    sO = SemCreate(0);

    for(i=0;i<5;i++)
    {
        Fork(O);
    }    

    for(i=0;i<8;i++)
    {
        Fork(H);
    }   

    SemDestroy(sH);
    SemDestroy(sO);

    // Exit this program
    Exit(0);
}
