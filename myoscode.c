#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <errno.h>
extern int errno;

#ifndef MAX_PATH
#define MAX_PATH 100
#endif

#ifndef HISTORY_SIZE
#define HISTORY_SIZE 10
#endif

#ifndef ARGS_SIZE
#define ARGS_SIZE 20
#endif


struct job {
  int pid;
  int pos;
  char** args;
  int status;
  struct job* next;
};

struct history_item {
  int bg;
  char** args;
};

struct job* jobs_head = NULL;

struct job* get_job_at_pos(int pos) {
  struct job * point = jobs_head;

  while(point != NULL) {
    if (point->pos == pos) {
      return point;
    }

    point = point->next;
  }

  return NULL;
}

void print_jobs() {
  struct job* point = jobs_head;

  if (jobs_head == NULL) {
    printf("in it!\n");
    return;
  }

  int count = 0;
  do {
    // int the_pid = point->pid;
    if (point == NULL) { printf("something is weird"); }

    printf("id: %d, PID %d, command: ", point->pos, point->pid);

    count++;

    for (int i = 0; i < 2; i++) {
      char* arg = point->args[i];
      if (arg == NULL) { break; }

      printf("%s ", arg);
    }

    printf("\n");
    point = point->next;
  } while(point != NULL && count < 2);
}

int insert_job(int pid, char* args[20]) {
  struct job* point = jobs_head;
  int which_index = 0;
  
  /*
  struct job new_job = {
    .pid = pid,
    .args = args,
    .status = 0,
    .pos = 0,
    .next = NULL,
  };
  */
  struct job * new_job = (struct job *)malloc(sizeof(struct job));

  new_job->pid = pid;
  new_job->args = (char **) malloc(ARGS_SIZE*sizeof(char*));
  memcpy(new_job->args, args, ARGS_SIZE*sizeof(char*));
  new_job->status = 0;
  new_job->pos = 0;
  new_job->next = NULL;
  
  do {
    if (jobs_head == NULL) {
      jobs_head = new_job;

      return 0;
    }

    if (point->next == NULL || point->next->pos != which_index+1) {
      new_job->next = point->next;
      point->next = new_job;
      new_job->pos = which_index+1;

      return new_job->pos;
    }
    
    point = point->next;
    which_index++;
  } while (point != NULL);
  
  return -1;
}

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);
    // printf("%d\n", length);
    // printf("errno: %d\n", errno);

    if (length <= 0) {
        exit(-1);
    }

    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (unsigned int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    
    return i;
}


int history_count = 0;
struct history_item ** shell_command_history;

void save_to_history(char** args) {
  struct history_item * his_item = (struct history_item *)
    malloc(sizeof(struct history_item));

  his_item->args = args;

  if (history_count > 9) {
    free(shell_command_history[history_count%HISTORY_SIZE]);
  }
  
  shell_command_history[history_count%HISTORY_SIZE] = his_item;

  history_count++;
}

char curdir[MAX_PATH+1];

void show_history() {
  int first_history_index = history_count-HISTORY_SIZE;
  if (first_history_index < 0) {
    first_history_index = 0;
  }

  for (int i = first_history_index; i < history_count; i++) {
    printf("%d ", i+1);

    for (int j = 0; j < ARGS_SIZE; j++) {
      char * arg;
      if ( (arg = shell_command_history[i%HISTORY_SIZE]->args[j]) == NULL) {
        break;
      } else {
        printf("%s ", arg);
      }
    }

    printf("\n");
  }
}

int remove_job(int pid) {
  if (jobs_head == NULL) {
    return -1;
  }
  
  if (jobs_head->pid == pid) {
    jobs_head = jobs_head->next;
    return 0;
  }
  
  struct job * point = jobs_head;
  
  while(point != NULL && point->next != NULL) {
    if (point->next->pid == pid) {
      point->next = point->next->next;
      return point->pos + 1;
    }

    point = point->next;
  }
  
  return -1;
}

void handle_child_death(int sig) {
  signal(SIGCHLD, handle_child_death);

  int status;
  int p;
  sig = sig+1;

  while((p=waitpid(-1, &status, WNOHANG)) != -1) {
    remove_job(p);
  }
}

int main()
{
    int bg;
    char *args[ARGS_SIZE];
    int cnt;
    char new_dir[MAX_PATH + 1];
    // new_dir[0] = '\0';

    char buf[MAX_PATH + 1];
    
    memcpy(curdir, getcwd(buf, MAX_PATH + 1), MAX_PATH + 1);
    
    signal(SIGCHLD, handle_child_death);

    // free(shell_command_history);
    shell_command_history = (struct history_item **) malloc(HISTORY_SIZE*sizeof(struct history_item *));
    for(int i = 0; i < HISTORY_SIZE; i++) {
      shell_command_history[i] = (struct history_item *) malloc(sizeof(struct history_item));
    }

    while(1) {
      bg = 0;
      cnt = getcmd("\n>>  ", args, &bg);
      args[cnt] = NULL;

      /*
      for (int i = 0; i < cnt; i++)
        printf("\nArg[%d] = %s", i, args[i]);
        */

      printf("\n");
    
      int history_element_to_execute;
      if ( (history_element_to_execute = atoi(args[0])) > 0) {
        printf("history_element_to_execute: %d\n", history_element_to_execute);
        memcpy(args, shell_command_history[history_element_to_execute-1], ARGS_SIZE);
      }
      
      save_to_history(args);

      if (strcmp(args[0],"history") == 0) {
        if (args[1] != NULL) {
          printf("history does not require any arguments.\n");
          exit(-1);
        }
        
        show_history();
        
        continue;
      }

      if (strcmp(args[0], "cd") == 0) {
        new_dir[0] = '\0';
        strcat(new_dir, curdir);
        printf("%s\n", new_dir);

        if (args[1][0] == '/') { 
          new_dir[0] = '\0';
        }
        char slash[] = {'/', '\0'};
        strcat(new_dir, slash);
        strcat(new_dir, args[1]);
        strcat(new_dir, slash);

        curdir[0] = '\0';
        strcat(curdir, new_dir);
  
        chdir(new_dir);
        
        continue;
      }
      

      if (strcmp(args[0], "pwd") == 0) {
        char* cwd;
        cwd = getcwd(buf, MAX_PATH + 1);
        printf("%s", cwd);

        continue;
      }
  
      if (strcmp(args[0], "exit") == 0) {
        exit(0);
      }

      if (strcmp(args[0], "jobs") == 0) {
        print_jobs();

        continue;
      }
      
      if (strcmp(args[0], "fg") == 0) {
        // waitpid();
        int pos = strtol(args[1], NULL, 10);

        struct job * selected_job = get_job_at_pos(pos);
        if (selected_job == NULL) {
          printf("There was an issue. The selected job, %d, has not been found.\n", pos);
          continue;
        } 

        int status = 0;

        waitpid(selected_job->pid, &status, 0);

        continue;
      } 
  
      // char * file = malloc(sizeof);
      int child_pid = 0;
      if((child_pid = fork()) == 0) {
        int output_filename_index = cnt - 1;
        if (bg) {
          output_filename_index--;
        }

        if (output_filename_index < 1) {
          output_filename_index = 1;
        }
        printf("cnt: %d.\n", cnt);

        char* output_filename = malloc((MAX_PATH+1)*sizeof(char));
        if (strcmp(args[output_filename_index - 1], ">") == 0) {
          char* arg = args[output_filename_index];
          memcpy(output_filename, arg, strlen(arg)+1);
          printf("%s\n", output_filename);
          close(1);
          open(output_filename, O_WRONLY | O_CREAT, 0666);
          // int fd[2] = {0,1};
          // pipe(fd);
          args[output_filename_index - 1] = NULL;
        }
      
        int result = execvp(args[0], args);
        if (result < 0) {
          printf("We got the errno %d", errno);
        } 
      } else {
        // continue execution
      }

      if (bg) {
          // printf("\nBackground enabled..\n");
          int inserted_job_pos = insert_job(child_pid, args);
          printf("Created job with pid: %d, at pos: %d", child_pid, inserted_job_pos);
      } else {
          // printf("\nBackground not enabled \n");
          int status = 0;
          waitpid(child_pid, &status, 0);
      }
      
      printf("\n\n");
    }
}
