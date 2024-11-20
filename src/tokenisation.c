#include <stdio.h>
#include <stdlib.h>
#include <tokenisation.h>
#include <string.h> // gonna be needed for string standard library functions


char* cmd_delimiters = {";|"};

int is_cmd_delimiter(char c){
    // returns 1 if the character is a command delimiter
    for (size_t i = 0; i < strlen(cmd_delimiters); i++){
        if (c == cmd_delimiters[i]){
            return 1;
        }
    }
    return 0;
}

int len_cmds(char* input){
    // returns the amount of commands in an input
    int cmd_count = 1;
    for (size_t i = 0 ; i < strlen(input); i++){
        if (is_cmd_delimiter(input[i])){
            cmd_count++;
        }
    }
    return cmd_count;
}
char* delimiters_in_order(char* input){
    // returns a string with each delimiter in order of it's appearance example : ";;|{}"
    char* result = malloc(len_cmds(input) * sizeof(char));
    if(result == NULL){
        perror("error allocating space in tokenisation.c");
        return NULL;
    }
    result[0] = '\0';
    for(size_t i = 0; i < strlen(input); i++){
        if (is_cmd_delimiter(input[i])){
            result[i] = input[i];
        }
    }
    return result;
}

int len_tokens(char* input){
    // retourn le nombre de tokens dans le string d'input
    // returns the number of tokens in the input string
    int token_count = 1;
    for (size_t i = 0 ; i < strlen(input); i++){
        if (input[i] == ' '){
            token_count++;
        }
    }
    return token_count;
}


// retourn un tableau de string et NULL si il y a une erreur
// le tableau de TOKEN contient un null terminator NULL a la fin
char** tokenise_cmd(char* input){
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
        while (*input == ' ') input++;

        // find the length of the token
        int token_length = 0;
        int text = 0; // flag that indicates if we are in a text or not
        while(input[token_length] != '\n' && input[token_length] != '\0'){
            if (input[token_length] == '"') // if we find a quote we toggle the text flag
                text = !text;

            if (input[token_length] == ' ') // if we find a delimiter we check if we are in a text or not
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

char*** tokenise_input(char* input){
    // takes in char* input and returns a 3D array of tokens
    if(strlen(input) == 0){ // base case , we can now suppose that len(input) > 0
        return NULL;
    }
    char* input_cpy = malloc(strlen(input) + 1);
    if (strcpy(input_cpy, input) == NULL){
        perror("error copying input in tokenisation.c");
        return NULL;
    }
    // array of tokenized commands
    char** result = malloc((len_cmds(input) +1) * sizeof(char*));
    char* cmd = strtok(input_cpy, cmd_delimiters);
    printf("cmd : %s\n", cmd);
    int i = 0;

    while(cmd != NULL){
        result[i] = tokenise_cmd(cmd);
        i++;
        cmd = strtok(NULL, cmd_delimiters);
    }
    result[i] = NULL; // null terminator for the end of the commands
    free(input_cpy);
    return result;
}