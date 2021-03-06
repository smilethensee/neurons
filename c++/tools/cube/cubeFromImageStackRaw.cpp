
/////////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or       //
// modify it under the terms of the GNU General Public License         //
// version 2 as published by the Free Software Foundation.             //
//                                                                     //
// This program is distributed in the hope that it will be useful, but //
// WITHOUT ANY WARRANTY; without even the implied warranty of          //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   //
// General Public License for more details.                            //
//                                                                     //
// Written and (C) by German Gonzalez                                  //
// Contact <ggonzale@atenea> for comments & bug reports                //
/////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "Cube.h"

using namespace std;

int main(int argc, char **argv) {

  if(argc!=9){
    printf("Usage: cubeFromImageStackRaw directory image_format.jpg nlayer_b nlayer_e voxel_width voxel_height, voxel_depth volume_name\n");
    exit(0);
  }

  printf("Directory %s\nImage fromat %s\n",
         argv[1], argv[2]);

  string directory = argv[1];
  Cube<float, double>* source = new Cube<float, double>();
  source->create_cube_from_raw_files
    (argv[1], argv[2], atoi(argv[3]),
     atoi(argv[4]), atof(argv[5]), atof(argv[6]),
     atof(argv[7]), argv[8], 1);
}
