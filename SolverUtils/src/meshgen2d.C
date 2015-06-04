#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <cassert>

#include "Mesh.H"

int main(int argc,char *argv[])
{
  if(!argv[1]){
    std::cerr << argv[0] << ":Error: input file required." << std::endl;
    return(1);
  }

  std::ifstream Inf;
  Inf.open(argv[1]);
  if(!Inf){
    std::cerr << argv[0] << ":Error: Unable to open input file, " << argv[1] << "." 
              << std::endl;
    return(1);
  }

  std::ostringstream outStream;
  std::string line;
  std::vector<double> limits;
  std::vector<unsigned int> sizes;
  while(std::getline(Inf,line)){
    std::istringstream Istr(line);
    double limit1, limit2;
    int nx;
    Istr >> limit1 >> limit2 >> nx;
    limits.push_back(limit1);
    limits.push_back(limit2);
    sizes.push_back(nx);
  }
  Inf.close();
  assert(limits.size() == 6);
  assert(sizes.size() == 3);
 

  if((sizes[0] > 0) && (sizes[1] > 0) && (sizes[2] > 0)){
    std::cerr << "Invalid sizes specifications (only 2d supported): (" 
              << sizes[0] << "," << sizes[1] << "," 
              << sizes[2] << ")" << std::endl;
    return(1);
  }

  int nX = (sizes[0] > 0 ? sizes[0] : 1);
  int nY = (sizes[1] > 0 ? sizes[1] : 1);
  int nZ = (sizes[2] > 0 ? sizes[2] : 1);
  int numberOfNodes = nX * nY * nZ;

  std::vector<double> coordinates;
  int numberElementsX = (nX-1);
  int numberElementsY = (nY-1);
  int numberElementsZ = (nZ-1);
  
  outStream << numberOfNodes << std::endl;
  double xSpacing = 0;
  if(nX > 1) xSpacing = (limits[1] - limits[0])/(nX-1);
  double ySpacing = 0;
  if(nY > 1) ySpacing = (limits[3] - limits[2])/(nY-1);
  double zSpacing = 0;
  if(nZ > 1) zSpacing = (limits[5] - limits[4])/(nZ-1);
  for(int iZ = 0;iZ < nZ;iZ++){
    for(int iY = 0;iY < nY;iY++){
      for(int iX = 0; iX < nX;iX++){    
        double xCoord = limits[0] + iX*xSpacing;
        double yCoord = limits[2] + iY*ySpacing;
        double zCoord = limits[4] + iZ*zSpacing;
        coordinates.push_back(xCoord);
        coordinates.push_back(yCoord);
        coordinates.push_back(zCoord);
        outStream << xCoord << " " << yCoord << " " << zCoord << std::endl;
      }
    }
  } 
  
  int nDir1 = numberElementsX;
  int nDir2 = numberElementsY;
  if(nDir1 == 0){
    nDir1 = numberElementsY;
    nDir2 = numberElementsZ;
  } else if(nDir2 == 0){
    nDir2 = numberElementsZ;
  }

  int numberOfElements = nDir1*nDir2;
  std::vector<std::vector<unsigned int> > connectivityArray;

  int nElem = 0;
  int nCount = 0;
  while(nElem++ < numberOfElements){
    std::vector<unsigned int> element;
    element.push_back(nCount+1);
    element.push_back((nCount++)+(nDir1+1)+1);
    element.push_back(nCount+(nDir1+1)+1);
    element.push_back(nCount+1);
    if(!((nCount+1)%(nDir1+1))) nCount++;
    connectivityArray.push_back(element);
  }
  
  std::vector<std::vector<unsigned int> >::iterator conIt = connectivityArray.begin();
  outStream << connectivityArray.size() << std::endl;
  while(conIt != connectivityArray.end()){
    //    std::cout << "Element " << (conIt - connectivityArray.begin())+1 << ": (";
    std::vector<unsigned int>::iterator elemIt = conIt->begin();
    while(elemIt != conIt->end())
      outStream << *elemIt++ << " ";
    outStream << std::endl;
    conIt++;
  }

  //  std::cout << outStream.str() << std::endl;
  std::istringstream inStream(outStream.str());
  SolverUtils::Mesh::UnstructuredMesh unMesh;
  SolverUtils::Mesh::ReadMeshFromStream(unMesh,inStream);
  std::ostringstream vtkOut;
  SolverUtils::Mesh::WriteVTKToStream("testMesh",unMesh,vtkOut);
  std::cout << vtkOut.str();
  return(0);
}