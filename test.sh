#!/bin/bash

if [ ! -f AUTHORS.md -o ! -s AUTHORS.md ]; then
  printf "Erreur: ce script doit être lancé depuis la racine du dépôt git de votre projet. \
Je n'ai pas détecté de fichier AUTHORS.md non vide dans le répertoire courant. Abandon.\n" >&2
  exit 1
fi

AUTOTEST_GIT="https://moule.informatique.univ-paris-diderot.fr/geoffroy/sy5-2024-2025-projet-fsh-autotests.git"
AUTOTEST_DIR=".sy5-2024-2025-projet-fsh-autotests.nosync"

function file_ends_on_line_start() {
    [ ! -s "$1" -o $(tail -c 1 "$1" | wc -l) -gt 0 ]
}

if [ -d ".git" ]; then
  if [ ! -e ".gitignore" ]; then
    touch ".gitignore"
  fi
  if ! grep "^$AUTOTEST_DIR\$" ".gitignore" >/dev/null 2>/dev/null; then
    if file_ends_on_line_start ".gitignore"; then
      printf "%s\n" "$AUTOTEST_DIR" >> .gitignore
    else
      printf "\n%s\n" "$AUTOTEST_DIR" >> .gitignore
    fi
    git add ".gitignore" >/dev/null 2>/dev/null
  fi
else
  printf "Attention: le répertoire courant n'est pas un dépôt git.\n" >&2
fi

if [ ! -d "$AUTOTEST_DIR" ]; then
   if ! git clone "$AUTOTEST_GIT" "$AUTOTEST_DIR"; then
      printf "Erreur: je n'ai pas réussi à cloner le dépôt $AUTOTEST_GIT. Abandon.\n" >&2
      exit 1
   fi
fi

if ! cd "$AUTOTEST_DIR"; then
  exit 1
fi

exec bash ./autotests.sh
