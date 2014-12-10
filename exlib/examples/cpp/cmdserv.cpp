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

#ifdef WIN32
#include <iostream>
int main() {
  std::cout << "cmdserv not yet ported on Windows." << std::endl;
  return 1;
}

#else //WIN32

#include <inlib/args>
#include <inlib/S_STRING>
#include <inlib/file>
#include <inlib/sep>
#include <inlib/tos>
#include <inlib/sys/dir>
#include <inlib/cmd_rest>

#include <map>

INLIB_GLOBAL_STRING(exlib_cmdserv_args)
INLIB_GLOBAL_STRING_VALUE(arg_verbose,-verbose)
INLIB_GLOBAL_STRING_VALUE(arg_can_shutdown,-can_shutdown)

namespace cmdserv {

INLIB_GLOBAL_STRING(done)
INLIB_GLOBAL_STRING(failed)

class session;

class base_cmd {
  INLIB_SCLASS(cmdserv::base_cmd)
public:
  virtual void exec(session&,const std::vector<std::string>&,std::ostream&,std::ostream&) = 0;
public:
  base_cmd(){
#ifdef INLIB_MEM
    inlib::mem::increment(s_class().c_str());
#endif
  }
  virtual ~base_cmd(){
#ifdef INLIB_MEM
    inlib::mem::decrement(s_class().c_str());
#endif
  }
public:
  base_cmd(const base_cmd&){
#ifdef INLIB_MEM
    inlib::mem::increment(s_class().c_str());
#endif
  }
  base_cmd& operator=(const base_cmd&){return *this;}
};

class session {
  INLIB_SCLASS(cmdserv::session)
public:
  session(inlib::args& a_args):m_args(a_args),m_verbose(false),m_can_shutdown(false){
#ifdef INLIB_MEM
    inlib::mem::increment(s_class().c_str());
#endif
    m_verbose = a_args.is_arg(s_arg_verbose());
    m_can_shutdown = a_args.is_arg(s_arg_can_shutdown());
    if(m_verbose) ::printf("cmdserv::session::session() : %lu\n",(unsigned long)this);
    m_home = inlib::dir::home();
    /*
    if(m_home.size()) {
      m_home += inlib::sep();
      m_home += "Documents";
      m_home += inlib::sep();
      m_home += "cmdserv";
    }
    */
    m_pwd = m_home;
  }
  virtual ~session(){
    inlib_mforit(std::string,base_cmd*,m_cmds,it){delete (*it).second;}
    m_cmds.clear();
    if(m_verbose) ::printf("cmdserv::session::~session() : %lu\n",(unsigned long)this);
#ifdef INLIB_MEM
    inlib::mem::decrement(s_class().c_str());
#endif
  }
private:
  session(const session&){}
  session& operator=(const session&){return *this;}
public:
  void add_cmd(const std::string& a_name,base_cmd* a_cmd) {m_cmds[a_name] = a_cmd;}
  base_cmd* find_cmd(const std::string& a_name) const {
    std::map<std::string,base_cmd*>::const_iterator it = m_cmds.find(a_name);
    if(it==m_cmds.end()) return 0;
    return (*it).second;    
  }
  void reply_failed(std::ostream& a_sout,const std::string& a_msg) {
    a_sout << s_failed() << " : " << a_msg;
    if(m_verbose) ::printf("cmdserv : failed : %s\n",a_msg.c_str());
  }
  bool verbose() const {return m_verbose;}
  void set_verbose(bool a_value) {m_verbose = a_value;}
  bool can_shutdown() const {return m_can_shutdown;}
  const std::string& home_dir() const {return m_home;}
  const std::string& pwd() const {return m_pwd;}
  void set_pwd(const std::string& a_value) {m_pwd = a_value;}
protected:
  inlib::args m_args;
  bool m_verbose;
  bool m_can_shutdown;
  std::string m_home;
  std::string m_pwd;
  std::map<std::string,base_cmd*> m_cmds;
};

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

class ls_cmd : public base_cmd {
  typedef base_cmd parent;
public:
  virtual void exec(session& a_session,const std::vector<std::string>& a_args,std::ostream& a_sout,std::ostream&) {
    std::string where = a_args.empty()?a_session.pwd():a_args[0];
    std::vector<std::string> ls;
    if(!inlib::dir::entries(where,ls,false)) {
      a_session.reply_failed(a_sout,"ls_cmd : get directory entries failed.");
      return;
    }
    a_sout << inlib::tos(ls);
    if(a_session.verbose()) ::printf("cmdserv : ls_cmd : \"%s\"\n",inlib::tos(ls).c_str());
  }
public:
  ls_cmd():parent(){}
  virtual ~ls_cmd(){}
public:
  ls_cmd(const ls_cmd& a_from):parent(a_from){}
  ls_cmd& operator=(const ls_cmd& a_from){
    parent::operator=(a_from);
    return *this;
  }
};

class lsd_cmd : public base_cmd {
  typedef base_cmd parent;
public:
  virtual void exec(session& a_session,const std::vector<std::string>& a_args,std::ostream& a_sout,std::ostream&) {
    std::string where = a_args.empty()?a_session.pwd():a_args[0];
    std::vector<std::string> ls;
    if(!inlib::dir::dirs(where,ls,false)) {
      a_session.reply_failed(a_sout,"lsd_cmd : get directory entries failed.");
      return;
    }
    a_sout << inlib::tos(ls);
    if(a_session.verbose()) ::printf("cmdserv : lsd_cmd : \"%s\"\n",inlib::tos(ls).c_str());
  }
public:
  lsd_cmd():parent(){}
  virtual ~lsd_cmd(){}
public:
  lsd_cmd(const lsd_cmd& a_from):parent(a_from){}
  lsd_cmd& operator=(const lsd_cmd& a_from){
    parent::operator=(a_from);
    return *this;
  }
};


class lsf_cmd : public base_cmd {
  typedef base_cmd parent;
public:
  virtual void exec(session& a_session,const std::vector<std::string>& a_args,std::ostream& a_sout,std::ostream&) {
    std::string where = a_args.empty()?a_session.pwd():a_args[0];
    std::vector<std::string> ls;
    if(!inlib::dir::files(where,ls,false)) {
      a_session.reply_failed(a_sout,"lsf_cmd : get directory entries failed.");
      return;
    }
    a_sout << inlib::tos(ls);
    if(a_session.verbose()) ::printf("cmdserv : lsf_cmd : \"%s\"\n",inlib::tos(ls).c_str());
  }
public:
  lsf_cmd():parent(){}
  virtual ~lsf_cmd(){}
public:
  lsf_cmd(const lsf_cmd& a_from):parent(a_from){}
  lsf_cmd& operator=(const lsf_cmd& a_from){
    parent::operator=(a_from);
    return *this;
  }
};

class size_cmd : public base_cmd {
  typedef base_cmd parent;
public:
  virtual void exec(session& a_session,
                    const std::vector<std::string>& a_args,
                    std::ostream& a_sout,std::ostream& a_out) {
    if(a_args.size()!=1) {
      a_session.reply_failed(a_sout,"size_cmd : one argument expected.");
      return;
    }    
    const std::string& arg0 = a_args[0];
    std::string path;
    if(inlib::is_absolute_path(arg0)) {
      path = arg0;
    } else {
      path = a_session.pwd()+inlib::sep()+arg0;
    }
    if(!inlib::file::exists(path)) {
      a_session.reply_failed(a_sout,"size_cmd : none existing file "+path);
      return;
    } 
    long sz;
    if(!inlib::file::size(path,sz)) {
      a_session.reply_failed(a_sout,"size_cmd : get file size failed.");
      return;
    }
    a_out << inlib::tos((inlib::uint64)sz);
    if(a_session.verbose()) ::printf("cmdserv : size_cmd : \"%s\" ok.\n",path.c_str());
  }
public:
  size_cmd():parent(){}
  virtual ~size_cmd(){}
public:
  size_cmd(const size_cmd& a_from):parent(a_from){}
  size_cmd& operator=(const size_cmd& a_from){
    parent::operator=(a_from);
    return *this;
  }
};

class cd_cmd : public base_cmd {
  typedef base_cmd parent;
public:
  virtual void exec(session& a_session,
                    const std::vector<std::string>& a_args,
                    std::ostream& a_sout,std::ostream&) {
    if(a_args.empty()) {
      a_session.set_pwd(a_session.home_dir());
      a_sout << s_done();
      return;
    }
    if(a_args.size()!=1) {
      a_session.reply_failed(a_sout,"cd_cmd : one argument expected.");
      return;
    }    

    std::string new_dir;
    if(a_args.empty()) {
      new_dir = a_session.home_dir();
    } else {
      const std::string& arg0 = a_args[0];
      if(inlib::is_absolute_path(arg0)) {
        new_dir = arg0;
      } else {
        new_dir = a_session.pwd();
        new_dir += inlib::sep();      
        new_dir += arg0;
      }
    }
    bool is;
    inlib::dir::is_a(new_dir,is);
    if(!is) {
      a_session.reply_failed(a_sout,"cd_cmd : unknown directory.");
      return;
    }
    a_session.set_pwd(new_dir);
    a_sout << s_done();
  }
public:
  cd_cmd():parent(){}
  virtual ~cd_cmd(){}
public:
  cd_cmd(const cd_cmd& a_from):parent(a_from){}
  cd_cmd& operator=(const cd_cmd& a_from){
    parent::operator=(a_from);
    return *this;
  }
};

}

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
  session* get_session(tnt::HttpRequest& a_request) {
    tnt::Scope& _scope = a_request.getSessionScope();

    //TNT_SESSION_COMPONENT_VAR(session,s_session,"session s_session",());
    const std::string key = getComponentScopePrefix(getCompident())+"::session";
    session* _session = _scope.get<session>(key);
    if(!_session) {
      std::string sargs;
      std::string app_name;
      inlib_mforcit(std::string,std::string,a_request.getArgs(),it) {
        if((*it).first==s_exlib_cmdserv_args()) sargs = (*it).second;
      }
      inlib::args args(sargs," ",true);

      _session = new session(args);
      if(!_session || _session->pwd().empty()) {
        delete _session;
        return 0;
      }
      _session->add_cmd(s_cmd_size(),new size_cmd);
      _session->add_cmd(s_cmd_ls(),new ls_cmd);
      _session->add_cmd(s_cmd_lsd(),new lsd_cmd);
      _session->add_cmd(s_cmd_lsf(),new lsf_cmd);

      _scope.put<session>(key,_session);
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

    session* psession = get_session(a_request);
    if(!psession) {
      std::string msg = " new session() failed, home directory not found.";
      a_reply.sout() << s_failed() << " : " << msg;
      return HTTP_OK;
    }
    session& _session = *psession;

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
      inlib::cmd_rest(cmd,rest);

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

        a_reply.sout() << inlib::tos(cmds);

      } else if(cmd==s_cmd_exit()) {
        delete_session(a_request);
        a_reply.sout() << s_done();

      } else if(cmd==s_cmd_shutdown()) {
        if(_session.can_shutdown()) {
          tnt::Tntnet::shutdown();
          a_reply.sout() << s_done();
        } else {
          _session.reply_failed(a_reply.sout(),"cmd=shutdown not enabled.");
        }

      //////////////////////////////////////////////////////
      /// other commands : /////////////////////////////////
      //////////////////////////////////////////////////////

      } else if(base_cmd* _cmd = _session.find_cmd(cmd)) {
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

#ifdef INLIB_MEM
#include <inlib/mem>
#endif

#include <inlib/args>
#include <inlib/S_STRING>
#include <inlib/net/base_socket>

#include <cstdlib>

INLIB_GLOBAL_STRING_VALUE(arg_help,-help)
INLIB_GLOBAL_STRING_VALUE(arg_host,-host)
INLIB_GLOBAL_STRING_VALUE(arg_port,-port)

int main(int argc,char* argv[]){
#ifdef INLIB_MEM
  inlib::mem::set_check_by_class(true);{
#endif

  inlib::args args(argc,argv);

  if(args.is_arg(s_arg_help())) {
    std::cout << "args :" << std::endl
              << s_arg_help() << std::endl
              << s_arg_verbose() << std::endl
              << s_arg_host() << std::endl
              << s_arg_port() << std::endl
              << s_arg_can_shutdown() << std::endl
              << std::endl;
    return EXIT_SUCCESS;
  }

  bool verbose = args.is_arg(s_arg_verbose());

  std::string host;
  if(args.is_arg(s_arg_host())) {
    args.find(s_arg_host(),host,"0.0.0.0");
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
    std::vector<std::string> v = args.tovector();
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

#endif //WIN32
