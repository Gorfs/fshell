#include <stdio.h>
#include <stdlib.h>
#include <tokenisation.h>
#include <string.h> // gonna be needed for string standard library functions


char DELIMITER = ' ';

int len_tokens(char* input){
    // retourn le nombre de tokens dans le string d'input
    // returns the number of tokens in the input string
    int token_count = 1;
    for (size_t i = 0 ; i < strlen(input); i++){
        if (input[i] == DELIMITER){
            token_count++;
        }
    }
    return token_count;
}

// retourn un tableau de string et NULL si il y a une erreur
// le tableau de TOKEN contient un null terminator NULL a la fin
char** tokenise(char* input){
    // returns a list of each token from the input string
    char** result = NULL;
    int token_count = len_tokens(input);

    // allocate the result array
    result = malloc((token_count + 1) * sizeof(char*));
    if (result == NULL){
        perror("error allocating space in tokenisation.c");
        return NULL;
    }

    int j = 0;
    while (*input) {
        // skip leading delimiters
        while (*input == DELIMITER) input++;

        // find the length of the token
        int token_length = 0;
        int text = 0; // flag that indicates if we are in a text or not
        while(input[token_length] != '\n' && input[token_length] != '\0'){
            if (input[token_length] == '"') // if we find a quote we toggle the text flag
                text = !text;

            if (input[token_length] == DELEMITER) // if we find a delimiter we check if we are in a text or not
            {
                if (text) // if we are in a text we continue and remove the delimiter from the count
                    token_count--;
                else // if we are not in a text we break from the loop
                    break;
            }
            token_length++;
        }

        if (token_length == 0)
            break;  // no more tokens

        // allocate memory for the token
        result[j] = malloc((token_length + 1) * sizeof(char));
        if (result[j] == NULL){
            perror("error allocating space in tokenisation.c");
            goto error;
        }

        // copy the token and add null terminator
        strncpy(result[j], input, token_length);
        result[j][token_length] = '\0';

        // move the input pointer to the start of the next token
        input += token_length;

        j++;
    }
    result[j] = NULL;  // null terminate the array
    return result;

    error:
        for (int i = 0; i < j; i++) {
            free(result[i]);
        }
        free(result);
        return NULL;
}
