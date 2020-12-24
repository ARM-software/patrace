#pragma once

#include "glsl_parser.h"

int count_varying_locations_used(GLSLRepresentation r);
int count_varyings_by_precision(GLSLRepresentation r, Keyword p);
std::string convert_to_tf(const std::string& s);
