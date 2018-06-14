// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include <unistd.h>
#include <iostream>
using namespace std;

#define KEY 0xB60380

SemTabla * SMT = new SemTabla();

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------
Semaphore* Console = new Semaphore((char*)'S', 1);
void returnFromSystemCall() {
    int pc, npc;
    pc = machine->ReadRegister( PCReg );
    npc = machine->ReadRegister( NextPCReg );
    machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
    machine->WriteRegister( PCReg, npc );           // PC <- NextPC
    machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4
}

void Nachos_Halt() {     // System call 0
    DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
    returnFromSystemCall();
} // Nachos_Halt

void Nachos_Create(){    // System call 4
    char name[128] = "";
    int c = 1;
    int direc = machine->ReadRegister(4);
    int it = 0;
    do{
        machine->ReadMem(direc, 1, &c);
        name[it++] = c;
        direc++;
    }while(c != 0);
	  int id = creat((const char*)name, O_CREAT | S_IRWXU);
    close(id);

    returnFromSystemCall();
} // Nachos_Create

void Nachos_Open(){    // System call 5
    DEBUG('a', "Shutdown, initiated by user program.\n");
    char name[128] = "";
    int c = 1;
    int direc = machine->ReadRegister(4);
    int it = 0;
    do{
        machine->ReadMem(direc, 1, &c);
        name[it++] = c;
        direc++;
    }while(c != 0);
    name[it] = 0;
    int id = open((const char*)name, O_RDWR);
    int idFake = currentThread->NOP->Open(id);
    machine->WriteRegister(2, idFake);

    returnFromSystemCall();
} // Nachos_Open

void Nachos_Read(){ //System call 6
  //Semaphore* Console = new Semaphore((char*)'S', 1);
  int size = machine->ReadRegister( 5 );	// Read size to write
  char buffer[size] = "";
  int r4 = machine->ReadRegister(4);

  // buffer = Read data from address given by user;
  OpenFileId id = machine->ReadRegister( 6 );	// Read file descriptor
  Console->P();
  switch (id) {
      case  ConsoleInput:	// User could not write to standard input
          machine->WriteRegister( 2, -1 );
          break;
      case  ConsoleOutput:
          buffer[ size ] = 0;
          printf( "%s", buffer );
          break;
      case ConsoleError:	// This trick permits to write integers to console
          printf( "%d\n", machine->ReadRegister( 4 ) );
          break;
      default:
          if(currentThread->NOP->isOpened(id)){
              int id2 = currentThread->NOP->getUnixHandle(id);
              int ret = read(id2, (void*)buffer, size );
              for(int i = 0; i < ret; i++){
                   machine->WriteMem(r4, 1, buffer[i]);
                   ++r4;
              }
              machine->WriteRegister( 2, ret );
          }else{
              machine->WriteRegister( 2, -1 );
          }
          break;
  }
  // Update simulation stats, see details in Statistics class in machine/stats.cc
  Console->V();

  returnFromSystemCall();
}

void Nachos_Write() {    // System call 7
//    Semaphore* Console = new Semaphore((char*)'S', 1);
    int size = machine->ReadRegister( 5 );	// Read size to write
    char buffer[size+1] = {0};

    // buffer = Read data from address given by user;
    OpenFileId id = machine->ReadRegister( 6 );	// Read file descriptor

    // Need a semaphore to synchronize access to console
    Console->P();
    int c;
    int direc;
    int it;
    c = 1;
    direc = machine->ReadRegister(4);
    it = 0;
    do{
      machine->ReadMem(direc, 1, &c);
      buffer[it++] = c;
      direc++;
    }while(c != 0);
    buffer[ it ] = 0;
    switch (id) {
        case  ConsoleInput:	// User could not write to standard input
            machine->WriteRegister( 2, -1 );
            break;
        case  ConsoleOutput:
           printf( "%s\n", buffer );
            break;
        case ConsoleError:	// This trick permits to write integers to console
            printf( "%d\n", machine->ReadRegister( 4 ) );
            break;
        default:
        // All other opened files
        // Verify if the file is opened, if not return -1 in r2  *
        // Get the unix handle from our table for open files  *
        // Do the write to the already opened Unix file  *
        // Return the number of chars written to user, via r2  *
            if(currentThread->NOP->isOpened(id)){/*
                 c = 1;
                 direc = machine->ReadRegister(4);
                 it = 0;
                do{
                    machine->ReadMem(direc, 1, &c);
                    buffer[it++] = c;
                    direc++;
                }while(c != 0);*/
				        int id2 = currentThread->NOP->getUnixHandle(id);
			 	        int ret = write(id2, (const void*)buffer, size );
				        machine->WriteRegister( 2, ret );
			      }else{
				        machine->WriteRegister( 2, -1 );
			      }
            break;
    }
	  // Update simulation stats, see details in Statistics class in machine/stats.cc
    //delete buffer;
    Console->V();
    returnFromSystemCall();		// Update the PC registers
}       // Nachos_Write

void Nachos_Close(){    // System call 8
    int id = machine->ReadRegister(4);
    int id2 = currentThread->NOP->getUnixHandle(id);
    close(id2);
    currentThread->NOP->Close(id);

    returnFromSystemCall();
} // Nachos_Close

//void NachosForkThread( int p ) { // for 32 bits version
void NachosForkThread( void * p ) { // for 64 bits version
    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

// Set the return address for this thread to the same as the main thread
// This will lead this thread to call the exit system call and finish
    machine->WriteRegister( RetAddrReg, 4 );

    machine->WriteRegister( PCReg, (long) p );
    machine->WriteRegister( NextPCReg, (long)(p)+ 4 );
    machine->Run();                     // jump to the user progam
    ASSERT(false);

}

void Nachos_Fork(){
    DEBUG( 'u', "Entering Fork System call\n" );
  	// We need to create a new kernel thread to execute the user thread
  	Thread * newT = new Thread( "child to execute Fork code" );

  	// We need to share the Open File Table structure with this new child

  	// Child and father will also share the same address space, except for the stack
  	// Text, init data and uninit data are shared, a new stack area must be created
  	// for the new child
  	// We suggest the use of a new constructor in AddrSpace class,
  	// This new constructor will copy the shared segments (space variable) from currentThread, passed
  	// as a parameter, and create a new stack for the new child
  	newT->space = new AddrSpace( currentThread->space );

  	// We (kernel)-Fork to a new method to execute the child code
  	// Pass the user routine address, now in register 4, as a parameter
  	// Note: in 64 bits register 4 need to be casted to (void *)
  	newT->Fork( NachosForkThread, (void *)machine->ReadRegister( 4 ));
    currentThread->Yield();
  	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers
  	DEBUG( 'u', "Exiting Fork System call\n" );
}

void Nachos_SemCreate(){
  int valorInicial = machine->ReadRegister( 4 );
  Semaphore * sem = new Semaphore("Semaforo", valorInicial);
  long id = (long)sem;
  int idFake = SMT->Create(id);
  machine->WriteRegister(2, idFake);
  returnFromSystemCall();
}

void Nachos_SemDestroy(){
  int id = machine->ReadRegister( 4 );
//  long idReal = currentThread->SMT->getSemaphore(id);
  int ret = SMT->Close(id);
  machine->WriteRegister(2, ret);
    returnFromSystemCall();
}

void Nachos_SemSignal(){
    int id = machine->ReadRegister( 4 );
    cout << "signal id :" << id << endl;
    long idReal = SMT->getSemaphore(id);
    if(idReal == -1){
      machine->WriteRegister(2, -1);
      cout << "no hago signal en sem:" << idReal << endl;
    }else{
      Semaphore * sem = (Semaphore *) idReal;
      cout << "hago signal en sem:" << idReal << endl;
      sem->V();
      machine->WriteRegister(2, 1);
    }
    returnFromSystemCall();
}
void Nachos_SemWait(){
    int id = machine->ReadRegister( 4 );
    cout << "wait id :" << id << endl;
    long idReal = SMT->getSemaphore(id);
    if(idReal == -1){
      machine->WriteRegister(2, -1);
    }else{
      Semaphore * sem = (Semaphore *) idReal;
      cout << "hago wait en sem:" << idReal << endl;
      sem->P();
      machine->WriteRegister(2, 1);
    }
    returnFromSystemCall();
}

void Nachos_Exit(){
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    Thread * next = scheduler->FindNextToRun();
    if(next == NULL){
      currentThread->Finish();
    }else{
      scheduler->Run(next);
    }
    interrupt->SetLevel(oldLevel);
}

void Nachos_Yield(){
  currentThread->Yield();
  returnFromSystemCall();
}

void NachosExecThread(void *p) { // for 64 bits version
    char name[128];
    int k = 1;
    int i = 0;
    int reg4 = (long)p;
    do{
      machine->ReadMem(reg4,1,&k);
      reg4++;
      name[i] = k;
      i++;
    }while(k != 0);
    name[i] = 0;
    OpenFile *executable = fileSystem->Open(name);
    AddrSpace *space = new AddrSpace(executable);
    delete executable;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register
    machine->Run();                     // jump to the user progam
    //ASSERT(false);

}

void Nachos_Exec(){
  Thread * newT = new Thread( "child to execute EXEC code" );
  newT->Fork( NachosExecThread, (void *)machine->ReadRegister( 4 ));
  //currentThread->Yield();
  returnFromSystemCall();
}

void Nachos_Join(){
  //OpenFile *executable = machine->ReadRegister( 4 );
  //AddrSpace *space = new AddrSpace(executable);

  //machine->WriteRegister(2, 1);
  returnFromSystemCall();
}

void
ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2);
  switch ( which ) {
     case SyscallException:
        switch ( type ) {
           case SC_Halt:
               cout << "Halt " << endl;
               Nachos_Halt();             // System call # 0
               break;
           case SC_Create:
               cout << "Create " << endl;
               Nachos_Create();             // System call # 4
               break;
           case SC_Open:
               cout << "Open " << endl;
               Nachos_Open();             // System call # 5
               break;
           case SC_Write:
               cout << "Write " << endl;
               Nachos_Write();             // System call # 7
               break;
          case SC_Close:
               cout << "Close " << endl;
               Nachos_Close();             // System call # 8
               break;
          case SC_Read:     // System call # 6
               cout << "Read " << endl;
               Nachos_Read();
               break;
          case SC_Fork:		                       // System call # 9
              cout << "Fork " << endl;
              Nachos_Fork();
              break;
          case SC_Exit:                           // System call # 1
              cout << "Exit " << endl;
              Nachos_Exit();
              break;
          case SC_SemCreate:                           // System call # 11
              cout << "Create " << endl;
              Nachos_SemCreate();
              break;
          case SC_SemDestroy:                           // System call # 12
              cout << "Destroy " << endl;
              Nachos_SemDestroy();
              break;
          case SC_SemSignal:                           // System call # 13
              cout << "Signal " << endl;
              Nachos_SemSignal();
              break;
          case SC_SemWait:                           // System call # 14
              cout << "Wait " << endl;
              Nachos_SemWait();
              break;
          case SC_Yield:                           // System call # 14
              cout << "Yield " << endl;
              Nachos_Yield();
              break;
          case SC_Exec:                           // System call # 14
              cout << "Exec " << endl;
              Nachos_Exec();
              break;
          case SC_Join:                           // System call # 14
              cout << "Join " << endl;
              Nachos_Join();
              break;
           default:
              cout << "defalut " << endl;
              printf("Unexpected syscall exception %d\n", type );
              ASSERT(false);
              break;
     }
     break;
     default:
        printf( "Unexpected exception %d\n", which );
        ASSERT(false);
        break;
  }
}
