// Your helper functions need to be here.
#include "helpers.h"


void create_bg(List_t *list, job_info *job, pid_t pid, time_t time)
{
    bgentry_t *bg = malloc(sizeof(bgentry_t));
    bg->job = job;
    bg->pid =pid;
    bg->seconds = time;

    insertRear(list, (void *) bg);
}

void handler(int sig) 
{ 
	c_flag = 1;
} 

int findIndex(List_t *list, pid_t pid)
{
    int i = 0; 
    node_t *iter = list->head; 
    while(iter != NULL)
    {
        bgentry_t *b = (bgentry_t *)iter->value;
        if (b->pid == pid)
        {
            printf(BG_TERM, b->pid, b->job->line);
            return i; 
        }
        iter = iter->next;
        ++ i; 
    }
    return -1;
}


int checkRedirection(const job_info *job)
{
    if(job->in_file != NULL && job->out_file != NULL)
    {
        if(strcmp(job->in_file, job->out_file) == 0)
        {
            return 1;
        }
    }

    if(job->out_file != NULL && job->procs->err_file != NULL)
    {
        if(strcmp(job->procs->err_file, job->out_file) == 0)
        {
            return 1;
        }
    }

    if(job->procs->err_file != NULL && job->in_file != NULL)
    {
        if(strcmp(job->procs->err_file, job->in_file) == 0)
        {
            return 1;
        }
    }

    return 0;
}

int redirectIn(char *fileName)
{
    int in;
    if((in = open(fileName, O_RDONLY)) < 0)
    {
        fprintf(stderr, RD_ERR);
        return in;
    }
    dup2(in, STDIN_FILENO);
    close(in);
    return 0; 
}

int redirectOut(char *fileName)
{
    int out;
    if( (out = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
    {
        return out;
    }
    dup2(out, STDOUT_FILENO);
    close(out);
    return 0; 
}

int redirectErr(char *fileName)
{
    int err;
    if ((err = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0)
    {
        return err;
    }
    dup2(err, STDERR_FILENO);
    close(err);
    return 0; 
}

int makePipe(int input, int last, job_info *job, char* line, int i)
{
    // printf("in make pipe : [%d]", input);
    int fd[2];
    int n = 0; 
    pid_t pid1;
    
    pipe(fd); 

    if ((pid1 = fork()) < 0) 
    {
        perror("fork error");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0)
    {
        // the middle pipe
        if (last == 0)
        {
            dup2(input, STDIN_FILENO);
            dup2(fd[1], STDOUT_FILENO);
        } else {
            dup2(input, STDIN_FILENO);
        }

        //get the command in the job list
        proc_info* proc = (job->procs);

        while(n < i)
        {
            proc = proc->next_proc;
            ++n;
        }

        if (execvp(proc->cmd, proc->argv) < 0) {  //Error checking
            printf(EXEC_ERR, proc->cmd);
            
            // Cleaning up to make Valgrind happy 
            // (not necessary because child will exit. Resources will be reaped by parent or init)
            free_job(job);  
            free(line);
            validate_input(NULL);

            exit(EXIT_FAILURE);
        }
    }

    close(input);

    close(fd[1]);

    if (last == 1)
    {
        close(fd[0]);
    }

    // printf("in make pipe end : [%d] [%d]", input, fd[0]);
    return fd[0];
}