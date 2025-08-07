#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../kernel/fcntl.h"
#include "../kernel/fs.h"
#include "../kernel/param.h"
#include "user.h"

int main(int argc, char *argv[]) {

  while(1){
    fprintf(1, ">>>");

    
    char formatedBuffer[512];
    char inputBuffer[500];
    char *argumentBuffer[MAXARG];
    int argCount = 0;
    int opCount = 0;
    int numOfPipes = 0;

    // Clear the formated buffer
    for (int i = 0; i < 512; i++)
    {
      formatedBuffer[i] = 0;
    }

    //read user input
    int inputSize = read(0, inputBuffer, sizeof(inputBuffer));

    //format inputBuffer
    int semicolonCount = 0;
    for(int i = 0; i < inputSize; i++){
      if(inputBuffer[i] == ';'){
        formatedBuffer[i+semicolonCount] = ' ';
        semicolonCount++;
      }
      formatedBuffer[i+semicolonCount] = inputBuffer[i];
    }
    inputSize += semicolonCount;


    char *operatorQueue[5];
    int subArgLength[5];
    subArgLength[0] = 0;
    subArgLength[1] = 0;
    subArgLength[2] = 0;
    subArgLength[3] = 0;
    subArgLength[4] = 0;
    char **commands[6];

    //flag for checking if iterating through a word. 0 = false, 1 = true
    int wordFound = 0;

    for(int i = 0; i < inputSize; i++){

      if(formatedBuffer[i] != ' ' && wordFound == 0){
        argumentBuffer[argCount++] = &formatedBuffer[i];
        wordFound = 1;
      }else if ((formatedBuffer[i] == ' ' || formatedBuffer[i] == '\n') && wordFound == 1){
        formatedBuffer[i] = '\0';
        wordFound = 0;
      }
    }

    // TODO: check for pointer/array overflow
    // Split args into commands
    for(int i = 0; i < argCount; i++){
      if(*argumentBuffer[i] == '|'){
        numOfPipes++;
      }
      if(*argumentBuffer[i] == '|' || *argumentBuffer[i] == '>' || *argumentBuffer[i] == '<' || *argumentBuffer[i] == ';'){
        operatorQueue[opCount] = argumentBuffer[i];
        commands[opCount] = malloc(sizeof(char*) * (subArgLength[opCount] + 2));
        for(int subArg=0; subArg < subArgLength[opCount]; subArg++){
          commands[opCount][subArg] = argumentBuffer[i-subArgLength[opCount]+subArg];
        }
        commands[opCount][subArgLength[opCount]] = '\0';

        opCount++;
      }else{
        subArgLength[opCount]++;
      }
    }
    commands[opCount] = malloc(sizeof(char*) * (subArgLength[opCount] + 2));
    for(int subArg=0; subArg < subArgLength[opCount]; subArg++){
      commands[opCount][subArg] = argumentBuffer[argCount-subArgLength[opCount]+subArg];
    }
    commands[opCount][subArgLength[opCount]] = '\0';


    int p[4][2];
    int systemPipeCount = 0;
    //int pipeOpen = 0;

    //command execution
    for(int i = 0; i < (opCount + 1); i++){
      
      //command with more than 1 pipe
      if (numOfPipes == 2 && systemPipeCount < 3){

        // first session of pipes
        if(systemPipeCount == 0){

          if(*operatorQueue[i] == '<'){
            pipe(p[systemPipeCount]);
            int pid = fork();
            if (pid > 0){ // Parent
              close(p[systemPipeCount][1]);
              systemPipeCount++;
              i++;
            } else if (pid == 0) { // Child
              //make stdin the file
              close(0);
              open(commands[i+1][0], O_CREATE | O_RDONLY);
              //make stdout the pipe write end
              close(1);
              dup(p[systemPipeCount][1]);
              close(p[systemPipeCount][1]);
              close(p[systemPipeCount][0]);
              exec(commands[i][0], commands[i]);
            }
          
          }else if(*operatorQueue[i] == '|'){
            pipe(p[systemPipeCount]);
            int pid = fork();
            if (pid > 0){// Parent
              close(p[systemPipeCount][1]);
              systemPipeCount++;
            }else if (pid == 0){ // Child
              //make stdout the pipe write end
              close(1);
              dup(p[systemPipeCount][1]);
              close(p[systemPipeCount][1]);
              close(p[systemPipeCount][0]);
              exec(commands[i][0], commands[i]);
            }
          }
        
        // second session of pipes
        }else if(systemPipeCount == 1){
          pipe(p[systemPipeCount]);
          int pid = fork();
          if(pid > 0){ // Parent
            close(p[systemPipeCount][1]);
            systemPipeCount++;
            wait(0);
          }else if(pid == 0){ // Child
            close(1);
            dup(p[systemPipeCount][1]);
            close(0);
            dup(p[systemPipeCount-1][0]);
            exec(commands[i][0], commands[i]);
          }

        // third session of pipes
        }else if(systemPipeCount == 2){
          if(*operatorQueue[i] == '>'){
            int pid = fork();
            if(pid > 0){ // Parent
              wait(0);
              i++;
            }else if(pid == 0){ // Child
              close(0);
              dup(p[systemPipeCount-1][0]);
              close(1);
              open(commands[i+1][0], O_CREATE | O_WRONLY);
              wait(0);
              wait(0);
              exec(commands[i][0], commands[i]);
            }
          }else{
            int pid = fork();
            if(pid > 0){ // Parent
              wait(0);
              i++;
            }else if(pid == 0){ // Child
              close(0);
              dup(p[systemPipeCount-1][0]);
              exec(commands[i][0], commands[i]);
            }
          }
          wait(0);
        }
       
      //cd command
      }else if(strcmp(argumentBuffer[0], "cd") == 0){
        chdir(argumentBuffer[1]);
      
      // < > command
      }else if(*operatorQueue[i] == '>' || *operatorQueue[i] == '<'){

        if(*operatorQueue[i] == '>'){

          int pid = fork();
          if (pid > 0){ // Parent
            wait(0);
            i++;
          }else if (pid == 0) { // Child
            close(1);
            open(commands[i+1][0], O_CREATE | O_WRONLY);
            exec(commands[i][0], commands[i]);
          }

        }else if(*operatorQueue[i] == '<'){

          int pid = fork();
          if (pid > 0){ // Parent
            wait(0);
            i++;
          } else if (pid == 0) { // Child
            close(0);
            open(commands[i+1][0], O_RDONLY);
            exec(commands[i][0], commands[i]);
          }
        }

      // single pipe command
      }else if(*operatorQueue[i] == '|'){

        int p[2];
        pipe(p);
        int pid = fork();

        if(pid>0){
          close(p[1]);
          int pid2 = fork();
          if(pid2==0){
            close(0);
            dup(p[0]);
            close(p[0]);
            exec(commands[i+1][0], commands[i+1]);
          }
        
          
        }else if(pid==0){
          close(1);
          dup(p[1]);
          close(p[0]);
          close(p[1]);
          exec(commands[i][0], commands[i]);
        }
        wait(0);
        wait(0);
        i++;
      
      // single command
      }else{
        int pid = fork();
        if (pid > 0){ // Parent
          wait(0);
        } else if (pid == 0) { // Child
          exec(commands[i][0], commands[i]);
        }
      }
    }


    // Clear the argument buffer
    for (int i = 0; i < argCount; i++)
    {
      argumentBuffer[i] = 0;
    }
    // clear command malloc
    for(int i = 0; i < opCount; i++){
      free(commands[i]);
    }
  }
  return 0;
}