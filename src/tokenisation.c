#include <stdio.h>
#include <stdlib.h>
#include <tokenisation.h>
#include <string.h> // gonna be needed for string standard library functions


char DELIMITER = ' ';

int len_tokens(char* input){
    // returns the number of tokens in the input string
    int token_count = 1;
    for (int i = 0 ; i < strlen(input); i++){
        if (input[i] == DELIMITER){
            token_count++;
        }
    }
    return token_count;
}

// returns the list of tokens or NULL if an error occurs
char** tokenise(char* input){
    // returns a list of each token from the input string
    char** result = NULL;
    int token_count = 1;
    
    // premiere passage pars la liste pour trouver sa longeur
    token_count = len_tokens(input);
    
    // allocate the result array
    result = malloc(((token_count) * sizeof(char*)));
    //error handling
    if (result == NULL){
        perror("error allocating space in tokenisation.c");
        goto error;
    }

    for(int j = 0 ; j <= token_count; j++){
        // find the length of the token
        int token_length = 0;
        while(input[token_length] != DELIMITER && input[token_length] != '\n' && input[token_length] != '\0'){
            token_length++;
        }
        // allocate the memory for the token
        result[j] = malloc(token_length * sizeof(char));
        // error handling
        if (result[j] == NULL){
            perror("error allocating space in tokenisation.c");
            goto error;
        }
        // copy the token into the result array
        strncpy(result[j], input, token_length);

        if (j != token_count - 1){
            // move the input pointer to the next token
            input += token_length + 1; // the +1 is to skip the delimiter
        }else{
            // avoiding segmentation fault
            break;
        }

    }
    // can't free the result as if we do the tokens will be lost
    // pas tres encapsulatoir lol return result;
    return result;

    error:
        free(result);
        return NULL;

}