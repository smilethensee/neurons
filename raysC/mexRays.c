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
// Written and (C) by Aurelien Lucchi and Kevin Smith                  //
// Contact aurelien.lucchi (at) gmail.com or kevin.smith (at) epfl.ch  // 
// for comments & bug reports                                          //
/////////////////////////////////////////////////////////////////////////

#include "mex.h"
#include <stdio.h>
#include "rays.h"
#include "cv.h"

/* function [RAY1 RAY3 RAY4] = rays(E, G, angle, stride)
 * RAYS computes RAY features
 *   Example:
 *   -------------------------
 *   I = imread('cameraman.tif');
 *   SPEDGE = spedge_dist(I,30,2, 11);
 *   imagesc(SPEDGE);  axis image;
 *
 *
 *   FEATURE = spedge_dist(E, G, ANGLE, STRIDE)  computes a spedge 
 *   feature on a grayscale image I at angle ANGLE.  Each pixel in FEATURE 
 *   contains the distance to the nearest edge in direction ANGLE.  Edges 
 *   are computed using Laplacian of Gaussian zero-crossings (!!! in the 
 *   future we may add more methods for generating edges).  SIGMA specifies 
 *   the standard deviation of the edge filter.  
 */
void mexFunction(int nlhs,       mxArray *plhs[],
                      int nrhs, const mxArray *prhs[])
{
    mwSize nElements,j;
    mwSize number_of_dims;
    double sigma;
    double angle;
    int strLength;
    mxArray    *tmp;
    char *pImageName;
    
    /* Check for proper number of input and output arguments */    
    if (nrhs != 3) {
      mexErrMsgTxt("Three input arguments required.");
    } 
    if (nlhs > 1){
	mexErrMsgTxt("Too many output arguments.");
    }
    
    /* Check data type of input argument */
    if (!(mxIsCell(prhs[0]))) {
      mexErrMsgTxt("Input array must be of type cell.");
    }
    if (!(mxIsDouble(prhs[1]))) {
      mexErrMsgTxt("Input array must be of type double.");
    }
    if (!(mxIsDouble(prhs[2]))) {
      mexErrMsgTxt("Input array must be of type double.");
    }
    
    mexPrintf("Loading input parameters\n");
  
    /* Get the real data */
    //pImage = (unsigned char *)mxGetPr(prhs[0]);
    //pGradient = (double *)mxGetPr(prhs[1]);
    //pImageName = (const char *)mxGetPr(prhs[0]);

    tmp = mxGetCell(prhs[0],0);
    strLength = mxGetN(tmp)+1;
    pImageName = (char*)mxCalloc(strLength, sizeof(char));
    mxGetString(tmp,pImageName,strLength);

    sigma=*((double *)mxGetPr(prhs[1]));
    angle=*((double *)mxGetPr(prhs[2]));

    /*
    for(j=0;j<10;j++){
      printf("%d\n",pImage[j]);
      //pResult[j] = pIndices[j];
    }
    */
    
    /* Invert dimensions :
       Matlab : height, width
       OpenCV : width, hieght
    */
    /* Create output matrix */
    /*
    number_of_dims=mxGetNumberOfDimensions(prhs[0]);
    dim_array=mxGetDimensions(prhs[0]);
    const mwSize dims[]={dim_array[0],dim_array[1]};
    plhs[0] = mxCreateNumericArray(2,dims,mxUINT32_CLASS,mxREAL);
    pResult = (int*)mxGetData(plhs[0]);
    copyIntegralImage(pImage,dim_array[1],dim_array[0],pResult);
    */

    mexPrintf("computeRays\n");
    IplImage* ray1;
    computeRays((const char*)pImageName, sigma, angle, ray1);

    mexPrintf("Cleaning\n");
    mxFree(pImageName);
}

/*

    // compute the gradient norm GN
    MatrixN G(pGradient);
    double GN = sqrt(sum((G.^2),3));

% convert the Gradient G into unit vectors
G = gradientnorm(G);



% get a scanline in direction angle
warning off MATLAB:nearlySingularMatrix; warning off MATLAB:singularMatrix;
[Sr, Sc] = linepoints(E,angle);
warning on MATLAB:nearlySingularMatrix; warning on MATLAB:singularMatrix;

% initialize the output matrices
RAY1 = zeros(size(E));  % distrance rays
RAY3 = zeros(size(E));  % gradient orientation
RAY4 = zeros(size(E));  % gradient norm

% determine the unit vector in the direction of the Ray
rayVector = unitvector(angle);

% if S touches the top & bottom of the image
if ((angle >= 45) && (angle <= 135))  || ((angle >= 225) && (angle <= 315))
    % SCAN TO THE LEFT!
    j = 0;
    c = Sc + j;
    inimage = find(c > 0);
    while ~isempty(inimage);
        r = Sr(inimage);
        c = Sc(inimage) + j;
        steps_since_edge = 0;  % the border of the image serves as an edge
        lastGN = 0;
        lastGA = 1;
        for i = 1:length(r);
            if E(r(i),c(i)) == 1
                steps_since_edge = 0;
                lastGA = rayVector * [G(r(i),c(i),1); G(r(i),c(i),2)]; 
                %if isnan(lastGA); keyboard; end;
                lastGN = GN(r(i),c(i));
            end
            RAY1(r(i),c(i)) = steps_since_edge;
            RAY3(r(i),c(i)) = lastGA;
            RAY4(r(i),c(i)) = lastGN;
            steps_since_edge = steps_since_edge +1;
        end
        j = j-1;
        c = Sc + j;
        inimage = find(c > 0);
    end


    % SCAN TO THE RIGHT!
    j = 1;
    c = Sc + j;
    inimage = find(c <= size(E,2));
    while ~isempty(inimage);
        r = Sr(inimage);
        c = Sc(inimage) + j;
        steps_since_edge = 0;  % the border of the image serves as an edge
        lastGN = 0;
        lastGA = 1;
        for i = 1:length(r);
            if E(r(i),c(i)) == 1
                steps_since_edge = 0;
                lastGA = rayVector * [G(r(i),c(i),1); G(r(i),c(i),2)]; 
                %if isnan(lastGA); keyboard; end;
                lastGN = GN(r(i),c(i));
            end
            RAY1(r(i),c(i)) = steps_since_edge;
            RAY3(r(i),c(i)) = lastGA;
            RAY4(r(i),c(i)) = lastGN;
            steps_since_edge = steps_since_edge +1;
        end
        j = j+1;
        c = Sc + j;
        inimage = find(c <= size(E,2));
    end
   
% if S touches left & right of image (-pi/4 > angle > pi/4) or (3pi/4 > angle > 5pi/4)
else
    % SCAN TO THE bottom!
    j = 0;
    r = Sr + j;
    inimage = find(r > 0);
    while ~isempty(inimage);
        r = Sr(inimage) + j;
        c = Sc(inimage);
        steps_since_edge = 0;  % the border of the image serves as an edge
        lastGN = 0;
        lastGA = 1;
        for i = 1:length(r);
            if E(r(i),c(i)) == 1
                steps_since_edge = 0;
                lastGA = rayVector * [G(r(i),c(i),1); G(r(i),c(i),2)]; 
                %if isnan(lastGA); keyboard; end;
                lastGN = GN(r(i),c(i));
            end
            RAY1(r(i),c(i)) = steps_since_edge;
            RAY3(r(i),c(i)) = lastGA;
            RAY4(r(i),c(i)) = lastGN;
            steps_since_edge = steps_since_edge +1;
        end
        j = j-1;
        r = Sr + j;
        inimage = find(r > 0);
    end


    % SCAN TO THE top!
    j = 1;
    r = Sr + j;
    inimage = find(r <= size(E,1));
    while ~isempty(inimage);
        r = Sr(inimage) + j;
        c = Sc(inimage);
        steps_since_edge = 0;  % the border of the image serves as an edge
        lastGN = 0;
        lastGA = 1;
        for i = 1:length(r);
            if E(r(i),c(i)) == 1
                steps_since_edge = 0;
                lastGA = rayVector * [G(r(i),c(i),1); G(r(i),c(i),2)]; 
                %if isnan(lastGA); keyboard; end;
                lastGN = GN(r(i),c(i));
            end
            RAY1(r(i),c(i)) = steps_since_edge;
            RAY3(r(i),c(i)) = lastGA;
            RAY4(r(i),c(i)) = lastGN;
            steps_since_edge = steps_since_edge +1;
        end
        j = j+1;
        r = Sr + j;
        inimage = find(r <= size(E,1));
    end
end

if stride ~= 1
    RAY1 = RAY1(1:stride:size(RAY1,1), 1:stride:size(RAY1,2));
    RAY3 = RAY3(1:stride:size(RAY3,1), 1:stride:size(RAY3,2));
    RAY4 = RAY4(1:stride:size(RAY4,1), 1:stride:size(RAY4,2));
end





function [Sr, Sc] = linepoints(E,Angle)
% defines the points in a line in an image at an arbitrary angle
%
%
%
%


% flip the sign of the angle (matlab y axis points down for images) and
% convert to radians
if Angle ~= 0
    %angle = deg2rad(Angle);
    angle = deg2rad(360 - Angle);
else
    angle = Angle;
end

% format the angle so it is between 0 and less than pi/2
if angle > pi; angle = angle - pi; end
if angle == pi; angle = 0; end


% find where the line intercepts the edge of the image.  draw a line to
% this point from (1,1) if 0<=angle<=pi/2.  otherwise pi/2>angle>pi draw 
% from the upper left corner down.  linex and liney contain the points of 
% the line
if (angle >= 0 ) && (angle <= pi/2)
    START = [1 1]; 
    A_bottom_intercept = [-tan(angle) 1; 0 1];  B_bottom_intercept = [0; size(E,1)-1];
    A_right_intercept  = [-tan(angle) 1; 1 0];  B_right_intercept  = [0; size(E,2)-1];
    bottom_intercept = round(A_bottom_intercept\B_bottom_intercept);
    right_intercept  = round(A_right_intercept\B_right_intercept);

    if right_intercept(2) <= size(E,1)-1
        END = right_intercept + [1; 1];
    else
        END = bottom_intercept + [1; 1];
    end
    [linex,liney] = intline(START(1), END(1), START(2), END(2));
else
    START = [1, size(E,1)];
    A_top_intercept = [tan(pi - angle) 1; 0 1];  B_top_intercept = [size(E,1); 1];
    A_right_intercept  = [tan(pi - angle) 1; 1 0];  B_right_intercept  = [size(E,1); size(E,2)-1];
    top_intercept = round(A_top_intercept\B_top_intercept);
    right_intercept  = round(A_right_intercept\B_right_intercept);

    if (right_intercept(2) < size(E,1)-1) && (right_intercept(2) >= 1)
        END = right_intercept + [1; 0];
    else
        END = top_intercept + [1; 0];
    end
    [linex,liney] = intline(START(1), END(1), START(2), END(2));
end

Sr = round(liney); Sc = round(linex);

% if the angle points to quadrant 2 or 3, we need to re-sort the elements 
% of Sr and Sc so they increase in the direction of the angle

if (270 <= Angle) || (Angle < 90)
    reverse_inds = length(Sr):-1:1;
    Sr = Sr(reverse_inds);
    Sc = Sc(reverse_inds);
end




function [x,y] = intline(x1, x2, y1, y2)
% intline creates a line between two points
%INTLINE Integer-coordinate line drawing algorithm.
%   [X, Y] = INTLINE(X1, X2, Y1, Y2) computes an
%   approximation to the line segment joining (X1, Y1) and
%   (X2, Y2) with integer coordinates.  X1, X2, Y1, and Y2
%   should be integers.  INTLINE is reversible; that is,
%   INTLINE(X1, X2, Y1, Y2) produces the same results as
%   FLIPUD(INTLINE(X2, X1, Y2, Y1)).

dx = abs(x2 - x1);
dy = abs(y2 - y1);

% Check for degenerate case.
if ((dx == 0) && (dy == 0))
  x = x1;
  y = y1;
  return;
end

flip = 0;
if (dx >= dy)
  if (x1 > x2)
    % Always "draw" from left to right.
    t = x1; x1 = x2; x2 = t;
    t = y1; y1 = y2; y2 = t;
    flip = 1;
  end
  m = (y2 - y1)/(x2 - x1);
  x = (x1:x2).';
  y = round(y1 + m*(x - x1));
else
  if (y1 > y2)
    % Always "draw" from bottom to top.
    t = x1; x1 = x2; x2 = t;
    t = y1; y1 = y2; y2 = t;
    flip = 1;
  end
  m = (x2 - x1)/(y2 - y1);
  y = (y1:y2).';
  x = round(x1 + m*(y - y1));
end
  
if (flip)
  x = flipud(x);
  y = flipud(y);
end


function v = gradientnorm(v)
v = double(v); eta = .00000001;
mag = sqrt(  sum( (v.^2),3)) + eta;
v = v ./ repmat(mag, [1 1 2]);

*/