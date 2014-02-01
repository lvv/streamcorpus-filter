// splits string into tokens
// part of lvvlib - https://github.com/lvv/lvvlib

				#ifndef LVV_TOKEN_H
				#define LVV_TOKEN_H
#include <assert.h>
#include <cstdint>
	using std::size_t;

#include <unordered_set>
#include <algorithm>

			       	namespace {
	typedef  const char*   	pos_t;
	constexpr char	token_separator = ' ';


//////////////////////////////////////////////////////////////////////////////////////////////////////  UNICODE
					#ifdef UNICODE
	typedef 	wchar_t 	codepoint; 

 	__attribute__((unused)) 
bool  get_codepoint_from_utf8(pos_t& p,  pos_t e, codepoint& cp)  {

	typedef 	uint8_t 	octet;

		constexpr octet     UTF8_CONT = 0x80;
		constexpr octet     UTF8_SEQ2 = 0xc0;
		constexpr octet     UTF8_SEQ3 = 0xe0;
		constexpr octet     UTF8_SEQ4 = 0xf0;
		constexpr octet     UTF8_SEQ5 = 0xf8;
		constexpr octet     UTF8_SEQ6 = 0xfc;
		constexpr codepoint UTF8_BOM  = 0xfeff;

	next_octet:;

	if (p>=e) return false;

	// ignore bad bytes
	switch (*(octet*)p) {
		case 0xc0:
		case 0xc1:
		case 0xf5:
		case 0xff:
			++p;
			goto next_octet;
	}


	// Get number of bytes in sequance
       
	codepoint 	first_octet_data; 
	int		seq_size = 1;

	if  ((*(octet*)p & 0x80) == 0) {
		// seq_size = 1;	
		cp = *(octet*)p++;
		return true;

	} else if ((*(octet*)p & 0xe0) == UTF8_SEQ2) { seq_size = 2; first_octet_data = *(octet*)p & 0x1f; 
	} else if ((*(octet*)p & 0xf0) == UTF8_SEQ3) { seq_size = 3; first_octet_data = *(octet*)p & 0x0f;
	} else if ((*(octet*)p & 0xf8) == UTF8_SEQ4) { seq_size = 4; first_octet_data = *(octet*)p & 0x07; 
	} else if ((*(octet*)p & 0xfc) == UTF8_SEQ5) { seq_size = 5; first_octet_data = *(octet*)p & 0x03; 
	} else if ((*(octet*)p & 0xfe) == UTF8_SEQ6) { seq_size = 6; first_octet_data = *(octet*)p & 0x01; 

	} else {
		//  utf8 error,  skip bad
		++p;  goto next_octet;
	}

	// verify that we have enough bytes
	if (e - p <= seq_size - 1)     return false;
	
	// check that all octets have continuation bits
								assert(seq_size>1);
	for (int i = 1;  i < seq_size;  i++) {
		if ((((octet*)p)[i] & 0xc0) != UTF8_CONT) {
			// utf8 error, skip bad octets
			p += i;   goto next_octet;
		}
	}

	// construct codepoint
	cp = 0;
	int bits_to_shift = 0;		
	for (int i = 1;  i < seq_size;  i++) {
		cp |= (codepoint)(((octet*)p)[seq_size - i] & 0x3f) << bits_to_shift;
		bits_to_shift += 6;
	}

	cp |= first_octet_data << bits_to_shift;
	p += seq_size;

	// check for bad codepoint
	if (cp >= 0xd800  &&  cp <= 0xdfff)	goto next_octet;      // certain cp values are forbidden
	if (cp == UTF8_BOM)			goto next_octet;      // not sure if this is correct

	return true;
 }
					#endif  // UNICODE


// names, token
struct __attribute__ ((aligned(1))) __attribute__ ((packed)) strref {
	//strref  ()                   : b(), e(nullptr)		{};
	strref  (pos_t b, pos_t e)   :  sz(e-b), b((uint64_t)b)			{};
	strref  (const char* cs)					{ b=(uint64_t)cs; sz=0; while(*(cs+sz)) ++sz; };

	size_t size()  const { return sz; };
	pos_t  begin() const { return pos_t(uint64_t(b)); }
	pos_t  end()   const { return pos_t(b+sz); }

	operator std::string () const { return std::string(b,b+sz); };
	bool operator==(const char* p)        const { const char* pe=p;  while(*pe) ++pe;  return  sz == pe-p  &&  std::equal(begin(),end(), p); };
	bool operator==(const std::string& s) const { return  sz == s.size()   &&  std::equal(begin(),end(), s.begin()); };

	unsigned int  sz:16;
	unsigned long  b:48;
};

	__attribute__((unused)) 
std::ostream&   operator<< (std::ostream& os, const strref& s)  {
	return os << "(" << std::string(s.begin(),s.end()) << ")";
};


struct  is_t {

	#ifdef  UNICODE

	char separator_tab[256]={0};
	std::unordered_set<codepoint> separator_set;
	is_t() :
		// '\t', '\n', '\x0b', u'\x0c', u'\r', u'\x1c', u'\x1d', u'\x1e', u'\x1f', u' ', u'\x85', u'\xa0', u'\u1680', u'\u180e', u'\u2000', u'\u2001', u'\u2002', u'\u2003', u'\u2004', u'\u2005', u'\u2006', u'\u2007', u'\u2008', u'\u2009', u'\u200a', u'\u2028', u'\u2029', u'\u202f', u'\u205f', u'\u3000']				
		separator_set {0x85, 0xa0, 0x1680, 0x180e, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200a, 0x2028, 0x2029, 0x202f, 0x205f, 0x3000}
	{
		for (auto c: " \t\n\r-,.:'~`!@#$%^&*()_-+={}[]|\\|:;\"',<>./?"  "\t\n\v\f\r\u001c\u001d\u001e\u001f ") 
			separator_tab[(size_t)c]=1;
	}
	bool token_separator(wchar_t c)  const { return c < 128  ?  (bool)separator_tab[(int)c]  :  separator_set.find(c) != separator_set.end(); }


	#else  // NON-UNICODE
	char separator_tab[256]={0};
	is_t() {
		for (auto c: " \t\n\r-,.:'~`!@#$%^&*()_-+={}[]|\\|:;\"',<>./?") 
			separator_tab[(size_t)c]=1;
	}
	bool token_separator(uint8_t c)  const { return (bool)separator_tab[c]; }

	#endif

};

is_t	is;


#ifdef  UNICODE
pos_t	get_tb(pos_t b, pos_t e)  {
						assert(b<=e);
	codepoint	cp;
	pos_t		p = b;
	pos_t		prev_p = p;

	for (; p<e  &&  get_codepoint_from_utf8(p,e,cp); prev_p = p) {
		if (!is.token_separator(cp))  return prev_p;
	}
	return p;
}
		
		
pos_t	get_te(pos_t b, pos_t e)  {
						assert(b<=e);
	codepoint	cp;
	pos_t		p = b;
	pos_t		prev_p = p;

	for (; p<e  &&  get_codepoint_from_utf8(p,e,cp);  prev_p = p) {
		if (is.token_separator(cp))  return prev_p;
	}
	return p;
}
		
#else   // NON-UNICODE

pos_t	get_tb(pos_t b, pos_t e)  { assert(b<=e);  return std::find_if     (b,e, [](char c){ return !is.token_separator(c); }); };
pos_t	get_te(pos_t b, pos_t e)  { assert(b<=e);  return std::find_if     (b,e, [](char c){ return  is.token_separator(c); }); };

#endif

bool	are_equal_token_chains (pos_t b1, pos_t e1, pos_t b2, pos_t e2)  {

		pos_t   tb1;
		pos_t   te1 = b1;     

		pos_t   tb2;
		pos_t   te2 = b2;     

	while (
		(tb1=get_tb(te1,e1)),
		(tb2=get_tb(te2,e2)),
		tb1!=e1  &&  tb2!=e2  
	) {
		te1=get_te(tb1,e1);
		te2=get_te(tb2,e2);
		if ( !((te1-tb1)==(te2-tb2) && std::equal (tb1,te1, tb2)) )  return false;
	}

	return tb1==e1  &&  tb2==e2;
}


bool	is_head_of (pos_t b1, pos_t e1, pos_t b2, pos_t e2, pos_t& match_e)  {
                                          // (b1,e1) - token chain;  
					  // (b2,e2) - string; 
					  // reuturns true if (b2,e2) starts with (b1,e1) and sets end of match in  match_e
		pos_t   tb1;               
		pos_t   te1 = b1;     

		pos_t   tb2;
		pos_t   te2 = b2;     

	while (
		(tb1=get_tb(te1,e1)),
		(tb2=get_tb(te2,e2)),
		tb1!=e1  &&  tb2!=e2  
	) {
		te1=get_te(tb1,e1);
		te2=get_te(tb2,e2);
		if ( !((te1-tb1)==(te2-tb2) && std::equal (tb1,te1, tb2)) )  return false;
	}

	
	if ( tb1==e1) {			// if 1st ended -> match
		match_e = te2;
		return true;
	}  else {
		return false;
	}
}

bool	are_equal_token_chains(strref n1, strref n2)      { return are_equal_token_chains(n1.begin(), n1.end(),  n2.begin(), n2.end()); }
bool	is_head_of(strref n1, strref n2, pos_t& match_e)  { return is_head_of(n1.begin(), n1.end(),  n2.begin(), n2.end(), match_e); }

	 			} // anon namespace
				#endif  // LVV_CHECK_H
