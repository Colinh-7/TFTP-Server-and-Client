#!/bin/bash

# Controle du nombre d'argument
#	- le nombre d'execution
#	- le numéro de port
if [ $# -ne 2 ]; then
    echo "Usage: $0 <nombre de fois> <numéro de port>"
    exit 1
fi

nbExec=$1
port=$2

# Execution la commande n fois
for ((i=1; i<=$nbExec; i++)); do
    ./bin/tftp --mode MULT --port $port &
done

