//----------------------------------------------------------------------------------------
// processtable.cc
// Estructura de datos que implementa una tabla de descriptores sobre los procesos que se
// estan ejecutando en el sistema y provee de una interfaz para utilizarla.
//----------------------------------------------------------------------------------------
// Edited by: Leonardo Forti, Sebastian Galiano, Diego Smania
//----------------------------------------------------------------------------------------


#include "processtable.h"


//----------------------------------------------------------------------------------------
// ProcessTable::ProcessTable()
//----------------------------------------------------------------------------------------

ProcessTable::ProcessTable()
{
	processList = new Process*[MAX_PROCESS];

	for (int i = 0; i < MAX_PROCESS; i++) {
		processList[i] = NULL;
	}
}

//----------------------------------------------------------------------------------------
// ProcessTable::~ProcessTable()
//----------------------------------------------------------------------------------------

ProcessTable::~ProcessTable()
{
	delete processList;
}

//----------------------------------------------------------------------------------------
// ProcessTable::attachProcess()
//----------------------------------------------------------------------------------------

SpaceId ProcessTable::attachProcess(Thread* thread)
{
	// Buscamos un descriptor disponible.

	int i = 0;

	while (processList[i] != NULL && i < MAX_PROCESS)
		i++;

	// Si encontramos descriptor, agregamos el proceso a la tabla y retornamos dicho
	// descriptor, de otra manera retornamos -1.

	if (i < MAX_PROCESS) {
		Process* p = new Process;
		p->thread = thread;
		p->status = ALIVE;
		p->exitValue = 0;

		DEBUG('y',"[PROCESSTABLE]: Process attached in position %d.\n", i);
		processList[i] = p;
		return i;
	}

	DEBUG('y',"[PROCESSTABLE]: Can't attach thread, no descriptor available!\n");
	return -1;
}

//----------------------------------------------------------------------------------------
// ProcessTable::detachProcess()
//----------------------------------------------------------------------------------------

bool ProcessTable::detachProcess(SpaceId id, int exitValue)
{
	// Controlamos que exista el proceso.

	if (id >= MAX_PROCESS || id < 0 || processList[id] == NULL)
	{
		DEBUG('y', "[PROCESSTABLE]: Can't detach process, invalid or free descriptor!\n");
		return false;
	}
	else if (processList[id]->status == DEAD)
	{
		DEBUG('y', "[PROCESSTABLE]: Can't detach process, process is already dead!\n");
		return false;
	}

	// Si el proceso existe, seteamos su valor de retorno y lo marcamos como DEAD.

	processList[id]->status = DEAD;
	processList[id]->exitValue = exitValue;

	DEBUG('y', "[PROCESSTABLE]: Process detached from decriptor %d, exitValue=%d.\n",
          id, exitValue);

	return true;
}

//----------------------------------------------------------------------------------------
// ProcessTable::getExitValue()
//----------------------------------------------------------------------------------------

bool ProcessTable::getExitValue(SpaceId id, int& exitValue)
{
	// Controlamos tener un valor de retorno.

	if (id >= MAX_PROCESS || id < 0 || processList[id] == NULL)
	{
		DEBUG('y', "[PROCESSTABLE]: Can't get exit value, invalid or free descriptor!\n");
		return false;
	}
	else if (processList[id]->status == ALIVE)
	{
		DEBUG('y', "[PROCESSTABLE]: Can't get exit value, process still alive!\n");
		return false;
	}

	// Si todo esta bien devolvemos el valor de retorno y liberamos el descriptor.

	exitValue = processList[id]->exitValue;
	delete processList[id];
	processList[id] = NULL;

	DEBUG('y', "[PROCESSTABLE]: Descriptor %d is now free again.\n", id);
	return true;
}

//----------------------------------------------------------------------------------------
// ProcessTable::getThread()
//----------------------------------------------------------------------------------------

Thread* ProcessTable::getThread(SpaceId id)
{
	// Controlamos que exista el thread.

	if (id >= MAX_PROCESS || id < 0 || processList[id] == NULL)
	{
		DEBUG('y', "[PROCESSTABLE]: Can't get thread, invalid (or free) descriptor!\n");
		return NULL;
	}

	// Retornamos el puntero al thread.

	return processList[id]->thread;
}

//----------------------------------------------------------------------------------------
// ProcessTable::getSpaceId()
//----------------------------------------------------------------------------------------

SpaceId ProcessTable::getSpaceId(Thread *thread)
{
	for (int i = 0; i < MAX_PROCESS; i++)
		if (processList[i] != NULL && processList[i]->thread == thread)
		{
			DEBUG('y', "[PROCESSTABLE]: Space id %d found for thread %s!\n", i, thread->getName());
			return i;
		}

	return -1;
}
