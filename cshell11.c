#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

/*struct for command and enviornment variables*/
typedef struct{
    char* name;
    char* value;
}EnvVar;

typedef struct{
    char* name;
    struct tm time;
    int value;
}Command;

static Command **commandLog;
static int commandSize = 0;
int MAX_COMMAND_SIZE = 10;

EnvVar **envVar;
int envVarSize = 0;
int MAX_ENVVAR_SIZE = 10;

void exiter(){
    printf("Bye!\n");
    exit(0);
}

/*Log Operation*/
void addLog(Command *cmd){
    if(commandSize == 0){
        MAX_COMMAND_SIZE = 10;
        commandLog = malloc(MAX_COMMAND_SIZE * sizeof(Command*));
        if(commandLog == NULL){
            fprintf(stderr, "cshell: allocation error\n");
            exiter();
        }
    }
    if(commandSize >= MAX_COMMAND_SIZE){
        MAX_COMMAND_SIZE *= 2;
        commandLog = realloc(commandLog, MAX_COMMAND_SIZE * sizeof(Command*));
        if(commandLog == NULL){
            fprintf(stderr, "cshell: allocation error\n");
            exiter();
        }
    }
    commandLog[commandSize++] = cmd;
}

void freeLog(){
    for(int i = 0; i < commandSize; i++){
        free(commandLog[i]->name);
        free(commandLog[i]);
    }
    free(commandLog);
    commandSize = 0;
}

int printLog(){
    if(commandSize == 0){
        printf("cshell: log: no commands in log\n");
        return 1;
    }
    for(int i = 0; i < commandSize; i++){
        printf("%s\n",asctime(&commandLog[i]->time));
        printf(" %s %d\n", commandLog[i]->name, commandLog[i]->value);
    }
    return 0;
}

/*EnvVar Operation*/
int addVar(EnvVar *var){
    if(envVarSize == 0){
        envVar = malloc(MAX_ENVVAR_SIZE * sizeof(EnvVar*));
        if(envVar == NULL){
            fprintf(stderr, "cshell: allocation error\n");
            exiter();
        }
    }
    if(envVarSize >= MAX_ENVVAR_SIZE){
        MAX_ENVVAR_SIZE *= 2;
        envVar = realloc(envVar, MAX_ENVVAR_SIZE * sizeof(EnvVar*));
        if(envVar == NULL){
            fprintf(stderr, "cshell: allocation error\n");
            exiter();
        }
    }
    envVar[envVarSize++] = var;
    return 0;
}

int updateVar(EnvVar *var){
    for(int i = 0; i < envVarSize; i++){
        if(strcmp(envVar[i]->name, var->name) == 0){
            free(envVar[i]);
            envVar[i] = var;
            return 0;
        }
    }
    return 1;
}

int checkVar(char *name){
    for(int i = 0; i < envVarSize; i++){
        if(strcmp(envVar[i]->name, name) == 0){
            return 1;
        }
    }
    return 0;
}

char *getVar(char *name){
    for(int i = 0; i < envVarSize; i++){
        if(strcmp(envVar[i]->name, name) == 0){
            return envVar[i]->value;
        }
    }
    return NULL;
}

/*check if argument is set variable*/
int isSetEnvVar(char *cmd){
    char c;
    for(int i = 0; ; i++){
        c = cmd[i];
        if(c == '='){
            return 1;
        }else if(c == '\0'){
            return 0;
        }
    }
}

/*check if argument is variable*/
int isEnvVar(char *cmd){
    if(cmd[0] == '$'){
        return 1;
    }
    return 0;
}

char* getCmd(){
    int len = 0;
    char *buffer = malloc(1024 * sizeof(char));
    int buffer_size = 1024;
    char c;
    if(!buffer){
        fprintf(stderr, "cshell: allocation error\n");
        exiter ();
    } 
    while(1){
        c = getchar();
        if(c == EOF){
            free(buffer);
            return NULL;
        }else if(c == '\n'){
            buffer[len] = '\0';
            return buffer;
        }
        if(len >= buffer_size){
            buffer_size += 1024;
            buffer = realloc(buffer, buffer_size);
            if(!buffer){
                fprintf(stderr, "cshell: allocation error\n");
                exiter ();
            }
            buffer[len++] = c;
        }
    }
}

void string_Parser(char *parse, char**token){
    int i = 0;
    token[i] = strtok(parse, "\n\t");
    while(token[i] != NULL && i < 100){
        token[++i] = strtok(NULL, "\n\t");
    }
}

int checkBuiltInCmd(char *cmd){
    if(strcmp(cmd, "exit") == 0){
        return 1;
    }else if(strcmp(cmd, "log") == 0){
        return 2;
    }else if(strcmp(cmd, "print") == 0){
        return 3;
    }else if(strcmp(cmd, "theme") == 0){
        return 4;
    }
    return -1;
}

/*Print Opertion*/
int printer(char **parse){
    int i = 1;
    while(parse[i] != NULL){
        if(isEnvVar(parse[i])){
            if(checkVar(parse[i])){
                printf("%s ", getVar(parse[i]));
            }else{
                printf("Error: No Enviornment Variable %s found\n", parse[i]);
                exiter();
            }
        }else{
            printf("%s ", parse[i]);
        }
        i++;
    }
    printf("\n");
    return 0;
}

/*Change Theme Operation*/
int changeTheme(char * color){
    if(color == NULL){
        printf("cshell: theme: no color specified\n");
        return 1;
    }else if(strcmp(color, "red") == 0){
        printf("\033[0;31m");
    }else if(strcmp(color, "green") == 0){
        printf("\033[0;32m");
    }else if(strcmp(color, "blue") == 0){
        printf("\033[0;34m");
    }else{
        printf("cshell: unsupported theme\n");
        return 1;
    }
    return 0;
}



int nonBuildCmd(char **parse){
    return 0;
}

int main(int argc, char *argv[]){
    time_t rnTime;
    int sw;
    if(argc < 2){
        while(1){
            char *cmdline, *args[100];
            printf("cshell$ ");
            cmdline = getCmd();
            Command *cmd = malloc(sizeof(Command));
            time(&rnTime);
            cmd->time = *localtime(&rnTime);
            string_Parser(cmdline, args);
            cmd->name = strdup(args[0]);
            if((sw = checkBuiltInCmd(args[0])) > 0){
                if(sw == 1){
                    exiter();
                }else if(sw == 2){
                    cmd->value = printLog();
                    break;
                }else if(sw == 3){
                    cmd->value = printer(args);
                    break;
                }else if(sw == 4){
                    cmd->value = changeTheme(args[1]);
                    break;
                }
            }else if(isEnvVar(args[0]) && isSetEnvVar(args[0])){
                EnvVar *var = malloc(sizeof(EnvVar));
                var->name = strdup(strtok(args[0], "="));
                var->value = strdup(strtok(NULL, "="));
                if(checkVar(var->name) == 0){
                    cmd->value = addVar(var);
                }else{
                    cmd->value = updateVar(var);
                }
            }else{
                if(!nonBuildCmd(args)){
                    cmd->value = 0;
                }else{
                    exiter();
                    cmd->value = 1;
                }
            }
            addLog(cmd);
        }
    }else{
        /*Script mode*/
        char *args[100], cmdline[1024];
        if(strstr(argv[1], ".txt") == NULL){
            strcat(argv[1], ".txt");
        }
        FILE *fp = fopen(argv[1], "r");
        if(fp == NULL){
            printf("cshell: %s: No such file or directory\n", argv[1]);
            exiter();
        }
        while(fgets(cmdline, sizeof(cmdline), fp) != NULL){
            Command *cmd = malloc(sizeof(Command));
            time(&rnTime);
            cmd->time = *localtime(&rnTime);
            string_Parser(cmdline, args);
            cmd->name = strdup(args[0]);
            if(sw = checkBuiltInCmd(args[0]) > 0){
                if(sw == 1){
                    exiter();
                }else if(sw == 2){
                    cmd->value = printLog();
                    break;
                }else if(sw == 3){
                    cmd->value = printer(args);
                    break;
                }else if(sw == 4){
                    cmd->value = changeTheme(args[1]);
                    break;
                }
            }else if(isEnvVar(args[0]) && isSetEnvVar(args[0])){
                EnvVar *var = malloc(sizeof(EnvVar));
                var->name = strdup(strtok(args[0], "="));
                var->value = strdup(strtok(NULL, "="));
                if(checkVar(var->name) == 0){
                    cmd->value = addVar(var);
                }else{
                    cmd->value = updateVar(var);
                }
            }else{
                if(!nonBuildCmd(args)){
                    cmd->value = 0;
                }else{
                    exiter();
                    cmd->value = 1;
                }
            }
            addLog(cmd);
        }
    }
    return 0;
}