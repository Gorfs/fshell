# Le prompt

la fonction "getPrompt" s'occupe de générer le prompt pour le shell en incluant le répertoire courant, le statut de la dernière commande utilisée et s'il y a eu un signal on ajoute un indicateur.
On a utilisé les codes de couleur ANSI pour améliorer la lisibilité (Bleu = répertoire courant, vert = succès, rouge = échec)

# La tokenisation

La tokenisation est séparé en deux : "tokenise_cmds" divise une chaîne de caractères de commandes en tokens (tableau de tableaux de chaînes de caractères) et "tokenise_pipeline" qui gère la tokenisation des pipelines.

Pour identifier les délimiteurs et les tokens on utilise un algorithme de recherche à deux pointeurs. (un pointeur sur la position actuelle dans la chaîne de caractères et un pointeur sur la position du prochain délimiteur ou de la fin du token).
Cela nous permet aussi de détecter les if et for et les accolades imbriquées en gérant les cas spéciaux selon les données des pointeurs.

On ignore les espaces -> on identifie le prochain délimiteur -> on extraie le token et on gère les cas spéciaux -> on continue en faisant avancer les pointeurs

# Les commandes

Les commandes sont traitées comme des tokens (tableaux de tableaux de chaînes de caractères) et sont gérées par le shell si elles sont internes (voir la suite de l'architecture en dessous) sinon si elles sont externes on les exécute avec fork() et execvp().

Les redirections (>, <, >>, etc.) sont gérées en modifiant les descripteurs de fichiers (STDIN, STDOUT, STDERR) avant l'exécution des commandes mais les pipelines (|) sont implémentés en créant des tubes et en redirigeant les entrées et sorties entre les commandes (voir aussi la partie pipeline en dessous).
Dans ce cas on utilise en structure de données un tableau de descripteurs de fichiers et chaque commande a son propre ensemble de descripteurs.

"run_commands" parcourt toutes les commandes et appelle "run_command" pour gérer la commande en elle-même.
"init_file_descriptor" initialise les descripteurs de fichiers pour la commande en question et "setup_file_descriptors" les configure en fonction des redirections et des pipelines.

# Les pipelines

"run_pipeline" gère l'exécution de plusieurs commandes connectées par des pipelines (|).
Chaque commande est exécutée dans un fork, et les sorties/entrées sont redirigées via des tubes (rediriger entre les commandes).

"run_one_pipe" configure les descripteurs de fichiers (STDIN, STDOUT) pour chaque commande dans le pipeline.
Les descripteurs inutiles sont fermés pour éviter des fuites de ressources.

Les processus enfants exécutent les commandes via "run_commands".
Le processus parent attend la fin de tous les enfants et récupère leurs statuts.

Les structures de données utilisées sont un tableau de PID pour suivre les processus enfants et un tableau de descripteurs de fichiers pour gérer les redirections et les pipelines.

# Les if

La fonction "run_if" exécute la structure conditionnelle mais vérifie avant la syntaxe de la structure et envoie un message d'erreur si elle est mal formée.
On exécute une commande comme condition :
Si la condition est vraie (statut 0), on exécute le bloc if.
Si la condition est fausse et qu'un bloc else est présent, on exécute le bloc else.
On utilise des tableaux de chaînes de caractères différents (if_condition, if_then, if_else) pour stocker les différentes commandes liés à chaque bloc.

# Les for

La fonction "run_for" exécute la boucle mais vérifie avant la syntaxe de la boucle et envoie un message d'erreur si elle est mal formée.
On stocke plusieurs informations et notamment s'il y a des options pour que la boucle prenne en compte les différentes options (-A, -r, -e, -t, -p).
Si on a besoin de paralléliser alors on fait plusieurs fork() à la suite (selon le nombre donné avec -p) qui vont exécuter leur propre commande et ensuite on attend la fin des processus avec waitpid() avant de continuer jusqu'à ce qu'il n'y ait plus de fichiers.
La fonction "list_of_path_files" sert justement à obtenir tous les fichiers que l'on doit prendre en compte (et s'il y a -r on le fait en récursif) et la fonction "replace_var_name_to_file_name" remplacera le nom de la variable par l'emplacement du fichier à prendre en compte.
A chaque étape de la boucle on retokenise la commande que l'on doit exécuter (c'est un string et on a besoin de tableaux de tableaux de caractères pour lancer la commande).

# La commande cd

La fonction "command_cd" s'occupe de changer le répertoire et on stockage le dernier répertoire visité dans une chaîne de caractères dynamique "last_dir"

Si l'argument est -, le programme tente de revenir au dernier répertoire visité.
Si l'argument est ~ ou absent, le programme change vers le répertoire home de l'utilisateur.
Si l'argument est un chemin valide, le programme change de répertoire.
En cas d'erreur, des messages d'erreur sont affichés.

# La commande exit

La fonction "command_exit" s'occupe de gérer la logique pour sortir du shell tout en utilisant la fonction "free_tokens" qui libère la mémoire des tokens des commandes.

Si aucun argument n'est fourni, le programme quitte avec la valeur de retour de la dernière commande (last_val).
Si un argument est fourni, il est converti en entier et utilisé comme code de sortie.
Si trop d'arguments sont fournis, un message d'erreur est affiché.

# La commande pwd

La fonction "command_pwd" affiche le répertoire courant en utilisant une chaîne de caractères et gère les arguments invalides (affiche un message d'erreur si des arguments supplémentaires sont fournis).
On utilise getcwd() pour obtenir le répertoire courant et on formate avec snprintf() et affiche avec write().

# La commande ftype

La fonction "command_ftype" vérifie et affiche le type du fichier spécifié et gère les erreurs si le fichier n'existe pas.
On utilise une structure pour stocker les informations sur le fichier et on les obtient avec lstat() ce qui nous permet d'obtenir les types symboliques, répertoire, fichiers réguliers, tubes nommés et sinon les autres (en écrivant "other") et on affiche avec write.