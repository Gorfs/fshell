#include <stdio.h>
#include <stdlib.h>
#include <tokenisation.h>
#include <string.h> // gonna be needed for string standard library functions


const char* cmd_delimiters[] = {";", "|", "{", "}", ">", ">>", "<", "<<", "|>>", "2>", "2>>", NULL};

int is_delimiter(char* potential_delimiter){
    for(size_t i = 0 ; cmd_delimiters[i] != NULL; i++){
        if(strcmp(potential_delimiter, cmd_delimiters[i]) == 0){
            return 1;
        }
    }
    return -1;
}


int len_cmds(char* input){
    // returns the number of commands seperated by delimiters in the input
    char* input_cpy = malloc(strlen(input) + 1);
    strcpy(input_cpy, input);
    char* cmd_token = strtok(input_cpy, " ");
    int cmd_nbr = 1;
    // determiner la longeur de la liste de commandes
    for(int i = 0; cmd_token != NULL; i++){
        if(is_delimiter(cmd_token)){
            cmd_nbr++;
        }
        cmd_token = strtok(NULL, " ");
    }
    free(input_cpy);
    return cmd_nbr;
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

int len_until_next_delimiter(char** tokens){
    // returns the amount of tokens until the next delimiter
    char** token = tokens;
    int count = 0;
    while(token != NULL){
        if (is_delimiter(*token)){
            return count;
        }
        count++;
        token++;
    }
    return count;

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
    // must have a null terminator at the end.
    if(strlen(input) == 0){ // base case , we can now suppose that len(input) > 0
        return NULL;
    }
    char* input_cpy = malloc(strlen(input) + 1); strcpy(input_cpy, input);
    char* save_ptr;

    char* cmd_token = strtok_r(input_cpy, " ", save_ptr);

    int cmd_nbr = len_cmds(input);    
    int cmd_index = 0;
    char*** result = malloc((cmd_nbr * 2) * sizeof(char**)); // does *2 to acount for the null terminator and the delimiter

    result[0] = malloc(2 * sizeof(char*));
    int j = 0;
    while(cmd_token != NULL){
        printf("cmd_at the first positions: %s\n",result[0][0] );
        if(is_delimiter(cmd_token) == -1){
            result[cmd_index][j] = strdup(cmd_token);
            j++;
        }else{
            result[cmd_index][j] = NULL;
            cmd_index++;
            result[cmd_index] = malloc(sizeof(char*));
            result[cmd_index][0] = strdup(cmd_token);
            cmd_index++;
            result[cmd_index] = malloc(2 * sizeof(char*));
            j = 0;
        }
        cmd_token = strtok_r(NULL, " ", save_ptr);
    }
    result[cmd_index + 1] = NULL;
    // TODO : maybe ad a null at the end of the final command, don't know if it is added already
    free(input_cpy);
    
    return result;
}