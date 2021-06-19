#include "icssh.h"
#include "linkedList.h"
#include "helpers.h"
#include <time.h>
#include <readline/readline.h>

volatile sig_atomic_t c_flag = 0;

int main(int argc, char* argv[]) {
	int fd[2], fd1[2];
	int exec_result;
	int exit_status;
	int i = 1; 
	pid_t pid, pid1, pid2;
	pid_t wait_result;
	char* line;
	List_t list;
	list.head = NULL;
	list.length = 0;
	list.comparator = NULL;

#ifndef DEBUG
	rl_outstream = fopen("/dev/null", "w");
#endif

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

	// Child 
	if (signal(SIGCHLD, handler) == SIG_ERR)
	{
		perror("Failed to set child handler");
		exit(EXIT_FAILURE);
	}

    // print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {

		// PART 2 ----------------------------------------------------------
		// MY CODE START HERE :: Background Jobs ---------------------------
		// Terminating and deleting background processe(s)
		if(c_flag)
		{	
			while ((pid = waitpid(-1,NULL,WNOHANG)) > 0) {
				int index = findIndex(&list, pid);
				if (index > -1)
				{
					removeByIndex(&list, index);
				}
			}
			c_flag = 0; 
		}
		// -----------------------------------------------------------------


        // MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid
		job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}

        //Prints out the job linked list struture for debugging
        debug_print_job(job);

		// example built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			// Killing all bg process(s)

			node_t *iter = list.head; 
			while(iter != NULL)
			{
				bgentry_t *result = (bgentry_t *)iter->value; 
				printf(BG_TERM, result->pid, result->job->line);
				free_job(result->job);
				kill(result->pid, SIGKILL);
				iter = iter->next;
				removeFront(&list);
			}

			// Terminating the shell
			free(line);
			free_job(job);
            validate_input(NULL);
            return 0;
		}


		// PART 1 ----------------------------------------------------------
		// MY CODE START HERE :: BUILT-IN: cd ------------------------------
		if (strcmp(job->procs->cmd, "cd") == 0) 
		{ 
			char s[64]; 

			// Checking # of arguments, if 1 = direct to home
			if (job->procs->argc == 1) 
			{
				chdir(getenv("HOME"));
				fprintf(stdout, "%s\n",getcwd(s, sizeof(s)));
			}
			// If more than 1, only care for the second argc 
			else 
			{
				getcwd(s, sizeof(s));
				strcat(s, "/");

				if(chdir(strcat(s,job->procs->argv[1])) == 0)
				{
					fprintf(stdout, "%s\n",getcwd(s, sizeof(s)));
				}
				else
				{
					fprintf(stderr, DIR_ERR);
				}
			}
			continue;
		}
		// -----------------------------------------------------------------


		// MY CODE START HERE :: BUILT-IN: bglist -------------------------
		if (strcmp(job->procs->cmd, "bglist") == 0)
		{
			node_t *iter = list.head; 
			while(iter != NULL)
			{
				bgentry_t *b = (bgentry_t *)iter->value;
				print_bgentry(b);
				iter = iter->next;
			}
			continue;
		}
		// -----------------------------------------------------------------

		// MY CODE START HERE :: BUILT-IN: UCI -----------------------------
		if (strcmp(job->procs->cmd, "ascii53") == 0)
		{
			printf("\n#######################################################\n\n");
			printf("%30s\n","UCI : ICS 53");
			printf("             ___  ___  ________  ___\n");
			printf("            |\\  \\|\\  \\|\\   ____\\|\\  \\ \n");   
			printf("            \\ \\  \\\\\\  \\ \\  \\___|\\ \\  \\ \n");  
			printf("             \\ \\  \\\\\\  \\ \\  \\    \\ \\  \\ \n"); 
			printf("              \\ \\  \\\\\\  \\ \\  \\____\\ \\  \\ \n"); 
			printf("               \\ \\_______\\ \\_______\\ \\__\\ \n");
			printf("                \\|_______|\\|_______|\\|__| \n\n");
			printf("#######################################################\n\n");
			continue;
		}
		// -----------------------------------------------------------------

		// example of good error handling!
		// MY CODE START HERE :: BUILT-IN: estatus -------------------------
		if (strcmp(job->procs->cmd, "estatus") != 0)
		{
			// If it's pipe command 
			if(job->nproc > 1)
			{
				// Creating Pipe 
				pipe(fd);
			}

			if ((pid = fork()) < 0) {
				perror("fork error");
				exit(EXIT_FAILURE);
			}

			// MY CODE START HERE :: Background Jobs -----------------------
			if(job->bg)
			{
				// Creating bgentry_t 
				create_bg(&list, job, pid, time(NULL));
			}
		}
		// -----------------------------------------------------------------
	


		if (pid == 0 && strcmp(job->procs->cmd, "estatus") != 0) {  //If zero, then it's the child process
			// PART 4 ------------------------------------------------------------------
			// MY CODE START HERE :: Redirection ---------------------------------------
			// Check if the files are valid argument 
			if(checkRedirection(job))
			{
				fprintf(stderr, RD_ERR);
				
				free_job(job);  
				free(line);
    			validate_input(NULL);

				exit(EXIT_FAILURE);
			}

			if(job->in_file != NULL)
			{
				if(redirectIn(job->in_file) < 0)
				{
					free_job(job);  
					free(line);
					validate_input(NULL);

					exit(EXIT_FAILURE);
				}
			}

			if(job->procs->err_file != NULL)
			{
				if(redirectErr(job->procs->err_file) < 0)
				{
					free_job(job);  
					free(line);
					validate_input(NULL);

					exit(EXIT_FAILURE);
				}
			}

			if(job->out_file != NULL)
			{
				if(redirectOut(job->out_file) < 0)
				{
					free_job(job);  
					free(line);
					validate_input(NULL);

					exit(EXIT_FAILURE);
				}
			}
			// -------------------------------------------------------------------------

			if(job->nproc > 1) 
			{
				dup2(fd[1], STDOUT_FILENO);
				close(fd[0]);
			}


			//get the first command in the job list
		    proc_info* proc = job->procs;
			exec_result = execvp(proc->cmd, proc->argv);
			if (exec_result < 0) {  //Error checking
				printf(EXEC_ERR, proc->cmd);
				
				// Cleaning up to make Valgrind happy 
				// (not necessary because child will exit. Resources will be reaped by parent or init)
				free_job(job);  
				free(line);
    			validate_input(NULL);

				exit(EXIT_FAILURE);
			}

		} else {

				if(job->nproc > 1)
				{
					close(fd[1]);
					
					int input = fd[0];
					
					while(i < job->nproc-1)
					{
						input = makePipe(input,0, job, line, i);
						++i;
					}

					makePipe(input,1, job, line, i);
				}

				if(!(job->bg))
				{	
						// As the parent, wait for the foreground job to finish
						wait_result = waitpid(pid, &exit_status, 0);
						
						if (job->nproc > 1)
						{
							while(i > 0)
							{
								waitpid(-1, NULL, 0);
								--i;
							}
						}
						

						// MY CODE START HERE :: BUILT-IN: estatus -------------------------
						if (strcmp(job->procs->cmd, "estatus") == 0) 
						{
							if ( WIFEXITED(exit_status) ) 
							{
								printf("%d\n", WEXITSTATUS(exit_status));
							}
							continue;
						}
						// -----------------------------------------------------------------

						if (wait_result < 0) {
							printf(WAIT_ERR);
							exit(EXIT_FAILURE);
						}
					
					
				}
				
		
		}


		if(!(job->bg))
		{
			free_job(job);  // if a foreground job, we no longer need the data
			free(line);
		}
	}

    // calling validate_input with NULL will free the memory it has allocated
    validate_input(NULL);

#ifndef DEBUG
	fclose(rl_outstream);
#endif
	return 0;
}
