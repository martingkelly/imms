/*************************************************************************/
/*                                                                       */
/*  Regexx - Regular Expressions C++ solution.                           */
/*                                                                       */
/*  http://projects.nn.com.br/                                           */
/*                                                                       */
/*  Copyright (C) 2000 Gustavo Niemeyer <gustavo@nn.com.br>              */
/*                                                                       */
/*  This library is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU Library General Public          */
/*  License as published by the Free Software Foundation; either         */
/*  version 2 of the License, or (at your option) any later version.     */
/*                                                                       */
/*  This library is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  Library General Public License for more details.                     */
/*                                                                       */
/*  You should have received a copy of the GNU Library General Public    */
/*  License along with this library; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/

// $Revision: 1.6 $
// $Date: 2003/10/20 12:22:38 $

#ifndef REGEXX_HH
#define REGEXX_HH

#include "immsconf.h"

#include <string>
#include <vector>
#ifdef HAVE_PCRE_PCRE_H
# include <pcre/pcre.h>
#else
# include <pcre.h>
#endif

using std::string;
using std::vector;
using std::ostream;

namespace regexx {

  class RegexxMatchAtom
  {

  public:

    inline
    RegexxMatchAtom(std::string& _str,
		    std::string::size_type _start,
		    std::string::size_type _length)
      : m_str(_str), m_start(_start), m_length(_length)
    {}

    inline RegexxMatchAtom&
    operator=(const RegexxMatchAtom& _rxxma)
    {
      m_str = _rxxma.m_str;
      m_start = _rxxma.m_start;
      m_length = _rxxma.m_length;
      return *this;
    }

    /// Retrieves the atom string.
    inline std::string
    str() const
    { return m_str.substr(m_start,m_length); }

    /// Returns the position in the original string where the atom starts.
    inline const std::string::size_type&
    start() const
    { return m_start; }

    /// Length of the atom string.
    inline const std::string::size_type&
    length() const
    { return m_length; }

    /// Operator to transform a RegexxMatchAtom into a string.
    inline operator
    std::string() const
    { return m_str.substr(m_start,m_length); }

    /// Operator to compare a RegexxMatchAtom with a string.
    inline bool
    operator==(const std::string& _s) const
    { return (m_str.substr(m_start,m_length)==_s); }

  private:

    std::string &m_str;
    std::string::size_type m_start;
    std::string::size_type m_length;

  };

#if 0 // Breaks *compilation* \w g++ 3.2.3 and -O3
  inline ostream& operator<<(ostream& _o, RegexxMatchAtom& _rxxma)
  {
    return _o << _rxxma.str();
  }
#endif 

  class RegexxMatch
  {

  public:

    inline
    RegexxMatch(std::string& _str,
		std::string::size_type _start,
		std::string::size_type _length)
      : m_str(_str), m_start(_start), m_length(_length)
    {}

    inline RegexxMatch&
    operator=(const RegexxMatch& _rxxm)
    {
      m_str = _rxxm.m_str;
      m_start = _rxxm.m_start;
      m_length = _rxxm.m_length;
      return *this;
    }

    /// Retrieves the match string.
    inline std::string
    str() const
    { return m_str.substr(m_start,m_length); }

    /// Returns the position in the original string where the match starts.
    inline const std::string::size_type&
    start() const
    { return m_start; }

    /// Length of the match string.
    inline const std::string::size_type&
    length() const
    { return m_length; }

    /// Operator to transform a RegexxMatch into a string.
    inline operator
    std::string() const
    { return m_str.substr(m_start,m_length); }

    /// Operator to compare a RegexxMatch with a string.
    inline bool
    operator==(const std::string& _s) const
    { return (m_str.substr(m_start,m_length)==_s); }

    /// Vector of atoms found in this match.
    std::vector<RegexxMatchAtom> atom;

  private:

    std::string &m_str;
    std::string::size_type m_start;
    std::string::size_type m_length;

  };

#if 0 // Breaks *compilation* \w g++ 3.2.3 and -O3
  inline ostream& operator<<(ostream& _o, RegexxMatch& _rxxm)
  {
    return (_o << _rxxm.str());
  }
#endif

  // The main Regexx class.
  class Regexx
  {

  public:

    /// These are the flags you can use with exec() and replace().
    enum flags {

      /// Global searching, otherwise Regexx stops after the first match.
      global  = 1,
      nocase  = 2,
      nomatch = 4,
      noatom  = 8,
      study   = 16,
      newline = 32,
      notbol  = 64,
      noteol  = 128
    };

    class Exception {
    public:
      Exception(const std::string& _message) : m_message(_message) {}
      /// Method to retrieve Exception information.
      inline const std::string& message() { return m_message; }
    private:
      std::string m_message;
    };

    /** This exception is thrown when there are errors while compiling
     *  expressions.
     */
    class CompileException : public Exception {
    public:
      CompileException(const std::string& _message) : Exception(_message) {}
    };

    /// Constructor
    inline
    Regexx()
      : m_compiled(false), m_study(false), m_matches(0), m_extra(NULL)
    {}

    /// Destructor
    inline
    ~Regexx()
    { if(m_compiled) { free((void *)m_preg); if(m_study) free((void *)m_extra); } }

    // Constructor with regular expression execution.
    inline
    Regexx(const std::string& _str, const std::string& _expr, int _flags = 0)
      throw(CompileException)
      : m_compiled(false), m_study(false), m_matches(0), m_extra(NULL)
    { exec(_str,_expr,_flags); }

    // Set the regular expression to use with exec() and replace().
    inline Regexx&
    expr(const std::string& _expr);

    // Set the string to use with exec() and replace().
    inline Regexx&
    str(const std::string& _str);

    // Execute a regular expression.
    const unsigned int&
    exec(int _flags = 0)
      throw(CompileException);

    // Execute a regular expression.
    inline const unsigned int&
    exec(const std::string& _str, const std::string& _expr, int _flags = 0)
      throw(CompileException);

    // Replace string with regular expression.
    const std::string&
    replace(const std::string& _repstr, int _flags = 0)
      throw(CompileException);

    inline const std::string&
    replace(const std::string& _str, const std::string& _expr, 
	    const std::string& _repstr, int _flags = 0)
      throw(CompileException);

    // Customized replace string with regular expression.
    inline const std::string&
    replacef(const std::string& _str, const std::string& _expr,
	     std::string (*_func)(const RegexxMatch&),
	     int _flags = 0)
      throw(CompileException);
    
    /** Returns the number of matches of the last exec()/replace()/replacef().
     */
    inline const unsigned int&
    matches() const
    { return m_matches; }

    inline operator
    unsigned int() const
    { return m_matches; }

    /// Returns the string of the last replace() or replacef().
    inline operator
    std::string() const
    { return m_replaced; }

    // The vector of matches.
    std::vector<RegexxMatch> match;

  private:
    
    bool m_compiled;
    bool m_study;
    std::string m_expr;
    std::string m_str;
    int m_capturecount;
    
    unsigned int m_matches;
    std::string m_replaced;

    const pcre* m_preg;
    const pcre_extra* m_extra;
  };

  inline Regexx&
  Regexx::expr(const std::string& _expr)
  {
    if(m_compiled) {
      free((void *)m_preg);
      m_compiled = false;
      if(m_study) {
	free((void *)m_extra);
	m_study = false;
	m_extra = NULL;
      }
    }
    m_expr = _expr;
    return *this;
  }
  
  inline Regexx&
  Regexx::str(const std::string& _str)
  {
    m_str = _str;
    return *this;
  }
  
  inline const unsigned int&
  Regexx::exec(const std::string& _str, const std::string& _expr, int _flags)
    throw(CompileException)
  {
    str(_str);
    expr(_expr);
    return exec(_flags);
  }

  inline const std::string&
  Regexx::replace(const std::string& _str, const std::string& _expr, 
		  const std::string& _repstr, int _flags)
    throw(CompileException)
  {
    str(_str);
    expr(_expr);
    return replace(_repstr,_flags);
  }

  inline const std::string&
  Regexx::replacef(const std::string& _str, const std::string& _expr,
		   std::string (*_func)(const RegexxMatch&),
		   int _flags)
    throw(CompileException)
  {
    str(_str);
    expr(_expr);
    exec(_flags&~nomatch);
    m_replaced = m_str;
    std::vector<RegexxMatch>::reverse_iterator m;
    for(m = match.rbegin(); m != match.rend(); m++)
      m_replaced.replace(m->start(),m->length(),_func(*m));
    return m_replaced;
  }

}

#endif // REGEXX_HH
