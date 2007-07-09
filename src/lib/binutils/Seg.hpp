// -*-Mode: C++;-*-
// $Id$

// * BeginRiceCopyright *****************************************************
// 
// Copyright ((c)) 2002-2007, Rice University 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// 
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage. 
// 
// ******************************************************* EndRiceCopyright *

//***************************************************************************
//
// File:
//   $Source$
//
// Purpose:
//   [The purpose of this file]
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#ifndef binutils_Seg_hpp 
#define binutils_Seg_hpp

//************************* System Include Files ****************************

#include <string>
#include <deque>
#include <iostream>

//*************************** User Include Files ****************************

#include <include/general.h>
#include <include/gnu_bfd.h>

#include "LM.hpp"

#include <lib/isa/ISATypes.hpp> 

//*************************** Forward Declarations **************************

//***************************************************************************
// Seg
//***************************************************************************

namespace binutils {

class Proc;
class Insn;

// --------------------------------------------------------------------------
// 'Seg' is a base class for representing file segments/sections
// of a 'LoadModule'.
// --------------------------------------------------------------------------

class Seg {
public: 
  enum Type { BSS, Text, Data, Unknown };
  
  Seg(LM* _lm, std::string& name, Type type, VMA beg, VMA end, uint64_t _sz);
  virtual ~Seg();

  LM* GetLM() const { return lm; }

  // Return name and type of section
  const std::string& name() const { return m_name; }
  
  Type  GetType()  const { return m_type; }
  
  // Return begin/end virtual memory address for section: [beg, end).
  // Note that the end of a section is equal to the begin address of
  // the next section (or the end of the file) which is different than
  // the convention used for a 'Proc'.
  VMA GetBeg() const { return m_beg; }
  VMA GetEnd() const { return m_end; }

  // Return size of section
  uint64_t GetSize() const { return m_size; }

  // Return true if virtual memory address 'vma' is within the section
  // WARNING: vma must be unrelocated
  bool IsIn(VMA vma) const { return (m_beg <= vma && vma < m_end); }

  // Convenient wrappers for the 'LM' versions of the same.
  MachInsn* findMachInsn(VMA vma, ushort &sz) const {
    return lm->findMachInsn(vma, sz);
  }
  Insn* findInsn(VMA vma, ushort opIndex) const {
    return lm->findInsn(vma, opIndex);
  }
  bool GetSourceFileInfo(VMA vma, ushort opIndex,
			 std::string& func, std::string& file, 
			 SrcFile::ln& line) const {
    return lm->GetSourceFileInfo(vma, opIndex, func, file, line);
  }

  bool GetSourceFileInfo(VMA begVMA, ushort bOpIndex,
			 VMA endVMA, ushort eOpIndex,
			 std::string& func, std::string& file,
			 SrcFile::ln& begLine, SrcFile::ln& endLine,
			 uint flags = 1) const {
    return lm->GetSourceFileInfo(begVMA, bOpIndex, endVMA, eOpIndex,
				 func, file, begLine, endLine, flags);
  }

  // -------------------------------------------------------
  // debugging
  // -------------------------------------------------------
  // Current meaning of 'flags'
  //   0 : short dump (without instructions)
  //   1 : full dump

  std::string toString(int flags = LM::DUMP_Short, const char* pre = "") const;

  virtual void dump(std::ostream& o = std::cerr, 
		    int flags = LM::DUMP_Short, const char* pre = "") const;
  void ddump() const;
  
protected:
  // Should not be used
  Seg() { } 
  Seg(const Seg& s) { }
  Seg& operator=(const Seg& s) { return *this; }
private:
  
protected:
private:
  LM* lm; // we are not owners

  std::string m_name;
  Type m_type;
  VMA  m_beg;  // beginning of section 
  VMA  m_end;  // end of section [equal to the beginning of next section]
  uint64_t m_size; // size in bytes
};

} // namespace binutils


//***************************************************************************
// TextSeg
//***************************************************************************

namespace binutils {

class TextSegImpl; 


// --------------------------------------------------------------------------
// 'TextSeg' represents a text segment in a 'LM' and
// implements special functionality pertaining to it.
// --------------------------------------------------------------------------

class TextSeg : public Seg { 
public:
  TextSeg(LM* _lm, std::string& name, VMA beg, VMA end,
	  uint64_t size, asymbol** syms, int numSyms, bfd* abfd);
  virtual ~TextSeg();

  uint GetNumProcs() const { return procedures.size(); }

  // -------------------------------------------------------
  // debugging
  // -------------------------------------------------------
  virtual void dump(std::ostream& o = std::cerr, 
		    int flags = LM::DUMP_Short, const char* pre = "") const;
  
  friend class TextSegProcIterator;

private:
  // Should not be used
  TextSeg() { }
  TextSeg(const TextSeg& s) { }
  TextSeg& operator=(const TextSeg& s) { return *this; }

  void Create_InitializeProcs();
  void Create_DisassembleProcs();

  // Construction helpers
  std::string FindProcName(bfd *abfd, asymbol *procSym) const;
  VMA FindProcEnd(int funcSymIndex) const;
  Insn* MakeInsn(bfd *abfd, MachInsn* mi, VMA vma,
		 ushort opIndex, ushort sz) const;
  
  // Proc sequence: 'deque' supports random access iterators (and
  // is thus sortable with std::sort) and constant time insertion/deletion at
  // beginning/end.
  typedef std::deque<Proc*> ProcSeq;
  typedef std::deque<Proc*>::iterator ProcSeqIt;
  typedef std::deque<Proc*>::const_iterator ProcSeqItC;

protected:
private:
  TextSegImpl* impl;
  ProcSeq procedures; // FIXME: we could make this a slice of the procs in LM
};

} // namespace binutils


//***************************************************************************
// TextSegProcIterator
//***************************************************************************

namespace binutils {

// --------------------------------------------------------------------------
// 'TextSegProcIterator': iterate over the 'Proc' in a
// 'TextSeg'.  No order is guaranteed.
// --------------------------------------------------------------------------

class TextSegProcIterator {
public: 
  TextSegProcIterator(const TextSeg& _sec);
  ~TextSegProcIterator();

  // Returns the current object or NULL
  Proc* Current() const {
    if (it != sec.procedures.end()) { return *it; }
    else { return NULL; }
  }
  
  void operator++()    { ++it; } // prefix increment
  void operator++(int) { it++; } // postfix increment

  bool IsValid() const { return it != sec.procedures.end(); } 
  bool IsEmpty() const { return it == sec.procedures.end(); }

  // Reset and prepare for iteration again
  void Reset() { it = sec.procedures.begin(); }

private:
  // Should not be used
  TextSegProcIterator();
  TextSegProcIterator(const TextSegProcIterator& i);
  TextSegProcIterator& operator=(const TextSegProcIterator& i) { return *this; }

protected:
private:
  const TextSeg& sec;
  TextSeg::ProcSeqItC it;
};

} // namespace binutils

//****************************************************************************

#endif // binutils_Seg_hpp
