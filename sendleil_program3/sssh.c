#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h> // for wait

#define MAXLENGTH 2048
#define MAXARGS 512

// stores shell child pids
int childPids[50] = {0};
int k = 0;

// fg process exit status
int exitStatus;

// tracks if we are in fg only mode
int fgOnly = 1;

// set after new bg process starts so that wifexited will not trigger
int newBackg = 1;


int eval(char **cmdl, char **inputOutput)
{
        // set CTRL-C action to exit function
        struct sigaction SIGINT_action = {0};
        SIGINT_action.sa_handler = SIG_DFL;
        sigfillset(&SIGINT_action.sa_mask);
        SIGINT_action.sa_flags = 0;

        // install signal handler
        sigaction(SIGINT, &SIGINT_action, NULL);

        // if there is no io redirection
        if ((inputOutput[0] == NULL) && (inputOutput[1] == NULL))
        {
                // forking model taken from Module 4 Process API- Monitoring Child Processes
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // run command in cmdl parameter
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(1);
                }
                else
                {
                        // wait for the child to exit
                        waitpid(childPid, &childStatus, 0);

                        // save exit status
                        if (WIFEXITED(childStatus))
                        {
                                exitStatus = WEXITSTATUS(childStatus);
                        }
                        else
                        {
                                exitStatus = WTERMSIG(childStatus);
                        }
                }
                return 0;
        }
        // if there is input and output redirection
        else if ((inputOutput[0] != NULL) && (inputOutput[1] != NULL))
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // open source file
                        int sourceFD = open(inputOutput[0], O_RDONLY);
                        if (sourceFD == -1)
                        {
                                perror("source open()");
                                exit(1);
                        }

                        // redirect stdin to source file
                        int result = dup2(sourceFD, 0);
                        if (result == -1)
                        {
                                perror("source dup2()");
                                exit(2);
                        }

                        // open target file
                        int targetFD = open(inputOutput[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (targetFD == -1)
                        {
                                perror("target open()");
                                exit(1);
                        }

                        // redirect stdout to target file
                        result = dup2(targetFD, 1);
                        if (result == -1)
                        {
                                perror("target dup2()");
                                exit(2);
                        }

                        // execute command
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(1);
                }
                else
                {
                        waitpid(childPid, &childStatus, 0);
                        // save the exit status
                        if (WIFEXITED(childStatus))
                        {
                                exitStatus = WEXITSTATUS(childStatus);
                        }
                        else
                        {
                                exitStatus = WTERMSIG(childStatus);
                        }

                }
                return 0;
        }
        // if there is input redirection but no output redirection
        else if (inputOutput[0] != NULL && inputOutput[1] == NULL)
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // open source file
                        int sourceFD = open(inputOutput[0], O_RDONLY);
                        if (sourceFD == -1)
                        {
                                perror("source open()");
                                exit(1);
                        }

                        // redirect stdin to source file
                        int result = dup2(sourceFD, 0);
                        if (result == -1)
                        {
                                perror("source dup2()");
                                exit(2);
                        }

                        // execute command
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(1);
                }
                else
                {
                        waitpid(childPid, &childStatus, 0);
                        // save the exit status
                        if (WIFEXITED(childStatus))
                        {
                                exitStatus = WEXITSTATUS(childStatus);
                        }
                        else
                        {
                                exitStatus = WTERMSIG(childStatus);
                        }

                }
                return 0;
        }
        // if there is output redirection but no input redirection
        else if (inputOutput[0] == NULL && inputOutput[1] != NULL)
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // open target file
                        int targetFD = open(inputOutput[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (targetFD == -1)
                        {
                                perror("target open()");
                                exit(1);
                        }

                        // redirect stdout to target file
                        int result = dup2(targetFD, 1);
                        if (result == -1)
                        {
                                perror("target open()");
                                exit(2);
                        }

                        // execute the command
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(1);
                }
                else
                {
                        waitpid(childPid, &childStatus, 0);
                        // save the exit status
                        if (WIFEXITED(childStatus))
                        {
                                exitStatus = WEXITSTATUS(childStatus);
                        }
                        else
                        {
                                exitStatus = WTERMSIG(childStatus);
                        }

                }
                return 0;
        }
}

int backg(char **cmdl, char **inputOutput)
{
        // set to ignore CTRL-C
        struct sigaction SIGINT_action = {0};
        SIGINT_action.sa_handler = SIG_IGN;
        sigfillset(&SIGINT_action.sa_mask);
        SIGINT_action.sa_flags = 0;

        // install signal handler
        sigaction(SIGINT, &SIGINT_action, NULL);

        if ((inputOutput[0] == NULL) && (inputOutput[1] == NULL))
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // run cmd in cmdl param
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(1);
                }
                else
                {
                        // if child has not returned, continue execution
                        waitpid(childPid, &childStatus, WNOHANG);
                        printf("background pid is %d\n", childPid);
                        childPids[k] = childPid;
                        k++;
                        newBackg = 0;
                }
                return 0;
        }
        // if there is both input redirection and output redirection
        else if ((inputOutput[0] != NULL) && (inputOutput[1] != NULL))
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // open source file
                        int sourceFD = open(inputOutput[0], O_RDONLY);
                        if (sourceFD == -1)
                        {
                                perror("source open()");
                                exit(1);
                        }

                        // redirect stdin to source file
                        int result = dup2(sourceFD, 0);
                        if (result == -1)
                        {
                                perror("source dup2()");
                                exit(2);
                        }

                        // open target file
                        int targetFD = open(inputOutput[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (targetFD == -1)
                        {
                                perror("target open()");
                                exit(1);
                        }

                        // redirect stdout to target file
                        result = dup2(targetFD, 1);
                        if (result == -1)
                        {
                                perror("target dup2()");
                                exit(2);
                        }

                        // execute the command
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(1);
                }
                else
                {
                        // if the child has not returned, resume execution
                        waitpid(childPid, &childStatus, WNOHANG);
                        printf("background pid is %d\n", childPid);
                        childPids[k] = childPid;
                        k++;
                        newBackg = 0;
                }
                return 0;
        }
        // if there is input redirection but no output redirection
        else if (inputOutput[0] != NULL && inputOutput[1] == NULL)
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // open source file
                        int sourceFD = open(inputOutput[0], O_RDONLY);
                        if (sourceFD == -1)
                        {
                                perror("source open()");
                                exit(1);
                        }

                        // redirect stdin to source file
                        int result = dup2(sourceFD, 0);
                        if (result == -1)
                        {
                                perror("target open()");
                                exit(1);
                        }

                        // execute the command
                        execvp(cmdl[0], cmdl);
                        perror("execvp has  failed");
                        exit(2);
                        }
                else
                {
                        // if child has not returned, resume execution
                        waitpid(childPid, &childStatus, WNOHANG);
                        printf("background pid is %d\n", childPid);
                        childPids[k] = childPid;
                        k++;
                        newBackg = 0;
                }
                return 0;
        }
        else if (inputOutput[0] == NULL && inputOutput[1] != NULL)
        {
                int childStatus;
                pid_t childPid = fork();

                if (childPid == -1)
                {
                        perror("fork() failed");
                        exit(1);
                }
                else if (childPid == 0)
                {
                        // open target file
                        int targetFD = open(inputOutput[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (targetFD == -1)
                        {
                                perror("target open()");
                                exit(1);
                        }

                        // redirect stdout to target file
                        int result = dup2(targetFD, 1);
                        if (result == -1)
                        {
                                perror("target open()");
                                exit(1);
                        }

                        // execute the command
                        execvp(cmdl[0], cmdl);
                        perror("execvp has failed");
                        exit(2);
                }
                else
                {
                        // if child has not returned, continue execution
                        waitpid(childPid, &childStatus, WNOHANG);
                        printf("background pid is %d\n", childPid);
                        childPids[k] = childPid;
                        k++;
                        newBackg = 0;
                }
                return 0;
        }
}

void parseCmd(char *aCmd)
{
        char *endLine;
        int bgprocess = 1;

        // ignore comment lines
        if (aCmd[0] == '#')
        {
                return;
        }

        // expand the shell variable if included
        // stringify the shell pid
        pid_t shellPid = getpid();
        char *shellPidString;
        sprintf(shellPidString, "%d", shellPid);

        // replace the $$ with shell pid
        int cmdlen = strlen(aCmd);
        int pidlen = strlen(shellPidString);
        char parsedVar[(cmdlen)+(pidlen-2)];

        int z = 0;
        int x = 0;
        int varExpansion = 1;
        while (z<(cmdlen)+(pidlen-2)-1)
        {
                if (aCmd[z] == '$' && aCmd[z+1] == '$')
                {
                        // if you actually end up replacing anything, indices will shift, so break after
                        varExpansion = 0;
                        while (x<pidlen)
                        {
                                parsedVar[z] = shellPidString[x];
                                x++;
                                z++;
                        }
                        while (z<(cmdlen)+(pidlen-2)-1)
                        {
                                parsedVar[z] = aCmd[z-(pidlen-2)];
                                z++;
                        }

                        parsedVar[(cmdlen)+(pidlen-2)-1] = aCmd[z-(pidlen-2)];
                        break;
                }
                parsedVar[z] = aCmd[z];
                z++;
        }
        parsedVar[(cmdlen)+(pidlen-2)-1] = aCmd[z-(pidlen-2)];

        // if there is a variable expansion, change the cmd to reflect that
        if (varExpansion == 0)
        {
                int parsedlen = strlen(parsedVar);
                aCmd = calloc(parsedlen+1, sizeof(char));
                strcpy(aCmd, parsedVar);
        }

        // remove newline from fgets
        if ((endLine = strchr(aCmd, '\n')) != NULL)
        {
                *endLine = '\0';
        }


        // check if the last char is ampersand
        if (aCmd[strlen(aCmd)-1] == '&')
        {
                // remove ampersand
                aCmd[strlen(aCmd)-1] = '\0';
                // set background process checker to true
                bgprocess = 0;

        }
        // prepare to look for i/o redirection symbols
        char *inputSymb;
        char *outputSymb;
        char *inputFile;
        char *outputFile;
        char *savePtr2;
        // create array for potential io redirection
        char *inputOutput[2];
        bzero(inputOutput, sizeof(inputOutput));
        // if only input symbol is entered
        if ((inputSymb = strchr(aCmd, '<')) != NULL)
        {
                // if both input and output symbols are entered
                if ((outputSymb = strchr(aCmd, '>')) != NULL)
                {

                        // retrieve and store input file
                        char *iotoken = strtok_r(aCmd, "<", &savePtr2);
                        iotoken = strtok_r(NULL, ">", &savePtr2);
                        inputFile = calloc(strlen(iotoken)+1, sizeof(char));
                        strcpy(inputFile, iotoken);

                        // remove leading whitespace from inputFile
                        int i = 0;
                        while (inputFile[i] != '\0')
                        {
                                inputFile[i] = inputFile[i+1];
                                i++;
                        }
                        inputFile[i-2] = '\0';

                        // retrieve and store output file
                        iotoken = strtok_r(NULL, "\0", &savePtr2);
                        outputFile = calloc(strlen(iotoken)+1, sizeof(char));
                        strcpy(outputFile, iotoken);

                        // remove leading whitespace from outputFile
                        int j = 0;
                        while (outputFile[j] != '\0')
                        {
                                outputFile[j] = outputFile[j+1];
                                j++;
                        }
                        outputFile[j-1] = '\0';

                        // set command to not have redirect parameters
                        iotoken = strtok_r(aCmd, "<", &savePtr2);
                        aCmd = calloc(strlen(iotoken)+1, sizeof(char));
                        strcpy(aCmd, iotoken);

                        // fill array with redirect information
                        inputOutput[0] = inputFile;
                        inputOutput[1] = outputFile;
                }
                else
                {
                        // retrieve and store input file
                        char *iotoken = strtok_r(aCmd, "<", &savePtr2);
                        iotoken = strtok_r(NULL, ">", &savePtr2);
                        inputFile = calloc(strlen(iotoken)+1, sizeof(char));
                        strcpy(inputFile, iotoken);

                        // remove leading whitespace from inputFile
                        int i = 0;
                        while (inputFile[i] != '\0')
                        {
                                inputFile[i] = inputFile[i+1];
                                i++;
                        }
                        inputFile[i-1] = '\0';

                        // set command to not have redirect parameters
                        iotoken = strtok_r(aCmd, "<", &savePtr2);
                        aCmd = calloc(strlen(iotoken)+1, sizeof(char));
                        strcpy(aCmd, iotoken);

                        // fill array with input info
                        inputOutput[0] = inputFile;
                }
        }

        // if only output symbol is entered
        else if ((outputSymb= strchr(aCmd, '>')) != NULL)
        {
                // retrieve and extract output file
                char *iotoken = strtok_r(aCmd, ">", &savePtr2);
                iotoken = strtok_r(NULL, "\0", &savePtr2);
                outputFile = calloc(strlen(iotoken)+1, sizeof(char));
                strcpy(outputFile, iotoken);

                // remove leading whitespace from outputFile
                int i = 0;
                while (outputFile[i] != '\0')
                {
                        outputFile[i] = outputFile[i+1];
                        i++;
                }
                outputFile[i-1] = '\0';

                // set command to not have redirect parameters
                iotoken = strtok_r(aCmd, ">", &savePtr2);
                aCmd = calloc(strlen(iotoken)+1, sizeof(char));
                strcpy(aCmd, iotoken);
                aCmd[strlen(aCmd)-1] = '\0';

                // fill array with output info
                inputOutput[1] = outputFile;
        }
        char *savePtr;
        char *token = strtok_r(aCmd, " ", &savePtr);
        char *args[MAXARGS];
        int pos = 0; // args array index
        bzero(args, sizeof(args));

        // while there are still tokens, fill an array args with those tokens
        while (token != NULL)
        {
                args[pos] = token;
                pos++;
                token = strtok_r(NULL, " ", &savePtr);
        }

        // if blank line, return to main
        if (args[0] == NULL)
        {
                return;
        }
        // if there is no args[0], it becomes pid, so ignore that
        int argsZero = atoi(args[0]);
        if (argsZero == getpid())
        {
                return;
        }

        // if cmd is exit, exit shell
        if (strcmp("exit", args[0]) == 0)
        {
                exit(0);
        }
        // if cmd is cd, change directory to args[1]
        else if (strcmp("cd", args[0]) == 0)
        {
                // ignore ampersand if there is one
                bgprocess = 1;

                // if no destination is specified, cd to home
                if (args[1] == NULL)
                {
                        int homeDir = chdir(getenv("HOME"));
                        if (homeDir<0)
                        {
                                printf("directory change unsuccessful\n");
                        }
                }
                // if the given path is absolute, follow it with no changes
                else if (strchr(args[1], '/') != NULL)
                {
                        int target = chdir(args[1]);
                        if (target<0)
                        {
                                printf("directory change unsuccessful\n");
                        }
                }
                // otherwise create a relative path from current directory
                else
                {
                        char *currDir;
                        currDir = calloc(3, sizeof(char));
                        currDir = "./";

                        char *dirName;
                        dirName = calloc(strlen(args[1]), sizeof(char));
                        dirName = args[1];

                        char *path;
                        path = calloc((strlen(args[1]) + 3), sizeof(char));
                        strcat(path, currDir);
                        strcat(path, dirName);

                        int target = chdir(path);
                        if (target<0)
                        {
                                printf("directory change unsuccessful\n");
                        }
                }

        }
        // if cmd is status, print out exit status or term signal of last process
        else if (strcmp("status", args[0]) == 0)
        {
                // ignore ampersand if there is one
                bgprocess = 1;

                printf("%d\n", exitStatus);
        }

        // if cmd is unrecognized, run it in bg or fg depending on bgprocess status
        else
        {
                // if foreground only mode has been enabled
                if (fgOnly == 0)
                {
                        eval(args, inputOutput);
                }
                // fg cmds go to eval
                else if (bgprocess == 1)
                {
                        eval(args, inputOutput);
                }
                // bg cmds go to backg
                else if (bgprocess == 0)
                {
                        backg(args, inputOutput);
                }
        }
}

int main()
{
        // set to ignore CTRL-C
        struct sigaction SIGINT_action = {0};
        SIGINT_action.sa_handler = SIG_IGN;
        sigfillset(&SIGINT_action.sa_mask);
        SIGINT_action.sa_flags = 0;

        // install signal handler
        sigaction(SIGINT, &SIGINT_action, NULL);

        void handle_SIGTSTP(int signo)
        {
                if (fgOnly == 1)
                {
                        char* fgOn = "Entering foreground-only mode now (& will be ignored)\n";
                        write(STDOUT_FILENO, fgOn, 55);
                        fflush(stdout);
                        fgOnly = 0;
                }
                else if (fgOnly == 0)
                {
                        char* fgOff = "Exiting foreground-only mode\n";
                        write(STDOUT_FILENO, fgOff, 30);
                        fflush(stdout);
                        fgOnly = 1;
                }
        }

        struct sigaction SIGTSTP_custom = {0};
        SIGTSTP_custom.sa_flags = SA_RESTART;
        SIGTSTP_custom.sa_handler = handle_SIGTSTP;
        sigaction(SIGTSTP, &SIGTSTP_custom, NULL);

        int childStatus;

        while(1) {
                char cmdl[MAXLENGTH];

                printf(":");
                fflush(stdout);

                fgets(cmdl, MAXLENGTH, stdin);

                parseCmd(cmdl);

                // check for bg processes
                int m;
                for (m=0;m<50;m++)
                {
                        if (childPids[m] != 0)
                        {
                                waitpid(childPids[m], &childStatus, WNOHANG);
                                if (newBackg == 1)
                                {
                                        if (WIFEXITED(childStatus))
                                        {
                                                printf("background pid %d is done: exit value %d\n", childPids[m], childStatus);
                                                childPids[m] = 0;
                                        }

                                        if (WIFSIGNALED(childStatus))
                                        {
                                                if (WTERMSIG(childStatus) == 15)
                                                {
                                                        printf("background pid %d is done: killed by signal %d\n", childPids[m], WTERMSIG(childStatus));
                                                        childPids[m] = 0;
                                                }
                                        }
                                }
                                newBackg = 1;
                        }
                }
        }

}


















