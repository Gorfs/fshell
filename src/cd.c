#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <tokenisation.h>
#include <string.h>


int command_cd(char** command){
    char** tokens = tokenise(command[1], '/');
    char** rev_tokens = malloc(len_tokens(command[1], '/') * sizeof(char));
    char* cwd = getcwd(NULL, 0);
    if (cwd == NULL)
    {
        perror("can't find current working directory in cd.");
		return -1;
    }

    char** cwd_tokens = tokenise(cwd + 1, '/');
    int cwd_len = len_tokens(cwd+1, '/');
    for (size_t i = 0; tokens[i] != NULL; i++)
    {
        if (i == 0 && *tokens[0] == '\0')
    	{
    		if (chdir("/") == -1)
    		{
    			perror("can't change directory to root in cd.");
    			return -1;
    		}
    	}
        else if (chdir(tokens[i]) == 0)
		{
		    if (strcmp(tokens[i], "..") != 0)
		    {
		        rev_tokens[i] = "..";
		    }
		    else
		    {
	            free(cwd);
				cwd = getcwd(NULL, 0);
				if (cwd == NULL)
				{
				    perror("can't find current working directory in cd.");
				    return -1;
				}
				cwd_tokens = tokenise(cwd+1, '/');
				cwd_len = len_tokens(cwd+1, '/');
				rev_tokens[i] = cwd_tokens[cwd_len - i -1];
		    }
		}
		else
		{
		    for (size_t j = i-1; j > 0; j--)
		    {
	            chdir(rev_tokens[j]);
		    }
		    return -1;
		}
    }
    return 0;
}
