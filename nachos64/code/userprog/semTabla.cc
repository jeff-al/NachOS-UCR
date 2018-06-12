#include "semTabla.h"
#include <iostream>
using namespace std;

SemTabla::SemTabla(){
  semaforosMap = new BitMap(maxSem);

  for(int i = 0; i< maxSem; i++){
    semaforosAbiertos[i] = -1;
  }
};

int SemTabla::Create(long val){ //direccion del semaforo
  int space = semaforosMap->Find();
  semaforosAbiertos[space] = val;
  return space;
};


int SemTabla::Close(int id){/*
  int ret = 1;
  if(isCreate(id)){
    semaforosMap->Clear(id);
    semaforosAbiertos[id] = -1;
  }
  else{
    ret = -1;
  }
  return ret;*/
};

bool SemTabla::isCreate(int id){
  bool ret = semaforosMap->Test(id);
  return ret;
};

long SemTabla::getSemaphore(int id){
  long ret = -1;
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
  for(int i = 0; i < maxSem; i++ ){
    printf("Position: %i |  File Number : %i ", i, semaforosAbiertos[i]);
  }
};
