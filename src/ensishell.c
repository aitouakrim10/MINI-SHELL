/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "variante.h"
#include "readcmd.h"
#include <unistd.h>
#include <signal.h>

/*----------------------------    JOBS   ---------------------------------*/
// structure descriptive de process s execute en background
struct Process_Bg {
     char* cmd;
     pid_t pid;
     int status; //verifier si le processus et terminer ou non
     struct Process_Bg * next;
    };

// initialisation de liste chainee : memorisation des bg process
struct Process_Bg * list = NULL;

// add bg_process to liste
struct Process_Bg* add_process(char* command,pid_t pid, struct Process_Bg* list){
    struct Process_Bg* bg = malloc( sizeof(struct Process_Bg));
    if(bg == NULL){
        free(bg);
        return list;// Si l'allocation a échoué, retourner la liste inchangée
    }
    bg->pid = pid;
    bg->cmd = malloc((strlen(command)+1)*sizeof(char));
    if(bg->cmd == NULL) {
        free(bg);
        return list;// Si l'allocation a échoué, retourner la liste inchangée
    }
    strcpy(bg->cmd, command);
    bg->status = 0;
    bg->next = list;
    //list = bg; 
    return bg;
    }

// check si le processus et terminer ou non
void check_pr_bg(int signum) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        struct Process_Bg *cur = list;
        while (cur != NULL) {
            if (cur->pid == pid) {
                cur->status = 1;
                break;
            }
            cur = cur->next;
        }
    }
}

// display background process
void display_jobs(struct Process_Bg* list) {
      // Affiche la liste des processus en arrière-plan
      struct Process_Bg *process = list;
      while (process != NULL) {
          printf("PID: %d     Command: %s    terminated: %s\n", process->pid, process->cmd, (process->status == 1 )? "Yes" : "NO");
          process = process->next;
      }
  }

// free background list
void free_list_bg(struct Process_Bg* list){
      while(list != NULL){
      struct Process_Bg *process = list;
      list = list->next;
      free(process->cmd);
      free(process);
      }
}

/*---------------------------------------------------------------------------------*/

#ifndef VARIANTE
#error "Variante non défini !!"
#endif
#include <fcntl.h>
/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */


#if USE_GUILE == 1
#include <libguile.h>
/*-------------------------------- Partie 5 -------------------------------------------------*/
int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
    struct cmdline *cmd = parsecmd(&line);

    pid_t pid;

    if (cmd->err) {
        fprintf(stderr, "%s\n", cmd->err);
        exit(EXIT_FAILURE);
    }
    if(strcmp(cmd->seq[0][0], "exit()") == 0){
        printf("exit\n");
        exit(EXIT_SUCCESS);
    }
    pid = fork(); //Crée un processus fils

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }else if (pid == 0) {
         if (cmd->err) {
                fprintf(stderr, "%s\n", cmd->err);
                exit(EXIT_FAILURE);
            }
         execvp(cmd->seq[0][0], cmd->seq[0]);
            perror("execvp");
            exit(EXIT_FAILURE);
    }else{
        if(!cmd->bg){
            // pid0 process wait the end of child process pid
            int status;
             waitpid(pid, &status, 0);
         }else{
            list = add_process(cmd->seq[0][0], pid, list);
            signal(SIGCHLD, check_pr_bg);
        }

    }
	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif
/*------------------------------------------------------------------------------------------*/

void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}

// cleanup 
void cleanup(char * line, struct Process_Bg * list){
     terminate(line);
     free_list_bg(list);
     exit(0);
 }


/*------------------------- execution commandline ---------------------------------*/
// fonction qui prend en parametre cmd string et elle l'execute
void run_cmd(struct cmdline *cmd){
    // new process
    pid_t pid = fork();
    if(pid < 0){
        // cas de fork echec
        fprintf(stderr,"error forking");
        exit(1);
    }else if(pid == 0){// pid fils
        /* execution de child process */
        if(cmd->out){
            /* redirection sortie */
            int opout = open(cmd->out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // open file
            if(opout < 0){// verification
                perror("Opening Error ");
                exit(1); // verification
            }
            if (ftruncate(opout, 0) < 0) { // reduction to 0
            perror("Reduction error");
            exit(1);
             }
            dup2(opout,STDOUT_FILENO); // redirection
            close(opout); // close file
        }
         if(cmd->in){
             /* redirection entrie */
             int opin = open(cmd->in, O_RDONLY); // open file
             if(opin < 0) exit(1); // verification
             dup2(opin,STDIN_FILENO); // redirection
             close(opin); // close file
         }

        if(cmd -> seq[1] != NULL){
            /* execution de child process */
            // cas de pipe
            /* execution de pipe commande */
            int pipefd[2]; // read end & write end

            pipe(pipefd);// opening the pipe

            pid_t pid2 = fork();  // child process 2
                                  //
            if(pid2 == 0){
                /* pid2  is process child of pid*/
                dup2(pipefd[0],0);
                close(pipefd[1]); //close write end
                close(pipefd[0]); // close read end
                execvp(cmd->seq[1][0], cmd->seq[1]);
                }
            dup2(pipefd[1], 1); // redirection
            close(pipefd[0]); // close read end
            close(pipefd[1]); // close write end
            }
            execvp(cmd->seq[0][0], cmd->seq[0]);
    }else{
        /* execution en arriere plan ou Non*/
        if(!cmd->bg){
                // NON -> parant process wait the end of child process
                int status;
                waitpid(pid, &status, 0);
        }else{
                list = add_process(cmd->seq[0][0], pid, list);
                signal(SIGCHLD, check_pr_bg);
            }
       }
}
/*----------------------------------------------------------------------------------*/

int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		struct cmdline *l;
		char *line=0;
		//int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (strncmp(line,"exit", 4) == 0){
            cleanup(line, list);
		}
#if USE_GNU_READLINE == 1
		add_history(line);
#endif

#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd( & line);

		/* If input stream closed, normal termination */
		if (!l) {

			terminate(0);
		}
        // absence de commande
        if (l->seq[0] == NULL) {
			continue;
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
        // jobs
         if(strcmp(l->seq[0][0], "jobs") == 0){
              //check_process_bg_status(list);
              display_jobs(list);
              continue;
         }

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

		/* Display each command of the pipe */
		/*
        for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
                        for (j=0; cmd[j]!=0; j++) {
                                printf("'%s' ", cmd[j]);
                        }
			printf("\n");
		}*/

    // execution de cmd 
    run_cmd(l);
    }
}



