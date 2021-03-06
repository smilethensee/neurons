
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
// Contact < german.gonzalez@epfl.ch > for comments & bug reports      //
/////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include "Cube.h"
#include "CubeLiveWire.h"
#include "Graph.h"
#include "GraphFactory.h"
#include "CloudFactory.h"
#include <gsl/gsl_rng.h>
#include <map>
#include "EdgeW.h"
#include "Neuron.h"
#ifdef WITH_OPENMP
#include <omp.h>
#endif



using namespace std;


class DMST : public DistanceDijkstra {
public:
  Cube<float, double>* cubeFloat;
  double ratioY, ratioZ;

  DMST(Cube_P* cube){
    this->cubeFloat = dynamic_cast<Cube< float, double>* >(cube);
    ratioZ = cube->voxelDepth/cube->voxelWidth;
    ratioY = cube->voxelHeight/cube->voxelWidth;
  }
  float distance(int x0, int y0, int z0, int x1, int y1, int z1){
    double dist;
    //    dist = sqrt((double) (x0-x1)*(x0-x1) + ratioY*(y0-y1)*(y0-y1) + ratioZ*(z0-z1)*(z0-z1));
    //    return 1-cubeFloat->at(x1,y1,z1)/dist;
    //    return -log(cubeFloat->at(x1,y1,z1)/dist);

    dist = sqrt((double) (x0-x1)*(x0-x1) +
                ratioY*ratioY*(y0-y1)*(y0-y1) +
                ratioZ*ratioZ*(z0-z1)*(z0-z1));
    float p1 = cubeFloat->at(x0,y0,z0);
    float p2 = cubeFloat->at(x1,y1,z1);
    //Cost Pascal
    // if(fabs(p1-p2) < 1e-4) return -dist*log10(p1);
    // return fabs(dist*((log10(p1) * p1 - p1- log10(p2) * p2 + p2) / (-p2 + p1)));

    //Standard cost that works
    return -dist*log10((p1+p2)/2);
    // return (dist*(1-p1));
  }
};



int main(int argc, char **argv) {

  if (argc!= 5){
    printf("Usage: graphAddWeightsFromCubeDijkstra complete.gr cubeProbabilities.nfo completeWeighted.gr directoryWhereStorePaths\n");
    exit(0);
  }
  string origGraphName(argv[1]);
  string cubeProbsName(argv[2]);
  string destGraphName(argv[3]);
  string destGraphsPaths(argv[4]);

  //In out
  Graph<Point3D, EdgeW<Point3D> >* orig =
    new   Graph<Point3D, EdgeW<Point3D> >(origGraphName);
  Graph<Point3D, EdgeW<Point3D> >* dest =
    new   Graph<Point3D, EdgeW<Point3D> >(orig->cloud);
  Cube<float, double>* probs = new Cube<float, double>(cubeProbsName);


  //Auxiliary variables

  int nthreads = 1;
  int endPoint = orig->cloud->points.size();
  vector<int>   idxs(3);
  vector<int>   idxs2(3);
  vector<float> microm(3);
  vector<float> microm2(3);

#ifdef WITH_OPENMP
    nthreads = omp_get_max_threads();
    omp_set_num_threads(nthreads);
    printf("Performing detection with N = %i threads\n", nthreads);
#endif
  vector< CubeLiveWire* > cubeLiveWires(nthreads);
  vector< Cube<float, double>*> cubes(nthreads);
  vector< Graph<Point3D, EdgeW<Point3D> >* > cptGraphs(nthreads);
  cptGraphs.resize(nthreads);

  for(int j = 0; j < nthreads; j++){
    cptGraphs[j] =
      new Graph<Point3D, EdgeW< Point3D> >(orig->cloud);
    cubes[j] = new Cube<float, double>
      (cubeProbsName);
    DMST* djkc
      = new DMST(cubes[j]);
    cubeLiveWires[j] = new CubeLiveWire(cubes[j], djkc);
  }

  vector< vector< int > > neighbors = orig->findNeighbors();

  //Now we do the computations
#ifdef WITH_OPENMP
#pragma omp parallel for
#endif
   for(int nP = 0; nP < endPoint-1; nP ++){
   // for(int nP = 0; nP < 1; nP ++){
   // for(int nP = 6; nP < 7; nP ++){
    int nth = 0;
#ifdef WITH_OPENMP
    nth = omp_get_thread_num();
#endif
    vector<int>   idxsOrig(3);
    vector<int>   idxsDest(3);
    vector<float> micsOrig(3);
    vector<float> micsDest(3);
    char graphName[1024];
    micsOrig[0] = orig->cloud->points[nP]->coords[0];
    micsOrig[1] = orig->cloud->points[nP]->coords[1];
    micsOrig[2] = orig->cloud->points[nP]->coords[2];
    cubes[nth]->micrometersToIndexes(micsOrig, idxsOrig);

    //Finding the region of interest to compute dijkstra
    int xInit= max(idxsOrig[0]-1, 0);
    int xEnd = min(idxsOrig[0]+1, (int)cubes[nth]->cubeWidth -1) ;
    int yInit= max(idxsOrig[1]-1, 0);
    int yEnd = min(idxsOrig[1]+1, (int)cubes[nth]->cubeHeight -1);
    int zInit= max(idxsOrig[2]-1, 0);
    int zEnd = min(idxsOrig[2]+1, (int)cubes[nth]->cubeDepth-1);
    //We might be doing some extra computations in here
    for(int i = 0; i < neighbors[nP].size(); i++){
      micsDest[0] = orig->cloud->points[neighbors[nP][i]]->coords[0];
      micsDest[1] = orig->cloud->points[neighbors[nP][i]]->coords[1];
      micsDest[2] = orig->cloud->points[neighbors[nP][i]]->coords[2];
      cubes[nth]->micrometersToIndexes(micsDest, idxsDest);
      if(idxsDest[0] < xInit) xInit = idxsDest[0]-1;
      if(idxsDest[0] > xEnd)  xEnd  = idxsDest[0]+1;
      if(idxsDest[1] < yInit) yInit = idxsDest[1]-1;
      if(idxsDest[1] > yEnd)  yEnd  = idxsDest[1]+1;
      if(idxsDest[2] < zInit) zInit = idxsDest[2]-1;
      if(idxsDest[2] > zEnd)  zEnd  = idxsDest[2]+1;
    }
    cubeLiveWires[nth]->iROIx = max(0, xInit);
    cubeLiveWires[nth]->iROIy = max(0, yInit);
    cubeLiveWires[nth]->iROIz = max(0, zInit);
    cubeLiveWires[nth]->eROIx = min((int)cubes[nth]->cubeWidth -1, xEnd);
    cubeLiveWires[nth]->eROIy = min((int)cubes[nth]->cubeHeight-1, yEnd);
    cubeLiveWires[nth]->eROIz = min((int)cubes[nth]->cubeDepth -1, zEnd);

    printf("Analyzing point %04i, thread %02i\n"
           "  - pointPos-> [%i,%i,%i]\n"
           "  - ROI size -> [%i,%i,%i]\n"
           "  - ROI coordinates [%i,%i,%i]-[%i,%i,%i]\n"
           "  - xCoordinates = [%i,%i,%i]-[%i,%i,%i]\n",
           nP, nth,
           idxsOrig[0], idxsOrig[1], idxsOrig[2], 
           cubeLiveWires[nth]->eROIx - cubeLiveWires[nth]->iROIx,
           cubeLiveWires[nth]->eROIy - cubeLiveWires[nth]->iROIy,
           cubeLiveWires[nth]->eROIz - cubeLiveWires[nth]->iROIz,
           cubeLiveWires[nth]->iROIx,  cubeLiveWires[nth]->iROIy,
           cubeLiveWires[nth]->iROIz,  cubeLiveWires[nth]->eROIx,
           cubeLiveWires[nth]->eROIy,  cubeLiveWires[nth]->eROIz,
           xInit, yInit, zInit, xEnd, yEnd, zEnd);

    // continue;

    cubeLiveWires[nth]->computeDistances(idxsOrig[0], idxsOrig[1], idxsOrig[2]);

    printf("and cubelivewire should be done\n");

    for(int i = 0; i < neighbors[nP].size(); i++){
      if(neighbors[nP][i] <= nP) continue; //to prevent double edges
      micsDest[0] = orig->cloud->points[neighbors[nP][i]]->coords[0];
      micsDest[1] = orig->cloud->points[neighbors[nP][i]]->coords[1];
      micsDest[2] = orig->cloud->points[neighbors[nP][i]]->coords[2];
      cubes[nth]->micrometersToIndexes(micsDest, idxsDest);
      printf("Csp beetween [%i,%i,%i] and [%i,%i,%i]"
             "where the limits are [%i,%i,%i]->[%i,%i,%i]\n",
             idxsOrig[0] ,idxsOrig[1] ,idxsOrig[2],
             idxsDest[0], idxsDest[1], idxsDest[2],
             cubeLiveWires[nth]->iROIx,
             cubeLiveWires[nth]->iROIy,
             cubeLiveWires[nth]->iROIz,
             cubeLiveWires[nth]->eROIx,
             cubeLiveWires[nth]->eROIy,
             cubeLiveWires[nth]->eROIz);
      Graph<Point3D, EdgeW<Point3D> >* shortestPath =
        cubeLiveWires[nth]->findShortestPathG(idxsOrig[0] ,idxsOrig[1] ,idxsOrig[2],
                                              idxsDest[0], idxsDest[1], idxsDest[2]);
      double cost = 0;
      for(int nedge=0; nedge < shortestPath->eset.edges.size(); nedge++){
        EdgeW<Point3D>* edge = dynamic_cast<EdgeW<Point3D>*>
          (shortestPath->eset.edges[nedge]);
        cost += edge->w;
      }

      shortestPath->cloud->v_r = cost;
      shortestPath->cloud->v_g = 1-cost;
      shortestPath->cloud->v_b = 0;
      // shortestPath->cloud->v_b = gsl_rng_uniform(r);
      shortestPath->cloud->v_radius = 0.4;
      sprintf(graphName,
              "%s/path_%04i_%04i.gr", destGraphsPaths.c_str(), nP, neighbors[nP][i]);
      // printf("saving path in %s\n", graphName);
      shortestPath->saveToFile(graphName);
      double length = sqrt((microm2[0]-microm[0])*(microm2[0]-microm[0]) +
                           (microm2[1]-microm[1])*(microm2[1]-microm[1]) +
                           (microm2[2]-microm[2])*(microm2[2]-microm[2]) );
      cptGraphs[nth]->eset.edges.push_back
        (new EdgeW< Point3D>
         (&cptGraphs[nth]->cloud->points, nP,neighbors[nP][i],
          cost ) );
      delete shortestPath;
    }//end of finding the path
    printf("Point completed  %04i, thread %04i.\n", nP, nth);
  }//end of the loops
  //  exit(0);

  //Now we merge the complete Graph
  printf("Building the complete graph\n"); fflush(stdout);
  for(int j = 0; j < nthreads; j++){
    for(int i = 0; i < cptGraphs[j]->eset.edges.size(); i++){
      EdgeW<Point3D>* edgeToAdd =
        dynamic_cast<EdgeW<Point3D>*>(cptGraphs[j]->eset.edges[i]);
      dest->eset.edges.push_back
        (new EdgeW< Point3D > ( &dest->cloud->points,
                                edgeToAdd->p0,
                                edgeToAdd->p1,
                                edgeToAdd->w )
         );
    }
  }


  dest->saveToFile(destGraphName);
}
