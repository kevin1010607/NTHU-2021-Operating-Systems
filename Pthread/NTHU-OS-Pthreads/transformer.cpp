// CODEGEN BY auto_gen_transformer.py; DO NOT EDIT.

#include <assert.h>
#include "transformer.hpp"

unsigned long long Transformer::producer_transform(char opcode, unsigned long long val) {
	TransformSpec* spec = new TransformSpec;

	switch (opcode) {
	// same speed
	case 'A':
		spec->a = 11;
		spec->b = 1111;
		spec->m = 1000000007;
		spec->iterations = 10000000;
		break;

	// same speed
	case 'B':
		spec->a = 13;
		spec->b = 1313;
		spec->m = 1000000007;
		spec->iterations = 10000000;
		break;

	// same speed
	case 'C':
		spec->a = 17;
		spec->b = 1717;
		spec->m = 1000000007;
		spec->iterations = 10000000;
		break;

	default:
		assert(false);
	}

	return transform(spec, val);
}

unsigned long long Transformer::consumer_transform(char opcode, unsigned long long val) {
	TransformSpec* spec = new TransformSpec;

	switch (opcode) {
	// same speed
	case 'A':
		spec->a = 19;
		spec->b = 1919;
		spec->m = 1000000007;
		spec->iterations = 10000000;
		break;

	// same speed
	case 'B':
		spec->a = 23;
		spec->b = 2323;
		spec->m = 1000000007;
		spec->iterations = 10000000;
		break;

	// same speed
	case 'C':
		spec->a = 29;
		spec->b = 2929;
		spec->m = 1000000007;
		spec->iterations = 10000000;
		break;

	default:
		assert(false);
	}

	return transform(spec, val);
}

unsigned long long Transformer::transform(TransformSpec* spec, unsigned long long val) {
	while (spec->iterations--) {
		val = (val * spec->a + spec->b) % spec->m;
	}
  return val;
}
