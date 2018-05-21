// Copyright (C) 2010, Guy Barrand. All rights reserved.
// See the file exlib.license for terms.

// example of a little server done with tntnet.
// From a web browser with cmdserv running on the <ip> machine looking at the <port> port number :
//   http://<ip>:<port>/?cmd=help
//   http://<ip>:<port>/?cmd=ls      //list files of the home directory on the <ip> machine.
//   http://<ip>:<port>/?cmd=lsd     //list directories of the home directory on the <ip> machine.
//   http://<ip>:<port>/?cmd=lsf     //list only regular files of the home directory on the <ip> machine.
//   http://<ip>:<port>/?cmd=size+<file name>
//   http://<ip>:<port>/hello.html
//   http://<ip>:<port>/form.html
//    For the form, an uploaded file is put under /tmp/exlib_cmdserv_<filename>
// From term2serv program :
//   2cmdserv> help
//   2cmdserv> ls
//   2cmdserv> exit


//exlib_build_use inlib tntnet zip zlib thread dl

#ifdef _WIN32
#include <iostream>
int main() {
  std::cout << "cmdserv not yet ported on Windows." << std::endl;
  return 1;
}

#else //_WIN32

#include <inlib/cmd/cmds>
#include <inlib/cmd/rest>

INLIB_GLOBAL_STRING(exlib_cmdserv_args)

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#include <tnt/ecpp.h>
#include <tnt/componentfactory.h>
#include <tnt/httpreply.h>
#include <tnt/tntnet.h> //for shutdown.

namespace cmdserv {

class component : public tnt::EcppComponent {

  //INLIB_CLASS_STRING(GET)

  INLIB_CLASS_STRING(cmd)

  //INLIB_CLASS_STRING_VALUE(cmd_empty,)
  INLIB_CLASS_STRING_VALUE(cmd_help,help)
  INLIB_CLASS_STRING_VALUE(cmd_exit,exit)
  INLIB_CLASS_STRING_VALUE(cmd_ls,ls)
  INLIB_CLASS_STRING_VALUE(cmd_lsd,lsd)
  INLIB_CLASS_STRING_VALUE(cmd_lsf,lsf)
  INLIB_CLASS_STRING_VALUE(cmd_size,size)
  INLIB_CLASS_STRING_VALUE(cmd_shutdown,shutdown)

public:
  component(const tnt::Compident& a_ci,const tnt::Urlmapper& a_um,tnt::Comploader& a_cl)
  :EcppComponent(a_ci,a_um,a_cl)
  {
    //::printf("debug : create component\n");
  }

protected:
  ~component(){
    //::printf("debug : delete component\n");
  }

protected:
  inlib::cmd::session* get_session(tnt::HttpRequest& a_request) {
    tnt::Scope& _scope = a_request.getSessionScope();

    //TNT_SESSION_COMPONENT_VAR(session,s_session,"session s_session",());
    const std::string key = getComponentScopePrefix(getCompident())+"::session";
    inlib::cmd::session* _session = _scope.get<inlib::cmd::session>(key);
    if(!_session) {
      std::string sargs;
      std::string app_name;
      inlib_mforcit(std::string,std::string,a_request.getArgs(),it) {
        if((*it).first==s_exlib_cmdserv_args()) sargs = (*it).second;
      }
      inlib::args args(sargs," ",true);

      _session = new inlib::cmd::session(args);
      if(!_session || _session->pwd().empty()) {
        delete _session;
        return 0;
      }
      _session->add_cmd(s_cmd_size(),new inlib::cmd::size_cmd);
      _session->add_cmd(s_cmd_ls(),new inlib::cmd::ls_cmd);
      _session->add_cmd(s_cmd_lsd(),new inlib::cmd::lsd_cmd);
      _session->add_cmd(s_cmd_lsf(),new inlib::cmd::lsf_cmd);

      _scope.put<inlib::cmd::session>(key,_session);
    }
    //::printf("debug : scopekey : %s : %lu\n",key.c_str(),_session);

    return _session;
  }
  void delete_session(tnt::HttpRequest& a_request) {
    tnt::Scope& _scope = a_request.getSessionScope();
    const std::string key = getComponentScopePrefix(getCompident())+"::session";
    _scope.erase(key); //it deletes the session object.
  }
public:
  unsigned operator() (tnt::HttpRequest& a_request,tnt::HttpReply& a_reply,tnt::QueryParams& a_qparam) {

    inlib::cmd::session* psession = get_session(a_request);
    if(!psession) {
      std::string msg = " new session() failed, home directory not found.";
      a_reply.sout() << inlib::cmd::s_failed() << " : " << msg;
      return HTTP_OK;
    }
    inlib::cmd::session& _session = *psession;

    if(_session.verbose()) {
      ::printf("debug : a_request.getBody() : \"%s\"\n",a_request.getBody().c_str());
      ::printf("debug : a_request.getMethod() : \"%s\"\n",a_request.getMethod().c_str());
      ::printf("debug : a_request.getQuery() : \"%s\"\n",a_request.getQuery().c_str());
      ::printf("debug : a_request.getUrl() : \"%s\"\n",a_request.getUrl().c_str());
      ::printf("debug : a_request.getPathInfo() : \"%s\"\n",a_request.getPathInfo().c_str());
    }

    //std::string cmd = a_qparam.getUrl();
    //::printf("debug : a_qparam.getUrl() : \"%s\"\n",a_qparam.getUrl().c_str());

    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////
    std::string url = a_request.getUrl();
    if(url=="/") {

      std::string cmd = a_qparam.param(s_cmd());
      if(_session.verbose()) ::printf("cmdserv : cmd : \"%s\"\n",cmd.c_str());

      std::string rest;
      inlib::cmd::rest(cmd,rest);

      //////////////////////////////////////////////////////
      /// special commands : ///////////////////////////////
      //////////////////////////////////////////////////////
      if(cmd==s_cmd_help()) {
        std::vector<std::string> cmds;
        cmds.push_back(s_cmd_help());
        cmds.push_back(s_cmd_exit());
        cmds.push_back(s_cmd_ls());
        cmds.push_back(s_cmd_lsd());
        cmds.push_back(s_cmd_lsf());
        cmds.push_back(s_cmd_size());

	std::string stmp;
        if(!inlib::nums2s<std::string>(cmds,stmp)) {}
        a_reply.sout() << stmp;

      } else if(cmd==s_cmd_exit()) {
        delete_session(a_request);
        a_reply.sout() << inlib::cmd::s_done();

      } else if(cmd==s_cmd_shutdown()) {
        if(_session.can_shutdown()) {
          tnt::Tntnet::shutdown();
          a_reply.sout() << inlib::cmd::s_done();
        } else {
          _session.reply_failed(a_reply.sout(),"cmd=shutdown not enabled.");
        }

      //////////////////////////////////////////////////////
      /// other commands : /////////////////////////////////
      //////////////////////////////////////////////////////

      } else if(inlib::cmd::base_cmd* _cmd = _session.find_cmd(cmd)) {
        std::vector<std::string> words;
        inlib::words(rest," ",false,words);
        _cmd->exec(_session,words,a_reply.sout(),a_reply.out());
      } else {
        _session.reply_failed(a_reply.sout(),"unknown command.");
      }

    //////////////////////////////////////////////////////
    /// request from a web browser : /////////////////////
    //////////////////////////////////////////////////////
    } else if(url=="/hello.html") {
      a_reply.setContentType("text/html");
      /*
<meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n \

*/
      a_reply.out() << "\
<html>\n\
<head><title>exlib/cmdserv</title></head>\n\
<body>Hello cmdserv</body>\n\
</html>\n\
";

    } else if(url=="/form.html") {
      a_reply.setContentType("text/html");
      a_reply.out() << "\
<html>\n\
<head><title>exlib/cmdserv</title></head>\n\
<body>\n\
<form action=\"upload_form\" enctype=\"multipart/form-data\" method=\"post\">\n\
  <input type=\"file\" name=\"my_file\"><br>\n\
  <input type=\"text\" name=\"my_text\"><br>\n\
  <input type=\"submit\" value=\"Submit\">\n\
</form>\n\
</body>\n\
</html>\n\
";

    } else if(url=="/upload_form") {

      if(a_request.isMultipart()) {
        const tnt::Multipart& mp = a_request.getMultipart();
        tnt::Multipart::const_iterator it;
        for(it=mp.begin();it!=mp.end();++it) {
          const tnt::Part& part = *it;
          if(_session.verbose()) {
            ::printf("part size %lu\n",part.getSize());
            ::printf("part name : %s\n",part.getName().c_str());
            ::printf("part file name : %s\n",part.getFilename().c_str());
          }
          if(part.getName()=="my_file") {
            if(part.getFilename().size()) {
              std::string fname = "/tmp/exlib_cmdserv_"+part.getFilename();
              std::ofstream ofs(fname.c_str());
              if(ofs.is_open()) {
                tnt::Part::const_iterator itp;
                for(itp=part.begin();itp!=part.end();++itp) ofs << *itp;
                ofs.close();
                if(_session.verbose()) ::printf("cmdserv : upload_form : file saved in \"%s\"\n",fname.c_str());
              } else {
                if(_session.verbose()) ::printf("cmdserv : upload_form : can't open \"%s\"\n",fname.c_str());
              }
            } else {
              if(_session.verbose()) ::printf("cmdserv : upload_form : no file given\n");
            }
          } else if(part.getName()=="my_text") {
            std::ostringstream ss;
            tnt::Part::const_iterator itp;
            for(itp=part.begin();itp!=part.end();++itp) ss << *itp;
            if(_session.verbose()) ::printf("cmdserv : upload_form : my_text \"%s\"\n",ss.str().c_str());         
          }

        }
      }


    }

    return HTTP_OK;
  }
};

}

//////////////////////////////////////////////////
/// main : ////////////////////////////////////////
//////////////////////////////////////////////////

#include <tnt/tntnet.h>

#include <inlib/net/base_socket>

#include <cstdlib>

int main(int argc,char* argv[]){
#ifdef INLIB_MEM
  inlib::mem::set_check_by_class(true);{
#endif

  inlib::args args(argc,argv);

  if(args.is_arg(inlib::s_arg_help())) {
    std::cout << "args :" << std::endl
              << inlib::s_arg_help() << std::endl
              << inlib::s_arg_verbose() << std::endl
              << inlib::s_arg_host() << std::endl
              << inlib::s_arg_port() << std::endl
              << inlib::s_arg_can_shutdown() << std::endl
              << std::endl;
    return EXIT_SUCCESS;
  }

  bool verbose = args.is_arg(inlib::s_arg_verbose());

  std::string host;
  if(args.is_arg(inlib::s_arg_host())) {
    args.find(inlib::s_arg_host(),host,"0.0.0.0");
  } else {
    if(!inlib::net::host_name(std::cout,host)) host = "0.0.0.0";
  }
  unsigned int port;
  args.find<unsigned int>("-port",port,8000);
  if(verbose) {
    std::cout << "host " << host << std::endl;
    std::cout << "port " << port << std::endl;
  }

  tnt::ComponentFactoryImpl<cmdserv::component> Factory("cmdserv");

  try  { 

    tnt::Tntnet app;    

    app.setAppName("cmdserv");

    app.listen(host,port);    

    //NOTE : tnt example is "^/$", but the below pattern is needed for web clients.
    tnt::Mapping& murl = app.mapUrl("^/(.*)$","cmdserv");
    murl.setPathInfo("/cmdserv");

   {std::map<std::string,std::string> _args;
    std::string sargs; //space separated args : "-key1=value1 -key2=value2 ..."
    std::vector<std::string> v;args.to_vector(v);
    inlib_vforcit(std::string,v,it) {
      sargs += " ";
      sargs += *it;
    }
    _args[s_exlib_cmdserv_args()] =  sargs;
    murl.setArgs(_args);}

    if(verbose) std::cout << "tnt::Tntnet.run() ..." << std::endl;
    app.run(); 

  }  catch (const std::exception& a_e)  {
    std::cerr << a_e.what() << std::endl;
  }

#ifdef INLIB_MEM
  }inlib::mem::balance(std::cout);
  std::cout << "cmdserv : (mem) exit..." << std::endl;
#else
  std::cout << "cmdserv : exit..." << std::endl;
#endif

  return EXIT_SUCCESS;
}

#endif //_WIN32
