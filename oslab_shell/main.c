#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
  Simple Shell (LSH-style)
  - Read line
  - Split into tokens
  - Execute builtins or launch external process
*/

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

/* Function declarations */
void lsh_loop(void);
char *lsh_read_line(void);
char **lsh_split_line(char *line);
int lsh_launch(char **args);
int lsh_execute(char **args);

/* Builtin declarations */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/* Builtin list */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[])(char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/* Builtin: cd */
int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/* Builtin: help */
int lsh_help(char **args) {
  (void)args;
  int i;
  printf("Stephen Brennan's LSH (lab version)\n");
  printf("Type program names and arguments, then hit enter.\n");
  printf("Builtins:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for info on other programs.\n");
  return 1;
}

/* Builtin: exit */
int lsh_exit(char **args) {
  (void)args;
  return 0;
}

/* Launch external program */
int lsh_launch(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    /* Child */
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    /* Error forking */
    perror("lsh");
  } else {
    /* Parent waits */
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
      (void)wpid;
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/* Execute: builtins or external */
int lsh_execute(char **args) {
  int i;

  if (args[0] == NULL) {
    /* empty command */
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/* Read a line from stdin (getline version) */
char *lsh_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0;

  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS); /* Ctrl-D */
    } else {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

/* Split line into tokens */
char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/* Main shell loop */
void lsh_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  /* Run command loop */
  lsh_loop();

  return EXIT_SUCCESS;
}
