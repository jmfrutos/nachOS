#include "OpenFilesTable.h"
OpenFilesTable::OpenFilesTable(){
	openFiles = new int[MAX_FILES];
	openFilesMap = new BitMap(MAX_FILES);
	usage = 1;
	openFilesMap->Mark(0);
	openFilesMap->Mark(1);
}
OpenFilesTable::~OpenFilesTable(){
	delete openFiles;
	delete openFilesMap;
}
int OpenFilesTable::Open( int UnixHandle ){
	int nachosId = openFilesMap->Find();
	if(nachosId != -1){
		openFiles[nachosId] = UnixHandle;
	}
	else{ 
		printf("Can't open any more files\n");
	}
	return nachosId;
}
int OpenFilesTable::Close( int NachosHandle ){
	if(isOpened(NachosHandle)){
		openFiles[NachosHandle] = 0;
		openFilesMap->Clear(NachosHandle);
		return 1;
	}
	else {
		return -1;
	}
}
bool OpenFilesTable::isOpened( int NachosHandle ){
	return (openFilesMap->Test(NachosHandle));
}
int OpenFilesTable::getUnixHandle( int NachosHandle ){
	if(isOpened(NachosHandle)){
		return openFiles[NachosHandle];
	} 
	else {
		return -1;
	}
}
void OpenFilesTable::addThread(){
	++usage;
}
void OpenFilesTable::delThread(){
	--usage;
	if(usage == 0){
		for(int i=0; i<MAX_FILES;++i){
			Close(i);
		}
	}
}
void OpenFilesTable::Print(){
	printf("Archivos Abiertos: \n");
	for(int i=0; i<MAX_FILES;++i){
		if(isOpened(i)){
			printf("\tIDNachos: %d | IDUnix: %s\n",i,openFiles[i]);
		}
	}
}
