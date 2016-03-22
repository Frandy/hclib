PROJECT_CXXFLAGS=-std=c++11 -DWITH_OCR -I$(HCPP_ROOT)/include -I${HCLIB_ROOT}/include
PROJECT_LDFLAGS=-L$(HCPP_ROOT)/lib -L${HCLIB_ROOT}/lib -L${OCR_INSTALL}/lib
PROJECT_LDLIBS=-lhcpp -lhclib -lm -pthread
ifdef TBB_MALLOC
  PROJECT_LDFLAGS+=-L$(TBB_MALLOC)
  PROJECT_LDLIBS+=-ltbbmalloc_proxy
endif
