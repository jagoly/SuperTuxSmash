#version 450

const float AO_RADIUS = 1.0;
const int NUM_STEPS = 4;
const int NUM_ROTATIONS = 2;
const int NUM_OFFSETS = 1;
const float LOD_BIAS = 0.6;

// makes very little performance difference, rather we actually want
// the overdarkening to help compensate for a tiny AO_RADIUS
#define FALLOFF_MODE_SIMPLE

#include "../GTAO.frag.glsl"
