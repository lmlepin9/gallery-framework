#ifndef GALLERY_FMWK_SUPERA_MODULE_BASE_CXX
#define GALLERY_FMWK_SUPERA_MODULE_BASE_CXX

#include "supera_module_base.h"

#include "LArUtil/Geometry.h"
#include "LArUtil/GeometryHelper.h"

#include "LArUtil/LArUtilServicesHandler.h"

namespace supera {
SuperaModuleBase::SuperaModuleBase() {

    auto _geo_service = larutil::LArUtilServicesHandler::GetGeometry(_fcl_file_name);
    auto _det_service = larutil::LArUtilServicesHandler::GetDetProperties(_fcl_file_name);

    // Use GeoService to build the meta information:

    auto fDetLength     = _geo_service->DetLength(0);
    auto fDetHalfWidth  = _geo_service->DetHalfWidth(0);
    auto fDetHalfHeight = _geo_service->DetHalfHeight(0);

    // In 3D, using z = length, y = height, x = width

    // _base_image_meta_2D
    // void set_dimension(size_t axis, double image_size, size_t number_of_voxels, double origin = 0);

    _base_image_meta_3D.set_dimension(0, 2*fDetHalfWidth,  size_t(2*fDetHalfWidth));
    _base_image_meta_3D.set_dimension(1, 2*fDetHalfHeight, size_t(2*fDetHalfHeight));
    _base_image_meta_3D.set_dimension(2, fDetLength,       size_t(2*fDetLength));

// int plane = i % (_geo_service->Nplanes(0));
// int tpc = int(i/_geo_service->Nplanes(0));
//
// fWireStartVtx.at(i).resize(_geo_service->Nwires(plane, tpc));
    _base_image_meta_2D.resize(3);
    // Set the total ticks per image:
    total_ticks = n_ticks_per_chamber + n_cathode_ticks;
    std::cout << "Compression factor: " << compression << std::endl; 
    for (size_t plane = 0; plane < 3; plane ++){
        // For the first dimension, x, we need the number of wires:
        int n_wires = _geo_service->Nwires(plane, 0);
        //std::cout<<"Plane: " << plane <<" Number of wires: " << n_wires << std::endl;
        _base_image_meta_2D[plane].set_dimension(0, 0.3*_geo_service->Nwires(plane), _geo_service->Nwires(plane));
        _base_image_meta_2D[plane].set_dimension(1, 0.055*total_ticks, total_ticks/compression );
        _base_image_meta_2D[plane].set_projection_id(plane);
    }   // 0.078 is the default value for SBND, from the uboone CNN paper it is 0.055, need to check this though. 
        // Time2cm from uboone detector services:  0.0548965

}

int SuperaModuleBase::projection_id(int channel) {
  // Pretty hacky code here ...

  // In SBND, channels go 0 to 1985 (plane 0), 1986 to 3971, 3972 to 5637
  // Then repeat on the other side with offset of 5638, for a total
  // of 11276 channels


  // MicroBooNE, channels go 0 to 2399 (plane 0), 2400 to 4799, 4800 to 8255 
  // Just one drift chamber

  if (channel < 2400)
    return 0;
  else if (channel < 4800)
    return 1;
  else 
    return 2;
}

int SuperaModuleBase::column(int channel) {
  // Pretty hacky code here ...

  // In SBND, channels go 0 to 1985 (plane 0), 1986 to 3971, 3972 to 5637
  // Then repeat on the other side with offset of 5638, for a total
  // of 11276 channels

    // MicroBooNE, channels go 0 to 2399 (plane 0), 2400 to 4799, 4800 to 8255 
  // Just one drift chamber

  if (channel < 2400)
    return channel;
  else if (channel < 4800)
    return channel - 2400;
  else {
    return (channel - 4800);
  }
}

int SuperaModuleBase::row(int tick, int channel) {
  //std::cout<< "Tick - offset :" << tick - tick_offset <<std::endl;
  return tick - tick_offset;
  
}

float SuperaModuleBase::wire_position(float x, float y, float z, int projection_id){
    double vtx[3];
    vtx[0] = x;
    vtx[1] = y;
    vtx[2] = z;
    try{
      return larutil::Geometry::GetME()->WireCoordinate(vtx, projection_id);
    }
    catch(...){
      return -999.;
    }
}
float SuperaModuleBase::tick_position(float x, float time_offset, int projection_id){
    // Convert an x coordinate to a tick position

    // First, convert the tick into the plane with the drift velocity:
    // (Add an offset for the distance between planes)
    float tick = x / larutil::GeometryHelper::GetME()->TimeToCm();

    //std::cout << "Tick : " << tick << std::endl;

    if (x > 0){
      tick -= 7;
      if (projection_id == 0){
        tick -= 0.48;
      }
      if (projection_id == 1){
        tick -= -3.035;
      }
      if (projection_id == 2){
        tick -= 3.646;
      }
    }

    // if there is a time offset, add it:
    if(time_offset != 0){
      tick += time_offset;
    }


    // if (x < 0){

    // }

    // Accomodate the central x=0 position:
    tick += n_ticks_per_chamber;

   // std::cout << "Tick : " << tick << std::endl;


    // Apply compression:
    tick /= compression;

    return tick;
}


}
#endif
