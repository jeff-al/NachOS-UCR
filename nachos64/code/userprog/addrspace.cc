// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
//#include "noff.h"

// Eliminar
#include <iostream>
using namespace std;

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable, const char *filename)
{

		if (Swap == NULL){
			Swap = fileSystem->Open("SWAP");
			ipt = new TPI;
		}
		#ifdef VM
			strcpy( ejecutable, filename );
		#endif
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC)) SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
		noffH1 = noffH;

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

		#ifndef VM
    	ASSERT(numPages <= NumPhysPages);		// check we're not trying
		#endif
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
					numPages, size);
// first, set up the translation
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
			pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
			#ifndef VM
			  pageTable[i].physicalPage = memoryMap->Find();
				pageTable[i].valid = true;
			#else
				pageTable[i].valid = false;
				pageTable[i].physicalPage = -1;
			#endif
			//pageTable[i].physicalPage = -1;
			pageTable[i].use = false;
			pageTable[i].dirty = false;
			pageTable[i].readOnly = false;  // if the code segment was entirely on
					// a separate page, we could set its
					// pages to be read-only
    }

// zero out the entire address space, to zero the unitialized data segment
// and the stack segment

  //  bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
		///*
		#ifndef VM
    int numPages2 = divRoundUp(noffH.code.size, numPages);
		int direccionDeMem = noffH.code.inFileAddr;
		for(int j = 0 ; j < numPages2; ++j){
				executable->ReadAt(&(machine->mainMemory[pageTable[j].physicalPage*128]), PageSize, direccionDeMem);
				direccionDeMem+=128;
				}
		//*/

		/*
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }*/
		//*/
		///*
		int numPages3 = divRoundUp(noffH.initData.size, numPages);
		direccionDeMem = noffH.initData.inFileAddr;
		for(int j = numPages2; j < numPages3; ++j){
				executable->ReadAt(&(machine->mainMemory[pageTable[j].physicalPage*128]), PageSize, direccionDeMem);
				direccionDeMem+=128;
				}
		#endif
		//*/
		/*
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
		*/
}

AddrSpace::AddrSpace(AddrSpace *addrspace){
	numPages = addrspace->numPages;
	pageTable = new TranslationEntry[addrspace->numPages];
	for(int i = 0; i < addrspace->numPages-8; i++){
		pageTable[i].virtualPage = addrspace->pageTable[i].virtualPage;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = addrspace->pageTable[i].physicalPage;
		pageTable[i].valid = addrspace->pageTable[i].valid;
		pageTable[i].use = addrspace->pageTable[i].use;
		pageTable[i].dirty = addrspace->pageTable[i].dirty;
		pageTable[i].readOnly = addrspace->pageTable[i].readOnly;
	}

	for(int i = addrspace->numPages-8; i < addrspace->numPages; i++){
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = memoryMap->Find();
		pageTable[i].valid = true;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;

	}
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	 for (int j = 0; j < numPages; j++){
		 memoryMap->Clear(pageTable[j].physicalPage );
	 }
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing! && !pageTable[machine->tlb[i].virtualPage].valid
//----------------------------------------------------------------------

/*
if(pageTable[machine->tlb[i].virtualPage].dirty){
	int campo = swapMap->Find();
	Swap->WriteAt(&(machine->mainMemory[(pageTable[i].physicalPage)*128]), PageSize, campo*128);
	memoryMap->Clear(machine->tlb[i].physicalPage);
	pageTable[machine->tlb[i].virtualPage].physicalPage = campo;
}
*/

void AddrSpace::SaveState()
{
	for(int i = 0; i < 4; i++){
		pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
		pageTable[machine->tlb[i].virtualPage].valid = machine->tlb[i].valid;
	}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
		#ifndef VM
    	machine->pageTable = pageTable;
    	machine->pageTableSize = numPages;
		#endif
}

int iter = 0;
void AddrSpace::MoveraMemoria(int vpn){
	for(int i = 0; i < 4; i++){
		pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
		pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
	}
	OpenFile *executable = fileSystem->Open(ejecutable);
	int numPages1 = divRoundUp(noffH1.code.size, PageSize);
	int numPages2 = divRoundUp(noffH1.initData.size, PageSize);
	bool valida = pageTable[vpn].valid;
	bool sucia = pageTable[vpn].dirty;
	int ubcMemFis = 0;
	if(vpn<numPages1){
		cout << "Codigo" << endl;
		if(!valida){
			// Cargar la pagina a MEM y al TLB
			ubcMemFis = memoryMap->Find();
			if(-1 == ubcMemFis){
				ubcMemFis = buscarVictima(vpn);
			}
			pageTable[vpn].physicalPage = ubcMemFis;
			pageTable[vpn].virtualPage = vpn;
	   		pageTable[vpn].valid = true;
			pageTable[vpn].use = true;
			pageTable[vpn].dirty = false;
			pageTable[vpn].readOnly = true;
			copiarATLB(vpn , iter);
			iter = ++iter % 4;
			executable->ReadAt(&(machine->mainMemory[(pageTable[vpn].physicalPage)*PageSize]), PageSize, sizeof(NoffHeader)+vpn*PageSize);
			ipt[ubcMemFis].vpn = vpn;
		}else{
			// Pasar del PT al TLB
			copiarATLB(vpn, iter);
			iter = ++iter % 4;
		}
	}else if(vpn<numPages2){
		cout << "Datos Inicializados" << endl;
		if(!valida && !sucia){
			// Cargar la pagina a MEM y al TLB
			ubcMemFis = memoryMap->Find();
			if(-1 == ubcMemFis){
				ubcMemFis = buscarVictima(vpn);
				if(pageTable[machine->tlb[iter].virtualPage].dirty){
					int campo = swapMap->Find();
					Swap->WriteAt(&(machine->mainMemory[(pageTable[iter].physicalPage)]), PageSize, campo*PageSize);
					memoryMap->Clear(machine->tlb[iter].physicalPage);
					pageTable[machine->tlb[iter].virtualPage].physicalPage = campo;
					pageTable[machine->tlb[iter].virtualPage].valid = false;
				}
			}
			pageTable[vpn].physicalPage = ubcMemFis;
			pageTable[vpn].virtualPage = vpn;
		    pageTable[vpn].valid = true;
			pageTable[vpn].use = true;
			pageTable[vpn].dirty = false;
			pageTable[vpn].readOnly = false;
			copiarATLB(vpn , iter);
			iter = ++iter % 4;
			executable->ReadAt(&(machine->mainMemory[(pageTable[vpn].physicalPage)*PageSize]), PageSize, sizeof(NoffHeader)+(vpn*PageSize));
			ipt[ubcMemFis].vpn = vpn;
		}else if(valida){
			// Pasar del PT al TLB
			copiarATLB(vpn, iter);
			iter = ++iter % 4;
		}else{
			// Pasar del SWAP a MEM y al TLB
			int campoViejo = pageTable[vpn].physicalPage;
			ubcMemFis = memoryMap->Find();
			if(-1 == ubcMemFis){
				ubcMemFis = buscarVictima(vpn);
				if(pageTable[machine->tlb[iter].virtualPage].dirty){
					int campo = swapMap->Find();
					Swap->WriteAt(&(machine->mainMemory[(pageTable[iter].physicalPage)]), PageSize, campo*PageSize);
					memoryMap->Clear(machine->tlb[iter].physicalPage);
					pageTable[machine->tlb[iter].virtualPage].physicalPage = campo;
					pageTable[machine->tlb[iter].virtualPage].valid = false;
				}
			}
			pageTable[vpn].physicalPage = ubcMemFis;
			pageTable[vpn].virtualPage = vpn;
		    pageTable[vpn].valid = true;
			pageTable[vpn].use = true;
			pageTable[vpn].dirty = true;
			pageTable[vpn].readOnly = false;
			copiarATLB(vpn , iter);
			iter = ++iter % 4;
			Swap->ReadAt(&(machine->mainMemory[(pageTable[vpn].physicalPage)*128]), PageSize, campoViejo*PageSize);
			swapMap->Clear(campoViejo);
			ipt[ubcMemFis].vpn = vpn;
		}
	}else{
		cout << "Datos NO Inicializados" << endl;
		if(!valida && !sucia){
			// Cargar al TLB
			ubcMemFis = memoryMap->Find();
			if(-1 == ubcMemFis){
				ubcMemFis = buscarVictima(vpn);
				if(pageTable[machine->tlb[iter].virtualPage].dirty){
					int campo = swapMap->Find();
					Swap->WriteAt(&(machine->mainMemory[(pageTable[iter].physicalPage)]), PageSize, campo*PageSize);
					memoryMap->Clear(machine->tlb[iter].physicalPage);
					pageTable[machine->tlb[iter].virtualPage].physicalPage = campo;
					pageTable[machine->tlb[iter].virtualPage].valid = false;
				}
			}
			pageTable[vpn].physicalPage = ubcMemFis;
			pageTable[vpn].virtualPage = vpn;
			pageTable[vpn].valid = true;
			pageTable[vpn].use = true;
			pageTable[vpn].dirty = false;
			pageTable[vpn].readOnly = false;
			copiarATLB(vpn , iter);
			iter = ++iter % 4;
			ipt[ubcMemFis].vpn = vpn;
		}else if(valida){
			// Pasar del PT al TLB
			copiarATLB(vpn, iter);
			iter = ++iter % 4;
		}else{
			// Pasar del SWAP a MEM y al TLB
			int campoViejo = pageTable[vpn].physicalPage;
			ubcMemFis = memoryMap->Find();
			if(-1 == ubcMemFis){
				ubcMemFis = buscarVictima(vpn);
				if(pageTable[machine->tlb[iter].virtualPage].dirty){
					int campo = swapMap->Find();
					Swap->WriteAt(&(machine->mainMemory[(pageTable[iter].physicalPage)]), PageSize, campo*PageSize);
					memoryMap->Clear(machine->tlb[iter].physicalPage);
					pageTable[machine->tlb[iter].virtualPage].physicalPage = campo;
					pageTable[machine->tlb[iter].virtualPage].valid = false;
				}
			}
			pageTable[vpn].physicalPage = ubcMemFis;
			pageTable[vpn].virtualPage = vpn;
			pageTable[vpn].valid = true;
			pageTable[vpn].use = true;
			pageTable[vpn].dirty = true;
			pageTable[vpn].readOnly = false;
			copiarATLB(vpn , iter);
			iter = ++iter % 4;
			Swap->ReadAt(&(machine->mainMemory[(pageTable[vpn].physicalPage)*128]), PageSize, campoViejo*PageSize);
			swapMap->Clear(campoViejo);
			ipt[ubcMemFis].vpn = vpn;
		}
	}
}

void AddrSpace::copiarATLB(int indPagT, int indTLB){
			machine->tlb[indTLB].virtualPage = pageTable[indPagT].virtualPage;
			machine->tlb[indTLB].physicalPage = pageTable[indPagT].physicalPage;
			machine->tlb[indTLB].valid = pageTable[indPagT].valid;
			machine->tlb[indTLB].use = pageTable[indPagT].use;
			machine->tlb[indTLB].dirty = pageTable[indPagT].dirty;
			machine->tlb[indTLB].readOnly = pageTable[indPagT].readOnly;
}

int AddrSpace::buscarVictima(int vpn){
	bool listo = false;
	int victima = -1;
	while(!listo)
	for(int i = 1; i < 32 ; i++){
		if(!pageTable[ipt[i].vpn].use){
			victima = pageTable[ipt[i].vpn].physicalPage;
			listo = true;
			break;
		}else{
			pageTable[ipt[i].vpn].use = false;
		}
	}
	return victima;
}
