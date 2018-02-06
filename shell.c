#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "syscall.h"

#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITER " \t\r\n\a"

char* command_history[1000];
int cc=0;
pid_t child_pid = -1;


void command_implement(char** args);

void welcome()
{
	printf("------------------------------------------------------------------------------\n");
	printf("            C based Virtual Unix Shell                                        \n");
	printf("------------------------------------------------------------------------------\n");
	printf("Srikar paruchuru          150101044\n");
	printf("------------------------------------------------------------------------------\n");
	printf("Note : rmexcept -f : deletes all FILES apart from the given list\n ");
	printf("       rmexcept    : deletes all files/directories apart from the given list\n");
	printf("------------------------------------------------------------------------------\n");
	return;
}

void print_pwd()
{
	char cwd[1024];
	getcwd(cwd,sizeof(cwd));
	printf("%s",cwd);
	printf(" $ ");
	return;
}

char *read_input(void)
{
	char *line = NULL;
	ssize_t bufsize = 0; 
	getline(&line, &bufsize, stdin);
	command_history[cc]=(char*)malloc(sizeof(line));
	strcpy(command_history[cc],line);
	return line;
}

char** tokenise_input(char* line)
{
	int bufsize = TOKEN_BUFFER_SIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	if (!tokens) 
	{
		fprintf(stderr, "lsh: allocation error\n");
		exit(0);
	}
	
	char *token;
	token = strtok(line,TOKEN_DELIMITER);
	while (token != NULL)
	{
		tokens[position] = token;
		position++;
		if (position >= bufsize)
		{
			bufsize += TOKEN_BUFFER_SIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) 
			{
		    	fprintf(stderr, "lsh: allocation error\n");
		    	exit(0);
		  	}
		}
		token = strtok(NULL,TOKEN_DELIMITER);
	}
	tokens[position] = NULL;
	return tokens;
}

int cd(char** args)
{
	if(args[1]==NULL)
	{
		printf("path is not specified!\n");
		return 0;
	}
	if(chdir(args[1])!=0)
	{
		printf("path is not found!\n");
		return 0;
	}
	return 1;
}

int ls(char** args)
{
	int f1 = fork();
	if(f1==0)
	{
		char* arg[] = {"/bin/ls",NULL};
		execvp(arg[0],args);
	}
	if(f1 > 0)
	{
		wait(NULL);
		return 1;
	}
	printf("ls fork failed\n");
	return 0;
}

int rm(char** args)
{
	int f1 = fork();
	if(f1==0)
	{
		char* arg[] = {"/bin/rm",NULL};
		execvp(arg[0],args);
	}
	if(f1 > 0)
	{
		wait(NULL);
		return 1;
	}
	printf("ls fork failed\n");
	return 0;
}

int convert_to_int(char* bla)
{
	int l=0;
	if(bla!=NULL)
	{
		int c=0;
		while(bla[c])
		{
			l = 10*l + (bla[c]-'0');
			c++;
		}
	}
	else
		l=cc;
	return l;
}

int history(char** args)
{
	int n = convert_to_int(args[1]);
	for(int i=cc-n;i>=0&&i<=cc-1;i++)
		printf("%d : %s",i+1,command_history[i]);
	return 1;
}

int issue(char** args)
{
	int n = convert_to_int(args[1]);
	if(n<1 || n>=cc+1)
	{
		printf("Invalid input\n");
		return 0;
	}
	printf("executing ... %s",command_history[n-1]);
	char * line = (char*)malloc(sizeof(command_history[n-1]));
	strcpy(line,command_history[n-1]);
	char** arg = tokenise_input(line);
	command_implement(arg);
	return 1;
}

int ex(char** args)
{
	exit(1);
}

void executable_implement(char** args)
{
	char *input,*output;
	int f2 = fork();
	if(f2==0)
	{
		for(int i=1;i<=2;i++)
		{
			if(args[i])
			{
				if(args[i][0]=='<')
				{
					input = (char*)malloc(sizeof(args[i])-1);
					for(int j=0;j<sizeof(args[i])-1;j++)
						input[j]=args[i][j+1];
					int fd0;
			        if ((fd0 = open(input, O_RDONLY, 0)) < 0) {
			            perror("Couldn't open input file");
			            exit(0);
			        }           
			        // dup2() copies content of fdo in input of preceeding file
			        dup2(fd0, STDIN_FILENO); // STDIN_FILENO here can be replaced by 0 

			        close(fd0);
				}
				if(args[i][0]=='>')
				{
					output = (char*)malloc(sizeof(args[i])-1);
					for(int j=0;j<sizeof(args[i])-1;j++)
						output[j]=args[i][j+1];
					int fd1 ;
			        if ((fd1 = creat(output , 0644)) < 0) {
			            perror("Couldn't open the output file");
			            exit(0);
			        }           

			        dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
			        close(fd1);
				}
			}
			else
				break;
		}
		if(execvp(args[0],args)!=0)
			printf("error\n");
		return;
	}
	if(f2 > 0)
	{
		wait(NULL);
		printf("%s succesfully executed.\n",args[0]);
		return;
	} 
	printf("fork failure\n");
	return;
}

int rmexcept(char **args)
{
	if(args[1]==NULL)
	{
		printf("Enter the filepaths!\n");
		return 0;
	}
	char *com = (char*)calloc(500,sizeof(char));
	strcat(com, "find .");
	for (int i = 2; args[i]!=NULL; ++i)
	{
		strcat(com," ! -name ");
		strcat(com,args[i]);
	}
	if(args[1] && args[1][0]!='\0' && args[1][1]=='f')
		strcat(com," -maxdepth 1 -type f -delete");
	else
	{
		strcat(com," ! -name ");
		strcat(com,args[1]);
		strcat(com," -maxdepth 1 -delete");

	}
	//strcat(com," 2>/dev/null");
	char **tokens = tokenise_input(com); 
	int f2 = fork();
	if(f2==0)
	{
		char* arg[] = {"/usr/bin/find",NULL};
		if(execvp(arg[0],tokens)==-1)
		{
			printf("error\n");
			return 0;
		}
	}
	if(f2 > 0)
	{
		wait(NULL);
		printf("Program rmexcept succesfully executed\n");
		return 1;
	}
	printf("fork error\n");
	return 0;
}

void kill_child(int sig)
{
    kill(child_pid,SIGKILL);
}

int m_executable_implement(char** args,int m)
{
	signal(SIGALRM,(void (*)(int))kill_child);
	int f2 = fork();
	if(f2==0)
	{
		printf("Running the program %s\n",args[0]);
		if(execvp(args[0],args)==-1)
		{
			printf("error\n");
			return 0;
		}
	}
	if(f2 > 0)
	{
		alarm(m);
		child_pid = f2;
		wait(NULL);
		return 1;
	}
	printf("fork error\n");
	return 0;
}


int check_m(char* a)
{
	if(a==NULL)
		return 0;
	int flag = 1;
	int j=0;
	while(a[j])
	{
		if(a[j]<'0' || a[j]>'9')
		{
			flag = 0;
			break;
		}
		j++;
	}
	return flag;
}

char* command[7] = {"cd","ls","rm","history","issue","exit","rmexcept"};
int (*command_execute[]) (char**) = {&cd,&ls,&rm,&history,&issue,&ex,&rmexcept};

void command_implement(char** args)
{
	int i=0;
	for(i=0;i<=6;i++)
	{
		if(strcmp(args[0],command[i])==0)
		{
			(*command_execute[i])(args);
			break;
		}
	}
	if(i==7)
	{
		int j=0;
		while(args[j])
			j++;
		if(j>=2 && check_m(args[j-1]))
		{
			int m = convert_to_int(args[j-1]);
			args[j-1]='\0';
			m_executable_implement(args,m);
		}
		else
			executable_implement(args);
	}
	return;
}


int main()
{
	welcome();
	while(1)
	{
		print_pwd();
		char* input = read_input();
		char** args = tokenise_input(input);
		command_implement(args);	
		cc++;
	}
			
}
