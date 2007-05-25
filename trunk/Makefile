LIB_NAME:= libresolv6
LIB_VERSION:= 1.0.0

SHARED_LIB:= $(LIB_NAME).so
SHARED_OBJS:= resolve.o

TEST_OBJS:= test.o
TEST_APP:= restest

all: $(SHARED_LIB) 

.c.o:
	$(CC) -c $*.c $(CFLAGS)

$(SHARED_LIB): $(SHARED_OBJS) 
	$(LD) $(LDFLAGS) -shared $< -o $@ 

.PHONY: test
test: $(TEST_APP) 
	
$(TEST_APP): $(TEST_OBJS) $(SHARED_LIB)
	$(CC) $(LDFLAGS) -L. -lresolv6 $< -o $@

.PHONY: release 
release: clean
	@(cd ..; \
	rm -r -f $(LIB_NAME)-$(LIB_VERSION); \
	cp -a $(LIB_NAME) $(LIB_NAME)-$(LIB_VERSION); \
	\
	find $(LIB_NAME)-$(LIB_VERSION)/ -type f \
		-name .\#* \
		-print \
		-exec rm -f {} \; ; \
	\
	tar --exclude=.svn -cjf $(LIB_NAME)-$(LIB_VERSION).tar.bz2 $(LIB_NAME)-$(LIB_VERSION)/;)

.PHONY: clean
clean:
	@rm -f *~ *.so *.o $(TEST_APP)
