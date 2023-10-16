#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <Vector.h>

namespace WRLParser {
	void send_to_file(std::vector<Aftr::Vector>& verts, std::vector<unsigned int>& indices, std::string path);
}