#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

//headers signature----------------------------------------------------------------------------------------------------------------------------------
void INThandler(int);
char **splitLine(char *line);
char *splitPipe(char *line, int firstDesired);
int createProcess(char **args);
int processBackground(char **args);
int createProcessWithPipe(char **cmd1, char **cmd2);
void outputChanger(char *output);
int inputChanger(char *output);
void *searchPositionThread(void *arguments);
//end

typedef struct s_argsThread
{
  char **args;
  char symbol;
  int output;
} argsThread;

int main(int argc, char *argv[])//main program ------------------------------------------------------------------------------------------------------
{
  signal(SIGINT, INThandler);
  char *line;
  size_t bufsize = 64;
  line = (char *)malloc(bufsize * sizeof(char));
  char *path = (char *)malloc(bufsize * sizeof(char));
  char **args = NULL;

  while (1)
  {
    getcwd(path, bufsize);
    printf("%s :3 ", path);
    getline(&line, &bufsize, stdin);

    if (strchr(line, '|'))
    {
      char *cmd1 = splitPipe(line, 1);
      char *cmd2 = splitPipe(line, 0);
      createProcessWithPipe(splitLine(cmd1), splitLine(cmd2));
    }
    else
    {
      args = splitLine(line);
      if (args[0] == NULL)
      {
        continue;
      }
      if (strcmp(args[0], "cd") == 0)
      {
        if (args[1] == NULL)
        {
          args[1] = "/";
        }
        chdir(args[1]);
      }
      else
      {
        if (strcmp(args[0], "exit") == 0)
        {
          return 0;
        }
        createProcess(args);
      }
    }
  }
  return 0;
}//end main

int createProcess(char **args) //Creates a new process ----------------------------------------------------------------------------------------------
{
  int ultimaPosicao = 0;
  while (args[ultimaPosicao++]){}
  ultimaPosicao -= 2;

  //preparing thread calls
  pthread_t tid[2];

  argsThread params1;
  params1.args = args;
  params1.symbol = '>';

  argsThread params2;
  params2.args = args;
  params2.symbol = '<';

  int err = pthread_create(&(tid)[0], NULL, &searchPositionThread, (void *)&params1);
  pthread_join(tid[0], NULL);
  int err = pthread_create(&(tid[1]), NULL, &searchPositionThread, (void *)&params2);
  pthread_join(tid[1], NULL);

  //end thread calls

  int pos1 = params1.output; //retrieving output from thread1
  int pos2 = params2.output; //retrieving output from thread2

  if (!strcmp(args[ultimaPosicao], "&"))
  { args[ultimaPosicao] = NULL;
    processBackground(args);//call to background process
  }
  else
  {
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0)
    {
      if (pos1)
      { args[pos1] = NULL;
        outputChanger(args[pos1+1]);
      }
      if (pos2)
      {
        args[pos2] = NULL;
        inputChanger(args[pos2+1]);
      }
      if (execvp(args[0], args) == -1)
      { perror("createProcess");}
      exit(EXIT_FAILURE);
    }
    else
    { do
      {
        waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
  }
  return 1;
}

int processBackground(char **args) { //creates a new process in background mode----------------------------------------------------------------------
  pid_t pid = fork();
  int status;
  if (pid == 0)
  {
    if (!fork()) {
      execvp(args[0], args);
      exit(0);
    }
    else {
      exit(0);
    }
  }
  else
  { do
    { waitpid(pid, &status, WUNTRACED);}
    while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

  int createProcessWithPipe(char **cmd1, char **cmd2) //creates a processes using pipe---------------------------------------------------------------
  {
    pid_t pid = fork();
    int status;
    if (!pid)
    {
      int fd[2];
      pipe(fd);
      if (!fork())
      {
        close(1);     // close STD_OUT
        dup(fd[1]);   // make STD_OUT same as fd[1]
        close(fd[0]); // we don't need this
        execvp(cmd1[0], cmd1);
        exit(0);
      }
      else
      {
        close(0);     // close STD_IN
        dup(fd[0]);   // make STD_IN same as fd[0]
        close(fd[1]); // we don't need this
        execvp(cmd2[0], cmd2);
        exit(0);
      }
    }
    else
    {
      do
      {
        waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      return 1;
    }
  }

  char **splitLine(char *line) //split line into an array of char array------------------------------------------------------------------------------
  {
    char **splited = malloc(64 * sizeof(char *));
    char *token;
    int i = 0;

    token = strtok(line, " \t\r\n");
    while (token != NULL)
    {
      splited[i++] = token;
      token = strtok(NULL, " \t\r\n");
    }
    splited[i] = NULL;
    return splited;
  }

  /*If there is a '|' character in the array, then the method
  will return all characters before the '|' if the param @firstDesired
  is equal to 1, and all characters after the '|' if the param @firstDesired
  is equal to 0;
  */
  char *splitPipe(char *line, int firstDesired)//split if there is a '|' in the array----------------------------------------------------------------
  {
    int i = 0;
    while (line[i])
    {
      if (line[i++] == '|')
        break;
    }
    char *splited = (char *)malloc(64 * sizeof(char));
    int j = 0;
    if (firstDesired)
    {
      while (j < i - 2)
      {
        splited[j] = line[j];
        j++;
      }
    }
    else
    {
      i++;
      while (line[i + j])
      {
        splited[j] = line[i + j];
        j++;
      }
    }
    return splited;
  }

  void INThandler(int sig)//Handler of system signals-----------------------------------------------------------------------------------------------
  {
    char c;

    signal(sig, SIG_IGN);
    printf("  Are You Crazy? [Y/N] ");
    c = getchar();
    if (c == 'y' || c == 'Y')
      exit(0);
    else
      signal(SIGINT, INThandler);
    getchar();
  }

  void outputChanger(char *output) //Changes the default output to a new one------------------------------------------------------------------------
  {
    printf(output);
    int fd = open(output, O_WRONLY | O_CREAT, 0777);
    fclose(stdout);
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }

  int inputChanger(char *input) //Changes the default input to a new one ---------------------------------------------------------------------------
  {
    int fd = open(input, O_RDONLY);
    if (fd == -1)
    { perror(input);
      return EXIT_FAILURE;
    }
    fclose(stdin);
    dup2(fd, STDIN_FILENO);
    close(fd);
    return 1;
  }

  void *searchPositionThread(void *arguments)
  { //Returns the index of a character if it exists.-----------------------------------------------------
    argsThread *argus = (argsThread *)arguments;
    char** args = argus->args;
    char symbol = argus->symbol;
    
    char *result = NULL;
    int pos = 0;
    while (args[pos])
    {
      result = strrchr(args[pos], symbol);
      if (result)
        break;
      pos++;
    }
    if (!result)
      pos = 0;
    argus->output = pos;
  }
