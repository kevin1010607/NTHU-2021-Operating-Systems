./scripts/auto_gen_transformer --input ./tests/01_spec.json --output transformer.cpp
make clean
make docker-build
./main 4000 ./tests/01.in ./tests/01.out
./scripts/verify --output ./tests/01.out --answer ./tests/01.ans
