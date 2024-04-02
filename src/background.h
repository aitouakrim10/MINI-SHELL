#ifndef BACkGROUND_H
#define BACkGROUND_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// list chainee des bg_process

// description de bg
struct Process_Bg {
     char* cmd;
     pid_t pid;
     int status; //verifier si le processus et terminer ou non
     struct Process_Bg * next;
    };


extern void add_process(char* command,pid_t pid, struct Process_Bg* list);
extern void check_process_bg_status(struct Process_Bg* list);
extern void display_jobs(struct Process_Bg* list);
extern void free_list_bg(struct Process_Bg* list);

#endif

