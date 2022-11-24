./scripts/auto_gen_transformer --input ./tests/00_spec.json --output transformer.cpp
make clean
make docker-build
./main 200 ./tests/00.in ./tests/00.out
./scripts/verify --output ./tests/00.out --answer ./tests/00.ans
