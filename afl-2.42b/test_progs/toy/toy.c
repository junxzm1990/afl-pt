#include<stdlib.h>
#include<assert.h>
#include<string.h>
#define SIZE 128
void bug(){
  assert(0);
}

void main(int argc, char *argv[], char *envp[]){
  char src[SIZE];
  char dst[SIZE] = {0};
  char target[]="aaaaabbbbb";
  int i;
  read(0, src, SIZE);
  for(i = 0; i < SIZE; i++){
    if(src[i] == 'a'){
      dst[i] = src[i];
      continue;
    }
    if(src[i] == 'b'){
      dst[i] = src[i];
      continue;
    }
    break;
  }
  if(!strncmp(dst, target, 10)){
    bug(); // Buggy function
  }
}
