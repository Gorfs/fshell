#include <stdio.h>
#include <stdlib.h>
#include <tokenisation.h>
#include <regex.h> // we are sooo back
#include <string.h> // gonna be needed for string standard library functions


char* cmd_delimiters[] = {";", "|", "{", "}", ">", ">>", "<", "<<","|>", "|>>", "2>", "2>>", NULL};
// char* cmd_delimiters[] = { "|>>", "2>", "2>>",">>", "<<","|>" ,"<" ,";", "|", "{", "}", ">", NULL};
int cmd_delimiters_count = 12;

int is_delimiter(char* potential_delimiter){
    for(int i = 0 ; i < cmd_delimiters_count; i++){
        if(strcmp(potential_delimiter, cmd_delimiters[i]) == 0){
            return 1;
        }
    }
    return -1;
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

int earlist_delimiter(char* input){
    // returns the earlier index of a delimiter in the input string
    int min_index = -1;
    for(int i = 0; i < cmd_delimiters_count; i++){
        char* delimiter = cmd_delimiters[i];
        char* found = strstr(input, delimiter);
        if (found != NULL){
            if (min_index == -1 || found < input + min_index){
                min_index = found - input;
            }
        }
    }
    return min_index;
}
int len_first_delimiter(char* input){
    int index = earlist_delimiter(input);
    int len = 1;
    char* delimiter = malloc(5 * sizeof(char)); // bigger than all possible delimiters 
    delimiter = strncpy(delimiter, input + index, 2);
    if(delimiter == NULL){
        return -1;
    }
    while(is_delimiter(delimiter) == 1){
        len++;
        delimiter = strncpy(delimiter, input + index, len);
    }
    printf("delimiter is %s\n", delimiter);
    free(delimiter);
    return len;
}

char*** tokenise_cmds(char* input){
    if (input == NULL || strlen(input) == 0){
        return NULL;
    }
    char*** result = malloc(2 * sizeof(char**)); // minimum space required for the result + it's null terminator
    int result_size = 1;
    if(result == NULL){
        perror("error allocating space in tokenisation.c");
        return NULL;
    }
    int cmd_index = 0;

    while(*input == ' '){
        input++; // skip leading spaces
    }

    int delimiter_length = 0;
    int p_pointer = 0;
    int c_pointer = 0;
    while(c_pointer != -1){

        while(input[0] == ' '){
            input++; // skip leading spaces
        }
        // increase length of result by 1
        result = realloc(result, (result_size + 1) * sizeof(char**));
        result_size++;

        c_pointer = earlist_delimiter(input);
        if(c_pointer == -1){
            break;
        }
        char* string_to_tokenise = malloc((c_pointer + 1) * sizeof(char));
        if(string_to_tokenise == NULL){
            perror("error allocating space in tokenisation.c");
            goto error;
        }
        if(c_pointer == 0){ // we landed on the delimiter
            delimiter_length = len_first_delimiter(input);
            string_to_tokenise = malloc((delimiter_length + 1) * sizeof(char));
            strncpy(string_to_tokenise, input, delimiter_length);
        }else{
            strncpy(string_to_tokenise, input, c_pointer - p_pointer);
        }
        result[cmd_index] = malloc(sizeof(char**)); // minimum space required for the result of tokenise_cmd
        if(result[cmd_index] == NULL){
            perror("error allocating space in tokenisation.c");
            goto error;
        }
        result[cmd_index] = tokenise_cmd(string_to_tokenise);
        if(result[cmd_index] == NULL){
            perror("error tokenising command in tokenisation.c");
            goto error;
        }
        cmd_index++;
        input += c_pointer;
        if(c_pointer == 0){
            printf("delmiter length is %d\n", delimiter_length);
            input += delimiter_length;
            delimiter_length = 0;
        }
    }
    // parse final token
    result[cmd_index] = malloc(sizeof(char**)); // minimum space required for the result of tokenise_cmd
    if(result[cmd_index] == NULL){
        perror("error allocating space in tokenisation.c");
        goto error;
    }
    result[cmd_index] = tokenise_cmd(input);
    result[cmd_index + 1] = NULL; // null terminate the array

    return result;

    error:
        for (int i = 0; i < cmd_index; i++) {
            for (int j = 0; result[i][j] != NULL; j++) {
                free(result[i][j]);
            }
            free(result[i]);
        }
        free(result);
        return NULL;
}

void print_tokenised_cmds(char*** tokenised_cmds) {
    if (tokenised_cmds == NULL) {
        printf("No commands to print.\n");
        return;
    }

    for (int i = 0; tokenised_cmds[i] != NULL; i++) {
        printf("[");
        for (int j = 0; tokenised_cmds[i][j] != NULL; j++) {
            printf("\"%s\"", tokenised_cmds[i][j]);
            if (tokenised_cmds[i][j + 1] != NULL) {
                printf(", ");
            }
        }
        printf("]");
        if (tokenised_cmds[i + 1] != NULL) {
            printf(", ");
        }
    }
    printf("\n");
}
