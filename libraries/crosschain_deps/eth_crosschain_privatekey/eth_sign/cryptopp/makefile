install:
        $(MKDIR) -p $(PREFIX)/include/cryptopp $(PREFIX)/lib $(PREFIX)/bin
        $(CP) *.h $(PREFIX)/include/cryptopp
        $(CP) *.a $(PREFIX)/lib
        $(CP) *.so $(PREFIX)/lib
        $(CP) *.exe $(PREFIX)/bin

libcryptopp.a: $(LIBOBJS)
        $(AR) $(ARFLAGS) $@ $(LIBOBJS)
        $(RANLIB) $@
libcryptopp.so: $(LIBOBJS)
        $(CXX) -shared -o $@ $(LIBOBJS)
