#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct command {
    char *cmd;              // string apenas com o comando
    int argc;               // número de argumentos
    char *argv[MAXARGS+1];  // vector de argumentos do comando
    struct command *next;   // apontador para o próximo comando
} COMMAND;

// variáveis globais
char* inputfile = NULL;	    // nome de ficheiro (em caso de redireccionamento da entrada padrão)
char* outputfile = NULL;    // nome de ficheiro (em caso de redireccionamento da saída padrão)
int background_exec = 0;    // indicação de execução concorrente com a mini-shell (0/1)

// declaração de funções
COMMAND* parse(char* linha);
void print_parse(COMMAND* commlist);
void execute_commands(COMMAND* commlist);
void free_commlist(COMMAND* commlist);

// include do código do parser da linha de comandos
#include "parser.c"

int main(int argc, char* argv[]) {
  char *linha;
  COMMAND *com;

  while (1) {
    if ((linha = readline("shell_sensual$ ")) == NULL)
      exit(0);    
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha);
      if (com) {
	//print_parse(com);
	execute_commands(com);
	free_commlist(com);
      }
    }
    free(linha);
  }
}


void print_parse(COMMAND* commlist) {
  int n, i;

  printf("---------------------------------------------------------\n");
  printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
  n = 1;
  while (commlist != NULL) {
    printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
    i = 0;
    while (commlist->argv[i] != NULL) {
      printf("%s,", commlist->argv[i]);
      i++;
    }
    printf("%s'\n", commlist->argv[i]);
    commlist = commlist->next;
    n++;
  }
  printf("---------------------------------------------------------\n");
}

void free_commlist(COMMAND *commlist){
  // ...
  // Esta função deverá libertar a memória alocada para a lista ligada.
  // ...
  if(commlist == NULL) return;
  free_commlist(commlist->next);
  free(commlist);
}

void execute_commands(COMMAND *commlist) {
  // ...
  // Esta função deverá "executar" a "pipeline" de comandos da lista commlist.
  // ...
  if(strcmp("exit", commlist->cmd) == 0)
    exit(0);
  pid_t pid;
  
  int npipes = 0;
  COMMAND* ptr = commlist;
  
  while(waitpid(NULL, NULL, WNOHANG) == 1);
  
  while(ptr != NULL) {
    ptr = ptr->next;
    npipes++;
  }
  npipes--;
  
  int fd[npipes][2];
  int i;
  
  i = 0;
  while(commlist != NULL) {
    //before fork  
    if(i < npipes)
      if(pipe(fd[i]) == -1) {
	perror("Error: pipe failed.\n");
	exit(1);
      }
    
    if((pid = fork()) < 0) {
      //fork failed
      perror("Error: fork failed.\n");
      exit(1);
    }
  
    else if(pid == 0) {
      //child code after fork
      if(inputfile != NULL && i == 0) {
	int fdin = open(inputfile, O_RDONLY);       
	if(fdin == -1) {
	  perror("Error: file could not be created.\n");  
	  exit(1);
	}
	dup2(fdin, STDIN_FILENO);
	close(fdin);
      }
    
      if(outputfile != NULL && i == npipes) {
	int fdout = open(outputfile, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
	if(fdout == -1) {      
	  perror("Error: file could not be opened or does not exist.\n");
	  exit(1);
	}
	dup2(fdout, STDOUT_FILENO);
	close(fdout);
      }
            
      if (i > 0) {
	dup2(fd[i-1][0], STDIN_FILENO); //receber input a partir da pipe anterior
	close(fd[i - 1][0]);
      }
      if (i < npipes) {
	dup2(fd[i][1], STDOUT_FILENO); //enviar output para a proxima pipe
      }
      
      execvp(commlist->cmd, commlist->argv);
    }
  
    else {
      //parent code after fork
      if(!background_exec)
	wait(NULL);
      if(i < npipes)
	close(fd[i][1]);
    }
    
    commlist = commlist->next;
    i++;
  }
} 

