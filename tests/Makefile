TOP=..

LIBS=glib-2.0 lunaservice pbnjson_cpp audio-utils media-api audio-utils
INCLUDE=. ../include ../src/utils $(INCLUDE_DIR)/glib-2.0

OBJDIR=objs-$(MACHINE_MODULE)
EXE=$(OBJDIR)/$(TEST)

more_cxxflags := $(addprefix -I, $(INCLUDE)) \
    $(ARCH_CFLAGS) \
	-D_GNU_SOURCE \
	-Wall -Werror

more_ldflags := -L../utils/$(OBJDIR)

cxxflags := $(CXXFLAGS) -fno-exceptions -fno-rtti 
ldflags := $(LDFLAGS)

# Backdoor overide of optimization flags to enable gdb debugging.
ifeq ("$(USE_GDB)","1")
	# Ensures that only local builds can be affected.
	ifneq ("$(ISAUTOBUILD)","1")
		cxxflags := $(shell echo $(CXXFLAGS) | sed -e 's/-O2 -g/-O0 -g3/' )
		ldflags := $(shell echo $(LDFLAGS) | sed -e 's/-O1/-O0 -g3/' )
	endif
endif

libs := $(addprefix -l, $(LIBS))

# for each test, specify here which local file(s) to use
ifeq ($(TEST),vctest)
srcs := voiceCommandTest.cpp
else ifeq ($(TEST),nptest)
srcs := namedPipeVoiceCommandTest.cpp
else ifeq ($(TEST),directrecord)
srcs := directrecordtest.cpp
endif

objs := $(srcs)
objs := $(addprefix $(OBJDIR)/, $(objs))

# grab utility files that aren't part of audio-utils (but should probably be...)
utils := $(wildcard $(TOP)/src/utils/*.cpp)
srcs += $(utils)
utils := $(subst $(TOP),$(OBJDIR),$(utils))
objs += $(utils)

objs := $(objs:.cpp=.o)

$(TEST): $(EXE)

# Main binary
$(EXE): $(objs)
	$(CXX) $(ldflags) $(more_ldflags) -o $@ $^ $(libs)
	@mkdir -p $(TOP)/bin
	@cp $(EXE) $(TOP)/bin

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) -MD -MP -MF $(OBJDIR)/$*.d $(more_cxxflags) $(cxxflags) -o $@ -c $<

# this rule to build utils files
$(OBJDIR)/%.o: $(TOP)/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) -MD -MP -MF $(OBJDIR)/$*.d $(more_cxxflags) $(cxxflags) -o $@ -c $<

-include $(srcs:%.cpp=$(OBJDIR)/%.d)

.PHONY: clean
clean:
	rm -fR $(OBJDIR)
	rm -fR objs-*
