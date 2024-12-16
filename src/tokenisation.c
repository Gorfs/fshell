#include <stdio.h>
#include <stdlib.h>
#include <tokenisation.h>
#include <string.h> 


#define MAX_DELIMITER_SIZE 3
char* cmd_delimiters[] = {";", "|", "{", "}", ">", ">>", "<", "<<","|>", "|>>", "2>", "2>>", NULL};
int cmd_delimiters_count = 12;

/**
 * @brief fonction qui retourne si un string est un délimiteur ou non
 * @param potential_delimiter : le string à tester
 * @return int : 1 si c'est un délimiteur, -1 sinon
 */
int is_delimiter(char* potential_delimiter){
    for(int i = 0 ; i < cmd_delimiters_count; i++){
        if(strcmp(potential_delimiter, cmd_delimiters[i]) == 0){
            return 1;
        }
    }
    return -1;
}


int len_tokens(char* input){ // TODO changer le nom de la fonction pour etre plus precis
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
/**
 * @brief fonction qui retourne un tableau de string qui represent chaque argument
 * d'une command du shell 
 * @param input : le string d'input
 * @return char** : le tableau de tokens, NULL si il y a une erreur
 */
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

/**
 * @brief fonction qui retourne l'index du premier délimiteur dans un string
 * @param input : le string d'input
 * @return int : l'index du premier délimiteur, -1 si il n'y a pas de délimiteur
 */
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

/**
 * @brief fonction qui retourne la longueur du premier délimiteur trouvé dans le string d'input
 * @param input : le string d'input
 * @return int :la longueur du premier délimiteur trouvé dans le string d'input
 * 0 si il n'y a pas de délimiteur
 */
int len_first_delimiter(char* input) {
    if (input == NULL){
        return 0; // on gere le NULL.
    }

    int index = earlist_delimiter(input);
    if(index == -1){
        return 0; // error handeling
    }
    int len = 0;
    char delimiter[MAX_DELIMITER_SIZE + 1];  // le +1 pour le null terminator
    
    // On construit le string du délimiteur
    while (input[index + len]) {
        delimiter[len] = input[index + len];
        delimiter[len + 1] = '\0';
        if (is_delimiter(delimiter) == 1) {
            len++;
        } else {
            delimiter[len] = '\0';
            break;
        }
    }
    return len;
}

/**
 * @brief algorithm a 2-pointers  pour separer les commandes par delmiters puis par espaces
 * pour plus facilement les traiter ensuite. 
 * @param input : the string to be tokenised
 * @return 2d array of commands and delimiters to be executed , NULL if there is an error
 *  */ 
char*** tokenise_cmds(char* input){
    if (input == NULL || strlen(input) == 0){
        return NULL;
    }
    char*** result = malloc(2 * sizeof(char**)); // minimum space required for the result + it's null terminator
    //int result_size = 2;
    if(result == NULL){
        perror("error allocating space in tokenisation.c");
        return NULL;
    }
    int cmd_index = 0;
    char* last_delimiter = NULL;
    while(*input == ' '){
        input++; // skip leading spaces
    }

    int delimiter_length = 0;
    //int p_pointer = 0;
    int c_pointer = 0;

    while(*input){
        while(input[0] == ' '){
            input++; // skip leading spaces
        }
        result = realloc(result, (cmd_index + 2) * sizeof(char**));
        if (result == NULL) {
            perror("erreur de réallocation de mémoire");
            goto error;
        }

        c_pointer = earlist_delimiter(input);
        if(c_pointer == -1){
            //Plus de délimitations, on continue sur la suite de l'input
            result[cmd_index] = tokenise_cmd(input);
            if(result[cmd_index] == NULL){
                perror("erreur de tokenisation de la commande");
                goto error;
            }
            cmd_index++;
            break;
        }

        if(c_pointer == 0){
            //On a trouvé un délimiteur au début de l'input
            delimiter_length = len_first_delimiter(input);
            char* delimiter = malloc((delimiter_length + 1) * sizeof(char));
            if (delimiter == NULL){
                perror("erreur d'allocation de mémoire");
                goto error;
            }
            strncpy(delimiter, input, delimiter_length);
            delimiter[delimiter_length] = '\0';
            
            result[cmd_index] = malloc(2 * sizeof(char*));
            if(result[cmd_index] == NULL){
                perror("erreur d'allocation de mémoire");
                free(delimiter);
                goto error;
            }
            result[cmd_index][0] = delimiter;
            last_delimiter = delimiter;
            result[cmd_index][1] = NULL;
            cmd_index++;
            input += delimiter_length;
        }
        else{
            //On a trouvé un token avant le délimiteur;
            char* string_to_tokenise = malloc((c_pointer + 1) * sizeof(char));
            if(string_to_tokenise == NULL){
                perror("erreur d'allocation de mémoire");
                goto error;
            }
            strncpy(string_to_tokenise, input, c_pointer);
            string_to_tokenise[c_pointer] = '\0';
            if (last_delimiter != NULL && strcmp(last_delimiter, "{") == 0){
                result[cmd_index] = malloc(2 * sizeof(char*));
                if(result[cmd_index] == NULL){
                    perror("erreur d'allocation de mémoire");
                    free(string_to_tokenise);
                    goto error;
                }
                result[cmd_index][0] = string_to_tokenise;
                result[cmd_index][1] = NULL;
            }
            else{
                result[cmd_index] = tokenise_cmd(string_to_tokenise);
                if(result[cmd_index] == NULL){
                    perror("erreur d'allocation de mémoire");
                    free(string_to_tokenise);
                    goto error;
                }
                free(string_to_tokenise);
            }
            cmd_index++;
            input += c_pointer;
        }
    }
    result[cmd_index] = NULL; // null terminate the array

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


/**
 * @brief fonction de debug pour afficher le resultat de la tokenisation
 * @param tokenised_cmds : tableau de commandes tokenisées
 * @return void
 */
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
