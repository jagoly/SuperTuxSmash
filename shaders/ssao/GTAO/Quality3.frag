#version 450

const float AO_RADIUS = 3.0;
const int NUM_STEPS = 12;
const int NUM_ROTATIONS = 8;
const int NUM_OFFSETS = 1;
const float LOD_BIAS = 0.4;

#include "../GTAO.frag.glsl"
