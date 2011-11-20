////////////////////////////////////////////////////////////////////////////////
/// @brief associative array for POD data
///
/// @file
/// Implementation of associative arrays for POD data. The description
/// about hashing and equality test must be passed as description
/// structure. This structure must define the following methods:
///
///   clearElement(ELEMENT&)
///   deleteElement (only required if clearAndDelete is used)
///   hashElement(ELEMENT const&)
///   hashKey(KEY const&)
///   isEmptyElement(ELEMENT const&)
///   isEqualElementElement(ELEMENT const&, ELEMENT const&)
///   isEqualKeyElement(KEY const&, ELEMENT const&)
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
/// @author Martin Schoenert
/// @author Copyright 2006-2011, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_BASICS_ASSOCIATIVE_ARRAY_H
#define TRIAGENS_BASICS_ASSOCIATIVE_ARRAY_H 1

#include <Basics/Common.h>

namespace triagens {
  namespace basics {

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief default fillup
    ////////////////////////////////////////////////////////////////////////////////

    struct ExtendAtFillup {
      template<typename T>
      static bool handle (T*) {
        return false;
      }
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @ingroup Utilities
    /// @brief associative array for POD data
    ///
    /// An associative array for POD data. You must use map or hash_map if you
    /// want to store real objects. The associative array stores elements of a given
    /// type. An element must contain its key. There is no seperate buffer for
    /// keys. The description describes how to generate the hash value for keys
    /// and elements, how to compare keys and elements, and how to check for empty
    /// elements.
    ////////////////////////////////////////////////////////////////////////////////

    template <typename KEY, typename ELEMENT, typename DESC, typename FUH = ExtendAtFillup>
    class AssociativeArray {
      private:
        AssociativeArray (AssociativeArray const&);
        AssociativeArray& operator= (AssociativeArray const&);

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new associative array for POD data
        ////////////////////////////////////////////////////////////////////////////////

        explicit AssociativeArray (uint64_t size) :
          _desc() {
          initialise(size);
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new associative array for POD data
        ////////////////////////////////////////////////////////////////////////////////

        AssociativeArray (uint64_t size, const DESC& desc) :
          _desc(desc) {
          initialise(size);
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief deletes a associative array for POD data
        ////////////////////////////////////////////////////////////////////////////////

        ~AssociativeArray () {
          delete[] _table;
        }

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief swaps two dictonaries
        ////////////////////////////////////////////////////////////////////////////////

        void swap (AssociativeArray* other) {
          DESC tmpDesc = _desc;
          _desc = other->_desc;
          other->_desc = tmpDesc;

          uint64_t tmpInt = _nrAlloc;
          _nrAlloc = other->_nrAlloc;
          other->_nrAlloc = tmpInt;

          tmpInt = _nrUsed;
          _nrUsed = other->_nrUsed;
          other->_nrUsed = tmpInt;

          tmpInt = _nrFinds;
          _nrFinds = other->_nrFinds;
          other->_nrFinds = tmpInt;

          tmpInt = _nrAdds;
          _nrAdds = other->_nrAdds;
          other->_nrAdds = tmpInt;

          tmpInt = _nrRems;
          _nrRems = other->_nrRems;
          other->_nrRems = tmpInt;

          tmpInt = _nrRems;
          _nrRems = other->_nrRems;
          other->_nrRems = tmpInt;

          tmpInt = _nrResizes;
          _nrResizes = other->_nrResizes;
          other->_nrResizes = tmpInt;

          tmpInt = _nrProbesF;
          _nrProbesF = other->_nrProbesF;
          other->_nrProbesF = tmpInt;

          tmpInt = _nrProbesA;
          _nrProbesA = other->_nrProbesA;
          other->_nrProbesA = tmpInt;

          tmpInt = _nrProbesD;
          _nrProbesD = other->_nrProbesD;
          other->_nrProbesD = tmpInt;

          tmpInt = _nrProbesR;
          _nrProbesR = other->_nrProbesR;
          other->_nrProbesR = tmpInt;

          ELEMENT* tmpTable = _table;
          _table = other->_table;
          other->_table = tmpTable;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns number of elements
        ////////////////////////////////////////////////////////////////////////////////

        uint64_t size () const {
          return _nrUsed;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns the capacity
        ////////////////////////////////////////////////////////////////////////////////

        uint64_t capacity () const {
          return _nrAlloc;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns element table
        ////////////////////////////////////////////////////////////////////////////////

        ELEMENT const * tableAndSize (size_t& size) const {
          size = _nrAlloc;

          return _table;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief clears the array
        ////////////////////////////////////////////////////////////////////////////////

        void clear () {
          delete[] _table;
          initialise(_nrAlloc);
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief clears the array and deletes the elements
        ////////////////////////////////////////////////////////////////////////////////

        void clearAndDelete () {
          for (uint64_t i = 0; i < _nrAlloc; i++) {
            _desc.deleteElement(_table[i]);
          }

          delete[] _table;
          initialise(_nrAlloc);
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief finds an element with a given key
        ////////////////////////////////////////////////////////////////////////////////

        ELEMENT const& findKey (KEY const& key) const {

          // update statistics
          _nrFinds++;

          // compute the hash
          uint32_t hash = _desc.hashKey(key);

          // search the table
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i]) && !_desc.isEqualKeyElement(key, _table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesF++;
          }

          // return whatever we found
          return _table[i];
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief finds a given element
        ////////////////////////////////////////////////////////////////////////////////

        ELEMENT const& findElement (ELEMENT const& element) const {

          // update statistics
          _nrFinds++;

          // compute the hash
          uint32_t hash = _desc.hashElement(element);

          // search the table
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i]) && !_desc.isEqualElementElement(element, _table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesF++;
          }

          // return whatever we found
          return _table[i];
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief adds a new element
        ////////////////////////////////////////////////////////////////////////////////

        bool addElement (ELEMENT const& element, bool overwrite = true) {

          // update statistics
          _nrAdds++;

          // search the table
          uint32_t hash = _desc.hashElement(element);
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i]) && !_desc.isEqualElementElement(element, _table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesA++;
          }

          // if we found an element, return
          if (!_desc.isEmptyElement(_table[i])) {
            if (overwrite) {
              memcpy(&_table[i], &element, sizeof(ELEMENT));
            }

            return false;
          }

          // add a new element to the associative array
          memcpy(&_table[i], &element, sizeof(ELEMENT));
          _nrUsed++;

          // if we were adding and the table is more than half full, extend it
          if (_nrAlloc < 2 * _nrUsed) {
            if (FUH::handle(this)) {
              return true;
            }

            ELEMENT * oldTable = _table;
            uint64_t oldAlloc = _nrAlloc;

            _nrAlloc = 2 * _nrAlloc + 1;
            _nrUsed = 0;
            _nrResizes++;

            _table = new ELEMENT[_nrAlloc];

            for (uint64_t j = 0; j < _nrAlloc; j++) {
              _desc.clearElement(_table[j]);
            }

            for (uint64_t j = 0; j < oldAlloc; j++) {
              if (!_desc.isEmptyElement(oldTable[j])) {
                addNewElement(oldTable[j]);
              }
            }

            delete[] oldTable;
          }

          return true;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief adds a new element with key
        ////////////////////////////////////////////////////////////////////////////////

        bool addElement (KEY const& key, ELEMENT const& element, bool overwrite = true) {

          // update statistics
          _nrAdds++;

          // search the table
          uint32_t hash = _desc.hashKey(key);
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i]) && !_desc.isEqualKeyElement(key, _table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesA++;
          }

          // if we found an element, return
          if (!_desc.isEmptyElement(_table[i])) {
            if (overwrite) {
              memcpy(&_table[i], &element, sizeof(ELEMENT));
            }

            return false;
          }

          // add a new element to the associative array
          memcpy(&_table[i], &element, sizeof(ELEMENT));
          _nrUsed++;

          // if we were adding and the table is more than half full, extend it
          if (_nrAlloc < 2 * _nrUsed) {
            if (FUH::handle(this)) {
              return true;
            }

            ELEMENT * oldTable = _table;
            uint64_t oldAlloc = _nrAlloc;

            _nrAlloc = 2 * _nrAlloc + 1;
            _nrUsed = 0;
            _nrResizes++;

            _table = new ELEMENT[_nrAlloc];

            for (uint64_t j = 0; i < _nrAlloc; i++) {
              _desc.clearElement(_table[j]);
            }

            for (uint64_t j = 0; i < oldAlloc; i++) {
              if (!_desc.isEmptyElement(oldTable[j])) {
                addNewElement(oldTable[j]);
              }
            }

            delete[] oldTable;
          }

          return true;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief removes a key
        ////////////////////////////////////////////////////////////////////////////////

        ELEMENT removeKey (KEY const& key) {

          // update statistics
          _nrRems++;

          // search the table
          uint32_t hash = _desc.hashKey(key);
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i]) && !_desc.isEqualKeyElement(key, _table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesD++;
          }

          // if we did not find such an item
          if (_desc.isEmptyElement(_table[i])) {
            return _table[i];
          }

          // return found element
          ELEMENT element = _table[i];

          // remove item
          _desc.clearElement(_table[i]);
          _nrUsed--;

          // and now check the following places for items to move here
          uint64_t k = (i + 1) % _nrAlloc;

          while (!_desc.isEmptyElement(_table[k])) {
            uint32_t j = _desc.hashElement(_table[k]) % _nrAlloc;

            if ((i < k && !(i < j && j <= k)) || (k < i && !(i < j || j <= k))) {
              _table[i] = _table[k];
              _desc.clearElement(_table[k]);
              i = k;
            }

            k = (k + 1) % _nrAlloc;
          }

          // return success
          return element;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief removes an element
        ////////////////////////////////////////////////////////////////////////////////

        bool removeElement (ELEMENT const& element) {

          // update statistics
          _nrRems++;

          // search the table
          uint32_t hash = _desc.hashElement(element);
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i]) && !_desc.isEqualElementElement(element, _table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesD++;
          }

          // if we did not find such an item return false
          if (_desc.isEmptyElement(_table[i])) {
            return false;
          }

          // remove item
          _desc.clearElement(_table[i]);
          _nrUsed--;

          // and now check the following places for items to move here
          uint64_t k = (i + 1) % _nrAlloc;

          while (!_desc.isEmptyElement(_table[k])) {
            uint32_t j = _desc.hashElement(_table[k]) % _nrAlloc;

            if ((i < k && !(i < j && j <= k)) || (k < i && !(i < j || j <= k))) {
              _table[i] = _table[k];
              _desc.clearElement(_table[k]);
              i = k;
            }

            k = (k + 1) % _nrAlloc;
          }

          // return success
          return true;
        }

        // -----------------------------------------------------------------------------
        // private methods
        // -----------------------------------------------------------------------------

      private:
        void initialise (uint64_t size) {
          _table = new ELEMENT[size];

          for (uint64_t i = 0; i < size; i++) {
            _desc.clearElement(_table[i]);
          }

          _nrAlloc = size;
          _nrUsed = 0;
          _nrFinds = 0;
          _nrAdds = 0;
          _nrRems = 0;
          _nrResizes = 0;
          _nrProbesF = 0;
          _nrProbesA = 0;
          _nrProbesD = 0;
          _nrProbesR = 0;
        }



        void addNewElement (ELEMENT const& element) {

          // compute the hash
          uint32_t hash = _desc.hashElement(element);

          // search the table
          uint64_t i = hash % _nrAlloc;

          while (!_desc.isEmptyElement(_table[i])) {
            i = (i + 1) % _nrAlloc;
            _nrProbesR++;
          }

          // add a new element to the associative array
          memcpy(&_table[i], &element, sizeof(ELEMENT));
          _nrUsed++;
        }

        // -----------------------------------------------------------------------------
        // private variables
        // -----------------------------------------------------------------------------

      private:
        DESC _desc;

        uint64_t _nrAlloc; // the size of the table
        uint64_t _nrUsed; // the number of used entries
        ELEMENT * _table; // the table itself

        mutable uint64_t _nrFinds; // statistics
        mutable uint64_t _nrAdds; // statistics
        mutable uint64_t _nrRems; // statistics
        mutable uint64_t _nrResizes; // statistics

        mutable uint64_t _nrProbesF; // statistics
        mutable uint64_t _nrProbesA; // statistics
        mutable uint64_t _nrProbesD; // statistics
        mutable uint64_t _nrProbesR; // statistics
    };
  }
}

#endif
