CXX = gcc
CXXFLAGS = -Wall
TARGETS = timer.out context.out main.out

.PHONY: all clean

all: $(TARGETS)

main: main.c
	$(CXX) $(CXXFLAGS) -o $@ $<

timer: timer.c
	$(CXX) $(CXXFLAGS) -o $@ $<

context: context.c
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)