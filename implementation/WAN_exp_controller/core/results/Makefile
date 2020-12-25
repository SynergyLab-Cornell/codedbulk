ORIGINS=$(shell find . -type f -name 'B4-*')
TARGETS=$(ORIGINS:./B4-%=raw/B4-%/traffic_summary.txt)

all: $(TARGETS)

raw/B4-%/traffic_summary.txt: B4-%
	mv $< $@
