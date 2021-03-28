.PHONY: all homework3 homework4

all: homework3 homework4

homework3:
	cd homework3 && $(MAKE) $(target)

homework4:
	cd homework4 && $(MAKE) $(target)
