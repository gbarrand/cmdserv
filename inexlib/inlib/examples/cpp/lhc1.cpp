// Copyright (C) 2010, Guy Barrand. All rights reserved.
// See the file inlib.license for terms.

//inlib_build_use socket

#include <inlib/args>
#include <inlib/net/http>

#include <cstdlib>
#include <iostream>

int main(int argc,char* argv[]) {

  inlib::args args(argc,argv);

  bool verbose = args.is_arg("-verbose");

  inlib::net::http http(std::cout,verbose);

  std::string host("vistar-capture.web.cern.ch");
  unsigned int port = 80;

  if(!http.start(host,port)) return EXIT_FAILURE;

  std::string request("/vistar-capture/lhc1.png"); //width=1024 height=768 bpp=3

 {std::string icookie,ocookie;
  char* doc;
  inlib::uint64 ldoc;
  if(!http.GET(request,icookie,doc,ldoc,ocookie)) return EXIT_FAILURE;
  if(!inlib::file::write_bytes("lhc1.png",doc,(size_t)ldoc)) {
    std::cout << "can't write lhc1.png file." << std::endl;
  }}

  return EXIT_SUCCESS;
}
