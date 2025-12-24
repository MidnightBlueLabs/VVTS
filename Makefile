TARGETS := \
	src/arpresponder \
	src/trafficmonitor \
	external/radvd-2.20/radvd \
	external/tayga-0.9.5/tayga \
	external/phpqrcode/phpqrcode.php

.PHONY: all

all: $(TARGETS)

src/arpresponder:
	cd src && make arpresponder

src/trafficmonitor:
	cd src && make trafficmonitor

external/radvd-2.20/radvd:
	cd external && make radvd-2.20/radvd

external/tayga-0.9.5/tayga:
	cd external && make tayga-0.9.5/tayga

external/phpqrcode/phpqrcode.php:
	cd external && make phpqrcode/phpqrcode.php

clean:
	cd src && make clean
	cd external && make clean
