////////////////////////////////////////////////////////////////////////////////
/// @brief https server
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

#ifndef TRIAGENS_REST_HTTPS_SERVER_H
#define TRIAGENS_REST_HTTPS_SERVER_H 1

#include <Rest/HttpServer.h>

#include <openssl/ssl.h>

namespace triagens {
  namespace rest {

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief http server
    ////////////////////////////////////////////////////////////////////////////////

    class HttpsServer : virtual public HttpServer {
      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief SSL protocol methods
        ////////////////////////////////////////////////////////////////////////////////

        enum protocol_e {
          SSL_V2 = 1,
          SSL_V3 = 2,
          SSL_V23 = 3,
          TLS_V1 = 4
        };

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief creates a SSL context
        ////////////////////////////////////////////////////////////////////////////////

        static SSL_CTX* sslContext (protocol_e, string const& keyfile);

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new http server
        ////////////////////////////////////////////////////////////////////////////////

        HttpsServer* create (Scheduler*, SSL_CTX*);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new http server
        ////////////////////////////////////////////////////////////////////////////////

        HttpsServer* create (Scheduler*, Dispatcher*, SSL_CTX*);

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief sets the verification mode
        ////////////////////////////////////////////////////////////////////////////////

        virtual void setVerificationMode (int mode) = 0;

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief sets the verification callback
        ////////////////////////////////////////////////////////////////////////////////

        virtual void setVerificationCallback (int (*func)(int, X509_STORE_CTX *)) = 0;
    };
  }
}

#endif

