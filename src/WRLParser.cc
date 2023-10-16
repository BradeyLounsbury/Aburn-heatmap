#include "WRLParser.h"
#include <fstream>
#include <string>
#include <vector>
#include <Vector.h>

namespace WRLParser {
void send_to_file(std::vector<Aftr::Vector>& verts, std::vector<unsigned int>& indices, std::string path) {
	std::fstream ofs(path);

	ofs << "#VRML V1.0 ascii\n\n";

	ofs << "Sperator {\n";
	ofs << "	DEF Model_01\n";

	ofs << "	Material {\n";
	ofs << "		diffuseColor [\n";
	ofs << "			1.000000 1.000000 1.000000,\n";
	ofs << "		]\n";
	ofs << "	}\n";
	
	ofs << "	Coordinate3 {\n";
	ofs << "		point [\n";
	for (int i = 0; i < verts.size(); i++) {
		double x, y, z;
		x = verts[i].x;
		y = verts[i].y;
		z = verts[i].z;
		ofs << "			" << x << ", " << y << ", " << z << ",\n";
	}
	ofs << "		]\n";
	ofs << "	}\n";

	ofs << "	IndexedFaceSet {\n";
	ofs << "		coordIndex [\n";
	for (int i = 0; i < indices.size(); i += 3) {
		double a, b, c;
		a = indices[i];
		b = indices[i + 1];
		c = indices[i + 2];
		ofs << "			" << a << ", " << b << ", " << c << ",\n";
	}
	ofs << "		]\n";
	ofs << "	}\n";

	ofs << "}\n";

	ofs << "Sperator {\n";
	ofs << "	Sperator {\n";
	ofs << "		MatrixTransform {\n";
	ofs << "		matrix\n";
	ofs << "			1.000000 0.000000 0.000000 0.000000\n";
	ofs << "			0.000000 1.000000 0.000000 0.000000\n";
	ofs << "			0.000000 0.000000 1.000000 0.000000\n";
	ofs << "			0.000000 0.000000 0.000000 1.000000\n";
	ofs << "		}\n";
	ofs << "		USE Model_01\n";
	ofs << "	}\n";
	ofs << "}\n";

	ofs.close();
}
}