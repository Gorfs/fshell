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
    int token_count = 1;
    
    // premiere passage pars la liste pour trouver sa longeur
    token_count = len_tokens(input);
    
    // allocate the result array
    result = malloc(((token_count + 1) * sizeof(char*)));
    //error handling
    if (result == NULL){
        perror("error allocating space in tokenisation.c");
        goto error;
    }

    for(int j = 0 ; j <= token_count; j++){

        // on trouve la longeur du token
        int token_length = 0;
        int text = 0; // flag that indicates if we are in a text or not
        while(input[token_length] != '\n' && input[token_length] != '\0'){
            if (input[token_length] == '"') // if we find a quote we toggle the text flag
                text = !text;

            if (input[token_length] == DELIMITER) // if we find a delimiter we check if we are in a text or not
            {
                if (text) // if we are in a text we continue and remove the delimiter from the count
                    token_count--;
                else // if we are not in a text we break from the loop
                    break;
            }
            token_length++;
        }
        // allocate the memory for the token
        result[j] = malloc(token_length * sizeof(char));
        // error handling
        if (result[j] == NULL){
            perror("error allocating space in tokenisation.c");
            goto error;
        }
        // copy le token dans le resultat
        strncpy(result[j], input, token_length);

        if (j != token_count - 1){
            // on bouge le cursor devant pour le prochain token
            input += token_length + 1; // the +1 pour eviter le delimiteur
        }else{
            // avoiding segmentation fault
            break;
        }

    }
    //  on peut pas free() le result ici car on doit le retourner
    // pas tres encapsulatoir lol;
    result[token_count] = NULL;
    return result;

    error:
        free(result);
        return NULL;

}