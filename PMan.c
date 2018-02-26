#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <inttypes.h>
#include <readline/readline.h>

/*-------------------Node for linked list structure---------------*/
typedef struct ProcessBag{
  pid_t pid;
  int isRunning;
  char* process;
  struct ProcessBag* next;
} ProcessBag;


#define userInputSize 100
#define bufferSize 1000

ProcessBag* head = NULL;

/* function to tell if the pid that user typed is valid
1 represent true
0 represent flase */

int isExistingPid(pid_t pid){
  ProcessBag* cursor = head;
  while(cursor != NULL){
    if(cursor->pid == pid){
      return 1;
    }
    cursor = cursor->next;
  }
  return 0;
}

/*---------------Linkedlist functions-----------------*/

void addProcessToList(pid_t pid,char* process){
  ProcessBag* pb = (ProcessBag*)malloc(sizeof(ProcessBag));
  pb->pid = pid;
  pb->process = process;
  pb->isRunning = 1;
  pb->next = NULL;
  if(head == NULL){
    head = pb;
  }else{
    ProcessBag* cursor = head;
    while (cursor->next != NULL) {
      cursor = cursor->next;
    }
    cursor->next = pb;
  }
}

void deleteProcessFromList(pid_t pid){
  if(isExistingPid(pid)==0){
    return;
  }
  ProcessBag* cursor_a = head;
  ProcessBag* cursor_b = NULL;
  while(cursor_a != NULL){
    if(cursor_a->pid == pid){
      if(cursor_a == head){
        head = head->next;
      }else{
        cursor_b->next = cursor_a->next;
      }
      free(cursor_a);
      return;
    }
    cursor_b = cursor_a;
    cursor_a = cursor_a->next;
  }
}

ProcessBag* getBagFromList(pid_t pid){
  if(isExistingPid(pid)==0){
    return NULL;
  }
  ProcessBag* cursor = head;
  while (cursor != NULL) {
    if(cursor->pid == pid){
      return cursor;
    }
    cursor = cursor->next;
  }

  return NULL;
}

/*-------------command functions----------------
there are six commands need to implement,including:
bglist
bg
bgstop
bgkill
pstat
bgstart  */

void bglist(){
  ProcessBag* pb = head;
  int count = 0;
  while(pb != NULL){
    printf("%d:\t %s\n",pb->pid,pb->process);
    pb = pb->next;
    count++;
  }
  printf("Total background jobs:\t %d\n",count);


}

void bg(char** fileName){
  pid_t pid = fork();
  if(pid == 0){
    execvp(fileName[1],fileName+1);
    printf("execvp failed\n");
    exit(1);
  }
  else if(pid < 0){
    printf("fork failed\n");
  }
  else if(pid > 0){
    printf("Started process %d\n",pid);
    addProcessToList(pid,fileName[1]);
    usleep(10);
  }
}


void bgstop(pid_t pid){
  if(isExistingPid(pid)==0){
    printf("Process %d does not exist\n",pid);
    return;
  }
  int returnValue = kill(pid,SIGSTOP);
  if(returnValue == 0){
    sleep(1);
  }else{
    printf("bgstop failed\n");
  }

}


void bgkill(pid_t pid){

  if(isExistingPid(pid)==0){
    printf("Process %d does not exist\n",pid);
    return;
  }
  int returnValue = kill(pid,SIGTERM);
  if(returnValue == 0){
    sleep(1);
  }else{
    printf("bgkill failed\n");
  }

}


void pstat(pid_t pid){
  if(isExistingPid(pid) == 0){
    printf("Process %d does not exist\n",pid);
    return;
  }
  FILE* file;
  char* token;
  char path[bufferSize];
  char stat[bufferSize];
  char* statArray[bufferSize];
  char status[64][bufferSize];
  int i, voluntary, nonvoluntary;

  sprintf(path,"/proc/%d/status",pid);
  if(access(path,F_OK) == -1){
    return;
  }
  file = fopen(path,"r");
  i = 0;
  while(fgets(status[i],bufferSize,file) != NULL){
    i++;
  }

  sscanf(status[i-2],"voluntary_ctxt_switches: %d",&voluntary);
  sscanf(status[i-1],"nonvoluntary_ctxt_switches: %d",&nonvoluntary);

  fclose(file);
  sprintf(path,"/proc/%d/stat",pid);
  file = fopen(path,"r");
  fgets(stat,sizeof(stat),file);

  token = strtok(stat," ");
  i=0;
  while(token != NULL){
    statArray[i] = token;
    token = strtok(NULL," ");
    i++;
  }
  fclose(file);

  printf("pid: %s\n", statArray[0]);
  printf("comm: %s\n", statArray[1]);
  printf("state: %s\n", statArray[2]);
  printf("utime: %f\n", atof(statArray[13]) / sysconf(_SC_CLK_TCK));
  printf("stime: %f\n", atof(statArray[14]) / sysconf(_SC_CLK_TCK));
  printf("rss: %s\n", statArray[23]);
  printf("voluntary_ctxt_switches: %d\n", voluntary);
  printf("nonvoluntary_ctxt_switches: %d\n", nonvoluntary);

}

void bgstart(pid_t pid){

  if(isExistingPid(pid)==0){
    printf("Process %d does not exist\n",pid);
    return;
  }
  int returnValue = kill(pid,SIGCONT);
  if(returnValue == 0){
    sleep(1);
  }else{
    printf("bgstart failed\n");
  }

}

/*-------------------update the process status--------------------- */
void updateProcess(){
  int status;
  while(1){
    pid_t pid = waitpid(-1,&status,WNOHANG|WUNTRACED|WCONTINUED);
    if(pid >0){
      if(WIFSTOPPED(status)){
        printf("Background Process %d was stopped\n",pid);
        ProcessBag* pb = getBagFromList(pid);
        pb->isRunning = 0;
      }

      else if(WIFCONTINUED(status)){
        printf("Background process %d was started\n",pid);
        ProcessBag* pb = getBagFromList(pid);
        pb->isRunning = 1;
      }

      else if(WIFSIGNALED(status)){
        printf("Background process %d was killed\n",pid);
        deleteProcessFromList(pid);
      }

      else if(WIFEXITED(status)){
        printf("Background process %d terminated\n",pid);
        deleteProcessFromList(pid);
      }

    }else{
      break;
    }
  }
  return;

}

/*-----------Main--------------
process input and call different command functions */


int main(void) {

  char* input[userInputSize];
  char* buffer;
  int i ;

  while(1){
    updateProcess();
    buffer = readline("PMan:  > ");

    if(strcmp(buffer,"") == 0){
      printf("Please type in command\n");
      continue;
    }

    char* token = strtok(buffer," ");
    i=0;
    while (token != NULL) {
      input[i] = token;
      token = strtok(NULL," ");
      i++;  //printf("%s\n",input[i]);
    }
    input[i] = NULL;

    if(strcmp(input[0],"bg") == 0){
      if(input[1] == NULL){
        printf("Type in a file name after bg and try again\n");
      }else{
        bg(input);
      }

    }

    else if(strcmp(input[0],"bglist") == 0){
      bglist();
    }

    else if(strcmp(input[0],"bgkill" ) == 0){
      if(input[1] == NULL){
        printf("Error:Invalid pid\n");
      }else{
        pid_t pid = atoi(input[1]);
        bgkill(pid);
      }
    }

    else if(strcmp(input[0],"bgstop" ) == 0){
      if(input[1] == NULL){
        printf("Error:Invalid pid\n");
      }else{
        pid_t pid = atoi(input[1]);
        bgstop(pid);
      }
    }

    else if(strcmp(input[0],"pstat" ) == 0){
      if(input[1] == NULL){
        printf("Error:Invalid pid\n");
      }else{
        pid_t pid = atoi(input[1]);
        pstat(pid);
      }
    }
    else if(strcmp(input[0],"bgstart" ) == 0){
      if(input[1] == NULL){
        printf("Error:Invalid pid\n");
      }else{

        pid_t pid = atoi(input[1]);
        bgstart(pid);
      }

    }

    else{
      printf("PMan:  > %s:\tcommand not found\n", input[0]);


    }
    updateProcess();
  }
  return 0;
}
