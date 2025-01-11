#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// include our own files
#include <tokenisation.h>

#define MAX_DELIMITER_SIZE 3
// list of delimiters, MUST BE SORTED BY LENGTH | VERY IMPORTANT
// !: the order of the delimiters is important
char *cmd_delimiters[] = {";",  "|",   "{",  "}",  ">",   ">>",  "<", "<<",
                          "|>", "|>>", ">|", "2>", "2>>", "2>|", NULL};
int cmd_delimiters_count = 14;


/**
 * @brief Function that checks if a string is a delimiter.
 * @param potential_delimiter the string to check
 * @return int : 1 if the string is a delimiter, 0 otherwise
 */
int is_delimiter(char *potential_delimiter) {
    for (int i = 0; i < cmd_delimiters_count; i++) {
        if (strcmp(potential_delimiter, cmd_delimiters[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


/**
 * @brief Function that returns the length of a command.
 * @param input input string
 * @return the length of the command
 */
int input_length(char *input) {
    int token_count = 1;
    for (size_t i = 0; i < strlen(input); i++) {
        if (input[i] == ' ') {
            token_count++;
        }
    }
    return token_count;
}


/**
 * @brief Function that returns an array of string from a string.
 *        It splits the string in single word.
 * @param input input string to be tokenised
 * @return an array of tokens
 */
char **tokenise_cmd(char *input) {
    char **result = NULL;
    int token_count = input_length(input);

    // allocate the result array
    result = malloc((token_count + 1) * sizeof(char *));
    if (result == NULL) { // Error handling
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
        while (input[token_length] != '\n' && input[token_length] != '\0') {
            if (input[token_length] == '"') text = !text; // if we find a quote we toggle the text flag
            if (input[token_length] == ' ') { // if we find a delimiter we check if we are in a text or not
                if (text) token_count--; // if we are in a text we continue and remove the delimiter from the count
                else break; // if we are not in a text we break from the loop
            }
            token_length++;
        }
        if (token_length == 0) break; // no more tokens

        // allocate memory for the token
        result[j] = malloc((token_length + 1) * sizeof(char));
        if (result[j] == NULL) {
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
    result[j] = NULL; // null terminate the array
    return result;

    error:
        for (int i = 0; i < j; i++) {
            free(result[i]);
        }
        free(result);
        return NULL;
}


/**
 * @brief Function that returns the first delimiter in a string.
 * @param input the string to search in
 * @return the first delimiter found
 */
char* earliest_delimiter(char *input) {
    // returns the earlier index of a delimiter in the input string
    int min_index = -1;
    char* min_delimiter = NULL;
    for (int i = cmd_delimiters_count - 1; i >= 0; i--) {
        char *delimiter = cmd_delimiters[i];
        char *found = strstr(input, delimiter);
        if (found != NULL) {
            if (min_index == -1 || found < input + min_index) {
                min_index = found - input;
                min_delimiter = delimiter;
            }
        }
    }
    return min_delimiter;
}


/**
 * @brief Function that returns the index of the first delimiter in a string.
 * @param input the string to search in
 * @return the index of the first delimiter found, -1 if no delimiter is found
 */
int earliest_delimiter_index(char *input) {
    if (input == NULL) return -1; // if the input is NULL, there is no delimiter
    else if (strlen(input) == 0) return -1; // if the input is empty, there is no delimiter
    else if (earliest_delimiter(input) == NULL) return -1; // no delimiter found
    char* result = strstr(input, earliest_delimiter(input)); // find the first delimiter
    if (result == NULL) return -1; // no delimiter found
    else return result - input; // return the index of the delimiter
}


/**
 * @brief Function that returns the index of the earliest unmatched closing bracket in a string.
 * @param input the string to search in
 * @return the index of the earliest unmatched closing bracket found, -1 if no unmatched closing bracket is found
 */
int earliest_unmatched_closing_bracket(char* input) {
    int number_of_open_brackets = 0;
    int index = 0;
    for (size_t i = 0; i < strlen(input); i++) {
        if (input[i] == '{') number_of_open_brackets++;
        else if (input[i] == '}') {
            if (number_of_open_brackets == 0) return index;
            else number_of_open_brackets--;
        }
        index++;
    }
    return -1; // No unmatched closing bracket found
}


int detect_pipeline(char *input) {
    for (size_t i = 0; i < strlen(input); i++) {
        if (input[i] == '|' && input[i-1] == ' ' && input[i+1] == ' ') return i;
    }
    return -1;
}


char*** tokenise_pipeline(char* input) {
    int res_i = 0; // index of the result array
    int res_max_size = 2; // initial size of the result array
    char*** result = malloc(res_max_size * sizeof(char**)); // minimum space required for the result + it's null terminator
    while(*input) {
        result = realloc(result, (res_max_size + 1) * sizeof(char**));
        if (result == NULL) { // Error handling
            perror("error reallocating space in tokenisation.c");
            goto error;
        }
        result[res_i] = malloc(2 * sizeof(char*));
        if (result[res_i] == NULL) { // Error handling
            perror("error allocating space in tokenisation.c");
            goto error;
        }
        int next_delimiter = detect_pipeline(input); // index of the next delimiter
        char* string_to_tokenise; // string to store

        if (next_delimiter == -1) { // No more delimiters
            string_to_tokenise = malloc((strlen(input) + 1) * sizeof(char));
            if (string_to_tokenise == NULL) { // Error handling
                perror("error allocating space in tokenisation.c");
                goto error;
            }
            strcpy(string_to_tokenise, input);
            result[res_i][0] = string_to_tokenise;
            result[res_i][1] = NULL;
            input += strlen(input);
        } else if (next_delimiter == 0) { // Delimiter at the beginning of the string
            result[res_i][0] = malloc(2 * sizeof(char)); // "|" + null terminator
            if (result[res_i][0] == NULL) { // Error handling
                perror("error allocating space in tokenisation.c");
                goto error;
            }
            strncpy(result[res_i][0], "|", 2);
            result[res_i][1] = NULL;
            input += 1; // skip the delimiter
        } else {
            string_to_tokenise = malloc((next_delimiter + 1) * sizeof(char));
            if (string_to_tokenise == NULL) { // Error handling
                perror("error allocating space in tokenisation.c");
                goto error;
            }
            strncpy(string_to_tokenise, input, next_delimiter);
            string_to_tokenise[next_delimiter] = '\0';
            result[res_i][0] = string_to_tokenise;
            result[res_i][1] = NULL;
            input += next_delimiter; // skip the string we just tokenised
        }
        res_max_size++;
        res_i++;
    }
    result[res_i] = NULL; // null terminate the array
    return result;

    error:
    for (int i = 0; i < res_i; i++) {
        for (int j = 0; result[i][j] != NULL; j++) {
            free(result[i][j]);
        }
        free(result[i]);
    }
    free(result);
    return NULL;
}


/**
 * @brief Function that tokenise a string of commands, it uses a 2-pointers algorithm
 * to separate the commands by delimiters then by spaces.
 * @param input the string to be tokenised
 * @return 2d array of commands and delimiters to be executed , NULL if there is an error
 *  */ 
char*** tokenise_cmds(char* input) {
    if (input == NULL || strlen(input) == 0) return NULL;
    char*** result = malloc(2 * sizeof(char**)); // minimum space required for the result + it's null terminator
    if(result == NULL) { // Error handling
        perror("error allocating space in tokenisation.c");
        return NULL;
    }
    int cmd_index = 0;
    int number_of_open_brackets = 0;
    char* last_delimiter = NULL;
    while (*input == ' ') input++; // skip leading spaces

    int delimiter_length = 0;
    int c_pointer = 0;

    if (detect_pipeline(input) > 0) { // detected a "|" delimiter
        free(result);
        return tokenise_pipeline(input);
    }

    while(*input) {
        while (input[0] == ' ') input++; // skip leading spaces
        if (!*input) break;

        result = realloc(result, (cmd_index + 2) * sizeof(char**));
        if (result == NULL) { // Error handling
            perror("erreur de réallocation de mémoire");
            goto error;
        }
        if (last_delimiter != NULL && strcmp(last_delimiter, "{") == 0) {
            // On a trouvé une accolade ouvrante, on cherche une accolade fermante
            c_pointer = earliest_unmatched_closing_bracket(input);
        } else {
            // On cherche une délimitation
            c_pointer = earliest_delimiter_index(input);
        }
        if(c_pointer == -1) {
            // Plus de délimitations, on continue sur la suite de l'input
            result[cmd_index] = tokenise_cmd(input);
            if(result[cmd_index] == NULL) { // Error handling
                perror("erreur de tokenisation de la commande");
                goto error;
            }
            cmd_index++;
            break;
        }

        if (c_pointer == 0) {
            // On a trouvé un délimiteur au début de l'input
            delimiter_length = strlen(earliest_delimiter(input));
            if (delimiter_length == 0) { // Error handling
                perror("erreur de calcul de la longueur du délimiteur");
                goto error;
            }
            // On copie le délimiteur dans une nouvelle chaîne de caractères
            char* delimiter = malloc((delimiter_length + 1) * sizeof(char));
            if (delimiter == NULL) { // Error handling
                perror("erreur d'allocation de mémoire");
                goto error;
            }
            strncpy(delimiter, input, delimiter_length);
            delimiter[delimiter_length] = '\0';

            result[cmd_index] = malloc(2 * sizeof(char*));
            if (result[cmd_index] == NULL) {
                perror("erreur d'allocation de mémoire");
                free(delimiter);
                goto error;
            }
            result[cmd_index][0] = delimiter;
            last_delimiter = delimiter;
            if (strcmp(last_delimiter, "{") == 0) {
                number_of_open_brackets++;
            } else if (strcmp(last_delimiter, "}") == 0) {
                number_of_open_brackets--;
            }
            result[cmd_index][1] = NULL;
            cmd_index++;
            input += delimiter_length;
        } else {
            // On a trouvé un token avant le délimiteur
            char* string_to_tokenise;
            if ((last_delimiter == NULL || strcmp(last_delimiter, "{") != 0) && strncmp(input, "if", strlen("if")) == 0) {
                // l'input commence par "if" et n'est pas précédé d'une accolade ouvrante
                result[cmd_index] = malloc(3 * sizeof(char*)); // On alloue de la mémoire pour 3 tokens
                if (result[cmd_index] == NULL) { // Error handling
                    perror("erreur d'allocation de mémoire");
                    goto error;
                }
                result[cmd_index][0] = malloc(3 * sizeof(char)); // On alloue de la mémoire pour le token "if".
                if (result[cmd_index][0] == NULL) { // Error handling
                    perror("erreur d'allocation de mémoire");
                    goto error;
                }
                strncpy(result[cmd_index][0], "if", 3); // On copie le token "if"
                input += 3; // On avance le pointeur d'input pour ignorer "if".
                c_pointer -= 3; // On décrémente la taille du token "if" de la taille du prochain délimiteur.

                // Si le prochain caractère n'est pas le premier bloc de "if", on cherche le début du premier bloc.
                if (input[c_pointer] != '{') while (input[c_pointer + 1] != '{' && input[c_pointer + 1] != '\0') c_pointer++;
                // On alloue de la mémoire pour la condition du "if".
                string_to_tokenise = malloc((c_pointer + 1) * sizeof(char));
                if (string_to_tokenise == NULL) { // Error handling
                    perror("erreur d'allocation de mémoire");
                    goto error;
                }
                strncpy(string_to_tokenise, input, c_pointer); // On copie la condition du "if".
                string_to_tokenise[c_pointer] = '\0';
                result[cmd_index][1] = string_to_tokenise; // On ajoute la condition du "if" aux tokens.
                result[cmd_index][2] = NULL; // On termine le tableau de tokens.
                cmd_index++;
                input += c_pointer;
                continue; // On passe à la prochaine itération de la boucle.
            }

            string_to_tokenise = malloc((c_pointer + 1) * sizeof(char));
            if (string_to_tokenise == NULL) { // Error handling
                perror("erreur d'allocation de mémoire");
                goto error;
            }
            strncpy(string_to_tokenise, input, c_pointer);
            string_to_tokenise[c_pointer] = '\0';
            if (last_delimiter != NULL && strcmp(last_delimiter, "{") == 0) {
                result[cmd_index] = malloc(2 * sizeof(char*));
                if (result[cmd_index] == NULL) {
                    perror("erreur d'allocation de mémoire");
                    free(string_to_tokenise);
                    goto error;
                }
                result[cmd_index][0] = string_to_tokenise;
                result[cmd_index][1] = NULL;
                last_delimiter = NULL;
            } else {
                result[cmd_index] = tokenise_cmd(string_to_tokenise);
                if (result[cmd_index] == NULL) {
                    perror("erreur d'allocation de mémoire");
                    free(string_to_tokenise);
                    goto error;
                }
                free(string_to_tokenise);
            }
            cmd_index++;
            input += c_pointer;
            while (*input == ' ') input++; // skip trailing spaces
        }
    }
    result[cmd_index] = NULL; // null terminate the array
    if (number_of_open_brackets == 0) return result;
    else goto error;

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
 * @brief Function that prints the result of the tokenisation.
 * // TODO: for debugging purposes, to remove later
 * @param tokenised_cmds array of tokenised commands
 */
void print_tokenised_cmds(char ***tokenised_cmds) {
    if (tokenised_cmds == NULL) {
        printf("No commands to print.\n");
        return;
    }

    for (int i = 0; tokenised_cmds[i] != NULL; i++) {
        printf("[");
        for (int j = 0; tokenised_cmds[i][j] != NULL; j++) {
            printf("\"%s\"", tokenised_cmds[i][j]);
            if (tokenised_cmds[i][j + 1] != NULL) printf(", ");
        }
        printf("]");
        if (tokenised_cmds[i + 1] != NULL) printf(", ");
    }
    printf("\n");
}
