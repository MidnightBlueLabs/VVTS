TARGETS := tayga-0.9.5/tayga radvd-2.20/radvd phpqrcode/phpqrcode.php

.PHONY: all

all: $(TARGETS)

radvd-2.20.tar.gz:
	wget https://github.com/radvd-project/radvd/releases/download/v2.20/radvd-2.20.tar.gz -O $@
	if ! sha256sum $@ | awk '{print $1}' | grep af37c5a81d59f3bdc00d83056606ffa1810d4550beed6caa4f81181246494220 >/dev/null; then rm -f $@; exit 1; fi

radvd-2.20/configure: radvd-2.20.tar.gz
	tar xvf $<
	touch $@

radvd-2.20/radvd: radvd-2.20/configure
	cd radvd-2.20 && ./configure && make
	touch $@

tayga-0.9.5.tar.gz:
	wget https://github.com/apalrd/tayga/archive/refs/tags/0.9.5.tar.gz -O $@
	if ! sha256sum $@ | awk '{print $1}' | grep d44cc1158f60623d1bcd245f811957a162092c8f5e725489438de12e5500ae49 >/dev/null; then rm -f $@; exit 1; fi

tayga-0.9.5/Makefile: tayga-0.9.5.tar.gz
	tar xvf $<
	cd tayga-0.9.5 && patch -p1 -i ../tayga_flush_stdout.patch
	touch $@

tayga-0.9.5/tayga: tayga-0.9.5/Makefile
	cd tayga-0.9.5 && make
	touch $@

phpqrcode-2010100721_1.1.4.zip:
	wget https://downloads.sourceforge.net/project/phpqrcode/releases/phpqrcode-2010100721_1.1.4.zip -O $@
	if ! sha256sum $@ | awk '{print $1}' | grep 5ad97a2ab62d704f60bd1d55e44e7b5dbfb41212d2ccabecb612b395d151fd8a >/dev/null; then rm -f $@; exit 1; fi

phpqrcode/phpqrcode.php: phpqrcode-2010100721_1.1.4.zip
	unzip $<
	touch $@

clean:
	rm -rf radvd-2.20.tar.gz radvd-2.20 tayga-0.9.5.tar.gz tayga-0.9.5 phpqrcode-2010100721_1.1.4.zip phpqrcode
