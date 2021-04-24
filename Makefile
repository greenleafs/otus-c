.PHONY: all homework3 homework4 homework8

all: homework3 homework4 homework8

homework3:
	cd homework3 && $(MAKE) $(target)

homework4:
	cd homework4 && $(MAKE) $(target)

homework8:
	cd homework8 && $(MAKE) $(target)
