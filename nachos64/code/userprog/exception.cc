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
//borrar todo
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "sych.h"
#include "OpenFilesTable.h"

static void ThreadFuncForUserProg(int arg);

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

void returnFromSystemCall() {
	/* 
		Controla el valor de retorno del system call.
	*/
	int pc, npc;
	
	pc = machine->ReadRegister( PCReg );
	npc = machine->ReadRegister( NextPCReg );
	machine->WriteRegister( PrevPCReg, pc );		// PrevPC <- PC
	machine->WriteRegister( PCReg, npc ); 			// PC <- NextPC
	machine->WriteRegister( NextPCReg, npc + 4 ); 	// NextPC <- NextPC + 4
}

char* readName(int addr){
	// Retorna el nombre del fichero guardado en una posición de memoria.
	char* name = new char[20];
	int i = 0;
	char letra;
	while(true){
		int value;
		machine->ReadMem(addr,1,&value); //Recibio un entero con la direccion en memoria donde se inicia la cadena.
		letra = (char)value;
		name[i] = letra;
		++i; ++addr;
		if(letra == '\0'){
			break;
		}
	}
	return name;						//Se devuelve el nombre del archivo.
}

// A dummy handler function of user program's Fork/Exec. The main purpose of this
// function is to run virtual machine in nachos(machine->Run()). Before do that,
// Fork need to restore virtual machine's registers and Exec need to init virtual
// machine's registers and pageTable.
static void ThreadFuncForUserProg(int arg)
{
    switch (arg)
    {
        case 0: // Fork
            // Fork just restore registers.
            currentThread->RestoreUserState();
            break;
        case 1: // Exec
            if (currentThread->space != NULL)
            {
                // Exec should initialize registers and restore address space.
                currentThread->space->InitRegisters();
                currentThread->space->RestoreState();
            }
            break;
        default:
            break;
    }

	machine->Run();
}

void Nachos_Open(){
	/* System call definition described to user
		int Open(
			char *name	// Register 4 - Dirección de la memoria donde está el nombre del archivo que se abrirá.
		);
	*/
	DEBUG('u', "Opening file.\n");
	int addr = machine->ReadRegister(4);
	char* filename = readName(addr); 						// Read the name from the user memory, see 4 below
	int openId = open(filename, O_RDWR|O_CREAT,S_IRWXU); 	// Use NachosOpenFilesTable class to create a relationship
	if(openId != -1){										// between user file and unix file
		int NachosHandle = tablaOpenFiles->Open(openId);
		machine->WriteRegister(2, NachosHandle); 			// NachosHandle del archivo que esta abierto. En caso error devuelve -1.
		printf("Opened file: \"%s\" \n", filename);
	}
	else{
		printf("File could not be opened\n");
		machine->WriteRegister(2,-1);						//Retorna -1 para indicar que hay un error.
	}
}

void Nachos_Halt() {                    
		// System call 0
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
}       // Nachos_Halt

void Nachos_Exec() //modificar
{
	char fileName[100];
	int arg = machine->ReadRegister(4);
	int i = 0;

    // Trae el nombre del archivo.
	do
	{
		machine->ReadMem(arg + i, 1, (int*)&fileName[i]);
	} while (fileName[i++] != '\0');

    // Abre el archivo ejecutable
	OpenFile* executable = fileSystem->Open(fileName);
	if (executable != NULL)	
	{
        Thread* thread = new Thread(fileName);
		
        // Devuelve el ID
		machine->WriteRegister(2, thread->getName());

		DEBUG('a', "Exec desde thread %d -> executable %s\n", 
				currentThread->getName(), fileName);
		thread->Fork(ThreadFuncForUserProg, 1);
	}
	else
	{
        // Si no se puede abrir devuelve -1.
		machine->WriteRegister(2, -1);
	}
	
	returnFromSystemCall();
}

void Nachos_Join()
{
    Thread *childThread;
    int exitStatus = 0;
    int childThreadId = machine->ReadRegister(4);
    
    // Verifica que exista el hilo.
    childThread = currentThread->getName();

    // Consigue el estado del hilo
    exitStatus = childThread->status;

    // Retorna el estado de hilo
    machine->WriteRegister(2, exitStatus);

	returnFromSystemCall();
}

void Nachos_Fork()
{
	DEBUG( 'u', "Entering Fork System call\n" );
	// We need to create a new kernel thread to execute the user thread
	Thread hiloNuevo = new Thread("Se creo nuevo hilo");
	// We need to share the Open File Table structure with this new child
	tablaOpenFiles->addThread();
	hiloNuevo->space = new AddrSpace(currentThread->space);
	hiloNuevo->main = false;
	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	// Note: in 64 bits register 4 need to be casted to (void *)
	hiloNuevo->Fork(NachosForkThread, machine->ReadRegister(4););
	
	returnFromSystemCall();
	DEBUG( 'u', "Exiting Fork System call\n" );
}

// Pass the user routine address as a parameter for this function
// This function is similar to "StartProcess" in "progtest.cc" file under "userprog"
// Requires a correct AddrSpace setup to work well

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
    machine->WriteRegister( NextPCReg, p + 4 );

    machine->Run();                     // jump to the user progam
    ASSERT(FALSE);

}


void Nachos_Yield()
{
	currentThread->Yield();

	returnFromSystemCall(); 
}

void Nachos_Exit() {
	int status = machine->ReadRegister(4);
	int thread = currentThread->getName();
	
	memoryManager->deleteAddrSpace(thread);
	DEBUG('a', "Exit SystemCall.\n");
	currentThread->setExitStatus(status);
	
	currentThread->Finish();
	
	DEBUG('a', "Program exits with status: %d\n", status);
}

void Nachos_Write(){
	/* System call definition described to user
        void Write(
		char *buffer,							// Register 4 - Dirección del Buffer.
		int size,								// Register 5 - Cantidad de Bytes a Escribir
		OpenFileId id							// Register 6 - Id del archivo donde se escribe.
	);
	*//*
	DEBUG('u', "Writing in file.\n");
	int addr = machine->ReadRegister(4);		// Read file address
	char* buffer = readName(addr);
	int size = machine->ReadRegister(5);		// Read size to write
	OpenFileId id = machine->ReadRegister(6);	// Read file descriptor
	int ret = -1;
	//Se supone que aqui se agrega un semaforo.
	switch (id) {
		case  ConsoleInput:						// User could not write to standard input
			machine->WriteRegister( 2, -1 );
			break;
		case  ConsoleOutput:
			buffer[ size ] = 0;
			printf( "%s", buffer );
	      

		break;
		case ConsoleError:						// This trick permits to write integers to console
			printf( "%d\n", machine->ReadRegister( 4 ) );
			break;
		default:								// All other opened files
			if(id > 1){
				if(tablaOpenFiles->isOpened(id)){						// Verify if the file is opened, if not return -1 in r2
					int unixHandle = tablaOpenFiles->getUnixHandle(id); // Get the unix handle from our table for open files
					ret = write(unixHandle, (void*)buffer, size);		// Do the write to the already opened Unix file
					if(ret != -1){				
						ret = size;				
						printf("Wrote %d bytes in file.\n", ret);
					}
					else{
						printf("Couldn't write in file.\n");
					}
				}
				else{
					printf("The file isn't opened.\n");
					ret = -1;
				}
			} 
			else {
				ret = write(id, (void*)buffer, size);
			}
			machine->WriteRegister(2, ret);		// Return the number of chars written to user, via r2			
	}
	*/
	returnFromSystemCall(); 					// Update the PC registers
}

void Nachos_SemSignal() {
	DEBUG('e', (char*)"Thread 0x%x %s V().\n", currentThread,currentThread->getName());

	Semaphore* sema;
	sema = (Semaphore*)machine->ReadIntRegister(4);

	if (tablaOpenFiles->Search((int *)sema) && sema->typeId == SEMAPHORE_TYPE_ID) {
		sema->V();
		machine->WriteIntRegister(2,0);      
	}
	else
	{
		sprintf(msg,"%d",(int)sema);
		machine->WriteIntRegister(2,-1);
	}
}

void Nachos_SemWait() {
	DEBUG('e', (char*)"Thread 0x%x %s P().\n", currentThread,currentThread->getName());

	Semaphore* idSema;
	idSema = (Semaphore*) machine->ReadIntRegister(4);

	if (tablaOpenFiles->Search((int *)idSema) && idSema->typeId == SEMAPHORE_TYPE_ID) {
		idSema->P();
		machine->WriteIntRegister(2,0);
	}
	else {
		sprintf(msg,"%d",(int)idSema);			
		machine->WriteIntRegister(2,-1);
	}
}
void Nachos_SemCreate() {
		DEBUG('e', (char*)"Thread 0x%x %s new Sema().\n", currentThread,currentThread->getName());

		Semaphore* idSema;
		int addr_name;
		char debugName[10];
		addr_name = machine->ReadIntRegister(4);
		GetStringParam(addr_name,debugName,10);
		idSema = new Semaphore(debugName, machine->ReadIntRegister(5) );//Erreurs Ã  lever

		tablaOpenFiles->Append(idSema);

		machine->WriteIntRegister(2, (int) idSema);
	}
void Nachos_SemDestroy() {
	DEBUG('e', (char*)"Thread 0x%x %s delete(sema).\n", currentThread,currentThread->getName());

	Semaphore* sema;
	sema = (Semaphore*)machine->ReadIntRegister(4);

	if (UserObj->Search((int *)sema) && sema->typeId == SEMAPHORE_TYPE_ID) {
		delete sema;
		machine->WriteIntRegister(2,0);
	}
	else {
		sprintf(msg,"%d",(int)sema);
		machine->WriteIntRegister(2,-1);
	}
}

void ExceptionHandler(ExceptionType which) {
	int type = machine->ReadRegister(2); 
	if ((which == SyscallException)) {
		switch(type){
			//System call #0
			case SC_Halt:			
				Nachos_Halt();
				break;
			//System call #1
			case SC_Exit:
				Nachos_Exit();
				break;
			//System call #2
			case SC_Exec:
				Nachos_Exec();
				break;
			//System call #3
			case SC_Join:
				Nachos_Join();
				break;
			//System call #4
			case SC_Create:
				//FALTA
				break;
			//System call #5
			case SC_Open:
				Nachos_Open();
				break;
			//System call #6
			case SC_Read:
				//FALTA
				break;
			//System call #7
			case SC_Write:
				Nachos_Write();
				break;
			//System call #8
			case SC_Close:
				//FALTA
				break;
			//System call #9
			case SC_Fork:
				Nachos_Fork();
				break;
			//System call #10
			case SC_Yield:
				Nachos_Yield();
				break;
			//System call #11
			case SC_SemCreate:
				Nachos_SemCreate();
				break;
			//System call #12
			case SC_SemDestroy:
				Nachos_SemDestroy();
				break;
			//System call #13
			case SC_SemSignal:
				Nachos_SemSignal();
				break;
			//System call #14
			case SC_SemWait:
				Nachos_SemWait();
				break;
			default:{
				printf( "Unexpected syscall exception %d\n", which );
				ASSERT(false);
			}break;
		}
		returnFromSystemCall();
	} 
	else{
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(false);
	}
}

