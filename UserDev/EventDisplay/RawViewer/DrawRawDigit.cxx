#ifndef EVD_DRAWRAWDIGIT_CXX
#define EVD_DRAWRAWDIGIT_CXX

#include "DrawRawDigit.h"

namespace evd {

DrawRawDigit::DrawRawDigit() {
  _name = "DrawRawDigit";
  _producer = "daq";

  // And whether or not to correct the data:
  _correct_data = false;

}

bool DrawRawDigit::initialize() {

  //
  // This function is called in the beggining of event loop
  // Do all variable initialization you wish to do here.
  // If you have a histogram to fill in the event loop, for example,
  // here is a good place to create one on the heap (i.e. "new TH1D").
  //
  //

  for (unsigned int p = 0; p < geoService->Nviews(); p++) {
    setXDimension(geoService->Nwires(p), p);
    setYDimension(detProp->ReadOutWindowSize(), p);
  }
  initDataHolder();

  if (larutil::LArUtilConfig::Detector() == galleryfmwk::geo::kMicroBooNE) {
    _noise_filter.init();
  }

  return true;
}

bool DrawRawDigit::analyze(gallery::Event *ev) {

  //
  // Do your event-by-event analysis here. This function is called for
  // each event in the loop. You have "storage" pointer which contains
  // event-wise data. To see what is available, check the "Manual.pdf":
  //
  // http://microboone-docdb.fnal.gov:8080/cgi-bin/ShowDocument?docid=3183
  //
  // Or you can refer to Base/DataFormatConstants.hh for available data type
  // enum values. Here is one example of getting PMT waveform collection.
  //
  // event_fifo* my_pmtfifo_v = (event_fifo*)(storage->get_data(DATA::PMFIFO));
  //
  // if( event_fifo )
  //
  //   std::cout << "Event ID: " << my_pmtfifo_v->event_id() << std::endl;
  //

  // This is an event viewer.  In particular, this handles raw wire signal
  // drawing.
  // So, obviously, first thing to do is to get the wires.

  art::InputTag wires_tag(_producer);
  auto const &raw_digits =
      ev->getValidHandle<std::vector<raw::RawDigit>>(wires_tag);

  // if the tick-length set is different from what is actually stored in the ADC
  // vector -> fix.
  if (raw_digits->size() > 0) {
    for (size_t pl = 0; pl < geoService->Nplanes(); pl++) {
      if (_y_dimensions[pl] != raw_digits->at(0).ADCs().size()) {
        _y_dimensions[pl] = raw_digits->at(0).ADCs().size();
      }
    }
  }

  _planeData.clear();


  if (larutil::LArUtilConfig::Detector() == galleryfmwk::geo::kMicroBooNE) {
    _noise_filter.set_n_time_ticks(_y_dimensions[0]);
  }
  initDataHolder();

  std::cout << "raw_digit size: " << raw_digits->size() << std::endl;

  for (auto const &rawdigit : *raw_digits) {
    unsigned int ch = rawdigit.Channel();
    if (larutil::LArUtilConfig::Detector() == galleryfmwk::geo::kMicroBooNE &&
        ch >= 8254)
      continue;

    unsigned int wire = geoService->ChannelToWire(ch);
    unsigned int plane = geoService->ChannelToPlane(ch);

    if (wire > geoService->Nwires(plane))
      continue;

    int offset = wire * _y_dimensions[plane];

    size_t i = 0;
    for (auto &tick : rawdigit.ADCs()) {
      _planeData.at(plane).at(offset + i) = tick;
      i++;
    }
  }

  if (larutil::LArUtilConfig::Detector() == galleryfmwk::geo::kMicroBooNE) {

    _noise_filter.set_data(&_planeData);
    if (_correct_data && ev->eventAuxiliary().isRealData()) {
      _noise_filter.clean_data();
    } else {
      _noise_filter.pedestal_subtract_only();
    }
  }

  return true;
}

bool DrawRawDigit::finalize() {

  // This function is called at the end of event loop.
  // Do all variable finalization you wish to do here.
  // If you need, you can store your ROOT class instance in the output
  // file. You have an access to the output file through "_fout" pointer.
  //
  // Say you made a histogram pointer h1 to store. You can do this:
  //
  // if(_fout) { _fout->cd(); h1->Write(); }
  //
  // else
  //   print(MSG::ERROR,__FUNCTION__,"Did not find an output file pointer!!!
  //   File not opened?");
  //

  return true;
}
}
#endif
