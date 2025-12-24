TARGETS := arpresponder trafficmonitor

CC := gcc
LD := gcc
CFLAGS := -Os
LDFLAGS := $(CFLAGS)
STRIP := strip

all: $(TARGETS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

arpresponder: common.o arpresponder.o
	$(LD) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

trafficmonitor: common.o trafficmonitor.o
	$(LD) $(LDFLAGS) -o $@ $^
	$(STRIP) $@

clean:
	rm -f $(TARGETS) *.o
