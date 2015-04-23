CC = gcc

CFLAGS = -Wall -O2 -g

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

DEPS = $(patsubst %.o,%.d,$(OBJS))

TARGET= test_epoll_serv

all : $(TARGET)

%.d : %.c
	@set -e;$(CC)  $(CFLAGS) -MM $< | sed -e 's/$(basename $@).o/$(basename $@).o $(basename $@).d/' > $@

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f *.d *.s *.o $(TARGET)

sinclude $(DEPS)
