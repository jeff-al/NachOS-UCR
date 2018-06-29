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
#include <vector>
#include <deque>
using namespace std;

#define KEY 0xB60380

SemTabla * SMT = new SemTabla();
Semaphore* Console = new Semaphore((char*)'S', 1);

// Estructura para los ejecutables
struct archivosEjec{
  long hilo;
  string nombre;
  Semaphore* sem;
  long hiloJ;
  archivosEjec(){
    hilo = -1;
	sem = NULL;
	hiloJ = -1;
  }
};
vector<archivosEjec*> ejecutables;	// vector de archivos ejecutables
long archivoAE = -1;
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

// Metodo para retornar de un llamado al sistema
void returnFromSystemCall() {
    int pc, npc;
    pc = machine->ReadRegister( PCReg );
    npc = machine->ReadRegister( NextPCReg );
    machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
    machine->WriteRegister( PCReg, npc );           // PC <- NextPC
    machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4
}

// System call 0
void Nachos_Halt() {
    DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
    returnFromSystemCall();
}

// System call 1
void Nachos_Exit(){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	for(unsigned int i=0; i<ejecutables.size(); ++i){
		if(ejecutables[i]->hilo == (long)currentThread){
			if(ejecutables[i]->sem != NULL){ // Revisamos si algun hilo esta esperando por mi
				ejecutables[i]->sem->V();	// Le avisamos al hilo para que pueda ejecutarse
				ejecutables[i]->sem = NULL;
			}
		}
	}
    currentThread->NOP->delThread();
    Thread * next = scheduler->FindNextToRun();		// Buscamos si hay alguien mas para correr
    if(next == NULL){
      currentThread->Finish(); // si no hay nadie finalizamos
    }else{
      scheduler->Run(next);	// si hay alguno lo mandamos a correr
    }
    interrupt->SetLevel(oldLevel);

    returnFromSystemCall();
}

void NachosExecThread(void *p) {
    archivosEjec * ejec = (archivosEjec *)p;
    OpenFile *executable = fileSystem->Open(ejec->nombre.c_str());  // Abrimos el ejecutable
    if (executable == NULL) {
       cout << "no se puede abrir el archivo: " << ejec->nombre << endl;
       return;
    }
    AddrSpace *space = new AddrSpace(executable); // Creamos un addrspace nuevo para el ejecutable
    delete currentThread->space;
    currentThread->space = space;	// Asignamos el addrspace creado
    delete executable;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register
    machine->Run();                     // jump to the user progam
    cout << "Fallo" << endl;
}

// System call 2
void Nachos_Exec(){
  // Leemos el nombre del ejecutable
  char name[128];
  int k = 1;
  int i = 0;
  int reg4 = machine->ReadRegister( 4 );
  do{
    machine->ReadMem(reg4,1,&k);
    reg4++;
    name[i] = k;
    i++;
  }while(k != 0);
  name[i] = 0;
  string nombre = name;
  archivosEjec* newEj = new archivosEjec(); // creamos un nuevo objeto para guardar el hilo del ejecutable
  Thread * newT = new Thread( "child to execute EXEC code" );	// Creamos el nuevo hilo
  archivoAE++;	// Aumentamos el contador de archivos a Ejecutar
  newEj->hilo = (long) newT;	// le asignamos al objeto el hilo creado
  newEj->nombre = nombre;	// le asignamos el nombre del ejecutable
  ejecutables.push_back(newEj);	// agregamos el objeto al vector de ejecutables
  currentThread->NOP->addThread();
  newT->Fork( NachosExecThread, (void *)newEj);
  machine->WriteRegister(2, archivoAE);

  returnFromSystemCall();
}

// System call 3
void Nachos_Join(){
	unsigned long id = machine->ReadRegister( 4 );  // Leemos el id del ejecutable que queremos ejecutar
	if(id < ejecutables.size()){
		// Si hay un ejecutable con ese id esperamos para ejecutarlo
		Semaphore* semafor = new Semaphore((char*)'S', 0);
    	ejecutables[id]->sem = semafor;
		ejecutables[id]->hiloJ = (long)currentThread;
    	semafor->P();
		machine->WriteRegister(2, 1);
	}else{
		// Si no hay ejecutables con ese id retornamos error = -1
		machine->WriteRegister(2, -1 );
	}
}

// System call 4
void Nachos_Create(){
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
}

// System call 5
void Nachos_Open(){
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
}

// System call 6
void Nachos_Read(){
  int size = machine->ReadRegister( 5 );	// Leemos el tamaño
  char buffer[size] = {0};
  int r4 = machine->ReadRegister(4);		// Leemos al dirreccion del buffer
  OpenFileId id = machine->ReadRegister( 6 );	// Leemos el id del archivo
  int bytesLeidos = 0;
  Console->P();		//Usamos semaforo para sincronizar
  switch (id) {
      case  ConsoleInput:
          fgets( buffer, size , stdin );
          bytesLeidos = strlen( buffer );
          for (int index = 0; index < bytesLeidos; ++  index ){
              machine->WriteMem(r4, 1, buffer[index] );
              ++r4;
          }
          machine->WriteRegister(2, bytesLeidos );
          break;
      case  ConsoleOutput:
		  // No se permite
          machine->WriteRegister( 2, -1 );
          break;
      case ConsoleError:
		  // No se permite
		  cout << "Error" << endl;
          machine->WriteRegister( 2, -1 );
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
  Console->V();
  returnFromSystemCall();
}

// System call 7
void Nachos_Write() {
    int size = machine->ReadRegister( 5 );	// Leemos el tamaño
    char buffer[size+1] = {0};
    OpenFileId id = machine->ReadRegister( 6 );
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
            if(currentThread->NOP->isOpened(id)){
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

// System call 8
void Nachos_Close(){
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

// System call 9
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
    currentThread->NOP->addThread();
  	// We (kernel)-Fork to a new method to execute the child code
  	// Pass the user routine address, now in register 4, as a parameter
  	// Note: in 64 bits register 4 need to be casted to (void *)
  	newT->Fork( NachosForkThread, (void *)machine->ReadRegister( 4 ));
    currentThread->Yield();
  	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers
  	DEBUG( 'u', "Exiting Fork System call\n" );
}

// System call 10
void Nachos_Yield(){
  currentThread->Yield();
  returnFromSystemCall();
}

// System call 11
void Nachos_SemCreate(){
  int valorInicial = machine->ReadRegister( 4 );
  Semaphore * sem = new Semaphore("Semaforo", valorInicial);
  long id = (long)sem;
  int idFake = SMT->Create(id);
  machine->WriteRegister(2, idFake);
  returnFromSystemCall();
}

// System call 12
void Nachos_SemDestroy(){
  int id = machine->ReadRegister( 4 );
  //long idReal = SMT->getSemaphore(id);
  int ret = SMT->Close(id);
  machine->WriteRegister(2, ret);
    returnFromSystemCall();
}

// System call 13
void Nachos_SemSignal(){
    int id = machine->ReadRegister( 4 );
    long idReal = SMT->getSemaphore(id);
    if(idReal == -1){
      machine->WriteRegister(2, -1);
    }else{
      Semaphore * sem = (Semaphore *) idReal;
      sem->V();
      machine->WriteRegister(2, 1);
    }
    returnFromSystemCall();
}

// System call 14
void Nachos_SemWait(){
    int id = machine->ReadRegister( 4 );
    long idReal = SMT->getSemaphore(id);
    if(idReal == -1){
      machine->WriteRegister(2, -1);
    }else{
      Semaphore * sem = (Semaphore *) idReal;
      sem->P();
      machine->WriteRegister(2, 1);
    }
    returnFromSystemCall();
}

void
ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2);
  switch ( which ) {
     case SyscallException:
        switch ( type ) {
          case SC_Halt:					// System call # 0
              cout << "Halt " << endl;
              Nachos_Halt();
              break;
		      case SC_Exit:					// System call # 1
              cout << "Exit " << endl;
              Nachos_Exit();
              break;
		      case SC_Exec:					// System call # 2
              cout << "Exec " << endl;
              Nachos_Exec();
              break;
          case SC_Join:					// System call # 3
              cout << "Join " << endl;
              Nachos_Join();
			        returnFromSystemCall();
              break;
          case SC_Create:				// System call # 4
              cout << "Create " << endl;
              Nachos_Create();
              break;
          case SC_Open:					// System call # 5
              cout << "Open " << endl;
              Nachos_Open();
              break;
		      case SC_Read:     			// System call # 6
              cout << "Read " << endl;
              Nachos_Read();
              break;
          case SC_Write:				// System call # 7
              cout << "Write " << endl;
              Nachos_Write();
              break;
          case SC_Close:				// System call # 8
              cout << "Close " << endl;
              Nachos_Close();
              break;
          case SC_Fork:					// System call # 9
              cout << "Fork " << endl;
              Nachos_Fork();
              break;
          case SC_Yield:				// System call # 10
              cout << "Yield " << endl;
              Nachos_Yield();
              break;
          case SC_SemCreate:			// System call # 11
              cout << "Create " << endl;
              Nachos_SemCreate();
              break;
          case SC_SemDestroy:			// System call # 12
              cout << "Destroy " << endl;
              Nachos_SemDestroy();
              break;
          case SC_SemSignal:			// System call # 13
              cout << "Signal " << endl;
              Nachos_SemSignal();
              break;
          case SC_SemWait:				// System call # 14
              cout << "Wait " << endl;
              Nachos_SemWait();
              break;
          default:
              cout << "defalut " << endl;
              printf("Unexpected syscall exception %d\n", type );
              ASSERT(false);
              break;
        }
        break;
     case PageFaultException:
        cout << "PageFault"<< endl;
        //ASSERT(false);
        break;
     default:
        printf( "Unexpected exception %d\n", which );
        ASSERT(false);
        break;
  }
}
