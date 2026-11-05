# Build both demos. Each directory also has its own makefile.
DIRS = hello messaging

all:
	@for d in $(DIRS); do $(MAKE) --no-print-directory -C $$d; done

clean:
	@for d in $(DIRS); do $(MAKE) --no-print-directory -C $$d clean; done

.PHONY: all clean $(DIRS)
