#!/bin/bash
scripts/raise.sh $@ || exit 1
scripts/recompile.sh $1
