//
//  Copyright@2013, Illinois Rocstar LLC. All rights reserved.
//        
//  See LICENSE file included with this source or
//  (opensource.org/licenses/NCSA) for license information. 
//

#include "Rocin.h"
#include "COM_base.hpp"
#include "com.h"
#include "com_devel.hpp"


#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cassert>
#include <sstream>

using namespace std;

#ifdef STATIC_LINK
extern "C" void SimOUT_load_module( const char *);
#endif

int main(int argc, char *argv[]) {
  COM_init( &argc, &argv);

#ifdef STATIC_LINK
  SimIN_load_module("IN");
  SimOUT_load_module("OUT");
#else
  COM_load_module("SimIN", "IN");
  COM_load_module("SimOUT", "OUT");
#endif

  if ( argc < 3 || argc > 3 ) {
    std::cout << "Usage: To test in serial: \n\t" << argv[0] 
	      << " <input HDF file|Rocin control file> <outputfile>\n"
	      << "To test in parallel: \n\t" << argv[0]
	      << " -com-mpi <Rocin control file> <output_prefix>\n"
	      << std::endl;
    // Note: -com-mpi option will be eaten away by COM_init.
    exit(-1);
  }

  COM_set_verbose(11);
  COM_set_profiling(1);

  const char *file_in  = argv[1];
  const char *file_out = argv[2];

  int IN_read;
  int IN_obtain = COM_get_function_handle( "IN.obtain_dataitem");

  const char *lastdot=std::strrchr( file_in, '.');
  if ( lastdot && std::strcmp( lastdot, ".hdf")==0) {
    IN_read = COM_get_function_handle( "IN.read_window");
  }
  else {
    IN_read = COM_get_function_handle( "IN.read_by_control_file");
  }

  const char *win_in = "rocin_win";
  const char *win_out= "user_win";

  std::string win_in_pre( win_in); win_in_pre.append(".");
  std::string win_out_pre( win_out); win_out_pre.append(".");

  MPI_Comm comm = MPI_COMM_WORLD;
  COM_call_function( IN_read, file_in, win_in, &comm);
  
  // Obtain the list of panes
  int np, *pane_ids;
  COM_get_panes( win_in, &np, &pane_ids);

  // Create a new window and register the dataitems
  COM_new_window( win_out);

  // Loop through the panes to register the meshes
  for ( int i=0; i<np; ++i) {
    int nconn;    // Number of connectivity tables
    char *cnames; // Names of connectivity tables separated by space

    // Obtain the connectivity tables
    COM_get_connectivities( win_in, pane_ids[i], &nconn, &cnames);

    if ( nconn == 1 && strncmp(cnames,":st",3)==0) { // Structured mesh
      int ndims;   // Number of elements
      int nglayers; // Number of ghost layers.
      COM_get_size( (win_in_pre+cnames).c_str(), pane_ids[i], &ndims, &nglayers);
      COM_set_size( (win_out_pre+cnames).c_str(), pane_ids[i], ndims, nglayers);

      // Obtain the dimensions (must be a const array) of the pane and set them
      const int *dims;
      COM_get_array_const( (win_in_pre+cnames).c_str(), pane_ids[i], &dims);
      COM_set_array_const( (win_out_pre+cnames).c_str(), pane_ids[i], dims);

      std::cout << "Structured information" << endl;
      cout << "  ndims = " << ndims << endl;
      cout << "  nglayers = " << nglayers << endl;
      cout << "  dims[0] = " << dims[0] << endl;
      if ( ndims>1) cout << "  dims[1] = " << dims[1] << endl;
      if ( ndims>2) cout << "  dims[2] = " << dims[2] << endl;
      
    }
    else { // Unstructured mesh
      int  nnodes;  // total number of nodes
      int  ngnodes; // Number of ghost nodes

      // Obtain the size of nodes
      COM_get_size((win_in_pre+"nc").c_str(), pane_ids[i], &nnodes, &ngnodes);
      COM_set_size((win_out_pre+"nc").c_str(), pane_ids[i], nnodes, ngnodes);
      std::cout << "# nodes in dest set to " << nnodes
		<< " & # gnodes in dest set to " << ngnodes << std::endl;
      
      // Obtain the sizes of connectivity tables
      if ( nconn>0) {
	std::istringstream is( cnames);
	for ( int k=0; k<nconn; ++k) {
	  std::string cname;
	  is >> cname;
	  int nelems, ng;
	  COM_get_size((win_in_pre+cname).c_str(), pane_ids[i], &nelems, &ng);
	  COM_set_size((win_out_pre+cname).c_str(), pane_ids[i], nelems, ng);
	  std::cout << "Connectivity table " << i << " has " << nelems << " elements and "
		    << ng << " ghost nodes" << std::endl;
	  COM_resize_array( (win_out_pre+cname).c_str(), pane_ids[i]);
	}
      }

      // free the buffer of cnames
      COM_free_buffer( &cnames);
    }

    COM_resize_array( (win_out_pre+"nc").c_str(), pane_ids[i]);
  }

  // Obtain the list of dataitems
  int na;      // Number of dataitems
  char *atts;  // names of dataitems separated by spaces
  COM_get_dataitems( win_in, &na, &atts);

  std::istringstream is(atts);
  for ( int i=0; i<na; ++i) {
    // Obtain the dataitem name
    std::string aname;  is >> aname; 
    char loc;
    int  type, ncomp;
    std::string unit;

    COM_get_dataitem( (win_in_pre+aname).c_str(), &loc, &type, &ncomp, &unit);
    std::cout << (win_in_pre+aname).c_str() << " has type " << type <<  endl; 
    std::cout << (win_in_pre+aname).c_str() << " has " << ncomp << " components" << endl; 
    std::string waname = win_out_pre+aname;
    COM_new_dataitem( waname.c_str(), loc, type, ncomp, unit.c_str());

    if ( loc == 'w') {
      std::cout << "Windowed dataitem " << endl;
      
      // Obtain the size for a window dataitem.
      int nitems, ng;
      COM_get_size((win_in_pre+aname).c_str(), 0, &nitems, &ng);

      COM_set_size( waname.c_str(), 0, nitems, ng);
      
      COM_resize_array( waname.c_str(), 0, NULL, ncomp);
    }
    // Loop through the panes to allocate memory
    else {
      std::cout << "Panel dataitem" << endl;
      // This is to demonstrate the loop over panes.
      // Could be replaced by a single call with paneID=0.
      for ( int i=0; i<np; ++i) {

	if ( loc == 'p') { 
	  // Obtain the size for a pane dataitem.
	  int nitems, ng;
	  COM_get_size((win_in_pre+aname).c_str(), pane_ids[i], &nitems, &ng);
	  COM_set_size( waname.c_str(), pane_ids[i], nitems, ng);
	}

	COM_resize_array( waname.c_str(), pane_ids[i], NULL, ncomp);
      }
    }
  }

  // Free buffers for pane ids and dataitem names
  COM_free_buffer( &pane_ids);
  COM_free_buffer( &atts);

  // Mark the end of initialization
  COM_window_init_done( win_out);

  //  Finally, copy data from in to out.

  int OUT_all = COM_get_dataitem_handle((win_out_pre+"all").c_str());
  int IN_all   = COM_get_dataitem_handle((win_in_pre+"all").c_str());

  COM_call_function(IN_obtain, &IN_all, &OUT_all);

  int OUT_write = COM_get_function_handle( "OUT.write_dataitem");
  COM_call_function( OUT_write, file_out, &OUT_all, win_out, "000");

  COM_print_profile("", "");
  COM_finalize();
}



