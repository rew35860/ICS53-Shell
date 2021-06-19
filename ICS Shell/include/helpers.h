// A header file for helpers.c
// Declare any additional functions in this file
#include "icssh.h"
#include "linkedList.h"
#include <signal.h>

extern volatile sig_atomic_t c_flag;

int bg_comparator(void *lhs, void *rhs);
void create_bg(List_t *list, job_info *job, pid_t pid, time_t time);
void handler(int sig);
int findIndex(List_t *list, pid_t pid);
int checkRedirection(const job_info *job);
int redirectIn(char *fileName);
int redirectOut(char *fileName);
int redirectErr(char *fileName);
int makePipe(int input, int last, job_info *job, char* line, int i);