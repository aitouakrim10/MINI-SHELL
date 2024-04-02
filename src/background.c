#include "background.h"


// add bg_process to liste
void add_process(char* command,pid_t pid, struct Process_Bg* list){
    struct Process_Bg* bg = malloc( sizeof(struct Process_Bg));
    if(!bg) return ;
    bg->pid = pid;
    bg->cmd = malloc((strlen(command)+1)*sizeof(char));
    if(bg->cmd == NULL) exit(1);
    strcpy(bg->cmd, command);
    bg->status = 0;
    bg->next = list;
    list = bg;
    }

// check si le processus et terminer ou non
void check_process_bg_status(struct Process_Bg * list) {
      struct  Process_Bg*  process = list;
      while (process != NULL) {
          int status;
          if (waitpid(process->pid, &status, WNOHANG) > 0) {
              // Le processus s'est terminé
              process->status = 1;
          }
          process = process->next;
      }
  }

// display background process
void display_jobs(struct Process_Bg* list) {
      // Affiche la liste des processus en arrière-plan
      struct Process_Bg *process = list;
      while (process != NULL) {
          printf("PID: %d     Command: %s    terminate; %s\n", process->pid, process->cmd, process->status ? "Yes" : "No");
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

