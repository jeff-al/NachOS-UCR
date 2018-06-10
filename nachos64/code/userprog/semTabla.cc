#include "semTabla.h"
#include <iostream>
using namespace std;

SemTabla::SemTabla(){
  semaforosMap = new BitMap(maxSem);

  for(int i = 0; i< maxSem; i++){
    semaforosAbiertos[i] = NULL;
  }
};

int SemTabla::Create(Semaphore *val){
  int space = semaforosMap->Find();
  semaforosAbiertos[space] = val;
  return space;
};


int SemTabla::Close(int id){
  int ret = 1;
  if(isCreate(id)){
    semaforosMap->Clear(id);
    semaforosAbiertos[id] = NULL;
  }
  else{
    ret = -1;
  }
  return ret;
};

bool SemTabla::isCreate(int id){
  bool ret = semaforosMap->Test(id);
  return ret;
};

Semaphore SemTabla::getSemaphore(int id){
  Semaphore ret = NULL;
  if(isCreate(id)){
    ret = semaforosAbiertos[id];
  }
  return ret;
};

void SemTabla::addThread(){
  usage++;
};

void SemTabla::delThread(){
  usage--;
};

void SemTabla::Print(){
  for(int i = 0; i < maxArch; i++ ){
    printf("Position: %i |  File Number : %i ", i, openFiles[i]);
  }
};
