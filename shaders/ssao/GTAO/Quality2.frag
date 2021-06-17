#version 450

const float AO_RADIUS = 2.0;
const int NUM_STEPS = 8;
const int NUM_ROTATIONS = 4;
const int NUM_OFFSETS = 1;
const float LOD_BIAS = 0.5;

#include "../GTAO.frag.glsl"
