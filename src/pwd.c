#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
// personal files includes
#include <pwd.h>

#define PATH_MAX 4096
/**
 * @brief fonction qui retourne le chemin absolu du répertoire courant
 * @return char* : le chemin absolu du répertoire courant
 */
char* my_getcwd() {
    // le buffer du resultat
    char* buffer = malloc(PATH_MAX);
    if (buffer == NULL) {
        perror("malloc");
        return NULL;
    }

    char* path = buffer + PATH_MAX - 1;
    *path = '\0';

    int fd = open(".", O_RDONLY);
    if (fd == -1) {
        perror("open");
        free(buffer);
        return NULL;
    }

    struct stat current_stat, parent_stat;
    if (lstat(".", &current_stat) == -1) {
        perror("lstat");
        close(fd);
        free(buffer);
        return NULL;
    }

    while (1) {
        if (lstat("..", &parent_stat) == -1) {
            perror("lstat");
            close(fd);
            free(buffer);
            return NULL;
        }

        if (current_stat.st_ino == parent_stat.st_ino) {
            // on a atteint la racine
            if (path[0] == '\0') {
                *--path = '/';
            }
            break;
        }

        DIR* parent_dir = opendir("..");
        if (parent_dir == NULL) {
            perror("opendir");
            close(fd);
            free(buffer);
            return NULL;
        }

        if (chdir("..") == -1) {
            perror("chdir");
            closedir(parent_dir);
            close(fd);
            free(buffer);
            return NULL;
        }

        struct dirent* entry;
        int found = 0;
        while ((entry = readdir(parent_dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            struct stat entry_stat;
            if (lstat(entry->d_name, &entry_stat) == -1) {
                continue;
            }

            if (entry_stat.st_ino == current_stat.st_ino && entry_stat.st_dev == current_stat.st_dev) {
                int name_len = strlen(entry->d_name);
                if ((path - buffer) < name_len + 1) {
                    // Buffer overflow
                    fprintf(stderr, "Path too long\n");
                    closedir(parent_dir);
                    close(fd);
                    free(buffer);
                    return NULL;
                }

                path -= name_len;
                memcpy(path, entry->d_name, name_len);
                *--path = '/';
                found = 1;
                break;
            }
        }

        closedir(parent_dir);

        if (!found) {
            fprintf(stderr, "Directory entry not found\n");
            close(fd);
            free(buffer);
            return NULL;
        }

        current_stat = parent_stat;
    }

    // Restore original directory
    if (fchdir(fd) == -1) {
        perror("fchdir");
        close(fd);
        free(buffer);
        return NULL;
    }
    close(fd);

    char* result = strdup(path);
    free(buffer);
    return result;
}

/**
 * @brief fonction qui affiche le répertoire courant dans le file_descriptor
 * @param file_descriptor : le file descriptor où on va écrire le répertoire courant
 * @return int : 1 si une erreur se produit, 0 sinon
 */
int command_pwd(int file_descriptor){
    // Initialize the working directory string
    char* dir_string = NULL;
    char* current_working_directory = my_getcwd();
    if (current_working_directory == NULL) // error handling
    {
        perror("can't find current working directory in main.c");
        goto error;
    }
    else
    {
        size_t dir_len = strlen(current_working_directory) + 2;
        // allocate memory for the prompt, the +2 is for the '\n' and '\0'
        dir_string = malloc(dir_len);
        if (dir_string == NULL) // error handling
        {
            perror("error allocating space in main.c");
            goto error;
        }
        snprintf(dir_string, dir_len, "%s\n", current_working_directory);
        if (write(file_descriptor, dir_string, strlen(dir_string)) == 1) // error handling
        {
            perror("error writing prompt in main.c");
            goto error;
        }
    }
    // function executed succesfully, we free the dynamically allocated memory
    free(dir_string);
    free(current_working_directory);
    return 0;

    error:
        free(dir_string);
        free(current_working_directory);
        return 1;
}