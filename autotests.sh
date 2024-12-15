#!/bin/bash

TESTS_DIR=./test-files
REFERENCE_1=/ens/geoffroy/Public/sy5/2024-2025/fsh

REFERENCE_2=reference
REFERENCE_2_URL="https://cloud.irif.fr/s/MNLKJpPG8GHyCny/download?path=%2F&files=reference"
REFERENCE_2_HASH_URL="https://cloud.irif.fr/s/MNLKJpPG8GHyCny/download?path=%2F&files=reference.md5"

TESTED=fsh

if [ -x "$REFERENCE_1" ]; then
  REFERENCE="$REFERENCE_1"
else
  if wget -q -O- "$REFERENCE_2_HASH_URL" | md5sum -c >/dev/null 2>/dev/null; then
    REFERENCE="$REFERENCE_2"
  else
    if wget -q -O "$REFERENCE_2" "$REFERENCE_2_URL" ; then
      chmod u+x "$REFERENCE_2"
      if "./$REFERENCE_2" --dry < /dev/null >/dev/null 2>/dev/null ; then
        REFERENCE="$REFERENCE_2"
      else
        printf "Erreur: le script doit être lancé depuis une machine de l'UFR (par exemple lulu) ou une machine linux avec une configuration similaire. Abandon." >&2
        exit 1
      fi
    else
      printf "Erreur: le script de test n'a pas été lancé depuis une machine de l'UFR (comme lulu), et je n'arrive pas à télécharger l'implémentation de référence ($REFERENCE_2_URL). Abandon." >&2 
      exit 1
    fi
  fi
fi

export AUTOTEST_DIR_ABS="$(pwd)"

if ! git pull 2>/dev/null >/dev/null; then
  printf "Attention: je n'ai pas réussi à mettre à jour le dépôt git contenant les tests." >&2
  if [ -n "$(git status --porcelain --untracked-files=no 2> /dev/null)" ]; then
    printf " Si vous avez modifié le contenu du dossier de tests, je ne pourrai pas mettre à \
jour celui-ci tant que vous n'aurez pas annulé vos modifications (avec git restore)\n" >&2
  else
    printf "\n"
  fi
fi

if ! make -s all; then
  printf "Erreur: je n'ai pas réussi à construire les programmes de test. Abandon.\n" >&2
  exit 1
fi

cd ..
if ! make -s >/dev/null 2>&1 || [ ! -x "$TESTED" ]; then
  printf "Erreur: la commande make a échoué ou n'a pas produit l'exécutable jsh. Abandon.\n"
  exit 1
fi
cd - >/dev/null

BASE_0="$(mktemp -d)"
if [ ! -d "$BASE_0" ]; then exit 1; fi
BASE="${BASE_0}/$(cat /dev/urandom | LANG=C tr -dc 'a-zA-Z0-9' | head -c 32)"
mkdir -p "$BASE"
if [ ! -d "$BASE" ]; then exit 1; fi

mkdir -p "$BASE_0/tmp"
export TEST_REL_DIR="$BASE_0/tmp"
export TEST_ABS_DIR=$(cd "$BASE_0/tmp" ; pwd -P)
if [ ! -d "$TEST_ABS_DIR" ]; then exit 1; fi

cp ./bin/test "$BASE/test"
cd ..
cp "$TESTED" "$BASE/$TESTED"
cd - > /dev/null
cp "$REFERENCE" "$BASE/reference"
cp -r "$TESTS_DIR" "$BASE/test-files"
mkdir "$BASE/main-dir"

if ! cd "$BASE/main-dir"; then exit 1; fi

export LC_ALL=C

../test ../reference "../$TESTED" ../test-files
RET="$?"

cd - >/dev/null
exit "$RET"
