#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <zconf.h>
#include <sys/wait.h>
#define printf myPrintf

/**
 CUSTOM SHELL, VERY BASIC. WISH Shell is a shell that is an experiment in shell creation, while not fully featured it deomonstrates basic concepts such as output redirection built in commands and path variables.
 While not recommended for any practical use, this shell can be a great starting point to jump in and learn about shell coding with a basic example. This shell was written with minimization of lines of code in mind and as such also
 demonstrates some (some would call ugly) unorthodox features of the C language. If you are looking for a quick dirty shell just to see whats its like, you have found one that should get you on the right path. Enjoy... W I  S  H 
 **/


// Variables to store file numbers for standard error and standard output
int myOut;
int myO;
// Linked List to store Paths
 typedef struct {
    char * p;
    struct Path * next;
} Path;
Path * pathList;


// This is a custom print function that will be used in place of printf, it is like printf but flushes stdout at the end
int myPrintf(char * format,...) {
    va_list args; va_start(args,format);

    vprintf(format, args);
    va_end(args);
    fflush(stdout);

    return 0;



}


// This function restores standard output and error to their initial values
void restore() {
    dup2(myOut,STDOUT_FILENO);
    dup2(myO,STDERR_FILENO);
}


// Creates a path list from given string array and its size. Paths are not stored in environment variables in this implementation
void createPath(int margc, char ** margv) {
    pathList = malloc(sizeof(Path));
    Path * mp = pathList;
    for(int i = 1; i < margc; i++) {
        mp->p = malloc(sizeof(char)*(strlen(margv[i]) + 2));
        strcpy(mp->p,margv[i]);
        char slash [2] = "/";
        if (mp->p[strlen(mp->p)] != '/') {
            strcat(mp->p,slash);
        }
        //printf("%s ",mp->p);
        if(i == margc -1) {
            mp->next= NULL;
            break;
        }
        mp->next = malloc(sizeof(Path));

        mp = (Path * ) mp->next;
    }
}

// Error utility function. writes that an error has occured to STDERR
void error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    fflush(stderr);
    fflush(stdout);
    restore();
}

// Trim utility function which is equvilient to trim in other languages, remove trailing and leading spaces
char * trim(char *st) {

    char * str = malloc(sizeof(char)*strlen(st));
    char * s  = str;

    while (*st ==' ') {
        st++;
    }
    for(int i = 0; i <strlen(st); i++) {
        if (st[i] != ' ' || (i > 0 && st[i] == ' ' && st[i-1]!=' ')) {
            *s = st[i];
            s++;
        }
    }
    size_t j = strlen(str);
    while(str[--j] ==' ' );
    str[j+1]='\0';
    return str;


}


 // Command Handler
int cmdH(char * myCmd) {
    //printf("Command: %s ",myCmd);
    //char pa [3] = "&\n";
    
    char * q = myCmd; q--;
    // Save STDOUT, STDERR redundant, not needed
    myOut = STDOUT_FILENO;
     myO = STDERR_FILENO;
    // Search command for output redirection symbol using q
    while(*(++q) != '>' && *q != '\0');
    
    // in case that redirection was found
    if (*q != '\0') {
        // Invalid syntax
        if (*myCmd == '\0' || *myCmd == '>') {error(); return -1;}
        
        char * myTemp = myCmd; myTemp --;
        //traling spaces check
        while(*(++myTemp) != '\0') { if (*myTemp != ' ') {break;} }
        //Invalid Syntax
        if(*myTemp == '\0' || *myTemp == ' ') {error(); return -1;}
        //Set q to end string
        *q ='\0';
        q++;
        // Trim sanity check
        char * p = trim(q);
        
        // Another syntax check
        while(*(++p) != '>' && *p != '\0' && *p != ' ' );

        if (*p != '\0') {error(); return -1;}
        //printf("redirecting, %s",q);
        // q is now at the output file name and all errors accounted for so open the new output file
        int newOut = open(trim(q),O_WRONLY| O_CREAT | O_TRUNC,0664);
        
        if (newOut < 0) {
            error();
            return -1;
        
        }
        //Save STDOUT, STDERR
         myOut = dup(STDOUT_FILENO);
         myO = dup(STDERR_FILENO);
        // Set STDOUT, STDERR to new file as defined by the redirect
        dup2(newOut, 1);
        dup2(newOut,2);

    }

    
    // following code essentaially parses command arguments and stores them in myArgv
    char ** myArgv;
    int n = -1;
    int onLet = 1;
    int count = 0;
    while(myCmd[++n]!='\0') {
       if(myCmd[n] == ' ' && onLet) {
           count++;
           onLet =0;
       }

       else if(myCmd[n] != ' ' && !onLet) {
            onLet =1;
        }
    }
    if (onLet) {
        count++;
        onLet = 0;
    }

    myArgv = malloc(sizeof(char *)*(count+1));
    myArgv[count] = NULL;
    int myArgc = count;
    count = 0;
    char * mptr = myCmd;
    char * saveptr2;
    char * sret = strtok_r(mptr," ",&saveptr2);
    myArgv[count] = sret;
    while((sret = strtok_r(NULL," ",&saveptr2)) != NULL  ) {
        myArgv[++count] = sret;
    }
    //int soopt = strcmp(myArgv[0],"cd");
    //char * moretemp = myArgv[0];
    //Error Check
    if (myArgv[0] == NULL) {return -1;}
    
    // Command CD changes directory, check for correct arguments and use chdir system call
    if (strcmp(myArgv[0],"cd")==0) {

        if (myArgc == 2 ) {
            //char * tempcharstar = myArgv[1];
            int tempThing = chdir(myArgv[1]);
            if(tempThing == -1) {
                error();
                return -1;
            }
        } else {
            error();
            return -1;
        }
    }

    // Command PATH modifies path
    if (strcmp(myArgv[0],"path")==0) {
        // if just path is typed path is cleared
        if (myArgc == 1) {
            pathList = NULL;
            return 0;
        }
        // if arguments are supplied a new path is created and set replacing the old one
        createPath(myArgc,myArgv);
        //Path * temppp = pathList;
//        while (temppp != NULL) {
//            printf("%s ",temppp->p);
//            temppp = (Path *)temppp->next;
//        }
    }
    // Command EXIT simply exits the shell
    if (strcmp(myArgv[0],"exit")==0) {
        if (myArgc != 1 ) {
            error();
            return -1;
        }
        exit(0);
    }
    if (strcmp(myArgv[0],"path") == 0 || strcmp(myArgv[0],"cd") == 0) {return 0;}
    
    // Now time to search the Path to see if the command can be found somwwhere else
    Path * itt = pathList;
    int found = 0;
    while (itt != NULL) {
        char * temp = malloc(strlen(itt->p));
        char * useMe = "";
        strcpy(temp,itt->p);
        // Check access and file found
        if (access((useMe = strcat(temp,myArgv[0])), X_OK) != 0) {
            itt = (Path * )itt->next;
            continue;

        }
        found = 1;
        //printf("access success");
        
        // Fork a new process and run command
        int ac = fork();
        
        if(ac == 0) {
            //printf("child started");
            if ((execv(useMe,myArgv)) == -1) {
                    error();
                    return -1;

            }

        } else {
            ac = (int) wait(NULL);
            break;
        }
        //printf("trying to run ");
    }

    // if no command is found error
    if (!found) {
        error();
        //printf("\n");
        return -1;
   }
    //else {
//        printf("not found");
//    }

    restore();
    return 0;

}



int main(int argc, char *argv[]) {
    FILE * fp;
    if (argc > 2) {error();exit(1);}
    // This is support for input from a file for testing purposes, if no file, then STDIN will be used
    if (argc  == 2) {
        fp = fopen(argv[1],"r");
        if (fp == NULL) {
            error();
            exit(1);
        }

    } else {
        fp = stdin;
    }
    // adds /bin to path
    pathList = malloc(sizeof(Path));
    char * bins = "/bin/";
    pathList->p =bins;
    pathList->next = NULL;
    // main loop
    while(1) {
        // Print shell name and get input
        if(fp==stdin) printf("wish> ");
        char * buf = NULL; size_t tmp = 0; char * saveptr; char par [3] = "&\n"; char * cmds;
        ssize_t tempVar = getline(&buf,&tmp,fp);
        if (tempVar == -1) {exit(0);}

        //int yc;
        // if no command then continue else handle the command
        cmds = strtok_r(buf,par,&saveptr);
        if (cmds == NULL) { continue;}
         cmdH(trim(cmds));
        //yc = fork();

        //if (yc == 0) {cmdH(trim(cmds));}


        while ( (cmds = strtok_r(NULL,par,&saveptr) )!= NULL) {

             //yc = fork();

             cmdH(trim(cmds));


        }
        //yc = (int) wait(NULL);
       // if(fp==stdin) printf("\n");
    }

}



