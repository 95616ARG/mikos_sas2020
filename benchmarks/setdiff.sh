#!/bin/bash

# Returns $2 - $1
grep -vf $1 $2 | sort | uniq > $3 
