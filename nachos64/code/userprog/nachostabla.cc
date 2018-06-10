#include "nachostabla.h"
#include <iostream>
using namespace std;

NachosOpenFilesTable::NachosOpenFilesTable(){
  openFilesMap = new BitMap(maxArch);
  int vec[maxArch];
  openFiles = vec;

  for(int i = 0; i < 3 ; i++){
    openFiles[i]= i;
    openFilesMap->Mark(i);
  }
  for(int i = 3; i < maxArch; i++ ){
   openFiles[i]= 0;
  }
};

int NachosOpenFilesTable::Open(int UnixHandle){
  int space = openFilesMap->Find();
  openFiles[space] = UnixHandle;
  return space;
};


int NachosOpenFilesTable::Close(int NachosHandle){
  int ret = 1;
  if(isOpened(NachosHandle) && NachosHandle > 2){
    openFilesMap->Clear(NachosHandle);
    openFiles[NachosHandle] = 0;
  }
  else{
    ret = -1;
  }
  return ret;
};

bool NachosOpenFilesTable:: isOpened(int NachosHandle){
  bool ret = openFilesMap->Test(NachosHandle);
  return ret;
};

int NachosOpenFilesTable::getUnixHandle(int NachosHandle){
  int ret = -1;
  if(isOpened(NachosHandle)){
    ret = openFiles[NachosHandle];
  }
  return ret;
};

void NachosOpenFilesTable::addThread(){
  usage++;
};

void NachosOpenFilesTable::delThread(){
  usage--;
};

void NachosOpenFilesTable::Print(){
  for(int i = 0; i < maxArch; i++ ){
    printf("Position: %i |  File Number : %i ", i, openFiles[i]);
  }
};
