////////////////////////////////////////////////////////////////////////////////
/// @brief AvocadoDB server
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2011 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2010-2011, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include <Basics/Common.h>

#include <v8.h>

#include "build.h"

#include <Basics/logging.h>
#include <Basics/Logger.h>
#include <Basics/ProgramOptions.h>
#include <Basics/ProgramOptionsDescription.h>
#include <Basics/files.h>
#include <Basics/init.h>
#include <Basics/safe_cast.h>
#include <Basics/strings.h>

#include <Rest/AnyServer.h>
#include <Rest/ApplicationHttpServer.h>
#include <Rest/ApplicationServerDispatcher.h>
#include <Rest/HttpHandlerFactory.h>
#include <Rest/Initialise.h>

#include <Admin/ApplicationAdminServer.h>
#include <Admin/RestHandlerCreator.h>

#include <VocBase/vocbase.h>

#include "RestHandler/RestActionHandler.h"
#include "RestHandler/RestDocumentHandler.h"

#include "Dispatcher/DispatcherThread.h"
#include "Dispatcher/DispatcherImpl.h"

#include "V8/v8-actions.h"
#include "V8/v8-globals.h"
#include "V8/v8-shell.h"
#include "V8/v8-utils.h"
#include "V8/v8-vocbase.h"

using namespace triagens;
using namespace triagens::basics;
using namespace triagens::rest;
using namespace triagens::admin;
using namespace triagens::avocado;

// -----------------------------------------------------------------------------
// --SECTION--                                          ACTION DISPTACHER THREAD
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                   private classes
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup AvocadoDB
/// @{
////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////
/// @brief dispatcher thread
////////////////////////////////////////////////////////////////////////////////

  class ActionDisptacherThread : public DispatcherThread {
    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief action path
////////////////////////////////////////////////////////////////////////////////

      static string _actionPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief startup path
////////////////////////////////////////////////////////////////////////////////

      static string _startupPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief vocbase
////////////////////////////////////////////////////////////////////////////////

      static TRI_vocbase_t* _vocbase;

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructs a new dispatcher thread
////////////////////////////////////////////////////////////////////////////////

      ActionDisptacherThread (DispatcherQueue* queue)
        : DispatcherThread(queue),
          _report(false),
          _isolate(0) {
      }

////////////////////////////////////////////////////////////////////////////////
/// @brief destructs a dispatcher thread
////////////////////////////////////////////////////////////////////////////////

      ~ActionDisptacherThread () {
      }

    public:

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

      void reportStatus () {
        _report = true;
      }

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

      void tick () {
        while(!v8::V8::IdleNotification()) {
        }

        if (! _report || _isolate == 0) {
          return;
        }

        _report = false;

        TRI_v8_global_t* v8g = (TRI_v8_global_t*) _isolate->GetData();

        if (v8g == 0) {
          return;
        }

        LOGGER_DEBUG << "active queries: " << v8g->JSQueries.size();
        LOGGER_DEBUG << "active result-sets: " << v8g->JSResultSets.size();
      }

    public:

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

      void run () {
        initialise();

        _isolate->Enter();
        _context->Enter();

        DispatcherThread::run();

        _context->Exit();
        _context.Dispose();

        _isolate->Exit();
        _isolate->Dispose();
      }

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief initialises the isolate and context
////////////////////////////////////////////////////////////////////////////////

      void initialise () {
        bool ok;
        char* filename;

        _isolate = v8::Isolate::New();

        _isolate->Enter();

        // create the context
        _context = v8::Context::New(0);

        if (_context.IsEmpty()) {
          LOGGER_FATAL << "cannot initialize V8 engine";
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }

        _context->Enter();

        TRI_InitV8VocBridge(_context, _vocbase);
        TRI_InitV8Actions(_context);
        TRI_InitV8Utils(_context);
        TRI_InitV8Shell(_context);

        filename = TRI_Concatenate2File(_startupPath.c_str(), "json.js");
        ok = TRI_LoadJavaScriptFile(_context, filename);

        if (! ok) {
          LOGGER_FATAL << "cannot load json utilities from file '" << filename << "'";
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }

        TRI_FreeString(filename);

        filename = TRI_Concatenate2File(_startupPath.c_str(), "actions.js");
        ok = TRI_LoadJavaScriptFile(_context, filename);

        if (! ok) {
          LOGGER_FATAL << "cannot load actions basics from file '" << filename << "'";
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }

        TRI_FreeString(filename);

        ok = TRI_LoadJavaScriptDirectory(_context, _actionPath.c_str());

        if (! ok) {
          LOGGER_FATAL << "cannot load actions from directory '" << filename << "'";
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }

        _context->Exit();
        _isolate->Exit();
      }

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief report required
////////////////////////////////////////////////////////////////////////////////

      bool _report;

////////////////////////////////////////////////////////////////////////////////
/// @brief isolate
////////////////////////////////////////////////////////////////////////////////

      v8::Isolate* _isolate;

////////////////////////////////////////////////////////////////////////////////
/// @brief context
////////////////////////////////////////////////////////////////////////////////

      v8::Persistent<v8::Context> _context;
  };

////////////////////////////////////////////////////////////////////////////////
/// @brief action path
////////////////////////////////////////////////////////////////////////////////

  string ActionDisptacherThread::_actionPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief startup path
////////////////////////////////////////////////////////////////////////////////

  string ActionDisptacherThread::_startupPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief vocbase
////////////////////////////////////////////////////////////////////////////////

  TRI_vocbase_t* ActionDisptacherThread::_vocbase;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief action dispatcher thread creator
////////////////////////////////////////////////////////////////////////////////

DispatcherThread* ActionDisptacherThreadCreator (DispatcherQueue* queue) {
  return new ActionDisptacherThread(queue);
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                    AVOCADO SERVER
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                   private classes
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup AvocadoDB
/// @{
////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////
/// @brief AvocadoDB server
////////////////////////////////////////////////////////////////////////////////

  class AvocadoDB : public AnyServer {
    private:
      AvocadoDB (const AvocadoDB&);
      AvocadoDB& operator= (const AvocadoDB&);

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief UnviversalVoc constructor
////////////////////////////////////////////////////////////////////////////////

      AvocadoDB (int argc, char** argv)
        : _argc(argc),
          _argv(argv),
          _applicationAdminServer(0),
          _applicationHttpServer(0),
          _httpServer(0),
          _httpPort("localhost:8529"),
          _dispatcherThreads(1),
          _startupPath("/usr/share/avocado/js"),
          _actionPath("/usr/share/avocado/js/actions"),
          _actionThreads(1),
          _databasePath("/var/lib/avocado"),
          _vocbase(0) {
        _workingDirectory = "/var/tmp";
      }

    public:

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

      void buildApplicationServer () {
        _applicationServer = ApplicationServerDispatcher::create("[<options>] - starts the triAGENS AvocadoDB", TRIAGENS_VERSION);
        _applicationServer->setSystemConfigFile("avocado.conf");
        _applicationServer->setUserConfigFile(".avocado/avocado.conf");

        // .............................................................................
        // allow multi-threading scheduler
        // .............................................................................

        _applicationServer->allowMultiScheduler(true);

        // .............................................................................
        // and start a simple admin server
        // .............................................................................

        _applicationAdminServer = ApplicationAdminServer::create(_applicationServer);
        _applicationServer->addFeature(_applicationAdminServer);

        _applicationAdminServer->allowLogViewer();
        _applicationAdminServer->allowVersion("avocado", TRIAGENS_VERSION);

        // .............................................................................
        // a http server
        // .............................................................................

        _applicationHttpServer = ApplicationHttpServer::create(_applicationServer);
        _applicationServer->addFeature(_applicationHttpServer);

        // .............................................................................
        // daemon and supervisor mode
        // .............................................................................

        map<string, ProgramOptionsDescription> additional;

        additional[ApplicationServer::OPTIONS_CMDLINE]
          ("daemon", "run as daemon")
          ("supervisor", "starts a supervisor and runs as daemon")
          ("pid-file", &_pidFile, "pid-file in daemon mode")
        ;

        // .............................................................................
        // for this server we display our own options such as port to use
        // .............................................................................

        _applicationHttpServer->showPortOptions(false);

        additional["PORT Options"]
          ("server.http-port", &_httpPort, "port for client access")
          ("dispatcher.threads", &_dispatcherThreads, "number of dispatcher threads")
        ;

        // .............................................................................
        // database options
        // .............................................................................

        additional["DATABASE Options"]
          ("database.path", &_databasePath, "path to the database directory")
        ;

        // .............................................................................
        // JavaScript options
        // .............................................................................

        additional["JAVASCRIPT Options"]
          ("startup.directory", &_startupPath, "path to the directory containing the startup scripts")
          ("action.directory", &_actionPath, "path to the action directory")
          ("action.threads", &_actionThreads, "threads for actions")
        ;

        // .............................................................................
        // parse the command line options - exit if there is a parse error
        // .............................................................................

        if (! _applicationServer->parse(_argc, _argv, additional)) {
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }

        // .............................................................................
        // sanity checks
        // .............................................................................

        if (_applicationServer->programOptions().has("daemon")) {
          _daemonMode = true;
        }

        if (_applicationServer->programOptions().has("supervisor")) {
          _supervisorMode = true;
        }

        if (_daemonMode) {
          if (_pidFile.empty()) {
            LOGGER_FATAL << "no pid-file defined, but daemon mode requested";
            TRI_ShutdownLogging();
            exit(EXIT_FAILURE);
          }
        }

        if (_databasePath.empty()) {
          LOGGER_FATAL << "no database path has been supplied, giving up";
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }
      }

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

      int startupServer () {
        v8::HandleScope handle_scope;

        // .............................................................................
        // open the database
        // .............................................................................

        _vocbase = TRI_OpenVocBase(_databasePath.c_str());

        if (_vocbase == 0) {
          LOGGER_FATAL << "cannot open database '" << _databasePath << "'";
          TRI_ShutdownLogging();
          exit(EXIT_FAILURE);
        }

        // .............................................................................
        // create the action dispatcher thread infor
        // .............................................................................

        ActionDisptacherThread::_actionPath = _actionPath;
        ActionDisptacherThread::_startupPath = _startupPath;
        ActionDisptacherThread::_vocbase = _vocbase;

        // .............................................................................
        // create the various parts of the universalVoc server
        // .............................................................................

        _applicationServer->buildScheduler();
        _applicationServer->buildSchedulerReporter();
        _applicationServer->buildControlCHandler();

        safe_cast<ApplicationServerDispatcher*>(_applicationServer)->buildDispatcher();
        safe_cast<ApplicationServerDispatcher*>(_applicationServer)->buildDispatcherReporter();
        safe_cast<ApplicationServerDispatcher*>(_applicationServer)->buildStandardQueue(_dispatcherThreads);

        Dispatcher* dispatcher = safe_cast<ApplicationServerDispatcher*>(_applicationServer)->dispatcher();

        if (_actionThreads < 1) {
          _actionThreads = 1;
        }

        safe_cast<DispatcherImpl*>(dispatcher)->addQueue("ACTION", ActionDisptacherThreadCreator, _actionThreads);

        // .............................................................................
        // create a http server and http handler factory
        // .............................................................................

        HttpHandlerFactory* factory = new HttpHandlerFactory();

        vector<AddressPort> ports;
        ports.push_back(AddressPort(_httpPort));

        _applicationAdminServer->addBasicHandlers(factory);

        factory->addPrefixHandler(RestVocbaseBaseHandler::DOCUMENT_PATH, RestHandlerCreator<RestDocumentHandler>::createData<TRI_vocbase_t*>, _vocbase);
        factory->addPrefixHandler(RestVocbaseBaseHandler::ACTION_PATH, RestHandlerCreator<RestActionHandler>::createData<TRI_vocbase_t*>, _vocbase);

        _httpServer = _applicationHttpServer->buildServer(factory, ports);

        // .............................................................................
        // start the main event loop
        // .............................................................................

        _applicationServer->start();
        _applicationServer->wait();

        TRI_CloseVocBase(_vocbase);

        LOGGER_INFO << "AvocadoDB has been shut down";

        return 0;
      }

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief number of command line arguments
////////////////////////////////////////////////////////////////////////////////

      int _argc;

////////////////////////////////////////////////////////////////////////////////
/// @brief command line arguments
////////////////////////////////////////////////////////////////////////////////

      char** _argv;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructed admin server application
////////////////////////////////////////////////////////////////////////////////

      ApplicationAdminServer* _applicationAdminServer;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructed http server application
////////////////////////////////////////////////////////////////////////////////

      ApplicationHttpServer* _applicationHttpServer;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructed http server
////////////////////////////////////////////////////////////////////////////////

      HttpServer* _httpServer;

////////////////////////////////////////////////////////////////////////////////
/// @brief list port for client requests
////////////////////////////////////////////////////////////////////////////////

      string _httpPort;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of dispatcher threads
////////////////////////////////////////////////////////////////////////////////

      int _dispatcherThreads;

////////////////////////////////////////////////////////////////////////////////
/// @brief path to the directory containing the startup scripts
////////////////////////////////////////////////////////////////////////////////

      string _startupPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief path to the action directory
////////////////////////////////////////////////////////////////////////////////

      string _actionPath;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of action threads
////////////////////////////////////////////////////////////////////////////////

      int _actionThreads;

////////////////////////////////////////////////////////////////////////////////
/// @brief path to the database
////////////////////////////////////////////////////////////////////////////////

      string _databasePath;

////////////////////////////////////////////////////////////////////////////////
/// @brief vocbase
////////////////////////////////////////////////////////////////////////////////

      TRI_vocbase_t* _vocbase;
  };
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup AvocadoDB
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief creates an application server
////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[]) {
  TRIAGENS_C_INITIALISE;
  TRIAGENS_REST_INITIALISE;

  TRI_InitialiseVocBase();
  TRI_SetLogLevelLogging("trace");

  // create and start a AvocadoDB server
  AvocadoDB server(argc, argv);

  int res = server.start();

  // shutdown database
  TRI_ShutdownVocBase();

  return res;
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\)"
// End:
