//
//  Copyright@2013, Illinois Rocstar LLC. All rights reserved.
//        
//  See LICENSE file included with this source or
//  (opensource.org/licenses/NCSA) for license information. 
//

#include "com.h"
#include "surfbasic.h"
#include "IM_Reader.h"
#include <iostream>

using namespace std;

COM_EXTERN_MODULE( SimOUT);
COM_EXTERN_MODULE( SurfUtil);
COM_EXTERN_MODULE( Simpal);

int main(int argc, char *argv[]) {
  COM_init( &argc, &argv);

  if ( argc != 3) {
    std::cout << "Usage:\n\tTo run in serial: " << argv[0] 
	      << " <surffile> <hdffile> " << endl;
    std::cout << "\n\tTo run in parallel: <mpirun-command> " << argv[0] 
	      << " -com-mpi <Rocin control file> <hdfoutput-prefix> " << endl;
    exit(-1);
  }

  std::cout << "Reading surface mesh file \"" << argv[1] << '"' << endl;

  std::string fname(argv[1]), wname;
  string::size_type n0 = fname.find_last_of( "/");

  if ( n0 != std::string::npos) 
    fname = fname.substr( n0+1, fname.size());

  string::size_type ni;
  ni = fname.find_first_of( ".:-*[]?\\\"\'0123456789");
  COM_assertion_msg(ni, "File name must start with a letter");

  if ( ni == std::string::npos) {
    wname = fname;
  }
  else {
    while (fname[ni-1]=='_') --ni; // Remove the '_' at the end.
    wname = fname.substr( 0, ni);
  }

  std::cout << "Creating window \"" << wname << '"' << endl;

  // Read in IM/HDF format
  int err = IM_Reader().read_winmesh( argv[1], wname); 
  COM_assertion( err>=0);

  COM_window_init_done( wname.c_str());


  std::string newwin("newwin");
  COM_new_window( newwin.c_str());
  COM_window_init_done( newwin.c_str());

  int pmesh_hdl = COM_get_dataitem_handle( (wname+".pmesh").c_str());

  COM_LOAD_MODULE_STATIC_DYNAMIC(SimOUT, "OUT");
  COM_LOAD_MODULE_STATIC_DYNAMIC(SurfUtil, "SURF");

  int SURF_init = COM_get_function_handle( "SURF.initialize");
  COM_call_function( SURF_init, &pmesh_hdl);

  int newmesh_hdl = COM_get_dataitem_handle( (newwin+".mesh").c_str());

  cout << "Serialize mesh..." << endl;
  int SURF_serialize = COM_get_function_handle( "SURF.serialize_mesh");
  COM_call_function( SURF_serialize, &pmesh_hdl, &newmesh_hdl);

  std::cout << "Output window into file..." << endl;

  // Output normals
  int OUT_set = COM_get_function_handle( "OUT.set_option");
  int OUT_write = COM_get_function_handle( "OUT.write_dataitem");

  COM_call_function( OUT_set, "mode", "w");
  int all_hdl = COM_get_dataitem_handle( (newwin+".all").c_str());
  COM_call_function( OUT_write, argv[2], &all_hdl, 
		     (char*)wname.c_str(), "000");

  COM_finalize();
}



