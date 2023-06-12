#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

/** structures for Command and Environmental variables **/
typedef struct{
    char *name;
    struct tm time;
    int value;
}Command;

typedef struct{
    char *name;
    char *value;
}EnvVar;

static Command **commandLog;
static int commandSize = 0;
int MAX_COMMAND_SIZE = 10;

EnvVar **envVar;
int envVarSize = 0;
int MAX_ENVVAR_SIZE = 10;

int BUFFER_SIZE = 1024;

/*Free log memeory when exit*/
void freeLog(){
    for(int i = 0; i < commandSize; i++){
        free(commandLog[i]->name);
        free(commandLog[i]);
    }
    free(commandLog);
    commandSize = 0;
}

void freeVar(){
    for(int i = 0; i < envVarSize; i++){
        free(envVar[i]->name);
        free(envVar[i]->value);
        free(envVar[i]);
    }
}

/** Exit*/
void exiter(){
    freeLog();
    freeVar();
    printf("Bye!\n");
    printf("\033[0m");
    exit(0);
}

/*Log operations*/
/** Add command to log */
void addLog(Command *cmd){
    if(commandSize == 0){
        MAX_COMMAND_SIZE = 10;
        commandLog = malloc(MAX_COMMAND_SIZE * sizeof(Command*));
    }
    if(commandSize >= MAX_COMMAND_SIZE){
        MAX_COMMAND_SIZE *= 2;
        commandLog = realloc(commandLog, MAX_COMMAND_SIZE * sizeof(Command*));
    }
    commandLog[commandSize++] = cmd;
}

/*Print log to terminal*/
int printLog(){
    if(commandSize == 0){
        printf("No commands in log\n");
        return 1;
    }
    for(int i = 0; i < commandSize; i++){
        printf("%s %s %d\n",asctime(&commandLog[i]->time), commandLog[i]->name, commandLog[i]->value);
    }
    return 0;
}

/*Enviornment Operations*/
/*** Add enviornment variable ***/
int addVar(EnvVar *var){
    if(envVarSize == 0){
        envVar = malloc(MAX_ENVVAR_SIZE * sizeof(EnvVar*));
    }
    if(envVarSize >= MAX_ENVVAR_SIZE){
        MAX_ENVVAR_SIZE *= 2;
        envVar = realloc(envVar, MAX_ENVVAR_SIZE * sizeof(EnvVar*));
    }
    envVar[envVarSize++] = var;
    return 0;
}

// Updates EnvVar's value and return 0 if sucess
int updateVar(EnvVar *var){
    for(int i = 0; i < envVarSize; i++){
        if(strcmp(envVar[i]->name, var->name)==0){
            free(envVar[i]);
            envVar[i] = var;
            return 0;
        }
    }
    return 1;
}

// Checks if an EnvVar exists, return 1 if found
int checkVar(char *name){
    for(int i = 0 ;i < envVarSize; i++){
        if(strcmp(envVar[i]->name,name)==0){
            return 1;
        }
    }
    return 0;
}

// Gets EnvVar value return 0 after finish.
char *getVar(char* name){
    for(int i = 0 ;i < envVarSize; i++){
        if(strcmp(envVar[i]->name,name)==0){
            return envVar[i]->value;
        }
    }
    return 0;
}

/*check if argument is setting variable*/
int isSetEnvVar(char *cmd){
    int flag = 0;
    char c;
    for(int i = 0; ; i++){
        c = cmd[i];
        if(c == '/' && flag == 0){ // Found '/' before '='
            return -1;
        }
        if(c == '='){ // Found '='
            flag = 1;
        }
        if(c == '\0'){ // Reached the end of the string
            return 0;
        }else if(flag == 1){ // Reached the value after '='
                return 1;
        }
    }
    return 0;
}

/*check if argument is variable*/
int isEnvVar(char *cmd){
    if(cmd[0] == '$'){
        return 1;
    }
    return 0;
}

/** Read command from terminal*/
char *getCmd(void){
    int len = 0;
    char *buffer = malloc(BUFFER_SIZE * sizeof (char));
    char c;
    if(!buffer) return NULL;
    int isEmptyLine = 1;
    while(1){
        c = getchar();
        if(c == EOF){
            free(buffer);
            return NULL;
        }else if( c == '\n'){
            buffer[len] = '\0';
            // Remove the trailing newline character
            if (len > 0 && buffer[len - 1] == '\r')
                buffer[len - 1] = '\0';
            for (int i = 0; i < len; i++) {
                if (!(buffer[i] == ' ')) {
                    isEmptyLine = 0;
                    break;
                }
            }
            if (isEmptyLine)
                return NULL;
            if (strlen(buffer) == 0)
                return NULL;
            return buffer;
        }
        if(len >= BUFFER_SIZE){
            BUFFER_SIZE += 1024;
            buffer = realloc(buffer,BUFFER_SIZE);
            if(!buffer){
                return NULL;
            }
        }
        buffer[len++] = c;
    }
}

/** Check command from file if it is invalid*/
int checkCmd(char *cmdline){
    int len = strlen(cmdline);
    int isEmptyLine = 1;
    for (int i = 0; i < len; i++) {
        char c = cmdline[i];
        if(c == '\n'){
            cmdline[i] = '\0';
            // Remove the trailing newline character
            for (int i = 0; i < len - 1; i++) {
                if (!(cmdline[i] == ' ')) {
                    isEmptyLine = 0;
                    break;
                }
            }
            if (isEmptyLine){return 0;}
            if (strlen(cmdline) == 0){return 0;}
        }
    }
    return 1;
}

/** tokenizes the input using space, tab and newline */
void stringParser(char *parse, char **token){
    int i = 0;
    token[i]= strtok(parse," \n\t");
    while( token[i] != NULL && i < 100) {
        i++;
        token[i] = strtok(NULL, " \n\t");
    }
}

/** checks if input command is a buildin command */
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

/** print operation **/
int printer(char **parse){
        int i = 1;
        while (parse[i] != NULL) {
            if(isEnvVar(parse[i])){ //Format check
                if(checkVar(parse[i])) {//check if arguement is EnvVar
                    printf("%s ", getVar(parse[i]));
                }
                else{
                    printf("Error: No Enviornment Variable %s found.\n", parse[i]);
                    return 0;
                }
            }else {printf("%s ", parse[i]);}
            i++;
        }
        printf("\n");
        return 0;
}

/** Changes the theme of the output */
int changeTheme(char *color){
    if(color == NULL){
        printf("unsupported theme\n");
        return -1;
    }else if(strcmp(color, "red") == 0){
        printf("\033[0;31m");
    }else if(strcmp(color, "green") == 0){
        printf("\033[0;32m");
    }else if(strcmp(color, "blue") == 0){
        printf("\033[0;34m");
    }else{
        printf("unsupported theme\n");
        return -1;
    }
    return 0;
}

/** execute none build-in commands */
int nonBuildCmd(char** parse){
    pid_t pid = fork();
    if (pid < 0) {
        // Fork failed
        return 1;
    } else if (pid == 0) {
        // Child process
        if (execvp(parse[0], parse) < 0) {
            // execvp failed, command not recognized or cannot be executed
            _exit(1);
        }
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            // The child process exited with a non-zero status
            return 1;
        }
    }
    return 0;
}




int main( int argc, char *argv[]) {
    time_t rtime;
    int sw;
    if(argc < 2) {
        while (1) {
            char *cmdline,*args[100];
            printf("cshell$ ");
            cmdline = getCmd();
            if (cmdline == NULL) {
                printf("Missing keyword or command, or permission problem\n");
                continue;
            }
            Command *cmd = malloc(sizeof (Command));
            time(&rtime);
            cmd->time = *localtime(&rtime);
            stringParser(cmdline, args);
            cmd->name = strdup(args[0]);
            if ((sw = checkBuiltInCmd(args[0])) > 0) {
                if(sw == 1){
                    exiter();
                }else if(sw == 2){
                    cmd->value = printLog();
                }else if(sw == 3){
                    cmd->value = printer(args);
                }else if(sw == 4){
                    cmd->value = changeTheme(args[1]);
                }
            } else if (isEnvVar(args[0])) {
                int res = isSetEnvVar(args[0]);
                if(res == 1){
                    EnvVar *var = malloc(sizeof(EnvVar));
                    var->name = strdup(strtok(args[0], "="));
                    var->value = strdup(strtok(NULL, "="));
                    if(checkVar(var->name)==0){
                        cmd->value = addVar(var);
                    }else{
                        cmd->value = updateVar(var);
                    }
                }else if(res == 0){
                    printf("Variable value expected\n");
                    cmd->value = -1;
                }else if(res == -1){
                    printf("it has '/' in var name\n");
                    cmd->value = -1;
                }
            } else {
                int res = nonBuildCmd(args);
                if (res == 0) {
                    cmd->value = 0;
                } else {
                    printf("Missing keyword or command, or permission problem\n");
                    cmd->value = -1;
                }
            }
            addLog(cmd);
        }
    }else{
        /*Script Mode*/
        char *args[100], cmdline[1024];
        if(strstr(argv[1], ".txt")==NULL){strcat(argv[1], ".txt");}
        FILE *fp = fopen(argv[1], "r");
        if(fp == NULL){
            printf("Unable to read script file: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        while (fgets(cmdline, sizeof(cmdline),fp)!=NULL){
            if(!checkCmd(cmdline)){
                printf("Missing keyword or command, or permission problem\n");
                continue;
            }
            Command *cmd = malloc(sizeof (Command));
            time(&rtime);
            cmd->time = *localtime(&rtime);
            stringParser(cmdline, args);
            cmd->name = strdup(args[0]);
            //Remove newline character from command
            if (cmd->name[strlen(cmd->name) - 1] == '\n')
            cmd->name[strlen(cmd->name) - 1] = '\0';

            if ((sw = checkBuiltInCmd(args[0])) > 0) {
                if(sw == 1){
                    exiter();
                }else if(sw == 2){
                    cmd->value = printLog();
                }else if(sw == 3){
                    cmd->value = printer(args);
                }else if(sw == 4){
                    cmd->value = changeTheme(args[1]);
                }
            } else if (isEnvVar(args[0])) {
                int res = isSetEnvVar(args[0]);
                if(res == 1){
                    EnvVar *var = malloc(sizeof(EnvVar));
                    var->name = strdup(strtok(args[0], "="));
                    var->value = strdup(strtok(NULL, "="));
                    if(checkVar(var->name)==0){
                        cmd->value = addVar(var);
                    }else{
                        cmd->value = updateVar(var);
                    }
                }else if(res == 0){
                    printf("Variable value expected\n");
                    cmd->value = -1;
                }else if(res == -1){
                    printf("it has '/' in var name\n");
                    cmd->value = -1;
                }
            } else {
                int res = nonBuildCmd(args);
                if (res == 0) {
                    cmd->value = 0;
                } else {
                    printf("Missing keyword or command, or permission problem\n");
                    cmd->value = -1;
                }
            }
            addLog(cmd);
        }
        exiter();
    }
    return 0;
}
