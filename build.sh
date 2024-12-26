#!/bin/sh

set -xe

go build cmd/main.go

./main
