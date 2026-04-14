#!/bin/bash

INPUT_DIR=$(pwd)/input/

cd build
mkdir -p data

USE_OPT=false

DESIGN="adder_2x2"
PORT=1234

while [[ $# -gt 0 ]]; do
  case $1 in
    -o|--opt)
      USE_OPT=true
      shift 1
      ;;
    -d|--design)
      DESIGN="$2"
      shift 2
      ;;
    -p|--port)
      PORT="$2"
      shift 2
      ;;
    -h|--help)
      echo "Usage: $0 [-o|--opt] [-d|--design Design_name] [-p|--port port_number]"
      exit 0
      ;;
    *)
      echo "Error: Unknown parameter $1"
      exit 1
      ;;
  esac
done

if [ "$USE_OPT" = true ]; then
    echo "Running zkcec_opt..."
    
    PROOF=${INPUT_DIR}${DESIGN}.prf.sorted
    INFO=${INPUT_DIR}${DESIGN}.info

    echo "Starting Prover..."
    ./zkcec_opt 1 $PORT $PROOF $INFO > /dev/null &

    echo "Starting Verifier..."
    ./zkcec_opt 2  $PORT $PROOF $INFO &
else
    echo "Running zkcec..."  

    PROOF=${INPUT_DIR}${DESIGN}.prf.sorted.unfold
    INFO=${INPUT_DIR}${DESIGN}.info

    echo "Starting Prover..."
    ./zkcec 1 $PORT $PROOF $INFO > /dev/null &

    echo "Starting Verifier..."
    ./zkcec 2  $PORT $PROOF $INFO &
fi

wait
