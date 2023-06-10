#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


typedef struct{
    char* name;
    char* value;
}EnvVar;

typedef struct{
    char* name;
    struct tm time;
    int value;
}Command;

void printoutPromot(){
    
}

int main(int argc, char *argv[]){
    time_t rnTime;
    int sw;
    if(argc < 2){
        printPromot();
        while(1){
            char *cmdline, *args[100];
            printf("cshell$ ");
            cmdline = getCmd();
            Command *cmd = malloc(sizeof(Command));
            time(&rnTime);
            cmd->time = *localtime(&rnTime);
            stringparser(cmdline, args);
            cmd->name = strdup(args[0]);
            if((sw = checkBICmd(args[0])) > 0){
                switch(sw){
                    case 1:
                        exiter();
                    case 2:
                        cmd->value = logPrint();
                        break;
                    case 3:
                        cmd->value = printer(args);
                        break;
                    case 4:
                        cmd->value = changeTheme(args[1]);
                        break;
                }
            }else if(isEnvVar(args[0]) && valEnvVar(args[0])){
                EnvVar *var = malloc(sizeof(EnvVar));
                var->name = strdup(strtok(args[0], "="));
                var->value = strdup(strtok(NULL, "="));
                if(checkEVar(var->name) == 0){
                    cmd->value = addEVar(var);
                }else{
                    cmd->value = upEVar(var);
                }
            }
        }
    }
}