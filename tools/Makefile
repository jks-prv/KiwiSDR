
UTIL = audio
UTILS = cw ansi audio integrate hog multiply ext64 decimate security e1b_fec viterbi27_test e1b_code multiply2 dx_huge

CMD =

ifeq ($(GET_TOOLS_EXCLUDE_RSYNC),true)
    TOOLS_EXCLUDE_RSYNC =  "$(UTIL)" "$(UTIL).o" "$(UTIL).d"
else

include ../Makefile.comp.inc

ifeq ($(UTIL),viterbi27_test)
    MORE = viterbi27_port.o
    ARGS = -l 120 -n 1 -e 10 -g 300
endif

ifeq ($(UTIL),e1b_fec)
    MORE = viterbi.o viterbi27_port.o
endif

ifeq ($(UTIL),decimate)
    CMD = /Applications/baudline.app/Contents/Resources/baudline -quadrature -overlays 2 /Users/jks/new.dec2.au
endif

ifeq ($(UTIL),dx_huge)
    CLEAN = dx_huge.json
endif

ARCH = sitara
PLATFORM = beaglebone_black
PKGS = pkgs/mongoose pkgs/jsmn pkgs/parson

GPS = gps gps/ka9q-fec gps/GNSS-SDRLIB
DIRS = . pru $(PKGS) web extensions
DIRS += platform/beaglebone platform/$(PLATFORM) $(EXT_DIRS) rx rx/CuteSDR rx/csdr $(GPS) init net ui support arch arch/$(ARCH)
DIRS += ../build/gen
VPATH = $(addprefix ../,$(DIRS))
I = $(addprefix -I../,$(DIRS)) -I/usr/local/include

all: $(UTIL)

$(UTIL): $(UTIL).o $(MORE)
	$(CPP) $(CFLAGS) $(I) -o $@ $?

%.o: %.cpp
	$(CPP) $(CFLAGS) $(I) -c $<

run: $(UTIL)
	./$(UTIL) $(ARGS)
	$(CMD)

clean:
	rm -f $(UTILS) *.o *.d *.dat *.xz $(CLEAN)

endif
