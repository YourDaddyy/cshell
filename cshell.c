#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>


/** defining my structures for log and Environmental variables **/
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

/** Exit CSHELL*/
void exiter(){
    printf("Bye!");
    exit(0);
}

/** Helper funcs for log */
void addlog(Command *cmd){
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

void freelog(){
    for(int i = 0; i < commandSize; i++){
        free(commandLog[i]->name);
        free(commandLog[i]);
    }
    free(commandLog);
    commandSize = 0;
}

/*** Helper funcs for EnvVar ***/
int addEVar(EnvVar *var){
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

// Updates EnvVar's value and return 0
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

// Checks if an EnvVar exists, return 1 if it does
int checkVar(char *name){
    for(int i = 0; i < envVarSize; i++){
        if(strcmp(envVar[i]->name, name) == 0){
            return 1;
        }
    }
    return 0;
}

// Gets EnvVar value
char *getEVar(char* name){
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

/** gets user input */
char *getCmd(void){
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

/** tokenizes the input using space, tab and newline */
void stringparser(char *parse, char **token){
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

/** print cmd **/
int printer(char **parse){
        int i = 1;
        while (parse[i] != NULL) {
            if(isEnvVar(parse[i])){ //Format check
                if(checkVar(parse[i])) {//check if arguement is EnvVar
                    printf("%s ", getEVar(parse[i]));
                }
                else{
                    exiter();
                }
            }else {
                printf("%s ", parse[i]);
            }
            i++;
        }
        printf("\n");
        return 0;
}

/** Changes the theme of the output */
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

/** execute none build-in commands */
int nonBuildCmd(char** parse){
    int pipein[2];
    int pipeout[2];
    if(pipe(pipein)<0){
        printf("Unable to initialize input pipe, Please try again\n");
        return 1;
    }
    if (pipe(pipeout)<0) {
        printf("Unable to initialize output pipe, Please try again\n");
        return 1;
    }
    close(pipein[0]);
    if(parse[1]!=NULL) {
        int i =1;
        while (parse[i] != NULL) {
            if (checkEVar(parse[i])) {
                write(pipein[1], getEVar(parse[i]), strlen(parse[i]));
            } else {
                write(pipein[1], parse[i], strlen(parse[i]));
            }
            i++;
        }
    }
    close(pipein[1]);
    pid_t p1 = fork();
    if(p1 == -1) {
        printf("Unable to fork a child, Please try again\n");
        return 1;
    }else if(p1 == 0) {
        close(pipein[1]);
        dup2(pipein[0],STDIN_FILENO);
        close(pipein[0]);
        int fail = 0;
        if (execvp(parse[0], parse) < 0) {
            fail = 1;
        }
        close(pipeout[0]);
        write(pipeout[1], &fail, sizeof(fail));
        close(pipeout[1]);
        exit(0);
    }else {
        wait(NULL);
        int ret;
        close(pipeout[1]);
        read(pipeout[0], &ret, sizeof(ret));
        close(pipeout[0]);
        if(ret == 1) {
            return 1;
        }else{
            return 0;
        }
    }
}

int main( int argc, char *argv[]) {
    time_t rnTime;
    int sw;
    if(argc < 2) {
        printPrompt();
        while (1) {
            char *cmdline,*args[100];
            printf("cshell$ ");
            cmdline = getCmd();
            Command *cmd = malloc(sizeof (Command));
            time(&rnTime);
            cmd->time = *localtime(&rnTime);
            stringparser(cmdline, args);
            cmd->name = strdup(args[0]);
            if ((sw = checkBICmd(args[0])) > 0) {
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
            } else if (isEnvVar(args[0]) && valEnvVar(args[0])) {
                EnvVar *var = malloc(sizeof(EnvVar));
                var->name = strdup(strtok(args[0], "="));
                var->value = strdup(strtok(NULL, "="));
                if(checkVar(var->name) == 0){
                    cmd->value = addVar(var);
                }else{
                    cmd->value = updateVar(var);
                }
            } else {
                if(!nonBuildCmd(args)){
                    cmd->value = 0;
                }else{
                    exiter();
                    cmd->value = 1;
                }
            }
            addlog(cmd);
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
        while (fgets(cmdline, sizeof(cmdline),fp)!=NULL){
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