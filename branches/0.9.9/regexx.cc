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

// $Revision: 1.1 $
// $Date: 2003/08/04 03:54:01 $

#include "regexx.h"

const unsigned int&
regexx::Regexx::exec(int _flags)
  throw(CompileException)
{
  if(!m_compiled) {
    const char *errptr;
    int erroffset;
    int cflags =
      ((_flags&nocase)?PCRE_CASELESS:0)
      | ((_flags&newline)?PCRE_MULTILINE:0);
    m_preg = pcre_compile(m_expr.c_str(),cflags,&errptr,&erroffset,0);
    if(m_preg == NULL) {
      throw CompileException(errptr);
    }
    pcre_fullinfo(m_preg, NULL, PCRE_INFO_CAPTURECOUNT, (void*)&m_capturecount);
    m_compiled = true;
  }

  if(!m_study && (_flags&study)) {
    const char *errptr;
    m_extra = pcre_study(m_preg, 0, &errptr);
    if(errptr != NULL)
      throw CompileException(errptr);
    m_study = true;
  }

  match.clear();

  int eflags = ((_flags&notbol)?PCRE_NOTBOL:0) | ((_flags&noteol)?PCRE_NOTEOL:0);

  int ssv[33];
  int ssc;
  m_matches = 0;

  ssc = pcre_exec(m_preg,m_extra,m_str.c_str(),m_str.length(),0,eflags,ssv,33);
  bool ret = (ssc > 0);

  if(_flags&global) {
    if(_flags&nomatch)
      while(ret) {
	m_matches++;
	ret = (pcre_exec(m_preg,m_extra,m_str.c_str(),m_str.length(),ssv[1],eflags,ssv,33) > 0);
      }
    else if(_flags&noatom)
      while(ret) {
	m_matches++;
	match.push_back(RegexxMatch(m_str,ssv[0],ssv[1]-ssv[0]));
	ret = (pcre_exec(m_preg,m_extra,m_str.c_str(),m_str.length(),ssv[1],eflags,ssv,33) > 0);
      }
    else
      while(ret) {
	m_matches++;
	match.push_back(RegexxMatch(m_str,ssv[0],ssv[1]-ssv[0]));
	match.back().atom.reserve(m_capturecount);
	for(int i = 1; i < ssc; i++) {
	  if (ssv[i*2] != -1)
	    match.back().atom.push_back(RegexxMatchAtom(m_str,ssv[i*2],ssv[(i*2)+1]-ssv[i*2]));
	  else
	    match.back().atom.push_back(RegexxMatchAtom(m_str,0,0));
        }
	ret = (pcre_exec(m_preg,m_extra,m_str.c_str(),m_str.length(),ssv[1],eflags,ssv,33) > 0);
      }
  }
  else {
    if(_flags&nomatch) {
      if(ret)
	m_matches=1;
    }
    else if(_flags&noatom) {
      if(ret) {
	m_matches=1;
	match.push_back(RegexxMatch(m_str,ssv[0],ssv[1]-ssv[0]));
      }
    }
    else {
      if(ret) {
	m_matches=1;
	match.push_back(RegexxMatch(m_str,ssv[0],ssv[1]-ssv[0]));
	match.back().atom.reserve(m_capturecount);
	for(int i = 1; i < ssc; i++) {
	  if (ssv[i*2] != -1)
	    match.back().atom.push_back(RegexxMatchAtom(m_str,ssv[i*2],ssv[(i*2)+1]-ssv[i*2]));
	  else
	    match.back().atom.push_back(RegexxMatchAtom(m_str,0,0));
	}
	ret = (pcre_exec(m_preg,m_extra,m_str.c_str(),m_str.length(),ssv[1],eflags,ssv,33) > 0);
      }
    }
  }
  return m_matches;
}

const std::string&
regexx::Regexx::replace(const std::string& _repstr, int _flags)
  throw(CompileException)
{
  exec(_flags&~nomatch);
  std::vector< std::pair<unsigned int,std::string::size_type> > v;
  v.reserve(m_capturecount);
  std::string::size_type pos = _repstr.find("%");
  while(pos != std::string::npos) {
    if(_repstr[pos-1] != '%'
       && _repstr[pos+1] >= '0'
       && _repstr[pos+1] <= '9') {
      v.push_back(std::pair<unsigned int,std::string::size_type>(_repstr[pos+1]-'0',pos));
    }
    pos = _repstr.find("%",pos+1);
  }
  m_replaced = m_str;
  std::vector<RegexxMatch>::reverse_iterator m;
  std::vector< std::pair<unsigned int,std::string::size_type> >::reverse_iterator i;
  for(m = match.rbegin(); m != match.rend(); m++) {
    std::string tmprep = _repstr;
    for(i = v.rbegin(); i != v.rend(); i++) {
      if(i->first < m->atom.size())
	tmprep.replace(i->second,2,m->atom[i->first]);
      else
	tmprep.erase(i->second,2);
    }
    m_replaced.replace(m->start(),m->length(),tmprep);
  }
  return m_replaced;
}

