// Copyright (C) 2010, Guy Barrand. All rights reserved.
// See the file exlib.license for terms.

//exlib_build_use inlib Getline socket

#include <Getline/ourex_Getline.h>

#ifdef INLIB_MEM
#include <inlib/mem>
#endif

#include <inlib/args>
#include <inlib/net/http>
#include <inlib/S_STRING>
#include <inlib/cmd/rest>
#include <inlib/sys/atime>
#include <inlib/sargs>
#include <inlib/srep>

#include <cstdlib>
#include <iostream>

INLIB_GLOBAL_STRING_VALUE(arg_prompt,-prompt)

INLIB_GLOBAL_STRING_VALUE(cmd_exit,exit)
INLIB_GLOBAL_STRING_VALUE(cmd_shutdown,shutdown)
INLIB_GLOBAL_STRING_VALUE(cmd_get,get)
INLIB_GLOBAL_STRING_VALUE(cmd_gradec,gradec)

INLIB_GLOBAL_STRING(failed)

int main(int argc,char* argv[]) {

#ifdef INLIB_MEM
  inlib::mem::set_check_by_class(true);{
#endif

  inlib::args args(argc,argv);

  if(args.is_arg(inlib::s_arg_help())) {
    std::cout << "<program> [options]" << std::endl
              << "options :" << std::endl
      << " " << inlib::s_arg_help() << " : help." << std::endl
      << " " << inlib::s_arg_verbose() << " : verbose." << std::endl
      << " " << inlib::s_arg_host() << "=<string> : IP of cmdserv computer." << std::endl
      << " " << inlib::s_arg_port() << "=<uint> : port of cmdserv." << std::endl
      << " " << s_arg_prompt() << "=<string> : prompt of terminal client." << std::endl
      << std::endl;
    return EXIT_SUCCESS;
  }

  bool verbose = args.is_arg(inlib::s_arg_verbose());

  std::string host;
  args.find(inlib::s_arg_host(),host,"0.0.0.0");

  unsigned int port;
  args.find<unsigned int>(inlib::s_arg_port(),port,8000);

  if(verbose) {
    std::cout << "host " << host << std::endl;
    std::cout << "port " << port << std::endl;
  }

  std::string prompt;
  args.find(s_arg_prompt(),prompt,"2cmdserv> ");
 
  std::string cookie;

  ::ourex_Gl_histinit((char*)"term_tntnet_client.hist");
  if(verbose) {
    std::cout << "type ^D to exit" << std::endl;
  }
  bool to_continue = true;
  while(to_continue) {
    std::string cmd;
    char* p = ::ourex_Getline(prompt.c_str());
    if(!(*p)) {
      //std::cout << "debug : ^D exit." << std::endl;
      if(cookie.size()) {
        cmd = s_cmd_exit()+" "+cookie;
        to_continue = false;
      } else {
        break;
      }
    } else {
      ourex_Gl_histadd(p);
      std::string s(p);
      cmd = s.substr(0,s.size()-1); //Remove last \n
      //std::cout << cmd << std::endl;
      if(cmd==s_cmd_exit()) {
        if(cookie.size()) {
          cmd = s_cmd_exit()+" "+cookie;
          to_continue = false;
        } else {
          break;
        }
      }
    }

    inlib::atime begin = inlib::atime::now();

    // send cmd :
    char* doc;
    inlib::uint64 ldoc;    

   {inlib::net::http http(std::cout,false);
    if(!http.start(host,port)) return EXIT_FAILURE;
    std::string ocookie;
    //if(!http.POST("cmd="+cmd,cookie,doc,ldoc,ocookie)) return EXIT_FAILURE;
    std::string req = "/?cmd="+cmd;
    inlib::replace(req," ","%20");
    if(!http.GET(req,cookie,doc,ldoc,ocookie)) return EXIT_FAILURE;
    if(cookie.empty()) cookie = ocookie;
    if(verbose) std::cout << "cookie :" << std::endl << cookie << std::endl;}

    // treat result :
   {std::string rest;
    inlib::cmd::rest(cmd,rest);
    if( (cmd==s_cmd_get()) || (cmd==s_cmd_gradec()) ) {
      std::string sout = rest;
      if(cmd==s_cmd_gradec()) sout = "out.fits";

      if(verbose) std::cout << "file : length " << ldoc << std::endl;

      if( (ldoc>=s_failed().size()) && (!::strncmp(doc,s_failed().c_str(),s_failed().size())) ) {
        std::cout << doc << std::endl;   
      } else {
        std::string local = inlib::base_name(sout);
        if(!inlib::file::write_bytes(local,doc,(size_t)ldoc)) {
          std::cout << "can't write local file " << inlib::sout(local) 
                    << std::endl;
        }
      }
    } else if(cmd==s_cmd_shutdown()) {
      to_continue = false;

    } else {
      if(ldoc) std::cout << doc << std::endl;   
    }}

    delete [] doc;

    if(verbose) std::cout << inlib::atime::elapsed(begin) << std::endl;
  }
  
#ifdef INLIB_MEM
  }inlib::mem::balance(std::cout);
#endif

  return EXIT_SUCCESS;
}
