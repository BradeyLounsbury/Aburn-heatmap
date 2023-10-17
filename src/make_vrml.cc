#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
    int num_squares = 10;

    fstream ofs;
    ofs.open("../include/models/grid2.wrl");

    if (ofs.fail()) {
        cout << "failed to open file\n";
        return -1;
    }

	ofs << "#VRML V1.0 ascii\n\n";

	ofs << "Sperator {\n";
    ofs << "    DEF mat_grid_01\n";
    ofs << "    Material {\n";
    ofs << "		diffuseColor 0.800000 0.800000 0.800000\n";
    ofs << "		specularColor 0.000000 0.000000 0.000000\n";
    ofs << "		shininess 0.010000\n";
    ofs << "		transparency 0.000000\n";
    ofs << "    }\n";

	ofs << "	DEF grid_01\n";
    ofs << "    Separator {\n";
    ofs << "        Texture2 {\n";
    ofs << "            filename ../include/models/grid.png\n";
    ofs << "            wrapS REPEAT\n";
    ofs << "            wrapT REPEAT\n";
    ofs << "        }\n";
	
	ofs << "	Coordinate3 {\n";
	ofs << "		point [\n";
	for (int i = -num_squares; i < num_squares; i++) {
        for (int j = -num_squares; j < num_squares; j++) {
            ofs << "			" << float(i+1) << " " << float(j) << " " << 0.0f << ",\n";
            ofs << "			" << float(i) << " " << float(j+1) << " " << 0.0f << ",\n";
            ofs << "			" << float(i) << " " << float(j) << " " << 0.0f << ",\n";
            ofs << "			" << float(i+1) << " " << float(j) << " " << 0.0f << ",\n";
            ofs << "			" << float(i+1) << " " << float(j+1) << " " << 0.0f << ",\n";
            ofs << "			" << float(i) << " " << float(j+1) << " " << 0.0f << ",\n";
        }
	}
	ofs << "		]\n";
	ofs << "	}\n";
    ofs << "    USE mat_grid_01\n";

    ofs << "    TextureCoordinate2 {\n";
    ofs << "        point [\n";
    float pt = 0.0f;
    double inc = double(1) / (double(num_squares * num_squares * 6 * 4));
    for (int i = 0; i < num_squares * 2; i++) {
        for (int j = 0; j < num_squares * 2; j++) {
            ofs << "			" << pt << " " << pt << ",\n";
            pt+=inc;
            ofs << "			" << pt << " " << pt << ",\n";
            pt+=inc;
            ofs << "			" << pt << " " << pt << ",\n";
            pt+=inc;
            ofs << "			" << pt << " " << pt << ",\n";
            pt+=inc;
            ofs << "			" << pt << " " << pt << ",\n";
            pt+=inc;
            ofs << "			" << pt << " " << pt << ",\n";
            pt+=inc;
        }
    }

	ofs << "	IndexedFaceSet {\n";
	ofs << "		coordIndex [\n";
	for (int i = 0; i < num_squares; i++) {
        for (int j = 0; j < num_squares; j++) {
            ofs << "			" << float(i+1) << " " << float(j) << " " << -1 << ",\n";
            ofs << "			" << float(i) << " " << float(j+1) << " " << -1 << ",\n";
            ofs << "			" << float(i) << " " << float(j) << " " << -1 << ",\n";
            ofs << "			" << float(i+1) << " " << float(j) << " " << -1 << ",\n";
            ofs << "			" << float(i+1) << " " << float(j+1) << " " << -1 << ",\n";
            ofs << "			" << float(i) << " " << float(j+1) << " " << -1 << ",\n";
        }
	}
	ofs << "		]\n";
	ofs << "	}\n";

    ofs << "		textureCoordIndex [\n";
	for (int i = 0; i < num_squares; i++) {
        for (int j = 0; j < num_squares; j++) {
            ofs << "			" << float(i+1) << " " << float(j) << " " << -1 << ",\n";
            ofs << "			" << float(i) << " " << float(j+1) << " " << -1 << ",\n";
            ofs << "			" << float(i) << " " << float(j) << " " << -1 << ",\n";
            ofs << "			" << float(i+1) << " " << float(j) << " " << -1 << ",\n";
            ofs << "			" << float(i+1) << " " << float(j+1) << " " << -1 << ",\n";
            ofs << "			" << float(i) << " " << float(j+1) << " " << -1 << ",\n";
        }
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

    return 0;
}

