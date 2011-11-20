////////////////////////////////////////////////////////////////////////////////
/// @brief variant class for blobs
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
/// @author Achim Brandt
/// @author Copyright 2008-2011, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_BASICS_VARIANT_BLOB_H
#define TRIAGENS_BASICS_VARIANT_BLOB_H 1

#include <Basics/VariantObject.h>

namespace triagens {
  namespace basics {

    ////////////////////////////////////////////////////////////////////////////////
    /// @ingroup Variants
    /// @brief variant class for blobs
    ////////////////////////////////////////////////////////////////////////////////

    class VariantBlob : public VariantObject {
      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief type of VariantObject
        ////////////////////////////////////////////////////////////////////////////////

        static ObjectType const TYPE = VARIANT_BLOB;

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new blob
        ////////////////////////////////////////////////////////////////////////////////

        VariantBlob ();

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new blob
        ////////////////////////////////////////////////////////////////////////////////

        VariantBlob (char const* pointer, size_t len);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief destructs a blob
        ////////////////////////////////////////////////////////////////////////////////

        ~VariantBlob ();

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        ObjectType type () const {
          return TYPE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        VariantObject* clone () const;

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        void print (StringBuffer&, size_t indent) const;

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns the value
        ////////////////////////////////////////////////////////////////////////////////

        const char* getValue () const {
          return value;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns the length
        ////////////////////////////////////////////////////////////////////////////////

        size_t getLength () const {
          return length;
        }

      private:
        char* value;
        size_t length;
    };
  }
}

#endif
