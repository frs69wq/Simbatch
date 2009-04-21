#!/bin/bash
# calcule le makespan dans un dossier contenant les Batch*.out

cat Batch*.out | grep job | cut -f 5 | sort -n | tail -n 1
