INPUT_DIR=../input/

cd build
mkdir -p data

DESIGN=adder_2x2 # the name of the circuit
PORT=1234

FILE1=${INPUT_DIR}${DESIGN}.prf.sorted.unfold
FILE2=${INPUT_DIR}${DESIGN}.info

echo "Starting Prover..."
./test 1 $PORT $FILE1 $FILE2 > /dev/null &

echo "Starting Verifier..."
./test 2  $PORT $FILE1 $FILE2 &

wait
