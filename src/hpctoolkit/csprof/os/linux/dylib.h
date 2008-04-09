#ifndef dylib_h
#define dylib_h

void dylib_note_dlopen(const char *module_name);


int dylib_find_module_containing_addr(unsigned long long addr, 
				      char *module_name,
				      unsigned long long *start, 
				      unsigned long long *end);

#endif
