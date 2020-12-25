Internet2=$(shell find . -name 'Internet2-*.txt')

all:	$(Internet2)
	$(foreach file,$(Internet2),python workload_transform.py $(file);)
	$(foreach file,$(Internet2),rm $(file); mv $(file)_out $(file);)