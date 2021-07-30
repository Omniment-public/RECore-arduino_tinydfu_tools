#!/bin/bash
set -o nounset # Treat unset variables as an error
STM32CP_CLI=STM32_Programmer_CLI
ADDRESS=0x8000000
ERASE=""
MODE=""
PORT=""
OPTS=""
DEVID=""

if [ $# -lt 2 ]; then
  echo "Not enough arguments!"
  $1
fi

CURRENT=$(cd $(dirname $0);pwd)

PORT=$2
FILEPATH=$3
DEVID=$4

${CURRENT}/tiny_dfu_writer ${PORT} ${FILEPATH} ${ADDRESS} ${DEVID}
exit $?
