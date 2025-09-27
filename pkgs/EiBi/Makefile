
# See HOWTO.update.database.txt

ABYY := a25

include ../../Makefile.comp.inc

PROG = eibi_proc

ARCH = sitara
PLATFORM = beaglebone_black
PKGS = pkgs/mongoose pkgs/jsmn pkgs/parson pkgs/sha256
GPS = gps gps/ka9q-fec gps/GNSS-SDRLIB
DIRS = . $(PKGS) web extensions
DIRS += platform/common platform/beaglebone platform/$(PLATFORM) $(EXT_DIRS) rx rx/CuteSDR rx/csdr $(GPS) cfg dx net ui support arch arch/$(ARCH)
DIRS += ../build/gen

VPATH = $(addprefix ../../,$(DIRS))
I = $(addprefix -I../../,$(DIRS)) -I/usr/local/include

all: $(PROG)

$(PROG): $(PROG).o
	$(CPP) $(CFLAGS) $(I) -o $@ $?

%.o: %.cpp
	$(CPP) $(CFLAGS) $(I) -c $<

fetch all:
	wget -O /tmp/eibi http://eibispace.de/dx/README.TXT       && iconv -f ISO-8859-1 -t UTF-8 /tmp/eibi > README.txt
	wget -O /tmp/eibi http://eibispace.de/dx/sked-$(ABYY).csv && iconv -f ISO-8859-1 -t UTF-8 /tmp/eibi > eibi.csv
	@echo "VERIFY: There should be matches for Rádio Clube do Pará below"
	@grep 'Rádio Clube do Pará' eibi.csv
	cat eibi.csv ALE.kiwi.csv > sked-current.csv.tmp
	sort -t ';' -s -n -k 1 sked-current.csv.tmp > sked-current.csv
	cp ../../dx/EiBi.h EiBi.h.prev
	make run

# always reads from sked-current.csv
# always writes file ../../dx/EiBi.h
run: $(PROG)
	./$(PROG) $(ABYY)
	@echo "VERIFY: There should be matches for Rádio Clube do Pará below"
	@grep 'Rádio Clube do Pará' ../../dx/EiBi.h
	wc ../../dx/EiBi.h EiBi.h.prev

clean:
	rm -f $(PROG) *.o *.d eibi.csv sked-current.* EiBi.h.prev README.txt
